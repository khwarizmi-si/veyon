# Pekerjaan UI dan lokalisasi

Dicatat: 20 Oktober 2026. Ditunda atas permintaan pengguna.

- [ ] Selesaikan 598 pesan lama yang belum diterjemahkan dalam
  `translations/veyon_id.ts` (jumlah saat pencatatan).
- [ ] Telusuri banner lisensi Master yang masih berbahasa Inggris meskipun
  katalog Indonesia sudah dimuat dan tes lookup katalog biner lulus.
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
