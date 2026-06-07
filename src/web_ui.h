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
<p class="hint" id="slotHelp">Try on panel shows the editor without saving. Show saved loads NVS. Save writes this slot.</p>
<label for="slotLabel">Slot label</label>
<input type="text" id="slotLabel" maxlength="16" placeholder="Optional name">
<div class="actions">
<button type="button" class="action-btn" id="duplicateSlot">Duplicate to…</button>
</div>
<p class="hint" id="slotInfo"></p>
</section>

<section id="playlistSection">
<label class="row"><input type="checkbox" id="playlistEnabled"> Auto-rotate presets</label>
<p class="hint">Cycles checked slots on the panel. Short press UP/DOWN still switches manually; hold for brightness in 10% steps.</p>
<label>Dwell time (seconds)</label>
<input type="number" id="playlistDwell" min="2" max="3600" value="8">
<div class="seg" id="playlistSlots"></div>
<button type="button" class="action-btn" id="savePlaylist">Save playlist</button>
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
<button type="button" data-type="clock">Clock</button>
<button type="button" data-type="countdown">Countdown</button>
<button type="button" data-type="gif">GIF</button>
<button type="button" data-type="effect">Effect</button>
</div>

<div id="textSection">
<label for="messageText">Message</label>
<textarea id="messageText" maxlength="200" placeholder="HELLO&#10;LINE TWO"></textarea>
<p class="hint">Use line breaks for multiple rows (max rows setting below). Supports č, š, ž.</p>
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
<div id="textOffsetSection">
<label>Text offset (px)</label>
<div class="row">
<span>X</span>
<input type="number" id="offsetX" min="-52" max="52" value="0" style="width:28%">
<span>Y</span>
<input type="number" id="offsetY" min="-52" max="52" value="0" style="width:28%">
</div>
<p class="hint">Nudges message text only — effects, GIF, clock, and countdown are unchanged.</p>
</div>
</div>

<div id="colorSection" class="hidden">
<label for="color">Color</label>
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
<label for="effectParam" id="effectParamLabel" class="hidden">Progress <span id="effectParamVal">50</span>%</label>
<input type="range" id="effectParam" class="hidden" min="0" max="100" value="50">
<p class="hint">STOP and hazard use fixed colours. Other effects can be recoloured below.</p>
</div>

<div id="timeSettingsSection" class="hidden">
<label for="timezone">Time zone</label>
<select id="timezone">
<option value="CET">Central European (CET/CEST)</option>
<option value="UTC">UTC</option>
<option value="WET">Western European (WET/WEST)</option>
<option value="EET">Eastern European (EET/EEST)</option>
<option value="GMT">UK (GMT/BST)</option>
</select>
<p class="hint" id="timeStatus">Time not set.</p>
<button type="button" class="action-btn" id="syncBrowserTime">Sync time from this device</button>
<p class="hint">Use when NTP is unavailable (AP-only). Home Wi-Fi uses NTP automatically when connected.</p>
</div>

<div id="clockSection" class="hidden">
<label class="row"><input type="checkbox" id="clockSeconds"> Show seconds</label>
<label class="row"><input type="checkbox" id="clockDate"> Show date (dd.mm.yyyy)</label>
</div>

<div id="countdownSection" class="hidden">
<label>Countdown mode</label>
<div class="seg" id="countdownModeSeg">
<button type="button" data-mode="target" class="active">Target time</button>
<button type="button" data-mode="duration">Duration</button>
</div>
<div id="countdownTargetWrap">
<label for="countdownAt">Target date/time</label>
<input type="datetime-local" id="countdownAt">
<p class="hint">Counts down to a moment in time. Set time zone above; sync from browser if NTP is unavailable.</p>
</div>
<div id="countdownDurationWrap" class="hidden">
<label>Duration (h : m : s)</label>
<div class="row">
<input type="number" id="countdownHours" min="0" max="99" value="0" aria-label="Hours">
<span>:</span>
<input type="number" id="countdownMinutes" min="0" max="59" value="5" aria-label="Minutes">
<span>:</span>
<input type="number" id="countdownSeconds" min="0" max="59" value="0" aria-label="Seconds">
</div>
<p class="hint">Starts when the slot is shown. No Wi-Fi needed. Resets each time the slot is activated.</p>
</div>
</div>
</section>

<details>
<summary>Backup &amp; restore</summary>
<p class="hint">Export all slots, labels, playlist, and brightness as JSON.</p>
<div class="actions">
<button type="button" class="action-btn" id="downloadBackup">Download backup</button>
<input type="file" id="restoreFile" accept="application/json,.json" hidden>
<button type="button" class="action-btn" id="restoreBackup">Restore backup</button>
</div>
</details>

<details>
<summary>Security</summary>
<p class="hint">Web UI uses HTTP Basic Auth. Default login <strong>admin</strong> / <strong>admin</strong> — change under Security before field use.</p>
<label for="newAdminPass">New Web UI password</label>
<input type="password" id="newAdminPass" autocomplete="new-password" minlength="8">
<button type="button" class="btn-secondary" id="changeAdminPass">Change Web UI password</button>
</details>

