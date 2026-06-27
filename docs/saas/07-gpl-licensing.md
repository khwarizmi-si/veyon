# 07 — Lisensi GPL (Kepatuhan)

> ⚠️ **Disclaimer:** Ini ringkasan teknis-praktis, **bukan nasihat hukum**. Untuk keputusan
> komersial besar, konsultasikan dengan pengacara yang paham lisensi open source.

## 7.1 Fakta dasar

- Veyon (dan fork Khwarizmi) berlisensi **GPLv2**.
- Kita sudah: fork, rebrand, **mempertahankan copyright asli** (Tobias Junghans) + mencantumkan
  **"Based on Veyon"** + menyebut **GNU GPL v2** (lihat About dialog & README). ✅

## 7.2 Aturan GPLv2 yang relevan untuk SaaS

| Situasi | Kewajiban GPLv2 |
|---------|-----------------|
| **Menjalankan di server (SaaS) tanpa distribusi biner ke user** | GPLv2 **TIDAK** mewajibkan buka source backend (GPLv2 bukan AGPL — tidak ada "klausa jaringan") |
| **Mendistribusikan agent/biner ke device sekolah** | **WAJIB** sediakan source code (termasuk modifikasimu) dari bagian berlisensi GPL kepada penerima |
| **Membuat backend proprietary di sekitar Veyon** | Boleh, **asalkan bukan "karya turunan" yang ter-link** dengan kode GPL |

**Poin kunci:** karena agent (Veyon yang dimodifikasi) **kamu kirim/instal ke PC sekolah**, itu
**distribusi** → kamu **wajib** menyediakan source code agent (modifikasi GPL) ke mereka.

## 7.3 "Karya turunan" vs "karya terpisah" — batas aman

Agar backend cloud (proprietary) **tidak tertular** copyleft GPL:

✅ **AMAN (terpisah):**
- Backend berkomunikasi dengan agent lewat **antarmuka jarak-arm's-length**: REST/gRPC/WebSocket,
  proses terpisah, IPC, pertukaran data.
- Backend & agent adalah **program berbeda** yang berkomunikasi via protokol, bukan satu binari.

❌ **BERISIKO (jadi turunan, wajib GPL):**
- Mengompilasi/me-link kode GPL Veyon **ke dalam** binari proprietary-mu.
- Memodifikasi sumber Veyon lalu menggabungnya statis dengan kode tertutup.

**Aturan praktis:** jaga **agent (GPL)** dan **backend (proprietary)** sebagai **dua program
terpisah** yang bicara lewat jaringan/IPC. "Agent bridge" yang memanggil Veyon lewat
loopback/IPC = aman. Jangan link library Veyon ke kode tertutup.

## 7.4 Yang HARUS kamu lakukan (checklist kepatuhan)

- [ ] **Publikasikan source code** fork agent (modifikasi GPL) — repo publik (sudah ada di GitHub) memenuhi ini.
- [ ] Sertakan **teks lisензи GPLv2 penuh** (`COPYING`/`LICENSE`) di distribusi agent. (Veyon sudah punya.)
- [ ] **Pertahankan copyright & notice** penulis asli (sudah ✅).
- [ ] Di installer/about, sediakan **cara mendapatkan source** (link ke repo). Tambahkan "Written
      Offer" / link source pada agent yang didistribusikan.
- [ ] Tandai modifikasimu (changelog/commit history sudah memenuhi).
- [ ] Jangan klaim Veyon sebagai buatanmu 100% — atribusi jujur (sudah ✅).

## 7.5 Yang BOLEH (tidak melanggar)

- ✅ Menjual **layanan SaaS** berbasis Veyon (jualan layanan, bukan software).
- ✅ Backend cloud, dashboard, relay, billing = **proprietary** (program terpisah).
- ✅ Rebrand nama/logo (GPL tidak melindungi merek; tapi hormati merek "Veyon" milik orang lain —
      jangan mengaku sebagai Veyon resmi).
- ✅ Open-core: inti open, layanan cloud berbayar.

## 7.6 Risiko & catatan

- **Trademark "Veyon":** nama & logo Veyon bisa jadi merek dagang Veyon Solutions. Kita sudah
  rebrand ke "Khwarizmi" → aman, selama tidak menyiratkan afiliasi resmi. Cukup sebut "based on Veyon".
- **AGPL?** Veyon GPLv2, bukan AGPL → tidak ada kewajiban rilis source backend SaaS. Tapi bila
  suatu saat menyertakan komponen AGPL lain, aturan berubah — cek tiap dependensi.
- **Dependensi pihak ketiga:** cek lisensi semua lib (Qt = LGPL/komersial, QCA, libvnc, dll).
  Qt LGPL punya syarat sendiri (dynamic linking, dll) — patuhi terpisah.

## 7.7 Ringkasan satu kalimat

> SaaS berbasis Veyon **boleh & legal**: jual layanan cloud (proprietary, terpisah), **tetapi
> agent GPL yang kamu distribusikan ke sekolah wajib disertai source code & lisensinya** —
> yang sudah terpenuhi dengan repo publik + atribusi yang kita pasang.
