
# ---- main 実行ファイル ----
#実行ファイルの登録
add_executable(myApp logger.cpp)

#依存関係のリンク
target_link_libraries(myApp
    PUBLIC
    ftxui
    lineStore
)

target_link_libraries(myApp PRIVATE nlohmann_json::nlohmann_json)
