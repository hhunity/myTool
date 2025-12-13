
エンコーダー同期をしていても画像が伸縮してしまうのですね。それは非常に厄介ですが、実はラインスキャン現場では「あるある」の現象です。
エンコーダー（ハードウェア同期）を入れているのにズレる場合、原因は**「電気的な同期」ではなく「物理的・光学的な要因」**に潜んでいることがほとんどです。
考えられる主要な原因3つと、それぞれの対策（良い方法）を挙げます。特に1番目の「ワークの高さ変動」が盲点になりやすいです。
原因1：ワークの高さ（カメラとの距離）がバタついている
通常のレンズを使っている場合、ワークがカメラに近づくと大きく写り（伸びる）、遠ざかると小さく写り（縮む）ます。
たとえ搬送速度と撮像タイミングが完璧に同期していても、ワークがコンベア上で数ミリ浮き沈みするだけで、画像上の倍率は変わってしまいます。
• 対策：テレセントリックレンズを使う
• これが「最強の物理的解決策」です。テレセントリックレンズは、対象物の距離（高さ）が変わっても倍率が変わらない特殊なレンズです。これを導入すると、ワークがバタついても画像のサイズは一定に保たれます。
• デメリット: レンズが大きく、高価です。
原因2：スリップ（滑り）が発生している
エンコーダーが「コンベアの軸」や「モーター」に付いている場合、**「コンベアベルトとワークの間」や「モーター軸とベルトの間」**で微細な滑りが起きている可能性があります。
• 対策A：測長ホイール（エンコーダー）をワークに直接当てる
• コンベア駆動軸ではなく、バネ付きのアームでエンコーダーのホイールを「ベルト表面」や「ワークそのもの」に押し当てて回転させ、実速度を測ります。
• 対策B：レーザードップラー速度計を使う
• 非接触でワークの移動速度を正確に測り、それをパルス変換してカメラに入れます。スリップの影響を完全に排除できますが高価です。
原因3：エンコーダーの振動（チャタリング）や分解能不足
エンコーダーの軸に微細な振動があると、余計なパルスが出たり、逆に高速すぎてパルスが欠落（ドロップ）したりして、画像の縦横比がランダムに暴れます。
• 対策：分周・逓倍（ていばい）設定の見直し
• カメラやグラバーボード側で、エンコーダーパルスのフィルタリング設定を見直してください。
ソフトウェア（画像処理）での解決策
ハードウェアの変更が難しい場合、アルゴリズム側でカバーする方法もあります。現在は通常のPOC（位相限定相関法）を使っているとのことですが、以下の手法への切り替えを検討してください。
1. 対数極座標変換 (Log-Polar) を併用する
通常のPOCは「ズレ（平行移動）」には強いですが、「拡大縮小」には弱いです。そこで、画像を対数極座標（Log-Polar）変換してからPOCにかける手法があります。
• 理屈: 画像の「拡大縮小」が、座標変換によって「平行移動」に変わります。これをPOCで検出することで、倍率ズレ（伸縮）そのものを検出・補正できます。
• 検索ワード: Log-Polar POC や RIPOC (Rotation Invariant POC)
2. 2点マーカーによる正規化
もしワーク上に「印刷マーク」や「穴」などの目印が2つ以上あるなら、
1. まずその2点の座標を見つける（シンプルなパターンマッチングでOK）。
2. 2点間の距離を測る。
3. その距離が「基準値」になるように画像をリサイズ（正規化）する。
4. その後に、本来やりたい微細な欠陥検査やズレ検出を行う。
推奨ステップ
まずは**「原因1（高さ変動）」**を疑ってみてください。
もし通常のCCTVレンズを使っていて、ワークが搬送中に少しでも上下しているなら、どんなに高価なエンコーダーをつけても画像は伸び縮みし続けます。この場合はレンズを変える（テレセントリックにする）か、被写界深度を深くして（絞りを絞って）影響を減らすのが第一歩です。


ZeroMQ
gRPC


# コーディング指針
## クラスにする指針
### 状態を保つ
	•	cv::VideoCapture（カメラの状態）
	•	cv::KalmanFilter（内部状態）
	•	cv::DNN::Net（重み情報）
	•	cv::TrackerCSRT（追跡状態）
	•	cv::StereoBM（パラメータ保持）
