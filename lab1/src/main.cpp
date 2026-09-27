#include "word_analysis.h"
#include <print>
#include <string>

int main(int argc, char* argv[]) {
  if (argc < 3) {
    std::println(stderr, "Usage: {} <path_to_file> <count|positions>", argv[0]);
    return 1;
  }

  std::string path = argv[1];
  std::string mode = argv[2];

  std::string text = readFile(path);

  if (mode == "count") {
    auto wordCount = countWords(text);
    auto sorted = sortByCount(wordCount);

    for (const auto& [word, count] : sorted) {
      std::println("{} - {}", word, count);
    }
  } else if (mode == "positions") {
    auto positions = indexWordPositions(text);

    for (const auto& [word, pos] : positions) {
      std::print("{} - ", word);
      for (size_t i = 0; i < pos.size(); ++i) {
        std::print("{}{}", pos[i], i + 1 < pos.size() ? ", " : "");
      }
      std::println("");
    }
  } else {
    std::println(stderr, "Unknown mode: {}. Use 'count' or 'positions'.", mode);
    return 1;
  }

  return 0;
}