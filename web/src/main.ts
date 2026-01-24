const app = document.querySelector<HTMLDivElement>("#app")!;

app.innerHTML = `
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
    </div>
  </div>
`;

const startBtn = document.getElementById("start") as HTMLButtonElement;
const bar = document.getElementById("bar") as HTMLDivElement;
const pct = document.getElementById("pct") as HTMLDivElement;
const label = document.getElementById("label") as HTMLSpanElement;

function setProgress(p: number) {
  const clamped = Math.max(0, Math.min(100, p));
  bar.style.width = `${clamped}%`;
  pct.textContent = `${clamped}%`;
}

startBtn.addEventListener("click", async () => {
  startBtn.disabled = true;
  label.textContent = "starting...";
  setProgress(0);

  // 1) start
  const startRes = await fetch("/api/job/start", { method: "POST" });
  const { id } = await startRes.json();

  // 2) poll
  label.textContent = `running (id=${id})`;

  const timer = window.setInterval(async () => {
    try {
      const r = await fetch(`/api/job/status?id=${encodeURIComponent(id)}`);
      const s = await r.json();
      setProgress(s.progress);

      if (s.done) {
        label.textContent = s.message ?? "done";
        window.clearInterval(timer);
        startBtn.disabled = false;
      }
    } catch (e) {
      label.textContent = `error: ${String(e)}`;
      window.clearInterval(timer);
      startBtn.disabled = false;
    }
  }, 200);
});