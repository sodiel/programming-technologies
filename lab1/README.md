# Lab 1 - STL Containers

## Requirements
- C++23 compiler
- CMake >= 3.23

## Build

From the repository root:

```bash
mkdir build && cd build
cmake ..
cmake --build . --target lab1
```

## Run

```bash
./lab1/lab1 ../lab1/data/war_and_peace.ru.txt
```

## What it does
Counts unique words in a UTF-8 text file and prints them as `word - count`,
sorted by count in descending order (custom quicksort implementation).

## Sources
- cppreference.com — std::unordered_map
- https://cmake.org/cmake/help/latest/command/target_include_directories.html
- https://cmake.org/cmake/help/latest/prop_tgt/CXX_STANDARD.html
- https://stackoverflow.com/questions/50403342/how-do-i-properly-use-stdstring-on-utf-8-in-c
