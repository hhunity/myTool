
# ---- main 実行ファイル ----
#実行ファイルの登録
add_library(LineStore SHARED
    LineStore.cpp
    LineStore.hpp
)

#依存関係のリンク
target_include_directories(LineStore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${OpenCV_INCLUDE_DIRS})
target_link_libraries(LineStore PRIVATE ${OpenCV_LIBS})