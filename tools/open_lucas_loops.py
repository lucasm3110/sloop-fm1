#!/usr/bin/env python3
"""Open the local four-loop emulator; never talks to physical MIDI hardware."""
from pathlib import Path
import shutil, subprocess, sys, urllib.request, webbrowser, zipfile
root=Path(__file__).resolve().parent.parent
url='http://127.0.0.1:8774/'
try:
 with urllib.request.urlopen(url,timeout=1) as r:
  if b'lucas' not in r.read(4096).lower():raise RuntimeError('Port 8774 is in use by another app.')
except (OSError, urllib.error.URLError):
 runtime=root/'build/loops-browser';runtime.mkdir(parents=True,exist_ok=True)
 with zipfile.ZipFile(root/'dist/Lucas-Loops-0.4-browser.zip') as z:z.extractall(runtime)
 personal=root/'private/lucas-fm-0.4-personal.fwsc'
 if personal.exists():shutil.copyfile(personal,runtime/'firmware.fwsc')
 with (runtime/'server.log').open('ab') as log:
  subprocess.Popen([sys.executable,'-m','http.server','8774','--bind','127.0.0.1','--directory',str(runtime)],stdout=log,stderr=log,start_new_session=True)
 import time
 for _ in range(40):
  try:
   with urllib.request.urlopen(url,timeout=.2):break
  except OSError:time.sleep(.1)
 else:raise RuntimeError('Could not start Lucas Loops. See build/loops-browser/server.log.')
webbrowser.open(url)
print('Lucas Loops is open at '+url)
