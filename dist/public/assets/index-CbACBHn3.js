(function(){const r=document.createElement("link").relList;if(r&&r.supports&&r.supports("modulepreload"))return;for(const e of document.querySelectorAll('link[rel="modulepreload"]'))i(e);new MutationObserver(e=>{for(const t of e)if(t.type==="childList")for(const o of t.addedNodes)o.tagName==="LINK"&&o.rel==="modulepreload"&&i(o)}).observe(document,{childList:!0,subtree:!0});function u(e){const t={};return e.integrity&&(t.integrity=e.integrity),e.referrerPolicy&&(t.referrerPolicy=e.referrerPolicy),e.crossOrigin==="use-credentials"?t.credentials="include":e.crossOrigin==="anonymous"?t.credentials="omit":t.credentials="same-origin",t}function i(e){if(e.ep)return;e.ep=!0;const t=u(e);fetch(e.href,t)}})();let c=0;function s(){const n=document.querySelector("#app");n.innerHTML=`
    <h1>Vite HMR test</h1>
    <p>count: <b>${c}</b></p>
    <button id="inc">+1</button>
    <p style="opacity:0.7">edit this file and save to see live updates</p>
  `,document.getElementById("inc").addEventListener("click",()=>{c++,s()})}s();
