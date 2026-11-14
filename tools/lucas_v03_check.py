#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Lucas 0.3 actual-package first boot, audible effects and bypass checks (no real device)."""
import argparse,json,os,struct,subprocess,wave,hashlib
from pathlib import Path
from PIL import Image
from lucas_metrics import report,symbols
p=argparse.ArgumentParser();p.add_argument('directory',type=Path);p.add_argument('--runner',required=True);a=p.parse_args();out=a.directory
fw=out/'felucca.play.fwsc';elf=out/'felucca.elf';syms=symbols(elf);boot_late=0
def run(name,script,ms=11500):
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
s,mem=run('preloaded','2200:HOME,2380:HOME^,2500:e0:+4,3000:EDIT,3180:EDIT^,3600:EDIT,3780:EDIT^,4300:e0:+1,4800:EDIT,4980:EDIT^')
lib=mem('lf_library');stride=len(lib)//10;assert all(lib[i*stride+53] for i in range(10))
run('starter-beat','3000:PLAY,3180:PLAY^,4000:SEQ,4180:SEQ^')
# 127 detents take about 1 s of physical quadrature edges; let them settle before bypass.
note=',5000:C4,8500:C4^'
reference,_=run('fx-dry','3000:HOME,3180:HOME^'+note)
for name,section,knobs in [('eq',0,'3400:e2:+110'),('drive',1,'3400:e2:+100'),('mod',2,'3400:e2:+127,3500:e4:+87'),('grain',3,'3400:e2:+127,3500:e5:+127'),('delay',4,'3400:e2:+100'),('reverb',5,'3400:e2:+72')]:
    script=f'3000:FX,3180:FX^,3300:e1:+{section},'+knobs+note
    wet,mem=run('fx-'+name,script)
    assert mem('lf_cpu_guard')==b'\0',name+' bypassed by CPU guard'
    # Same clock/note schedule: require substantial waveform change over the note and tail.
    lo,hi=round(5.5*44100)*2,min(len(wet),len(reference),round(10*44100)*2)
    diff=sum(abs(wet[i]-reference[i]) for i in range(lo,hi))/(hi-lo)
    assert diff>20,(name,diff)
    print(f'PASS: {name} changes captured firmware audio, mean absolute difference {diff:.1f}',flush=True)
    if name in ['eq','grain']:
        dry,mem=run('fx-'+name+'-bypass',script.replace(note,',4600:EDIT,4780:EDIT^'+note))
        idx=5 if name=='eq' else 14
        assert mem('lf')[idx]==0,(name,'bypass not applied')
print('PASS: ten preloaded imports, audible starter drum machine, six real audio effects and accessible bypass')

# Named controls must reach real engine parameters in the compiled package.
_,mem=run('lfo-controls','3000:LFO,3180:LFO^,3300:e1:+1,3400:e3:+24,5000:C4,8500:C4^')
assert mem('lf_page')==bytes([9])
assert struct.unpack_from('<h',mem('trk'),10*2)[0]==1  # P_LWAVE
assert struct.unpack_from('<h',mem('trk'),13*2)[0]==24 # P_LD_PIT
_,mem=run('arp-controls','3000:ARP,3180:ARP^,3300:e1:+1,5000:C4,8500:C4^')
assert mem('lf_page')==bytes([10])
assert struct.unpack_from('<h',mem('trk'),17*2)[0]==1 # P_AMODE
print('PASS: physical LFO/ARP buttons and ALGORITHM operate actual sound parameters')
# A BPM-only clip can move both ways through its valid choices with single detents.
for name,extra,flags in [('free','',0),('bpm',',4300:e1:+1',2)]:
    _,mem=run('match-'+name,'2200:HOME,2380:HOME^,2500:e0:+4,3000:EDIT,3180:EDIT^,3300:e0:+9,3800:e1:-1'+extra+note)
    assert mem('lf_pick')==bytes([9])
    assert mem('lf')[33+9]==flags,('BPM-only matching navigation',name)
print('PASS: BPM-only clip skips unavailable KEY matching in both directions')
