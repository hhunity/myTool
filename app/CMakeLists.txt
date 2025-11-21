
# ---- main 実行ファイル ----
#実行ファイルの登録
add_executable(myApp main.cpp)

find_package(Taskflow REQUIRED)      # vcpkg 版
find_package(TBB REQUIRED)           # 必要なら
find_package(nlohmann_json REQUIRED)

#依存関係のリンク
target_link_libraries(myApp
    PUBLIC
    ftxui
    lineStore
)

# Windows のリソースファイルを追加
if (WIN32)
    target_sources(myApp PRIVATE version.rc)
endif()

target_link_libraries(myApp
    PRIVATE 
        Taskflow::Taskflow
        nlohmann_json::nlohmann_json
)
