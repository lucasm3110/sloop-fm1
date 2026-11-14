#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/host
python3 - <<'PY'
import base64,json
from pathlib import Path
pack=json.loads(Path('assets/lucas-loops/four-layers.lucaspack').read_text())
flash=bytearray(b'\xff'*0x100000)
for s in pack['slots']:
    start=[0xA0000,0xB4000,0xC8000,0xE7000][s['slot']]
    data=base64.b64decode(s['data']);flash[start:start+len(data)]=data
Path('build/host/lucas-samples.bin').write_bytes(flash)
PY
${CC:-cc} -O1 -g -w -fsanitize=address,undefined -fno-sanitize=shift-base,signed-integer-overflow -fno-sanitize-recover=all -Ibuild/gen -Ifirmware/src -Ifirmware/hal tests/lucas_ui_test.c -lm -o build/host/lucas_test
build/host/lucas_test build/host/lucas-samples.bin ${LUCAS_SCREENSHOTS:-}
python3 -m unittest discover -s tests -p lucas_pack_test.py
node tests/lucas_web_test.mjs
