#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Bundle resident loops into a play-ready emulator .fwsc, without flashing hardware.
The retained physical update loader preserves user data: use editor transfer there.
"""
import argparse,base64,json,struct
from pathlib import Path
from fm1pkg_make import enc,crc16,ufw
from lucas_pack import validate_pack,OFFSETS,SLOT_SIZE

def entries(raw):
    logical=bytearray().join(raw[i*48:i*48+47] for i in range(20))+raw[960:]
    h=bytearray(logical[:64]);enc(h,0,64)
    if len(h)!=64 or crc16(h[2:])!=struct.unpack_from('<H',h)[0]:raise ValueError('bad package header')
    result={}
    for i in range(struct.unpack_from('<H',h,8)[0]):
        e=bytearray(logical[64+i*80:144+i*80]);enc(e,0,80)
        typ,_,crc,_,off,size,_=struct.unpack_from('<HHHHIII',e)
        data=bytes(logical[off:off+size])
        if len(data)!=size or crc16(data)!=crc:raise ValueError('bad package payload')
        result[typ]=data
    if 0 not in result or 100 not in result:raise ValueError('missing flash or loader')
    return result

def preload(flash,pack,kit=None):
    if not 0x93000<=len(flash)<=0x100000:raise ValueError('unexpected flash layout')
    p=validate_pack(pack);out=bytearray(flash)+bytearray(b'\xff'*(0x100000-len(flash)))
    for s in p['slots']:
        off=OFFSETS[s['slot']];out[off:off+SLOT_SIZE]=b'\xff'*SLOT_SIZE
        d=base64.b64decode(s['data']);out[off:off+len(d)]=d
    if kit:
        if 0 not in p.get('preserve_slots',[]):raise ValueError('kit needs reserved USR1')
        if not 512<=len(kit)<=SLOT_SIZE or kit[:4]!=b'FSMP':raise ValueError('bad kit')
        out[OFFSETS[0]:OFFSETS[0]+SLOT_SIZE]=b'\xff'*SLOT_SIZE
        out[OFFSETS[0]:OFFSETS[0]+len(kit)]=kit
    assert out[:0x93000]==flash[:0x93000]
    return bytes(out)

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('firmware',type=Path);p.add_argument('pack',type=Path);p.add_argument('output',type=Path);p.add_argument('--kit',type=Path);a=p.parse_args()
    raw=a.firmware.read_bytes();e=entries(raw)
    product=''.join(chr((raw[i*48+47]-i-1)&255) for i in range(20) if raw[i*48+47]!=125)
    flash=preload(e[0],json.loads(a.pack.read_text()),a.kit.read_bytes() if a.kit else None)
    a.output.write_bytes(ufw(flash,e[100],product));print(f'Preloaded package: {a.output}, {a.output.stat().st_size} bytes')
