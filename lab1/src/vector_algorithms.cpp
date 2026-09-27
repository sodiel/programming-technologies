#include "vector_algorithms.h"

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