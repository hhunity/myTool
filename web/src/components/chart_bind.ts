import { Chart } from "chart.js/auto";

type ChartApiResponse = {
  labels: string[];
  datasets: Array<{ label: string; data: number[] }>;
};

export function initChartPanel() {
  const canvas = document.getElementById("chartCanvas") as HTMLCanvasElement | null;
  if (!canvas) throw new Error("chartCanvas not found");

  const status = document.getElementById("chartStatus") as HTMLSpanElement | null;
  const btnRefresh = document.getElementById("chartRefresh") as HTMLButtonElement | null;
  const btnToggle = document.getElementById("chartToggle") as HTMLButtonElement | null;

  let paused = false;
  let timer: number | null = null;

  const ctx = canvas.getContext("2d");
  if (!ctx) throw new Error("2D context not available");

  const chart = new Chart(ctx, {
    type: "line",
    data: {
      labels: [],
      datasets: [{ label: "value", data: [] }],
    },
    options: {
      responsive: true,
      animation: false,
      parsing: false,
      plugins: { legend: { display: true } },
      scales: {
        x: { ticks: { maxRotation: 0 } },
        y: { beginAtZero: false },
      },
    },
  });

  function setStatus(s: string) {
    if (status) status.textContent = s;
  }

  function applyData(j: ChartApiResponse) {
    if (!Array.isArray(j.labels) ){}
    chart.update("none");
  }

  // ダミー（APIがまだ無い場合用）
  function makeDummy(n = 60): ChartApiResponse {
    const labels: string[] = [];
    const data: number[] = [];
    let v = 50;
    for (let i = 0; i < n; i++) {
      v += (Math.random() - 0.5) * 2;
      labels.push(String(i));
      data.push(Number(v.toFixed(2)));
    }
    return { labels, datasets: [{ label: "dummy", data }] };
  }

  async function refresh() {
    if (paused) return;

    try {
      const r = await fetch(`/api/chart?ts=${Date.now()}`, { cache: "no-store" });
      if (!r.ok) throw new Error(`HTTP ${r.status}`);
      const j = (await r.json()) as ChartApiResponse;

      // ざっくり妥当性チェック
      if (!Array.isArray(j.labels) || !Array.isArray(j.datasets)) throw new Error("invalid json");
      applyData(j);
      setStatus("source=/api/chart");
    } catch (e) {
      // API未実装でも見た目の確認ができるようにフォールバック
      applyData(makeDummy());
      setStatus(`source=dummy (${String(e)})`);
    }
  }

  btnRefresh?.addEventListener("click", () => refresh());

  btnToggle?.addEventListener("click", () => {
    paused = !paused;
    btnToggle.textContent = paused ? "Resume" : "Pause";
    if (!paused) refresh();
  });

  // 初回
  refresh();

  // 定期更新（例：1秒）
  timer = window.setInterval(() => {
    if (!paused) refresh();
  }, 1000);

  // 破棄（HMR対策など）
  return function destroy() {
    if (timer) window.clearInterval(timer);
    chart.destroy();
  };
}