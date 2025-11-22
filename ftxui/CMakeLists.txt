
# STATIC:静的ライブラリ
# SHARED:DLL
# OBJECT:.oを作る。リンクが難しい

add_library(ftxui STATIC
    ftxui_main.cpp
)

find_package(ftxui CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

target_link_libraries(ftxui PRIVATE ftxui::component)
target_link_libraries(ftxui PRIVATE nlohmann_json::nlohmann_json)
