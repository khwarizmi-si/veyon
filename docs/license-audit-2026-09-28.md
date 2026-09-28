# Audit lisensi dan server — 28 September 2026

## Bukti yang dijalankan

- Backend `khwarizmi-license`: `npm test` lulus, 9 berkas/117 tes (termasuk typecheck).
- Tes reproduksi tambahan pada `test/activate.test.ts`: 7 tes lulus termasuk karakterisasi kegagalan signing yang menghabiskan kode. Tes ini mendokumentasikan bug, bukan membuktikan bug sudah diperbaiki.
- Produksi: GET `https://license.khwarizmi.co.id/health` menghasilkan `{"ok":true}`; POST `/v1/checkin` tanpa kredensial ditolak dengan `unauthorized`. HEAD pada endpoint POST menghasilkan 404 dan bukan bukti endpoint rusak.
- Tidak menggunakan kode aktivasi, secret, atau token sekolah; tidak mengubah data produksi.
- Belum ada reproduksi Windows end-to-end dari penambahan PC ke D1. Build installer sukses tidak menggantikan uji ini.

## Temuan

1. **P1 — Inventaris tidak disinkronkan saat device ditambahkan.** `master/src/VeyonMaster.cpp:141` hanya check-in bila waktu token terakhir sudah 24 jam. Pemicu hanya startup, koneksi lokal, dan timer per jam; tidak ada pemicu perubahan model. Aktivasi/check-in kosong yang baru dilakukan membuat check-in inventaris tertunda sekitar sehari. Ini sangat cocok dengan gejala device baru tidak terhitung, tetapi penyebab pada PC pengguna belum dibuktikan dari log Windows.
2. **P1 — Kode aktivasi habis jika server gagal setelah klaim.** Backend `src/routes/activate.ts:20` menandai kode terpakai sebelum insert Master dan signing (`:53`). Reproduksi dengan signing key invalid: 500, kode terpakai, satu Master tertinggal, retry 400. Perlu preflight signing, transaksi untuk perubahan DB, dan strategi pemulihan/idempotensi saat respons hilang; preflight saja tidak menyelesaikan response-loss.
3. **P1 — Device yang ditolak kuota tidak otomatis dimulai setelah kuota naik.** `master/src/VeyonMaster.cpp:124` membandingkan hanya level/reason/deadline dan reload hanya ketika suspended berubah menjadi unblocked. `maxDevices` bukan bagian pembanding. Jika token Normal dengan kuota 1 diperbarui menjadi Normal dengan kuota 2, interface yang tidak pernah start tetap tidak aktif sampai model dimuat ulang.
4. **P2 — Inventaris bukan daftar device aktif.** `core/src/LicenseSyncFeature.cpp:101` mengirim semua interface di daftar yang dipilih, termasuk offline/tidak pernah tersambung; bukan seluruh inventaris sekolah dan bukan hanya sesi aktif. Backend menghitung last_seen dalam jendela 30 hari (`src/devices.ts:2`), sehingga pelaporan ulang PC offline membuatnya terus dianggap aktif. Definisi slot berlisensi perlu diputuskan sebelum billing, dan identitas harus konsisten antara klien dan backend.
5. **P2 — Check now tidak menghitung device.** `configurator/src/LicensePage.cpp:145` mengirim `checkIn({})`. Itu valid untuk memeriksa langganan, tetapi tidak mengirim inventaris, sementara waktu check-in yang baru menunda check-in Master. UI perlu membedakan cek langganan dan sinkronisasi perangkat.
6. **P2 — Penyebab gagal sinkronisasi hanya muncul di log.** Jika koneksi Master ke server lokal tidak Connected, check-in langsung return (`VeyonMaster.cpp:144`). Gagal jaringan/token hanya dicatat di log. Token offline bisa tetap valid; status aktif tidak membuktikan server sedang terhubung. Perlu status koneksi terakhir, hasil check-in, jumlah perangkat terkirim, dan retry yang terjadwal.

## Kekurangan langganan

Backend menyediakan tenant, kuota, tanggal langganan, aktivasi, dan check-in. Invoice, pembayaran, webhook gateway, rekonsiliasi, serta pembaruan langganan otomatis belum ditemukan dalam sumber backend. Saat ini pengelolaan langganan masih manual; belum layak disebut billing SaaS otomatis.

## Urutan perbaikan dan tes