<details>
<summary>Firmware upgrade</summary>
<p class="hint">Current version: <strong id="fwVersion">—</strong>. Manual upload works on AP. Remote check/upgrade needs home Wi‑Fi.</p>
<label for="otaUrl">OTA URL</label>
<input type="text" id="otaUrl" autocomplete="off" spellcheck="false">
<button type="button" class="btn-secondary" id="saveOtaUrl">Save OTA URL</button>
<p class="hint" id="otaStatus">Ready.</p>
<div class="actions">
<button type="button" class="action-btn" id="checkFirmware">Check for updates</button>
<button type="button" class="action-btn" id="upgradeFirmware" disabled>Upgrade from URL</button>
</div>
<input type="file" id="firmwareFile" accept=".bin,application/octet-stream" hidden>
<button type="button" class="btn-secondary" id="pickFirmware">Choose firmware file…</button>
<button type="button" class="action-btn" id="uploadFirmware" disabled>Upgrade from file</button>
</details>

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
<button type="button" class="secondary" id="tryOnPanel">Try on panel</button>
<button type="button" class="secondary" id="showSlot">Show saved</button>
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
let effectParam=50;
let playlistEnabled=false;
let playlistMask=0xFF;
let playlistDwellMs=8000;
let countdownMode="target";
const EFFECT_DEFAULT_COLORS={
  bright_white:"#FFFFFF",
  flashing_halves:"#FFAA00",
  full_strobe:"#FF0000",
  pulse:"#0066FF",
  border_chase:"#00FF00",
  progress_bar:"#00FF00",
  game_of_life:"#00FF00",
  arrow_left:"#FFAA00",
  arrow_right:"#FFAA00"
};
let globalBrightness=10;
let effects=[];
let presets=[];
let presetsReady=false;
let connData={};
let busy=false;
let toastTimer=null;
let brightTimer=null;
let previewScroll=104;
let previewTimer=null;
let firmwareVersion="";
let otaPollTimer=null;
let selectedFirmwareFile=null;
const PANEL_W=104;
const PANEL_H=52;
const $=id=>document.getElementById(id);

function setMessageText(value){
  const el=$("messageText");
  if(el)el.value=value==null?"":String(value);
}

function messageTextValue(){
  const el=$("messageText");
  return el?el.value:"";
}

function normalizeEffectId(id){
  if(id==="blue_emergency"||id==="yellow_emergency")return"flashing_halves";
  return id||"bright_white";
}

function blockHeightPx(){return glyphHeightPx*rowCount;}

function glyphFromBlock(block,rows){
  const lineH=Math.floor(block/Math.max(1,rows));
  const size=Math.max(1,Math.min(6,Math.floor(lineH/8)));
  return size*8;
}

function utf8CodepointCount(str){
  let n=0;
  for(let i=0;i<str.length;){
    const c=str.charCodeAt(i);
    if(c<0x80)i+=1;
    else if(c<0x800)i+=2;
    else if(c<0xD800||c>=0xE000)i+=3;
    else i+=4;
    n++;
  }
  return n;
}

function isExtendedLatin(ch){
  return"čšžČŠŽ".includes(ch);
}

function lineHasCaron(line){
  for(const ch of line){if(isExtendedLatin(ch))return true;}
  return false;
}

function optimalGlyphHeight(rows,text,scroll){
  const lines=text.split("\n").slice(0,rows);
  while(lines.length<rows)lines.push("");
  const maxLen=Math.max(1,...lines.map(l=>utf8CodepointCount(l)));
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
  const opt=optimalGlyphHeight(rowCount,messageTextValue(),$("scroll").checked);
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
  if($("tryOnPanel"))$("tryOnPanel").disabled=v;
}

