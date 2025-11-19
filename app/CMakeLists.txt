
# ---- main 実行ファイル ----
#実行ファイルの登録
add_executable(myApp cppFlow2.cpp)

#依存関係のリンク
target_link_libraries(myApp
    PUBLIC
    ftxui
    lineStore
)

# ==============================
# Taskflow を git から取ってくる
# ==============================
include(FetchContent)

FetchContent_Declare(taskflow
  GIT_REPOSITORY https://github.com/taskflow/taskflow.git
  # 必要なら特定バージョンに固定
  # GIT_TAG        v3.10.0
)

FetchContent_MakeAvailable(Taskflow)

target_link_libraries(myApp
    PRIVATE 
        Taskflow::Taskflow
        nlohmann_json::nlohmann_json
)
