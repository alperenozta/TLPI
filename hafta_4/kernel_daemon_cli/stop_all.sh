#!/usr/bin/env bash
set -euo pipefail

# --------- Ayarlar ---------
SERVER_BIN="./server"
CLIENT_BIN="./client"
MOD_NAME="pktfilter"
DEV_NODE="/dev/pktfilter"
KERNEL_DIR="./kernel"

LOG_DIR="./logs"
PID_DIR="./.pids"

CLEAN_BUILD=false     # --clean-build ile etkin
DEEP_CLEAN=false      # --deep ile etkin
KEEP_LOGS=false       # --keep-logs ile logları sakla

log(){ printf "[%(%F %T)T] %s\n" -1 "$*"; }

usage(){
  cat <<'USAGE'
Kullanım:
  ./stop_all.sh [opsiyonlar]

Opsiyonlar:
  --keep-logs     : logs/ klasörünü silme
  --clean-build   : make clean ve kernel out-of-tree temizliği yap
  --deep          : --clean-build + derlenmiş çıktıların derin temizliği (ko, o, .cmd vs.)
  -h, --help      : bu yardımı göster
USAGE
}

for arg in "$@"; do
  case "$arg" in
    --keep-logs)    KEEP_LOGS=true ;;
    --clean-build)  CLEAN_BUILD=true ;;
    --deep)         CLEAN_BUILD=true; DEEP_CLEAN=true ;;
    -h|--help)      usage; exit 0 ;;
    *)
      echo "Bilinmeyen argüman: $arg"; usage; exit 2 ;;
  esac
done

# Script'i proje kökünden çalıştır
cd "$(dirname "$0")"

abs_path(){ readlink -f "$1" 2>/dev/null || python3 -c "import os,sys;print(os.path.abspath(sys.argv[1]))" "$1"; }

kill_by_pidfile(){
  local name="$1" bin="$2" pidfile="$3"
  if [[ -f "$pidfile" ]]; then
    local pid; pid="$(cat "$pidfile" || true)"
    if [[ -n "${pid:-}" ]] && kill -0 "$pid" 2>/dev/null; then
      # Güvenlik: gerçekten bu projedeki bin mi?
      local exe; exe="$(readlink -f "/proc/$pid/exe" 2>/dev/null || true)"
      local want; want="$(abs_path "$bin")"
      if [[ "$exe" == "$want" ]]; then
        log "$name (PID $pid) SIGTERM..."
        kill "$pid" || true
        for _ in {1..20}; do
          kill -0 "$pid" 2>/dev/null || break
          sleep 0.1
        done
        if kill -0 "$pid" 2>/dev/null; then
          log "$name (PID $pid) hala yaşıyor, SIGKILL..."
          kill -9 "$pid" || true
        fi
      else
        log "Uyarı: $pidfile içindeki PID $pid farklı bir exe çalıştırıyor ($exe). Atlıyorum."
      fi
    else
      log "$name için PID aktif değil (pidfile var ama süreç yok)."
    fi
    rm -f "$pidfile"
  else
    log "$name için pid dosyası yok: $pidfile (isimden taramayı deneyeceğim)"
    # İkinci şans: isim tabanlı ama exe eşlemesiyle güvenli
    local want; want="$(abs_path "$bin")"
    local base; base="$(basename "$bin")"
    if command -v pgrep >/dev/null 2>&1; then
      while read -r pid; do
        [[ -z "$pid" ]] && continue
        local exe; exe="$(readlink -f "/proc/$pid/exe" 2>/dev/null || true)"
        if [[ "$exe" == "$want" ]]; then
          log "$name (PID $pid) SIGTERM..."
          kill "$pid" || true
        fi
      done < <(pgrep -x "$base" || true)
    fi
  fi
}

remove_module(){
  if lsmod | awk '{print $1}' | grep -qx "$MOD_NAME"; then
    log "Kernel modülü kaldırılıyor: $MOD_NAME"
    if [[ $EUID -ne 0 ]]; then
      sudo rmmod "$MOD_NAME"
    else
      rmmod "$MOD_NAME"
    fi
  else
    log "$MOD_NAME modülü yüklü değil."
  fi
}

# --------- 1) Server & Client'ı durdur ---------
mkdir -p "$PID_DIR"
kill_by_pidfile "server" "$SERVER_BIN" "$PID_DIR/server.pid"
kill_by_pidfile "client" "$CLIENT_BIN" "$PID_DIR/client.pid"

# --------- 2) Kernel modülünü kaldır ---------
remove_module

# /dev düğümü kaldıysa (genelde rmmod ile silinir), temizle
if [[ -e "$DEV_NODE" ]]; then
  log "$DEV_NODE halen mevcut, siliyorum (genelde gerekmemeli)"
  if [[ $EUID -ne 0 ]]; then sudo rm -f "$DEV_NODE"; else rm -f "$DEV_NODE"; fi
fi

# --------- 3) Loglar ve pid dosyaları ---------
if [[ "$KEEP_LOGS" == false ]]; then
  log "Loglar temizleniyor: $LOG_DIR"
  rm -rf "$LOG_DIR"
else
  log "Loglar saklandı: $LOG_DIR"
fi
log "PID klasörü temizleniyor: $PID_DIR"
rm -rf "$PID_DIR"

# --------- 4) (opsiyonel) Build temizliği ---------
if [[ "$CLEAN_BUILD" == true ]]; then
  if [[ -f Makefile ]]; then
    log "Top-level 'make clean' çalıştırılıyor"
    make clean || true
  fi
  # Kernel OOT temizliği
  if [[ -d "$KERNEL_DIR" ]]; then
    if [[ -d "/lib/modules/$(uname -r)/build" ]]; then
      log "Kbuild temizliği: $KERNEL_DIR"
      make -C "/lib/modules/$(uname -r)/build" M="$(abs_path "$KERNEL_DIR")" clean || true
    fi
    if [[ "$DEEP_CLEAN" == true ]]; then
      log "Derin kernel temizliği (obj/cmd/mod dosyaları)"
      rm -f "$KERNEL_DIR"/{Module.symvers,modules.order} \
            "$KERNEL_DIR"/*.o "$KERNEL_DIR"/.*.cmd "$KERNEL_DIR"/*.mod* \
            "$KERNEL_DIR"/*.ko 2>/dev/null || true
    fi
  fi
  log "Build temizliği tamam."
fi

log "Tüm süreçler durduruldu ve modül kaldırıldı. ✅"
