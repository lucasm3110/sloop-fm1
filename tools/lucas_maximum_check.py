#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Extreme VOICE + four-loop + maximum-effect test of actual compiled firmware.
Run lucas_emulator.py --stress first; this uses its script and matching fresh flash.
"""
import argparse,json,os,subprocess,shutil
from pathlib import Path
from lucas_metrics import report
p=argparse.ArgumentParser();p.add_argument('directory',type=Path);p.add_argument('--runner',required=True);p.add_argument('--preset',type=int,default=2);p.add_argument('--voices',type=int,choices=[7,8],default=7);a=p.parse_args();out=a.directory
j=json.loads((out/'load-strict240.metrics.json').read_text())
s=j['script'].replace('e2:+80','e2:+127').replace('e4:+40','e4:+87').replace('e2:+100','e2:+127').replace('e4:+100','e4:+127')+',17500:e6:+3,19000:e3:+77,19000:e4:+57,19000:e5:+127,22000:e3:+127,22000:e5:+27'
for n in [62,65,69,72]:s=s.replace(f'm90{n:02x}64',f'm91{n:02x}64')
s=s.replace('m932464932a50932650','m992464992a50992650')
if a.voices==7:s=s.replace(',24280:m914864','')
name='maximum-'+str(a.voices)
# Select VOICE on parts 1/2, POLY explicitly, maximum delay/reverb sends.
usb='8000:f07d464c1b00f7;8200:f07d464c080500f7;8400:f07d464c1b01f7;8600:f07d464c080500f7;8700:f07d464c1f00250040f7;8800:f07d464c1f01250040f7;8900:f07d464c1f00237f40f7;9000:f07d464c1f01237f40f7;9100:f07d464c1f00247f40f7;9200:f07d464c1f01247f40f7'
usb=usb.replace('080500',f'0805{a.preset:02x}')
(out/(name+'-input.json')).write_text(json.dumps(dict(keys=s,usb=usb),indent=2))
env=dict(os.environ,FM1_STRICT='1',FM1_LCD_TIMED='1',FM1_BENCH_KEYS=s,FM1_USB_SEND=usb,FM1_DUMPRAM=str(out/(name+'.ram')),FM1_USB_WAV=str(out/(name+'.usb.pcm')))
Path('work').mkdir(exist_ok=True)
f=out/(name+'.flash.bin');f.write_bytes((out/'starter-flash.bin').read_bytes())
with (out/(name+'.log')).open('w') as log:subprocess.run([a.runner,'--fwsc',str(out/'felucca.fwsc'),'--flash',str(f),'--usb','--midi','none','--mhz','240','--shot','33000'],env=env,stdout=log,stderr=log,check=True)
shutil.copyfile('work/panel.ppm',out/(name+'.ppm'))
r=report(out/'felucca.elf',out/(name+'.ram'));(out/(name+'.metrics.json')).write_text(json.dumps(r,indent=2));print(json.dumps(r,indent=2))
assert r['audio']['pitched_voices']==[4,a.voices-4,0],'held notes were shed'
assert r['audio']['shed_count']==0,'overload killed a pitched voice'
assert r['audio']['late']==0,'missed audio deadline'
print(f'PASS: {a.voices} heavy voices retained, four imports, original send buses and USB audio; new master FX yield visibly when necessary')
