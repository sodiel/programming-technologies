#include "markdown_analyzer.h"

#include <fstream>
#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace {

/** @brief Read one line, remove a trailing CR, and advance parser state. */
void appendLine(MarkdownElement& element, const std::string& line, std::size_t lineNumber,
                bool collectText) {
    if (element.startLine == 0) element.startLine = lineNumber;
    else if (collectText) element.text += '\n';
    if (collectText) element.text += line;
    element.endLine = lineNumber;
}

/** @brief Compare an ASCII string at an offset without allocating a substring. */
bool startsWithAt(const std::string& line, std::size_t offset, std::string_view value) {
    return offset <= line.size() && line.compare(offset, value.size(), value) == 0;
}

/** @brief Match a document-signature token using ASCII case-insensitive comparison. */
bool startsWithAsciiInsensitiveAt(const std::string& line, std::size_t offset,
                                  std::string_view value) {
    if (offset > line.size() || line.size() - offset < value.size()) return false;
    for (std::size_t i = 0; i < value.size(); ++i) {
        char ch = line[offset + i];
        if (ch >= 'A' && ch <= 'Z') ch = static_cast<char>(ch - 'A' + 'a');
        if (ch != value[i]) return false;
    }
    return true;
}

/** @brief Recognize a fenced-code opener and return its marker and length. */
bool parseFenceOpening(const std::string& line, std::size_t start,
                       char& marker, std::size_t& length) {
    if (start >= line.size() || (line[start] != '`' && line[start] != '~')) return false;
    marker = line[start];
    std::size_t end = start;
    while (end < line.size() && line[end] == marker) ++end;
    length = end - start;
    return length >= 3;
}

/** @brief Recognize a matching fenced-code closer. */
bool isFenceClosing(const std::string& line, std::size_t start,
                    char marker, std::size_t minimumLength) {
    if (start >= line.size() || line[start] != marker) return false;
    std::size_t end = start;
    while (end < line.size() && line[end] == marker) ++end;
    if (end - start < minimumLength) return false;
    while (end < line.size() && (line[end] == ' ' || line[end] == '\t')) ++end;
    return end == line.size();
}

/** @brief Recognize the supported ATX heading syntax and extract its level. */
bool parseHeading(const std::string& line, std::size_t start, unsigned& level) {
    if (start >= line.size() || line[start] != '#') return false;
    std::size_t end = start;
    while (end < line.size() && line[end] == '#') ++end;
    const auto count = end - start;
    if (count > 6 || end == line.size() || (line[end] != ' ' && line[end] != '\t')) {
        return false;
    }
    while (end < line.size() && (line[end] == ' ' || line[end] == '\t')) ++end;
    std::size_t contentEnd = line.size();
    while (contentEnd > end && (line[contentEnd - 1] == ' ' || line[contentEnd - 1] == '\t')) {
        --contentEnd;
    }
    if (contentEnd == end) return false;
    level = static_cast<unsigned>(count);
    return true;
}

/** @brief Recognize a simple ordered or unordered list item. */
bool parseListItem(const std::string& line, std::size_t start, bool& ordered) {
    if (start >= line.size()) return false;
    std::size_t end = start;
    if (line[end] == '-' || line[end] == '+' || line[end] == '*') {
        ordered = false;
        ++end;
    } else if (line[end] >= '0' && line[end] <= '9') {
        ordered = true;
        while (end < line.size() && line[end] >= '0' && line[end] <= '9') ++end;
        if (end == line.size() || (line[end] != '.' && line[end] != ')')) return false;
        ++end;
    } else {
        return false;
    }
    return end < line.size() && (line[end] == ' ' || line[end] == '\t');
}

/** @brief Detect the HTML signatures supported by the lab checker. */
bool isHtmlSignature(const std::string& line, std::size_t start) {
    if (startsWithAsciiInsensitiveAt(line, start, "<html")) {
        const auto after = start + 5;
        return after == line.size() || line[after] == ' ' || line[after] == '\t' || line[after] == '>';
    }
    if (!startsWithAsciiInsensitiveAt(line, start, "<!doctype")) return false;
    std::size_t end = start + 9;
    if (end == line.size() || (line[end] != ' ' && line[end] != '\t')) return false;
    while (end < line.size() && (line[end] == ' ' || line[end] == '\t')) ++end;
    if (!startsWithAsciiInsensitiveAt(line, end, "html")) return false;
    end += 4;
    return end == line.size() || line[end] == ' ' || line[end] == '\t' || line[end] == '>';
}

