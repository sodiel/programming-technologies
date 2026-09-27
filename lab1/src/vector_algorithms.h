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