import { createLivePanel } from "./components/livePanel";

export function createApp() {
  const app = document.querySelector<HTMLDivElement>("#app")!;
  app.innerHTML = `
    <div class="container">
      <div class="header">
        <div>
          <div class="title">Dashboard</div>
        </div>
      </div>

      <div class="grid" id="grid"></div>
    </div>
  `;

  const grid = app.querySelector<HTMLDivElement>("#grid")!;

  // ここに順にパネルを追加していく
  const live = createLivePanel();
  grid.appendChild(live.root);

  // TODO: chart / upload も同様に createChartPanel(), createUploadPanel() を追加

  return {
    destroy() {
      live.destroy();
    }
  };
}
