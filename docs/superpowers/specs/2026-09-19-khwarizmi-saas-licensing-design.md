# Desain: SaaS Licensing untuk Khwarizmi (fork Veyon)

- **Tanggal:** 2026-09-19
- **Status:** Disetujui, siap masuk tahap perencanaan implementasi
- **Lingkup:** Langganan berbasis invoice dengan kuota per-device, gate di Veyon Master

## 1. Masalah & tujuan

Khwarizmi (fork Veyon) akan dijual sebagai layanan berlangganan bulanan/tahunan
untuk sekolah. Sekolah yang membayar mendapat Veyon yang aktif; yang tidak
membayar masuk masa peringatan lalu disuspend. Tersedia free trial.

Tujuan:

- Sekolah membeli kuota device (mis. 40 slot), ditagih per bulan atau per tahun.
- Master menolak melampaui kuota; kelebihan tingkat sekolah ditangani backend.
- Langganan habis → peringatan dulu, baru suspend.
- Free trial melekat pada akun sekolah.

## 2. Non-tujuan (eksplisit)

Hal-hal berikut **tidak** dibangun, dan keputusannya disengaja:

- **Tidak ada relay/remote view lintas internet.** Monitoring tetap LAN-direct
  seperti Veyon sekarang. Ini bukan produk RMM.
- **Tidak ada metering pemakaian.** Kuota dijual, bukan diukur. Metering bisa
  ditambahkan nanti di backend karena data check-in sudah memuat yang diperlukan,
  tetapi tidak ada yang dibangun untuk itu sekarang.
- **Tidak ada perubahan di sisi PC murid.** `veyon-server` dan PC murid tidak
  disentuh sama sekali: tidak ada agent baru, tidak perlu internet, tidak ada
  redeploy ke ratusan komputer.
- **Tidak ada pengikatan ke hardware.** Lihat §11.
- **Tidak ada UI manajemen device di dashboard.** Digantikan aturan luruh
  otomatis di §7.

## 3. Batasan yang diterima: Veyon adalah GPL

Kode license-check yang ditambahkan juga berlisensi GPL dan wajib dipublikasikan.
Siapa pun dapat meng-compile ulang tanpa gate tersebut. **Penegakan di sisi klien
tidak dapat dibuat anti-bobol**, dan desain ini tidak berpura-pura sebaliknya.

Konsekuensi yang membentuk seluruh desain:

- Gate di klien berfungsi menghalau non-pembayar kasual, bukan penyerang niat.
- Gigi penegakan yang sesungguhnya ada di **backend**, yang tidak bisa di-compile
  ulang: penerbitan token, dedup kuota, deteksi kelebihan.
- Nilai komersial produk ada pada **layanan** (dashboard, update, dukungan),
  bukan pada DRM.

## 4. Arsitektur

```
[Dashboard web]  daftar sekolah, kuota, tagihan, kode aktivasi
       |
[Backend lisensi] <--webhook "lunas"-- [Payment gateway (TBD)]
       |   tenant - kuota - sub_end - daftar master & device
       |   menerbitkan token JWT bertanda tangan
       |
   HTTPS, sekali sehari
       |
[Veyon Master]  LicenseManager (core) + penegakan (master)
       |   verifikasi tanda tangan offline, public key tertanam
       |
[PC murid / veyon-server]  TIDAK DIUBAH
```

### Letak kode

| Lapis | Isi |
|---|---|
| `core/src/LicenseManager.{h,cpp}` | Parsing token, verifikasi tanda tangan, hitung status, deteksi jam mundur |
| `master/` | Dialog peringatan, blokir sesi baru, penegakan kuota di `ComputerControlListModel` |
| `configurator/` | Halaman Lisensi: input kode aktivasi, tampilan status |

Pemisahan ini disengaja: *"apa kata lisensi ini"* adalah logika murni yang dapat
diuji headless, terpisah dari *"lalu harus berbuat apa"* yang menyangkut UI.

**Bukan plugin.** Plugin dimuat dari `.dll` terpisah sehingga menghapus filenya
akan menghilangkan gate. Modularitas yang tepat untuk fitur menjadi kerentanan
untuk penegakan.

`core/src/CryptoCore.h` sudah menyediakan QCA dengan RSA-4096 dan
`EMSA3_SHA512`; tidak ada dependency kripto baru.

## 5. Format token

