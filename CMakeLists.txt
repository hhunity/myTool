
# 最低限必要なCMakeのバージョン
# 古い環境で誤動作しないようにするための指定。
cmake_minimum_required(VERSION 3.16)

#プロジェクト名と使う言語
#このプロジェクトの名前を LineStoreDemo に。CXX はC++プロジェクトであることを示す。
project(LineStoreDemo CXX)

# ---- OpenCV 設定 ----
# Homebrew のパスを明示（自動で見つからない場合用）
set(OpenCV_DIR "/opt/homebrew/lib/cmake/opencv4")
# OpenCVを探して読み込む
# 成功すると OpenCV_INCLUDE_DIRS と OpenCV_LIBS が定義される。
find_package(OpenCV REQUIRED)

add_subdirectory(lineStore)
add_subdirectory(app)

message(STATUS "...")


#cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
#-S .ソースディレクトリを指定（ここではカレントフォルダ .）
#-B build生成されるビルドファイル（MakefileやNinjaファイル）を build/ フォルダに出力する
#-D CMAKE_BUILD_TYPE=Releaseビルドモードを「Release」に設定（最適化オン・デバッグ情報少なめ）
#（省略可）-G Ninja#Ninjaビルドを使う（指定しないと自動で選ばれる）

#cmake --build build -j
#オプション
#意味 --build build build/ に生成されたビルドスクリプトを使ってビルド
#-j 並列ビルド。CPUコア数に応じて自動で並列化する（例: -j8）