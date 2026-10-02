#include "word_analysis.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace {

using WordPair = std::pair<std::string, std::uint64_t>;

int utf8CharLength(unsigned char leadByte) {
    if ((leadByte & 0x80) == 0x00) return 1;
    if ((leadByte & 0xE0) == 0xC0) return 2;
    if ((leadByte & 0xF0) == 0xE0) return 3;
    if ((leadByte & 0xF8) == 0xF0) return 4;
    return 1;
}

bool isWordCodePoint(char32_t codePoint) {
    if ((codePoint >= U'a' && codePoint <= U'z') ||
        (codePoint >= U'A' && codePoint <= U'Z')) {
        return true;
    }
    if (codePoint >= 0x0410 && codePoint <= 0x044F) return true;
    if (codePoint == 0x0401 || codePoint == 0x0451) return true;
    return codePoint == U'-';
}

char32_t toLowerCodePoint(char32_t codePoint) {
    if (codePoint >= U'A' && codePoint <= U'Z') {
        return codePoint - U'A' + U'a';
    }
    if (codePoint >= 0x0410 && codePoint <= 0x042F) {
        return codePoint + 0x20;
    }
    if (codePoint == 0x0401) return 0x0451;
    return codePoint;
}

void appendUtf8(char32_t codePoint, std::string& out) {
    if (codePoint <= 0x7F) {
        out += static_cast<char>(codePoint);
    } else if (codePoint <= 0x7FF) {
        out += static_cast<char>(0xC0 | (codePoint >> 6));
        out += static_cast<char>(0x80 | (codePoint & 0x3F));
    } else if (codePoint <= 0xFFFF) {
        out += static_cast<char>(0xE0 | (codePoint >> 12));
        out += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (codePoint & 0x3F));
    } else {
        out += static_cast<char>(0xF0 | (codePoint >> 18));
        out += static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (codePoint & 0x3F));
    }
}

bool decodeCodePoint(const std::string& bytes, std::size_t pos, int length,
                     char32_t& codePoint) {
    if (pos + static_cast<std::size_t>(length) > bytes.size()) return false;

    const auto first = static_cast<unsigned char>(bytes[pos]);
    if (length == 1) {
        codePoint = first;
        return true;
    }

    codePoint = length == 2 ? first & 0x1F : length == 3 ? first & 0x0F : first & 0x07;
    for (int i = 1; i < length; ++i) {
        const auto continuation = static_cast<unsigned char>(bytes[pos + i]);
        if ((continuation & 0xC0) != 0x80) {
            codePoint = 0;
            return true;
        }
        codePoint = (codePoint << 6) | (continuation & 0x3F);
    }
    return true;
}

int partition(std::vector<WordPair>& data, int low, int high) {
    const auto pivot = data[high].second;
    int i = low - 1;

    for (int j = low; j < high; ++j) {
        if (data[j].second > pivot) {
            ++i;
            std::swap(data[i], data[j]);
        }
    }
    std::swap(data[i + 1], data[high]);
    return i + 1;
}

void quicksort(std::vector<WordPair>& data, int low, int high) {
    if (low < high) {
        const int pivotIndex = partition(data, low, high);
        quicksort(data, low, pivotIndex - 1);
        quicksort(data, pivotIndex + 1, high);
    }
}

} // namespace

std::string readFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

WordAnalyzer::WordAnalyzer(bool collectPositions)
    : collectPositions_(collectPositions) {}

void WordAnalyzer::consume(std::string_view chunk) {
    if (finished_) throw std::logic_error("Cannot consume text after finish()");
    pendingBytes_.append(chunk);

    std::size_t pos = 0;
    while (pos < pendingBytes_.size()) {
        const int length = utf8CharLength(static_cast<unsigned char>(pendingBytes_[pos]));
        if (pos + static_cast<std::size_t>(length) > pendingBytes_.size()) break;

        char32_t codePoint = 0;
        if (!decodeCodePoint(pendingBytes_, pos, length, codePoint)) break;
        if (codePoint == 0) {
            // A malformed sequence is treated as a separator, one byte at a time.
            ++pos;
            finishWord();
            continue;
        }

        if (isWordCodePoint(codePoint)) {
            appendUtf8(toLowerCodePoint(codePoint), currentWord_);
        } else {
            finishWord();
        }
        pos += static_cast<std::size_t>(length);
    }
    pendingBytes_.erase(0, pos);
}

void WordAnalyzer::finish() {
    if (finished_) return;
    // Incomplete UTF-8 bytes at EOF are separators.
    pendingBytes_.clear();
    finishWord();
    finished_ = true;
}

const WordCounts& WordAnalyzer::counts() const {
    return counts_;
}

const WordPositions& WordAnalyzer::positions() const {
    return positions_;
}

std::vector<std::pair<std::string, std::uint64_t>> WordAnalyzer::sortedCounts() const {
    std::vector<WordPair> sorted(counts_.begin(), counts_.end());
    if (!sorted.empty()) quicksort(sorted, 0, static_cast<int>(sorted.size()) - 1);
    return sorted;
}

void WordAnalyzer::finishWord() {
    if (currentWord_.empty()) return;
    if (collectPositions_) {
        positions_[currentWord_].push_back(wordIndex_);
    } else {
        ++counts_[currentWord_];
    }
    ++wordIndex_;
    currentWord_.clear();
}
