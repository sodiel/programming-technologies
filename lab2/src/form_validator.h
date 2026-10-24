#pragma once

#include <optional>
#include <string>
#include <vector>

/** Holds the six text fields used by the laboratory questionnaire. */
struct UserData {
    /** @brief Family name supplied by the user. */
    std::string surname;
    /** @brief Given name supplied by the user. */
    std::string firstName;
    /** @brief Patronymic supplied by the user. */
    std::string patronymic;
    /** @brief Birth date in a supported input format. */
    std::string birthDate;
    /** @brief Russian phone number in a supported input format. */
    std::string phone;
    /** @brief Email address. */
    std::string email;
};

/** Contains normalized fields or validation errors for one questionnaire entry. */
struct ValidationResult {
    /** @brief Present only if every field passed validation. */
    std::optional<UserData> normalized;
    /** @brief Human-readable errors for invalid fields. */
    std::vector<std::string> errors;
};

/** Validates questionnaire fields and converts accepted values to the lab format. */
class FormValidator {
public:
    /** Validate and normalize all fields independently.
     * @param input Raw questionnaire values.
     * @return Normalized record if every field is valid, otherwise field errors.
     */
    ValidationResult normalize(const UserData& input) const;
};
