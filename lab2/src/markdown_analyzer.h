#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <string_view>
#include <vector>

/** Identifies one of the Markdown block kinds collected by the lab analyzer. */
enum class MarkdownElementType {
    Heading,
    Paragraph,
    List
};

/** Stores one extracted Markdown block and its one-based source line range. */
struct MarkdownElement {
    /** @brief Block kind. */
    MarkdownElementType type = MarkdownElementType::Paragraph;
    /** @brief ATX heading level from 1 through 6, or zero for other blocks. */
    unsigned headingLevel = 0;
    /** @brief One-based first source line. */
    std::size_t startLine = 0;
    /** @brief One-based last source line. */
    std::size_t endLine = 0;
    /** @brief Original block text, including Markdown markers. */
    std::string text;
};

/** Describes a validation error at a one-based source line. */
struct MarkdownError {
    /** @brief One-based source line associated with this error. */
    std::size_t line = 0;
    /** @brief Diagnostic suitable for command-line output. */
    std::string message;
};

/** Holds extracted blocks keyed by their first line and any validation errors. */
struct MarkdownDocument {
    /** @brief Extracted elements keyed by their first source line. */
    std::map<std::size_t, MarkdownElement> elements;
    /** @brief Number of parsed blocks, including blocks not retained in the map. */
    std::size_t elementCount = 0;
    /** @brief Validation and format-detection errors. */
    std::vector<MarkdownError> errors;
};

/** Parses the documented Markdown block subset and indexes elements by source line. */
class MarkdownAnalyzer {
public:
    /** Parse Markdown text and return extracted blocks and line-based errors.
     * @param text UTF-8 Markdown source.
     * @return Parsed document with one-based line numbers.
     */
    MarkdownDocument parseText(std::string_view text) const;

    /** Read and parse a UTF-8 Markdown file line by line.
     * @param path Input file path.
     * @param collectElements Retain extracted blocks for later searching.
     * @return Parsed document with one-based line numbers.
     * @throws std::runtime_error if the file cannot be opened or read.
     */
    MarkdownDocument parseFile(const std::string& path, bool collectElements = true) const;

    /** Stream matching blocks from a file to an output file without indexing all blocks.
     * @param inputPath Markdown source path.
     * @param type Element kind to write.
     * @param headingLevel Heading level filter; ignored for non-heading elements.
     * @param outputPath Destination file, overwritten if it already exists.
     * @return Document containing errors and the number of matching blocks in elementCount.
     */
    MarkdownDocument findFileTo(const std::string& inputPath,
                                MarkdownElementType type,
                                unsigned headingLevel,
                                const std::string& outputPath) const;

    /** Find blocks of one type, optionally limited to a heading level.
     * @param document Parsed document to search.
     * @param type Required block kind.
     * @param headingLevel Required level for headings; ignored for other types.
     * @return Pointers to matching blocks in source order; blocks remain owned by document.
     */
    std::vector<const MarkdownElement*> findElements(
        const MarkdownDocument& document,
        MarkdownElementType type,
        unsigned headingLevel = 0) const;
};
