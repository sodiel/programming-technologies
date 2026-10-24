#include "random_user_generator.h"

#include <chrono>
#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#ifndef LAB2_DATA_DIR
#define LAB2_DATA_DIR "data"
#endif

namespace {

struct DictionaryEntry {
    std::string surname;
    std::string firstName;
    std::string patronymic;
    std::string gender;
};

/** @brief Split one unquoted four-column CSV row from the supplied dictionary. */
bool parseDictionaryRow(const std::string& line, DictionaryEntry& entry) {
    const auto first = line.find(',');
    const auto second = first == std::string::npos ? first : line.find(',', first + 1);
    const auto third = second == std::string::npos ? second : line.find(',', second + 1);
    if (first == std::string::npos || second == std::string::npos || third == std::string::npos ||
        line.find(',', third + 1) != std::string::npos) {
        return false;
    }
    entry.surname = line.substr(0, first);
    entry.firstName = line.substr(first + 1, second - first - 1);
    entry.patronymic = line.substr(second + 1, third - second - 1);
    entry.gender = line.substr(third + 1);
    return !entry.surname.empty() && !entry.firstName.empty() && !entry.patronymic.empty() &&
           (entry.gender == "М" || entry.gender == "Ж");
}

/** @brief Produce a Gregorian date as DD.MM.YYYY. */
std::string formatDate(unsigned day, unsigned month, int year) {
    std::ostringstream output;
    output << std::setfill('0') << std::setw(2) << day << '.'
           << std::setw(2) << month << '.' << std::setw(4) << year;
    return output.str();
}

/** @brief Generate a random real date between 1940 and 2005. */
std::string randomBirthDate(std::mt19937_64& engine) {
    std::uniform_int_distribution<int> yearDistribution(1940, 2005);
    std::uniform_int_distribution<unsigned> monthDistribution(1, 12);
    const int year = yearDistribution(engine);
    const unsigned month = monthDistribution(engine);
    const auto lastDay = std::chrono::year_month_day_last{
        std::chrono::year{year}, std::chrono::month_day_last{std::chrono::month{month}}}.day();
    std::uniform_int_distribution<unsigned> dayDistribution(1, static_cast<unsigned>(lastDay));
    const unsigned day = dayDistribution(engine);
    if (std::uniform_int_distribution<int>(0, 1)(engine) == 0) {
        std::ostringstream output;
        output << std::setfill('0') << std::setw(4) << year << '-'
               << std::setw(2) << month << '-' << std::setw(2) << day;
        return output.str();
    }
    return formatDate(day, month, year);
}

/** @brief Generate a Russian mobile number with a randomly chosen accepted spelling. */
std::string randomPhone(std::mt19937_64& engine) {
    std::uniform_int_distribution<int> digitDistribution(0, 9);
    std::string digits = "9";
    for (int i = 1; i < 10; ++i) digits.push_back(static_cast<char>('0' + digitDistribution(engine)));
    switch (std::uniform_int_distribution<int>(0, 3)(engine)) {
        case 0:
            return "+7 (" + digits.substr(0, 3) + ") " + digits.substr(3, 3) + "-" +
                   digits.substr(6, 2) + "-" + digits.substr(8, 2);
        case 1:
            return "8 " + digits.substr(0, 3) + " " + digits.substr(3, 3) + "-" +
                   digits.substr(6, 2) + "-" + digits.substr(8, 2);
        case 2:
            return "7" + digits;
        default:
            return digits;
    }
}

/** @brief Generate a synthetic email address on the reserved .test domain. */
std::string randomEmail(std::mt19937_64& engine) {
    std::uniform_int_distribution<unsigned> numberDistribution(0, 999999);
    std::ostringstream output;
    if (std::uniform_int_distribution<int>(0, 1)(engine) == 0) output << "User";
    else output << "user";
    output << std::setfill('0') << std::setw(6) << numberDistribution(engine) << "@example.test";
    return output.str();
}

} // namespace

std::vector<GeneratedUserRecord> RandomUserGenerator::generate(
    std::size_t count, std::optional<std::uint64_t> seed) const {
    if (count == 0) throw std::invalid_argument("Record count must be greater than zero");

    const auto actualSeed = seed ? *seed :
        static_cast<std::uint64_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count()) ^
        static_cast<std::uint64_t>(std::random_device{}());
    std::mt19937_64 engine(actualSeed);

    std::ifstream input(std::string(LAB2_DATA_DIR) + "/data.csv", std::ios::binary);
    if (!input) throw std::runtime_error("Cannot open FIO dictionary: " +
                                         std::string(LAB2_DATA_DIR) + "/data.csv");

    std::vector<DictionaryEntry> reservoir;
    reservoir.reserve(count);
    std::size_t validRows = 0;
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        DictionaryEntry entry;
        if (!parseDictionaryRow(line, entry)) continue;
        ++validRows;
        if (reservoir.size() < count) {
            reservoir.push_back(std::move(entry));
            continue;
        }
        std::uniform_int_distribution<std::size_t> select(0, validRows - 1);
        const auto index = select(engine);
        if (index < count) reservoir[index] = std::move(entry);
    }
    if (input.bad()) throw std::runtime_error("Error while reading FIO dictionary");
    if (validRows < count) throw std::runtime_error("FIO dictionary contains fewer records than requested");

    FormValidator validator;
    std::vector<GeneratedUserRecord> result;
    result.reserve(count);
    for (const auto& person : reservoir) {
        UserData raw;
        raw.surname = person.surname;
        raw.firstName = person.firstName;
        raw.patronymic = person.patronymic;
        raw.birthDate = randomBirthDate(engine);
        raw.phone = randomPhone(engine);
        raw.email = randomEmail(engine);

        const auto validation = validator.normalize(raw);
        if (!validation.normalized) {
            throw std::runtime_error("A sampled FIO record could not be normalized");
        }
        result.push_back({std::move(raw), *validation.normalized});
    }
    return result;
}
