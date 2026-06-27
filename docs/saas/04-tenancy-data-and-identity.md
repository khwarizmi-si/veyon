# 04 — Multi-Tenancy, Model Data & Identitas

## 4.1 Multi-tenancy

Setiap **sekolah/organisasi = 1 tenant**. Isolasi data antar tenant adalah keharusan mutlak.

**Strategi isolasi (pilih sesuai skala):**

| Strategi | Cara | Cocok untuk |
|----------|------|-------------|
| Shared DB + `tenant_id` (row-level) | Semua tabel punya kolom `tenant_id`, difilter di query/RLS | Awal, hemat, mayoritas SaaS |
| Schema per tenant | 1 schema PostgreSQL per tenant | Skala menengah, isolasi lebih kuat |
| DB per tenant | 1 database per tenant | Enterprise, regulasi ketat |

**Rekomendasi MVP:** shared DB + `tenant_id` + **PostgreSQL Row-Level Security (RLS)** agar
kebocoran lintas-tenant dicegah di level DB, bukan hanya aplikasi.

## 4.2 Model data (entitas inti)

```
Tenant (sekolah)
  ├─ User (admin sekolah, guru)         ── peran via Role
  ├─ Location (gedung/lab/kelas)
  │     └─ Device (PC murid)
  │           └─ DeviceSession (riwayat online)
  ├─ ComputerGroup (grup untuk kontrol)
  ├─ License (paket, kuota device, masa berlaku)
  ├─ Policy / Config (access control, pengaturan)
  ├─ ClassSession (sesi mengajar: guru, kelas, waktu)
  │     └─ ActivityLog (aksi: lock, view, message, dst — untuk audit)
  └─ AuditLog (jejak admin/keamanan)
```

**Tabel kunci (ringkas):**

| Entitas | Field penting |
|---------|---------------|
| `tenants` | id, name, domain, plan, status, created_at |
| `users` | id, tenant_id, email, role, sso_subject, mfa_enabled |
| `devices` | id, tenant_id, location_id, hostname, os, agent_version, cert_fingerprint, last_seen, status |
| `locations` | id, tenant_id, name, parent_id |
| `licenses` | id, tenant_id, plan, device_quota, seats, valid_until |
| `class_sessions` | id, tenant_id, teacher_id, location_id, started_at, ended_at |
| `activity_logs` | id, tenant_id, session_id, device_id, action, actor, timestamp |

## 4.3 Identitas & autentikasi pengguna

**Pengguna (guru/admin) — login ke cloud:**
- Email + password (Argon2/bcrypt) + **MFA** untuk admin.
- **SSO** sangat penting di sekolah:
  - **Google Workspace for Education** (OAuth/OIDC) — paling umum
  - **Microsoft Entra ID / Microsoft 365 Education** (OIDC)
  - SAML untuk institusi besar
- **RBAC:** Super Admin (kamu), Tenant Admin (admin sekolah), Teacher, (opsional) Observer.

**Token sesi:** JWT akses (pendek) + refresh token (rotasi), simpan refresh di httpOnly cookie.

## 4.4 Identitas device (ganti key-file LAN)

Veyon pakai key file manual. Untuk SaaS, ganti dengan **enrollment berbasis cloud**:

```
1. Admin sekolah generate "enrollment token" (per-lokasi, masa berlaku terbatas)
2. Installer agent dibekali token → saat pertama jalan:
      agent → POST /enroll {token, hostname, os, hw-id}
      cloud → verifikasi token → terbitkan device-id + sertifikat klien (mTLS) / device-secret
3. Agent simpan sertifikat aman (DPAPI di Windows / keyring) → dipakai untuk semua koneksi
4. Admin bisa revoke device kapan saja (cabut sertifikat → agent ditolak)
```

**Keunggulan vs key-file manual:**
- Tidak perlu menyalin key ke tiap PC manual.
- Bisa revoke per-device.
- Identitas terikat tenant → cegah cross-tenant.
- Audit: tahu device mana, kapan enroll, versi agent.

## 4.5 Otorisasi akses kelas (siapa boleh kontrol PC mana)

- Guru hanya boleh kontrol device di **lokasi/kelas** yang ditugaskan padanya.
- Diberlakukan di **cloud** (control plane mengeluarkan token sesi terbatas-scope) **dan** di agent
  (agent hanya menerima sesi yang ditandatangani cloud untuk tenant & device-nya).
- Ganti model "access group = BUILTIN\Administrators" (yang menyusahkan di Docker) dengan
  **otorisasi berbasis peran cloud** — jauh lebih cocok untuk SaaS.

## 4.6 Provisioning & onboarding tenant

```
Sekolah daftar → verifikasi email/domain → buat tenant → pilih paket (trial)
   → buat lokasi/kelas → unduh installer + enrollment token
   → deploy agent ke PC murid (manual / GPO / MDM / script)
   → undang guru (email/SSO) → siap pakai
```
