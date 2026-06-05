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
input[type=text],input[type=number],input[type=password]{width:100%;padding:10px;border:1px solid #333;border-radius:8px;background:#222;color:#fff;font-size:1rem}
.row{display:flex;align-items:center;gap:8px;margin:8px 0}
input[type=range]{flex:1}
.slots{display:flex;gap:6px;flex-wrap:wrap;margin:8px 0}
.slot{padding:10px 12px;border:1px solid #444;border-radius:8px;background:#222;color:#fff;cursor:pointer;min-width:42px;text-align:center}
.slot.active{border-color:#6af;background:#1a2a44}
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
#status{font-size:.85rem;color:#888;margin-top:16px;line-height:1.5}
.warn{color:#fa0;font-size:.8rem;margin-top:4px}
.hint{color:#666;font-size:.8rem;margin-top:4px}
</style>
</head>
<body>
<h1>MatrixSign</h1>
<div class="banner"><strong>Field mode:</strong> Connect phone to Wi-Fi <strong>MatrixSign</strong>, then open <strong>http://192.168.4.1</strong></div>

<section>
<label>Presets (1–8)</label>
<div class="slots" id="slots"></div>
<div class="actions">
<button type="button" class="action-btn primary" id="saveSlot">Save to slot</button>
<button type="button" class="action-btn" id="activateSlot">Activate slot</button>
</div>
<p class="hint" id="slotInfo">Active: 1 · GIF: none</p>
<label>GIF for selected slot</label>
<input type="file" id="gifFile" accept="image/gif">
<div class="actions">
<button type="button" class="action-btn primary" id="uploadGif">Upload GIF</button>
<button type="button" class="action-btn" id="removeGif">Remove GIF</button>
</div>
<p class="hint">Best: 104×52 px, max 256 KB. GIF replaces text on the panel when uploaded.</p>
</section>

<section>
<label for="text">Message</label>
<input type="text" id="text" maxlength="200" placeholder="HELLO">
<p class="warn" id="lenWarn" hidden>Long text will scroll — panel is 104 px wide.</p>
<label class="row"><input type="checkbox" id="scroll" checked> Scroll</label>
<label for="delay">Scroll speed (ms)</label>
<input type="number" id="delay" min="10" max="500" value="40">
<label for="bright">Brightness <span id="brightVal">10</span>%</label>
<input type="range" id="bright" min="1" max="100" value="10">
<label>Color</label>
<div class="colors">
<button type="button" class="color-btn active" data-color="#FFFFFF">White</button>
<button type="button" class="color-btn" data-color="#FF0000">Red</button>
<button type="button" class="color-btn" data-color="#00FF00">Green</button>
<button type="button" class="color-btn" data-color="#FFAA00">Amber</button>
</div>
<input type="color" id="color" value="#ffffff">
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
let presets=[];
const $=id=>document.getElementById(id);

function formBody(){
  return{text:$("text").value,scroll:$("scroll").checked,scrollDelayMs:parseInt($("delay").value,10),brightness:parseInt($("bright").value,10),color:selectedColor};
}

function fillForm(p){
  $("text").value=p.text||"";
  $("scroll").checked=!!p.scroll;
  $("delay").value=p.scrollDelayMs||40;
  $("bright").value=p.brightness||10;
  $("brightVal").textContent=p.brightness||10;
  selectedColor=p.color||"#FFFFFF";
  $("color").value=selectedColor;
  document.querySelectorAll(".color-btn").forEach(b=>b.classList.toggle("active",b.dataset.color.toUpperCase()===selectedColor.toUpperCase()));
  updateLenWarn();
}

function renderSlots(){
  const el=$("slots");
  el.innerHTML="";
  for(let i=0;i<8;i++){
    const b=document.createElement("button");
    b.type="button";
    b.className="slot"+(i===selectedSlot?" active":"")+(i===activeSlot?" live":"");
    b.textContent=i+1;
    b.onclick=()=>{selectedSlot=i;fillForm(presets[i]||{});renderSlots();updateSlotInfo();};
    el.appendChild(b);
  }
}

function updateSlotInfo(){
  const p=presets[selectedSlot]||{};
  $("slotInfo").textContent=`Editing slot ${selectedSlot+1} · Active: ${activeSlot+1} · GIF: ${p.hasGif?"yes":"none"}`;
}

function updateStatus(c){
  let s=`AP ${c.apSsid||"MatrixSign"} · ${c.apIp||"192.168.4.1"}`;
  if(c.staConnected)s+=` · Home ${c.staIp} (${c.staRssi} dBm)`;
  $("status").textContent=s;
}

async function loadPresets(){
  const r=await fetch("/api/presets");
  const data=await r.json();
  presets=data.presets||[];
  activeSlot=data.activeIndex||0;
  selectedSlot=activeSlot;
  fillForm(presets[selectedSlot]||{});
  renderSlots();
  updateSlotInfo();
  updateStatus(data);
}

function updateLenWarn(){$("lenWarn").hidden=($("text").value.length<20);}

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
  if(r.ok)loadPresets();
});

$("saveSlot").addEventListener("click",async()=>{
  const body={...formBody(),id:selectedSlot};
  const r=await fetch("/api/presets",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify(body)});
  $("status").textContent=r.ok?`Saved slot ${selectedSlot+1}.`:"Save failed.";
  if(r.ok)loadPresets();
});

$("activateSlot").addEventListener("click",async()=>{
  const r=await fetch("/api/presets/select",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({id:selectedSlot})});
  $("status").textContent=r.ok?`Activated slot ${selectedSlot+1}.`:"Activate failed.";
  if(r.ok)loadPresets();
});

$("uploadGif").addEventListener("click",async()=>{
  const f=$("gifFile").files[0];
  if(!f){$("status").textContent="Choose a GIF file first.";return;}
  if(f.size>262144){$("status").textContent="GIF too large (max 256 KB).";return;}
  const fd=new FormData();
  fd.append("file",f,f.name);
  const r=await fetch(`/api/presets/gif?id=${selectedSlot}`,{method:"POST",body:fd});
  $("status").textContent=r.ok?`GIF uploaded to slot ${selectedSlot+1}.`:"Upload failed.";
  if(r.ok)loadPresets();
});

$("removeGif").addEventListener("click",async()=>{
  const r=await fetch(`/api/presets/gif?id=${selectedSlot}`,{method:"DELETE"});
  $("status").textContent=r.ok?`GIF removed from slot ${selectedSlot+1}.`:"Remove failed.";
  if(r.ok)loadPresets();
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

loadPresets();
</script>
</body>
</html>
)rawliteral";
