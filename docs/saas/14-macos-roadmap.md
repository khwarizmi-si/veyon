# 14 — Roadmap macOS

> Status: rencana teknis awal. macOS dianggap platform tambahan setelah Windows/Linux stabil.
> Disusun 2026-06-27.

## 14.1 Kesimpulan singkat

Khwarizmi bisa dikembangkan untuk macOS, tetapi harus diperlakukan sebagai **porting platform**
yang berdiri sendiri, bukan hanya kompilasi ulang. Aplikasi master/admin relatif lebih mudah
karena berbasis Qt, sedangkan agent/client membutuhkan integrasi OS khusus untuk screen capture,
remote input, service startup, permission, packaging, signing, dan deployment.

Rekomendasi:

1. Stabilkan Windows/Linux terlebih dahulu.
2. Pastikan arsitektur SaaS, device activation, dan licensing tidak tergantung OS tertentu.
3. Mulai macOS dari **Master app**.
4. Lanjut ke **Client read-only monitoring**.
5. Baru tambah **remote control penuh** setelah permission dan packaging matang.

## 14.2 Target platform

| Area | Target awal | Catatan |
|------|-------------|---------|
| Arsitektur CPU | Apple Silicon + Intel | Build universal binary ideal, tetapi Apple Silicon bisa diprioritaskan |
| Versi macOS | macOS 13+ | ScreenCaptureKit lebih layak untuk target modern |
| Format instalasi | `.pkg` | Lebih cocok untuk deployment sekolah/MDM |
| Deployment sekolah | MDM/Jamf/Mosyle | Diperlukan untuk izin dan konfigurasi massal |
| Signing | Developer ID | Dibutuhkan agar instalasi tidak terasa berbahaya |
| Notarization | Wajib untuk distribusi publik | Mengurangi warning Gatekeeper |

## 14.3 Fase porting

### Fase M0 — Audit kesiapan kode

**Tujuan:** tahu bagian mana yang sudah portable dan mana yang perlu platform adapter.

- [ ] Audit dependency Qt di macOS.
- [ ] Audit `Platform*Functions` yang belum punya implementasi macOS.
- [ ] Pisahkan fitur yang bisa jalan di master tanpa service lokal.
- [ ] Cek plugin yang sangat bergantung Linux/Windows.
- [ ] Tentukan minimum macOS version.

**Output:** daftar gap teknis dan keputusan target macOS.

### Fase M1 — Master app macOS

**Tujuan:** guru/admin bisa menjalankan Khwarizmi Master dari Mac.

- [ ] Build `veyon-master` di macOS.
- [ ] Pastikan UI, icon, stylesheet, dan resource Khwarizmi tampil benar.
- [ ] Pastikan koneksi ke client Windows/Linux tetap jalan.
- [ ] Packaging awal `.app`/`.dmg` untuk uji internal.
- [ ] Logging dan crash diagnostics dasar.

**Output:** Mac bisa dipakai sebagai komputer guru/admin untuk mengontrol lab Windows/Linux.

### Fase M2 — Agent macOS read-only

**Tujuan:** Mac bisa masuk daftar device dan dapat dimonitor.

- [ ] Implement device identity macOS.
- [ ] Implement service/daemon startup via `launchd`.
- [ ] Implement screen capture read-only.
- [ ] Implement heartbeat dan device activation per-PC.
- [ ] Tampilkan status permission yang belum diberikan.
- [ ] Packaging `.pkg` untuk pilot terbatas.

**Output:** Mac muncul di dashboard/master dan layarnya bisa dilihat.

### Fase M3 — Remote control macOS

**Tujuan:** guru bisa mengontrol Mac murid setelah izin OS terpenuhi.

- [ ] Implement keyboard/mouse injection.
- [ ] Integrasi permission Accessibility.
- [ ] Handle kondisi permission dicabut.
- [ ] Tambah UI/status "izin belum lengkap".
- [ ] Uji behavior saat lock screen, logout, dan fast user switching.

**Output:** Mac dapat dikontrol dengan batasan permission macOS yang jelas.

### Fase M4 — Deployment sekolah

**Tujuan:** instalasi massal siap untuk lingkungan sekolah.

- [ ] Developer ID signing.
- [ ] Notarization.
- [ ] Installer `.pkg`.
- [ ] MDM profile untuk Screen Recording dan Accessibility bila memungkinkan.
- [ ] Dokumentasi deployment Jamf/Mosyle/manual.
- [ ] Auto-update atau update policy.

**Output:** sekolah bisa deploy Khwarizmi ke Mac secara terkelola.

## 14.4 Komponen macOS yang perlu dibuat

| Komponen | Kebutuhan |
|----------|-----------|
| Platform functions | user/session info, hostname, power/session commands |
| Screen capture | ScreenCaptureKit atau API macOS lain sesuai target versi |
| Remote input | Accessibility permission + event injection |
| Service/daemon | `launchd` agent/daemon |
| Permission UX | status Screen Recording, Accessibility, network access |
| Packaging | `.app`, `.pkg`, signing, notarization |
| Enrollment | device activation SaaS yang sama dengan Windows/Linux |
| Logging | log file + opsi collect diagnostic |

## 14.5 Risiko utama

| Risiko | Dampak | Mitigasi |
|--------|--------|----------|
| Permission Screen Recording/Accessibility | Monitoring/kontrol gagal tanpa izin | Buat status permission jelas dan panduan MDM |
| Signing/notarization | Instalasi terasa tidak dipercaya | Siapkan Apple Developer account sejak awal |
| API macOS berubah | Maintenance meningkat | Target versi modern dan minim API deprecated |
| Remote control saat login/lock screen | Tidak semua skenario bisa dikontrol | Definisikan batasan resmi MVP |
| Build universal binary | CI lebih rumit | Mulai Apple Silicon dulu, universal menyusul |
| Dukungan MDM beragam | Deployment sekolah berbeda-beda | Dokumentasikan Jamf/Mosyle/manual |

## 14.6 Hubungan dengan SaaS

macOS harus memakai arsitektur SaaS yang sama:

- tenant dan user tetap sama;
- device activation tetap per-PC/per-device;
- quota lisensi menghitung Mac sebagai device;
- heartbeat dan config cloud memakai API yang sama;
- fitur yang belum tersedia di macOS harus dilaporkan sebagai capability, bukan error umum.

Contoh capability:

```json
{
  "platform": "macos",
  "capabilities": {
    "screen_view": true,
    "remote_control": false,
    "file_transfer": false,
    "power_control": "limited"
  }
}
```

Dengan model capability, dashboard dan master bisa menampilkan fitur sesuai kemampuan OS tanpa
mengganggu Windows/Linux.

## 14.7 Keputusan awal

1. macOS masuk roadmap setelah Windows/Linux stabil.
2. Target pertama adalah Master app untuk guru/admin.
3. Agent macOS dimulai dari read-only monitoring.
4. Remote control penuh menjadi fase terpisah karena bergantung pada permission Accessibility.
5. SaaS harus mendukung capability per device sejak awal agar multi-platform lebih aman.

## 14.8 Definition of Done MVP macOS

MVP macOS dianggap selesai bila:

- Master app macOS bisa membuka daftar komputer dari konfigurasi/cloud.
- Master app macOS bisa melihat/mengontrol client Windows/Linux yang sudah ada.
- Agent macOS bisa enrollment, heartbeat, dan tampil sebagai device aktif.
- Agent macOS bisa share layar read-only setelah permission diberikan.
- Installer internal tersedia untuk uji pilot.
- Batasan permission dan fitur belum tersedia terdokumentasi jelas.
