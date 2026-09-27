#include "word_analysis.h"

#include <fstream>
#include <sstream>
#include <cctype>

namespace {

using WordPair = std::pair<std::string, int>;

/**
 * @brief Determines how many bytes a UTF-8 character starting at this byte occupies.
 * @param leadByte The first byte of a (potentially multi-byte) UTF-8 character.
 * @return Number of bytes the character occupies (1 to 4).
 */
int utf8CharLength(unsigned char leadByte) {
    if ((leadByte & 0x80) == 0x00) return 1; // 0xxxxxxx -> ASCII
    if ((leadByte & 0xE0) == 0xC0) return 2; // 110xxxxx -> 2-byte sequence
    if ((leadByte & 0xF0) == 0xE0) return 3; // 1110xxxx -> 3-byte sequence
    if ((leadByte & 0xF8) == 0xF0) return 4; // 11110xxx -> 4-byte sequence
    return 1; // invalid lead byte, treat as single byte to avoid infinite loops
}

/**
 * @brief Decodes a UTF-8 code point into its Unicode scalar value.
 * @param text Full text buffer.
 * @param pos Byte offset where the character starts.
 * @param length Number of bytes this character occupies (from utf8CharLength).
 * @return The Unicode code point, or 0 if the sequence is malformed/out of bounds.
 */
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
        if ((continuationByte & 0xC0) != 0x80) return 0; // not a valid continuation byte
        codePoint = (codePoint << 6) | (continuationByte & 0x3F);
    }

    return codePoint;
}

/**
 * @brief Checks whether a Unicode code point is a letter we treat as part of a word.
 * Covers ASCII letters, Cyrillic letters (basic block), and the hyphen.
 * @param codePoint Unicode scalar value.
 * @return true if the code point should be treated as a word character.
 */
bool isWordCodePoint(char32_t codePoint) {
    // ASCII letters
    if ((codePoint >= U'a' && codePoint <= U'z') ||
        (codePoint >= U'A' && codePoint <= U'Z')) {
        return true;
    }
    // Cyrillic basic block: U+0410-U+044F covers А-Я, а-я; U+0401/U+0451 are Ё/ё
    if (codePoint >= 0x0410 && codePoint <= 0x044F) return true;
    if (codePoint == 0x0401 || codePoint == 0x0451) return true; // Ё, ё
    // Hyphen, to keep hyphenated words together (e.g. "из-за", "кто-то")
    if (codePoint == U'-') return true;

    return false;
}

/**
 * @brief Converts a Unicode code point to lowercase, for the ranges we support.
 * @param codePoint Unicode scalar value.
 * @return Lowercase version of the code point, or the same value if not applicable.
 */
char32_t toLowerCodePoint(char32_t codePoint) {
    if (codePoint >= U'A' && codePoint <= U'Z') {
        return codePoint - U'A' + U'a';
    }
    if (codePoint >= 0x0410 && codePoint <= 0x042F) { // А-Я -> а-я
        return codePoint + 0x20;
    }
    if (codePoint == 0x0401) return 0x0451; // Ё -> ё
    return codePoint;
}

/**
 * @brief Encodes a Unicode code point back into a UTF-8 byte sequence, appended to out.
 * @param codePoint Unicode scalar value to encode.
 * @param out String to append the encoded bytes to.
 */
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
 * @brief Partitions the range [low, high] around a pivot, descending by count.
 * @param data Vector being sorted.
 * @param low Index of the first element in the range.
 * @param high Index of the last element in the range.
 * @return Final index of the pivot element.
 */
int partition(std::vector<WordPair>& data, int low, int high) {
    int pivot = data[high].second;
    int i = low - 1;

    for (int j = low; j < high; ++j) {
        if (data[j].second > pivot) { // descending order
            ++i;
            std::swap(data[i], data[j]);
        }
    }
    std::swap(data[i + 1], data[high]);
    return i + 1;
}

/**
 * @brief Recursively sorts [low, high] in descending order of count.
 * @param data Vector being sorted.
 * @param low Index of the first element in the range.
 * @param high Index of the last element in the range.
 */
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
                wordCount[currentWord]++;
                currentWord.clear();
            }
        }

        pos += static_cast<size_t>(length);
    }

    if (!currentWord.empty()) {
        wordCount[currentWord]++;
    }

    return wordCount;
}

std::vector<WordPair> sortByCount(const std::unordered_map<std::string, int>& wordCount) {
    std::vector<WordPair> sorted(wordCount.begin(), wordCount.end());

    if (!sorted.empty()) {
        quicksort(sorted, 0, static_cast<int>(sorted.size()) - 1);
    }

    return sorted;
}