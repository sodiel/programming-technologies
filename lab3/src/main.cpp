#include "form_validator.h"
#include "markdown_analyzer.h"
#include "random_user_generator.h"

#include <charconv>
#include <exception>
#include <iostream>
#include <optional>
#include <string>

namespace {

/** @brief Print supported command forms.
 * @param executable Program name used in the usage lines.
 */
void printUsage(const char* executable) {
    std::cerr << "Usage:\n"
              << "  " << executable << " check -f <file>\n"
              << "  " << executable << " check <inline-markdown>\n"
              << "  " << executable << " find -f <file> <h1|h2|h3|h4|h5|h6|paragraph|list>\n"
              << "      writes matching lines to find-result.txt\n"
              << "  " << executable << " form\n"
              << "  " << executable << " generate [count] [seed]\n";
}

/** @brief Parse a positive decimal size without accepting trailing characters.
 * @param text User-provided decimal value.
 * @param value Parsed result on success.
 * @return True when text is a positive integer representable as size_t.
 */
bool parsePositiveSize(const char* text, std::size_t& value) {
    const std::string input(text);
    const auto [end, error] = std::from_chars(input.data(), input.data() + input.size(), value);
    return error == std::errc{} && end == input.data() + input.size() && value > 0;
}

/** @brief Convert a command argument to an extraction type and optional heading level.
 * @param text Type spelling from the command line.
 * @param headingLevel Receives 1 through 6 for heading types, otherwise zero.
 * @return Parsed element type or nullopt for an unknown spelling.
 */
std::optional<MarkdownElementType> parseElementType(const std::string& text,
                                                   unsigned& headingLevel) {
    headingLevel = 0;
    if (text == "paragraph") return MarkdownElementType::Paragraph;
    if (text == "list") return MarkdownElementType::List;
    if (text.size() == 2 && text[0] == 'h' && text[1] >= '1' && text[1] <= '6') {
        headingLevel = static_cast<unsigned>(text[1] - '0');
        return MarkdownElementType::Heading;
    }
    return std::nullopt;
}

/** @brief Validate an inline Markdown argument and print any line-based errors.
 * @param text Markdown source.
 * @param sourceName Label used in diagnostics.
 * @return Zero for valid input, nonzero when validation errors were found.
 */
int runMarkdownCheck(const std::string& text, const std::string& sourceName) {
    MarkdownAnalyzer analyzer;
    const auto document = analyzer.parseText(text);
    if (!document.errors.empty()) {
        for (const auto& error : document.errors) {
            std::cerr << sourceName << ':' << error.line << ": " << error.message << '\n';
        }
        return 1;
    }

    std::cout << "Valid Markdown subset: " << document.elementCount
              << " elements found\n";
    return 0;
}

/** @brief Read one prompted form field and report end-of-input immediately.
 * @param prompt Label displayed before reading the field.
 * @param value Receives the complete line entered by the user.
 * @return True if a line was read successfully.
 */
bool readFormField(const char* prompt, std::string& value) {
    std::cout << prompt << "\n> " << std::flush;
    if (std::getline(std::cin, value)) return true;
    std::cerr << "Ввод анкеты прерван: не получены все поля.\n";
    return false;
}

} // namespace

/** @brief Dispatch Markdown, questionnaire, and generated-data commands.
 * @param argc Number of command-line arguments.
 * @param argv Command-line argument values.
 * @return Process exit code.
 */
int main(int argc, char* argv[]) {
    try {
        if (argc < 2) {
            printUsage(argv[0]);
            return 1;
        }

        const std::string command = argv[1];
        MarkdownAnalyzer markdown;

        if (command == "check") {
            if (argc == 4 && std::string(argv[2]) == "-f") {
                const auto document = markdown.parseFile(argv[3], false);
                if (!document.errors.empty()) {
                    for (const auto& error : document.errors) {
                        std::cerr << argv[3] << ':' << error.line << ": " << error.message << '\n';
                    }
                    return 1;
                }
                std::cout << "Valid Markdown subset: " << document.elementCount
                          << " elements found\n";
                return 0;
            }
            if (argc == 3) return runMarkdownCheck(argv[2], "<inline>");
            printUsage(argv[0]);
            return 1;
        }

        if (command == "find") {
            if (argc != 5 || std::string(argv[2]) != "-f") {
                printUsage(argv[0]);
                return 1;
            }
            unsigned level = 0;
            const auto type = parseElementType(argv[4], level);
            if (!type) {
                std::cerr << "Unknown element type: " << argv[4] << '\n';
                return 1;
            }
            constexpr const char* resultPath = "find-result.txt";
            const auto document = markdown.findFileTo(argv[3], *type, level, resultPath);
            if (!document.errors.empty()) {
                for (const auto& error : document.errors) {
                    std::cerr << argv[3] << ':' << error.line << ": " << error.message << '\n';
                }
                return 1;
            }
            std::cout << "Wrote " << document.elementCount << " matching blocks to "
                      << resultPath << '\n';
            return 0;
        }

        if (command == "form") {
            UserData input;
            if (!readFormField("Фамилия: ", input.surname) ||
                !readFormField("Имя: ", input.firstName) ||
                !readFormField("Отчество: ", input.patronymic) ||
                !readFormField("Дата рождения: ", input.birthDate) ||
                !readFormField("Телефон: ", input.phone) ||
                !readFormField("Почта: ", input.email)) {
                return 1;
            }

            const auto result = FormValidator{}.normalize(input);
            if (!result.errors.empty()) {
                for (const auto& error : result.errors) std::cerr << error << '\n';
                return 1;
            }
            std::cout << result.normalized->surname << ' '
                      << result.normalized->firstName << ' '
                      << result.normalized->patronymic << ' '
                      << result.normalized->birthDate << ' '
                      << result.normalized->phone << ' '
                      << result.normalized->email << '\n';
            return 0;
        }

        if (command == "generate") {
            std::size_t count = 10;
            if (argc >= 3 && !parsePositiveSize(argv[2], count)) {
                std::cerr << "Count must be a positive integer\n";
                return 1;
            }
            std::optional<std::uint64_t> seed;
            if (argc >= 4) {
                std::uint64_t value = 0;
                const std::string seedText(argv[3]);
                const auto [end, error] = std::from_chars(
                    seedText.data(), seedText.data() + seedText.size(), value);
                if (error != std::errc{} || end != seedText.data() + seedText.size()) {
                    std::cerr << "Seed must be an unsigned integer\n";
                    return 1;
                }
                seed = value;
            }
            if (argc > 4) {
                printUsage(argv[0]);
                return 1;
            }

            RandomUserGenerator generator;
            const auto records = generator.generate(count, seed);
            for (const auto& record : records) {
                std::cout << record.raw.surname << ' ' << record.raw.firstName << ' '
                          << record.raw.patronymic << ' ' << record.raw.birthDate << ' '
                          << record.raw.phone << ' ' << record.raw.email << " -> ";
                std::cout << record.normalized.surname << ' '
                          << record.normalized.firstName << ' '
                          << record.normalized.patronymic << ' '
                          << record.normalized.birthDate << ' '
                          << record.normalized.phone << ' '
                          << record.normalized.email << '\n';
            }
            return 0;
        }

        printUsage(argv[0]);
        return 1;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
