/**
 * @file version_range.cpp
 * @brief Version range implementation
 */

#include "core/plugin/version_range.h"
#include "liblogger/logger.h"
#include <algorithm>
#include <cctype>
#include <format>

namespace DearTs::Core::Plugin {

// ================ Comparator ================

bool Comparator::satisfies(const Version& v) const {
    switch (type) {
        case ComparatorType::Exact:
            return v.major == version.major &&
                   v.minor == version.minor &&
                   v.patch == version.patch &&
                   v.prerelease == version.prerelease;  // Build ignored

        case ComparatorType::Greater:
            return v > version;

        case ComparatorType::GreaterEq:
            return v >= version;

        case ComparatorType::Less:
            return v < version;

        case ComparatorType::LessEq:
            return v <= version;

        case ComparatorType::Tilde: {
            // ~1.2.3 means >=1.2.3 <1.3.0
            Version min_version = version;
            Version max_version = Version{version.major, version.minor + 1, 0};

            return v >= min_version && v < max_version;
        }

        case ComparatorType::Caret: {
            // ^1.2.3 means >=1.2.3 <2.0.0
            // ^0.2.3 means >=0.2.3 <0.3.0
            // ^0.0.3 means >=0.0.3 <0.0.4

            Version max_version;

            if (version.major > 0) {
                // ^1.2.3 -> <2.0.0
                max_version = Version{version.major + 1, 0, 0};
            } else if (version.minor > 0) {
                // ^0.2.3 -> <0.3.0
                max_version = Version{0, version.minor + 1, 0};
            } else {
                // ^0.0.3 -> <0.0.4
                max_version = Version{0, 0, version.patch + 1};
            }

            return v >= version && v < max_version;
        }

        case ComparatorType::Hyphen: {
            // 1.2.3 - 2.3.4 means >=1.2.3 <=2.3.4
            return v >= version && v <= upper_bound;
        }

        case ComparatorType::Wildcard: {
            // 1.2.* means >=1.2.0 <1.3.0
            // 1.* means >=1.0.0 <2.0.0
            // * means anything

            if (version.major == 0 && version.minor == 0) {
                // * accepts anything
                return true;
            }

            Version max_version;

            if (version.minor == 0) {
                // 1.* -> <2.0.0
                max_version = Version{version.major + 1, 0, 0};
            } else {
                // 1.2.* -> <1.3.0
                max_version = Version{version.major, version.minor + 1, 0};
            }

            // For wildcard, we also check that the minimum version is met
            // e.g., 1.2.* should match 1.2.0 and above
            return v >= Version{version.major, version.minor, 0} && v < max_version;
        }

        default:
            return false;
    }
}

std::string Comparator::to_string() const {
    switch (type) {
        case ComparatorType::Exact:
            return version.to_spec_string();

        case ComparatorType::Greater:
            return std::format(">{}", version.to_spec_string());

        case ComparatorType::GreaterEq:
            return std::format(">={}", version.to_spec_string());

        case ComparatorType::Less:
            return std::format("<{}", version.to_spec_string());

        case ComparatorType::LessEq:
            return std::format("<={}", version.to_spec_string());

        case ComparatorType::Tilde:
            return std::format("~{}", version.to_spec_string());

        case ComparatorType::Caret:
            return std::format("^{}", version.to_spec_string());

        case ComparatorType::Hyphen:
            return std::format("{} - {}", version.to_spec_string(), upper_bound.to_spec_string());

        case ComparatorType::Wildcard:
            if (version.major == 0 && version.minor == 0) {
                return "*";
            } else if (version.minor == 0) {
                return std::format("{}.x", version.major);
            } else {
                return std::format("{}.*", version.to_spec_string());
            }

        default:
            return "?";
    }
}

// ================ VersionRange ================

Result<VersionRange, std::string> VersionRange::parse(const std::string& range_str) {
    if (range_str.empty()) {
        return Result<VersionRange, std::string>::err("Range string is empty");
    }

    VersionRange range;

    // Handle wildcard "*" (special case)
    std::string trimmed = trim(range_str);
    if (trimmed == "*") {
        Comparator cmp;
        cmp.type = ComparatorType::Wildcard;
        cmp.version = Version{0, 0, 0};
        range.m_comparators.push_back(cmp);
        return Result<VersionRange, std::string>::ok(range);
    }

    // Check for hyphen range (e.g., "1.2.3 - 2.3.4")
    size_t hyphen_pos = trimmed.find(" - ");
    if (hyphen_pos != std::string::npos) {
        auto hyphen_result = parse_hyphen_range(trimmed);
        if (hyphen_result.isErr()) {
            return Result<VersionRange, std::string>::err(hyphen_result.error());
        }
        range.m_comparators = hyphen_result.unwrap();
        return Result<VersionRange, std::string>::ok(range);
    }

    // Split by spaces for multiple comparators (e.g., ">=1.2.3 <2.0.0")
    auto parts = split_range(trimmed);

    for (const auto& part : parts) {
        auto cmp_result = parse_comparator(part);
        if (cmp_result.isErr()) {
            return Result<VersionRange, std::string>::err(cmp_result.error());
        }
        range.m_comparators.push_back(cmp_result.unwrap());
    }

    if (range.m_comparators.empty()) {
        return Result<VersionRange, std::string>::err(
            std::format("Failed to parse range: '{}'", range_str)
        );
    }

    LOG_DEBUG("Parsed version range: {} from '{}'", range.to_string(), range_str);
    return Result<VersionRange, std::string>::ok(range);
}

bool VersionRange::satisfies(const Version& version) const {
    // All comparators must be satisfied (AND logic)
    for (const auto& cmp : m_comparators) {
        if (!cmp.satisfies(version)) {
            return false;
        }
    }
    return true;
}

std::string VersionRange::to_string() const {
    if (m_comparators.empty()) {
        return "";
    }

    if (m_comparators.size() == 1) {
        return m_comparators[0].to_string();
    }

    // Multiple comparators: join with spaces
    std::string result;
    for (size_t i = 0; i < m_comparators.size(); ++i) {
        if (i > 0) {
            result += " ";
        }
        result += m_comparators[i].to_string();
    }
    return result;
}

Result<Comparator, std::string> VersionRange::parse_comparator(const std::string& cmp_str) {
    if (cmp_str.empty()) {
        return Result<Comparator, std::string>::err("Comparator string is empty");
    }

    std::string trimmed = trim(cmp_str);
    Comparator cmp;

    // Check for operator prefix
    ComparatorType type = ComparatorType::Exact;
    size_t version_start = 0;

    if (trimmed[0] == '>') {
        if (trimmed.size() > 1 && trimmed[1] == '=') {
            type = ComparatorType::GreaterEq;
            version_start = 2;
        } else {
            type = ComparatorType::Greater;
            version_start = 1;
        }
    } else if (trimmed[0] == '<') {
        if (trimmed.size() > 1 && trimmed[1] == '=') {
            type = ComparatorType::LessEq;
            version_start = 2;
        } else {
            type = ComparatorType::Less;
            version_start = 1;
        }
    } else if (trimmed[0] == '~') {
        type = ComparatorType::Tilde;
        version_start = 1;
    } else if (trimmed[0] == '^') {
        type = ComparatorType::Caret;
        version_start = 1;
    } else if (trimmed[0] == '*') {
        type = ComparatorType::Wildcard;
        cmp.type = type;
        cmp.version = Version{0, 0, 0};
        return Result<Comparator, std::string>::ok(cmp);
    }

    // Check for wildcard in version part (e.g., "1.2.*", "1.x")
    bool has_wildcard = false;
    size_t wildcard_pos = trimmed.find('*', version_start);
    if (wildcard_pos == std::string::npos) {
        wildcard_pos = trimmed.find('x', version_start);
    }
    if (wildcard_pos != std::string::npos) {
        // Parse wildcard version
        auto wildcard_result = parse_wildcard(trimmed);
        if (wildcard_result.isErr()) {
            return Result<Comparator, std::string>::err(wildcard_result.error());
        }
        return wildcard_result;
    }

    // Parse version
    std::string version_str = trimmed.substr(version_start);
    auto version_result = Version::parse(version_str);
    if (version_result.isErr()) {
        return Result<Comparator, std::string>::err(
            std::format("Failed to parse version in comparator '{}': {}",
                       cmp_str, version_result.error())
        );
    }

    // If no operator was found and it's not a wildcard, it's an exact match
    if (version_start == 0) {
        type = ComparatorType::Exact;
    }

    cmp.type = type;
    cmp.version = version_result.unwrap();

    return Result<Comparator, std::string>::ok(cmp);
}

Result<std::vector<Comparator>, std::string> VersionRange::parse_hyphen_range(
    const std::string& range) {
    // Format: "1.2.3 - 2.3.4"
    size_t hyphen_pos = range.find(" - ");
    if (hyphen_pos == std::string::npos) {
        return Result<std::vector<Comparator>, std::string>::err(
            std::format("Invalid hyphen range: '{}'", range)
        );
    }

    std::string left_str = trim(range.substr(0, hyphen_pos));
    std::string right_str = trim(range.substr(hyphen_pos + 3));

    auto left_result = Version::parse(left_str);
    if (left_result.isErr()) {
        return Result<std::vector<Comparator>, std::string>::err(
            std::format("Failed to parse left version in hyphen range: {}", left_result.error())
        );
    }

    auto right_result = Version::parse(right_str);
    if (right_result.isErr()) {
        return Result<std::vector<Comparator>, std::string>::err(
            std::format("Failed to parse right version in hyphen range: {}", right_result.error())
        );
    }

    // Create a single Hyphen comparator
    std::vector<Comparator> comparators;
    Comparator cmp;
    cmp.type = ComparatorType::Hyphen;
    cmp.version = left_result.unwrap();
    cmp.upper_bound = right_result.unwrap();
    comparators.push_back(cmp);

    return Result<std::vector<Comparator>, std::string>::ok(comparators);
}

Result<Comparator, std::string> VersionRange::parse_wildcard(const std::string& wildcard_str) {
    // Formats: "1.2.*", "1.x", "*"
    std::string trimmed = trim(wildcard_str);

    if (trimmed == "*" || trimmed == "x") {
        Comparator cmp;
        cmp.type = ComparatorType::Wildcard;
        cmp.version = Version{0, 0, 0};
        return Result<Comparator, std::string>::ok(cmp);
    }

    // Parse major.minor.*
    size_t dot_count = std::count(trimmed.begin(), trimmed.end(), '.');

    if (dot_count == 0) {
        // Format: "1.x" or "1.*"
        auto version_result = Version::parse(trimmed.substr(0, 1) + ".0.0");
        if (version_result.isErr()) {
            return Result<Comparator, std::string>::err(
                std::format("Failed to parse wildcard version: '{}'", wildcard_str)
            );
        }

        auto v = version_result.unwrap();

        Comparator cmp;
        cmp.type = ComparatorType::Wildcard;
        cmp.version = Version{v.major, 0, 0};
        return Result<Comparator, std::string>::ok(cmp);
    }

    if (dot_count == 1) {
        // Format: "1.2.x" or "1.2.*"
        size_t first_dot = trimmed.find('.');
        auto major_result = Version::parse(trimmed.substr(0, first_dot) + ".0.0");
        if (major_result.isErr()) {
            return Result<Comparator, std::string>::err(
                std::format("Failed to parse wildcard version: '{}'", wildcard_str)
            );
        }

        auto minor_str = trimmed.substr(first_dot + 1);
        size_t x_pos = minor_str.find_first_of("xX*");
        if (x_pos == std::string::npos) {
            return Result<Comparator, std::string>::err(
                std::format("Invalid wildcard format: '{}'", wildcard_str)
            );
        }

        auto minor_num_str = minor_str.substr(0, x_pos);
        uint32_t minor_num = 0;
        auto [ptr, ec] = std::from_chars(
            minor_num_str.data(), minor_num_str.data() + minor_num_str.size(), minor_num
        );
        if (ec != std::errc{}) {
            return Result<Comparator, std::string>::err(
                std::format("Failed to parse minor version in wildcard: '{}'", wildcard_str)
            );
        }

        Comparator cmp;
        cmp.type = ComparatorType::Wildcard;
        cmp.version = Version{major_result.unwrap().major, minor_num, 0};
        return Result<Comparator, std::string>::ok(cmp);
    }

    if (dot_count == 2) {
        // Format: "1.2.3" with wildcard (shouldn't happen here)
        return Result<Comparator, std::string>::err(
            std::format("Invalid wildcard format (too many dots): '{}'", wildcard_str)
        );
    }

    return Result<Comparator, std::string>::err(
        std::format("Invalid wildcard format: '{}'", wildcard_str)
    );
}

std::vector<std::string> VersionRange::split_range(const std::string& range) {
    std::vector<std::string> parts;
    std::string current;
    bool in_quotes = false;

    for (char c : range) {
        if (c == '"') {
            in_quotes = !in_quotes;
        } else if (std::isspace(static_cast<unsigned char>(c)) && !in_quotes) {
            if (!current.empty()) {
                parts.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }

    if (!current.empty()) {
        parts.push_back(current);
    }

    return parts;
}

std::string VersionRange::trim(const std::string& str) {
    size_t start = 0;
    while (start < str.size() && std::isspace(static_cast<unsigned char>(str[start]))) {
        ++start;
    }

    if (start == str.size()) {
        return "";
    }

    size_t end = str.size() - 1;
    while (end > start && std::isspace(static_cast<unsigned char>(str[end]))) {
        --end;
    }

    return str.substr(start, end - start + 1);
}

} // namespace DearTs::Core::Plugin
