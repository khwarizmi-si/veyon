# Device Activation Per PC

Dokumen ini mencatat keputusan desain untuk membatasi penggunaan Khwarizmi Surveillance System per perangkat/PC.

## Keputusan

Pembatasan instalasi dan penggunaan per PC akan memakai kombinasi:

- Device registration
- Server-side activation
- Admin approval
- Kuota device aktif per sekolah/tenant

Installer tetap boleh dipasang di PC, tetapi agent belum dapat digunakan sebelum perangkat terdaftar, disetujui admin, dan memiliki token aktivasi yang valid.

## Alur Utama

1. Agent diinstall di PC.
2. Agent membuat atau membaca `device_id` lokal.
3. Agent mengirim registrasi perangkat ke server.
4. Server menyimpan perangkat dengan status `pending`.
5. Admin melihat perangkat baru di dashboard.
6. Admin menyetujui atau menolak perangkat.
7. Jika disetujui dan kuota masih tersedia, server menerbitkan token aktivasi.
8. Agent menyimpan token aktivasi secara lokal.
9. Agent hanya aktif jika status device `active` dan token valid.
10. Admin dapat melakukan `revoke` kapan saja.

## Status Device

| Status | Arti |
|---|---|
| `pending` | Device sudah registrasi, tetapi belum disetujui admin. |
| `active` | Device disetujui, token valid, dan dihitung dalam kuota. |
| `revoked` | Device dinonaktifkan admin dan tidak boleh connect. |

## Data Minimum Device

Data minimum yang dikirim agent saat registrasi:

- `tenant_id` atau kode sekolah
- `device_id`
- `hostname`
- OS dan versi OS
- versi agent
- fingerprint hardware non-sensitif
- waktu registrasi

Fingerprint sebaiknya dipakai sebagai sinyal pendukung, bukan satu-satunya identitas, karena hardware bisa berubah setelah servis, reinstall OS, atau penggantian komponen.

## Endpoint MVP

Endpoint awal yang dibutuhkan:

- `POST /devices/register`
- `GET /devices`
- `POST /devices/{id}/approve`
- `POST /devices/{id}/revoke`
- `POST /devices/heartbeat`

## Validasi Kuota

Kuota dihitung dari jumlah device dengan status `active` per tenant.

Contoh:

- Sekolah A memiliki kuota 30 device.
- Sudah ada 30 device `active`.
- Device baru boleh masuk `pending`, tetapi tidak bisa di-approve sampai admin revoke device lama atau kuota dinaikkan.

## Token Aktivasi

Untuk MVP, server dapat menerbitkan token aktivasi random yang disimpan agent secara lokal.

Server hanya menyimpan hash token, bukan token plaintext.

Setiap heartbeat agent harus membawa token. Server memvalidasi:

- token cocok
- device masih `active`
- tenant masih aktif
- lisensi/kuota tenant masih valid

## Revoke Dan Pindah PC

Jika PC rusak, hilang, diganti, atau tidak lagi dipakai:

1. Admin revoke device lama.
2. Device lama tidak dapat connect lagi.
3. Device baru install agent dan registrasi sebagai `pending`.
4. Admin approve device baru jika kuota tersedia.

## Penguatan Setelah MVP

Setelah MVP stabil, sistem dapat diperkuat dengan:

- device certificate, bukan token biasa
- rotasi token berkala
- deteksi perubahan fingerprint besar
- audit log approval/revoke
- notifikasi device baru
- policy auto-approve untuk tenant tertentu
- pairing code sekali pakai saat instalasi

## Rekomendasi Implementasi

Mulai dari backend dan dashboard terlebih dahulu:

1. Model/table `devices`
2. Endpoint register
3. Endpoint approve/revoke
4. Validasi kuota tenant
5. Heartbeat agent dengan token aktif
6. UI dashboard untuk daftar pending/active/revoked

Setelah itu agent/installer dibuat mengirim identitas device dan menunggu approval admin.
