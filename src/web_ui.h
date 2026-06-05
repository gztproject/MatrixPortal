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
h1{font-size:1.25rem;margin:0 0 16px}
label{display:block;margin:12px 0 4px;font-size:.9rem;color:#aaa}
input[type=text],input[type=number]{width:100%;padding:10px;border:1px solid #333;border-radius:8px;background:#222;color:#fff;font-size:1rem}
.row{display:flex;align-items:center;gap:8px;margin:8px 0}
input[type=range]{flex:1}
.presets{display:flex;gap:8px;flex-wrap:wrap;margin:8px 0}
.preset{padding:8px 14px;border:1px solid #444;border-radius:8px;background:#222;color:#fff;cursor:pointer}
.preset.active{border-color:#6af;background:#1a2a44}
button{width:100%;padding:12px;margin-top:12px;border:none;border-radius:8px;font-size:1rem;cursor:pointer}
.btn-primary{background:#2563eb;color:#fff}
.btn-danger{background:#444;color:#fcc;margin-top:8px}
#status{font-size:.85rem;color:#888;margin-top:16px;line-height:1.5}
.warn{color:#fa0;font-size:.8rem;margin-top:4px}
</style>
</head>
<body>
<h1>MatrixSign</h1>
<label for="text">Message</label>
<input type="text" id="text" maxlength="200" placeholder="HELLO">
<p class="warn" id="lenWarn" hidden>Long text will scroll — panel is 104 px wide.</p>
<label class="row"><input type="checkbox" id="scroll" checked> Scroll</label>
<label for="delay">Scroll speed (ms)</label>
<input type="number" id="delay" min="10" max="500" value="40">
<label for="bright">Brightness <span id="brightVal">10</span>%</label>
<input type="range" id="bright" min="1" max="100" value="10">
<label>Color</label>
<div class="presets">
<button type="button" class="preset active" data-color="#FFFFFF">White</button>
<button type="button" class="preset" data-color="#FF0000">Red</button>
<button type="button" class="preset" data-color="#00FF00">Green</button>
<button type="button" class="preset" data-color="#FFAA00">Amber</button>
</div>
<input type="color" id="color" value="#ffffff">
<button class="btn-primary" id="apply">Apply</button>
<button class="btn-danger" id="wifiReset">Reset Wi-Fi</button>
<div id="status">Loading...</div>
<script>
let selectedColor="#FFFFFF";
const $=id=>document.getElementById(id);
async function loadConfig(){
  const r=await fetch("/api/config");
  const c=await r.json();
  $("text").value=c.text||"";
  $("scroll").checked=!!c.scroll;
  $("delay").value=c.scrollDelayMs||40;
  $("bright").value=c.brightness||10;
  $("brightVal").textContent=c.brightness||10;
  selectedColor=c.color||"#FFFFFF";
  $("color").value=selectedColor;
  document.querySelectorAll(".preset").forEach(b=>b.classList.toggle("active",b.dataset.color.toUpperCase()===selectedColor.toUpperCase()));
  $("status").textContent=`IP: ${c.ip||"—"} · RSSI: ${c.rssi??"—"} dBm`;
  updateLenWarn();
}
function updateLenWarn(){
  $("lenWarn").hidden=($("text").value.length<20);
}
document.querySelectorAll(".preset").forEach(b=>b.addEventListener("click",()=>{
  selectedColor=b.dataset.color;
  $("color").value=selectedColor;
  document.querySelectorAll(".preset").forEach(x=>x.classList.remove("active"));
  b.classList.add("active");
}));
$("color").addEventListener("input",e=>{selectedColor=e.target.value;document.querySelectorAll(".preset").forEach(x=>x.classList.remove("active"));});
$("bright").addEventListener("input",e=>{$("brightVal").textContent=e.target.value;});
$("text").addEventListener("input",updateLenWarn);
$("apply").addEventListener("click",async()=>{
  const body={text:$("text").value,scroll:$("scroll").checked,scrollDelayMs:parseInt($("delay").value,10),brightness:parseInt($("bright").value,10),color:selectedColor};
  const r=await fetch("/api/config",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify(body)});
  if(r.ok){$("status").textContent="Applied.";loadConfig();}
  else{$("status").textContent="Apply failed.";}
});
$("wifiReset").addEventListener("click",async()=>{
  if(!confirm("Clear Wi-Fi and reboot to setup AP?"))return;
  await fetch("/api/wifi/reset",{method:"POST"});
  $("status").textContent="Rebooting to setup mode...";
});
loadConfig();
</script>
</body>
</html>
)rawliteral";
