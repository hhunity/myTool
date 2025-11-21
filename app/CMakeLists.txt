
# ---- main 実行ファイル ----
#実行ファイルの登録
add_executable(app main.cpp)

find_package(Taskflow REQUIRED)      # vcpkg 版
find_package(TBB REQUIRED)           # 必要なら
find_package(nlohmann_json REQUIRED)
find_package(quill REQUIRED)

# ---------------------------
# Precompiled header　自動的に挿入される
# ---------------------------
target_precompile_headers(app PRIVATE pch.hpp)

# ---------------------------
# Other dependencies
# ---------------------------
target_link_libraries(app
    PRIVATE
        Taskflow::Taskflow
        nlohmann_json::nlohmann_json
        quill::quill
        ftxui
        lineStore
)

# Windows のリソースファイルを追加
if (WIN32)
    target_sources(app PRIVATE version.rc)
endif()

