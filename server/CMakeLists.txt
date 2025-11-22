
# STATIC:静的ライブラリ
# SHARED:DLL
# OBJECT:.oを作る。リンクが難しい

add_library(server STATIC
    http_server.cpp
)

find_package(civetweb CONFIG REQUIRED)

target_link_libraries(server
    PRIVATE
        civetweb::civetweb
)