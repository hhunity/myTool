import { qs } from "../lib/dom";

export type LivePanelOptions = {
  streamUrl?: string;          // default: /stream.mjpg
  heartbeatUrl?: string;       // default: /api/live/heartbeat
  pollMs?: number;             // default: 500
  deadMs?: number;             // default: 2000
};

type Heartbeat = { seq: number; last_epoch_ms: number };

export function createLivePanel(opts: LivePanelOptions = {}) {
  const streamUrl = opts.streamUrl ?? "/stream.mjpg";
  const heartbeatUrl = opts.heartbeatUrl ?? "/api/live/heartbeat";
  const pollMs = opts.pollMs ?? 500;
  const deadMs = opts.deadMs ?? 2000;

  const root = document.createElement("div");
  root.className = "card";
  root.innerHTML = `
    <h2>Live</h2>

    <div class="row" style="justify-content:space-between; align-items:center;">
      <div class="row" style="align-items:center;">
        <span id="status" class="pill">
          <span id="dot" style="width:8px;height:8px;border-radius:999px;background:#999;"></span>
          <span id="label">UNKNOWN</span>
        </span>
        <span id="stats" class="pill mono">fps=--  age=--ms</span>
      </div>
      <div style="height:10px"></div>
      <button id="reconnect" class="btn">Reconnect</button>
    </div>

    <div style="height:10px"></div>

    <div id="box" style="position:relative; border-radius:12px; overflow:hidden; border:1px solid var(--border);">
    <img id="live" style="display:block; width:100%;" />
    <div id="overlay" class="live-overlay">
    <div class="live-overlay-label">OFFLINE</div>
    </div>
</div>
  `;

  const img = qs<HTMLImageElement>(root, "#live");
  const overlay = qs<HTMLDivElement>(root, "#overlay");
  const dot = qs<HTMLSpanElement>(root, "#dot");
  const label = qs<HTMLSpanElement>(root, "#label");
  const stats = qs<HTMLSpanElement>(root, "#stats");
  const btn = qs<HTMLButtonElement>(root, "#reconnect");

  let online = true;

  // FPS 推定（EMA）
  let lastSeq: number | null = null;
  let lastHbAt: number | null = null;
  let fpsEma = 0;
  const alpha = 0.25;

  function reconnect() {
    img.src = `${streamUrl}?ts=${Date.now()}`;
  }

  function setOnline(next: boolean) {
    online = next;
    overlay.style.visibility = next ? "hidden" : "visible";
    dot.style.background = next ? "#22c55e" : "#ef4444";
    label.textContent = next ? "LIVE" : "OFFLINE";
    // OFFLINE のときだけ強調
    btn.classList.toggle("primary", !next);
  }

  btn.onclick = () => reconnect();

  // 初期
  setOnline(true);
  reconnect();

  const timer = window.setInterval(async () => {
    try {
      const r = await fetch(`${heartbeatUrl}?ts=${Date.now()}`, { cache: "no-store" });
      if (!r.ok) throw new Error(`HTTP ${r.status}`);
      const hb = (await r.json()) as Heartbeat;

      const now = Date.now();
      const age = hb.last_epoch_ms > 0 ? now - hb.last_epoch_ms : Number.POSITIVE_INFINITY;
      const alive = age <= deadMs;

      // FPS 推定（seq増分 / 経過秒）
      if (lastSeq !== null && lastHbAt !== null) {
        const dSeq = hb.seq - lastSeq;
        const dt = (now - lastHbAt) / 1000;
        if (dt > 0 && dSeq >= 0) {
          const inst = dSeq / dt;
          fpsEma = fpsEma === 0 ? inst : (alpha * inst + (1 - alpha) * fpsEma);
        }
      }
      lastSeq = hb.seq;
      lastHbAt = now;

      stats.textContent = `fps=${fpsEma ? fpsEma.toFixed(1) : "--"}  age=${Number.isFinite(age) ? age.toFixed(0) : "--"}ms`;

      const wasOnline = online;
      setOnline(alive);
      if (!wasOnline && alive) reconnect();
    } catch {
      stats.textContent = `fps=--  age=--ms`;
      setOnline(false);
    }
  }, pollMs);

  // 画像エラーでもOFFLINEへ（srcは触らない）
  img.onerror = () => setOnline(false);

  // 破棄関数：タイマー停止など
  function destroy() {
    window.clearInterval(timer);
    btn.onclick = null;
    img.onerror = null;
  }

  return { root, destroy };
}
