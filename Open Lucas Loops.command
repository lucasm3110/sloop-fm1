#!/bin/zsh
set -eu
cd "$(dirname "$0")"
python3 tools/open_lucas_loops.py