async function apiJson(url,opts={}){
  opts.credentials=opts.credentials||"include";
  const r=await fetch(url,opts);
  let data={};
  try{data=await r.json();}catch(e){}
  if(r.status===401){
    throw new Error("Login required — default admin/admin, or use Security to set a new password");
  }
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

function normalizedBlockHeight(p){
  const rows=p.rowCount||1;
  return glyphFromBlock(resolveBlockHeight(p),rows)*rows;
}

function clockEffectParam(){
  let v=0;
  if($("clockSeconds").checked)v|=1;
  if($("clockDate").checked)v|=2;
  return v;
}

function presetFormBody(){
  const body={
    contentType,
    effectId:normalizeEffectId(selectedEffect),
    textHeightPx:blockHeightPx(),
    rowCount,
    text:messageTextValue(),
    label:$("slotLabel").value.trim(),
    scroll:$("scroll").checked,
    scrollDelayMs:parseInt($("delay").value,10)||40,
    color:selectedColor,
    effectParam:contentType==="clock"?clockEffectParam():effectParam,
    countdownEndUnix:countdownEndUnix(),
    countdownDurationSec:countdownDurationSec(),
    contentOffsetX:parseInt($("offsetX").value,10)||0,
    contentOffsetY:parseInt($("offsetY").value,10)||0
  };
  return body;
}

function countdownDurationSec(){
  if(contentType!=="countdown"||countdownMode!=="duration")return 0;
  const h=parseInt($("countdownHours").value,10)||0;
  const m=parseInt($("countdownMinutes").value,10)||0;
  const s=parseInt($("countdownSeconds").value,10)||0;
  return h*3600+m*60+s;
}

function setCountdownDurationFields(totalSec){
  const sec=Math.max(0,parseInt(totalSec,10)||0);
  $("countdownHours").value=Math.floor(sec/3600);
  $("countdownMinutes").value=Math.floor((sec%3600)/60);
  $("countdownSeconds").value=sec%60;
}

function updateCountdownModeUi(){
  $("countdownTargetWrap").classList.toggle("hidden",countdownMode!=="target");
  $("countdownDurationWrap").classList.toggle("hidden",countdownMode!=="duration");
  setSegActive($("countdownModeSeg"),"mode",countdownMode);
}

function countdownEndUnix(){
  if(contentType!=="countdown"||countdownMode!=="target")return 0;
  const v=$("countdownAt").value;
  if(!v)return 0;
  return Math.floor(new Date(v).getTime()/1000);
}

function unixToDatetimeLocal(unix){
  if(!unix)return"";
  const d=new Date(unix*1000);
  const pad=n=>String(n).padStart(2,"0");
  return`${d.getFullYear()}-${pad(d.getMonth()+1)}-${pad(d.getDate())}T${pad(d.getHours())}:${pad(d.getMinutes())}`;
}

function syncEffectParamUi(){
  $("effectParam").value=effectParam;
  $("effectParamVal").textContent=effectParam;
}

function applyDefaultEffectColor(id){
  const hex=EFFECT_DEFAULT_COLORS[id];
  if(!hex||id==="stop"||id==="hazard_triangle")return;
  selectedColor=hex.toUpperCase();
  $("color").value=selectedColor;
  document.querySelectorAll(".color-btn").forEach(b=>b.classList.toggle("active",b.dataset.color.toUpperCase()===selectedColor));
}

function savedPresetBody(p){
  if(!p)return null;
  const rows=p.rowCount||1;
  return{
    contentType:(p.contentType||"text").toLowerCase(),
    effectId:normalizeEffectId(p.effectId||"bright_white"),
    textHeightPx:normalizedBlockHeight(p),
    rowCount:rows,
    text:p.text||"",
    label:p.label||"",
    scroll:!!p.scroll,
    scrollDelayMs:p.scrollDelayMs||40,
    color:(p.color||"#FFFFFF").toUpperCase(),
    effectParam:p.effectParam!=null?p.effectParam:50,
    countdownEndUnix:p.countdownEndUnix||0,
    countdownDurationSec:p.countdownDurationSec||0,
    contentOffsetX:p.contentOffsetX||0,
    contentOffsetY:p.contentOffsetY||0
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

function effectIsMonochrome(id){
  const e=effects.find(x=>x.id===id);
  return!!(e&&e.monochrome);
}

function showSections(){
  $("textSection").classList.toggle("hidden",contentType!=="text");
  $("gifSection").classList.toggle("hidden",contentType!=="gif");
  $("effectSection").classList.toggle("hidden",contentType!=="effect");
  $("clockSection").classList.toggle("hidden",contentType!=="clock");
  $("countdownSection").classList.toggle("hidden",contentType!=="countdown");
  $("timeSettingsSection").classList.toggle("hidden",contentType!=="clock"&&contentType!=="countdown");
  const showParam=contentType==="effect"&&selectedEffect==="progress_bar";
  $("effectParam").classList.toggle("hidden",!showParam);
  $("effectParamLabel").classList.toggle("hidden",!showParam);
  const showColor=contentType==="text"||(contentType==="effect"&&effectIsMonochrome(selectedEffect));
  $("colorSection").classList.toggle("hidden",!showColor);
  setSegActive($("contentTypeSeg"),"type",contentType);
  setSegActive($("rowSeg"),"rows",rowCount);
  updateFontSizeUi();
  syncScrollSeg();
  updateGifStatus();
  drawPreview();
}

function slotLabel(p){
  if(!p)return"empty";
  if(p.label&&p.label.trim())return p.label.trim().slice(0,12);
  if(p.contentType==="effect")return(p.effectLabel||p.effectId||"Effect").slice(0,12);
  if(p.contentType==="clock")return"Clock";
  if(p.contentType==="countdown")return"Countdown";
  if(p.contentType==="gif"&&p.hasGif)return"GIF";
  const t=(p.text||"").split("\n")[0].trim();
  return(t||"Message").slice(0,12);
}

function fillForm(p){
  contentType=(p.contentType||"text").toLowerCase();
  setMessageText(p.text);
  $("slotLabel").value=p.label||"";
  rowCount=p.rowCount||1;
  selectedEffect=normalizeEffectId(p.effectId);
  if((p.countdownDurationSec||0)>0){
    countdownMode="duration";
    setCountdownDurationFields(p.countdownDurationSec);
    $("countdownAt").value="";
  }else{
    countdownMode="target";
    $("countdownAt").value=unixToDatetimeLocal(p.countdownEndUnix||0);
    setCountdownDurationFields(300);
  }
  updateCountdownModeUi();
  const ep=p.effectParam!=null?p.effectParam:0;
  if(contentType==="clock"){
    $("clockSeconds").checked=!!((ep&1)||ep>3);
    $("clockDate").checked=!!(ep&2);
    effectParam=ep;
  }else{
    $("clockSeconds").checked=false;
    $("clockDate").checked=false;
    effectParam=ep||50;
  }
  syncEffectParamUi();
  $("offsetX").value=p.contentOffsetX||0;
  $("offsetY").value=p.contentOffsetY||0;
  $("scroll").checked=!!p.scroll;
  $("delay").value=p.scrollDelayMs||40;
  selectedColor=(p.color||"#FFFFFF").toUpperCase();
  $("color").value=selectedColor;
  document.querySelectorAll(".color-btn").forEach(b=>b.classList.toggle("active",b.dataset.color.toUpperCase()===selectedColor));
  setGlyphHeight(glyphFromBlock(resolveBlockHeight(p),rowCount));
  showSections();
  renderEffects();
  updateLenWarn();
  renderSlots();
}

async function selectSlot(index){
  selectedSlot=index;
  if(!presetsReady){
    fillForm(presets[index]||{});
    updateSlotInfo();
    return;
  }
  try{
    await loadPresets({keepSelection:true, keepForm:false});
  }catch(e){
    fillForm(presets[index]||{});
    showToast(e.message,true);
  }
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
    b.onclick=()=>{selectedEffect=e.id;applyDefaultEffectColor(e.id);renderEffects();showSections();drawPreview();renderSlots();};
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
  $("showSlot").textContent=`Show saved ${selectedSlot+1}`;
  if($("tryOnPanel"))$("tryOnPanel").textContent="Try on panel";
}

function updateSlotInfo(){
  const p=presets[selectedSlot]||{};
  $("slotInfo").textContent=`Editing slot ${selectedSlot+1} · Live: slot ${activeSlot+1} · ${slotLabel(p)}`;
}

function renderPlaylist(){
  const el=$("playlistSlots");
  if(!el)return;
  el.innerHTML="";
  for(let i=0;i<8;i++){
    const b=document.createElement("button");
    b.type="button";
    const on=!!(playlistMask&(1<<i));
    b.className=on?"active":"";
    b.textContent=String(i+1);
    b.setAttribute("aria-pressed",on?"true":"false");
    b.onclick=()=>{playlistMask^=(1<<i);renderPlaylist();};
    el.appendChild(b);
  }
  $("playlistEnabled").checked=playlistEnabled;
  $("playlistDwell").value=Math.max(2,Math.round(playlistDwellMs/1000));
}

function updateTimeStatus(data){
  const el=$("timeStatus");
  if(!el)return;
  if(data.timeValid){
    if(data.timeSource==="ntp"){
      el.textContent="Time synced via NTP.";
    }else if(data.timeSource==="browser"){
      el.textContent="Time synced from this browser.";
    }else{
      el.textContent="Time is set.";
    }
  }else if(data.staConnected){
    el.textContent="Waiting for NTP… or sync from this device below.";
  }else{
    el.textContent="Time not set — sync from this device below (AP mode).";
  }
}

function updateBanner(c){
  if(c.staConnected){
    $("banner").innerHTML=`<strong>Connected</strong> · ${c.staIp||""}${c.staRssi!=null?` · ${c.staRssi} dBm`:""} · open <strong>http://${c.staIp||""}/</strong>`;
    return;
  }
  $("banner").innerHTML=`<strong>Field mode:</strong> join Wi-Fi <strong>${c.apSsid||"MatrixSign"}</strong> (password set at build time — see README), then open <strong>http://${c.apIp||"192.168.4.1"}</strong>. Web UI login: <strong>admin</strong> / <strong>admin</strong>.`;
}

function updateConnBar(c){
  connData=c||connData;
  let s="";
  const ver=firmwareVersion||c.firmwareVersion||"";
  if(c.staConnected)s=`Home ${c.staIp} · ${c.staRssi} dBm · v${ver||"?"} · brightness ${globalBrightness}%`;
  else s=`AP ${c.apSsid||"MatrixSign"} · ${c.apIp||"192.168.4.1"} · v${ver||"?"} · brightness ${globalBrightness}%`;
  $("connBar").textContent=s;
}

function updateFirmwareUi(data){
  if(!data)return;
  if(data.firmwareVersion||data.version){
    firmwareVersion=data.firmwareVersion||data.version;
    $("fwVersion").textContent=firmwareVersion;
  }
  if(data.otaUrl&&$("otaUrl")&&!$("otaUrl").matches(":focus"))$("otaUrl").value=data.otaUrl;
  const state=data.otaState||data.state||"idle";
  const progress=typeof data.otaProgress==="number"?data.otaProgress:(typeof data.progress==="number"?data.progress:0);
  const remote=data.otaRemoteVersion||data.remoteVersion||"";
  const avail=!!(data.otaUpdateAvailable||data.updateAvailable);
  let msg="Ready.";
  if(state==="checking")msg="Checking for updates…";
  else if(state==="downloading")msg=`Downloading… ${progress}%`;
  else if(state==="flashing")msg=`Flashing… ${progress}%`;
  else if(state==="error")msg=data.otaError||data.error||"Update failed.";
  else if(remote){
    msg=avail?`Update available: v${remote}`:`Remote v${remote} · up to date (v${firmwareVersion})`;
  }
  $("otaStatus").textContent=msg;
  $("upgradeFirmware").disabled=busy||!(avail||remote)||state==="downloading"||state==="flashing";
  if(state==="downloading"||state==="flashing"){
    if(!otaPollTimer)otaPollTimer=setInterval(pollOtaStatus,1000);
  }else if(otaPollTimer){
    clearInterval(otaPollTimer);
    otaPollTimer=null;
  }
}

async function pollOtaStatus(){
  try{
    const data=await apiJson("/api/firmware");
    updateFirmwareUi(data);
  }catch(e){}
}

async function saveOtaUrl(){
  setBusy(true);
  try{
    await apiJson("/api/firmware/url",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({otaUrl:$("otaUrl").value})});
    showToast("OTA URL saved");
  }catch(e){showToast(e.message,true);}
  setBusy(false);
}

async function checkFirmwareUpdate(){
  setBusy(true);
  try{
    const data=await apiJson("/api/firmware/check",{method:"POST"});
    updateFirmwareUi(data);
    showToast(data.updateAvailable?"Update available":"Firmware up to date");
  }catch(e){
    try{
      const data=await apiJson("/api/firmware");
      updateFirmwareUi(data);
    }catch(_e){}
    showToast(e.message,true);
  }
  setBusy(false);
}

async function upgradeFirmware(){
  if(!confirm("Download and install firmware update? The sign will reboot."))return;
  setBusy(true);
  try{
    await apiJson("/api/firmware/upgrade",{method:"POST",headers:{"Content-Type":"application/json"},body:"{}"});
    showToast("Upgrading…");
    pollOtaStatus();
  }catch(e){showToast(e.message,true);setBusy(false);}
}

async function uploadFirmwareFile(file){
  if(!file)return;
  if(!confirm(`Install ${file.name}? The sign will reboot.`))return;
  setBusy(true);
  try{
    const fd=new FormData();
    fd.append("file",file,file.name);
    const r=await fetch("/api/firmware/upload",{method:"POST",body:fd,credentials:"include"});
    let data={};
    try{data=await r.json();}catch(e){}
    if(!r.ok)throw new Error(data.error||(`Upload failed (${r.status})`));
    showToast("Flashing… device will reboot");
    pollOtaStatus();
  }catch(e){showToast(e.message,true);setBusy(false);}
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
    const rgb=hexToRgb(selectedColor);
    if(eff&&eff.monochrome){
      if(selectedEffect==="bright_white"){
        ctx.fillStyle=`rgb(${rgb.r},${rgb.g},${rgb.b})`;
        ctx.fillRect(0,0,PANEL_W,PANEL_H);
        $("previewCap").textContent=`Solid fill · ${selectedColor}`;
        return;
      }
      if(selectedEffect==="arrow_left"||selectedEffect==="arrow_right"){
        ctx.fillStyle=`rgb(${rgb.r},${rgb.g},${rgb.b})`;
        const right=selectedEffect==="arrow_right";
        const w=26,h=52,bw=10;
        const x=right?60:18;
        for(let i=0;i<h;i++){
          const t=Math.abs(i-(h-1)/2)/((h-1)/2);
          const off=Math.round(t*(w-bw));
          const barX=right?x+(w-bw-off):x+off;
          ctx.fillRect(barX,i,bw,1);
        }
        $("previewCap").textContent=`Arrows · ${selectedColor}`;
        return;
      }
      if(selectedEffect==="flashing_halves"){
        ctx.fillStyle=`rgb(${rgb.r},${rgb.g},${rgb.b})`;
        ctx.fillRect(0,0,PANEL_W/2,PANEL_H);
        $("previewCap").textContent=`Flashing halves · ${selectedColor}`;
        return;
      }
      if(selectedEffect==="full_strobe"){
        ctx.fillStyle=`rgb(${rgb.r},${rgb.g},${rgb.b})`;
        ctx.fillRect(0,0,PANEL_W,PANEL_H);
        $("previewCap").textContent=`Full strobe · ${selectedColor}`;
        return;
      }
      if(selectedEffect==="pulse"){
        ctx.fillStyle=`rgb(${rgb.r},${rgb.g},${rgb.b})`;
        ctx.globalAlpha=0.5;
        ctx.fillRect(0,0,PANEL_W,PANEL_H);
        ctx.globalAlpha=1;
        $("previewCap").textContent=`Pulse · ${selectedColor}`;
        return;
      }
      if(selectedEffect==="border_chase"){
        ctx.strokeStyle=`rgb(${rgb.r},${rgb.g},${rgb.b})`;
        ctx.lineWidth=2;
        ctx.strokeRect(1,1,PANEL_W-2,PANEL_H-2);
        $("previewCap").textContent=`Border chase · ${selectedColor}`;
        return;
      }
      if(selectedEffect==="progress_bar"){
        const pct=effectParam/100;
        ctx.fillStyle=`rgb(${rgb.r},${rgb.g},${rgb.b})`;
        ctx.fillRect(0,0,Math.round(PANEL_W*pct),PANEL_H);
        $("previewCap").textContent=`Progress ${effectParam}% · ${selectedColor}`;
        return;
      }
      if(selectedEffect==="game_of_life"){
        ctx.fillStyle=`rgb(${rgb.r},${rgb.g},${rgb.b})`;
        for(let y=0;y<PANEL_H;y+=4)for(let x=0;x<PANEL_W;x+=4)if((x+y)%8===0)ctx.fillRect(x,y,2,2);
        $("previewCap").textContent=`Game of Life · ${selectedColor}`;
        return;
      }
    }
    ctx.fillStyle="#333";
    ctx.fillRect(0,0,PANEL_W,PANEL_H);
    ctx.fillStyle="#ff0";
    ctx.font="8px monospace";
    ctx.fillText(eff?eff.label:"Effect",4,28);
    $("previewCap").textContent="Effect preview (fixed colours)";
    return;
  }
  if(contentType==="clock"){
    const showSec=$("clockSeconds").checked;
    const showDate=$("clockDate").checked;
    ctx.fillStyle="#fff";
    ctx.font="14px monospace";
    ctx.fillText(showSec?"12:34:56":"12:34",showSec?12:28,showDate?18:32);
    if(showDate){
      ctx.font="10px monospace";
      ctx.fillText("05.06.2026",16,38);
    }
    $("previewCap").textContent="Clock preview (needs NTP)";
    return;
  }
  if(contentType==="countdown"){
    let label="Set target";
    if(countdownMode==="duration"){
      const sec=countdownDurationSec();
      if(sec>0){
        const h=Math.floor(sec/3600);
        const m=Math.floor((sec%3600)/60);
        const s=sec%60;
        label=h>0?`${String(h).padStart(2,"0")}:${String(m).padStart(2,"0")}:${String(s).padStart(2,"0")}`:`${String(m).padStart(2,"0")}:${String(s).padStart(2,"0")}`;
      }
    }else{
      const end=countdownEndUnix();
      if(end>0){
        const sec=Math.max(0,end-Math.floor(Date.now()/1000));
        const h=Math.floor(sec/3600);
        const m=Math.floor((sec%3600)/60);
        const s=sec%60;
        label=h>0?`${String(h).padStart(2,"0")}:${String(m).padStart(2,"0")}:${String(s).padStart(2,"0")}`:`${String(m).padStart(2,"0")}:${String(s).padStart(2,"0")}`;
      }
    }
    ctx.fillStyle="#fff";
    ctx.font="12px monospace";
    ctx.fillText(label,8,32);
    $("previewCap").textContent=countdownMode==="duration"?"Duration countdown preview":"Target countdown preview";
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
  const blockTop=Math.floor((PANEL_H-blockH)/2)+(parseInt($("offsetY").value,10)||0);
  const contentOffsetX=parseInt($("offsetX").value,10)||0;
  const rgb=hexToRgb(selectedColor);
  const lines=messageTextValue().split("\n").slice(0,rowCount);
  while(lines.length<rowCount)lines.push("");
  ctx.fillStyle="rgba(255,255,255,0.06)";
  ctx.fillRect(0,blockTop,PANEL_W,blockH);
  const glyphH=m.size*8;
  const caronH=m.size*2;
  ctx.fillStyle=`rgb(${rgb.r},${rgb.g},${rgb.b})`;
  const scrolling=$("scroll").checked;
  let totalH=0;
  for(let i=0;i<rowCount;i++){
    if(i>0&&lineHasCaron(lines[i]))totalH+=caronH;
    totalH+=lineHasCaron(lines[i])?caronH+glyphH:glyphH;
  }
  let rowY=blockTop+Math.floor((blockH-totalH)/2);
  for(let i=0;i<rowCount;i++){
    const line=lines[i]||"";
    const textW=utf8CodepointCount(line)*6*m.size;
    const x=(scrolling?previewScroll:Math.floor((PANEL_W-textW)/2))+contentOffsetX;
    if(i>0&&lineHasCaron(line))rowY+=caronH;
    const hasCaron=lineHasCaron(line);
    const y=hasCaron?rowY+caronH:rowY;
    let cx=x;
    for(const ch of line){
      if(cx>=PANEL_W)break;
      if(isExtendedLatin(ch)){
        ctx.fillRect(cx+m.size*0,y-caronH+m.size*0,m.size,m.size);
        ctx.fillRect(cx+m.size*2,y-caronH+m.size*0,m.size,m.size);
        ctx.fillRect(cx+m.size*1,y-caronH+m.size,m.size,m.size);
      }
      ctx.fillRect(cx,y,glyphH*0.75,glyphH);
      cx+=6*m.size;
    }
    rowY+=hasCaron?caronH+glyphH:glyphH;
  }
  $("previewCap").textContent=`${glyphHeightPx}px font · ${rowCount} row(s) · size ${m.size}`;
}

function startPreviewAnim(){
  if(previewTimer)clearInterval(previewTimer);
  previewTimer=setInterval(()=>{
    if(contentType!=="text"||!$("scroll").checked)return;
    previewScroll--;
    const m=textMetrics(glyphHeightPx,rowCount);
    const lines=messageTextValue().split("\n").slice(0,rowCount);
    let maxW=0;
    lines.forEach(l=>{const w=utf8CodepointCount(l)*6*m.size;if(w>maxW)maxW=w;});
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
  if(typeof data.playlistEnabled==="boolean")playlistEnabled=data.playlistEnabled;
  if(typeof data.playlistMask==="number")playlistMask=data.playlistMask&0xFF;
  if(typeof data.playlistDwellMs==="number")playlistDwellMs=data.playlistDwellMs;
  renderPlaylist();
  if(data.timezoneId)$("timezone").value=data.timezoneId;
  if(!keepSelection)selectedSlot=activeSlot;
  else selectedSlot=Math.min(Math.max(prevSlot,0),presets.length-1);
  if(!keepForm)fillForm(presets[selectedSlot]||{});
  else{renderSlots();updateSlotInfo();}
  updateBanner(data);
  updateConnBar(data);
  updateTimeStatus(data);
  updateFirmwareUi(data);
  drawPreview();
  presetsReady=true;
}

async function saveBrightness(val){
  try{
    await apiJson("/api/brightness",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({brightness:val})});
    globalBrightness=val;
    updateConnBar(connData);
    showToast(`Brightness ${val}%`);
  }catch(e){showToast(e.message,true);}
}

async function saveTimezone(){
  try{
    await apiJson("/api/timezone",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({timezoneId:$("timezone").value})});
    showToast("Time zone saved");
    await loadPresets({keepSelection:true,keepForm:true});
  }catch(e){showToast(e.message,true);}
}

async function syncBrowserTime(){
  setBusy(true);
  try{
    const unix=Math.floor(Date.now()/1000);
    await apiJson("/api/time/sync",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({unix})});
    showToast("Time synced from browser");
    await loadPresets({keepSelection:true,keepForm:true});
  }catch(e){showToast(e.message,true);}
  setBusy(false);
}

async function saveSlot(){
  setBusy(true);
  try{
    const body={...presetFormBody(),id:selectedSlot};
    await apiJson("/api/presets",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify(body)});
    showToast(`Saved slot ${selectedSlot+1}`);
    await loadPresets({keepSelection:true,keepForm:true});
    renderSlots();
  }catch(e){showToast(e.message,true);}
  setBusy(false);
}

