
add_library(lineStore STATIC
    LineStore.cpp
    LineStore.hpp
)

target_include_directories(lineStore PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${OpenCV_INCLUDE_DIRS})
target_link_libraries(lineStore PRIVATE ${OpenCV_LIBS})