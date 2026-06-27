# 08 — Privasi, Legal & Etika

> ⚠️ **Bukan nasihat hukum.** Ini software **monitoring siswa** (sering anak di bawah umur) →
> risiko hukum & reputasi **tertinggi** dari seluruh proyek. Libatkan ahli hukum sebelum produksi.

## 8.1 Mengapa ini krusial

Memantau & merekam layar siswa = memproses **data pribadi** (kadang **anak**). Salah tangani →
sanksi hukum, kehilangan kepercayaan sekolah/orang tua, reputasi hancur. Perlakukan privasi
sebagai **fitur inti**, bukan tempelan.

## 8.2 Regulasi yang relevan

| Regulasi | Wilayah | Inti |
|----------|---------|------|
| **UU PDP** (UU 27/2022) | Indonesia | Persetujuan, hak subjek data, pelindungan, sanksi; data anak butuh persetujuan orang tua/wali |
| **GDPR** | Uni Eropa | Bila ada user/anak EU; basis hukum, DPA, hak hapus |
| **COPPA** | AS (anak <13) | Persetujuan orang tua untuk data anak |
| **FERPA** | AS (data pendidikan) | Bila menyasar sekolah AS |

Untuk pasar Indonesia: **UU PDP** adalah fokus utama. Sediakan dasar pemrosesan yang sah,
persetujuan, dan pelindungan teknis.

## 8.3 Prinsip privacy-by-design

1. **Minimisasi data** — kumpulkan seperlunya. Hindari merekam layar terus-menerus tanpa alasan.
2. **Transparansi** — siswa & orang tua tahu kapan dipantau (indikator visual saat layar dilihat).
3. **Tujuan terbatas** — data hanya untuk manajemen kelas, bukan dijual/iklan.
4. **Retensi terbatas** — screenshot/log dihapus otomatis setelah periode tertentu.
5. **Kontrol akses** — hanya guru berwenang melihat kelasnya; audit setiap akses.
6. **Keamanan** — enkripsi in-transit & at-rest (lihat dok 09).
7. **Akuntabilitas** — audit log immutable, bisa pertanggungjawabkan siapa lihat apa kapan.

## 8.4 Implikasi desain produk (konkret)

| Prinsip | Wujud fitur |
|---------|-------------|
| Transparansi | Ikon/notifikasi di layar siswa saat sedang dilihat/dikontrol guru |
| Persetujuan | Banner consent saat enroll device; mode "jam sekolah saja" |
| Minimisasi | Default: monitoring thumbnail low-res; screenshot hanya manual/atas izin |
| Retensi | Auto-hapus screenshot & activity log setelah X hari (configurable per tenant) |
| E2EE | Streaming layar end-to-end encrypted → relay/kamu tak bisa lihat (lihat dok 03) |
| Akses | RBAC ketat + audit setiap "view screen" |
| Batas waktu | Monitoring hanya aktif pada jadwal sekolah; non-aktif di luar jam |

## 8.5 Peran data (penting untuk kontrak)

- **Sekolah = Data Controller** (penentu tujuan); **kamu (SaaS) = Data Processor** (memproses
  atas instruksi sekolah).
- Sediakan **DPA (Data Processing Agreement)** standar untuk diteken sekolah.
- Sekolah yang bertanggung jawab atas consent siswa/orang tua; kamu sediakan alat untuk itu.

## 8.6 Dokumen legal yang wajib disiapkan

- [ ] **Privacy Policy** (jelas, bahasa Indonesia, ramah orang tua)
- [ ] **Terms of Service**
- [ ] **DPA** (Data Processing Agreement) untuk sekolah
- [ ] **Consent template** untuk sekolah berikan ke orang tua/siswa
- [ ] **Data retention & deletion policy**
- [ ] **Sub-processor list** (cloud provider, payment, dll.)
- [ ] **Incident/breach response plan** (UU PDP wajib notifikasi kebocoran)

## 8.7 Etika & batasan (jaga reputasi)

- **Jangan** monitoring diam-diam tanpa sepengetahuan siswa → indikator visual wajib.
- **Jangan** aktif di luar jam/di perangkat pribadi.
- **Jangan** akses kamera/mikrofon/keylogger — itu jauh melampaui "manajemen kelas" & sangat
  berisiko hukum. Batasi pada layar & kontrol aplikasi di lingkungan sekolah.
- Posisikan sebagai **"alat bantu mengajar & menjaga fokus"**, bukan "mata-mata".
- Pertimbangkan **dewan/penasihat etika** & transparansi publik tentang apa yang dikumpulkan.

## 8.8 Lokalisasi data

- Pertimbangkan **hosting data di Indonesia** (atau region terdekat) untuk kepatuhan UU PDP &
  latensi. Cek penyedia dengan region ID (AWS Jakarta, GCP Jakarta, Alibaba, Biznet, dll.).
- Catat lokasi penyimpanan di Privacy Policy.

## 8.9 Ringkasan risiko

> Risiko hukum/reputasi **lebih besar** daripada risiko teknis. Bangun **transparansi, consent,
> minimisasi, retensi pendek, E2EE, dan audit** sejak awal. Jadikan privasi keunggulan jual
> ("aman & sesuai UU PDP"), bukan beban.