async function savePlaylist(){
  setBusy(true);
  try{
    const dwellSec=parseInt($("playlistDwell").value,10)||8;
    await apiJson("/api/playlist",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({enabled:$("playlistEnabled").checked,slotMask:playlistMask,dwellMs:Math.max(2000,dwellSec*1000)})});
    playlistEnabled=$("playlistEnabled").checked;
    playlistDwellMs=Math.max(2000,dwellSec*1000);
    showToast("Playlist saved");
  }catch(e){showToast(e.message,true);}
  setBusy(false);
}

async function duplicateSlot(){
  const raw=prompt(`Duplicate slot ${selectedSlot+1} to which slot? (1-8)`);
  if(raw==null)return;
  const to=parseInt(raw,10)-1;
  if(to<0||to>7||to===selectedSlot){showToast("Invalid slot.",true);return;}
  setBusy(true);
  try{
    await apiJson("/api/presets/duplicate",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({from:selectedSlot,to})});
    showToast(`Duplicated slot ${selectedSlot+1} → ${to+1}`);
    selectedSlot=to;
    await loadPresets({keepSelection:true,keepForm:false});
  }catch(e){showToast(e.message,true);}
  setBusy(false);
}

async function downloadBackup(){
  setBusy(true);
  try{
    const r=await fetch("/api/backup",{credentials:"include"});
    if(!r.ok)throw new Error(`Request failed (${r.status})`);
    const blob=await r.blob();
    const a=document.createElement("a");
    a.href=URL.createObjectURL(blob);
    a.download="matrixsign-backup.json";
    a.click();
    URL.revokeObjectURL(a.href);
    showToast("Backup downloaded");
  }catch(e){showToast(e.message,true);}
  setBusy(false);
}

