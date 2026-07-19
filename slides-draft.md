# 📊 Draft Slide Presentasi
# Khwarizmi Surveillance System
# Produk Knowledge — Sistem Surveillance

---

## SLIDE 1: Cover

**Khwarizmi Surveillance System**
*Produk Knowledge — Sistem Surveillance*

📅 [Tanggal Presentasi]
👤 [Nama Presenter]
🏢 Al-Khwarizmi

---

## SLIDE 2: Agenda

1. Latar Belakang & Masalah
2. Apa itu Khwarizmi?
3. Fitur Utama
4. Arsitektur Sistem
5. Use Cases / Skenario Penggunaan
6. Keunggulan Kompetitif
7. Demo Sekilas
8. Deployment & Instalasi
9. Roadmap
10. Q&A

---

## SLIDE 3: Latar Belakang & Masalah

**Mengapa Sistem Surveillance Dibutuhkan?**

- 📈 Perubahan ke arah pembelajaran digital & hybrid semakin cepat
- 🏫 Institusi pendidikan butuh kontrol atas puluhan–ratusan komputer sekaligus
- 💻 Remote support yang efisien mengurangi downtime dan biaya operasional
- 🔒 Kebutuhan keamanan: mencegah akses tidak sah, dokumentasi aktivitas user
- 🌍 Kebutuhan monitoring multi-lokasi dari satu titik kontrol

> *"Bagaimana mengelola, memonitor, dan mengontrol banyak komputer secara efisien dari satu dashboard?"*

---

## SLIDE 4: Apa itu Khwarizmi?

**Khwarizmi Surveillance System** adalah perangkat lunak gratis dan open-source untuk **monitoring dan kontrol komputer** lintas platform.

- 🔱 **Fork dari Veyon** (Virtual Eye On Networks) — proyek open-source yang sudah teruji
- 📜 **Lisensi GPLv2** — bebas digunakan, dimodifikasi, dan didistribusikan
- 🌐 **Cross-platform** — mendukung Linux dan Windows
- 🎯 **Fokus**: pendidikan, pelatihan virtual, dan remote support
- 🏷️ Rebranding dari Veyon dengan identitas Al-Khwarizmi

> Nama "Al-Khwarizmi" diambil dari ilmuwan Muslim yang dikenal sebagai Bapak Aljabar — melambangkan fondasi komputasi modern.

---

## SLIDE 5: Fitur Utama — Overview & Monitoring

| Fitur | Deskripsi |
|-------|-----------|
| 🖥️ **Overview** | Monitor semua komputer di satu atau banyak lokasi/kelas secara real-time dalam satu dashboard |
| 🔍 **Remote Access** | Lihat atau kendalikan komputer user untuk observasi dan bantuan teknis |
| 📺 **Demo / Broadcast** | Broadcast layar guru/instruktur secara real-time (fullscreen atau window) |
| 🔒 **Screen Lock** | Kunci layar semua komputer untuk menarik perhatian user |
| 💬 **Communication** | Kirim pesan teks ke siswa/user secara langsung |

---

## SLIDE 6: Fitur Utama — Manajemen & Administrasi

| Fitur | Deskripsi |
|-------|-----------|
| 📚 **Lesson Management** | Login dan logout semua user sekaligus (mulai & akhiri sesi) |
| 📸 **Screenshots** | Ambil screenshot untuk dokumentasi progres dan pelanggaran |
| 🚀 **Program & Website** | Jalankan program dan buka URL website dari jarak jauh |
| 📂 **Teaching Material** | Distribusikan dokumen, gambar, dan video ke semua komputer |
| ⚡ **Power Management** | Nyalakan, matikan, dan reboot komputer secara remote |

---

## SLIDE 7: Arsitektur Sistem

```
┌─────────────────────────────────────────────┐
│              MASTER (Teacher)                │
│         veyon-master + configurator          │
└──────────────────┬──────────────────────────┘
                   │  TCP (Port 11100)
        ┌──────────┼──────────┐
        │          │          │
   ┌────▼───┐ ┌───▼────┐ ┌──▼─────┐
   │Client 1│ │Client 2│ │Client N│
   │veyon-  │ │veyon-  │ │veyon-  │
   │service │ │service │ │service │
   └────────┘ └────────┘ └────────┘
```

**Komponen Utama:**
- **veyon-master**: Aplikasi kontrol utama (sisi guru/admin)
- **veyon-service**: Service daemon yang berjalan di setiap client
- **veyon-configurator**: Tool konfigurasi untuk setup dan manajemen
- Komunikasi via protokol RFB (VNC-based) melalui port 11100

---

## SLIDE 8: Use Cases / Skenario Penggunaan

**🎓 Pendidikan — Smart Classroom**
> Guru memonitor 40+ komputer lab, broadcast layar, kunci layar saat menjelaskan materi

**🏢 Perusahaan — IT Remote Support**
> Tim IT melakukan troubleshooting tanpa harus ke meja karyawan, distribusi software massal

