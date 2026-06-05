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
body{font-family:system-ui,sans-serif;margin:0;padding:16px;background:#111;color:#eee;max-width:480px;margin-inline:auto}
h1{font-size:1.25rem;margin:0 0 8px}
.banner{background:#1a2a44;border:1px solid #2563eb;border-radius:8px;padding:12px;margin-bottom:16px;font-size:.9rem;line-height:1.4}
.banner strong{color:#6af}
section{margin-bottom:20px}
label{display:block;margin:12px 0 4px;font-size:.9rem;color:#aaa}
input[type=text],input[type=number],input[type=password],textarea,select{width:100%;padding:10px;border:1px solid #333;border-radius:8px;background:#222;color:#fff;font-size:1rem}
textarea{min-height:72px;resize:vertical}
.row{display:flex;align-items:center;gap:8px;margin:8px 0}
input[type=range]{flex:1}
.seg,.slots,.effects{display:flex;gap:6px;flex-wrap:wrap;margin:8px 0}
.seg button,.slot,.effect{padding:10px 12px;border:1px solid #444;border-radius:8px;background:#222;color:#fff;cursor:pointer;min-width:42px;text-align:center}
.seg button.active,.slot.active,.effect.active{border-color:#6af;background:#1a2a44}
.slot.live{box-shadow:0 0 0 1px #6af inset}
.colors,.actions{display:flex;gap:8px;flex-wrap:wrap;margin:8px 0}
.color-btn,.action-btn{padding:8px 14px;border:1px solid #444;border-radius:8px;background:#222;color:#fff;cursor:pointer}
.color-btn.active,.action-btn.primary{border-color:#6af;background:#1a2a44}
button{width:100%;padding:12px;margin-top:12px;border:none;border-radius:8px;font-size:1rem;cursor:pointer}
.btn-primary{background:#2563eb;color:#fff}
.btn-secondary{background:#333;color:#eee;margin-top:8px}
.btn-danger{background:#444;color:#fcc;margin-top:8px}
details{background:#181818;border:1px solid #333;border-radius:8px;padding:10px;margin-top:12px}
summary{cursor:pointer;color:#aaa}
.hidden{display:none}
#status{font-size:.85rem;color:#888;margin-top:16px;line-height:1.5}
.warn{color:#fa0;font-size:.8rem;margin-top:4px}
.hint{color:#666;font-size:.8rem;margin-top:4px}
.cheat{width:100%;border-collapse:collapse;font-size:.75rem;margin-top:8px}
.cheat th,.cheat td{border:1px solid #333;padding:4px 6px;text-align:center}
.cheat th{color:#888;font-weight:600}
.cheat td{color:#ccc}
</style>
</head>
<body>
<h1>MatrixSign</h1>
<div class="banner"><strong>Field mode:</strong> Connect to Wi-Fi <strong>MatrixSign</strong> (password <strong id="apPass">matrixsign</strong>), then open <strong>http://192.168.4.1</strong></div>

<section>
<label>Presets (1–8)</label>
<div class="slots" id="slots"></div>
<div class="actions">
<button type="button" class="action-btn primary" id="saveSlot">Save to slot</button>
<button type="button" class="action-btn" id="activateSlot">Activate slot</button>
</div>
<p class="hint" id="slotInfo">Active: 1</p>
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
<label>Text block height (px)</label>
<div class="seg" id="textHeightSeg">
<button type="button" data-height="13">13</button>
<button type="button" data-height="26" class="active">26</button>
<button type="button" data-height="39">39</button>
<button type="button" data-height="52">52</button>
</div>
<div class="row">
<input type="number" id="textHeight" min="8" max="52" value="26">
<span class="hint">px total · panel is 52 px tall</span>
</div>
<details>
<summary>Text size cheatsheet (104×52 panel)</summary>
<p class="hint">Default font: 6×8 px per character at size 1. Block height splits evenly across rows.</p>
<table class="cheat" id="textCheat"></table>
</details>
<label>Rows</label>
<div class="seg" id="rowSeg">
<button type="button" data-rows="1" class="active">1</button>
<button type="button" data-rows="2">2</button>
<button type="button" data-rows="3">3</button>
<button type="button" data-rows="4">4</button>
</div>
<p class="warn" id="lenWarn" hidden>Long text will scroll — panel is 104 px wide.</p>
<label class="row"><input type="checkbox" id="scroll" checked> Scroll</label>
<label for="delay">Scroll speed (ms)</label>
<input type="number" id="delay" min="10" max="500" value="40">
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

<label for="bright">Brightness <span id="brightVal">10</span>%</label>
<input type="range" id="bright" min="1" max="100" value="10">
<button class="btn-primary" id="apply">Apply to active preset</button>
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

<div id="status">Loading...</div>
<script>
let selectedColor="#FFFFFF";
let selectedSlot=0;
let activeSlot=0;
let contentType="text";
let textHeightPx=26;
let rowCount=1;
let selectedEffect="bright_white";
let effects=[];
let presets=[];
const PANEL_W=104;
const PANEL_H=52;
const TEXT_HEIGHTS=[13,26,39,52];
const $=id=>document.getElementById(id);

function textMetrics(height,rows){
  const line=Math.floor(height/rows);
  const size=Math.max(1,Math.min(6,Math.floor(line/8)));
  const glyph=size*8;
  const chars=Math.floor(PANEL_W/(6*size));
  return{line,size,glyph,chars};
}

function renderTextCheatsheet(){
  const el=$("textCheat");
  if(!el)return;
  let html="<tr><th>Block px</th><th>Rows</th><th>Line px</th><th>Size</th><th>Glyph px</th><th>~Chars</th></tr>";
  TEXT_HEIGHTS.forEach(h=>{
    for(let rows=1;rows<=4;rows++){
      const m=textMetrics(h,rows);
      html+=`<tr><td>${h}</td><td>${rows}</td><td>${m.line}</td><td>${m.size}</td><td>${m.glyph}</td><td>${m.chars}</td></tr>`;
    }
  });
  el.innerHTML=html;
}

function resolveTextHeight(p){
  if(p.textHeightPx)return p.textHeightPx;
  const legacy={quarter:13,half:26,three_quarter:39,full:52};
  return legacy[p.fontScale]||26;
}

function setTextHeight(px){
  textHeightPx=Math.min(PANEL_H,Math.max(8,parseInt(px,10)||26));
  $("textHeight").value=textHeightPx;
  setSegActive($("textHeightSeg"),"height",textHeightPx);
}

function formBody(){
  return{
    contentType,
    effectId:selectedEffect,
    textHeightPx,
    rowCount,
    text:$("text").value,
    scroll:$("scroll").checked,
    scrollDelayMs:parseInt($("delay").value,10),
    brightness:parseInt($("bright").value,10),
    color:selectedColor
  };
}

function setSegActive(container,matchAttr,value){
  container.querySelectorAll("button").forEach(b=>b.classList.toggle("active",b.dataset[matchAttr]===String(value)));
}

function showSections(){
  $("textSection").classList.toggle("hidden",contentType!=="text");
  $("gifSection").classList.toggle("hidden",contentType!=="gif");
  $("effectSection").classList.toggle("hidden",contentType!=="effect");
  setSegActive($("contentTypeSeg"),"type",contentType);
  setSegActive($("textHeightSeg"),"height",textHeightPx);
  setSegActive($("rowSeg"),"rows",rowCount);
}

function fillForm(p){
  contentType=(p.contentType||"text").toLowerCase();
  setTextHeight(resolveTextHeight(p));
  rowCount=p.rowCount||1;
  selectedEffect=p.effectId||"bright_white";
  $("text").value=p.text||"";
  $("scroll").checked=!!p.scroll;
  $("delay").value=p.scrollDelayMs||40;
  const bright=p.brightness||10;
  $("bright").value=bright;
  $("brightVal").textContent=bright;
  selectedColor=(p.color||"#FFFFFF").toUpperCase();
  $("color").value=selectedColor;
  document.querySelectorAll(".color-btn").forEach(b=>b.classList.toggle("active",b.dataset.color.toUpperCase()===selectedColor));
  showSections();
  renderEffects();
  updateLenWarn();
}

function selectSlot(index){
  selectedSlot=index;
  fillForm(presets[index]||{});
  renderSlots();
  updateSlotInfo();
}

function renderEffects(){
  const el=$("effectGrid");
  el.innerHTML="";
  effects.forEach(e=>{
    const b=document.createElement("button");
    b.type="button";
    b.className="effect"+(e.id===selectedEffect?" active":"");
    b.dataset.id=e.id;
    b.textContent=e.label;
    b.onclick=()=>{selectedEffect=e.id;renderEffects();};
    el.appendChild(b);
  });
}

function renderSlots(){
  const el=$("slots");
  el.innerHTML="";
  for(let i=0;i<8;i++){
    const b=document.createElement("button");
    b.type="button";
    b.className="slot"+(i===selectedSlot?" active":"")+(i===activeSlot?" live":"");
    b.textContent=i+1;
    b.onclick=()=>selectSlot(i);
    el.appendChild(b);
  }
}

function slotSummary(p){
  if(!p)return"empty";
  if(p.contentType==="effect")return"Effect: "+(p.effectLabel||p.effectId);
  if(p.contentType==="gif"&&p.hasGif)return"GIF";
  return`Message (${resolveTextHeight(p)} px, ${p.rowCount||1} row)`;
}

function updateSlotInfo(){
  const p=presets[selectedSlot]||{};
  $("slotInfo").textContent=`Editing slot ${selectedSlot+1} · Active: ${activeSlot+1} · ${slotSummary(p)}`;
}

function updateStatus(c){
  let s=`AP ${c.apSsid||"MatrixSign"} · ${c.apIp||"192.168.4.1"}`;
  if(c.apPassword)s+=` · pass ${c.apPassword}`;
  if(c.staConnected)s+=` · Home ${c.staIp} (${c.staRssi} dBm)`;
  $("status").textContent=s;
  if(c.apPassword)$("apPass").textContent=c.apPassword;
}

async function loadEffects(){
  const r=await fetch("/api/effects");
  const data=await r.json();
  effects=data.effects||[];
  renderEffects();
}

async function loadPresets(opts={}){
  const keepSelection=!!opts.keepSelection;
  const keepForm=!!opts.keepForm;
  const prevSlot=selectedSlot;
  const r=await fetch("/api/presets");
  const data=await r.json();
  presets=data.presets||[];
  activeSlot=data.activeIndex||0;
  if(!keepSelection){
    selectedSlot=activeSlot;
  }else{
    selectedSlot=Math.min(Math.max(prevSlot,0),presets.length-1);
  }
  if(!keepForm){
    fillForm(presets[selectedSlot]||{});
  }
  renderSlots();
  updateSlotInfo();
  updateStatus(data);
}

function updateLenWarn(){$("lenWarn").hidden=($("text").value.length<20);}

document.querySelectorAll("#contentTypeSeg button").forEach(b=>b.addEventListener("click",()=>{
  contentType=b.dataset.type;showSections();updateSlotInfo();
}));
document.querySelectorAll("#textHeightSeg button").forEach(b=>b.addEventListener("click",()=>{
  setTextHeight(parseInt(b.dataset.height,10));showSections();
}));
$("textHeight").addEventListener("input",e=>setTextHeight(e.target.value));
document.querySelectorAll("#rowSeg button").forEach(b=>b.addEventListener("click",()=>{
  rowCount=parseInt(b.dataset.rows,10);showSections();
}));
document.querySelectorAll(".color-btn").forEach(b=>b.addEventListener("click",()=>{
  selectedColor=b.dataset.color;$("color").value=selectedColor;
  document.querySelectorAll(".color-btn").forEach(x=>x.classList.remove("active"));b.classList.add("active");
}));
$("color").addEventListener("input",e=>{selectedColor=e.target.value;document.querySelectorAll(".color-btn").forEach(x=>x.classList.remove("active"));});
$("bright").addEventListener("input",e=>{$("brightVal").textContent=e.target.value;});
$("text").addEventListener("input",updateLenWarn);

$("apply").addEventListener("click",async()=>{
  const r=await fetch("/api/config",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify(formBody())});
  $("status").textContent=r.ok?"Applied to active preset.":"Apply failed.";
  if(r.ok)loadPresets({keepSelection:true,keepForm:true});
});

$("saveSlot").addEventListener("click",async()=>{
  const body={...formBody(),id:selectedSlot};
  const r=await fetch("/api/presets",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify(body)});
  $("status").textContent=r.ok?`Saved slot ${selectedSlot+1}.`:"Save failed.";
  if(r.ok)loadPresets({keepSelection:true,keepForm:true});
});

$("activateSlot").addEventListener("click",async()=>{
  const r=await fetch("/api/presets/select",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({id:selectedSlot})});
  $("status").textContent=r.ok?`Activated slot ${selectedSlot+1}.`:"Activate failed.";
  if(r.ok)loadPresets({keepSelection:true,keepForm:true});
});

$("uploadGif").addEventListener("click",async()=>{
  const f=$("gifFile").files[0];
  if(!f){$("status").textContent="Choose a GIF file first.";return;}
  if(f.size>262144){$("status").textContent="GIF too large (max 256 KB).";return;}
  const fd=new FormData();
  fd.append("file",f,f.name);
  const r=await fetch(`/api/presets/gif?id=${selectedSlot}`,{method:"POST",body:fd});
  $("status").textContent=r.ok?`GIF uploaded to slot ${selectedSlot+1}.`:"Upload failed.";
  if(r.ok){contentType="gif";showSections();loadPresets({keepSelection:true,keepForm:true});}
});

$("removeGif").addEventListener("click",async()=>{
  const r=await fetch(`/api/presets/gif?id=${selectedSlot}`,{method:"DELETE"});
  $("status").textContent=r.ok?`GIF removed from slot ${selectedSlot+1}.`:"Remove failed.";
  if(r.ok)loadPresets({keepSelection:true,keepForm:true});
});

$("connectWifi").addEventListener("click",async()=>{
  const body={ssid:$("ssid").value,password:$("password").value};
  const r=await fetch("/api/wifi/connect",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify(body)});
  $("status").textContent=r.ok?"Connecting to home Wi-Fi in background...":"Connect failed.";
  setTimeout(loadPresets,2000);
});

$("wifiReset").addEventListener("click",async()=>{
  if(!confirm("Forget saved home Wi-Fi credentials?"))return;
  await fetch("/api/wifi/reset",{method:"POST"});
  $("status").textContent="Home Wi-Fi credentials cleared.";
  loadPresets();
});

loadEffects().then(()=>{renderTextCheatsheet();loadPresets();});
</script>
</body>
</html>
)rawliteral";
