# Khwarizmi Surveillance System — Rencana SaaS

Dokumentasi perencanaan untuk mengubah **Khwarizmi Surveillance System** (fork dari Veyon,
GPLv2) dari aplikasi LAN menjadi produk **SaaS** (Software as a Service) untuk sekolah.

> Status: **dokumen perencanaan** — belum implementasi. Disusun 2026-06-18.

## Daftar isi

| # | Dokumen | Isi |
|---|---------|-----|
| 00 | [README](README.md) | Indeks + ringkasan eksekutif (file ini) |
| 01 | [Strategi & Model](01-strategy-and-models.md) | Visi, pasar, Light vs Full SaaS |
| 02 | [Arsitektur](02-architecture.md) | Komponen sistem, diagram alur data |
| 03 | [Konektivitas & Relay](03-connectivity-and-relay.md) | Inti masalah LAN→cloud, desain relay |
| 04 | [Tenancy, Data & Identitas](04-tenancy-data-and-identity.md) | Multi-tenant, model data, auth, enrollment |
| 05 | [Backend & Tech Stack](05-backend-and-tech-stack.md) | Layanan, API, pilihan teknologi |
| 06 | [Billing & Pricing](06-billing-and-pricing.md) | Model langganan, metering, harga |
| 07 | [Lisensi GPL](07-gpl-licensing.md) | Kepatuhan GPLv2 untuk SaaS |
| 08 | [Privasi, Legal & Etika](08-privacy-legal-ethics.md) | UU PDP, GDPR, COPPA, consent |
| 09 | [Keamanan](09-security.md) | Arsitektur keamanan, ancaman |
| 10 | [Infrastruktur & Ops](10-infrastructure-ops.md) | Cloud, skalabilitas, biaya, observability |
| 11 | [Roadmap](11-roadmap.md) | Tahapan, milestone, tim, estimasi |
| 12 | [Device Activation](12-device-activation.md) | Registrasi, approval, token aktivasi, dan kuota per PC |
| 13 | [Scope SaaS](13-saas-scope.md) | Prioritas fitur SaaS MVP dan tahap lanjutan |
| 14 | [Roadmap macOS](14-macos-roadmap.md) | Strategi porting macOS, fase MVP, permission, dan packaging |

## Ringkasan eksekutif

**Tantangan inti:** Veyon adalah aplikasi **LAN** — master terhubung langsung ke PC murid
via VNC (port 11100) dalam satu jaringan. SaaS membutuhkan akses lintas internet, multi-tenant,
dan berlangganan. PC murid di balik NAT tidak bisa dijangkau langsung dari cloud.

**Solusi bertahap:**

1. **Light SaaS (hybrid)** ⭐ — Cloud mengelola akun, lisensi, inventaris device, distribusi
   konfigurasi, dan dashboard. **Kontrol layar tetap berjalan di LAN.** Realistis sebagai MVP.
2. **Full SaaS** — Semuanya di cloud termasuk streaming layar lewat relay. Rekayasa berat
   (relay/TURN, bandwidth, latensi).

**Rekomendasi:** mulai dari Light SaaS, validasi pasar, lalu tambah konektivitas cloud bertahap.

**Risiko terbesar (non-teknis):** kepatuhan privasi untuk monitoring siswa (UU PDP / GDPR / COPPA)
dan kewajiban lisensi GPLv2 untuk agent yang didistribusikan. Lihat dok 07 & 08.

## Cara membaca

- Baru mulai? Baca **01 → 02 → 03** untuk gambaran teknis besar.
- Fokus bisnis/legal? Baca **06 → 07 → 08**.
- Siap eksekusi? Baca **11 (Roadmap)**.
