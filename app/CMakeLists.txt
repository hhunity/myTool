
# ---- main 実行ファイル ----
#実行ファイルの登録
add_executable(myApp main.cpp)

#依存関係のリンク
target_link_libraries(myApp
    PUBLIC
    ftxui
    lineStore
)
