# QA fitur Windows (pilot)

Laporan pengguna dari Windows installer build #44 (`7a4f4976e`): webcam, audio, USB, chat wajib dijawab, filter aktif/offline,
peringatan diskoneksi, serta hasil rekaman dan tangkapan layar belum bekerja
sesuai harapan. Status di bawah adalah audit kode, **bukan** bukti lulus uji PC.

| Fitur | Temuan kode | Uji yang harus lulus di PC santri dan Master |
| --- | --- | --- |
| Bisukan audio | Build #44 mengubah endpoint Core Audio dari Veyon Service (session 0). Perubahan lokal sekarang meneruskan perintah ke worker sesi santri, tetapi belum dibangun/diuji di Windows dan hasilnya belum dikirim balik ke Master. | Bisukan dan pulihkan speaker serta mikrofon pada sesi santri yang sedang login; laporkan kegagalan per PC. |
| Blokir webcam | Auvidus menulis consent webcam tingkat mesin di registry. Belum ada bukti aplikasi yang sudah membuka kamera ikut terblokir, dan hasil perintah tidak dilaporkan. | Uji kamera sebelum/sesudah blokir pada aplikasi Windows yang sama, termasuk setelah logout/login. |
| Blokir USB | Auvidus mengubah `USBSTOR\Start`; ini mengendalikan driver penyimpanan USB, bukan semua jenis USB, dan belum menangani perangkat yang telah terpasang. Hasil perintah tidak dilaporkan. | Uji flashdisk yang telah terpasang dan yang baru dipasang; pastikan keyboard/mouse USB tetap berfungsi. |
| Chat | Jendela santri sekarang menolak close sampai balasan dikirim dalam perubahan lokal yang belum dibangun/diuji. Menutup proses dari Task Manager tetap tidak bisa dicegah oleh jendela ini. | Kirim pesan, coba tombol X dan Alt+F4 sebelum balasan, lalu balas dan tutup; ulangi setelah reconnect. |
| Filter status | Build #44 hanya punya filter "powered on" dan "ada pengguna login". Perubahan lokal menambah pilihan Semua PC/Aktif/Tidak aktif dan mematikan filter pengguna login saat Tidak aktif dipilih; belum diuji di Windows. | Pastikan seluruh PC online/offline terlihat dan pilihan filter tidak menyembunyikan PC yang perlu ditindak. |
| Peringatan diskoneksi | Perubahan lokal kini menulis pesan status 15 detik setelah PC yang pernah Connected terputus. Belum berupa notifikasi persisten. | Putuskan koneksi PC santri setelah Connected; cek pesan, reconnect, lalu ulangi. |
| Tangkapan layar | Perubahan lokal memakai framebuffer berskala saat framebuffer penuh kosong dan melaporkan jika tidak ada berkas tersimpan. | Ambil screenshot PC Connected, buka panel Tangkapan dan berkas di screenshot directory; cek gambar tidak kosong. |
| Perekam layar | Add-on Windows membutuhkan `screenrecorder.dll` dan `ffmpeg.exe`. Perubahan lokal kini menolak start jika encoder/folder/target Connected tidak tersedia dan menunjukkan lokasi output; belum dibangun/diuji. Hasil MP4 ditulis di `Videos/VeyonRecordings` pada PC Master, bukan panel Tangkapan. | Periksa kedua berkas terpasang, mulai rekam PC Connected, tunggu beberapa detik, hentikan agar MP4 selesai, lalu putar hasil. |

Catat untuk setiap uji: commit/branch installer, nama artifact, versi Windows,
hak akun santri, daftar add-on yang terpasang pada Master dan PC santri, waktu
uji, nama PC, hasil yang diharapkan/terjadi, lokasi output, dan log Service.
Jangan tandai fitur lulus hanya karena tombol bisa diklik. Perubahan lokal di
chat/screenshot/diskoneksi harus dibangun dan diuji di Windows sebelum diklaim
sebagai perbaikan.
