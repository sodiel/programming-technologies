#include "form_validator.h"

#include <chrono>
#include <cstdint>
#include <iomanip>
#include <regex>
#include <sstream>
#include <string_view>

namespace {

/** @brief Remove leading and trailing ASCII whitespace. */
std::string trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

/** @brief Decode one UTF-8 code point without accepting truncated sequences. */
bool decodeUtf8(std::string_view text, std::size_t pos, std::size_t& length, char32_t& cp) {
    const auto first = static_cast<unsigned char>(text[pos]);
    if (first < 0x80) {
        length = 1;
        cp = first;
        return true;
    }
    if ((first & 0xE0) == 0xC0) length = 2;
    else if ((first & 0xF0) == 0xE0) length = 3;
    else if ((first & 0xF8) == 0xF0) length = 4;
    else return false;
    if (pos + length > text.size()) return false;

    cp = length == 2 ? first & 0x1F : length == 3 ? first & 0x0F : first & 0x07;
    for (std::size_t i = 1; i < length; ++i) {
        const auto next = static_cast<unsigned char>(text[pos + i]);
        if ((next & 0xC0) != 0x80) return false;
        cp = (cp << 6) | (next & 0x3F);
    }
    if ((length == 2 && cp < 0x80) ||
        (length == 3 && (cp < 0x800 || (cp >= 0xD800 && cp <= 0xDFFF))) ||
        (length == 4 && (cp < 0x10000 || cp > 0x10FFFF))) {
        return false;
    }
    return true;
}

/** @brief Check whether a code point belongs to the Russian alphabet. */
bool isRussianLetter(char32_t cp) {
    return (cp >= 0x0410 && cp <= 0x042F) ||
           (cp >= 0x0430 && cp <= 0x044F) || cp == 0x0401 || cp == 0x0451;
}

/** @brief Change the case of a Russian code point. */
char32_t russianCase(char32_t cp, bool upper) {
    if (upper && cp >= 0x0430 && cp <= 0x044F) return cp - 0x20;
    if (!upper && cp >= 0x0410 && cp <= 0x042F) return cp + 0x20;
    if (upper && cp == 0x0451) return 0x0401;
    if (!upper && cp == 0x0401) return 0x0451;
    return cp;
}

/** @brief Append a Unicode scalar value encoded as UTF-8. */
void appendUtf8(char32_t cp, std::string& output) {
    if (cp <= 0x7F) output.push_back(static_cast<char>(cp));
    else if (cp <= 0x7FF) {
        output.push_back(static_cast<char>(0xC0 | (cp >> 6)));
        output.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp <= 0xFFFF) {
        output.push_back(static_cast<char>(0xE0 | (cp >> 12)));
        output.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        output.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else {
        output.push_back(static_cast<char>(0xF0 | (cp >> 18)));
        output.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        output.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        output.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
}

/** @brief Remove digits, validate a Russian name, and apply title case. */
bool normalizeName(std::string value, std::string& normalized) {
    static const std::regex digits(R"([0-9])");
    value = trim(std::regex_replace(value, digits, ""));
    if (value.empty()) return false;

    std::string candidate;
    bool capitalize = true;
    bool previousHyphen = true;
    for (std::size_t pos = 0; pos < value.size();) {
        std::size_t length = 0;
        char32_t cp = 0;
        if (!decodeUtf8(value, pos, length, cp)) return false;
        if (cp == U'-') {
            if (previousHyphen) return false;
            candidate.push_back('-');
            capitalize = true;
            previousHyphen = true;
        } else {
            if (!isRussianLetter(cp)) return false;
            appendUtf8(russianCase(cp, capitalize), candidate);
            capitalize = false;
            previousHyphen = false;
        }
        pos += length;
    }
    if (previousHyphen) return false;
    normalized = std::move(candidate);
    return true;
}

/** @brief Parse a supported date and format it as DD.MM.YYYY. */
bool normalizeDate(const std::string& raw, std::string& normalized) {
    static const std::regex iso(R"(^([0-9]{4})-([0-9]{2})-([0-9]{2})$)");
    static const std::regex local(R"(^([0-9]{1,2})\.([0-9]{1,2})\.([0-9]{4})$)");
    std::smatch match;
    int year = 0;
    unsigned month = 0;
    unsigned day = 0;
    if (std::regex_match(raw, match, iso)) {
        year = std::stoi(match[1].str());
        month = static_cast<unsigned>(std::stoi(match[2].str()));
        day = static_cast<unsigned>(std::stoi(match[3].str()));
    } else if (std::regex_match(raw, match, local)) {
        day = static_cast<unsigned>(std::stoi(match[1].str()));
        month = static_cast<unsigned>(std::stoi(match[2].str()));
        year = std::stoi(match[3].str());
    } else {
        return false;
    }

    const std::chrono::year_month_day date{
        std::chrono::year{year}, std::chrono::month{month}, std::chrono::day{day}};
    if (!date.ok()) return false;
    const auto today = std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now());
    if (std::chrono::sys_days{date} > today) return false;

    std::ostringstream output;
    output << std::setfill('0') << std::setw(2) << day << '.'
           << std::setw(2) << month << '.' << std::setw(4) << year;
    normalized = output.str();
    return true;
}

/** @brief Normalize an accepted Russian phone spelling to +7 followed by ten digits. */
bool normalizePhone(std::string value, std::string& normalized) {
    static const std::regex separators(R"([ ()-])");
    static const std::regex digitsOnly(R"(^[0-9]+$)");
    value = std::regex_replace(trim(value), separators, "");
    std::string digits;
    if (value.starts_with("+7") || value.starts_with("+8")) digits = value.substr(2);
    else if (value.starts_with('+')) return false;
    else digits = value;

    if (!std::regex_match(digits, digitsOnly)) return false;
    if (digits.size() == 11 && (digits.front() == '7' || digits.front() == '8')) {
        digits.erase(digits.begin());
    }
    if (digits.size() != 10) return false;
    normalized = "+7" + digits;
    return true;
}

/** @brief Validate an ASCII email address and lowercase its bytes. */
bool normalizeEmail(std::string value, std::string& normalized) {
    static const std::regex emailPattern(
        R"(^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,63}$)");

    value = trim(value);
    if (!std::regex_match(value, emailPattern)) {
        return false;
    }
    for (char& ch : value) {
        if (ch >= 'A' && ch <= 'Z') ch = static_cast<char>(ch - 'A' + 'a');
    }
    normalized = std::move(value);
    return true;
}

} // namespace

ValidationResult FormValidator::normalize(const UserData& input) const {
    UserData normalized;
    ValidationResult result;
    if (!normalizeName(input.surname, normalized.surname)) {
        result.errors.emplace_back("Фамилия должна содержать русские буквы и дефисы");
    }
    if (!normalizeName(input.firstName, normalized.firstName)) {
        result.errors.emplace_back("Имя должно содержать русские буквы и дефисы");
    }
    if (!normalizeName(input.patronymic, normalized.patronymic)) {
        result.errors.emplace_back("Отчество должно содержать русские буквы и дефисы");
    }
    if (!normalizeDate(trim(input.birthDate), normalized.birthDate)) {
        result.errors.emplace_back("Дата должна быть реальной и не будущей: YYYY-MM-DD или DD.MM.YYYY");
    }
    if (!normalizePhone(input.phone, normalized.phone)) {
        result.errors.emplace_back("Телефон должен содержать российский номер из 10 цифр");
    }
    if (!normalizeEmail(input.email, normalized.email)) {
        result.errors.emplace_back("Почта должна иметь формат name@domain.tld");
    }
    if (result.errors.empty()) result.normalized = std::move(normalized);
    return result;
}