**JWT dengan RS512.** `EMSA3_SHA512` di `CryptoCore` persis sama dengan RS512
(PKCS#1 v1.5 + SHA-512), sehingga backend dapat memakai library JWT standar dan
master memverifikasi dengan QCA yang sudah ada. Tidak ada format buatan sendiri.

```json
{
  "iss": "license.khwarizmi.co.id",
  "sub": "mst_8f3a2b",
  "tenant": "sch_91c4",
  "tenant_name": "SMAN 1 Bandung",
  "plan": "trial" | "paid",
  "max_devices": 40,
  "sub_end": "2026-10-19T00:00:00Z",
  "overage_since": null,
  "iat": 1760832000,
  "exp": 1761436800
}
```

Header membawa `kid` untuk rotasi kunci. Saat ini hanya satu public key tertanam
di binary, tetapi dengan `kid` sejak awal, penggantian kunci tidak memerlukan
perubahan format bila private key bocor.

### Dua tanggal yang tidak boleh dicampur

| Klaim | Arti | Kalau lewat |
|---|---|---|
| `exp` | Token habis — urusan **teknis** | Gagal menghubungi server → toleransi offline |
| `sub_end` | Langganan habis — urusan **bisnis** | Belum bayar → peringatan lalu suspend |

Mencampur keduanya menyebabkan sekolah dengan internet buruk diperlakukan sebagai
penunggak. Keduanya dihitung terpisah dan digabung oleh satu aturan eksplisit
(§8).

**Tidak ada klaim `status`.** Status diturunkan dari `sub_end` dan waktu
sekarang. Menyimpannya berarti dua sumber kebenaran yang dapat berselisih.
`plan` tetap ada karena merupakan fakta berbeda, bukan turunan: dipakai untuk
membedakan pesan trial dari pesan langganan.

**Penyimpanan:** token dan secret master disimpan di `VeyonConfiguration`.

## 6. Identitas device & dedup

`Computer` sudah memiliki `macAddress()` dan `hostName()`
(`core/src/Computer.h`), tetapi MAC **sering kosong** karena admin tidak selalu
mengisinya di builtin directory. MAC tidak dapat menjadi satu-satunya kunci.

Aturan dedup di backend, dalam lingkup satu tenant:

1. MAC ada dan cocok dengan device lama → device yang sama
2. Jika tidak, hostname cocok → device yang sama
3. Jika tidak → device baru

Bila device yang sebelumnya hanya dikenal lewat hostname kemudian melapor membawa
MAC, backend mengisi MAC tersebut dan menggabungkan kedua catatan.

Normalisasi MAC (buang `:`, `-`, `.`, jadikan huruf kecil) meniru logika yang
sudah ada di `plugins/powercontrol/PowerControlFeaturePlugin.cpp`. Tabrakan
hostname antar sekolah tidak relevan karena dedup selalu per tenant.

## 7. Penegakan kuota

Kuota berlaku **per sekolah**, tetapi penegakan terjadi di master yang tidak
mengetahui daftar master lain. Penyelesaiannya dua lapis:

**Lapis 1 — batas keras lokal.** Master menegakkan `max_devices` terhadap
daftarnya sendiri, mencegah satu master mendaftarkan ratusan PC. Device di luar
kuota ditampilkan dengan status jelas "di luar kuota lisensi", diurutkan
deterministik berdasarkan identitas device sehingga yang tersisih selalu sama
dan tidak berganti-ganti antar restart.

**Lapis 2 — kelebihan tingkat sekolah di backend.** Backend menghitung device
unik seluruh tenant. Bila melewati kuota, token berikutnya membawa
`overage_since` dan master menampilkan banner pemakaian. **Master tetap berjalan
penuh.** Bila tidak diselesaikan dalam **14 hari**, master masuk Suspend dengan
pesan khusus kelebihan kuota.

Backend **tetap menerbitkan token seperti biasa** selama kelebihan kuota.
Menghentikan penerbitan token akan membuat `exp` lewat, sehingga master masuk
sumbu koneksi (§8) dan menampilkan "periksa koneksi internet" — pesan yang salah
untuk sebab yang sebenarnya. Kelebihan kuota karena itu menjadi sumbunya sendiri
dengan pesannya sendiri.

Alasan: kelebihan kuota adalah percakapan tagihan, bukan kegagalan teknis.
Memutus kelas yang sedang berjalan karena kelebihan beberapa slot merusak
kepercayaan, sementara penegakan instan di klien toh dapat di-compile ulang.
Penegakan ditempatkan di bagian yang tidak dapat dibobol, dengan tempo hari
bukan detik. 14 hari dipilih karena penyelesaiannya menuntut tindakan manusia:
membeli slot tambahan.

**Device luruh otomatis.** Device hanya dihitung bila terlihat dalam **30 hari**
terakhir. Tanpa aturan ini, PC yang rusak, diganti, atau di-reimage memakan slot
selamanya dan sekolah mencapai batas kuota padahal komputernya sudah tidak ada.
Efek sampingnya: UI "hapus device" di dashboard tidak diperlukan sama sekali.

## 8. Siklus status

**Trial melekat pada akun sekolah, bukan instalasi.** Bila trial dihitung sejak
pertama dijalankan di PC, install ulang menghasilkan trial baru tanpa batas.
Trial diberikan saat pendaftaran di dashboard dan dibawa oleh kode aktivasi.

### Alur aktivasi

1. Admin mendaftar di dashboard → menerima kode aktivasi + trial 30 hari
2. Admin membuka Veyon Configurator → halaman Lisensi → memasukkan kode
3. Master menukar kode ke backend → menerima `master_id`, secret permanen, token pertama
4. Secret disimpan di config; kode aktivasi hangus
5. Setiap hari master check-in dengan secret + daftar device → menerima token baru

### Perhitungan status

Tiga sumbu dihitung terpisah, lalu **diambil yang terburuk**:

| Sumbu langganan (`sub_end`) | Sumbu koneksi (`exp`) | Sumbu kuota (`overage_since`) |
|---|---|---|
| Aktif → Normal | Aktif → Normal | Kosong → Normal |
| Lewat <= 7 hari → Peringatan | Lewat <= 14 hari → Toleransi offline | <= 14 hari → Peringatan kuota |
| Lewat > 7 hari → Suspend | Lewat > 14 hari → Suspend | > 14 hari → Suspend |

Setiap sumbu membawa pesannya sendiri, sehingga status Suspend selalu dapat
menjelaskan sebab yang benar: belum bayar, tidak dapat menghubungi server, atau
kelebihan kuota. Bila lebih dari satu sumbu memburuk bersamaan, pesan mengikuti
sumbu terburuk; bila setara, urutan prioritas pesan adalah langganan, kuota,
lalu koneksi.

Di atas keduanya:

- `JamMundur` — jam sistem lebih awal dari waktu-server terakhir yang tersimpan
  → paksa cek online sebelum melanjutkan.
- `BelumAktivasi` — belum ada token → blokir monitoring, tampilkan dialog aktivasi.

Toleransi offline (14 hari) sengaja lebih longgar daripada masa peringatan
(7 hari): sekolah dengan internet buruk adalah pelanggan yang membayar, bukan
penunggak.

### Perilaku master per status

| Status | Perilaku |
|---|---|
| Normal | Jalan penuh |
| Toleransi offline | Jalan penuh + banner koneksi |
| Peringatan kuota | Jalan **penuh** + banner pemakaian kuota |
| Peringatan | Jalan **penuh** + dialog saat start + banner tetap |
| Suspend | Tolak **sesi baru**; sesi berjalan tidak pernah diputus |
| BelumAktivasi | Monitoring diblokir, dialog aktivasi |

### Kasus khusus: sudah bayar tetapi offline

Bila `sub_end` lewat sementara internet mati, master tidak dapat mengetahui
pembayaran telah dilakukan, sehingga tetap masuk peringatan lalu suspend.
**Pesannya wajib berbunyi "tidak dapat memverifikasi pembayaran, periksa koneksi
internet", bukan "Anda belum membayar".** Menuduh pelanggan yang membayar sebagai
penunggak adalah kerusakan yang tidak sebanding dengan penyebabnya. Status pulih
seketika setelah online dan terverifikasi.

## 9. Permukaan backend

Karena pembayaran berbasis invoice, **tidak diperlukan mesin langganan berulang**.
Perpanjangan hanya menggeser `sub_end`; tidak ada proration, retry gagal-debit,
atau kartu kedaluwarsa.

| Endpoint | Auth | Isi | Balasan |
|---|---|---|---|
| `POST /v1/activate` | kode aktivasi | kode, hostname & versi master | `master_id`, secret, token pertama |
| `POST /v1/checkin` | secret master | daftar device (hostname, MAC) | token baru (exp +7 hari) berisi `overage_since` |
| `POST /v1/webhooks/payment` | tanda tangan gateway | notifikasi lunas | menggeser `sub_end` |

Pilihan payment gateway hanya menyentuh endpoint ketiga.

### Model data

| Tabel | Kolom inti |
|---|---|
| `tenant` | nama, plan, `max_devices`, `sub_end` |
| `master` | tenant, hash secret, terakhir terlihat, versi |
| `device` | tenant, MAC (nullable), hostname, pertama & terakhir terlihat |
| `activation_code` | kode, tenant, dipakai pada, kedaluwarsa |
| `invoice` | tenant, nominal, periode, status, referensi gateway |

### Dashboard (minimal untuk peluncuran)

Daftar sekolah, pemakaian kuota, tagihan, kode aktivasi. Tanpa manajemen device
(§7), tanpa manajemen user berlapis.

## 10. Invariant

Invariant berikut bersifat mengikat dan harus diperiksa pada setiap perubahan:

1. **Jalur lisensi steril dari data pemantauan.** Check-in hanya mengirim
   `master_id`, identitas device (hostname/MAC), dan jumlah. **Tidak pernah**
   screenshot, aktivitas murid, nama pengguna, atau data pemantauan apa pun.
   Begitu jalur lisensi membawa data pemantauan, produk masuk wilayah UU PDP yang
   jauh lebih berat. Kebocoran semacam ini biasanya terjadi perlahan lewat
   "sekalian kirim ini juga".
2. **Sesi yang sedang berjalan tidak pernah diputus oleh keadaan lisensi.**
   Suspend hanya menolak sesi baru.
3. **Kegagalan condong ke arah membiarkan sekolah bekerja** (§11).
4. **`LicenseManager` tidak membaca jam sistem sendiri** (§12).

## 11. Penanganan kegagalan

| Kejadian | Perlakuan |
|---|---|
| Check-in gagal (jaringan) | Bukan error. Pakai token tersimpan, coba lagi dengan backoff. |
| Token rusak / tanda tangan tidak cocok | Tolak token baru, **pertahankan token lama**, catat log. Token lama tetap punya `exp` sendiri sehingga tidak ada celah permanen. |
| Backend membalas 401/403 | Perlakukan sebagai kegagalan koneksi (masuk toleransi offline), **kecuali** ada penanda eksplisit `tenant_terminated`. |

Baris terakhir adalah yang paling penting. Memperlakukan "secret ditolak" sebagai
otoritatif terasa benar, tetapi satu bug deploy di backend yang salah menolak
auth akan mematikan seluruh sekolah pelanggan secara serentak.

**Pengikatan hardware sengaja tidak dilakukan.** Menyalin config ke PC lain
menghasilkan master kedua dengan `sub` sama; backend akan melihat satu id
check-in dari dua tempat dan dapat menandainya. Mengikat ke hardware menambah
kerumitan besar (ganti motherboard, clone image lab, re-aktivasi massal) demi
penghalang yang tetap dapat dibobol dengan compile ulang (§3). Deteksi di backend
jauh lebih murah daripada pencegahan di klien.

## 12. Pengujian

Repo belum memiliki unit test; `tests/` hanya berisi fuzzer. Qt sudah menyediakan
QTest sehingga tidak perlu dependency baru.

**Syarat desain:** `LicenseManager` menerima "waktu sekarang" sebagai parameter
dan tidak membaca jam sendiri. Tanpa itu, menguji kedaluwarsa berarti menunggu
berhari-hari atau mengubah jam sistem, dan matriks tiga-sumbu di §8 praktis tidak
akan pernah diuji.

Yang harus diuji:

- Matriks status tiga sumbu (uji tabel), termasuk perpotongan antar sumbu dan
  prioritas pesan saat lebih dari satu sumbu memburuk bersamaan
- Verifikasi tanda tangan: token valid, token dirusak, `kid` tidak dikenal
- Deteksi jam mundur
- Aturan dedup device (§6), termasuk penggabungan hostname → MAC
- Aturan luruh 30 hari (§7)

## 13. Dekomposisi & urutan

Ini tiga subsistem, masing-masing dengan rencana implementasi terpisah:

1. **Backend lisensi** — API, penandatanganan token, model data
2. **Klien Veyon** — `LicenseManager` di core, penegakan di master, halaman Configurator
3. **Dashboard + gateway pembayaran**

Urutan: **1 → 2 → 3**. Setelah 1 dan 2 selesai, produk berlisensi sudah utuh dan
dapat dipakai pilot dengan tenant yang dibuat manual. Billing otomatis tidak
diperlukan untuk menandatangani beberapa sekolah pertama, dan menunda tahap 3
membuat keputusan payment gateway tidak memblokir apa pun.

## 14. Keputusan terbuka

| Hal | Status |
|---|---|
| Payment gateway | Akan ditentukan pemilik produk. Kandidat lokal: Midtrans, Xendit (VA, QRIS, e-wallet). Hanya menyentuh `POST /v1/webhooks/payment`. |
| Bahasa/stack backend | Belum ditentukan; harus punya library JWT RS512 yang matang. |
| Harga per slot, bulanan vs tahunan | Keputusan bisnis, tidak memengaruhi desain teknis. |
