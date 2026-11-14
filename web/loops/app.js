import {SaveStore,packageHash,changedSectors,packSectors} from './engine/persistence.js';
const $=id=>document.getElementById(id), status=$('status'), canvas=$('lcd'), ctx=canvas.getContext('2d'), pixels=ctx.createImageData(240,240);
const store=new SaveStore();let ac,node,base,record,revision=0,chain=Promise.resolve(),selected=0,browsing=false,playing=false,ready=false;
const roles=['pad','bass','melody','rhythm'];
const send=m=>node?.port.postMessage(m);
const fail=e=>{status.textContent=String(e.message||e);};
const tap=id=>{if(!ready)return;send({type:'key',id,down:true});setTimeout(()=>send({type:'key',id,down:false}),150);};
const encoder=(enc,clicks)=>{if(ready&&clicks)send({type:'enc',enc,clicks});};
function state(){document.querySelectorAll('.select-layer').forEach((b,i)=>b.setAttribute('aria-pressed',String(selected===i)));$('edit').textContent=browsing?'Load loop':'Layer on / off';$('play').textContent=playing?'Ⅱ Stop':'▶ Play';}
function select(i){if(i===selected)tap(8);else encoder(0,i-selected);selected=i;browsing=false;state();}
function preset(n){encoder(6,n);browsing=true;state();}
for(let i=0;i<4;i++){
 const lane=document.createElement('div');lane.className='lane';
 const pick=document.createElement('button');pick.className='select-layer';pick.textContent=roles[i];pick.disabled=true;pick.setAttribute('aria-label','Select '+roles[i]);pick.onclick=()=>select(i);
 const knob=document.createElement('button');knob.className='knob';knob.disabled=true;knob.setAttribute('aria-label',roles[i]+' volume: drag, scroll or use arrow keys');knob.title='Drag up/down or scroll to mix';
 let last=null,angle=0;const turn=n=>{encoder(i+2,n);angle+=n*3;knob.style.setProperty('--angle',angle+'deg');};
 knob.onpointerdown=e=>{if(!ready)return;last=e.clientY;knob.setPointerCapture(e.pointerId);};
 knob.onpointermove=e=>{if(last===null)return;const n=Math.trunc((last-e.clientY)/3);if(n){turn(n);last-=n*3;}};
 knob.onpointerup=knob.onpointercancel=()=>last=null;
 knob.addEventListener('wheel',e=>{e.preventDefault();if(ready)turn(e.deltaY<0?2:-2);},{passive:false});
 knob.onkeydown=e=>{if(['ArrowUp','ArrowRight','ArrowDown','ArrowLeft'].includes(e.key)){e.preventDefault();e.stopPropagation();turn(e.key==='ArrowUp'||e.key==='ArrowRight'?1:-1);}};
 lane.append(pick,knob);$('layers').append(lane);
}
$('previous').onclick=()=>preset(-1);$('next').onclick=()=>preset(1);
$('edit').onclick=()=>{tap(6);browsing=false;state();};$('home').onclick=()=>{tap(8);browsing=false;state();};
$('play').onclick=()=>{tap(12);playing=!playing;state();status.textContent=playing?'Playing. Add layers and turn their knobs to mix.':'Stopped. Save to keep your setup.';};$('save').onclick=()=>{tap(9);status.textContent=playing?'Stop playback before saving.':'Saving your four layers…';};
$('master').oninput=e=>send({type:'pot',value:Number(e.target.value)});
document.addEventListener('keydown',e=>{if(!ready||e.repeat||e.target.matches('input,.knob'))return;
 const action={' ':()=>$('play').click(),ArrowLeft:()=>select(Math.max(0,selected-1)),ArrowRight:()=>select(Math.min(3,selected+1)),ArrowUp:()=>preset(1),ArrowDown:()=>preset(-1),Enter:()=>$('edit').click(),Escape:()=>$('home').click()}[e.key];if(action){e.preventDefault();action();}});
async function fetchBytes(url){const r=await fetch(url);if(!r.ok)throw Error('Could not load '+url);return new Uint8Array(await r.arrayBuffer());}
$('start').onclick=async()=>{
 $('start').disabled=true;status.textContent='Opening your loops…';
 try{
  ac=new AudioContext({sampleRate:44100});await ac.resume();
  const [wasm,pkg]=await Promise.all([fetchBytes('./engine/fm1_web.wasm'),fetchBytes('./firmware.fwsc')]);
  const hash=await packageHash(pkg);record=await store.load(hash)||{version:1,hash,package:pkg,sectors:[],revision:0,name:'Lucas Loops 0.4'};revision=record.revision;
  await ac.audioWorklet.addModule('./engine/worklet.js');node=new AudioWorkletNode(ac,'fm1',{numberOfInputs:0,outputChannelCount:[2]});node.connect(ac.destination);
  node.port.onmessage=({data:m})=>{
   if(m.type==='error'){fail(m.message);return;}
   if(m.type==='booted'){base=m.base;send({type:'pot',value:Number($('master').value)});status.textContent='Ready. Press Play, then bring in a layer.';}
   if(m.type==='frame'&&m.lcd){
    for(let i=0;i<m.lcd.length;i++){const p=m.lcd[i],o=i*4;pixels.data[o]=((p>>11)&31)*255/31;pixels.data[o+1]=((p>>5)&63)*255/63;pixels.data[o+2]=(p&31)*255/31;pixels.data[o+3]=255;}ctx.putImageData(pixels,0,0);
   }
   if(m.type==='frame'&&!ready&&m.ms>1600){ready=true;$('start').hidden=true;document.querySelectorAll('button,input').forEach(x=>x.disabled=false);state();}
   if(m.type==='frame'&&m.halted)fail('The emulator stopped. Reload to restart.');
   if(m.type==='snapshot'&&m.flash&&base){const sectors=changedSectors(base,m.flash);chain=chain.then(async()=>{revision=await store.save({...record,sectors,revision},'lucas-loops-0.4');status.textContent='Your setup is saved in this browser.';}).catch(fail);}
  };
  send({type:'boot',seq:1,wasm:wasm.buffer,package:pkg.buffer,mhz:240,sectors:packSectors(record.sectors).buffer});
 }catch(e){fail(e);$('start').disabled=false;node?.disconnect();await ac?.close();}
};state();
