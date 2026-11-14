#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Prepare ten imported loops for Lucas FM; reuses SLOOP's IMA ADPCM encoder.
python3 tools/lucas_pack.py collection.json output.lucaspack --key D --bpm 72
Optional --flash BASE.bin writes a separate emulator flash with these sample slots.
Never connects to or flashes a physical device.
"""
import argparse, base64, hashlib, json, math, shutil, struct, subprocess, tempfile, wave, zlib
from pathlib import Path
from sampleio import ima_encode, read_any_wav, resample
SLOT_SIZE=0x14000
OFFSETS=[0xA0000,0xB4000,0xC8000,0xE7000]
NOTES=['C','C#','D','D#','E','F','F#','G','G#','A','A#','B']
KINDS=['texture','tonal','rhythmic']

def key(value):
    if value is None: return 255
    if isinstance(value,int) and 0<=value<12: return value
    try: return NOTES.index(str(value).upper())
    except ValueError: raise ValueError('root must be C, C#, D, D#, E, F, F#, G, G#, A, A#, or B')

def prepare(entry,base,target_key,target_bpm,rate):
    kind=KINDS.index(entry.get('type','texture'))
    root=key(entry.get('root')); bpm=float(entry.get('bpm',0)); bars=int(entry.get('bars',0))
    follow_key=bool(entry.get('follow_key',kind==1)); follow_bpm=bool(entry.get('follow_bpm',kind==2))
    if kind==0 and (follow_key or follow_bpm): raise ValueError('textures must keep natural pitch and speed')
    if follow_key and root==255: raise ValueError('Follow Key needs declared source root')
    if follow_bpm and not 30<=bpm<=300: raise ValueError('Follow BPM needs source BPM from 30 to 300')
    if kind==2 and not 1<=bars<=16: raise ValueError('rhythmic loops need 1..16 bars')
    if not 0<=bars<=16: raise ValueError('invalid bar count')
    start=float(entry.get('start',0)); duration=float(entry.get('seconds',4))
    if not math.isfinite(start+duration) or start<0 or not 0.1<=duration<=30: raise ValueError('trim must be 0.1..30 seconds')
    path=(base/entry['file']).resolve()
    pitch=2**((((target_key-root+6)%12)-6)/12) if follow_key else 1
    tempo=target_bpm/bpm if follow_bpm else 1
    # Pitch via resampling, tempo restored independently with ffmpeg's WSOLA atempo.
    filters=f'asetrate=44100*{pitch:.10f},aresample=44100,atempo={tempo/pitch:.10f}'
    with tempfile.TemporaryDirectory(prefix='lucas-audio-') as tmp:
        out=Path(tmp)/'audio.wav'
        subprocess.run(['ffmpeg','-v','error','-i',str(path),'-ss',str(start),'-t',str(duration),
                        '-ac','1','-ar','44100','-f','wav',str(Path(tmp)/'trim.wav')],check=True)
        subprocess.run(['ffmpeg','-v','error','-i',str(Path(tmp)/'trim.wav'),'-af',filters,'-ar',str(rate),str(out)],check=True)
        sr,x=read_any_wav(out)
    if kind==2:
        expected=round(rate*240*bars/(target_bpm if follow_bpm else bpm))
        if abs(len(x)-expected)>rate*.05: raise ValueError(f'{entry["name"]}: duration must describe {bars} whole bars')
        x=x[:expected]+[0.0]*max(0,expected-len(x))
    peak=max(abs(v) for v in x)
    if not peak: raise ValueError('empty/silent audio')
    x=[v*0.5/peak for v in x]  # -6 dBFS before the per-layer gain
    fade=min(round(rate*.015),len(x)//8)
    # Equal-length boundary taper; keeps rhythmic bar length exact.
    for i in range(fade):
        x[i]*=i/fade; x[-1-i]*=i/fade
    samples=[max(-32768,min(32767,round(v*32767))) for v in x]
    data,state=ima_encode(samples,0)
    return dict(name=str(entry['name']).encode('ascii','replace').decode()[:11],kind=kind,
                root=target_key if follow_key else root,bpm=round(target_bpm if follow_bpm else bpm),bars=bars,
                flags=int(follow_key)|int(follow_bpm)<<1,rate=rate,n=len(samples),data=data,
                source=str(entry['file']),sha256=hashlib.sha256(path.read_bytes()).hexdigest())

def pack(entries,available=(0,1,2,3)):
    if not 1<=len(entries)<=10: raise ValueError('one to ten entries per collection')
    banks=[[] for _ in range(4)]; sizes=[128]*4
    for id,e in sorted(enumerate(entries),key=lambda pair:len(pair[1]['data']),reverse=True):
        candidates=[k for k in available if len(banks[k])<4 and sizes[k]+len(e['data'])<=SLOT_SIZE-512]
        if not candidates: raise ValueError('collection does not fit four 80 KiB slots; shorten sounds or lower rate')
        k=min(candidates,key=lambda k:sizes[k]); banks[k].append((id,e));sizes[k]+=len(e['data'])
    slots=[]
    for k,bank in enumerate(banks):
        if not bank: continue
        header=bytearray(512);data=bytearray(128)
        struct.pack_into('<IHB',header,0,0x504D5346,1,len(bank));header[8:16]=b'LUCASFM\0'
        struct.pack_into('<I',header,24,0x314C464C)
        for j,(id,e) in enumerate(bank):
            struct.pack_into('<12sBBBBHHI',data,j*24,e['name'].encode(),e['kind'],e['root'],e['flags'],id,e['bpm'],e['bars'],0)
            struct.pack_into('<IIIIIhhBBBB',header,32+j*28,len(data),e['n'],0,e['n']-1,round(e['rate']/44100*65536),
                             (60+(e['root'] if e['root']<12 else 0))*16,0,0,60+id,60+id,1)
            data.extend(e['data'])
        struct.pack_into('<II',header,16,len(data),zlib.crc32(data))
        raw=bytes(header+data)
        slots.append(dict(slot=k,bytes=len(raw),crc=zlib.crc32(raw),data=base64.b64encode(raw).decode()))
    return slots

def validate_pack(p):
    if p.get('format')!='lucas-fm-loops' or p.get('version')!=1: raise ValueError('not a Lucas loop pack v1')
    used=set(); ids=set()
    preserve=p.get('preserve_slots',[])
    if not p.get('slots') or any(k not in range(4) for k in preserve):raise ValueError('invalid reserved slots')
    for item in p['slots']:
        k=item['slot'];raw=base64.b64decode(item['data'],validate=True)
        if k not in range(4) or k in preserve or k in used or len(raw)!=item['bytes'] or len(raw)>SLOT_SIZE or zlib.crc32(raw)!=item['crc']: raise ValueError('damaged or duplicate slot')
        used.add(k)
        magic,version,nz=struct.unpack_from('<IHB',raw)
        length,crc,marker=struct.unpack_from('<III',raw,16)
        if magic!=0x504D5346 or version!=1 or not 1<=nz<=4 or marker!=0x314C464C or length!=len(raw)-512 or zlib.crc32(raw[512:])!=crc: raise ValueError('invalid slot header')
        for j in range(nz):
            name,kind,root,flags,id,bpm,bars,_=struct.unpack_from('<12sBBBBHHI',raw,512+j*24)
            off,n,ls,le,rate,root16,pred,idx,lo,hi,loop=struct.unpack_from('<IIIIIhhBBBB',raw,32+j*28)
            if id in ids or id>=10 or kind>2 or flags>3 or (root>11 and root!=255) or bpm>300 or bars>16: raise ValueError('invalid metadata')
            if off<128 or off+(n+1)//2>length or not 0<=ls<=le<n or not 0<rate<=65536 or idx>88 or not loop: raise ValueError('invalid audio zone')
            ids.add(id)
    return p

def main():
    a=argparse.ArgumentParser(description=__doc__);a.add_argument('manifest',type=Path);a.add_argument('output',type=Path)
    a.add_argument('--key',default='D');a.add_argument('--bpm',type=int,default=72);a.add_argument('--rate',type=int,default=16000,choices=[12000,16000,22050])
    a.add_argument('--flash',type=Path);args=a.parse_args()
    if not 30<=args.bpm<=300:a.error('BPM must be 30..300')
    source=json.loads(args.manifest.read_text());entries=[prepare(e,args.manifest.parent,key(args.key),args.bpm,args.rate) for e in source['sounds']]
    available=source.get('slots',[0,1,2,3])
    if not available or len(set(available))!=len(available) or any(k not in range(4) for k in available):raise ValueError('slots must be unique indices 0..3')
    result=dict(format='lucas-fm-loops',version=1,name=source.get('name','Collection'),key=args.key,bpm=args.bpm,
                entries=[{k:v for k,v in e.items() if k!='data'} for e in entries],slots=pack(entries,available),preserve_slots=[k for k in range(4) if k not in available])
    validate_pack(result);args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(result,separators=(',',':')))
    for s in result['slots']: print(f'USR{s["slot"]+1}: {s["bytes"]:,} / {SLOT_SIZE:,} bytes')
    print(f'{len(entries)} library entries; four playback layers; {sum(e["n"]/e["rate"] for e in entries):.2f} seconds total')
    if args.flash:
        flash=bytearray(args.flash.read_bytes())
        if len(flash)!=1048576:raise ValueError('emulator flash must be exactly 1 MiB')
        for s in result['slots']:
            raw=base64.b64decode(s['data']);off=OFFSETS[s['slot']];flash[off:off+SLOT_SIZE]=raw+b'\xff'*(SLOT_SIZE-len(raw))
        args.output.with_suffix('.flash.bin').write_bytes(flash)
if __name__=='__main__':main()
