#include "vector_algorithms.h"

#include <utility>
#include <set>

namespace {

/**
 * @brief Decides whether a should come before b under the target ordering:
 * odd numbers ascending first, then even numbers descending.
 * @param a First number.
 * @param b Second number.
 * @return true if a should be placed before b.
 */
bool shouldComeBefore(int a, int b) {
    bool aOdd = (a % 2 != 0);
    bool bOdd = (b % 2 != 0);

    if (aOdd && bOdd) return a < b;
    if (!aOdd && !bOdd) return a > b;
    return aOdd;
}

/**
 * @brief Partitions [low, high] according to shouldComeBefore, using the last element as pivot.
 * @param data Vector being sorted.
 * @param low Index of the first element in the range.
 * @param high Index of the last element in the range.
 * @return Final index of the pivot element.
 */
int partitionByRule(std::vector<int>& data, int low, int high) {
    int pivot = data[high];
    int i = low - 1;

    for (int j = low; j < high; ++j) {
        if (shouldComeBefore(data[j], pivot)) {
            ++i;
            std::swap(data[i], data[j]);
        }
    }
    std::swap(data[i + 1], data[high]);
    return i + 1;
}

/**
 * @brief Recursively sorts [low, high] according to shouldComeBefore.
 * @param data Vector being sorted.
 * @param low Index of the first element in the range.
 * @param high Index of the last element in the range.
 */
void quicksortByRule(std::vector<int>& data, int low, int high) {
    if (low < high) {
        int pivotIndex = partitionByRule(data, low, high);
        quicksortByRule(data, low, pivotIndex - 1);
        quicksortByRule(data, pivotIndex + 1, high);
    }
}

} // namespace

bool isPrime(int n) {
    if (n < 2) return false;
    for (int i = 2; i * i <= n; ++i) {
        if (n % i == 0) return false;
    }
    return true;
}

void squarePrimes(std::vector<int>& numbers) {
    for (int& n : numbers) {
        if (isPrime(n)) {
            n = n * n;
        }
    }
}

void sortOddAscEvenDesc(std::vector<int>& numbers) {
    if (!numbers.empty()) {
        quicksortByRule(numbers, 0, static_cast<int>(numbers.size()) - 1);
    }
}

std::vector<int> findUniqueInRange(const std::vector<int>& numbers, int minValue, int maxValue) {
    std::set<int> uniqueValues;

    for (int n : numbers) {
        if (n >= minValue && n <= maxValue) {
            uniqueValues.insert(n);
        }
    }

    return std::vector<int>(uniqueValues.begin(), uniqueValues.end());
}