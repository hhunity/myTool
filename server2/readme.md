
# Nodejs
 - Vitaを使うためにインストールする。これ入れるとviteも入る
 - Volta は Node.js / npm / yarn / pnpm といった JavaScript ツールチェーンの “バージョン管理ツール”
 - 
## インストール方法
 1. Volta をインストール（公式の Windows インストーラ）
 1. volta -v
 1. その後 PowerShell でNode.js（LTS）をインストール
          volta install node@lts
          node -v
          npm -v
 1. ypeScript用の web/ プロジェクトで pin する（推奨）
	1. Vite プロジェクト作成（package.json ができます）
          cd /Users/hiroyukih/vscode/myTool
          npm create vite@latest web -- --template vanilla-ts
	1. web/ に移動して pin
	1. web/ に移動して pin
          以後、この web/ では固定バージョンの Node が使われます。

# Vite
 - 開発サーバ（dev server）**を提供. npm run devで起動する
 - typescriptを編集するとリアルタイムで反映させることが可能
 - 開発時の関係
     - cpp-httplib（C++）：API サーバ
       - 例：http://127.0.0.1:8080/api/hello を返す 
	- Vite dev server（Node上）：フロント（TypeScript）の開発サーバ
      	- 例：http://localhost:5173/ を配る（HMR込み）
  	- 必要なら proxy で API も転送できる
	- ブラウザはまず http://localhost:5173（Vite）へアクセス
	- TypeScript の fetch("/api/hello") は Vite の proxy によってhttp://127.0.0.1:8080/api/hello（cpp-httplib）へ転送される
     Browser
          ├─ GET / (HTML/JS/CSS) ─────────────→ Vite dev server (5173)
          │        (TS→JS変換/HMR)
          └─ GET /api/hello ─(proxy)──────────→ cpp-httplib (8080)
 - 開発時と運用時の違い
   - 開発時
	•	Node.js + npm + Vite dev server が必要
	•	TypeScript編集 → 即反映（HMR）を実現

   - 運用時（一般的）
	•	npm run build で生成された **静的ファイル（dist）**だけを使う
	•	その静的ファイルを cpp-httplib が配信するなら、Vite dev server は不要
	•	Node.js も「運用環境でビルドしない」なら不要

 - Node.js と Vite の関係
	- Node.js：JavaScript を PC 上で実行するための実行環境（ランタイム）
	- Vite：Node.js 上で動くフロントエンド開発ツール（開発サーバ＋ビルド）
 - 関係を一言で言うと：
	- Vite は Node.js が無いと動きません。
	- Node.js は “Viteを動かす土台” です。 
 - npm
   - Node.js の標準的な パッケージ管理ツール
 - 
- tsの役割
  - web/src/main.ts
    - ブラウザで動く TypeScript（フロントエンド）
    - Vite dev server（npm run dev）が監視していて、保存すると **即反映（HMR）**されます
    - 画面表示、ボタン操作、fetch("/api/...") などはここ

## typescriptビルド方法
1. ビルド 
     cd /Users/hiroyukih/vscode/myTool/web
     npm install
     npm run build
2. 成果物をserver.exe直下に置く
3. serverを動かすと成果物を見に行くので

