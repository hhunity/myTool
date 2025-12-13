
# ---- main 実行ファイル ----
#実行ファイルの登録
add_executable(comms comms.cpp)

find_package(cppzmq CONFIG REQUIRED)
find_package(ZeroMQ CONFIG REQUIRED)

target_link_libraries(comms PRIVATE cppzmq libzmq)
