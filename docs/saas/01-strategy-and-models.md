# 01 — Strategi & Model SaaS

## 1.1 Visi produk

Khwarizmi Surveillance System sebagai **platform manajemen kelas berbasis cloud** untuk
sekolah/lembaga pendidikan: guru memantau & mengarahkan komputer siswa, admin sekolah
mengelola perangkat & lisensi dari satu dashboard, tanpa harus mengurus server sendiri.

## 1.2 Target pasar

| Segmen | Karakteristik | Kebutuhan |
|--------|---------------|-----------|
| Sekolah K-12 | Lab komputer, Chromebook/Windows | Monitoring kelas, kunci layar, kirim tugas |
| SMK / vokasi | Lab teknis besar | Demo layar guru, distribusi materi |
| Bimbel / kursus | Multi-cabang | Manajemen lintas lokasi, lisensi terpusat |
| Universitas | Lab besar, multi-fakultas | Skala besar, integrasi SSO kampus |

**Diferensiasi vs LAN tradisional (Veyon murni):** tanpa setup server lokal, akses dari mana saja,
manajemen multi-cabang terpusat, lisensi & billing otomatis, update terkelola.

## 1.3 Dua model arsitektur

### Model A — Light SaaS (Hybrid) ⭐ Rekomendasi awal

Cloud menangani **management plane**; **kontrol layar real-time tetap di LAN**.

```
   Cloud (kamu kelola)                     Sekolah (LAN)
   ┌────────────────────┐                  ┌─────────────────────────────┐
   │ Akun & SSO          │   config/        │  Master (web/desktop)       │
   │ Lisensi             │◄──lisensi───────►│      │ kontrol layar (LAN) │
   │ Inventaris device   │   telemetri      │      ▼                     │
   │ Distribusi config   │                  │  PC Murid (agent)           │
   │ Dashboard & laporan │                  └─────────────────────────────┘
   └────────────────────┘
```

**Yang di-cloud:** registrasi sekolah, akun guru/admin, SSO, daftar perangkat, push konfigurasi
(grup komputer, access control), aktivasi lisensi, dashboard penggunaan, laporan.

**Yang tetap LAN:** koneksi master↔murid (VNC), streaming layar, kontrol input — persis Veyon
sekarang. Tidak ada beban bandwidth streaming di cloud.

**Kelebihan:** cepat dibangun, biaya cloud rendah, latensi kontrol tetap rendah (LAN), risiko
privasi lebih kecil (layar tidak melewati cloud).
**Kekurangan:** guru & murid harus satu LAN; tidak bisa kontrol lintas internet.

### Model B — Full SaaS (Cloud Relay)

Semua trafik (termasuk streaming layar) lewat cloud relay → guru bisa di mana saja.

```
   PC Murid (agent) ──dial-out──► Cloud Relay ◄──── Master (web)
                                  (signaling + media relay)
```

**Kelebihan:** kontrol dari mana saja, cocok untuk pembelajaran jarak jauh / multi-lokasi.
**Kekurangan:** rekayasa berat (relay skalabel), biaya egress bandwidth besar, latensi lebih
tinggi, isu privasi lebih besar (layar siswa melewati server kamu). Lihat dok 03.

## 1.4 Rekomendasi strategi

1. **Fase MVP = Light SaaS.** Memberi nilai jual SaaS nyata (manajemen terpusat, lisensi,
   dashboard) tanpa harus memecahkan masalah relay.
2. **Validasi pasar dulu** — apakah sekolah bersedia berlangganan? Uji dengan 2–3 sekolah pilot.
3. **Tambah konektivitas cloud bertahap** (dok 03): mulai fitur ringan (lock, pesan, monitoring
   thumbnail) lewat relay, baru streaming penuh.
4. **Hybrid permanen juga sah** — banyak produk kelas tetap LAN-first dengan cloud management;
   tidak semua harus Full SaaS.

## 1.5 Pesaing & referensi (untuk riset)

- Veyon (upstream, gratis/open source, LAN) — basis kita
- NetSupport School, Faronics Insight, LanSchool, GoGuardian, Securly, Hapara, Impero
- Pelajari model harga & fitur cloud mereka untuk positioning.

> Catatan: GoGuardian/Securly/Hapara fokus Chromebook + cloud filtering; NetSupport/LanSchool
> mirip Veyon (LAN + sebagian cloud). Posisi kita: **open-core, harga terjangkau, lokal (Indonesia)**.
