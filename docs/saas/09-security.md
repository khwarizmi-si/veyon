# 09 — Keamanan

## 9.1 Permukaan ancaman (threat surface)

| Aset | Ancaman |
|------|---------|
| Stream layar siswa | Penyadapan (eavesdrop), akses tak sah |
| Kontrol device | Pengambilalihan PC (RCE) bila agent dibajak |
| Data tenant | Kebocoran lintas-tenant, dump DB |
| Kredensial | Pencurian token/sertifikat, brute force |
| Relay/signaling | DoS, abuse, cross-tenant routing |
| Supply chain | Installer agent dipalsukan/disusupi |

Karena produk ini bisa **mengontrol komputer**, agent yang dibajak = malware. Keamanan = prioritas P0.

## 9.2 Kontrol keamanan inti

**Transport**
- TLS 1.2+ di semua koneksi (HTTPS/WSS). HSTS. Sertifikat valid (Let's Encrypt/ACM).
- **mTLS** untuk agent↔cloud (sertifikat per-device dari enrollment).

**Autentikasi & otorisasi**
- Argon2id untuk password; MFA untuk admin.
- JWT akses pendek + refresh rotation; revoke sesi.
- RBAC + scoping: token sesi guru hanya berlaku untuk device/kelas-nya.
- **PostgreSQL RLS** untuk isolasi tenant di level DB.

**Data**
- Enkripsi at-rest (DB, object storage).
- **E2EE** untuk stream layar (relay tak bisa dekripsi) — penting privasi.
- Rahasia (secret) di **secret manager** (Vault/cloud KMS), bukan di kode/env plaintext.
- Sertifikat device disimpan aman di perangkat (DPAPI Windows / keyring).

**Agent (paling sensitif)**
- Agent hanya menerima perintah **bertanda-tangan** dari cloud untuk tenant & device-nya.
- **Signed binaries** + auto-update terverifikasi (cegah update palsu).
- Prinsip least-privilege: jalankan komponen seminimal hak yang perlu.
- Validasi ketat semua input perintah (cegah command injection).

**Relay/signaling**
- Hanya menyambungkan peer **dalam tenant sama** (otorisasi server-side).
- Rate limiting, quota, proteksi DoS.

## 9.3 Hardening operasional

- Pemindaian dependensi (SCA) & SAST di CI; patuhi update keamanan upstream Veyon.
- Pen-test sebelum rilis produksi & berkala.
- Bug bounty / responsible disclosure (kontak keamanan).
- Pemisahan environment (dev/staging/prod), least-privilege IAM cloud.
- Backup terenkripsi + uji restore; rencana DR.

## 9.4 Audit & deteksi

- **Audit log immutable**: setiap akses layar, perintah, login admin, perubahan config.
- Alerting anomali (login aneh, lonjakan akses, enroll massal).
- Log terpusat (SIEM ringan) + retensi sesuai kebijakan.

## 9.5 Incident response

- Playbook kebocoran data (deteksi → kontain → notifikasi → remediasi).
- **UU PDP mewajibkan notifikasi kebocoran** (umumnya 72 jam) → siapkan proses.
- Kontak keamanan publik (`security@...`) + `SECURITY.md` (Veyon sudah punya, perbarui).

## 9.6 Checklist keamanan minimum sebelum produksi

- [ ] TLS di mana-mana + mTLS agent
- [ ] RLS multi-tenant aktif & diuji (uji kebocoran lintas-tenant)
- [ ] MFA admin + password hashing kuat
- [ ] Secret di KMS/Vault, tidak ada secret di repo
- [ ] Perintah agent bertanda-tangan & tervalidasi
- [ ] Installer & update agent ditandatangani
- [ ] Audit log immutable berjalan
- [ ] Backup terenkripsi + uji restore
- [ ] Pen-test selesai, temuan kritis ditutup
- [ ] Incident response plan + notifikasi kebocoran siap
