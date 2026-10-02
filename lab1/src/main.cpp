#include "word_analysis.h"
#include "vector_algorithms.h"
#include <print>
#include <string>
#include <vector>
#include <chrono>
#include <exception>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::println(stderr, "Usage: {} <count|positions|primes|sort|range> [path_to_file]", argv[0]);
        return 1;
    }

    std::string mode = argv[1];

    if (mode == "count" || mode == "positions") {
        if (argc < 3) {
            std::println(stderr, "Usage: {} {} <path_to_file>", argv[0], mode);
            return 1;
        }
        const bool collectPositions = mode == "positions";
        auto start = std::chrono::high_resolution_clock::now();
        WordAnalyzer analyzer(collectPositions);
        try {
            analyzer.processFile(argv[2]);
        } catch (const std::exception& error) {
            std::println(stderr, "{}", error.what());
            return 1;
        }

        if (mode == "count") {
            auto sorted = analyzer.sortedCounts();
            auto end = std::chrono::high_resolution_clock::now();

            for (const auto& [word, count] : sorted) {
                std::println("{} - {}", word, count);
            }
            std::println(stderr, "Time: {:.3f} ms",
                std::chrono::duration<double, std::milli>(end - start).count());
        } else {
            const auto& positions = analyzer.positions();
            auto end = std::chrono::high_resolution_clock::now();

            for (const auto& [word, pos] : positions) {
                std::print("{} - ", word);
                for (size_t i = 0; i < pos.size(); ++i) {
                    std::print("{}{}", pos[i], i + 1 < pos.size() ? ", " : "");
                }
                std::println("");
            }
            std::println(stderr, "Time: {:.3f} ms",
                std::chrono::duration<double, std::milli>(end - start).count());
        }
    } else if (mode == "primes") {
        std::vector<int> numbers = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};

        auto start = std::chrono::high_resolution_clock::now();
        squarePrimes(numbers);
        auto end = std::chrono::high_resolution_clock::now();

        for (int n : numbers) {
            std::print("{} ", n);
        }
        std::println("");
        std::println(stderr, "Time: {:.3f} ms",
            std::chrono::duration<double, std::milli>(end - start).count());
    } else if (mode == "sort") {
        std::vector<int> numbers = {5, 2, 9, 4, 1, 8, 3, 6, 7, 10};

        auto start = std::chrono::high_resolution_clock::now();
        sortOddAscEvenDesc(numbers);
        auto end = std::chrono::high_resolution_clock::now();

        for (int n : numbers) {
            std::print("{} ", n);
        }
        std::println("");
        std::println(stderr, "Time: {:.3f} ms",
            std::chrono::duration<double, std::milli>(end - start).count());
    } else if (mode == "range") {
        std::vector<int> numbers = {5, 2, 8, 2, 10, 5, 3, 8, 1, 15};
        int minValue = 2;
        int maxValue = 8;

        auto start = std::chrono::high_resolution_clock::now();
        auto result = findUniqueInRange(numbers, minValue, maxValue);
        auto end = std::chrono::high_resolution_clock::now();

        for (int n : result) {
            std::print("{} ", n);
        }
        std::println("");
        std::println(stderr, "Time: {:.3f} ms",
            std::chrono::duration<double, std::milli>(end - start).count());
    } else {
        std::println(stderr, "Unknown mode: {}", mode);
        return 1;
    }

    return 0;
}
