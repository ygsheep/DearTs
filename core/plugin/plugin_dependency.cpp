/**
 * @file plugin_dependency.cpp
 * @brief Plugin dependency implementation
 */

#include "core/plugin/plugin_dependency.h"
#include "liblogger/logger.h"
#include <format>

namespace DearTs::Core::Plugin {

// ================ PluginDependency ================

Result<PluginDependency, std::string> PluginDependency::required(
    std::string name, std::string version_range) {
    auto range_result = VersionRange::parse(version_range);
    if (range_result.isErr()) {
        return Result<PluginDependency, std::string>::err(
            std::format("Failed to parse version range for required dependency '{}': {}",
                       name, range_result.error())
        );
    }

    PluginDependency dep{
        std::move(name),
        range_result.unwrap(),
        DependencyType::Required
    };

    LOG_DEBUG("Created required dependency: {}", dep.to_string());
    return Result<PluginDependency, std::string>::ok(dep);
}

Result<PluginDependency, std::string> PluginDependency::optional(
    std::string name, std::string version_range) {
    auto range_result = VersionRange::parse(version_range);
    if (range_result.isErr()) {
        return Result<PluginDependency, std::string>::err(
            std::format("Failed to parse version range for optional dependency '{}': {}",
                       name, range_result.error())
        );
    }

    PluginDependency dep{
        std::move(name),
        range_result.unwrap(),
        DependencyType::Optional
    };

    LOG_DEBUG("Created optional dependency: {}", dep.to_string());
    return Result<PluginDependency, std::string>::ok(dep);
}

Result<PluginDependency, std::string> PluginDependency::soft(
    std::string name, std::string version_range) {
    auto range_result = VersionRange::parse(version_range);
    if (range_result.isErr()) {
        return Result<PluginDependency, std::string>::err(
            std::format("Failed to parse version range for soft dependency '{}': {}",
                       name, range_result.error())
        );
    }

    PluginDependency dep{
        std::move(name),
        range_result.unwrap(),
        DependencyType::Soft
    };

    LOG_DEBUG("Created soft dependency: {}", dep.to_string());
    return Result<PluginDependency, std::string>::ok(dep);
}

std::string PluginDependency::to_string() const {
    std::string type_str;
    switch (type) {
        case DependencyType::Required:
            type_str = "Required";
            break;
        case DependencyType::Optional:
            type_str = "Optional";
            break;
        case DependencyType::Soft:
            type_str = "Soft";
            break;
    }

    return std::format("{} ({} {})", plugin_name, version_range.to_string(), type_str);
}

} // namespace DearTs::Core::Plugin
