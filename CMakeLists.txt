cmake_minimum_required(VERSION 3.16)
# ----- define project and version for config.h ----
project(myTool VERSION 0.1.0 LANGUAGES CXX)
# ----- define timestamp -----
string(TIMESTAMP BUILD_TIME "%Y-%m-%d %H:%M:%S")

# ---- git hash ----
find_package(Git)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    # build numberのためにgitのコミット数
    execute_process(
        COMMAND git rev-list --count HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE BUILD_NUMBER
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
endif()
add_definitions(-DGIT_HASH="${GIT_HASH}")

set(APP_VERSION_BUILD ${BUILD_NUMBER})

# ---- create config file  ----
configure_file(config.h.in config.h)
# ---- link config.h ----
include_directories(${CMAKE_BINARY_DIR})
# ---- requar C++ version
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- ローカルサブディレクトリ ----
add_subdirectory(lineStore)
add_subdirectory(ftxui)
add_subdirectory(app)

# ----- install cmake --install build で Linux / Mac に自動インストール。
install(TARGETS myApp RUNTIME DESTINATION bin)
# ----- asetfile ex xxx.jsonやリソースファイル linux,macは実行ファイルはbinにおいて,データはassetにおく
#install(DIRECTORY assets/ DESTINATION share/myApp/assets)

