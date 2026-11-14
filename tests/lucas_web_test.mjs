// SPDX-License-Identifier: GPL-3.0-only
import vm from 'node:vm';
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
const html=readFileSync('web/editor.html','utf8');
const proto=html.slice(html.indexOf('/*PROTO-BEGIN*/'),html.indexOf('/*PROTO-END*/'));
const pack=html.slice(html.indexOf('const LFP ='),html.indexOf('function lfPackSummary'));
const ctx=vm.createContext({TextEncoder,TextDecoder,setTimeout,clearTimeout,console});
vm.runInContext(proto+'\n'+pack,ctx);
const run=code=>vm.runInContext(code,ctx);
ctx.starter=JSON.parse(readFileSync('assets/loops-cc0/park-in-d.lucaspack','utf8'));
run('lfPackValidate(starter)');
run(`lfDraft=Array.from({length:10},(_,i)=>({name:'Test '+i,x:Int16Array.from({length:22050},(_,n)=>Math.sin(n/13)*25000),start:0,seconds:1,kind:'texture',root:'',bpm:0,bars:0,followKey:false,followBpm:false}));`);
const p=run('lfPackBuild()');assert.equal(p.slots.length,4);ctx.p=p;run('lfPackValidate(p)');
let data=Buffer.from(p.slots[0].data,'base64');assert.ok(data.subarray(640).some(x=>x!==0 && x!==0x80));
assert.throws(()=>run('p.slots.push(p.slots[0]);lfPackValidate(p)'));
assert.throws(()=>run('lfDraft[0].followKey=true;lfPackBuild()'));
console.log('PASS: browser pack creation, ten entries, audible encoding, Python compatibility, corruption and root validation');
