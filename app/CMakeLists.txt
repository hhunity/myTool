
# ---- main 実行ファイル ----
#実行ファイルの登録
add_executable(demo main.cpp)
#依存関係のリンク
target_link_libraries(demo PRIVATE LineStore ${OpenCV_LIBS})
