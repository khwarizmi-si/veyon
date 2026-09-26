# Status lisensi pilot Khwarizmi

Terakhir diperiksa: 26 September 2026. Dokumen ini mencatat bukti, bukan persetujuan rilis.

## Implementasi saat ini

- API aktivasi dan check-in backend, halaman Lisensi Configurator, serta verifikasi token sudah ada sebelum pekerjaan enforcement.
- Dalam worktree `feat/license-client` (belum diverifikasi lewat build): file aktivasi dikunci sebelum secret ditulis dan izinnya ditegaskan lagi sesudahnya; identitas Master dan token bertanda tangan disalin ke file sistem terpisah tanpa secret agar dapat dibaca Master.
- `LicenseSyncFeature` meminta server lokal melakukan check-in di thread kerja. Master menyimpan token hasil verifikasi, menjadwalkan check-in harian, menampilkan banner, dan menolak sesi baru ketika lisensi tertangguhkan. Kuota berlebih tidak menangguhkan seluruh kelas; hanya perangkat di luar jatah yang tidak dibuka sebagai sesi baru.
- Permintaan `LicenseSync` hanya diterima dari koneksi loopback. Tombol Aktivasi dan Check now di Configurator menjalankan permintaan jaringan di thread kerja agar UI tidak berhenti merespons.

## Gerbang yang belum lulus

1. Build dan seluruh tes C++ di Linux dan Windows. `cmake` tidak tersedia di host ini dan Docker daemon tidak aktif saat pemeriksaan; `git diff --check` serta pemeriksaan XML saja yang lulus. Tes backend terpisah `khwarizmi-license`: 9 berkas, 117 tes lulus pada 26 September 2026; ini bukan bukti build klien C++.
2. Uji Windows sebagai administrator dan pengguna standar: file `KhwarizmiLicense.json` harus tidak dapat dibaca pengguna standar, sedangkan `KhwarizmiLicensePublic.json` harus dapat dibaca. Pastikan `veyon-server.exe` tetap bisa menulis token yang diperbarui.
3. Uji aktivasi baru, startup offline, check-in, perpanjangan, kedaluwarsa, kuota, dan pemulihan tanpa memutus sesi yang sedang berlangsung. Pastikan format installer dan terjemahan Indonesia benar.
   Tombol Check now di Configurator memverifikasi status langganan tanpa mengirim inventaris perangkat; inventaris dikirim oleh Master pada check-in rutinnya.
4. Verifikasi pemakaian CPU/memori dan tampilan Master pada lab Windows sungguhan. Jangan gunakan hasil tes landing page sebagai bukti untuk aplikasi desktop.
5. Harga final, cakupan dukungan, dan gateway pembayaran belum diputuskan. Pilot manual lebih dulu; jangan iklankan billing otomatis sebelum tersedia.

## Catatan desain

Rencana awal di `docs/superpowers/plans/2026-09-26-license-enforcement.md` menganggap Master dapat membaca token dari file aktivasi setelah ACL file itu diketatkan. Itu tidak benar untuk pengguna standar. Implementasi memakai file publik terpisah yang hanya berisi `masterId` dan token bertanda tangan; secret tetap di file terbatas. Semua klaim token wajib diverifikasi sebelum dipakai.

## Data pilot untuk harga dan billing

Belum ada data pilot yang cukup untuk menetapkan harga jual. Catat per sekolah: jumlah PC berlisensi dan aktif, durasi pemasangan, jam dukungan jarak jauh, kunjungan teknisi, biaya perjalanan, permintaan add-on, kendala OS/jaringan, serta kesediaan membayar dan siklus anggaran. Simpan identitas sekolah dan perangkat sesuai persetujuan serta kebijakan privasi yang berlaku.

Setelah pilot, hitung biaya implementasi satu kali terpisah dari biaya dukungan dan pembaruan berulang per PC. Uji paket minimum per lab, harga per PC tambahan, serta diskon pembayaran tahunan terhadap biaya layanan dan margin nyata. Selama pilot, admin dapat membuat sekolah, kode aktivasi, kuota, dan tanggal langganan secara manual; belum ada invoice atau pembayaran otomatis. Gateway dan webhook baru dipilih setelah alur penagihan, pajak, jatuh tempo, kegagalan bayar, dan rekonsiliasi disetujui.