**🏫 Pelatihan — Virtual Training**
> Instruktur mengontrol semua workstation peserta, mengirim materi pelajaran sekaligus

**🏛️ Ujian — Computer-Based Test**
> Proktor mengunci layar, memonitor aktivitas, mencegah kecurangan selama ujian

---

## SLIDE 9: Keunggulan Kompetitif

| Aspek | Khwarizmi |
|-------|-----------|
| 💰 **Biaya** | Gratis & open-source — tanpa biaya lisensi |
| 🔧 **Customizable** | Kode sumber terbuka, bisa dimodifikasi sesuai kebutuhan |
| 🌐 **Multi-platform** | Linux & Windows |
| 📍 **Multi-location** | Monitoring banyak lokasi dari satu dashboard |
| 🔒 **Security** | Dukungan PAM, LDAP, SASL authentication |
| 📦 **Lightweight** | Resource ringan, tidak membebani sistem |
| 🌍 **Community** | Berbasis komunitas Veyon yang aktif & terdokumentasi baik |

---

## SLIDE 10: Demo Sekilas

**Live Demo / Screenshot Walkthrough:**

1. 🔑 Login ke Khwarizmi Master
2. 📍 Lihat overview semua komputer per lokasi
3. 🔍 Klik komputer → Remote View / Remote Control
4. 📺 Aktifkan Demo Mode (broadcast layar)
5. 🔒 Kunci semua layar sekaligus
6. 💬 Kirim pesan ke semua user
7. 📸 Ambil screenshot satu/banyak komputer
8. ⚡ Remote power on/off

*[Sisipkan screenshot di sini]*

---

## SLIDE 11: Deployment & Instalasi

**Server/Master:**
```bash
# Debian/Ubuntu
sudo apt install veyon-master veyon-configurator

# RHEL/CentOS
sudo dnf install veyon-master veyon-configurator
```

**Client:**
```bash
# Debian/Ubuntu
sudo apt install veyon-service

# RHEL/CentOS
sudo dnf install veyon-service
```

**Build from source:**
```bash
git clone --recursive https://github.com/veyon/veyon.git
cd veyon && mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr ..
make -j4
sudo make install
```

**Prerequisites:** Qt5, X11, OpenSSL, zlib, libjpeg, PAM, LZO, QCA, LDAP, SASL

---

## SLIDE 12: Spesifikasi Teknis

| Komponen | Detail |
|----------|--------|
| **Bahasa** | C++ (Qt5 framework) |
| **Build System** | CMake |
| **Protokol** | RFB/VNC-based (port 11100) |
| **Auth** | PAM, LDAP, SASL |
| **Kompresi** | LZO |
| **Enkripsi** | OpenSSL |
| **OS Support** | Linux (Debian, Ubuntu, RHEL, CentOS, Arch), Windows |
| **Package Format** | .deb, .rpm, PKGBUILD (AUR) |
| **Lisensi** | GPLv2 |

---

## SLIDE 13: Roadmap

**Fase 1 — Stabilisasi** ✅
- Rebranding Veyon → Khwarizmi
- Setup build system & CI/CD
- Dokumentasi dasar

**Fase 2 — Enhancement** 🔄
- Peningkatan UI/UX
- Plugin tambahan
- Optimasi performa

**Fase 3 — Ekosistem** 📋
- Web-based dashboard
- Mobile companion app
- API untuk integrasi pihak ketiga
- Cloud management console

---

## SLIDE 14: Referensi & Resources

- 🌐 **Upstream Project**: https://veyon.io
- 📖 **Dokumentasi**: https://docs.veyon.io
- 💻 **Source Code**: https://github.com/veyon/veyon
- 📜 **Lisensi**: GNU General Public License v2
- 🌍 **Translasi**: https://app.transifex.com/veyon-solutions/veyon

---

## SLIDE 15: Penutup & Q&A

**Terima Kasih!**

> *"Khwarizmi — Fondasi Monitoring Modern"*

📧 [Email Kontak]
🔗 [Link Repository / Website]

**Q&A** ❓

---

## CATATAN UNTUK TIM

### Yang Perlu Disiapkan Sebelum Presentasi:
1. ✅ Sisipkan **screenshot** aplikasi di Slide 10 (Demo)
2. ✅ Ganti placeholder `[Nama Presenter]`, `[Tanggal]`, `[Email Kontak]`
3. ✅ Siapkan **live demo** jika memungkinkan
4. ✅ Tambahkan **data spesifik** (jumlah client yang di-handle, spesifikasi server, dll.)
5. ✅ Sesuaikan use case dengan **konteks audiens** (pendidikan/perusahaan/pemerintahan)
6. ✅ Tambahkan **slide perbandingan** dengan kompetitor jika diperlukan (e.g., dibandingkan NetSupport, LanSchool)

### Estimasi Durasi:
- ~20-30 menit presentasi
- ~10 menit Q&A
- Total: ~30-40 menit
