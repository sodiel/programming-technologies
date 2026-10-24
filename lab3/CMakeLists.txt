cmake_minimum_required(VERSION 3.23)
project(programming-technologies)
add_executable(lab3
        src/main.cpp
        src/markdown_analyzer.cpp
        src/form_validator.cpp
        src/random_user_generator.cpp
)

target_include_directories(lab2 PRIVATE src)
target_compile_definitions(lab2 PRIVATE LAB2_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")
