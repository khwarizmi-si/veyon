# 03 — Konektivitas & Relay (Inti Masalah)

Ini bagian **paling sulit** dan paling menentukan kelayakan Full SaaS.

## 3.1 Masalah inti

Veyon: **master → connect → client:11100**. Master harus bisa membuka koneksi TCP ke client.
Di internet, ini gagal karena:
- PC murid di balik **NAT** (tidak punya IP publik).
- **Firewall** memblok inbound.
- IP dinamis.

Maka client **tidak bisa di-dial** dari luar. Solusinya: **balik arahnya** — client yang
**dial-out** ke cloud, lalu cloud menjembatani.

## 3.2 Pola koneksi reverse (dial-out)

```
   Tradisional (LAN):     Master ──connect──► Client:11100   ❌ gagal di internet

   SaaS (reverse):        Client ──dial-out──► Cloud ◄──dial-out── Master
                          (keduanya outbound, NAT-friendly)
```

Karena **outbound** umumnya diizinkan firewall/NAT, kedua sisi menelepon ke cloud. Cloud
mempertemukan (matchmaking) lalu menyalurkan (relay) data antar keduanya.

## 3.3 Dua sub-bidang

### a) Signaling (ringan, selalu nyala)
- Setiap agent menjaga **koneksi persisten** ke cloud (WebSocket over TLS, atau gRPC stream).
- Fungsi: presence (online/offline), terima perintah (lock, message, "mulai sesi lihat layar"),
  kirim telemetri/heartbeat.
- Hemat: koneksi idle murah, bisa ribuan per node.

### b) Media relay (berat, on-demand)
- Saat guru "lihat layar murid", buka stream layar (VNC/RFB) lewat relay.
- **Boros bandwidth** & sensitif latensi → infra terpisah, skalakan horizontal.

## 3.4 Opsi teknologi relay

| Opsi | Cara kerja | Pro | Kontra |
|------|-----------|-----|--------|
| **TCP relay sederhana** | Cloud pipe byte VNC antar 2 koneksi | Mudah, reuse protokol Veyon | Semua trafik via server (mahal egress), latensi |
| **WebRTC (SFU/TURN)** | Media P2P, server hanya signaling+TURN fallback | Hemat (sering P2P), latensi rendah | Kompleks; perlu transcoding layar → video |
| **Self-hosted tunnel (frp/rathole)** | Reverse tunnel expose 11100 ke cloud | Cepat dibangun (pakai tool jadi) | Skalabilitas & multi-tenant perlu kerja ekstra |
| **WebSocket relay** | Bungkus VNC dalam WS, browser-friendly | Cocok untuk master web | Overhead WS, tetap via server |

**Rekomendasi bertahap:**
1. **Awal:** WebSocket/TCP relay (reuse stream VNC Veyon, mudah). Cukup untuk skala kecil & fitur ringan.
2. **Skala:** pindah media ke **WebRTC** (P2P + TURN) agar hemat egress & latensi turun.

## 3.5 Bandwidth & biaya (kalkulasi kasar)

Streaming layar VNC ~ **0.5–3 Mbps per layar aktif** (tergantung resolusi, encoding, gerakan).

- 1 guru melihat **30 thumbnail** low-res @ ~150 kbps = ~4.5 Mbps masuk relay.
- 1 sesi "lihat penuh" @ 2 Mbps.
- Egress cloud ~ $0.08–0.12/GB (AWS/GCP). 2 Mbps × 1 jam ≈ 0.9 GB ≈ **$0.07–0.11/jam/stream**.
- 1000 stream simultan × 1 jam ≈ **$70–110/jam** egress saja → **mahal**.

**Implikasi:** Full SaaS streaming itu cost-driver utama. Mitigasi:
- Thumbnail low-FPS/low-res default; full-res hanya saat dipilih.
- WebRTC P2P (egress nol bila P2P berhasil).
- Relay regional (kurangi latensi & lintas-region).
- Batasi jumlah stream simultan per paket harga (lihat dok 06).

## 3.6 Desain agent dial-out (konsep)

```
agent (di PC murid):
  1. Saat start: baca enrollment token → daftar ke cloud → dapat device-id + sertifikat
  2. Buka koneksi WSS persisten ke signaling.cloud → kirim heartbeat tiap N detik
  3. Tunggu perintah dari cloud:
        - lock/unlock, message, run, screenshot  → eksekusi via Veyon feature
        - "open-stream {session, relay-url}"      → buka koneksi media ke relay,
                                                     pipe framebuffer VNC lokal
  4. Reconnect otomatis bila putus (backoff)
```

Implementasi: bungkus Veyon sebagai library/proses lokal; agent baru bertindak sebagai
**jembatan** antara cloud dan Veyon service lokal (loopback 127.0.0.1:11100).

## 3.7 Keamanan kanal

- Semua koneksi **TLS** (wss/https), verifikasi sertifikat.
- Agent autentikasi ke cloud pakai **sertifikat/token per-device** (bukan key manual Veyon).
- Relay hanya menyambungkan device & master **dalam tenant yang sama** (cegah cross-tenant).
- Media stream idealnya **end-to-end encrypted** (E2EE) agar relay tak bisa "mengintip" layar
  siswa — penting untuk privasi (lihat dok 08). WebRTC mendukung DTLS-SRTP E2EE.

## 3.8 Keputusan kunci yang harus diambil

1. Light SaaS dulu (tanpa relay) atau langsung Full?
2. Relay: WebSocket/TCP (cepat) vs WebRTC (skalabel)?
3. E2EE media wajib? (disarankan ya untuk privasi siswa)
4. Master web (butuh relay WS) atau master desktop (bisa tunnel)?