/** @brief Detect an AsciiDoc title, attribute, macro, or source block marker. */
bool isAsciiDocMarker(const std::string& line, std::size_t start) {
    if (start >= line.size()) return false;
    if (line[start] == '=') {
        std::size_t end = start + 1;
        if (end == line.size() || (line[end] != ' ' && line[end] != '\t')) return false;
        while (end < line.size() && (line[end] == ' ' || line[end] == '\t')) ++end;
        return end < line.size() && line[end] != '=';
    }
    if (line[start] == ':') {
        std::size_t end = start + 1;
        while (end < line.size()) {
            const char ch = line[end];
            if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                (ch >= '0' && ch <= '9') || ch == '_' || ch == '-') {
                ++end;
            } else {
                break;
            }
        }
        return end > start + 1 && end < line.size() && line[end] == ':';
    }
    if (line[start] == '[') {
        return startsWithAt(line, start, "[source,") || startsWithAt(line, start, "[source]");
    }
    return startsWithAt(line, start, "image::") ||
           startsWithAt(line, start, "include::") ||
           startsWithAt(line, start, "link::");
}

/** @brief Writes only blocks matching a requested kind while the source is scanned. */
struct FindOutput {
    std::ostream& stream;
    MarkdownElementType type;
    unsigned headingLevel;
    std::size_t blockCount = 0;

    bool matches(MarkdownElementType candidate, unsigned level = 0) const {
        return candidate == type &&
               (type != MarkdownElementType::Heading || headingLevel == 0 || level == headingLevel);
    }

    void beginBlock() { ++blockCount; }

    void writeLine(std::size_t lineNumber, const std::string& line) {
        stream << lineNumber << ": " << line << '\n';
    }
};

