#pragma once

#include <pgmspace.h>

static const char WEB_UI_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>MatrixSign</title>
<style>
*{box-sizing:border-box}
body{font-family:system-ui,sans-serif;margin:0;padding:16px;padding-bottom:88px;background:#111;color:#eee;max-width:480px;margin-inline:auto}
h1{font-size:1.25rem;margin:8px 0}
#connBar{font-size:.8rem;color:#888;padding:8px 10px;background:#181818;border:1px solid #333;border-radius:8px;margin-bottom:12px;line-height:1.4}
.banner{background:#1a2a44;border:1px solid #2563eb;border-radius:8px;padding:12px;margin-bottom:16px;font-size:.9rem;line-height:1.4}
.banner strong{color:#6af}
section{margin-bottom:20px}
label{display:block;margin:12px 0 4px;font-size:.9rem;color:#aaa}
input[type=text],input[type=number],input[type=password],textarea,select{width:100%;padding:10px;border:1px solid #333;border-radius:8px;background:#222;color:#fff;font-size:1rem}
textarea{min-height:72px;resize:vertical}
.row{display:flex;align-items:center;gap:8px;margin:8px 0}
input[type=range]{flex:1;width:100%}
.seg,.slots,.effects{display:flex;gap:6px;flex-wrap:wrap;margin:8px 0}
.seg button,.effect{padding:10px 12px;border:1px solid #444;border-radius:8px;background:#222;color:#fff;cursor:pointer;min-width:42px;text-align:center}
.seg button.active,.effect.active{border-color:#6af;background:#1a2a44}
.slot{display:flex;flex-direction:column;align-items:stretch;padding:8px 6px;border:1px solid #444;border-radius:8px;background:#222;color:#fff;cursor:pointer;min-width:72px;flex:1 1 calc(25% - 6px);max-width:calc(25% - 6px);text-align:center;gap:2px}
.slot.active{border-color:#6af;background:#1a2a44}
.slot.live{box-shadow:0 0 0 1px #6af inset}
.slot.dirty::after{content:"";width:6px;height:6px;border-radius:50%;background:#fa0;margin:2px auto 0}
.slot-num{font-size:1rem;font-weight:600}
.slot-label{font-size:.65rem;color:#888;overflow:hidden;text-overflow:ellipsis;white-space:nowrap;max-width:100%}
.colors,.actions{display:flex;gap:8px;flex-wrap:wrap;margin:8px 0}
.color-btn,.action-btn{padding:8px 14px;border:1px solid #444;border-radius:8px;background:#222;color:#fff;cursor:pointer}
.color-btn.active,.action-btn.primary{border-color:#6af;background:#1a2a44}
button:not(.action-btn):not(.color-btn):not(.slot):not(.seg button):not(.effect){width:100%;padding:12px;margin-top:12px;border:none;border-radius:8px;font-size:1rem;cursor:pointer}
.btn-primary{background:#2563eb;color:#fff}
.btn-secondary{background:#333;color:#eee;margin-top:8px}
.btn-danger{background:#444;color:#fcc;margin-top:8px}
.btn-ghost{background:#222;color:#ccc;border:1px solid #444;margin-top:8px}
details{background:#181818;border:1px solid #333;border-radius:8px;padding:10px;margin-top:12px}
summary{cursor:pointer;color:#aaa}
.hidden{display:none}
.legend{font-size:.75rem;color:#666;margin:4px 0 8px}
.legend span{margin-right:10px}
.preview-wrap{margin:12px 0;padding:10px;background:#000;border:1px solid #333;border-radius:8px;text-align:center}
#preview{display:block;margin:0 auto;width:520px;height:260px;image-rendering:pixelated;border:1px solid #222}
.preview-cap{font-size:.75rem;color:#666;margin-top:6px}
#toast{position:fixed;left:50%;bottom:80px;transform:translateX(-50%);max-width:90%;padding:10px 16px;border-radius:8px;background:#1a2a44;border:1px solid #2563eb;color:#eee;font-size:.9rem;z-index:100;opacity:0;pointer-events:none;transition:opacity .2s}
#toast.show{opacity:1}
#toast.err{border-color:#a44;background:#2a1515}
#stickyBar{position:fixed;left:0;right:0;bottom:0;padding:10px 16px;background:#111;border-top:1px solid #333;display:flex;gap:8px;max-width:480px;margin-inline:auto}
#stickyBar button{flex:1;margin:0;padding:12px;border:none;border-radius:8px;font-size:.95rem;cursor:pointer}
#stickyBar .primary{background:#2563eb;color:#fff}
#stickyBar .secondary{background:#333;color:#eee}
.warn{color:#fa0;font-size:.8rem;margin-top:4px}
.hint{color:#666;font-size:.8rem;margin-top:4px}
button:disabled{opacity:.5;cursor:not-allowed}
</style>
</head>
<body>
<div id="connBar">Loading...</div>
<h1>MatrixSign</h1>
<div class="banner" id="banner"></div>

<section id="brightnessSection">
<label for="bright">Panel brightness <span id="brightVal">10</span>%</label>
<input type="range" id="bright" min="1" max="100" value="10">
<p class="hint">Applies immediately to the sign — not saved per preset.</p>
</section>

<section>
<label>Presets (1–8)</label>
<p class="legend"><span>▢ outline = editing</span><span>▣ glow = live</span><span>● = unsaved</span></p>
<div class="slots" id="slots"></div>
<p class="hint" id="slotHelp">Save stores this slot. Show on panel switches what the sign displays.</p>
<p class="hint" id="slotInfo"></p>
<div class="actions hidden" id="applyLiveWrap">
<button type="button" class="action-btn" id="applyLive">Apply edits to live slot</button>
</div>
</section>

<section>
<label>Preview (104×52)</label>
<div class="preview-wrap">
<canvas id="preview" width="104" height="52"></canvas>
<p class="preview-cap" id="previewCap">Scaled preview</p>
</div>
</section>

<section>
<label>Content type</label>
<div class="seg" id="contentTypeSeg">
<button type="button" data-type="text" class="active">Message</button>
<button type="button" data-type="gif">GIF</button>
<button type="button" data-type="effect">Effect</button>
</div>

<div id="textSection">
<label for="text">Message</label>
<textarea id="text" maxlength="200" placeholder="HELLO&#10;LINE TWO"></textarea>
<p class="hint">Use line breaks for multiple rows (max rows setting below).</p>
<label for="fontHeight">Font height <span id="fontHeightVal">16</span> px</label>
<input type="range" id="fontHeight" min="8" max="48" step="8" value="16">
<p class="hint" id="fontSuggest">Suggested size appears here.</p>
<button type="button" class="action-btn" id="useSuggest">Use suggested size</button>
<label>Rows</label>
<div class="seg" id="rowSeg">
<button type="button" data-rows="1" class="active">1</button>
<button type="button" data-rows="2">2</button>
<button type="button" data-rows="3">3</button>
<button type="button" data-rows="4">4</button>
</div>
<p class="warn" id="lenWarn" hidden>Long text will scroll — panel is 104 px wide.</p>
<label class="row"><input type="checkbox" id="scroll" checked> Scroll</label>
<label>Scroll speed</label>
<div class="seg" id="scrollSpeedSeg">
<button type="button" data-ms="80">Slow</button>
<button type="button" data-ms="40" class="active">Normal</button>
<button type="button" data-ms="20">Fast</button>
</div>
<details>
<summary>Advanced scroll (ms)</summary>
<input type="number" id="delay" min="10" max="500" value="40">
</details>
<label>Color</label>
<div class="colors">
<button type="button" class="color-btn active" data-color="#FFFFFF">White</button>
<button type="button" class="color-btn" data-color="#FF0000">Red</button>
<button type="button" class="color-btn" data-color="#00FF00">Green</button>
<button type="button" class="color-btn" data-color="#FFAA00">Amber</button>
</div>
<input type="color" id="color" value="#ffffff">
</div>

<div id="gifSection" class="hidden">
<label>GIF file</label>
<p class="hint" id="gifStatus"></p>
<input type="file" id="gifFile" accept="image/gif">
<div class="actions">
<button type="button" class="action-btn primary" id="uploadGif">Upload GIF</button>
<button type="button" class="action-btn" id="removeGif">Remove GIF</button>
</div>
<p class="hint">Best: 104×52 px, max 256 KB.</p>
</div>

<div id="effectSection" class="hidden">
<label>Built-in effect</label>
<div class="effects" id="effectGrid"></div>
<p class="hint">Effects are stored in firmware — no upload needed.</p>
</div>
</section>

<details>
<summary>Home Wi-Fi (optional)</summary>
<label for="ssid">SSID</label>
<input type="text" id="ssid" autocomplete="off">
<label for="password">Password</label>
<input type="password" id="password" autocomplete="off">
<button class="btn-secondary" id="connectWifi">Connect in background</button>
<button class="btn-danger" id="wifiReset">Forget home Wi-Fi</button>
</details>

<div id="stickyBar">
<button type="button" class="secondary" id="showSlot">Show on panel</button>
<button type="button" class="primary" id="saveSlot">Save slot</button>
</div>
<div id="toast" role="status" aria-live="polite"></div>
<script>
let selectedColor="#FFFFFF";
let selectedSlot=0;
let activeSlot=0;
let contentType="text";
let glyphHeightPx=16;
let rowCount=1;
let selectedEffect="bright_white";
let globalBrightness=10;
let effects=[];
let presets=[];
let connData={};
let busy=false;
let toastTimer=null;
let brightTimer=null;
let previewScroll=104;
let previewTimer=null;
const PANEL_W=104;
const PANEL_H=52;
const $=id=>document.getElementById(id);

function blockHeightPx(){return glyphHeightPx*rowCount;}

function glyphFromBlock(block,rows){
  const lineH=Math.floor(block/Math.max(1,rows));
  const size=Math.max(1,Math.min(6,Math.floor(lineH/8)));
  return size*8;
}

function optimalGlyphHeight(rows,text,scroll){
  const lines=text.split("\n").slice(0,rows);
  while(lines.length<rows)lines.push("");
  const maxLen=Math.max(1,...lines.map(l=>l.length));
  let byHeight=Math.floor(PANEL_H/rows);
  byHeight=Math.max(8,Math.min(48,Math.floor(byHeight/8)*8));
  if(!scroll){
    const maxSize=Math.floor(PANEL_W/(6*maxLen));
    const byWidth=Math.max(8,Math.min(48,Math.max(1,Math.min(6,maxSize))*8));
    return Math.min(byHeight,byWidth);
  }
  return byHeight;
}

function updateFontSizeUi(){
  $("fontHeight").value=glyphHeightPx;
  $("fontHeightVal").textContent=glyphHeightPx;
  const opt=optimalGlyphHeight(rowCount,$("text").value,$("scroll").checked);
  const chars=Math.floor(PANEL_W/(6*(opt/8)));
  if(opt===glyphHeightPx){
    $("fontSuggest").textContent=`${glyphHeightPx}px fits ${rowCount} row(s) on the panel${$("scroll").checked?"":" (~"+chars+" chars/line)"}.`;
    $("useSuggest").disabled=true;
    return;
  }
  if($("scroll").checked){
    $("fontSuggest").textContent=`Suggested: ${opt}px — tallest size for ${rowCount} row(s) on the ${PANEL_H}px panel.`;
  }else{
    $("fontSuggest").textContent=`Suggested: ${opt}px — fits ${rowCount} row(s) and up to ${chars} characters per line without scrolling.`;
  }
  $("useSuggest").disabled=false;
}

function setGlyphHeight(px){
  glyphHeightPx=Math.min(48,Math.max(8,Math.round(parseInt(px,10)/8)*8));
  updateFontSizeUi();
  drawPreview();
}

function showToast(msg,isErr){
  const el=$("toast");
  el.textContent=msg;
  el.className="show"+(isErr?" err":"");
  clearTimeout(toastTimer);
  toastTimer=setTimeout(()=>el.classList.remove("show"),3000);
}

function setBusy(v){
  busy=v;
  document.body.setAttribute("aria-busy",v?"true":"false");
  $("saveSlot").disabled=v;
  $("showSlot").disabled=v;
  if($("applyLive"))$("applyLive").disabled=v;
}

async function apiJson(url,opts={}){
  const r=await fetch(url,opts);
  let data={};
  try{data=await r.json();}catch(e){}
  if(!r.ok){
    const err=data.error||(`Request failed (${r.status})`);
    throw new Error(err);
  }
  return data;
}

function textMetrics(glyph,rows){
  const block=glyph*rows;
  const size=glyph/8;
  const chars=Math.floor(PANEL_W/(6*size));
  return{block,size,glyph,chars};
}

function resolveBlockHeight(p){
  if(p.textHeightPx)return p.textHeightPx;
  const legacy={quarter:13,half:26,three_quarter:39,full:52};
  return legacy[p.fontScale]||26;
}

function presetFormBody(){
  return{
    contentType,
    effectId:selectedEffect,
    textHeightPx:blockHeightPx(),
    rowCount,
    text:$("text").value,
    scroll:$("scroll").checked,
    scrollDelayMs:parseInt($("delay").value,10),
    color:selectedColor
  };
}

function savedPresetBody(p){
  if(!p)return null;
  const rows=p.rowCount||1;
  return{
    contentType:(p.contentType||"text").toLowerCase(),
    effectId:p.effectId||"bright_white",
    textHeightPx:resolveBlockHeight(p),
    rowCount:rows,
    text:p.text||"",
    scroll:!!p.scroll,
    scrollDelayMs:p.scrollDelayMs||40,
    color:(p.color||"#FFFFFF").toUpperCase()
  };
}

function hexToRgb(hex){
  const h=(hex||"#FFFFFF").replace("#","");
  const n=parseInt(h.length===3?h.split("").map(c=>c+c).join(""):h,16);
  return{r:(n>>16)&255,g:(n>>8)&255,b:n&255};
}

function isDirty(){
  const saved=savedPresetBody(presets[selectedSlot]);
  if(!saved)return false;
  const cur=presetFormBody();
  return JSON.stringify(cur)!==JSON.stringify(saved);
}

function setSegActive(container,matchAttr,value){
  container.querySelectorAll("button").forEach(b=>{
    const on=b.dataset[matchAttr]===String(value);
    b.classList.toggle("active",on);
    b.setAttribute("aria-pressed",on?"true":"false");
  });
}

function syncScrollSeg(){
  const ms=parseInt($("delay").value,10)||40;
  let best=40,d=999;
  [80,40,20].forEach(v=>{const d2=Math.abs(v-ms);if(d2<d){d=d2;best=v;}});
  setSegActive($("scrollSpeedSeg"),"ms",best);
}

function showSections(){
  $("textSection").classList.toggle("hidden",contentType!=="text");
  $("gifSection").classList.toggle("hidden",contentType!=="gif");
  $("effectSection").classList.toggle("hidden",contentType!=="effect");
  setSegActive($("contentTypeSeg"),"type",contentType);
  setSegActive($("rowSeg"),"rows",rowCount);
  updateFontSizeUi();
  syncScrollSeg();
  updateGifStatus();
  drawPreview();
}

function slotLabel(p){
  if(!p)return"empty";
  if(p.contentType==="effect")return(p.effectLabel||p.effectId||"Effect").slice(0,12);
  if(p.contentType==="gif"&&p.hasGif)return"GIF";
  const t=(p.text||"").split("\n")[0].trim();
  return(t||"Message").slice(0,12);
}

function fillForm(p){
  contentType=(p.contentType||"text").toLowerCase();
  rowCount=p.rowCount||1;
  setGlyphHeight(glyphFromBlock(resolveBlockHeight(p),rowCount));
  selectedEffect=p.effectId||"bright_white";
  $("text").value=p.text||"";
  $("scroll").checked=!!p.scroll;
  $("delay").value=p.scrollDelayMs||40;
  selectedColor=(p.color||"#FFFFFF").toUpperCase();
  $("color").value=selectedColor;
  document.querySelectorAll(".color-btn").forEach(b=>b.classList.toggle("active",b.dataset.color.toUpperCase()===selectedColor));
  showSections();
  renderEffects();
  updateLenWarn();
  updateApplyLive();
  renderSlots();
}

function selectSlot(index){
  selectedSlot=index;
  fillForm(presets[index]||{});
  updateSlotInfo();
}

function renderEffects(){
  const el=$("effectGrid");
  el.innerHTML="";
  effects.forEach(e=>{
    const b=document.createElement("button");
    b.type="button";
    const on=e.id===selectedEffect;
    b.className="effect"+(on?" active":"");
    b.setAttribute("aria-pressed",on?"true":"false");
    b.dataset.id=e.id;
    b.textContent=e.label;
    b.onclick=()=>{selectedEffect=e.id;renderEffects();drawPreview();updateApplyLive();renderSlots();};
    el.appendChild(b);
  });
}

function renderSlots(){
  const el=$("slots");
  el.innerHTML="";
  for(let i=0;i<8;i++){
    const p=presets[i]||{};
    const b=document.createElement("button");
    b.type="button";
    let cls="slot";
    if(i===selectedSlot)cls+=" active";
    if(i===activeSlot)cls+=" live";
    if(i===selectedSlot&&isDirty())cls+=" dirty";
    b.className=cls;
    b.innerHTML=`<span class="slot-num">${i+1}</span><span class="slot-label">${slotLabel(p)}</span>`;
    b.onclick=()=>selectSlot(i);
    b.ondblclick=()=>activateSlot(i);
    el.appendChild(b);
  }
  $("saveSlot").textContent=`Save slot ${selectedSlot+1}`;
  $("showSlot").textContent=`Show slot ${selectedSlot+1}`;
}

function updateSlotInfo(){
  const p=presets[selectedSlot]||{};
  $("slotInfo").textContent=`Editing slot ${selectedSlot+1} · Live: slot ${activeSlot+1} · ${slotLabel(p)}`;
}

function updateApplyLive(){
  $("applyLiveWrap").classList.toggle("hidden",selectedSlot===activeSlot);
  $("applyLive").textContent=`Apply edits to live slot ${activeSlot+1}`;
}

function updateBanner(c){
  if(c.staConnected){
    $("banner").innerHTML=`<strong>Connected</strong> · ${c.staIp||""}${c.staRssi!=null?` · ${c.staRssi} dBm`:""} · open <strong>http://${c.staIp||""}/</strong>`;
    return;
  }
  $("banner").innerHTML=`<strong>Field mode:</strong> join Wi-Fi <strong>${c.apSsid||"MatrixSign"}</strong>, then open <strong>http://${c.apIp||"192.168.4.1"}</strong><details style="margin-top:8px"><summary>AP password</summary><p class="hint" style="margin:6px 0 0">${c.apPassword||"matrixsign"}</p></details>`;
}

function updateConnBar(c){
  connData=c||connData;
  let s="";
  if(c.staConnected)s=`Home ${c.staIp} · ${c.staRssi} dBm · brightness ${globalBrightness}%`;
  else s=`AP ${c.apSsid||"MatrixSign"} · ${c.apIp||"192.168.4.1"} · brightness ${globalBrightness}%`;
  $("connBar").textContent=s;
}

function updateGifStatus(){
  const p=presets[selectedSlot]||{};
  if(p.hasGif)$("gifStatus").textContent=`GIF on slot ${selectedSlot+1}${p.gifPath?": "+p.gifPath:""}`;
  else $("gifStatus").textContent="No GIF on this slot.";
}

function drawPreview(){
  const canvas=$("preview");
  if(!canvas)return;
  const ctx=canvas.getContext("2d");
  ctx.fillStyle="#000";
  ctx.fillRect(0,0,PANEL_W,PANEL_H);
  if(contentType==="effect"){
    const eff=effects.find(e=>e.id===selectedEffect);
    ctx.fillStyle="#333";
    ctx.fillRect(0,0,PANEL_W,PANEL_H);
    ctx.fillStyle="#ff0";
    ctx.font="8px monospace";
    ctx.fillText(eff?eff.label:"Effect",4,28);
    $("previewCap").textContent="Effect preview (label only)";
    return;
  }
  if(contentType==="gif"){
    const p=presets[selectedSlot]||{};
    ctx.fillStyle="#224";
    ctx.fillRect(0,0,PANEL_W,PANEL_H);
    ctx.fillStyle="#6af";
    ctx.font="8px monospace";
    ctx.fillText(p.hasGif?"GIF":"No GIF",4,28);
    $("previewCap").textContent="GIF slot preview";
    return;
  }
  const blockH=blockHeightPx();
  const m=textMetrics(glyphHeightPx,rowCount);
  const blockTop=Math.floor((PANEL_H-blockH)/2);
  const lineH=Math.floor(blockH/rowCount);
  const rgb=hexToRgb(selectedColor);
  const lines=$("text").value.split("\n").slice(0,rowCount);
  while(lines.length<rowCount)lines.push("");
  ctx.fillStyle="rgba(255,255,255,0.06)";
  ctx.fillRect(0,blockTop,PANEL_W,blockH);
  const glyphH=m.size*8;
  ctx.fillStyle=`rgb(${rgb.r},${rgb.g},${rgb.b})`;
  const scrolling=$("scroll").checked;
  for(let i=0;i<rowCount;i++){
    const y=blockTop+i*lineH+Math.floor((lineH-glyphH)/2);
    const line=lines[i]||"";
    const textW=line.length*6*m.size;
    const x=scrolling?previewScroll:Math.floor((PANEL_W-textW)/2);
    for(let ci=0;ci<line.length;ci++){
      const cx=x+ci*6*m.size;
      if(cx>=PANEL_W)break;
      ctx.fillRect(cx,y,glyphH*0.75,glyphH);
    }
  }
  $("previewCap").textContent=`${glyphHeightPx}px font · ${rowCount} row(s) · size ${m.size}`;
}

function startPreviewAnim(){
  if(previewTimer)clearInterval(previewTimer);
  previewTimer=setInterval(()=>{
    if(contentType!=="text"||!$("scroll").checked)return;
    previewScroll--;
    const m=textMetrics(glyphHeightPx,rowCount);
    const lines=$("text").value.split("\n").slice(0,rowCount);
    let maxW=0;
    lines.forEach(l=>{const w=l.length*6*m.size;if(w>maxW)maxW=w;});
    if(previewScroll<-maxW)previewScroll=PANEL_W;
    drawPreview();
  },120);
}

async function loadEffects(){
  const data=await apiJson("/api/effects");
  effects=data.effects||[];
  renderEffects();
}

async function loadPresets(opts={}){
  const keepSelection=!!opts.keepSelection;
  const keepForm=!!opts.keepForm;
  const prevSlot=selectedSlot;
  const data=await apiJson("/api/presets");
  presets=data.presets||[];
  activeSlot=data.activeIndex||0;
  if(typeof data.brightness==="number")globalBrightness=data.brightness;
  $("bright").value=globalBrightness;
  $("brightVal").textContent=globalBrightness;
  if(!keepSelection)selectedSlot=activeSlot;
  else selectedSlot=Math.min(Math.max(prevSlot,0),presets.length-1);
  if(!keepForm)fillForm(presets[selectedSlot]||{});
  else{renderSlots();updateSlotInfo();updateApplyLive();}
  updateBanner(data);
  updateConnBar(data);
  drawPreview();
}

async function saveBrightness(val){
  try{
    await apiJson("/api/brightness",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({brightness:val})});
    globalBrightness=val;
    updateConnBar(connData);
    showToast(`Brightness ${val}%`);
  }catch(e){showToast(e.message,true);}
}

async function saveSlot(){
  setBusy(true);
  try{
    const body={...presetFormBody(),id:selectedSlot};
    await apiJson("/api/presets",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify(body)});
    showToast(`Saved slot ${selectedSlot+1}`);
    await loadPresets({keepSelection:true,keepForm:true});
    renderSlots();
    updateApplyLive();
  }catch(e){showToast(e.message,true);}
  setBusy(false);
}

async function activateSlot(index){
  if(index==null)index=selectedSlot;
  setBusy(true);
  try{
    await apiJson("/api/presets/select",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({id:index})});
    showToast(`Showing slot ${index+1} on panel`);
    await loadPresets({keepSelection:true,keepForm:true});
  }catch(e){showToast(e.message,true);}
  setBusy(false);
}

async function applyLive(){
  setBusy(true);
  try{
    await apiJson("/api/config",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify(presetFormBody())});
    showToast(`Applied to live slot ${activeSlot+1}`);
    await loadPresets({keepSelection:true,keepForm:true});
  }catch(e){showToast(e.message,true);}
  setBusy(false);
}

function updateLenWarn(){$("lenWarn").hidden=($("text").value.length<20);}

document.querySelectorAll("#contentTypeSeg button").forEach(b=>b.addEventListener("click",()=>{
  contentType=b.dataset.type;showSections();updateApplyLive();renderSlots();
}));
$("fontHeight").addEventListener("input",e=>{
  setGlyphHeight(e.target.value);updateApplyLive();renderSlots();
});
$("useSuggest").addEventListener("click",()=>{
  setGlyphHeight(optimalGlyphHeight(rowCount,$("text").value,$("scroll").checked));
  updateApplyLive();renderSlots();
});
document.querySelectorAll("#rowSeg button").forEach(b=>b.addEventListener("click",()=>{
  rowCount=parseInt(b.dataset.rows,10);showSections();updateApplyLive();renderSlots();
}));
document.querySelectorAll("#scrollSpeedSeg button").forEach(b=>b.addEventListener("click",()=>{
  $("delay").value=b.dataset.ms;syncScrollSeg();updateApplyLive();renderSlots();
}));
$("delay").addEventListener("input",()=>{syncScrollSeg();updateApplyLive();renderSlots();});
document.querySelectorAll(".color-btn").forEach(b=>b.addEventListener("click",()=>{
  selectedColor=b.dataset.color;$("color").value=selectedColor;
  document.querySelectorAll(".color-btn").forEach(x=>x.classList.remove("active"));b.classList.add("active");
  drawPreview();updateApplyLive();renderSlots();
}));
$("color").addEventListener("input",e=>{selectedColor=e.target.value.toUpperCase();document.querySelectorAll(".color-btn").forEach(x=>x.classList.remove("active"));drawPreview();updateApplyLive();renderSlots();});
$("bright").addEventListener("input",e=>{
  $("brightVal").textContent=e.target.value;
  clearTimeout(brightTimer);
  brightTimer=setTimeout(()=>saveBrightness(parseInt(e.target.value,10)),300);
});
$("text").addEventListener("input",()=>{updateLenWarn();updateFontSizeUi();drawPreview();updateApplyLive();renderSlots();});
$("scroll").addEventListener("change",()=>{updateFontSizeUi();drawPreview();updateApplyLive();renderSlots();});

$("saveSlot").addEventListener("click",saveSlot);
$("showSlot").addEventListener("click",()=>activateSlot(selectedSlot));
$("applyLive").addEventListener("click",applyLive);

$("uploadGif").addEventListener("click",async()=>{
  const f=$("gifFile").files[0];
  if(!f){showToast("Choose a GIF file first.",true);return;}
  if(f.size>262144){showToast("GIF too large (max 256 KB).",true);return;}
  setBusy(true);
  try{
    const fd=new FormData();
    fd.append("file",f,f.name);
    const r=await fetch(`/api/presets/gif?id=${selectedSlot}`,{method:"POST",body:fd});
    let data={};try{data=await r.json();}catch(e){}
    if(!r.ok)throw new Error(data.error||"Upload failed");
    contentType="gif";showSections();
    showToast(`GIF uploaded to slot ${selectedSlot+1}`);
    await loadPresets({keepSelection:true,keepForm:true});
  }catch(e){showToast(e.message,true);}
  setBusy(false);
});

$("removeGif").addEventListener("click",async()=>{
  setBusy(true);
  try{
    await apiJson(`/api/presets/gif?id=${selectedSlot}`,{method:"DELETE"});
    showToast(`GIF removed from slot ${selectedSlot+1}`);
    await loadPresets({keepSelection:true,keepForm:true});
  }catch(e){showToast(e.message,true);}
  setBusy(false);
});

$("connectWifi").addEventListener("click",async()=>{
  setBusy(true);
  try{
    await apiJson("/api/wifi/connect",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({ssid:$("ssid").value,password:$("password").value})});
    showToast("Connecting to home Wi-Fi...");
    setTimeout(()=>loadPresets({keepSelection:true,keepForm:true}),2000);
  }catch(e){showToast(e.message,true);}
  setBusy(false);
});

$("wifiReset").addEventListener("click",async()=>{
  if(!confirm("Forget saved home Wi-Fi credentials?"))return;
  setBusy(true);
  try{
    await apiJson("/api/wifi/reset",{method:"POST"});
    showToast("Home Wi-Fi credentials cleared.");
    await loadPresets({keepSelection:true,keepForm:true});
  }catch(e){showToast(e.message,true);}
  setBusy(false);
});

loadEffects().then(()=>loadPresets().then(()=>{drawPreview();startPreviewAnim();})).catch(e=>showToast(e.message,true));
</script>
</body>
</html>
)rawliteral";
