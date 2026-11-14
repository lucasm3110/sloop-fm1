#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Build the fixed ten-loop library (D major, 72 BPM, one bar each).
Run tests/lucas_loop_render.c into build/loops04 first. --kit uses local WAVs
and writes only private/loops04/personal.lucaspack; no hardware is contacted.
"""
import argparse,json,struct,wave
from pathlib import Path
from lucas_pack import prepare,pack,validate_pack
from sampleio import read_any_wav,resample
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--kit',type=Path);a=p.parse_args()
root=Path(__file__).resolve().parents[1];work=root/'build/loops04';names=['warm pad','soft air','glass tide','sub bass','round bass','soft keys','tiny bells','slow pluck','soft beat','light steps']
files=[work/(n.replace(' ','-')+'.wav') for n in names[:8]]
kit=a.kit or root/'assets/samples-cc0/KIT'
hits=['01 KICK.wav','03 SNARE.wav','05 HAT.wav'] if a.kit else ['10_kick_BDrumNew_hit_v5.wav','11_snare_Snare2_HitSN_v7.wav','14_hihat_HitC_v3.wav']
out=root/('private/loops04' if a.kit else 'assets/lucas-loops');out.mkdir(parents=True,exist_ok=True)
for variant in range(2):
 n=147000;mix=[0.0]*n
 patterns=[[0,8] if not variant else [0,6,10],[4,12],list(range(0 if not variant else 1,16,2))]
 for file,steps,gain in zip(hits,patterns,[.8,.45,.22]):
  rate,x=read_any_wav(kit/file);x=resample(x,rate,44100)
  for step in steps:
   at=round(step*n/16)
   for j,v in enumerate(x[:n-at]):mix[at+j]+=v*gain
 path=(out if a.kit else work)/('rhythm-'+str(variant)+'.wav');files.append(path)
 with wave.open(str(path),'wb') as w:
  w.setparams((1,2,44100,0,'NONE','not compressed'));w.writeframes(struct.pack('<%dh'%n,*[int(max(-1,min(1,x))*26000) for x in mix]))
entries=[]
for i,(name,file) in enumerate(zip(names,files)):
 e=prepare(dict(name=name,file=str(file),type='rhythmic' if i>=8 else 'tonal',root=None if i>=8 else 'D',bpm=72,bars=1,seconds=240/72,follow_key=False,follow_bpm=False),root,2,72,16000)
 e['source']=str(file.relative_to(root)) if file.is_relative_to(root) else file.name
 entries.append(e)
result=dict(format='lucas-fm-loops',version=1,name='Four layers / D major / 72',key='D',bpm=72,entries=[{k:v for k,v in e.items() if k!='data'} for e in entries],slots=pack(entries),preserve_slots=[])
validate_pack(result);dest=out/('personal.lucaspack' if a.kit else 'four-layers.lucaspack');dest.write_text(json.dumps(result,separators=(',',':'))+'\n')
print(dest);print('Ten loops:',sum(s['bytes'] for s in result['slots']),'bytes in four sample slots')
