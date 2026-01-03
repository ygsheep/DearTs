/**
 * @file version.h
 * @brief Semantic Version (SemVer 2.0.0)
 * @details Implements semantic versioning with major.minor.patch, prerelease, and build metadata
 * @author DearTs Team
 * @date 2025
 * @version 1.0.0
 * @see https://semver.org/
 */

#pragma once

#include "core/result.h"
#include <string>
#include <vector>
#include <cstdint>
#include <compare>

namespace DearTs::Core::Plugin {

/**
 * @brief Semantic Version (SemVer 2.0.0)
 * @details
 * Format: MAJOR.MINOR.PATCH-PRERELEASE+BUILD
 *
 * - MAJOR: Incompatible API changes
 * - MINOR: Backwards-compatible functionality additions
 * - PATCH: Backwards-compatible bug fixes
 * - PRERELEASE: Hyphen followed by identifiers (e.g., "alpha", "beta.1")
 * - BUILD: Plus followed by identifiers (e.g., "20130313144700")
 *
 * Examples:
 *   - "1.2.3"
 *   - "1.2.3-alpha"
 *   - "1.2.3-beta.1+build123"
 *   - "2.0.0-rc.1+exp.sha.5114f85"
 *
 * Comparison rules:
 *   1. Compare MAJOR, MINOR, PATCH numerically
 *   2. Pre-release versions have lower precedence than normal versions
 *   3. Pre-release identifiers compared dot-separated: numeric > numeric, alphabetic > alphabetic
 *   4. Build metadata does not affect precedence
 */
class Version {
public:
    uint32_t major = 0;      ///< Major version (incompatible changes)
    uint32_t minor = 0;      ///< Minor version (backwards-compatible features)
    uint32_t patch = 0;      ///< Patch version (bug fixes)
    std::string prerelease;  ///< Pre-release identifiers (e.g., "alpha.1")
    std::string build;       ///< Build metadata (ignored in comparisons)

    /**
     * @brief Default constructor (version 0.0.0)
     */
    constexpr Version() = default;

    /**
     * @brief Construct version with major, minor, patch
     */
    constexpr Version(uint32_t major, uint32_t minor, uint32_t patch)
        : major(major), minor(minor), patch(patch) {}

    /**
     * @brief Construct version with all components
     */
    Version(uint32_t major, uint32_t minor, uint32_t patch,
            std::string prerelease, std::string build);

    /**
     * @brief Parse version from string
     * @param version_str Version string (e.g., "1.2.3-alpha.1+build123")
     * @return Parsed version or error message
     *
     * @examples
     * Version::parse("1.2.3")              // OK: 1.2.3
     * Version::parse("1.2.3-alpha")        // OK: 1.2.3-alpha
     * Version::parse("1.2.3+build")        // OK: 1.2.3+build
     * Version::parse("1.2.3-beta.1+build") // OK: 1.2.3-beta.1+build
     * Version::parse("invalid")            // Error
     */
    [[nodiscard]] static Result<Version, std::string> parse(const std::string& version_str);

    /**
     * @brief Convert to string representation
     * @return Version string with all components (e.g., "1.2.3-alpha.1+build123")
     */
    [[nodiscard]] std::string to_string() const;

    /**
     * @brief Convert to spec string (without build metadata)
     * @return Version string for specification (e.g., "1.2.3-alpha.1")
     */
    [[nodiscard]] std::string to_spec_string() const;

    /**
     * @brief Check if version is valid (non-zero or has prerelease)
     */
    [[nodiscard]] bool is_valid() const {
        return major > 0 || minor > 0 || patch > 0 || !prerelease.empty();
    }

    /**
     * @brief Check if version is a pre-release
     */
    [[nodiscard]] bool is_prerelease() const {
        return !prerelease.empty();
    }

    /**
     * @brief Three-way comparison operator (C++20)
     * @details Build metadata is ignored in comparisons
     */
    [[nodiscard]] auto operator<=>(const Version& other) const = default;

private:
    /**
     * @brief Split dotted identifiers (e.g., "alpha.1" -> ["alpha", "1"])
     */
    [[nodiscard]] static Result<std::vector<std::string>, std::string> split_dotted_identifier(
        const std::string& str);

    /**
     * @brief Compare dotted identifiers
     * @return -1 if a < b, 0 if a == b, 1 if a > b
     */
    [[nodiscard]] static int compare_identifiers(
        const std::vector<std::string>& a,
        const std::vector<std::string>& b);
};

} // namespace DearTs::Core::Plugin
