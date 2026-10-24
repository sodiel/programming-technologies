cmake_minimum_required(VERSION 3.23)
project(programming-technologies)

# CMake instructions to make the static lib
add_library(libvalid SHARED src/form_validator.cpp)
add_library(libmd SHARED src/markdown_analyzer.cpp)
add_library(librandgen SHARED src/random_user_generator.cpp)

add_executable(lab3
        src/main.cpp
)


target_include_directories(lab3 PRIVATE src)
target_compile_definitions(lab3 PRIVATE LAB3_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")
target_link_libraries(lab3 PRIVATE libvalid libmd librandgen)
