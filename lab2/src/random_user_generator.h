#pragma once

#include "form_validator.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

/** Holds one generated raw questionnaire record and its normalized result. */
struct GeneratedUserRecord {
    /** @brief Values before validation and normalization. */
    UserData raw;
    /** @brief Canonical values returned by the form validator. */
    UserData normalized;
};

/** Samples complete Russian FIO records and generates valid form fields. */
class RandomUserGenerator {
public:
    /** Generate records using reservoir sampling over the configured CSV dictionary.
     * @param count Number of records requested.
     * @param seed Optional deterministic seed for repeatable output.
     * @return Raw and normalized forms in generated order.
     * @throws std::runtime_error if the dictionary cannot be read or has too few entries.
     */
    std::vector<GeneratedUserRecord> generate(
        std::size_t count,
        std::optional<std::uint64_t> seed = std::nullopt) const;
};
