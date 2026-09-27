cmake_minimum_required(VERSION 3.23)
project(programming_technologies CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(lab1)
# add_subdirectory(lab2)

target_include_directories(lab1 PRIVATE src)
