# SPDX-License-Identifier: GPL-3.0-only
import copy,sys,unittest,base64,zlib,struct,shutil,tempfile,wave,math
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from lucas_pack import pack,validate_pack,prepare,key
from sampleio import IMA_STEP,IMA_IDX
from lucas_preload import preload,entries
from fm1pkg_make import ufw
class PackTests(unittest.TestCase):
    def entry(self,n=1000):return dict(name='Birds',kind=0,root=255,bpm=0,bars=0,flags=0,rate=16000,n=n*2,data=bytes(n))
    def test_ten_slots(self):
        p=dict(format='lucas-fm-loops',version=1,slots=pack([self.entry() for _ in range(10)]))
        validate_pack(p);self.assertEqual(len(p['slots']),4)
        p['slots'][0]['data']='bad';self.assertRaises(Exception,validate_pack,p)
    def test_preloaded_package(self):
        import json
        p=json.loads(Path('assets/loops-cc0/park-in-d.lucaspack').read_text())
        original=bytes([42])*0x93000;image=preload(original,p)
        self.assertEqual(image[:0x93000],original)
        self.assertEqual(len(image),0x100000)
        decoded=entries(ufw(image,b'loader','FM-1_802'))
        self.assertEqual(decoded[0],image);self.assertEqual(decoded[100],b'loader')
        for slot in p['slots']:
            data=base64.b64decode(slot['data']);off=[0xa0000,0xb4000,0xc8000,0xe7000][slot['slot']]
            self.assertEqual(image[off:off+len(data)],data)
    def test_overflow(self):self.assertRaises(ValueError,pack,[self.entry(80000) for _ in range(5)])
    def test_boundary(self):
        p=dict(format='lucas-fm-loops',version=1,slots=pack([self.entry()]))
        slot=p['slots'][0];d=bytearray(base64.b64decode(slot['data']));struct.pack_into('<I',d,32,0)
        slot.update(data=base64.b64encode(d).decode(),crc=zlib.crc32(d));self.assertRaises(ValueError,validate_pack,p)
    def test_metadata(self):
        self.assertEqual(key('D'),2);self.assertRaises(ValueError,key,'banana')
        self.assertRaises(ValueError,prepare,dict(type='tonal',follow_key=True,name='x',file='x'),Path('.'),2,72,16000)
    def test_reserved_kit_slot(self):
        p=dict(format='lucas-fm-loops',version=1,preserve_slots=[0],slots=pack([self.entry() for _ in range(10)],[1,2,3]))
        validate_pack(p);self.assertEqual({s['slot'] for s in p['slots']},{1,2,3})
        p['preserve_slots']=[1];self.assertRaises(ValueError,validate_pack,p)
    @unittest.skipUnless(shutil.which('ffmpeg'),'ffmpeg optional for offline preparation test')
    def test_independent_pitch_and_tempo(self):
        with tempfile.TemporaryDirectory() as folder:
            p=Path(folder)/'source.wav';sr=44100;n=round(sr*240/72)
            x=[round(20000*math.sin(2*math.pi*200*i/sr)) for i in range(n)]
            with wave.open(str(p),'wb') as w:w.setparams((1,2,sr,0,'NONE','not compressed'));w.writeframes(struct.pack('<%dh'%n,*x))
            e=prepare(dict(name='test',file=p.name,type='rhythmic',root='C',bpm=72,bars=1,seconds=240/72,follow_key=True,follow_bpm=True),p.parent,2,96,16000)
            self.assertEqual(e['n'],40000);self.assertEqual(e['root'],2);self.assertEqual(e['bpm'],96)
            # Independent IMA decoding to measure the prepared audio, not just its metadata.
            pred=idx=0;decoded=[]
            for byte in e['data']:
                for code in [byte&15,byte>>4]:
                    step=IMA_STEP[idx];delta=(step>>3)+((step if code&4 else 0))+((step>>1) if code&2 else 0)+((step>>2) if code&1 else 0)
                    pred=max(-32768,min(32767,pred+(-delta if code&8 else delta)));idx=max(0,min(88,idx+IMA_IDX[code&7]));decoded.append(pred)
            middle=decoded[8000:32000];rises=sum(a<=0<b for a,b in zip(middle,middle[1:]));hz=rises/1.5
            self.assertAlmostEqual(hz,200*2**(2/12),delta=1.5)
if __name__=='__main__':unittest.main()
