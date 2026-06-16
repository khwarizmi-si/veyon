FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
ENV DISPLAY=:1

# --- Build dependencies untuk Veyon (sesuai README) + desktop + noVNC ---
RUN apt-get update && apt-get install -y --no-install-recommends \
    # build tools
    g++ libc6-dev make cmake git pkg-config ca-certificates \
    # Qt5 + dev
    qtbase5-dev qtbase5-private-dev qttools5-dev qttools5-dev-tools \
    libqt5x11extras5-dev \
    # crypto / ssl
    libqca-qt5-2-dev libssl-dev \
    # X11 libs yang dibutuhkan Veyon
    libxtst-dev libxrandr-dev libxinerama-dev libxcursor-dev \
    libxdamage-dev libxfixes-dev libxext-dev \
    # auth / directory
    libpam0g-dev libldap2-dev libsasl2-dev \
    # image / kompresi
    libjpeg-dev zlib1g-dev libpng-dev liblzo2-dev \
    # proses enumerasi (platform linux plugin)
    libprocps-dev \
    # desktop ringan + VNC + noVNC (untuk lihat GUI via browser)
    xfce4 xfce4-terminal dbus-x11 \
    tigervnc-standalone-server tigervnc-common \
    novnc websockify python3 \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

# noVNC entry point (symlink supaya /vnc.html -> index)
RUN ln -sf /usr/share/novnc/vnc.html /usr/share/novnc/index.html || true

WORKDIR /veyon
EXPOSE 6080

COPY start.sh /usr/local/bin/start.sh
RUN chmod +x /usr/local/bin/start.sh

ENTRYPOINT ["/usr/local/bin/start.sh"]
