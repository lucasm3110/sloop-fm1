#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Read firmware ELF symbols and optional native-emulator RAM diagnostics."""
import argparse,json,struct
from pathlib import Path

def symbols(path):
    b=Path(path).read_bytes();assert b[:5]==b'\x7fELF\x01'
    off=struct.unpack_from('<I',b,32)[0];size,n=struct.unpack_from('<HH',b,46)
    sections=[struct.unpack_from('<10I',b,off+i*size) for i in range(n)]
    result={}
    for sh in sections:
        if sh[1]!=2:continue
        st=sections[sh[6]];strings=b[st[4]:st[4]+st[5]]
        for at in range(sh[4],sh[4]+sh[5],sh[9]):
            name,value,length,info,other,index=struct.unpack_from('<IIIBBH',b,at)
            name=strings[name:strings.index(b'\0',name)].decode(errors='replace')
            result[name]=(value,length)
    return result

def report(elf,ram=None):
    s=symbols(elf);v=lambda name:s[name][0]
    r={'data_bss_bytes':v('_bss_end')-0x1c08000,'data_bss_limit':98304,'pool_bytes':v('_pool_end')-v('_pool_start'),'pool_limit':0x54000,'lowram_bytes':v('_lr_end')-v('_lr_start'),'app_bytes':Path(elf).with_suffix('.bin').stat().st_size}
    if ram:
        b=Path(ram).read_bytes();names='magic halves max_us nested in_audio late timer_irqs ui_frames last_us cpu_q8 boots stage page home prev_stage prev_page prev_home prev_rst prev_frames'.split()
        r['audio']=dict(zip(names,struct.unpack_from('<19I',b,v('felucca_dbg')-0x1c00000)))
        r['audio']['last_cpu_percent']=round(r['audio']['cpu_q8']*100/256,2)
        r['audio']['buffer_deadline_us']=256*1000000//44100
    return r
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('elf');p.add_argument('--ram');a=p.parse_args();print(json.dumps(report(a.elf,a.ram),indent=2))
