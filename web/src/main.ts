const app = document.querySelector<HTMLDivElement>("#app")!;
app.innerHTML = `
  <div style="padding:16px; max-width:980px; margin:0 auto;">
    <h1 style="margin:0 0 10px;">Dashboard</h1>

    <div style="display:flex; gap:10px; align-items:center;">
      <button id="openMonitor">Open Live Monitor</button>
      <button id="closeMonitor" disabled>Close</button>
    </div>

    <!-- ここに小画面を出す -->
    <div id="monitorHost" style="margin-top:12px;"></div>
  </div>
`;

const openBtn = document.getElementById("openMonitor") as HTMLButtonElement;
const closeBtn = document.getElementById("closeMonitor") as HTMLButtonElement;
const host = document.getElementById("monitorHost") as HTMLDivElement;

const STREAM_URL = "/stream.mjpg";
const HEARTBEAT_URL = "/api/live/heartbeat";
const DEAD_MS = 2000;
const POLL_MS = 500;

type Cleanup = () => void;

function mountLiveMonitorIntoHost(hostEl: HTMLElement): Cleanup {
  // host内に出すコンテナ
  const panel = document.createElement("div");
  panel.style.cssText = `
    width: 420px;
    border: 1px solid rgba(0,0,0,0.18);
    border-radius: 12px;
    overflow: hidden;
    background: white;
  `;

  panel.innerHTML = `
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
  `;

  // hostにマウント（これで host が “使われる”）
  hostEl.replaceChildren(panel);

  // panel内の要素参照（panel.querySelector なので id 重複問題を避けやすい）
  const img = panel.querySelector("#live") as HTMLImageElement;
  const overlay = panel.querySelector("#overlay") as HTMLDivElement;
  const status = panel.querySelector("#status") as HTMLSpanElement;
  const dot = panel.querySelector("#dot") as HTMLSpanElement;
  const label = panel.querySelector("#label") as HTMLSpanElement;
  const stats = panel.querySelector("#stats") as HTMLSpanElement;
  const reconnectBtn = panel.querySelector("#reconnect") as HTMLButtonElement;

  // 状態
  let lastSeq: number | null = null;
  let lastHbAt: number | null = null;
  let fpsEma = 0;
  const FPS_EMA_ALPHA = 0.25;

  let online = true;
  let timer: number | null = null;

  function reconnect() {
    img.src = `${STREAM_URL}?ts=${Date.now()}`;
  }

  function setOnline(next: boolean) {
    online = next;
    overlay.style.visibility = next ? "hidden" : "visible";
    dot.style.background = next ? "#22c55e" : "#ef4444";
    label.textContent = next ? "LIVE" : "OFFLINE";
    status.style.borderColor = next ? "rgba(34,197,94,0.35)" : "rgba(239,68,68,0.35)";
    status.style.background  = next ? "rgba(34,197,94,0.10)" : "rgba(239,68,68,0.10)";
  }

  reconnectBtn.onclick = () => reconnect();
  img.onerror = () => setOnline(false);

  // ポーリング開始
  timer = window.setInterval(async () => {
    try {
      const r = await fetch(`${HEARTBEAT_URL}?ts=${Date.now()}`, { cache: "no-store" });
      if (!r.ok) throw new Error(`HTTP ${r.status}`);
      const j = (await r.json()) as { seq: number; last_epoch_ms: number };

      const now = Date.now();
      const age = (j.last_epoch_ms > 0) ? (now - j.last_epoch_ms) : Number.POSITIVE_INFINITY;
      const alive = age <= DEAD_MS;

      if (lastSeq !== null && lastHbAt !== null) {
        const dSeq = j.seq - lastSeq;
        const dt = (now - lastHbAt) / 1000;
        if (dt > 0 && dSeq >= 0) {
          const inst = dSeq / dt;
          fpsEma = fpsEma === 0 ? inst : (FPS_EMA_ALPHA * inst + (1 - FPS_EMA_ALPHA) * fpsEma);
        }
      }
      lastSeq = j.seq;
      lastHbAt = now;

      stats.textContent = `fps=${fpsEma ? fpsEma.toFixed(1) : "--"}  age=${Number.isFinite(age) ? age.toFixed(0) : "--"}ms`;

      const wasOnline = online;
      setOnline(alive);
      if (!wasOnline && alive) reconnect();
    } catch {
      stats.textContent = `fps=--  age=--ms`;
      setOnline(false);
    }
  }, POLL_MS);

  // 初期
  setOnline(true);
  reconnect();

  // クリーンアップ（重要）
  const cleanup: Cleanup = () => {
    if (timer !== null) window.clearInterval(timer);
    timer = null;
    hostEl.replaceChildren(); // hostから消す
  };

  return cleanup;
}

let cleanup: Cleanup | null = null;

openBtn.onclick = () => {
  if (cleanup) return; // 二重起動防止
  cleanup = mountLiveMonitorIntoHost(host);
  openBtn.disabled = true;
  closeBtn.disabled = false;
};

closeBtn.onclick = () => {
  if (!cleanup) return;
  cleanup();
  cleanup = null;
  openBtn.disabled = false;
  closeBtn.disabled = true;
};