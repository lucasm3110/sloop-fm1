# SPDX-License-Identifier: GPL-3.0-only
import copy,sys,unittest,base64,zlib,struct
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from lucas_pack import pack,validate_pack,prepare,key
class PackTests(unittest.TestCase):
    def entry(self,n=1000):return dict(name='Birds',kind=0,root=255,bpm=0,bars=0,flags=0,rate=16000,n=n*2,data=bytes(n))
    def test_ten_slots(self):
        p=dict(format='lucas-fm-loops',version=1,slots=pack([self.entry() for _ in range(10)]))
        validate_pack(p);self.assertEqual(len(p['slots']),4)
        p['slots'][0]['data']='bad';self.assertRaises(Exception,validate_pack,p)
    def test_overflow(self):self.assertRaises(ValueError,pack,[self.entry(80000) for _ in range(5)])
    def test_boundary(self):
        p=dict(format='lucas-fm-loops',version=1,slots=pack([self.entry()]))
        slot=p['slots'][0];d=bytearray(base64.b64decode(slot['data']));struct.pack_into('<I',d,32,0)
        slot.update(data=base64.b64encode(d).decode(),crc=zlib.crc32(d));self.assertRaises(ValueError,validate_pack,p)
    def test_metadata(self):
        self.assertEqual(key('D'),2);self.assertRaises(ValueError,key,'banana')
        self.assertRaises(ValueError,prepare,dict(type='tonal',follow_key=True,name='x',file='x'),Path('.'),2,72,16000)
if __name__=='__main__':unittest.main()
