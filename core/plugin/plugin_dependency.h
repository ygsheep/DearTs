/**
 * @file plugin_dependency.h
 * @brief Plugin dependency specification
 * @details Defines dependency types and structures for plugin dependency management
 * @author DearTs Team
 * @date 2025
 * @version 1.0.0
 */

#pragma once

#include "core/plugin/version_range.h"
#include <string>
#include <vector>

namespace DearTs::Core::Plugin {

/**
 * @brief Dependency type
 * @details Defines how the plugin handles missing dependencies
 */
enum class DependencyType {
    Required,   ///< Plugin MUST load - error if dependency is missing or incompatible
    Optional,   ///< Plugin MAY load - warning if dependency is missing or incompatible
    Soft        ///< Plugin MAY enhance - silent if dependency is missing or incompatible
};

/**
 * @brief Plugin dependency specification
 * @details
 * Defines a single plugin dependency with version constraints.
 *
 * **Required**: The plugin cannot function without this dependency.
 * If the dependency is missing or has an incompatible version, the plugin will not load.
 *
 * **Optional**: The plugin can function without this dependency, but with reduced functionality.
 * A warning will be logged if the dependency is missing or incompatible.
 *
 * **Soft**: The plugin can enhance its functionality if this dependency is available.
 * No warning or error if the dependency is missing.
 *
 * @example
 * // Required dependency
 * auto dep1 = PluginDependency::required("Live2DPlugin", ">=1.0.0");
 *
 * // Optional dependency
 * auto dep2 = PluginDependency::optional("FFmpegPlugin", "^2.1.0");
 *
 * // Soft dependency
 * auto dep3 = PluginDependency::soft("AudioPlugin", "~1.2.0");
 */
struct PluginDependency {
    std::string plugin_name;      ///< Name of the required plugin
    VersionRange version_range;   ///< Version constraint
    DependencyType type;          ///< Dependency type

    /**
     * @brief Construct dependency
     */
    PluginDependency(std::string name, VersionRange range, DependencyType type)
        : plugin_name(std::move(name)), version_range(std::move(range)), type(type) {}

    /**
     * @brief Create a required dependency
     * @param name Plugin name
     * @param version_range Version range string (e.g., ">=1.0.0", "^1.2.3")
     * @return Required dependency or error if version_range is invalid
     */
    [[nodiscard]] static Result<PluginDependency, std::string> required(
        std::string name, std::string version_range);

    /**
     * @brief Create an optional dependency
     * @param name Plugin name
     * @param version_range Version range string (e.g., ">=1.0.0", "^1.2.3")
     * @return Optional dependency or error if version_range is invalid
     */
    [[nodiscard]] static Result<PluginDependency, std::string> optional(
        std::string name, std::string version_range);

    /**
     * @brief Create a soft dependency
     * @param name Plugin name
     * @param version_range Version range string (e.g., ">=1.0.0", "^1.2.3")
     * @return Soft dependency or error if version_range is invalid
     */
    [[nodiscard]] static Result<PluginDependency, std::string> soft(
        std::string name, std::string version_range);

    /**
     * @brief Convert dependency to string representation
     * @return String like "Live2DPlugin (>=1.0.0, Required)"
     */
    [[nodiscard]] std::string to_string() const;

    /**
     * @brief Check if dependency is satisfied by a given plugin version
     * @param version Available plugin version
     * @return true if version satisfies the version range
     */
    [[nodiscard]] bool is_satisfied_by(const Version& version) const {
        return version_range.satisfies(version);
    }
};

} // namespace DearTs::Core::Plugin
