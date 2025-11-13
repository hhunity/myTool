
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

# ---- FFXUI -----
include(FetchContent)

FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
  GIT_TAG v5.0.0
)

FetchContent_GetProperties(ftxui)
if(NOT ftxui_POPULATED)
  FetchContent_Populate(ftxui)
  add_subdirectory(${ftxui_SOURCE_DIR} ${ftxui_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

# --- nlohmann/json を GitHub から取得 ---
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3   # 安定版
)
FetchContent_MakeAvailable(nlohmann_json)

#add_subdirectory(lineStore)
#add_subdirectory(app)
# ---- main 実行ファイル ----
#実行ファイルの登録
add_executable(demo ftxui/main.cpp)
#依存関係のリンク
#target_link_libraries(demo PRIVATE LineStore ${OpenCV_LIBS})
target_link_libraries(demo PRIVATE ftxui::component)
target_link_libraries(demo PRIVATE nlohmann_json::nlohmann_json)

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