### リソースを保持する必要がある
	•	cv::Mat（画像メモリ）
	•	cv::UMat（OpenCL/GPU メモリ）
	•	cv::FileStorage（YAML / XML ハンドル）
	•	cv::VideoWriter（FFmpeg ハンドル）
### 初期化、処理、終了といった段階を踏む必要がある
	•	cv::ORB
	•	cv::SIFT
	•	cv::KalmanFilter
	•	cv::dnn::Net
## クラスにしないルール
### 純粋な画像処理
### パラメータが少ない
### リソースを持たない
## クラスにした場合の指針
### コントラクターは軽く
失敗するような処理は入れない
存在しないと失敗するものをコントラクターに入れる

## 引数のルール
### 1) 引数4個まで → 直接引数
### 2) 5〜7個 → 構造体
### 3) それ以上 → クラス
## 命名規則
### クラス名は大文字
### メソッドは小文字
### メンバにプレフィクスつけない
### 関数名も小文字
### 定数は大文字アンダースコア

#システム構成
#画像処理PC
#＃httpクライアント
libcurl

##httpサーバ
CivetWeb

##ファイル監視ソフト
Fluent Bit

#ログサーバ
##DB
Loki

##ダッシュボード
Grafi

##バックエンド
python
Flask:Webサーバ
watchdog:定期的にファイルを監視

#OpenCV with cuda
- OpenCV のポートをコピーして編集する。
mkdir myports
xcopy /E /I ports\opencv4 myports\opencv4

vcpkg/
 ├─ myports/
 │   └─ opencv4/
 │       ├─ portfile.cmake
 │       └─ CONTROL / vcpkg.json

 - 以下の -DWITH_CUDA=ON を追加する。
-DWITH_CUDA=ON
-DCUDA_ARCH_BIN=8.6         # ← 自分のGPUの compute capability
-DCUDA_FAST_MATH=ON
-DWITH_CUBLAS=ON
-DWITH_NVCUVID=ON
-ex
vcpkg_configure_cmake(
  SOURCE_PATH ${SOURCE_PATH}
  PREFER_NINJA
  OPTIONS
    -DWITH_CUDA=ON
    -DCUDA_ARCH_BIN=8.6
    -DCUDA_FAST_MATH=ON
    -DWITH_CUBLAS=ON
)

- overlay-ports を指定して install
vcpkg install opencv4:x64-windows --overlay-ports=myports
これで CUDA-enabled OpenCV が そのマシン専用にビルドされる。
初回は 40～60分かかる。

- Cmakeで指定
{
  "dependencies": [
    { "name": "opencv4" }
  ]
}

- vcpkg の「ポート(port)」とは何？
vcpkg がライブラリをビルド・インストールするための “レシピ” のこと。
具体的には、
vcpkg/ports/opencv4/
 ├─ portfile.cmake   ← どうビルドするかのレシピ
 └─ vcpkg.json       ← 依存関係など

- ただし、CPU版と共存できない？
CPU版は公式、CUDA版は手動 CMake ビルド
vcpkg 使わず、自分でビルドして CMake find_package(OpenCV) で指定する。
これは副作用ゼロ。

CPU
# vcpkg 経由の CPU OpenCV
target_include_directories(cpu_module PRIVATE ${VCPKG_INSTALLED_DIR}/x64-windows/include)
target_link_directories(cpu_module PRIVATE ${VCPKG_INSTALLED_DIR}/x64-windows/lib)
target_link_libraries(cpu_module PRIVATE opencv_world4100)

CUDA
# CUDA ビルド OpenCV
set(OPENCV_CUDA_DIR "C:/opencv_cuda/build/install")

target_include_directories(cuda_module PRIVATE 
    ${OPENCV_CUDA_DIR}/include
)

target_link_directories(cuda_module PRIVATE 
    ${OPENCV_CUDA_DIR}/x64/vc17/lib
)

target_link_libraries(cuda_module PRIVATE
    opencv_core470
    opencv_cudawarping470
    opencv_cudaarithm470
    opencv_cudafilters470
)