cmake_minimum_required(VERSION 3.23)
project(programming-technologies)

add_library(valid_static STATIC src/form_validator.cpp)
add_library(md_static STATIC src/markdown_analyzer.cpp)
add_library(randgen_static STATIC src/random_user_generator.cpp)

add_library(valid_shared SHARED src/form_validator.cpp)
add_library(md_shared SHARED src/markdown_analyzer.cpp)
add_library(randgen_shared SHARED src/random_user_generator.cpp)

add_executable(lab3_static src/main.cpp)
target_link_libraries(lab3_static PRIVATE valid_static md_static randgen_static)

add_executable(lab3_shared src/main.cpp)
target_link_libraries(lab3_shared PRIVATE valid_shared md_shared randgen_shared)