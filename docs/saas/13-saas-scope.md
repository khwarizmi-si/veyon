# Scope SaaS Khwarizmi

Dokumen ini mencatat keputusan awal tentang fitur apa saja yang akan dijadikan SaaS untuk Khwarizmi Surveillance System.

## Prinsip Produk

Khwarizmi SaaS sebaiknya dimulai sebagai **control plane SaaS**, bukan langsung full streaming layar lewat cloud.

Artinya, cloud dipakai dulu untuk:

- akun sekolah dan user
- lisensi
- aktivasi device
- inventaris perangkat
- konfigurasi
- audit
- dashboard status

Fitur berat seperti thumbnail relay dan remote control via cloud masuk tahap lanjut setelah pondasi SaaS stabil.

## MVP SaaS

Fitur yang paling realistis untuk MVP:

1. Multi sekolah/tenant
2. Admin sekolah dan guru/operator
3. Device registration dan admin approval
4. Kuota PC aktif per tenant
5. Inventaris perangkat
6. Lisensi/subscription
7. Konfigurasi cloud
8. Audit log
9. Dashboard status online/offline

## Detail MVP

### Multi Sekolah/Tenant

Setiap sekolah menjadi tenant terpisah. Data device, user, lisensi, konfigurasi, dan audit log harus terisolasi per tenant.

### Admin Dan Guru/Operator

Role awal:

- owner/admin sekolah
- guru/operator

Admin mengelola device, user, kuota, dan konfigurasi. Guru/operator memakai aplikasi sesuai akses lab atau perangkat yang diberikan.

### Device Registration Dan Approval

Agent yang diinstall di PC akan registrasi ke cloud sebagai `pending`. Admin harus approve sebelum device aktif.

Detail desain ada di [Device Activation Per PC](12-device-activation.md).

### Kuota PC Aktif

Kuota dihitung dari jumlah device dengan status `active`. Device `pending` tidak dihitung sebagai device aktif, tetapi tidak bisa digunakan sebelum approve.

### Inventaris Perangkat

Dashboard cloud menampilkan:

- nama device
- hostname
- OS
- versi agent
- lokasi/lab
- status online/offline
- waktu terakhir aktif
- status approval

### Lisensi/Subscription

Subscription sekolah menentukan:

- batas device aktif
- masa aktif layanan
- fitur yang tersedia
- status billing/manual payment

Untuk MVP, billing boleh manual terlebih dahulu. Sistem cukup menyimpan status subscription dan tanggal expired.

### Konfigurasi Cloud

Cloud menyimpan konfigurasi yang dapat diambil agent/master, misalnya:

- identitas tenant
- lokasi/lab
- policy fitur
- update interval
- flag fitur aktif/nonaktif

### Audit Log

Event minimum yang dicatat:

- user login
- device register
- device approve
- device revoke
- perubahan kuota/lisensi
- perubahan konfigurasi
- command penting yang dijalankan

### Dashboard Status

Dashboard menampilkan ringkasan:

- total device aktif
- total device pending
- total device revoked
- device offline lama
- device versi lama
- kuota terpakai
- status subscription

## Tahap Setelah MVP

Fitur yang masuk tahap lanjutan:

1. Remote command ringan
2. Alert/notifikasi
3. Laporan penggunaan
4. Screenshot on demand
5. Thumbnail relay
6. Remote control via cloud

## Remote Command Ringan

Command yang bisa dikirim via cloud:

- lock
- text message
- shutdown
- reboot
- refresh config
- update agent

Command ini lebih ringan daripada streaming layar dan cocok menjadi fitur SaaS tahap menengah.

## Alert Dan Notifikasi

Contoh alert:

- device offline terlalu lama
- kuota hampir penuh
- subscription hampir expired
- agent versi lama
- device baru menunggu approval

## Laporan Penggunaan

Laporan awal:

- jumlah device aktif per bulan
- aktivitas login admin/guru
- command yang dijalankan
- uptime/heartbeat agent
- riwayat approval/revoke

## Screenshot On Demand

Cloud dapat meminta screenshot rendah resolusi untuk cek status. Fitur ini harus memiliki kontrol akses, audit log, dan aturan privasi yang jelas.

## Thumbnail Relay

Cloud dapat menampilkan thumbnail berkala, bukan streaming penuh.

Rekomendasi awal:

- resolusi rendah
- interval lambat
- hanya saat diminta
- audit log wajib
- batasi bandwidth per tenant

## Remote Control Via Cloud

Remote control via cloud adalah fitur paling berat dan sebaiknya masuk tahap akhir.

Tantangan:

- bandwidth tinggi
- latency
- NAT traversal
- relay server
- biaya operasional
- compliance privasi siswa

## Rekomendasi Urutan Implementasi

1. Tenant dan auth
2. Device activation per PC
3. Inventaris perangkat
4. Subscription dan kuota
5. Dashboard status
6. Konfigurasi cloud
7. Audit log
8. Remote command ringan
9. Alert dan laporan
10. Screenshot/thumbnail relay
11. Remote control via cloud

## Kesimpulan

Produk SaaS pertama sebaiknya fokus pada:

- lisensi per PC
- approval device
- inventaris
- konfigurasi cloud
- audit
- dashboard status

Ini paling dekat dengan kebutuhan komersial, lebih cepat dibuat, dan lebih aman dibanding langsung membangun full remote-screen SaaS.
