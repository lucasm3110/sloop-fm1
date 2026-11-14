#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Repeatable actual-firmware checks; never talks to physical MIDI or flash.
python3 tools/lucas_emulator.py build/tested --runner ../fm1-emu/target/release/fm1-live
Requires felucca.fwsc, .elf, .bin in the directory; writes fresh emulator state.
"""
import argparse,base64,json,os,struct,subprocess,wave,hashlib
from pathlib import Path
from PIL import Image
from lucas_metrics import report,symbols
p=argparse.ArgumentParser(description=__doc__);p.add_argument('directory',type=Path);p.add_argument('--runner',type=Path,required=True);p.add_argument('--screens',action='store_true');p.add_argument('--stress',action='store_true');a=p.parse_args();out=a.directory
fw=out/'felucca.fwsc';elf=out/'felucca.elf';syms=symbols(elf)

def run(name,events=(),ms=8000,mhz=100,strict=False,flash=None,usb=False):
    env=dict(os.environ,FM1_BENCH_KEYS=','.join(f'{t}:{v}' for t,v in sorted(events)) or '3000:HOME,3200:HOME^',FM1_BENCH_PANEL=str(out/(name+'.ppm')),FM1_BENCH_WAV=str(out/(name+'.pcm')),FM1_DUMPRAM=str(out/(name+'.ram')),FM1_BENCH_JSON=str(out/(name+'.timing.json')),FM1_LCD_TIMED='1')
    env.pop('FM1_BENCH_NOKEYS',None);env.pop('FM1_STRICT',None)
    if strict:env['FM1_STRICT']='1'
    args=[str(a.runner),'--fwsc',str(fw),'--mhz',str(mhz),'--bench',str(ms),'--midi','none']
    if flash is not None:
        f=out/(name+'.flash.bin');f.write_bytes(flash);args+=['--flash',str(f)]
    if usb:env['FM1_USB_WAV']=str(out/(name+'.usb.pcm'));args+=['--usb']
    with (out/(name+'.log')).open('w') as f:subprocess.run(args,env=env,stdout=f,stderr=f,check=True)
    Image.open(out/(name+'.ppm')).save(out/(name+'.png'))
    Image.open(out/(name+'.ppm')).crop((370,42,850,522)).resize((240,240),Image.Resampling.NEAREST).save(out/(name+'.lcd.png'))
    r=report(elf,out/(name+'.ram'));r.update(mhz=mhz,strict=strict,script=env['FM1_BENCH_KEYS'],firmware_sha256=hashlib.sha256(fw.read_bytes()).hexdigest())
    b=(out/(name+'.pcm')).read_bytes();s=struct.unpack('<%dh'%(len(b)//2),b);r['audio']['pcm_peak']=max(map(abs,s),default=0)
    with wave.open(str(out/(name+'.wav')),'wb') as w:w.setparams((2,2,44100,0,'NONE','not compressed'));w.writeframes(b)
    ram=(out/(name+'.ram')).read_bytes()
    def mem(n):addr,size=syms[n];return ram[addr-0x1c00000:addr-0x1c00000+size]
    for n in ['lf_page','lf_note','lf','lf_layers','song']:
        if n in syms:r[n]=list(mem(n))
    (out/(name+'.metrics.json')).write_text(json.dumps(r,indent=2));print(name,r['audio'],flush=True)
    return r,ram

def tap(ev,t,k):ev.extend([(t,k),(t+180,k+'^')])
# Generate a fresh flash from THIS package. Reusing an older complete flash also
# restores its code and invalidates a firmware comparison (restore_flash in fm1-emu).
basefile=out/'fresh-base.bin'
if basefile.exists():basefile.unlink()
with (out/'fresh-boot.log').open('w') as f:subprocess.run([str(a.runner),'--fwsc',str(fw),'--flash',str(basefile),'--mhz','100','--bench','3000','--midi','none'],env=dict(os.environ,FM1_BENCH_NOKEYS='1'),stdout=f,stderr=f,check=True)
base=bytearray(basefile.read_bytes())
for s in json.loads(Path('assets/loops-cc0/park-in-d.lucaspack').read_text())['slots']:
 d=base64.b64decode(s['data']);off=[0xa0000,0xb4000,0xc8000,0xe7000][s['slot']];base[off:off+len(d)]=d
(out/'starter-flash.bin').write_bytes(base)
if a.screens:
 cases={}
 for name,button in [('home','HOME'),('key','SCL'),('mix','GLO'),('sequence','SEQ')]:
  ev=[];tap(ev,3000,button);cases[name]=ev
 ev=[];tap(ev,3000,'ARP');tap(ev,3800,'ARP');tap(ev,4600,'C4');cases['play']=ev
 ev=[];tap(ev,3000,'HOME');tap(ev,4500,'EDIT');cases['sound']=ev
 ev=[];tap(ev,3000,'LFO');tap(ev,3800,'EDIT');ev.append((4300,'e0:+1'));tap(ev,5000,'EDIT');ev.append((5500,'e0:+5'));tap(ev,6500,'EDIT');ev.append((7100,'e0:-6'));cases['loops']=ev
 ev=[];tap(ev,3000,'LFO');ev.append((3500,'e0:+6'));tap(ev,4500,'LFO');cases['loop-settings']=ev
 ev=[];tap(ev,3000,'FX');ev.append((3500,'e0:+3'));ev.append((4500,'e2:+65'));tap(ev,6000,'C4');cases['effects']=ev
 for name,ev in cases.items():
  r,ram=run(name,ev,9000,flash=base)
  expected={'home':0,'play':1,'sound':2,'sequence':3,'loops':4,'mix':5,'key':6,'effects':7,'loop-settings':9}[name]
  assert r['lf_page']==[expected],(name,'wrong destination',r['lf_page'])
  if name=='play':assert r['lf_note'][0]==62,('wrong played root',r['lf_note'])
 # Record three chords, stop, retain undo, save and load an actual flash section.
 ev=[];tap(ev,3000,'SEQ');tap(ev,3600,'REC');tap(ev,4200,'C4');tap(ev,5000,'F4');tap(ev,5800,'G4');tap(ev,6800,'REC');tap(ev,7400,'PLAY')
 # Let note releases decay before flash save.
 tap(ev,11000,'SAVE');tap(ev,11700,'EDIT');tap(ev,12300,'EDIT')
 r,ram=run('record-save',ev,15000,flash=base)
 saved=(out/'record-save.flash.bin').read_bytes();assert saved!=base,'no flash save'
 ev=[];tap(ev,3000,'SAVE');ev.append((3500,'e0:+1'));tap(ev,4300,'EDIT');tap(ev,4900,'EDIT');tap(ev,5500,'SEQ');tap(ev,6100,'PLAY')
 r,ram=run('reload-play',ev,10000,flash=saved);assert r['audio']['pcm_peak']>10,'saved sequence silent'
if a.stress:
 run('idle-strict100',ms=6000,strict=True)
 # Four decoded imports, all shared new effects, 8 pitched MIDI notes + repeated drums.
 ev=[];tap(ev,3000,'LFO')
 for i in range(4):
  if i:ev.append((3500+i*1200,'e0:+1'))
  tap(ev,4000+i*1200,'EDIT')
 tap(ev,9000,'FX');tap(ev,10500,'FX');ev.append((11000,'e5:+1'))
 ev.append((12000,'e0:+1'));ev.append((13000,'e2:+80'))
 ev.append((15000,'e0:+1'));ev.append((16000,'e2:+80'));ev.append((16000,'e4:+40'))
 ev.append((18000,'e0:+1'));ev.append((19000,'e2:+100'));tap(ev,21000,'FX');ev.append((22000,'e2:+80'));ev.append((22000,'e4:+100'))
 for i,n in enumerate([48,52,55,59,62,65,69,72]):ev.append((24000+i*40,f'm90{n:02x}64'))
 for t in range(24500,32000,250):ev.append((t,'m992464992a50992650'))
 for mhz in [100,240]:run('load-strict'+str(mhz),ev,33000,mhz,True,base,True)
