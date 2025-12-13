cmake_minimum_required(VERSION 3.16)
# ----- define project and version for config.h ----
project(myApp VERSION 0.1.0 LANGUAGES CXX)
# ----- define timestamp -----
string(TIMESTAMP BUILD_TIME "%Y-%m-%d %H:%M:%S")

# 
#  VSCode に正しいコンパイル情報（include path、フラグ、定義、PCH などすべて）を教えるファイルを生成
#  .vscode setting jsonに"C_Cpp.default.compileCommands": "build/debug/compile_commands.json"を書くこと
# 
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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
# ---- link source_tree ----
include_directories(${CMAKE_SOURCE_DIR})

# ---- requar C++ version
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- ローカルサブディレクトリ ----
add_subdirectory(lineStore)
add_subdirectory(ftxui)
add_subdirectory(server)
add_subdirectory(sandbox)
add_subdirectory(opencv)
add_subdirectory(comms)
add_subdirectory(app)

# ----- install cmake --install build で Linux / Mac に自動インストール。
install(TARGETS app RUNTIME DESTINATION bin)
# ----- asetfile ex xxx.jsonやリソースファイル linux,macは実行ファイルはbinにおいて,データはassetにおく
#install(DIRECTORY assets/ DESTINATION share/myApp/assets)

# ----- プリコンパイルヘッダ（PCH）
#target_precompile_headers(app PRIVATE pch.hpp)
# ----- Unity Build（自動的に複数ソースをまとめてコンパイル）
#set_target_properties(app PROPERTIES UNITY_BUILD ON)

# ----- test
#enable_testing()
#add_executable(test_linestore test_linestore.cpp)
#add_test(NAME LineStoreTest COMMAND test_linestore)

# ==============================
# install() でパッケージ内容を定義
# ==============================

# 実行ファイル
install(TARGETS app
    RUNTIME DESTINATION bin
)

install(FILES ${CMAKE_SOURCE_DIR}/readme.md
    DESTINATION share/mytool
)

# ==============================
# CPack 設定
# ==============================
set(CPACK_PACKAGE_NAME "mytool")
set(CPACK_PACKAGE_VENDOR "hiro")
set(CPACK_PACKAGE_CONTACT "hiro@example.com")

set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})

# 出力パッケージ名: mytool-1.0.0-<OS>.zip
set(CPACK_PACKAGE_FILE_NAME
    "${CPACK_PACKAGE_NAME}-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}"
)

# 形式（macOS / Linux / Windows すべて zip で統一）
set(CPACK_GENERATOR "ZIP")

# ==============================
# macOS 用の設定 (.app + .dmg)
# ==============================
if(APPLE)
    message(STATUS "Configuring CPack for macOS (DMG)")

    # .app バンドル
    set_target_properties(app PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/mac/Info.plist"
    )

    # .dmg を作る
    set(CPACK_GENERATOR "DragNDrop")

    # .dmg のファイル名
    set(CPACK_PACKAGE_FILE_NAME
        "${CPACK_PACKAGE_NAME}-${PROJECT_VERSION}-macOS"
    )

    # .app を CPack に渡す
    install(TARGETS app BUNDLE DESTINATION .)
endif()

# ==============================
# Windows 用の設定 (.exe installer)
# ==============================
if (WIN32)
    message(STATUS "Configuring CPack for Windows (NSIS)")

    set(CPACK_GENERATOR "NSIS")

    set(CPACK_PACKAGE_FILE_NAME
        "${CPACK_PACKAGE_NAME}-${PROJECT_VERSION}-win"
    )

    # インストール先
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "mytool")

    # ショートカット作成
    set(CPACK_NSIS_MENU_LINKS
        "mytool.exe" "myTool Application"
    )

    # アンインストール設定
    set(CPACK_NSIS_DISPLAY_NAME "mytool ${PROJECT_VERSION}")
endif()

# ==============================
# Linux / その他 → ZIP のまま
# ==============================
if(UNIX AND NOT APPLE)
    message(STATUS "Configuring CPack for Linux (ZIP)")
    set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${PROJECT_VERSION}-linux")
    set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
endif()

# 一部 OS・パスの注意
include(CPack)