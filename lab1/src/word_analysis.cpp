#include "word_analysis.h"

#include <fstream>
#include <sstream>
#include <cctype>

namespace {

using WordPair = std::pair<std::string, int>;

int utf8CharLength(unsigned char leadByte) {
    if ((leadByte & 0x80) == 0x00) return 1;
    if ((leadByte & 0xE0) == 0xC0) return 2;
    if ((leadByte & 0xF0) == 0xE0) return 3;
    if ((leadByte & 0xF8) == 0xF0) return 4;
    return 1;
}

char32_t decodeUtf8CodePoint(const std::string& text, size_t pos, int length) {
    if (pos + static_cast<size_t>(length) > text.size()) return 0;

    unsigned char first = static_cast<unsigned char>(text[pos]);
    char32_t codePoint = 0;

    if (length == 1) {
        codePoint = first;
    } else if (length == 2) {
        codePoint = first & 0x1F;
    } else if (length == 3) {
        codePoint = first & 0x0F;
    } else if (length == 4) {
        codePoint = first & 0x07;
    }

    for (int i = 1; i < length; ++i) {
        unsigned char continuationByte = static_cast<unsigned char>(text[pos + i]);
        if ((continuationByte & 0xC0) != 0x80) return 0;
        codePoint = (codePoint << 6) | (continuationByte & 0x3F);
    }

    return codePoint;
}

bool isWordCodePoint(char32_t codePoint) {
    if ((codePoint >= U'a' && codePoint <= U'z') ||
        (codePoint >= U'A' && codePoint <= U'Z')) {
        return true;
    }
    if (codePoint >= 0x0410 && codePoint <= 0x044F) return true;
    if (codePoint == 0x0401 || codePoint == 0x0451) return true;
    if (codePoint == U'-') return true;

    return false;
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

/**
 * @brief Splits UTF-8 text into a sequence of lowercase words, in order of appearance.
 * @param text Input text, UTF-8 encoded.
 * @return Vector of words in the order they appear in the text.
 */
std::vector<std::string> splitWords(const std::string& text) {
    std::vector<std::string> words;
    std::string currentWord;

    size_t pos = 0;
    while (pos < text.size()) {
        unsigned char leadByte = static_cast<unsigned char>(text[pos]);
        int length = utf8CharLength(leadByte);
        char32_t codePoint = decodeUtf8CodePoint(text, pos, length);

        if (isWordCodePoint(codePoint)) {
            appendUtf8(toLowerCodePoint(codePoint), currentWord);
        } else {
            if (!currentWord.empty()) {
                words.push_back(currentWord);
                currentWord.clear();
            }
        }

        pos += static_cast<size_t>(length);
    }

    if (!currentWord.empty()) {
        words.push_back(currentWord);
    }

    return words;
}

int partition(std::vector<WordPair>& data, int low, int high) {
    int pivot = data[high].second;
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
        int pivotIndex = partition(data, low, high);
        quicksort(data, low, pivotIndex - 1);
        quicksort(data, pivotIndex + 1, high);
    }
}

} // namespace

std::string readFile(const std::string& path) {
    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::unordered_map<std::string, int> countWords(const std::string& text) {
    std::unordered_map<std::string, int> wordCount;

    for (const auto& word : splitWords(text)) {
        wordCount[word]++;
    }

    return wordCount;
}

std::unordered_map<std::string, std::vector<int>> indexWordPositions(const std::string& text) {
    std::unordered_map<std::string, std::vector<int>> positions;

    auto words = splitWords(text);
    for (int i = 0; i < static_cast<int>(words.size()); ++i) {
        positions[words[i]].push_back(i);
    }

    return positions;
}

std::vector<WordPair> sortByCount(const std::unordered_map<std::string, int>& wordCount) {
    std::vector<WordPair> sorted(wordCount.begin(), wordCount.end());

    if (!sorted.empty()) {
        quicksort(sorted, 0, static_cast<int>(sorted.size()) - 1);
    }

    return sorted;
}