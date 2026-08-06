/**
 * @file version.cpp
 * @brief Semantic Version implementation
 */

#include "core/plugin/version.h"
#include "liblogger/logger.h"
#include <algorithm>
#include <charconv>
#include <format>

namespace DearTs::Core::Plugin {

// ================ Version ================

Version::Version(uint32_t major, uint32_t minor, uint32_t patch,
                 std::string prerelease, std::string build)
    : major(major), minor(minor), patch(patch)
    , prerelease(std::move(prerelease))
    , build(std::move(build)) {
}

Result<Version, std::string> Version::parse(const std::string& version_str) {
    if (version_str.empty()) {
        return Result<Version, std::string>::err("Version string is empty");
    }

    Version version;

    // Parse main version (X.Y.Z)
    size_t pos = 0;

    // Parse MAJOR
    size_t dot_pos = version_str.find('.', pos);
    if (dot_pos == std::string::npos) {
        return Result<Version, std::string>::err(
            std::format("Invalid version format: '{}', expected X.Y.Z", version_str)
        );
    }

    auto major_str = version_str.substr(pos, dot_pos - pos);
    auto [major_ptr, major_ec] = std::from_chars(
        major_str.data(), major_str.data() + major_str.size(), version.major
    );
    if (major_ec != std::errc{} || major_ptr != major_str.data() + major_str.size()) {
        return Result<Version, std::string>::err(
            std::format("Invalid major version number: '{}'", major_str)
        );
    }

    pos = dot_pos + 1;

    // Parse MINOR
    dot_pos = version_str.find('.', pos);
    if (dot_pos == std::string::npos) {
        return Result<Version, std::string>::err(
            std::format("Invalid version format: '{}', expected X.Y.Z", version_str)
        );
    }

    auto minor_str = version_str.substr(pos, dot_pos - pos);
    auto [minor_ptr, minor_ec] = std::from_chars(
        minor_str.data(), minor_str.data() + minor_str.size(), version.minor
    );
    if (minor_ec != std::errc{} || minor_ptr != minor_str.data() + minor_str.size()) {
        return Result<Version, std::string>::err(
            std::format("Invalid minor version number: '{}'", minor_str)
        );
    }

    pos = dot_pos + 1;

    // Find end of PATCH (may be followed by -, +, or end of string)
    size_t patch_end = version_str.find_first_of("-+", pos);
    if (patch_end == std::string::npos) {
        patch_end = version_str.size();
    }

    auto patch_str = version_str.substr(pos, patch_end - pos);
    auto [patch_ptr, patch_ec] = std::from_chars(
        patch_str.data(), patch_str.data() + patch_str.size(), version.patch
    );
    if (patch_ec != std::errc{} || patch_ptr != patch_str.data() + patch_str.size()) {
        return Result<Version, std::string>::err(
            std::format("Invalid patch version number: '{}'", patch_str)
        );
    }

    pos = patch_end;

    // Parse PRERELEASE (after -)
    if (pos < version_str.size() && version_str[pos] == '-') {
        size_t prerelease_end = version_str.find('+', pos);
        if (prerelease_end == std::string::npos) {
            prerelease_end = version_str.size();
        }

        version.prerelease = version_str.substr(pos + 1, prerelease_end - pos - 1);

        // Validate prerelease identifiers (must be [0-9A-Za-z-]+)
        if (!version.prerelease.empty()) {
            for (char c : version.prerelease) {
                if (c != '.' && c != '-' &&
                    !std::isalnum(static_cast<unsigned char>(c))) {
                    return Result<Version, std::string>::err(
                        std::format("Invalid prerelease identifier: '{}'", version.prerelease)
                    );
                }
            }
        }

        pos = prerelease_end;
    }

    // Parse BUILD (after +)
    if (pos < version_str.size() && version_str[pos] == '+') {
        version.build = version_str.substr(pos + 1);
        pos = version_str.size();

        // Validate build identifiers (must be [0-9A-Za-z-]+)
        if (!version.build.empty()) {
            for (char c : version.build) {
                if (c != '.' && c != '-' &&
                    !std::isalnum(static_cast<unsigned char>(c))) {
                    return Result<Version, std::string>::err(
                        std::format("Invalid build metadata: '{}'", version.build)
                    );
                }
            }
        }
    }

    // Check for trailing characters
    if (pos < version_str.size()) {
        return Result<Version, std::string>::err(
            std::format("Unexpected trailing characters in version: '{}'", version_str)
        );
    }

    LOG_DEBUG("Parsed version: {} from '{}'", version.to_string(), version_str);
    return Result<Version, std::string>::ok(version);
}

std::string Version::to_string() const {
    std::string result = std::format("{}.{}.{}", major, minor, patch);

    if (!prerelease.empty()) {
        result += "-";
        result += prerelease;
    }

    if (!build.empty()) {
        result += "+";
        result += build;
    }

    return result;
}

std::string Version::to_spec_string() const {
    std::string result = std::format("{}.{}.{}", major, minor, patch);

    if (!prerelease.empty()) {
        result += "-";
        result += prerelease;
    }

    // Build metadata is excluded from spec string

    return result;
}

Result<std::vector<std::string>, std::string> Version::split_dotted_identifier(
    const std::string& str) {
    if (str.empty()) {
        return Result<std::vector<std::string>, std::string>::ok({});
    }

    std::vector<std::string> result;
    std::string current;

    for (char c : str) {
        if (c == '.') {
            if (!current.empty()) {
                result.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }

    if (!current.empty()) {
        result.push_back(current);
    }

    return Result<std::vector<std::string>, std::string>::ok(result);
}

int Version::compare_identifiers(
    const std::vector<std::string>& a,
    const std::vector<std::string>& b) {
    size_t max_len = std::max(a.size(), b.size());

    for (size_t i = 0; i < max_len; ++i) {
        // If one list is shorter, it has lower precedence
        if (i >= a.size()) {
            return -1;
        }
        if (i >= b.size()) {
            return 1;
        }

        const std::string& id_a = a[i];
        const std::string& id_b = b[i];

        // Check if both are numeric
        bool a_is_numeric = std::all_of(id_a.begin(), id_a.end(), ::isdigit);
        bool b_is_numeric = std::all_of(id_b.begin(), id_b.end(), ::isdigit);

        if (a_is_numeric && b_is_numeric) {
            // Numeric comparison
            uint64_t num_a = std::stoull(id_a);
            uint64_t num_b = std::stoull(id_b);

            if (num_a < num_b) return -1;
            if (num_a > num_b) return 1;
        } else if (a_is_numeric) {
            // Numeric identifiers have lower precedence than non-numeric
            return -1;
        } else if (b_is_numeric) {
            return 1;
        } else {
            // String comparison (case-sensitive)
            int cmp = id_a.compare(id_b);
            if (cmp != 0) {
                return cmp;
            }
        }
    }

    // Identical
    return 0;
}

// Note: operator<=> is auto-generated by compiler
// Build metadata is ignored by default implementation (as it should be)

} // namespace DearTs::Core::Plugin
