add_executable(lab1
        src/main.cpp
        src/word_analysis.cpp
        src/vector_algorithms.cpp
)

target_include_directories(lab1 PRIVATE src)