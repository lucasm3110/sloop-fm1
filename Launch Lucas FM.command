#!/bin/zsh
set -eu
cd "$(dirname "$0")"
runner="../fm1-emu/target/release/fm1-live"
if [[ ! -x "$runner" ]]; then
  print "Build the sibling fm1-emu repository first: cargo build --release -p fm1-live"
  exit 1
fi
state="private/personal-d72.flash.bin"
[[ -f "$state" ]] || state="build/validated/starter-flash.bin"
if [[ -f "$state" ]]; then
  exec "$runner" --fwsc dist/lucas-fm-0.1.fwsc --flash "$state" --midi none --mhz 240
fi
exec "$runner" --fwsc dist/lucas-fm-0.1.fwsc --midi none --mhz 240
