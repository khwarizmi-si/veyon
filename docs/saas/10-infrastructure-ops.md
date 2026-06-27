# 10 — Infrastruktur & Operasional

## 10.1 Pilihan cloud

| Provider | Pertimbangan |
|----------|--------------|
| **AWS / GCP / Azure** | Region Jakarta tersedia (kepatuhan UU PDP + latensi); layanan lengkap |
| **Cloudflare** | R2 (storage murah, egress gratis), Workers, TURN — bagus untuk relay/biaya egress |
| **Lokal (Biznet/IDCloudHost/Alibaba ID)** | Data di Indonesia, dukungan lokal |

**Saran:** mulai 1 region (Jakarta) untuk control plane; tambah region untuk relay bila perlu
latensi rendah. Pertimbangkan **Cloudflare R2** untuk storage (egress gratis → hemat biaya
distribusi installer & screenshot).

## 10.2 Topologi deployment (MVP → skala)

```
MVP (hemat):
  - 1 VM/container: monolith API + signaling
  - 1 PostgreSQL terkelola (managed)
  - 1 Redis terkelola
  - Object storage (R2/S3)
  - CDN untuk installer

Skala:
  - Kubernetes: API (autoscale), signaling (sticky/sharded), relay (per-region, autoscale)
  - PostgreSQL HA + read replica
  - Redis cluster / NATS untuk pub-sub
  - Load balancer + WAF
  - Multi-region relay
```

## 10.3 CI/CD

- Git → CI (build, test, SAST/SCA, sign artifact) → CD ke staging → prod (approval gate).
- **Agent:** build multi-OS (Windows/Linux), **sign installer**, publikasikan ke CDN + endpoint update.
- Infra sebagai kode (Terraform) → reproducible, review-able.

## 10.4 Observability

| Pilar | Tool (opsi) |
|-------|-------------|
| Logs | Loki / ELK / Cloud logging |
| Metrics | Prometheus + Grafana |
| Traces | OpenTelemetry + Tempo/Jaeger |
| Uptime/alert | Grafana alerting / PagerDuty / Better Stack |
| Error tracking | Sentry |

Metrik kunci: device online, sesi aktif, latensi relay, error rate API, egress bandwidth,
biaya per tenant.

## 10.5 Skalabilitas — titik tekan

1. **Signaling:** ribuan koneksi WS persisten → Go + sharding + sticky routing; presence di Redis.
2. **Relay media:** cost & bandwidth driver utama → autoscale per region, prioritaskan WebRTC P2P.
3. **DB:** index `tenant_id`, RLS, read replica untuk dashboard/laporan.
4. **Heartbeat storm:** ribuan agent heartbeat → batch/throttle, jitter interval.

## 10.6 Estimasi biaya cloud (kasar, untuk perencanaan)

| Komponen | Skala kecil (~10 sekolah, ~500 device) | Catatan |
|----------|----------------------------------------|---------|
| Compute (API+signaling) | ~$50–150/bln | 1–2 VM kecil |
| PostgreSQL managed | ~$30–80/bln | |
| Redis managed | ~$20–50/bln | |
| Storage + CDN | ~$10–30/bln | installer + screenshot (R2 egress gratis) |
| Egress relay (Full SaaS) | **variabel besar** | lihat dok 03 §3.5 — bisa dominan |
| **Total (Light SaaS)** | **~$110–310/bln** | tanpa streaming relay = murah |

> **Insight:** Light SaaS biayanya kecil & dapat diprediksi. Full SaaS biaya **egress streaming**
> bisa meledak → harus dibatasi paket harga & dioptimasi (P2P, low-res default).

## 10.7 Reliabilitas & SLA

- Internet sekolah sering tak stabil → **offline grace** untuk lisensi & config (agent simpan
  state terakhir).
- Kontrol LAN (Light SaaS) tetap jalan walau cloud down (nilai jual!).
- Definisikan SLA per paket (mis. 99.5% uptime control plane untuk Pro/Enterprise).
- Backup harian terenkripsi + uji restore berkala.

## 10.8 Operasional tim kecil

- Mulai dengan **layanan terkelola** (managed DB/Redis/K8s) → kurangi beban ops.
- Otomatisasi: IaC, CI/CD, alert. Hindari "pet servers".
- Dokumentasi runbook (deploy, rollback, incident).
