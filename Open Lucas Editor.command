#!/bin/zsh
set -eu
cd "$(dirname "$0")"
python3 -m http.server 8769 --bind 127.0.0.1 --directory web &
server_pid=$!
trap 'kill "$server_pid" 2>/dev/null || true' EXIT INT TERM
sleep 0.5
open -a 'Google Chrome' 'http://127.0.0.1:8769/editor.html#samples'
wait "$server_pid"
