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