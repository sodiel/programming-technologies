#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

using WordCounts = std::unordered_map<std::string, std::uint64_t>;
using WordPositions = std::unordered_map<std::string, std::vector<std::uint64_t>>;

/** Incrementally tokenizes UTF-8 text and collects either counts or positions. */
class WordAnalyzer {
public:
    explicit WordAnalyzer(bool collectPositions = false);

    /** Add the next text fragment. Fragments may end inside a UTF-8 character or word. */
    void consume(std::string_view chunk);

    /** Read a file in fixed-size chunks and finish analysis at EOF. */
    void processFile(const std::string& path, std::size_t chunkSize = 64 * 1024);

    /** Mark input complete and flush the last word. Safe to call more than once. */
    void finish();

    const WordCounts& counts() const;
    const WordPositions& positions() const;
    std::vector<std::pair<std::string, std::uint64_t>> sortedCounts() const;

private:
    void finishWord();

    bool collectPositions_;
    bool finished_ = false;
    std::uint64_t wordIndex_ = 0;
    std::string pendingBytes_;
    std::string currentWord_;
    WordCounts counts_;
    WordPositions positions_;
};
