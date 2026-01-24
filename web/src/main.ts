const app = document.querySelector<HTMLDivElement>("#app")!;

app.innerHTML = `
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
`;

const img = document.getElementById("live") as HTMLImageElement;
const overlay = document.getElementById("overlay") as HTMLDivElement;

const status = document.getElementById("status") as HTMLSpanElement;
const dot = document.getElementById("dot") as HTMLSpanElement;
const label = document.getElementById("label") as HTMLSpanElement;
const stats = document.getElementById("stats") as HTMLSpanElement;

const btn = document.getElementById("reconnect") as HTMLButtonElement;

const STREAM_URL = "/stream.mjpg";
const HEARTBEAT_URL = "/api/live/heartbeat";

const DEAD_MS = 2000;
const POLL_MS = 500;

// FPS 推定（移動平均）
let lastSeq: number | null = null;
let lastHbAt: number | null = null;
let fpsEma = 0;                // Exponential Moving Average
const FPS_EMA_ALPHA = 0.25;    // 0〜1（大きいほど反応が速い）

let online = true;

function reconnect() {
  img.src = `${STREAM_URL}?ts=${Date.now()}`;
}

btn.onclick = () => reconnect();

function setOnline(next: boolean) {
  online = next;

  overlay.style.visibility = next ? "hidden" : "visible";
  dot.style.background = next ? "#22c55e" : "#ef4444";
  label.textContent = next ? "LIVE" : "OFFLINE";

  // OFFLINE のときだけボタンを強調（クラスはCSSに合わせて調整）
  btn.classList.toggle("primary", !next);

  // statusバッジも少し色味を変える（任意）
  status.style.borderColor = next ? "rgba(34,197,94,0.35)" : "rgba(239,68,68,0.35)";
  status.style.background  = next ? "rgba(34,197,94,0.10)" : "rgba(239,68,68,0.10)";
}

// 初期表示
setOnline(true);
reconnect();

setInterval(async () => {
  try {
    const r = await fetch(`${HEARTBEAT_URL}?ts=${Date.now()}`, { cache: "no-store" });
    if (!r.ok) throw new Error(`HTTP ${r.status}`);

    const j = (await r.json()) as { seq: number; last_epoch_ms: number };

    const now = Date.now();
    const age = (j.last_epoch_ms > 0) ? (now - j.last_epoch_ms) : Number.POSITIVE_INFINITY;
    const alive = age <= DEAD_MS;

    // FPS 推定（seq増分 / 経過秒）
    if (lastSeq !== null && lastHbAt !== null) {
      const dSeq = j.seq - lastSeq;
      const dt = (now - lastHbAt) / 1000;
      if (dt > 0 && dSeq >= 0) {
        const inst = dSeq / dt;                 // 瞬間fps
        fpsEma = fpsEma === 0 ? inst : (FPS_EMA_ALPHA * inst + (1 - FPS_EMA_ALPHA) * fpsEma);
      }
    }
    lastSeq = j.seq;
    lastHbAt = now;

    // 表示更新
    stats.textContent = `fps=${fpsEma ? fpsEma.toFixed(1) : "--"}  age=${Number.isFinite(age) ? age.toFixed(0) : "--"}ms`;

    // ONLINE/OFFLINE 切替
    const wasOnline = online;
    setOnline(alive);

    // OFFLINE→ONLINE 復帰時は張り直す（安定化）
    if (!wasOnline && alive) reconnect();
  } catch (e) {
    // heartbeat が取れない＝OFFLINE扱い
    stats.textContent = `fps=--  age=--ms`;
    setOnline(false);
  }
}, POLL_MS);

// 画像読み込み自体が失敗した場合もOFFLINE寄せ（補助）
img.onerror = () => setOnline(false);