#pragma once

#include <vector>

/**
 * @brief Checks whether a number is prime.
 * @param n Number to check.
 * @return true if n is prime, false otherwise.
 */
bool isPrime(int n);

/**
 * @brief Squares every prime number in the vector, in place.
 * @param numbers Vector to modify.
 */
void squarePrimes(std::vector<int>& numbers);

/**
 * @brief Sorts the vector: odd numbers ascending first, then even numbers descending.
 * @param numbers Vector to sort, in place.
 */
void sortOddAscEvenDesc(std::vector<int>& numbers);

/**
 * @brief Finds unique elements from the vector that fall within [minValue, maxValue].
 * @param numbers Input vector.
 * @param minValue Lower bound of the range (inclusive).
 * @param maxValue Upper bound of the range (inclusive).
 * @return Vector of unique values within the range, in ascending order.
 */
std::vector<int> findUniqueInRange(const std::vector<int>& numbers, int minValue, int maxValue);