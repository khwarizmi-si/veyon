# 06 — Billing & Pricing

## 6.1 Model penetapan harga

| Model | Cara | Cocok |
|-------|------|-------|
| **Per-device / tahun** ⭐ | Bayar per PC murid terkelola | Sekolah (jumlah PC stabil) — paling umum & mudah dipahami |
| Per-seat (guru) | Bayar per akun guru | Lembaga dgn sedikit guru, banyak device |
| Per-lokasi/kelas | Flat per lab | Sederhana untuk sekolah kecil |
| Tiered (paket) | Basic/Pro/Enterprise dgn batas fitur & device | Kombinasi semua di atas |

**Rekomendasi:** **per-device/tahun, dikemas dalam paket bertingkat**. Sekolah berpikir dalam
"jumlah komputer lab", jadi per-device intuitif.

## 6.2 Contoh struktur paket (ilustrasi, sesuaikan riset pasar)

| Fitur | Free/Trial | Basic | Pro | Enterprise |
|-------|-----------|-------|-----|------------|
| Device | s/d 10 | s/d 50 | s/d 500 | unlimited |
| Monitoring & lock & message | ✅ | ✅ | ✅ | ✅ |
| Demo & remote control (LAN) | ✅ | ✅ | ✅ | ✅ |
| Kontrol lintas-internet (relay) | ❌ | ❌ | ✅ (kuota) | ✅ |
| Multi-lokasi/cabang | ❌ | 1 | banyak | banyak |
| SSO (Google/MS) | ❌ | ❌ | ✅ | ✅ |
| Laporan & audit | dasar | dasar | lengkap | lengkap + ekspor |
| Dukungan | komunitas | email | prioritas | dedicated + SLA |
| Harga (contoh) | gratis | Rp X/device/th | Rp Y/device/th | nego |

> Harga aktual butuh riset: daya beli sekolah Indonesia, anggaran BOS, pesaing. Pertimbangkan
> harga lebih rendah dari pemain global (LanSchool/NetSupport) sebagai positioning lokal.

## 6.3 Metering (yang diukur untuk penagihan/penegakan)

- Jumlah **device aktif** (terdaftar & heartbeat dalam periode).
- (Full SaaS) **menit/GB streaming relay** — bila ingin batasi/biaya berbasis pemakaian.
- Jumlah **seat guru**.
- Penegakan kuota: tolak enroll device ke-(N+1) bila melebihi paket; beri grace + notifikasi.

## 6.4 Integrasi pembayaran

| Wilayah | Provider | Catatan |
|---------|----------|---------|
| Indonesia | **Midtrans / Xendit / DOKU** | Dukung VA, e-wallet, kartu, QRIS — penting untuk sekolah lokal |
| Global | **Stripe** | Langganan, invoice, proration matang |
| Merchant of Record | Paddle / LemonSqueezy | Mereka urus pajak global (kalau jual lintas negara) |

**Untuk sekolah Indonesia:** banyak transaksi **manual/PO/transfer + invoice** (anggaran sekolah).
Sediakan opsi **invoice/PO offline** + aktivasi manual, jangan hanya kartu kredit.

## 6.5 Alur langganan

```
Trial (14–30 hari, tanpa kartu) → pilih paket → checkout (Midtrans/Stripe / atau invoice PO)
   → lisensi aktif (license-service set quota & valid_until)
   → reminder sebelum expiry → perpanjang/auto-renew
   → jika lewat: grace period → mode read-only/suspend (jangan langsung matikan kelas berjalan)
```

## 6.6 Lisensi & penegakan teknis

- `license-service` menerbitkan **lisensi tertanda (signed)** berisi: tenant, paket, kuota device,
  fitur aktif, `valid_until`.
- Agent & master verifikasi tanda tangan → fitur premium (relay, multi-lokasi) hanya aktif bila
  lisensi mengizinkan.
- **Offline grace:** agent simpan lisensi terakhir; bila cloud tak terjangkau sementara, tetap
  jalan s/d X hari (penting untuk sekolah dengan internet tidak stabil).

## 6.7 Pertimbangan model bisnis open-core

Karena basis GPL (open source), pikirkan **open-core**:
- **Inti (agent/Veyon)** tetap open source (wajib GPL).
- **Nilai berbayar** = layanan cloud (manajemen, relay, dashboard, SSO, support, lisensi terpusat).
- Pelanggan bayar untuk **kenyamanan & layanan**, bukan untuk software-nya semata.
- Ini model sah & umum (mis. GitLab, Mattermost). Lihat dok 07 untuk batas lisensi.
