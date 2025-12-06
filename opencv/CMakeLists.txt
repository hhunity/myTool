
# ---- main 実行ファイル ----
#実行ファイルの登録
add_executable(cvpahse cv_pahse.cpp)

find_package(OpenCV REQUIRED)

target_link_libraries(cvpahse PRIVATE 
    opencv_core
    opencv_imgproc
    opencv_highgui
)