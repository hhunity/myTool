(function(){const n=document.createElement("link").relList;if(n&&n.supports&&n.supports("modulepreload"))return;for(const e of document.querySelectorAll('link[rel="modulepreload"]'))o(e);new MutationObserver(e=>{for(const s of e)if(s.type==="childList")for(const i of s.addedNodes)i.tagName==="LINK"&&i.rel==="modulepreload"&&o(i)}).observe(document,{childList:!0,subtree:!0});function r(e){const s={};return e.integrity&&(s.integrity=e.integrity),e.referrerPolicy&&(s.referrerPolicy=e.referrerPolicy),e.crossOrigin==="use-credentials"?s.credentials="include":e.crossOrigin==="anonymous"?s.credentials="omit":s.credentials="same-origin",s}function o(e){if(e.ep)return;e.ep=!0;const s=r(e);fetch(e.href,s)}})();const v=document.querySelector("#app");v.innerHTML=`
  <div style="padding:16px; max-width:980px; margin:0 auto;">
    <h1 style="margin:0 0 10px;">Live Monitor</h1>

    <div style="display:flex; gap:10px; align-items:center; margin-bottom:10px;">
      <span id="status" style="
        display:inline-flex; align-items:center; gap:8px;
        border:1px solid rgba(0,0,0,0.15);
        padding:6px 10px; border-radius:999px; font-size:12px;">
        <span id="dot" style="width:8px;height:8px;border-radius:999px;background:#999;"></span>
        <span id="label">UNKNOWN</span>
      </span>
      <button id="reconnect">Reconnect</button>
        <span id="status" class="pill">
          <span id="dot" style="width:8px;height:8px;border-radius:999px;background:#999;"></span>
        </span>
      <span class="pill mono" id="stats">fps=--  age=--ms</span>
    </div>

    <div id="box" style="
      position:relative;
      border-radius:12px;
      overflow:hidden;
      border:1px solid rgba(0,0,0,0.15);
      background: rgba(0,0,0,0.04);
    ">
      <img id="live" src="/stream.mjpg" style="display:block; width:100%;" />
      <div id="overlay" style="
        position:absolute; inset:0;
        display:flex; align-items:center; justify-content:center;
        background: rgba(0,0,0,0.55);
        color: white; font-weight: 800; letter-spacing: 1px;
        font-size: 18px;
        visibility: hidden;
      ">OFFLINE</div>
    </div>
    <pre id="dbg" style="margin-top:10px; font-size:12px; opacity:0.75;"></pre>
  </div>
`;const b=document.getElementById("live"),E=document.getElementById("overlay"),m=document.getElementById("status"),w=document.getElementById("dot"),I=document.getElementById("label"),f=document.getElementById("stats"),h=document.getElementById("reconnect"),L="/stream.mjpg",O="/api/live/heartbeat",N=2e3,_=500;let c=null,d=null,a=0;const y=.25;let x=!0;function p(){b.src=`${L}?ts=${Date.now()}`}h.onclick=()=>p();function l(t){x=t,E.style.visibility=t?"hidden":"visible",w.style.background=t?"#22c55e":"#ef4444",I.textContent=t?"LIVE":"OFFLINE",h.classList.toggle("primary",!t),m.style.borderColor=t?"rgba(34,197,94,0.35)":"rgba(239,68,68,0.35)",m.style.background=t?"rgba(34,197,94,0.10)":"rgba(239,68,68,0.10)"}l(!0);p();setInterval(async()=>{try{const t=await fetch(`${O}?ts=${Date.now()}`,{cache:"no-store"});if(!t.ok)throw new Error(`HTTP ${t.status}`);const n=await t.json(),r=Date.now(),o=n.last_epoch_ms>0?r-n.last_epoch_ms:Number.POSITIVE_INFINITY,e=o<=N;if(c!==null&&d!==null){const i=n.seq-c,u=(r-d)/1e3;if(u>0&&i>=0){const g=i/u;a=a===0?g:y*g+(1-y)*a}}c=n.seq,d=r,f.textContent=`fps=${a?a.toFixed(1):"--"}  age=${Number.isFinite(o)?o.toFixed(0):"--"}ms`;const s=x;l(e),!s&&e&&p()}catch{f.textContent="fps=--  age=--ms",l(!1)}},_);b.onerror=()=>l(!1);
