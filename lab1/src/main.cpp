#include "word_analysis.h"
#include <print>

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::println(stderr, "Usage: {} <path_to_file>", argv[0]);
    return 1;
  }

  std::string text = readFile(argv[1]);
  auto wordCount = countWords(text);
  auto sorted = sortByCount(wordCount);

  for (const auto& [word, count] : sorted) {
    std::println("{} - {}", word, count);
  }

  return 0;
}