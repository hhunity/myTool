(function(){const r=document.createElement("link").relList;if(r&&r.supports&&r.supports("modulepreload"))return;for(const e of document.querySelectorAll('link[rel="modulepreload"]'))s(e);new MutationObserver(e=>{for(const t of e)if(t.type==="childList")for(const a of t.addedNodes)a.tagName==="LINK"&&a.rel==="modulepreload"&&s(a)}).observe(document,{childList:!0,subtree:!0});function o(e){const t={};return e.integrity&&(t.integrity=e.integrity),e.referrerPolicy&&(t.referrerPolicy=e.referrerPolicy),e.crossOrigin==="use-credentials"?t.credentials="include":e.crossOrigin==="anonymous"?t.credentials="omit":t.credentials="same-origin",t}function s(e){if(e.ep)return;e.ep=!0;const t=o(e);fetch(e.href,t)}})();const l=document.querySelector("#app");l.innerHTML=`
  <div class="container">
    <div class="header">
      <div>
        <div class="title">Job progress</div>
        <div class="subtitle">Polling /api/job/status</div>
      </div>
      <button class="btn primary" id="start">Start</button>
    </div>

    <div class="card">
      <h2>Progress</h2>
      <div class="pill"><span id="label">idle</span></div>
      <div style="height:10px"></div>

      <div style="border:1px solid var(--border); border-radius:999px; overflow:hidden; background:rgba(255,255,255,0.05); height:14px;">
        <div id="bar" style="height:14px; width:0%; background:rgba(124,92,255,0.8);"></div>
      </div>

      <div style="height:10px"></div>
      <div class="mono" id="pct">0%</div>
      <body style="margin:20px;font-family:system-ui">
  <h1>MJPEG Test</h1>
  <img src="/stream.mjpg" style="max-width:100%;border:1px solid #ccc;border-radius:12px" />
</body>
    </div>
  </div>
`;const n=document.getElementById("start"),u=document.getElementById("bar"),p=document.getElementById("pct"),d=document.getElementById("label");function c(i){const r=Math.max(0,Math.min(100,i));u.style.width=`${r}%`,p.textContent=`${r}%`}n.addEventListener("click",async()=>{n.disabled=!0,d.textContent="starting...",c(0);const i=await fetch("/api/job/start",{method:"POST"}),{id:r}=await i.json();d.textContent=`running (id=${r})`;const o=window.setInterval(async()=>{try{const e=await(await fetch(`/api/job/status?id=${encodeURIComponent(r)}`)).json();c(e.progress),e.done&&(d.textContent=e.message??"done",window.clearInterval(o),n.disabled=!1)}catch(s){d.textContent=`error: ${String(s)}`,window.clearInterval(o),n.disabled=!1}},200)});
