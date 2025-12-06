
# ---- main 実行ファイル ----
#実行ファイルの登録
add_executable(sand_capture capture.cpp)
add_executable(sand_move move.cpp)
target_compile_options(sand_move PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/Od>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-O0>
)
