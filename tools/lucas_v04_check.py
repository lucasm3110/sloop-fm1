#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Actual four-loop firmware: boot, controls, persistence and strict playback."""
import argparse,os,json,subprocess,struct,hashlib
from pathlib import Path
from PIL import Image
from lucas_metrics import report,symbols
p=argparse.ArgumentParser();p.add_argument('directory',type=Path);p.add_argument('--runner',required=True);a=p.parse_args();d=a.directory;s=symbols(d/'felucca.elf')
def run(name,script,ms=10000,flash=None,mhz=240):
 env=dict(os.environ,FM1_STRICT='1',FM1_LCD_TIMED='1',FM1_BENCH_KEYS=script,FM1_BENCH_PANEL=str(d/(name+'.ppm')),FM1_BENCH_WAV=str(d/(name+'.pcm')),FM1_DUMPRAM=str(d/(name+'.ram')))
 env.pop('FM1_BENCH_NOKEYS',None)
 cmd=[a.runner,'--fwsc',str(d/'felucca.play.fwsc'),'--midi','none','--mhz',str(mhz),'--bench',str(ms)]
 if flash:cmd+=['--flash',str(flash)]
 with (d/(name+'.log')).open('w') as log:subprocess.run(cmd,env=env,stdout=log,stderr=log,check=True)
 ram=(d/(name+'.ram')).read_bytes()
 def mem(n):off,size=s[n];return ram[off-0x1c00000:off-0x1c00000+size]
 r=report(d/'felucca.elf',d/(name+'.ram'));r.update(script=script,mhz=mhz,sha256=hashlib.sha256((d/'felucca.play.fwsc').read_bytes()).hexdigest(),scene=list(mem('lf')[:9]))
 pcm=(d/(name+'.pcm')).read_bytes();samples=struct.unpack('<%dh'%(len(pcm)//2),pcm);r['peak']=max(map(abs,samples),default=0)
 (d/(name+'.json')).write_text(json.dumps(r,indent=2)+'\n');Image.open(d/(name+'.ppm')).crop((370,42,850,522)).resize((240,240),Image.Resampling.NEAREST).save(d/(name+'.png'))
 assert r['audio']['pitched_voices']==[0,0,0] and r['audio']['late']<=1 and r['audio']['max_us']<5804,r
 print(name,r,flush=True);return mem,r
m,r=run('home','');assert m('lf_page')==b'\0' and r['scene'][:4]==[0,3,5,8]
m,r=run('chooser','2200:e6:+1');assert m('lf_page')==b'\1' and r['scene'][0]==0
m,r=run('cancel','2200:e6:+1,3000:HOME,3180:HOME^');assert r['scene'][0]==0 and m('lf_page')==b'\0'
allloops='2200:PLAY,2380:PLAY^,3000:e0:+1,3400:EDIT,3580:EDIT^,4000:e0:+1,4400:EDIT,4580:EDIT^,5000:e0:+1,5400:EDIT,5580:EDIT^'
for mhz in [240,100]:
 m,r=run('four-loops-'+str(mhz),allloops,20000,mhz=mhz);assert r['peak']>1000 and r['scene'][8]==15 and int.from_bytes(m('lf_running'),'little')==1
f=d/'saved-scene.bin'
if f.exists():f.unlink()
m,r=run('saved',allloops+',6000:e0:-3,6600:e6:+1,7000:EDIT,7180:EDIT^,11000:PLAY,11180:PLAY^,12000:SAVE,12180:SAVE^',14000,f)
assert r['scene'][0]==1 and int.from_bytes(m('lf_running'),'little')==0
m,r=run('reboot','',5000,f);assert r['scene'][0]==1 and r['scene'][8]==15 and int.from_bytes(m('lf_running'),'little')==0
print('PASS: final compiled package, chooser/cancel, four audible loops at 100/240 MHz, bar-boundary loading, SAVE/reboot persistence; no synth voices')
