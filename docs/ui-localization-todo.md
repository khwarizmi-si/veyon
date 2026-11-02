# Pekerjaan UI dan lokalisasi

Dicatat: 20 Oktober 2026. Ditunda atas permintaan pengguna.

- [x] Selesaikan pesan lama yang belum diterjemahkan dalam
  `translations/veyon_id.ts`: 0 entri unfinished dari 1.266 pesan pada
  pemeriksaan terakhir 2 November 2026. Termasuk 68 terjemahan sumber baru
  yang ditemukan melalui audit lupdate pada salinan sementara.
- [x] Verifikasi banner lisensi Master dalam bahasa Indonesia setelah katalog
  build terbaru dipasang dan aplikasi dimulai ulang. Status Suspended terlihat
  dalam bahasa Indonesia di noVNC; tes katalog juga mencakup Warning.
- [ ] Audit bahasa pada tooltip, panel tindakan, dialog, pesan kesalahan,
  halaman lanjutan Configurator, serta add-on.
- [ ] Pastikan katalog Indonesia disertakan dalam installer dan preferensi
  bahasa tampil sesuai nilai konfigurasi, bukan kembali ke bahasa sistem.
- [ ] Verifikasi bahasa dan teks panjang pada Windows serta jendela sempit.
- [ ] Uji audio dengan PC siswa yang benar-benar terhubung; verifikasi hasil
  perangkat dan label tindakan kebalikannya. PC Self dikecualikan oleh Auvidus.

## Catatan Tombol Audio

Master menonaktifkan kontrol mode selain Pantau jika tidak ada komputer
terhubung dalam tampilan. Langganan kedaluwarsa dengan tingkat Warning masih
dalam masa tenggang; `licenseBlocksNewSessions` baru memblokir pada tingkat
Suspended (selain kasus kuota yang ditangani terpisah). Status lisensi tetap
harus diperiksa ulang saat pengujian berikutnya.

## Kemajuan 2 November 2026

- Terjemahan tambahan: 64 pesan pada panel komputer, konfirmasi daya/sesi,
  buka aplikasi/situs, pesan teks, transfer dan pengumpulan berkas.
- Pilihan tema System kini dapat diterjemahkan menjadi Ikuti sistem.
- Build Docker menggunakan WITH_TRANSLATIONS=ON dan
  VEYON_UPDATE_TRANSLATIONS=OFF; katalog Indonesia dipasang dalam jalur
  runtime build-tree.
- Preferensi Indonesia cocok dengan entri pemilih bahasa di Configurator.
- Lisensi tester saat pemeriksaan berstatus Suspended. Tidak ada perpanjangan,
  aktivasi baru, atau perubahan kuota produksi pada pekerjaan ini.
- QA Windows dan perilaku perangkat siswa masih belum dilakukan.

## Penyelesaian Katalog 2 November 2026

- Sisa 534 entri lama selesai, mencakup pengaturan Master/Layanan, LDAP/AD,
  Windows/Linux, transfer berkas, pesan koneksi, dan bantuan CLI.
- Tambahkan konteks baru chat, Entra ID, blokir internet, penemuan jaringan,
  perekam layar, serta nama produk Khwarizmi yang belum ada di katalog lama.
- Samakan istilah toolbar: Percakapan, Mulai ulang, Buka aplikasi/situs,
  Kirim/Kumpulkan berkas, Blokir internet, dan Matikan/Aktifkan kamera.
- Tes XML menyeluruh memeriksa entri kosong/unfinished dan kesetaraan
  placeholder. Pemeriksaan format rich text juga lulus.
- Build katalog menghasilkan 1.266 terjemahan selesai. license-test dan
  toolbutton-test lulus dengan katalog biner aktif.
- QA noVNC: halaman Master/Layanan Configurator, dialog masuk, dan panel
  tindakan Master. Tidak ada tindakan perangkat atau perubahan lisensi.
- Staging Windows menyalin translations/*.qm; instalasi Windows nyata masih
  perlu diverifikasi, bukan dianggap selesai berdasarkan inspeksi skrip.
- Untuk pembaruan katalog pada proses yang sedang berjalan, gunakan berkas
  sementara lalu rename atau mulai ulang aplikasi; jangan timpa berkas QM
  yang sedang dimuat.
- Kelengkapan katalog bukan jaminan seluruh teks runtime atau tata letak
  sudah diaudit. Pemeriksaan semua dialog, installer, dan Windows tetap
  berada dalam daftar pekerjaan di atas.

## QA Lanjutan 2 November 2026

- Entra ID diperiksa pada jendela normal dan sempit. Petunjuk izin Graph
  membungkus dalam area konten; kredensial tidak diisi dan tes koneksi tidak
  dijalankan.
- Reproduksi bug: pindah dari Entra ID yang sudah digulir ke Transfer Berkas
  mempertahankan posisi gulir sehingga halaman baru terbuka di tengah.
  Diperbaiki dengan mereset kedua scrollbar saat kategori berubah.
- Dropdown dengan opsi panjang memaksa formulir melebar. Minimum lebar
  dropdown kini berdasarkan 12 karakter, bukan opsi terpanjang; nilai dan
  indeks pilihan tidak diubah.
- Uji ulang 900x700: Transfer Berkas terbuka dari atas, tanpa scrollbar
  horizontal, dan footer Terapkan tetap terlihat. Halaman panjang masih
  dapat digulir vertikal.
- CI Windows sekarang membangun veyon_id_qm, license-test dan toolbutton-test,
  serta memasok VEYON_TEST_ID_QM agar tes katalog biner tidak dilewati.
  Sintaks YAML/bash tervalidasi lokal; run GitHub/installer Windows belum
  dijalankan pada checkpoint ini.
- Kedua target CTest lokal lulus dengan katalog biner aktif. Pengujian ini
  tidak mencakup perangkat siswa atau layanan produksi.

## Tema Gelap dan Keyboard

- Periksa Configurator Umum/Lisensi dan Master beserta panel tindakannya
  dalam tema gelap. Tema preview dipulihkan ke nilai awal 0 (Ikuti sistem).
- Tes membuktikan tombol fitur dapat menerima fokus Tab.
- Tes kontras mereproduksi kegagalan fokus tema gelap: hijau utama terhadap
  permukaan input sekitar 2,78:1. Warna fokus khusus #4ade80 menghasilkan
  sekitar 8:1; warna utama #15803d dipertahankan untuk teks putih (~5:1).
  Ini pemeriksaan pasangan warna, bukan sertifikasi aksesibilitas menyeluruh.
- Tambahkan batas fokus putus-putus pada tombol dan pastikan warna fungsi
  toolbar tidak menimpa indikator fokus.
- Total padding vertikal tombol saat ditekan dipertahankan sama dengan
  kondisi normal (14px), dengan pergeseran isi 2px.
- Perbaiki empat ikon panel bawah dan elipsis yang sebelumnya hitam pada
  tema gelap menggunakan tint eksplisit sesuai fungsi/tema.
- Tombol aksi perangkat dalam dialog tidak menjadi auto-default. noVNC
  memverifikasi tombol default adalah Tutup dan Enter menutup panel.
- Build dan kedua target CTest lulus setelah perubahan. Pengujian Windows,
  screen reader, serta aksi pada perangkat siswa tetap belum dilakukan.