1. Pisahkan jadwal refresh token dari sinkronisasi inventaris; perubahan device memicu check-in terdebounce dengan retry, tidak menunggu 24 jam.
2. Tambahkan tes naik/turun kuota dan pemulihan interface tanpa memutus sesi yang berjalan.
3. Perbaiki transaksi/pemulihan aktivasi; uji signing failure, DB failure, respons hilang, dan retry konkuren.
4. Tentukan apakah slot dihitung sebagai terdaftar, tersambung, atau aktif dalam periode; samakan admin, klien, dan billing.
5. QA tenant khusus: aktivasi → tambah dua PC → lihat D1/admin → tambah satu PC → cek hitungan → ubah kuota/perpanjang → reboot/offline/reconnect. Rekam waktu dan hasil tiap tahap tanpa menyimpan secret pada laporan.

## Progres patch setelah audit

- Status overage dihitung, ditulis, dan dikembalikan dalam satu `UPDATE ... RETURNING`. Tes dua caller konkuren sebelumnya gagal karena salah satu menerima waktu overage berbeda dari DB; setelah patch lulus. Seluruh backend: 9 berkas/121 tes dan typecheck lulus. Ini belum memperbaiki dedup insert perangkat konkuren atau definisi slot aktif.
- Backend aktivasi menyiapkan token sebelum menulis DB; insert Master bersyarat dan pemakaian kode berjalan dalam satu `DB.batch`. Signing failure mempertahankan kode, kegagalan update kode me-rollback Master, dan empat aktivasi konkuren menghasilkan tepat satu Master.
- Verifikasi backend setelah patch: `npm test` lulus, 9 berkas/120 tes termasuk typecheck. D1 lokal, tidak mengubah produksi. Semantik rollback mengacu pada https://developers.cloudflare.com/d1/worker-api/d1-database/#batch.
- Backend menyediakan recovery respons aktivasi hilang melalui `activation_secret` opsional (64 karakter hex lowercase, dibuat acak oleh klien). Hash dicocokkan dengan Master yang ditautkan pada kode terpakai; retry tidak membuat Master baru. Kode saja atau secret berbeda tetap ditolak. Migrasi `0002_activation_recovery.sql` diperlukan sebelum deploy.
- Desktop kini membuat secret dari `QRandomGenerator::system`, menyimpannya pada store aktivasi yang diproteksi sebelum request, dan memverifikasi hasil simpan. Retry kode/server yang sama memakai secret yang sama. Store publik dan ekspor konfigurasi tidak memuat credential retry. Tes payload recovery ditambahkan tetapi belum dijalankan karena toolchain C++ lokal tidak tersedia; build dan recovery Windows end-to-end belum selesai.
- Tes recovery lokal: retry respons hilang, secret salah/hilang/malformed, dan empat retry identik konkuren. Seluruh backend: 9 berkas/124 tes dan typecheck lulus. Tidak ada migrasi/deploy produksi pada tahap ini.
- Perubahan inventaris memicu check-in terdebounce; retry tidak lagi menunggu refresh harian.
- Master mencoba kembali hanya interface yang tertahan lisensi ketika token/kuota diperbarui, termasuk Normal ke Normal. Pemulihan tidak lagi me-reload semua sesi.
- Patch ini belum dibangun atau diuji pada Windows. `git diff --check` lulus; itu hanya pemeriksaan format, bukan bukti perilaku runtime.

### Regresi pemulihan kuota (QA Windows)

1. Gunakan tenant uji berkuota 1 dan pilih dua PC. Pastikan satu sesi aktif dan satu tertahan.
2. Naikkan kuota menjadi 2 lalu jalankan check-in. Kedua sesi harus aktif tanpa memuat ulang daftar atau me-restart Master.
3. Pastikan sesi pertama tidak terputus dan fitur yang sedang berjalan tetap berlangsung.
4. Ulangi check-in tiga kali: tidak boleh ada koneksi atau penanganan sinyal ganda.
5. Hapus PC tertahan sebelum menaikkan kuota: tidak boleh crash atau menghidupkan kembali PC yang telah dihapus.
6. Uji pemulihan suspended ke aktif. Sesi tertahan harus mulai tanpa reload seluruh model.

### Regresi respons aktivasi hilang (tenant uji)

1. Terapkan migrasi dan backend recovery pada lingkungan uji terlebih dahulu.
2. Putuskan respons pertama setelah server berhasil commit (gunakan proxy uji; jangan log body atau secret).
3. Tutup dan buka Configurator lalu retry kode yang sama. Harus mendapatkan Master yang sama, total Master tetap satu.
4. Coba kode terpakai dari PC lain tanpa credential retry: harus ditolak.
5. Cabut izin tulis store lokal sebelum aktivasi: tidak boleh mengirim request atau menghabiskan kode.
6. Periksa ACL store, ekspor konfigurasi, dan log: credential retry tidak boleh terbaca user biasa atau muncul dalam ekspor/log.

Temuan belum boleh ditandai selesai berdasarkan patch atau health check saja. Aktivasi atomik telah lolos tes lokal tetapi belum deployed; recovery respons hilang, semantik slot, dan billing masih belum selesai.
