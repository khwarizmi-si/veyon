# 05 — Backend & Tech Stack

## 5.1 Prinsip

- **Pisahkan control plane (API biasa) dari media plane (relay).**
- Pilih teknologi yang kamu/tim kuasai > yang "paling hype".
- Mulai **monolith modular**, pecah ke service hanya saat perlu (jangan over-engineer microservices di awal).

## 5.2 Pilihan stack (rekomendasi pragmatis)

| Lapisan | Opsi rekomendasi | Alternatif |
|---------|------------------|-----------|
| Bahasa backend | **Go** (cocok untuk relay/koneksi konkuren) atau **Node/TypeScript** | Python (FastAPI), Rust |
| Framework API | Go: chi/echo/gin; Node: NestJS/Fastify | — |
| Database | **PostgreSQL** (+ RLS multi-tenant) | — |
| Cache/queue/presence | **Redis** | NATS (untuk pub/sub skala) |
| Realtime/signaling | **WebSocket** (Go gorilla/nhooyr, atau Centrifugo) | gRPC streams |
| Media relay (Full) | **Pion (WebRTC, Go)** atau LiveKit (SFU) | coturn (TURN) |
| Object storage | S3 / GCS / Cloudflare R2 | MinIO (self-host) |
| Frontend dashboard | **React/Next.js** atau Vue/Nuxt + Tailwind | SvelteKit |
| Auth | Buat sendiri (OIDC client) atau **Keycloak/Ory/Auth0** | Supabase Auth |
| Billing | **Stripe** (global) / **Midtrans/Xendit** (Indonesia) | Paddle (MoR) |
| IaC | Terraform | Pulumi |
| Container/orchestration | Docker + **Kubernetes** (saat skala) | Nomad, ECS |

> **Catatan Go:** relay & ribuan koneksi WebSocket persisten = sweet spot Go. Karena agent
> (Veyon) C++/Qt, "agent bridge" baru bisa ditulis Go/Rust dan memanggil Veyon lokal.

## 5.3 Layanan (services) & tanggung jawab

| Service | Tanggung jawab |
|---------|----------------|
| **auth-service** | Registrasi, login, SSO/OIDC, MFA, JWT, refresh |
| **tenant-service** | CRUD tenant, lokasi, user, RBAC |
| **device-service** | Enrollment, registry, heartbeat, status, revoke |
| **config-service** | Simpan & push konfigurasi/policy ke agent |
| **command-service** | Kirim perintah fitur (lock, message, run) → agent via signaling |
| **signaling-service** | WebSocket hub: presence + delivery perintah + setup sesi media |
| **relay/media-service** (Full) | Salurkan stream layar antar agent↔master |
| **license-service** | Aktivasi, kuota, masa berlaku, penegakan |
| **billing-service** | Integrasi pembayaran, langganan, invoice, metering |
| **audit-service** | Catat aktivitas & jejak keamanan (immutable) |

MVP: gabung beberapa ini dalam **1 monolith** (auth+tenant+device+config+command+license)
+ **1 service signaling** terpisah (karena pola koneksi beda).

## 5.4 Desain API (contoh endpoint REST)

```
POST   /v1/auth/login                 login user
POST   /v1/auth/sso/callback          callback OIDC
GET    /v1/tenants/me                 info tenant
POST   /v1/devices/enroll             agent enroll (token → cert)
POST   /v1/devices/{id}/heartbeat     agent heartbeat
GET    /v1/devices                    daftar device (filtered tenant)
POST   /v1/devices/{id}/revoke        cabut device
GET    /v1/locations                  daftar lokasi/kelas
POST   /v1/sessions                   mulai sesi kelas → dapat token sesi
POST   /v1/commands                   kirim perintah {device_ids, action, args}
GET    /v1/config                     agent ambil config
POST   /v1/billing/checkout           buat sesi checkout
```

Signaling (WebSocket): `wss://signal.khwarizmi.app/agent` & `/master` dengan auth token,
pesan JSON `{type, payload}` (heartbeat, command, stream-offer/answer, dll).

## 5.5 Realtime & antrian

- **Presence/online:** Redis (set device online + TTL via heartbeat).
- **Perintah → agent:** pub/sub (Redis/NATS) ke node signaling yang memegang koneksi agent.
- **Job async** (laporan, email, cleanup): worker queue (Redis/Asynq, atau BullMQ untuk Node).

## 5.6 Strategi versi & kompatibilitas agent

- Endpoint `/v1/...` berversi; jaga kompatibilitas mundur (agent lama tetap jalan).
- Agent kirim `agent_version`; cloud bisa minta update (auto-update terkelola).
- Pisahkan **versi protokol** dari versi aplikasi.
