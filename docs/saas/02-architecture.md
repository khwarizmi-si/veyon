# 02 — Arsitektur Sistem

## 2.1 Pemisahan bidang (planes)

Pisahkan sistem menjadi tiga bidang agar skalabel & aman:

| Plane | Fungsi | Sensitivitas trafik |
|-------|--------|---------------------|
| **Control plane** | Akun, tenant, device, lisensi, config, perintah | Metadata (ringan) |
| **Data/Media plane** | Streaming layar, input, file (hanya Full SaaS) | Berat, real-time |
| **Management UI** | Dashboard web, portal admin | Request/response |

Pemisahan ini penting: control plane bisa pakai DB + API biasa; media plane butuh infra relay
terpisah yang skalabel. Mencampur keduanya = sulit di-scale.

## 2.2 Arsitektur Light SaaS (MVP)

```
                         ┌──────────────────── CLOUD ────────────────────┐
                         │                                                │
   Browser (admin/guru)──┼─► Web Dashboard (SPA) ─► API Gateway           │
                         │                              │                 │
                         │                ┌─────────────┼─────────────┐   │
                         │                ▼             ▼             ▼   │
                         │           Auth/SSO     Control API    Billing  │
                         │                │             │                 │
                         │                └──────┬──────┘                 │
                         │                       ▼                        │
                         │             PostgreSQL  +  Redis (cache/queue)  │
                         │                       │                        │
                         └───────────────────────┼────────────────────────┘
                                                  │ HTTPS (poll/websocket)
                                                  │ config, lisensi, telemetri
              ┌───────────────────────────────────┼─────────────── LAN SEKOLAH ──┐
              │                                    ▼                              │
              │   Connector sekolah (opsional) / Master langsung                 │
              │        │                                                          │
              │        ▼   kontrol layar (VNC, tetap LAN)                          │
              │   PC Murid (Khwarizmi agent/service)                              │
              └──────────────────────────────────────────────────────────────────┘
```

**Alur Light SaaS:**
1. Admin sekolah daftar → buat tenant → unduh installer ber-enrollment-token.
2. Agent di tiap PC murid **dial-out** ke cloud untuk: registrasi device, ambil config, kirim
   heartbeat/telemetri, cek lisensi.
3. Guru buka master (web atau desktop) → login cloud → master ambil daftar komputer & config
   dari cloud → **kontrol layar tetap lewat LAN**.

> Catatan: "Connector sekolah" = satu mesin/container kecil per sekolah yang menjembatani cloud
> ↔ LAN bila master berbasis web. Bila master desktop, bisa langsung.

## 2.3 Arsitektur Full SaaS (Relay)

```
   PC Murid (agent)                Cloud                      Master (web)
   ─────────────────         ─────────────────────         ──────────────
   dial-out (wss/TLS) ─────► Signaling Service ◄──────────── dial-out (wss)
        │                          │ matchmaking                │
        │                          ▼                            │
        └──── media (VNC/WebRTC) ─► Relay/Media Server ◄─────────┘
                                   (pipe stream, per-region)
```

Lihat dok 03 untuk detail desain relay, protokol, dan skalabilitas.

## 2.4 Komponen utama (daftar)

**Cloud:**
- API Gateway / Ingress (TLS termination, rate-limit)
- Auth service (akun, sesi, SSO, MFA)
- Control API (tenant, device, classroom, config, command)
- Device registry & heartbeat collector
- Licensing service
- Billing service (integrasi Stripe/Midtrans)
- (Full) Signaling service + Relay/Media servers
- Web dashboard (SPA)
- PostgreSQL (data), Redis (cache/queue/presence), Object storage (installer, materi, screenshot)
- Observability (logs, metrics, traces)

**Edge / device:**
- Khwarizmi agent (= Veyon service, dimodifikasi untuk dial-out + enrollment cloud)
- Connector sekolah (opsional, untuk Light SaaS berbasis web)

## 2.5 Hubungan dengan kode Veyon yang ada

| Kebutuhan SaaS | Basis di Veyon | Pekerjaan |
|----------------|----------------|-----------|
| API perintah | `plugins/webapi` (REST + qthttpserver) | Perluas, amankan, multi-tenant |
| Identitas device | `authkeys` (key file) | Ganti/perluas: token/sertifikat dari cloud |
| Config terpusat | `VeyonConfiguration` (JSON/local store) | Tambah backend "cloud config" |
| Koneksi | `VncConnection`, `VeyonConnection`, `VncProxyServer` | Tambah mode dial-out/relay |
| Direktori komputer | `NetworkObjectDirectory` (builtin/LDAP) | Tambah plugin direktori "cloud" |
| Access control | `AccessControlProvider` | Integrasikan dgn identitas cloud |

> Strategi rekayasa: sebisa mungkin **bungkus** (wrap) Veyon lewat plugin & API, bukan
> mengubah inti — agar tetap mudah merge update upstream & menjaga batas lisensi GPL (lihat dok 07).
