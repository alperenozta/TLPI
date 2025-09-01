#!/usr/bin/env bash
set -euo pipefail

CLIENT_BIN="./client"
[[ -x "$CLIENT_BIN" ]] || { echo "[ERR] client bulunamadı ya da çalıştırılamıyor."; exit 2; }

exec "$CLIENT_BIN"
