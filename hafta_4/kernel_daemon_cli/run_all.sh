#!/usr/bin/env bash
set -euo pipefail

# === Basit Ayarlar ===
SERVER_BIN="./server"
KO_PATH="./kernel/pktfilter.ko"
DEV_NODE="/dev/pktfilter"
LOG_DIR="./logs"

mkdir -p "$LOG_DIR"

# Kontroller
[[ -x "$SERVER_BIN" ]] || { echo "[ERR] server bulunamadı ya da çalıştırılamıyor."; exit 2; }
[[ -f "$KO_PATH"    ]] || { echo "[ERR] pktfilter.ko bulunamadı."; exit 2; }

# 1) Kernel modülünü temiz/yükle
if lsmod | awk '{print $1}' | grep -qx pktfilter; then
  echo "[i] pktfilter yüklü; kaldırıyorum..."
  sudo rmmod pktfilter
fi

echo "[i] pktfilter.ko yükleniyor..."
sudo insmod "$KO_PATH"

# /dev düğümünü bekle (varsa izinleri düzelt)
for _ in {1..20}; do [[ -e "$DEV_NODE" ]] && break; sleep 0.1; done
if [[ -e "$DEV_NODE" ]]; then
  sudo chgrp "$(id -gn)" "$DEV_NODE" || true
  sudo chmod 660 "$DEV_NODE" || true
  echo "[i] $DEV_NODE hazır."
else
  echo "[!] $DEV_NODE görünmüyor (udev/devtmpfs gecikmesi olabilir). Devam ediyorum."
fi

# 2) Server'ı arka planda başlat
echo "[i] server başlatılıyor (arka plan)..."
nohup "$SERVER_BIN" >>"$LOG_DIR/server.log" 2>&1 &
echo $! > .server.pid
echo "[ok] server PID $(cat .server.pid)"
echo "[i] Log: $LOG_DIR/server.log"
