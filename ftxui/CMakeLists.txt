
# STATIC:静的ライブラリ
# SHARED:DLL
# OBJECT:.oを作る。リンクが難しい

add_library(ftxui STATIC
    ftxui_main.cpp
)

target_link_libraries(ftxui PRIVATE ftxui::component)
target_link_libraries(ftxui PRIVATE nlohmann_json::nlohmann_json)
