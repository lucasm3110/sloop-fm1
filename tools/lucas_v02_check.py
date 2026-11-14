#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Actual-package first boot, audible effects and bypass checks (no real device)."""
import argparse,json,os,struct,subprocess,wave,hashlib
from pathlib import Path
from PIL import Image
from lucas_metrics import report,symbols
p=argparse.ArgumentParser();p.add_argument('directory',type=Path);p.add_argument('--runner',required=True);a=p.parse_args();out=a.directory
fw=out/'felucca.play.fwsc';elf=out/'felucca.elf';syms=symbols(elf);boot_late=0
def run(name,script,ms=10500):
    env=dict(os.environ,FM1_STRICT='1',FM1_LCD_TIMED='1',FM1_BENCH_KEYS=script,FM1_BENCH_PANEL=str(out/(name+'.ppm')),FM1_BENCH_WAV=str(out/(name+'.pcm')),FM1_DUMPRAM=str(out/(name+'.ram')))
    env.pop('FM1_BENCH_NOKEYS',None)
    with (out/(name+'.log')).open('w') as f:subprocess.run([a.runner,'--fwsc',str(fw),'--midi','none','--mhz','240','--bench',str(ms)],env=env,stdout=f,stderr=f,check=True)
    Image.open(out/(name+'.ppm')).crop((370,42,850,522)).resize((240,240),Image.Resampling.NEAREST).save(out/(name+'.lcd.png'))
    raw=(out/(name+'.pcm')).read_bytes();ram=(out/(name+'.ram')).read_bytes()
    with wave.open(str(out/(name+'.wav')),'wb') as w:w.setparams((2,2,44100,0,'NONE','not compressed'));w.writeframes(raw)
    r=report(elf,out/(name+'.ram'));r['firmware_sha256']=hashlib.sha256(fw.read_bytes()).hexdigest()
    (out/(name+'.metrics.json')).write_text(json.dumps(r,indent=2))
    assert r['audio']['shed_count']==0 and (r['audio']['late']<=boot_late or name=='boot'),(name,r['audio'])
    def mem(n):addr,size=syms[n];return ram[addr-0x1c00000:addr-0x1c00000+size]
    s=struct.unpack('<%dh'%(len(raw)//2),raw);assert name=='boot' or max(map(abs,s))>100,(name,'silent')
    print(name,r['audio'],flush=True);return s,mem
_,_=run('boot','',3000)
boot_late=json.loads((out/'boot.metrics.json').read_text())['audio']['late']
assert boot_late<=1,('startup misses',boot_late)
# No restored flash and no sample overlay: everything must come from the package.
s,mem=run('preloaded','3000:LFO,3180:LFO^,3600:EDIT,3780:EDIT^,4300:e0:+1,4800:EDIT,4980:EDIT^')
lib=mem('lf_library');stride=len(lib)//10;assert all(lib[i*stride+53] for i in range(10))
run('starter-beat','3000:PLAY,3180:PLAY^,4000:SEQ,4180:SEQ^')
note=',4000:C4,7500:C4^'
reference,_=run('fx-dry','3000:HOME,3180:HOME^'+note)
for name,section,knobs in [('eq',0,'3400:e2:+110'),('drive',1,'3400:e2:+100'),('mod',2,'3400:e2:+127,3500:e4:+87'),('grain',3,'3400:e2:+127,3500:e5:+127'),('delay',4,'3400:e2:+100'),('reverb',5,'3400:e2:+72')]:
    script=f'3000:FX,3180:FX^,3300:e0:+{section},'+knobs+note
    wet,mem=run('fx-'+name,script)
    assert mem('lf_cpu_guard')==b'\0',name+' bypassed by CPU guard'
    # Same clock/note schedule: require substantial waveform change over the note and tail.
    lo,hi=round(4.5*44100)*2,min(len(wet),len(reference),round(9*44100)*2)
    diff=sum(abs(wet[i]-reference[i]) for i in range(lo,hi))/(hi-lo)
    assert diff>20,(name,diff)
    print(f'PASS: {name} changes captured firmware audio, mean absolute difference {diff:.1f}',flush=True)
    if name in ['eq','grain']:
        dry,mem=run('fx-'+name+'-bypass',script.replace(note,',3700:EDIT,3880:EDIT^'+note))
        idx=5 if name=='eq' else 14
        assert mem('lf')[idx]==0,(name,'bypass not applied')
print('PASS: ten preloaded imports, audible starter drum machine, six real audio effects and accessible bypass')