async function restoreBackup(file){
  if(!file)return;
  setBusy(true);
  try{
    const text=await file.text();
    if(!confirm("Restore backup? This overwrites all slots and playlist settings."))return;
    await apiJson("/api/restore",{method:"POST",headers:{"Content-Type":"application/json"},body:text});
    showToast("Backup restored");
    await loadPresets({keepSelection:false,keepForm:false});
  }catch(e){showToast(e.message,true);}
  finally{setBusy(false);$("restoreFile").value="";}
}

async function tryOnPanel(){
  setBusy(true);
  try{
    await apiJson("/api/preview",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({...presetFormBody(),slot:selectedSlot})});
    showToast("Showing on panel (not saved)");
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

function updateLenWarn(){$("lenWarn").hidden=(messageTextValue().length<20);}

document.querySelectorAll("#contentTypeSeg button").forEach(b=>b.addEventListener("click",()=>{
  contentType=b.dataset.type;showSections();renderSlots();
}));
$("fontHeight").addEventListener("input",e=>{
  setGlyphHeight(e.target.value);renderSlots();
});
$("useSuggest").addEventListener("click",()=>{
  setGlyphHeight(optimalGlyphHeight(rowCount,messageTextValue(),$("scroll").checked));
  renderSlots();
});
document.querySelectorAll("#rowSeg button").forEach(b=>b.addEventListener("click",()=>{
  rowCount=parseInt(b.dataset.rows,10);showSections();renderSlots();
}));
document.querySelectorAll("#scrollSpeedSeg button").forEach(b=>b.addEventListener("click",()=>{
  $("delay").value=b.dataset.ms;syncScrollSeg();renderSlots();
}));
$("delay").addEventListener("input",()=>{syncScrollSeg();renderSlots();});
document.querySelectorAll(".color-btn").forEach(b=>b.addEventListener("click",()=>{
  selectedColor=b.dataset.color;$("color").value=selectedColor;
  document.querySelectorAll(".color-btn").forEach(x=>x.classList.remove("active"));b.classList.add("active");
  drawPreview();renderSlots();
}));
$("color").addEventListener("input",e=>{selectedColor=e.target.value.toUpperCase();document.querySelectorAll(".color-btn").forEach(x=>x.classList.remove("active"));drawPreview();renderSlots();});
$("slotLabel").addEventListener("input",()=>{renderSlots();updateSlotInfo();});
$("effectParam").addEventListener("input",e=>{effectParam=parseInt(e.target.value,10)||0;syncEffectParamUi();drawPreview();renderSlots();});
$("countdownAt").addEventListener("input",()=>{drawPreview();renderSlots();});
["countdownHours","countdownMinutes","countdownSeconds"].forEach(id=>{
  $(id).addEventListener("input",()=>{drawPreview();renderSlots();});
});
["offsetX","offsetY"].forEach(id=>{
  $(id).addEventListener("input",()=>{drawPreview();renderSlots();});
});
["clockSeconds","clockDate"].forEach(id=>{
  $(id).addEventListener("change",()=>{drawPreview();renderSlots();});
});
$("timezone").addEventListener("change",saveTimezone);
$("syncBrowserTime").addEventListener("click",syncBrowserTime);
document.querySelectorAll("#countdownModeSeg button").forEach(b=>b.addEventListener("click",()=>{
  countdownMode=b.dataset.mode;
  updateCountdownModeUi();
  drawPreview();
  renderSlots();
}));
$("bright").addEventListener("input",e=>{
  $("brightVal").textContent=e.target.value;
  clearTimeout(brightTimer);
  brightTimer=setTimeout(()=>saveBrightness(parseInt(e.target.value,10)),300);
});
const messageTextEl=$("messageText");
if(messageTextEl)messageTextEl.addEventListener("input",()=>{updateLenWarn();updateFontSizeUi();drawPreview();renderSlots();});
$("scroll").addEventListener("change",()=>{updateFontSizeUi();drawPreview();renderSlots();});

if($("tryOnPanel"))$("tryOnPanel").addEventListener("click",tryOnPanel);
$("saveSlot").addEventListener("click",saveSlot);
$("showSlot").addEventListener("click",()=>activateSlot(selectedSlot));
$("duplicateSlot").addEventListener("click",duplicateSlot);
$("savePlaylist").addEventListener("click",savePlaylist);
$("downloadBackup").addEventListener("click",downloadBackup);
$("restoreBackup").addEventListener("click",()=>$("restoreFile").click());
$("restoreFile").addEventListener("change",e=>restoreBackup(e.target.files[0]));

$("uploadGif").addEventListener("click",async()=>{
  const f=$("gifFile").files[0];
  if(!f){showToast("Choose a GIF file first.",true);return;}
  if(f.size>262144){showToast("GIF too large (max 256 KB).",true);return;}
  setBusy(true);
  try{
    const fd=new FormData();
    fd.append("file",f,f.name);
    const r=await fetch(`/api/presets/gif?id=${selectedSlot}`,{method:"POST",body:fd,credentials:"include"});
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

async function changeAdminPassword(){
  const password=$("newAdminPass").value;
  if(!password||password.length<8){showToast("Password must be at least 8 characters",true);return;}
  setBusy(true);
  try{
    await apiJson("/api/auth/password",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({password})});
    $("newAdminPass").value="";
    showToast("Web UI password changed");
  }catch(e){showToast(e.message,true);}
  setBusy(false);
}

$("changeAdminPass").addEventListener("click",changeAdminPassword);
$("saveOtaUrl").addEventListener("click",saveOtaUrl);
$("checkFirmware").addEventListener("click",checkFirmwareUpdate);
$("upgradeFirmware").addEventListener("click",upgradeFirmware);
$("pickFirmware").addEventListener("click",()=>$("firmwareFile").click());
$("firmwareFile").addEventListener("change",e=>{
  selectedFirmwareFile=e.target.files&&e.target.files[0]?e.target.files[0]:null;
  $("uploadFirmware").disabled=!selectedFirmwareFile||busy;
});
$("uploadFirmware").addEventListener("click",()=>uploadFirmwareFile(selectedFirmwareFile));
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
