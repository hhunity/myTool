cmake_minimum_required(VERSION 3.16)
project(LineStoreDemo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# vcpkg の toolchain は presets.json が設定するため書かなくて良い
# find_package を普通に使うだけで OK

# ---- 外部ライブラリ ----


# ---- ローカルサブディレクトリ ----
add_subdirectory(lineStore)
add_subdirectory(ftxui)
add_subdirectory(app)

