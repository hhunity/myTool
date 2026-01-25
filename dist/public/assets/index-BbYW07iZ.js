(function(){const t=document.createElement("link").relList;if(t&&t.supports&&t.supports("modulepreload"))return;for(const e of document.querySelectorAll('link[rel="modulepreload"]'))a(e);new MutationObserver(e=>{for(const n of e)if(n.type==="childList")for(const s of n.addedNodes)s.tagName==="LINK"&&s.rel==="modulepreload"&&a(s)}).observe(document,{childList:!0,subtree:!0});function l(e){const n={};return e.integrity&&(n.integrity=e.integrity),e.referrerPolicy&&(n.referrerPolicy=e.referrerPolicy),e.crossOrigin==="use-credentials"?n.credentials="include":e.crossOrigin==="anonymous"?n.credentials="omit":n.credentials="same-origin",n}function a(e){if(e.ep)return;e.ep=!0;const n=l(e);fetch(e.href,n)}})();const N=document.querySelector("#app");N.innerHTML=`
  <div style="padding:16px; max-width:980px; margin:0 auto;">
    <h1 style="margin:0 0 10px;">Dashboard</h1>

    <div style="display:flex; gap:10px; align-items:center;">
      <button id="openMonitor">Open Live Monitor</button>
      <button id="closeMonitor" disabled>Close</button>
    </div>

    <!-- ここに小画面を出す -->
    <div id="monitorHost" style="margin-top:12px;"></div>
  </div>
`;const x=document.getElementById("openMonitor"),v=document.getElementById("closeMonitor"),F=document.getElementById("monitorHost"),_="/stream.mjpg",T="/api/live/heartbeat",A=2e3,H=500;function P(u){const t=document.createElement("div");t.style.cssText=`
    width: 420px;
    border: 1px solid rgba(0,0,0,0.18);
    border-radius: 12px;
    overflow: hidden;
    background: white;
  `,t.innerHTML=`
    <div style="
      display:flex; align-items:center; justify-content:space-between;
      padding:10px 10px; gap:8px;
      background: rgba(0,0,0,0.04);
      border-bottom: 1px solid rgba(0,0,0,0.10);
      user-select: none;
    ">
      <div style="display:flex; align-items:center; gap:10px;">
        <span id="status" style="
          display:inline-flex; align-items:center; gap:8px;
          border:1px solid rgba(0,0,0,0.15);
          padding:4px 8px; border-radius:999px; font-size:12px;
        ">
          <span id="dot" style="width:8px;height:8px;border-radius:999px;background:#999;"></span>
          <span id="label">UNKNOWN</span>
        </span>
        <span id="stats" style="font-family: ui-monospace, SFMono-Regular, Menlo, monospace; font-size:12px;">
          fps=--  age=--ms
        </span>
      </div>
      <button id="reconnect" style="padding:4px 8px;">Reconnect</button>
    </div>

    <div style="position:relative; background: rgba(0,0,0,0.04);">
      <img id="live" style="display:block; width:100%;" />
      <div id="overlay" style="
        position:absolute; inset:0;
        display:flex; align-items:center; justify-content:center;
        background: rgba(0,0,0,0.55);
        color: white; font-weight: 800; letter-spacing: 1px;
        font-size: 18px;
        visibility: hidden;
      ">OFFLINE</div>
    </div>
  `,u.replaceChildren(t);const l=t.querySelector("#live"),a=t.querySelector("#overlay"),e=t.querySelector("#status"),n=t.querySelector("#dot"),s=t.querySelector("#label"),h=t.querySelector("#stats"),O=t.querySelector("#reconnect");let f=null,g=null,i=0;const w=.25;let L=!0,c=null;function y(){l.src=`${_}?ts=${Date.now()}`}function d(o){L=o,a.style.visibility=o?"hidden":"visible",n.style.background=o?"#22c55e":"#ef4444",s.textContent=o?"LIVE":"OFFLINE",e.style.borderColor=o?"rgba(34,197,94,0.35)":"rgba(239,68,68,0.35)",e.style.background=o?"rgba(34,197,94,0.10)":"rgba(239,68,68,0.10)"}return O.onclick=()=>y(),l.onerror=()=>d(!1),c=window.setInterval(async()=>{try{const o=await fetch(`${T}?ts=${Date.now()}`,{cache:"no-store"});if(!o.ok)throw new Error(`HTTP ${o.status}`);const p=await o.json(),b=Date.now(),m=p.last_epoch_ms>0?b-p.last_epoch_ms:Number.POSITIVE_INFINITY,S=m<=A;if(f!==null&&g!==null){const E=p.seq-f,I=(b-g)/1e3;if(I>0&&E>=0){const M=E/I;i=i===0?M:w*M+(1-w)*i}}f=p.seq,g=b,h.textContent=`fps=${i?i.toFixed(1):"--"}  age=${Number.isFinite(m)?m.toFixed(0):"--"}ms`;const q=L;d(S),!q&&S&&y()}catch{h.textContent="fps=--  age=--ms",d(!1)}},H),d(!0),y(),()=>{c!==null&&window.clearInterval(c),c=null,u.replaceChildren()}}let r=null;x.onclick=()=>{r||(r=P(F),x.disabled=!0,v.disabled=!1)};v.onclick=()=>{r&&(r(),r=null,x.disabled=!1,v.disabled=!0)};
