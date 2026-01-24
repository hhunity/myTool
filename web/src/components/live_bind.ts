import { qs } from "../lib/dom"; // 置き場所に合わせて調整してください

type Heartbeat = { seq: number; last_epoch_ms: number };

export type LiveBindOptions = {
  streamUrl?: string;
  heartbeatUrl?: string;
  pollMs?: number;
  deadMs?: number;
};

export function initLivePanel(opts: LiveBindOptions = {}) {
  const streamUrl = opts.streamUrl ?? "/stream.mjpg";
  const heartbeatUrl = opts.heartbeatUrl ?? "/api/live/heartbeat";
  const pollMs = opts.pollMs ?? 500;
  const deadMs = opts.deadMs ?? 2000;

  // index.html に置いた要素を拾う
  const root = document;

  const img = qs<HTMLImageElement>(root, "#live");
  const overlay = qs<HTMLDivElement>(root, "#overlay");
  const dot = qs<HTMLSpanElement>(root, "#dot");
  const label = qs<HTMLSpanElement>(root, "#label");
  const stats = qs<HTMLSpanElement>(root, "#stats");
  const btn = qs<HTMLButtonElement>(root, "#reconnect");

  let online = true;

  // FPS EMA
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

  img.onerror = () => setOnline(false);

  // 破棄（HMR対策など）
  return function destroy() {
    window.clearInterval(timer);
    btn.onclick = null;
    img.onerror = null;
  };
}