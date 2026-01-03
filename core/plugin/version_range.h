/**
 * @file version_range.h
 * @brief Version range specification (npm-style)
 * @details Supports npm-style version ranges for dependency constraints
 * @author DearTs Team
 * @date 2025
 * @version 1.0.0
 * @see https://docs.npmjs.com/cli/v10/using-npm/semver
 */

#pragma once

#include "core/plugin/version.h"
#include "core/result.h"
#include <vector>
#include <string>

namespace DearTs::Core::Plugin {

/**
 * @brief Version comparison operators
 */
enum class ComparatorType {
    Exact,      // 1.2.3       (exact version match)
    Greater,    // >1.2.3      (greater than)
    GreaterEq,  // >=1.2.3     (greater or equal)
    Less,       // <1.2.3      (less than)
    LessEq,     // <=1.2.3     (less or equal)
    Tilde,      // ~1.2.3      (>=1.2.3 <1.3.0, patch-level updates)
    Caret,      // ^1.2.3      (>=1.2.3 <2.0.0, compatible changes)
    Hyphen,     // 1.2.3 - 2.3.4 (inclusive range)
    Wildcard    // 1.2.*       (>=1.2.0 <1.3.0)
};

/**
 * @brief Single comparator in version range
 */
struct Comparator {
    ComparatorType type;
    Version version;
    Version upper_bound;  // For Hyphen ranges

    /**
     * @brief Check if version satisfies this comparator
     */
    [[nodiscard]] bool satisfies(const Version& v) const;

    /**
     * @brief Convert comparator to string
     */
    [[nodiscard]] std::string to_string() const;
};

/**
 * @brief Version range specification
 * @details
 * Supports npm-style version ranges with multiple comparators.
 * All comparators must be satisfied (AND logic).
 *
 * Examples:
 *   - "1.2.3"           (exact version)
 *   - ">=1.2.3"         (greater or equal)
 *   - "^1.2.3"          (caret: compatible with version, >=1.2.3 <2.0.0)
 *   - "~1.2.3"          (tilde: patch-level updates, >=1.2.3 <1.3.0)
 *   - "1.2.3 - 2.3.4"   (inclusive range)
 *   - "1.2.*"           (wildcard, >=1.2.0 <1.3.0)
 *   - "1.x"             (wildcard, >=1.0.0 <2.0.0)
 *   - "*"               (any version)
 *   - ">=1.2.3 <2.0.0"  (multiple comparators)
 *
 * Precedence rules:
 *   1. Exact: Only accepts exact version
 *   2. Caret (^): Accepts compatible versions (leftmost non-zero component)
 *   3. Tilde (~): Accepts patch-level updates
 *   4. Wildcard (*): Accepts any version matching pattern
 *   5. Hyphen (-): Inclusive range
 *   6. Operators: >, >=, <, <=
 *
 * Examples:
 *   - ^1.2.3 accepts: 1.2.3, 1.2.4, 1.5.0, 1.9.9 (NOT 2.0.0, NOT 0.9.0)
 *   - ~1.2.3 accepts: 1.2.3, 1.2.4, 1.2.99 (NOT 1.3.0, NOT 2.0.0)
 *   - 1.2.* accepts: 1.2.0, 1.2.1, ..., 1.2.99 (NOT 1.3.0)
 *   - 1.x accepts: 1.0.0, 1.1.0, ..., 1.99.99 (NOT 2.0.0)
 */
class VersionRange {
public:
    /**
     * @brief Parse version range from string
     * @param range_str Range specification (e.g., "^1.2.3", ">=1.0.0 <2.0.0")
     * @return Parsed range or error message
     */
    [[nodiscard]] static Result<VersionRange, std::string> parse(const std::string& range_str);

    /**
     * @brief Check if version satisfies this range
     * @param version Version to check
     * @return true if version satisfies all comparators
     */
    [[nodiscard]] bool satisfies(const Version& version) const;

    /**
     * @brief Convert range to string representation
     */
    [[nodiscard]] std::string to_string() const;

    /**
     * @brief Check if range is valid (has comparators)
     */
    [[nodiscard]] bool is_valid() const {
        return !m_comparators.empty();
    }

    /**
     * @brief Check if range accepts any version (*)
     */
    [[nodiscard]] bool is_any() const {
        return m_comparators.size() == 1 &&
               m_comparators[0].type == ComparatorType::Wildcard &&
               m_comparators[0].version.major == 0;
    }

private:
    std::vector<Comparator> m_comparators;

    // Parse helpers
    [[nodiscard]] static Result<Comparator, std::string> parse_comparator(
        const std::string& cmp_str);

    [[nodiscard]] static Result<std::vector<Comparator>, std::string> parse_hyphen_range(
        const std::string& range);

    [[nodiscard]] static Result<Comparator, std::string> parse_wildcard(
        const std::string& wildcard_str);

    // Split range by spaces for multiple comparators
    [[nodiscard]] static std::vector<std::string> split_range(const std::string& range);

    // Trim whitespace
    [[nodiscard]] static std::string trim(const std::string& str);
};

} // namespace DearTs::Core::Plugin
