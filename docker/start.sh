#!/usr/bin/env bash
# Menyalakan desktop XFCE di display virtual :1 dan menyajikannya via noVNC (port 6080).
# Buka http://localhost:6080/vnc.html di browser Mac untuk melihat desktop Linux.
set -e

export HOME="${HOME:-/root}"
export DISPLAY=:1
GEOMETRY="${GEOMETRY:-1600x900}"

# Set password user 'root' untuk autentikasi Veyon Logon (default: veyon).
# Bisa di-override lewat env VEYON_LOGON_PASSWORD di docker-compose.yml.
echo "root:${VEYON_LOGON_PASSWORD:-veyon}" | chpasswd 2>/dev/null || true

# Bersihkan lock lama bila container di-restart
rm -f /tmp/.X1-lock /tmp/.X11-unix/X1 2>/dev/null || true

# Jalankan X virtual + VNC server.
# VNC hanya listen di localhost DALAM container (-localhost yes); yang diekspos
# ke Mac cuma port 6080 lewat websockify. Tanpa password karena akses lokal saja.
vncserver "$DISPLAY" -geometry "$GEOMETRY" -depth 24 \
    -localhost yes -SecurityTypes None --I-KNOW-THIS-IS-INSECURE >/tmp/vnc.log 2>&1

# Pastikan sesi XFCE jalan di display itu
cat > "$HOME/.xsession" <<'EOF'
#!/bin/sh
exec startxfce4
EOF
chmod +x "$HOME/.xsession"
DISPLAY=:1 startxfce4 >/tmp/xfce.log 2>&1 &

# Jembatan websocket -> VNC supaya bisa dibuka di browser
echo ">>> Desktop siap. Buka di browser Mac:  http://localhost:6080/vnc.html"
echo ">>> Source code Veyon ter-mount di:     /veyon"
echo ">>> Build:  cd /veyon && mkdir -p build && cd build && cmake -DWITH_QT6=OFF -DWITH_BUNDLED_LIBVNC=ON .. && make -j\$(nproc)"
echo ">>> Run:    ./master/veyon-master   (dari folder build)"
websockify --web=/usr/share/novnc 6080 localhost:5901
