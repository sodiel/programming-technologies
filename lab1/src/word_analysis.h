#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <utility>

/**
 * @brief Reads the entire content of a file into a string.
 * @param path Path to the file.
 * @return File content as a single string (raw bytes, UTF-8 encoded).
 */
std::string readFile(const std::string& path);

/**
 * @brief Counts occurrences of each unique word in the given UTF-8 text.
 * @param text Input text, UTF-8 encoded.
 * @return Map from word to its occurrence count.
 */
std::unordered_map<std::string, int> countWords(const std::string& text);

/**
 * @brief Sorts word-count pairs by count in descending order using a custom quicksort.
 * @param wordCount Map of word counts.
 * @return Vector of (word, count) pairs sorted by count descending.
 */
std::vector<std::pair<std::string, int>> sortByCount(
    const std::unordered_map<std::string, int>& wordCount);

/**
 * @brief Indexes the position (word index, not byte offset) of every occurrence
 * of each unique word in the given UTF-8 text.
 * @param text Input text, UTF-8 encoded.
 * @return Map from word to a vector of its positions (0-based word index) in the text.
 */
std::unordered_map<std::string, std::vector<int>> indexWordPositions(const std::string& text);