#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Check a .lucaspack transfer through the compiled firmware's emulated USB MIDI.
No physical device. Native runner and a freshly generated matching base flash required.
"""
import argparse,base64,json,os,subprocess
from pathlib import Path
from fm1_sample_upload import pack7
from lucas_pack import validate_pack,OFFSETS
p=argparse.ArgumentParser();p.add_argument('directory',type=Path);p.add_argument('--runner',required=True);p.add_argument('--pack',type=Path,default=Path('assets/loops-cc0/park-in-d.lucaspack'));a=p.parse_args();out=a.directory
Path('work').mkdir(exist_ok=True)
pack=validate_pack(json.loads(a.pack.read_text()));sends=[];t=3000

def send(cmd,data,gap=70):
 global t
 b=bytes([0xf0,0x7d,0x46,0x4c,cmd]+list(data)+[0xf7]);sends.append(f'{t}:{b.hex()}');t+=gap
send(1,[])
for s in pack['slots']:
 k=s['slot'];d=base64.b64decode(s['data']);send(11,[k],400)
 for off in range(512,len(d),256):send(12,[k,off&127,(off>>7)&127,(off>>14)&127]+pack7(d[off:off+256]))
 send(13,[k]+pack7(d[:512]),500)
send(15,[])
script=out/'upload.usb.txt';script.write_text(';'.join(sends));flash=out/'upload.flash.bin';flash.write_bytes((out/'fresh-base.bin').read_bytes())
env=dict(os.environ,FM1_USB_SEND='@'+str(script),FM1_BENCH_KEYS=f'{t+1000}:HOME,{t+1200}:HOME^,{t+1300}:e0:+4,{t+1800}:EDIT,{t+2000}:EDIT^,{t+2500}:EDIT,{t+2700}:EDIT^',FM1_BENCH_WAV=str(out/'upload.pcm'),FM1_BENCH_PANEL=str(out/'upload.ppm'))
env.pop('FM1_BENCH_NOKEYS',None)
with (out/'upload.log').open('w') as f:subprocess.run([a.runner,'--fwsc',str(out/'felucca.fwsc'),'--flash',str(flash),'--usb','--midi','none','--mhz','240','--shot',str(t+5000)],env=env,stdout=f,stderr=f,check=True)
b=flash.read_bytes()
for s in pack['slots']:
 d=base64.b64decode(s['data']);off=OFFSETS[s['slot']];assert b[off:off+480]==d[:480] and b[off+512:off+len(d)]==d[512:],('USB transfer mismatch',s['slot'])
print(f'PASS: {len(pack["slots"])} slots: 480-byte FSMP headers and all metadata/audio transferred byte-for-byte through compiled firmware (32 unused padding bytes excluded), {len(sends)} USB commands, {t+5000} emulated ms')
