# Veyon Dev Container (Docker + noVNC)

Lingkungan development Veyon di dalam Docker, dengan desktop Linux yang bisa
dibuka lewat **browser** (noVNC). Cocok untuk mengubah UI dan melihat hasilnya
dari Mac/Windows tanpa install Qt/cmake di host.

## Alur singkat

```bash
cd docker

# 1. Build image (sekali saja, agak lama: download deps Qt dkk)
docker compose build

# 2. Jalankan container
docker compose up -d

# 3. Buka desktop di browser Mac:
#    http://localhost:6080/vnc.html  -> klik "Connect"

# 4. Masuk ke shell container untuk build Veyon
docker compose exec veyon-dev bash
#   di dalam container:
cd /veyon
mkdir -p build && cd build
cmake -DWITH_QT6=OFF -DWITH_BUNDLED_LIBVNC=ON ..
make -j$(nproc)
./master/veyon-master      # window muncul di desktop noVNC
```

## Login dialog "Veyon Logon"

Saat `veyon-master` dibuka muncul dialog autentikasi Logon. Kredensialnya
memakai user sistem di dalam container:

- **Username:** `root`
- **Password:** `veyon`  (di-set otomatis oleh `start.sh`; ubah via env
  `VEYON_LOGON_PASSWORD` di docker-compose.yml)

## Loop kerja saat mengubah UI

1. Edit file `.ui` / `.cpp` di Mac (mis. `master/src/MainWindow.ui`).
2. Di shell container: `cd /veyon/build && make -j$(nproc)`
3. Jalankan ulang `./master/veyon-master`, lihat perubahan di browser noVNC.

> Source code di-mount (bukan disalin), jadi perubahan di Mac langsung terbaca
> di container. Folder `build/` juga ikut muncul di host.

## Stop / hapus

```bash
docker compose down        # stop container
docker compose down --rmi local   # stop + hapus image
```
