#!/bin/zsh
set -eu
cd "$(dirname "$0")"
runner="../fm1-emu/target/release/fm1-live"
if [[ ! -x "$runner" ]]; then
  print "Build the sibling fm1-emu repository first: cargo build --release -p fm1-live"
  exit 1
fi
mkdir -p private
package="dist/lucas-fm-0.4.fwsc"
state="private/public-0.4-session.bin"
if [[ -f "private/lucas-fm-0.4-personal.fwsc" ]]; then
  package="private/lucas-fm-0.4-personal.fwsc"
  state="private/personal-0.4-session.bin"
fi
exec "$runner" --fwsc "$package" --flash "$state" --midi none --mhz 240
