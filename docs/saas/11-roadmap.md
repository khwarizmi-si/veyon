# 11 — Roadmap & Eksekusi

> Estimasi waktu mengasumsikan tim kecil (2–4 orang). Sesuaikan dengan realita timmu.
> Disusun 2026-06-18.

## 11.1 Fase

### Fase 0 — Validasi (2–4 minggu)
**Tujuan:** pastikan ada pasar sebelum membangun.
- [ ] Wawancara 5–10 sekolah: masalah, anggaran, kesediaan bayar
- [ ] Cek kepatuhan awal (UU PDP) dengan ahli hukum
- [ ] Tentukan model harga awal & paket
- [ ] Putuskan: Light SaaS dulu (rekomendasi)
- **Output:** keputusan go/no-go + 2–3 sekolah pilot

### Fase 1 — Light SaaS MVP (2–3 bulan)
**Tujuan:** produk berlangganan nyata, kontrol tetap LAN.
- [ ] Backend: auth + tenant + device enrollment + config + license (monolith)
- [ ] Dashboard web: registrasi sekolah, daftar device, grup, lisensi, laporan dasar
- [ ] Agent: dial-out enrollment + heartbeat + ambil config dari cloud (bungkus Veyon)
- [ ] Master (desktop existing) ambil daftar komputer & config dari cloud
- [ ] Billing dasar (Midtrans/Stripe + opsi invoice/PO)
- [ ] SSO Google (opsional di fase ini)
- [ ] Dokumen legal: Privacy Policy, ToS, DPA
- **Output:** sekolah bisa daftar, deploy agent, kelola lisensi & device dari cloud; kontrol LAN

### Fase 2 — Konektivitas Cloud (2–4 bulan)
**Tujuan:** fitur lintas-internet ringan.
- [ ] Signaling service (WebSocket presence + command delivery)
- [ ] Perintah lewat cloud: lock, unlock, message, screenshot, monitoring thumbnail (low-res)
- [ ] Relay WebSocket/TCP untuk thumbnail/monitoring
- [ ] Master web (mulai)
- **Output:** guru bisa monitor & lock lintas jaringan tanpa harus satu LAN

### Fase 3 — Full Streaming (3–5 bulan)
**Tujuan:** lihat/kontrol layar penuh dari mana saja.
- [ ] Relay media skalabel (WebRTC/SFU) + TURN
- [ ] E2EE stream layar
- [ ] Optimasi bandwidth (low-res default, full on-demand), batas kuota per paket
- [ ] Relay multi-region
- **Output:** Full SaaS

### Fase 4 — Skala & Matang (berkelanjutan)
- [ ] Multi-region, HA, autoscale
- [ ] SSO Microsoft, SAML, integrasi LMS
- [ ] Audit & laporan lanjutan, ekspor
- [ ] Pen-test, sertifikasi keamanan, compliance audit
- [ ] Support tier, SLA, onboarding terkelola

## 11.2 Milestone teknis pemicu keputusan

| Milestone | Pertanyaan validasi |
|-----------|---------------------|
| Akhir Fase 0 | Apakah sekolah mau bayar? Berapa? |
| Akhir Fase 1 | Apakah pilot aktif pakai & perpanjang? |
| Sebelum Fase 3 | Apakah permintaan "lintas internet" cukup besar untuk membenarkan biaya relay? |

## 11.3 Tim ideal (minimum)

| Peran | Fokus |
|-------|-------|
| Backend/Cloud engineer | API, signaling, relay, infra |
| Agent/C++ engineer | Modifikasi Veyon, dial-out, integrasi |
| Frontend engineer | Dashboard & master web |
| Product/Sales (kamu) | Pasar, pilot sekolah, harga, legal |

Di awal, 1–2 orang full-stack bisa menutup Fase 0–1 bila pragmatis (pakai managed services).

## 11.4 Risiko utama & mitigasi

| Risiko | Mitigasi |
|--------|----------|
| Biaya egress streaming meledak | Light SaaS dulu; WebRTC P2P; batasi kuota; low-res default |
| Masalah hukum privasi siswa | Privacy-by-design, consent, E2EE, ahli hukum sejak awal |
| Sekolah tak mau bayar | Validasi Fase 0; harga lokal; opsi invoice/PO |
| Kompleksitas relay | Bertahap; mulai fitur ringan; pakai lib jadi (Pion/LiveKit) |
| Beban maintain fork Veyon | Bungkus via plugin/IPC, minim ubah inti → mudah merge upstream |
| Internet sekolah tak stabil | Offline grace; kontrol LAN tetap jalan saat cloud down |

## 11.5 Prinsip eksekusi

1. **Validasi sebelum bangun** (Fase 0 jangan dilewati).
2. **Light SaaS dulu** — nilai nyata, biaya & risiko rendah.
3. **Bungkus, jangan bedah** Veyon — jaga batas GPL & kemudahan update.
4. **Privasi & keamanan sebagai fitur**, bukan tempelan.
5. **Iterasi dengan sekolah pilot** — bangun yang mereka mau bayar.

---

## Penutup

Dokumen ini adalah peta. Mulai dari **Fase 0 (validasi)** dan **Fase 1 (Light SaaS MVP)**.
Detail teknis tiap area ada di dokumen 01–10. Perbarui dokumen ini seiring keputusan diambil.