/** @brief Parse a stream incrementally while retaining only extracted blocks. */
MarkdownDocument parseLines(std::istream& input, bool collectElements,
                            FindOutput* findOutput = nullptr) {
    MarkdownDocument document;
    MarkdownElement current;
    bool hasCurrent = false;
    bool currentListOrdered = false;
    bool insideFence = false;
    char fenceMarker = 0;
    std::size_t fenceLength = 0;
    std::size_t fenceStartLine = 0;
    bool formatErrorReported = false;
    bool firstContentLine = true;
    std::size_t lineNumber = 0;

    auto flush = [&] {
        if (!hasCurrent) return;
        ++document.elementCount;
        if (collectElements) {
            document.elements.emplace(current.startLine, std::move(current));
        }
        current = MarkdownElement{};
        hasCurrent = false;
    };

    std::string line;
    while (std::getline(input, line)) {
        ++lineNumber;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (lineNumber == 1 && line.starts_with("\xEF\xBB\xBF")) {
            line.erase(0, 3);
        }
        const auto firstNonSpace = line.find_first_not_of(" \t");
        const bool blank = firstNonSpace == std::string::npos;
        const char first = blank ? '\0' : line[firstNonSpace];

        if (insideFence) {
            if (isFenceClosing(line, firstNonSpace, fenceMarker, fenceLength)) {
                insideFence = false;
            }
            continue;
        }

        if (firstContentLine && !blank) {
            firstContentLine = false;
            if (first == '=' && isAsciiDocMarker(line, firstNonSpace)) {
                document.errors.push_back({lineNumber, "Input looks like an AsciiDoc document, not Markdown"});
                formatErrorReported = true;
            }
        }

        const bool html = first == '<' && isHtmlSignature(line, firstNonSpace);
        const bool asciidoc = (first == ':' || first == '[' || first == 'i' || first == 'l') &&
                              isAsciiDocMarker(line, firstNonSpace);
        if (!formatErrorReported && (html || asciidoc)) {
            document.errors.push_back({lineNumber, html
                ? "Input looks like an HTML document, not Markdown"
                : "Input contains an AsciiDoc marker, not supported Markdown"});
            formatErrorReported = true;
        }

        char openingFenceMarker = 0;
        std::size_t openingFenceLength = 0;
        if (parseFenceOpening(line, firstNonSpace, openingFenceMarker, openingFenceLength)) {
            flush();
            insideFence = true;
            fenceMarker = openingFenceMarker;
            fenceLength = openingFenceLength;
            fenceStartLine = lineNumber;
            continue;
        }

        if (blank) {
            flush();
            continue;
        }

        unsigned headingLevel = 0;
        if (first == '#' && parseHeading(line, firstNonSpace, headingLevel)) {
            flush();
            MarkdownElement heading;
            heading.type = MarkdownElementType::Heading;
            heading.headingLevel = headingLevel;
            appendLine(heading, line, lineNumber, collectElements);
            ++document.elementCount;
            if (collectElements) {
                document.elements.emplace(heading.startLine, std::move(heading));
            }
            if (findOutput && findOutput->matches(MarkdownElementType::Heading, headingLevel)) {
                findOutput->beginBlock();
                findOutput->writeLine(lineNumber, line);
            }
            continue;
        }

        bool ordered = false;
        if ((first == '-' || first == '+' || first == '*' ||
             (first >= '0' && first <= '9')) &&
            parseListItem(line, firstNonSpace, ordered)) {
            const bool startsNewList = !hasCurrent || current.type != MarkdownElementType::List ||
                                       currentListOrdered != ordered;
            if (startsNewList) {
                flush();
                current.type = MarkdownElementType::List;
                hasCurrent = true;
                currentListOrdered = ordered;
            }
            appendLine(current, line, lineNumber, collectElements);
            if (findOutput && findOutput->matches(MarkdownElementType::List)) {
                if (startsNewList) findOutput->beginBlock();
                findOutput->writeLine(lineNumber, line);
            }
            continue;
        }

        const bool startsNewParagraph = !hasCurrent || current.type != MarkdownElementType::Paragraph;
        if (startsNewParagraph) {
            flush();
            current.type = MarkdownElementType::Paragraph;
            hasCurrent = true;
        }
        appendLine(current, line, lineNumber, collectElements);
        if (findOutput && findOutput->matches(MarkdownElementType::Paragraph)) {
            if (startsNewParagraph) findOutput->beginBlock();
            findOutput->writeLine(lineNumber, line);
        }
    }

    flush();
    if (insideFence) {
        document.errors.push_back({fenceStartLine, "Unclosed fenced code block"});
    }
    if (input.bad()) throw std::runtime_error("Error while reading Markdown input");
    return document;
}

} // namespace

MarkdownDocument MarkdownAnalyzer::parseText(std::string_view text) const {
    std::istringstream input{std::string(text)};
    return parseLines(input, true);
}

MarkdownDocument MarkdownAnalyzer::parseFile(const std::string& path, bool collectElements) const {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("Cannot open Markdown file: " + path);
    return parseLines(input, collectElements);
}

MarkdownDocument MarkdownAnalyzer::findFileTo(const std::string& inputPath,
                                               MarkdownElementType type,
                                               unsigned headingLevel,
                                               const std::string& outputPath) const {
    std::ifstream input(inputPath, std::ios::binary);
    if (!input) throw std::runtime_error("Cannot open Markdown file: " + inputPath);

    std::ofstream output(outputPath, std::ios::binary | std::ios::trunc);
    if (!output) throw std::runtime_error("Cannot create result file: " + outputPath);

    FindOutput findOutput{output, type, headingLevel};
    auto document = parseLines(input, false, &findOutput);
    output.close();
    if (!output) throw std::runtime_error("Error while writing result file: " + outputPath);

    document.elementCount = findOutput.blockCount;
    if (!document.errors.empty()) {
        std::error_code ignored;
        std::filesystem::remove(outputPath, ignored);
    }
    return document;
}

std::vector<const MarkdownElement*> MarkdownAnalyzer::findElements(
    const MarkdownDocument& document,
    MarkdownElementType type,
    unsigned headingLevel) const {
    std::vector<const MarkdownElement*> result;
    for (const auto& [line, element] : document.elements) {
        (void)line;
        if (element.type == type &&
            (type != MarkdownElementType::Heading || headingLevel == 0 ||
             element.headingLevel == headingLevel)) {
            result.push_back(&element);
        }
    }
    return result;
}
