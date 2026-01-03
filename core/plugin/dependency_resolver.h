/**
 * @file dependency_resolver.h
 * @brief Plugin dependency resolver
 * @details Handles plugin dependency resolution, load ordering, and validation
 * @author DearTs Team
 * @date 2025
 * @version 1.0.0
 */

#pragma once

#include "core/plugin/plugin.h"
#include "core/plugin/plugin_dependency.h"
#include "core/plugin/version.h"
#include "core/result.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>

namespace DearTs::Core::Plugin {

/**
 * @brief Dependency resolution error types
 */
enum class DependencyErrorType {
    MissingDependency,      ///< Required plugin not found
    VersionConflict,        ///< Version requirement not satisfied
    CircularDependency,     ///< Circular dependency detected
    InvalidVersionSpec      ///< Invalid version specification
};

/**
 * @brief Detailed dependency error information
 */
struct DependencyError {
    DependencyErrorType type;
    std::string plugin_name;
    std::string dependency_name;
    std::string details;                   ///< Human-readable error details
    std::vector<std::string> dependency_chain;  ///< For circular deps

    /**
     * @brief Convert error to string
     */
    [[nodiscard]] std::string to_string() const;
};

/**
 * @brief Load order entry
 */
struct LoadOrderEntry {
    std::string plugin_name;
    int load_order;  ///< Lower = load first
    std::vector<std::string> dependencies;
    PluginState target_state;  ///< Enabled, Loaded, or Disabled

    /**
     * @brief Convert entry to string
     */
    [[nodiscard]] std::string to_string() const;
};

/**
 * @brief Dependency resolution mode
 */
enum class DependencyResolutionMode {
    Lenient,    ///< Disable plugins with missing deps, continue loading
    Strict      ///< Stop loading on first dependency error
};

/**
 * @brief Dependency resolution result
 */
struct DependencyResolutionResult {
    bool success;
    std::vector<LoadOrderEntry> load_order;      ///< Calculated load order
    std::vector<DependencyError> errors;         ///< All errors found
    std::vector<std::string> disabled_plugins;   ///< Plugins disabled (lenient mode)
    std::vector<std::string> missing_plugins;    ///< Plugins that need to be loaded

    /**
     * @brief Convert result to summary string
     */
    [[nodiscard]] std::string to_string() const;
};

/**
 * @brief Dependency resolver
 * @details
 * Handles plugin dependency resolution with the following features:
 * - Topological sort for load ordering
 * - Circular dependency detection
 * - Version conflict detection
 * - Lenient/Strict mode support
 *
 * @example
 * auto result = DependencyResolver::resolve(plugins, DependencyResolutionMode::Lenient);
 * if (!result.success) {
 *     for (const auto& error : result.errors) {
 *         LOG_ERROR("Dependency error: {}", error.to_string());
 *     }
 * }
 */
class DependencyResolver {
public:
    /**
     * @brief Resolve dependencies for all plugins
     * @param available_plugins All available plugin instances (name -> plugin)
     * @param mode Resolution mode (lenient/strict)
     * @return Resolution result with load order and errors
     */
    [[nodiscard]] static DependencyResolutionResult resolve(
        const std::unordered_map<std::string, IPlugin*>& available_plugins,
        DependencyResolutionMode mode = DependencyResolutionMode::Lenient
    );

    /**
     * @brief Generate dependency graph visualization (for debugging)
     * @param plugins Available plugins
     * @return DOT format graph string (for Graphviz)
     *
     * @example
     * auto dot_graph = DependencyResolver::visualize_dependency_graph(plugins);
     * std::cout << dot_graph << std::endl;
     */
    [[nodiscard]] static std::string visualize_dependency_graph(
        const std::unordered_map<std::string, IPlugin*>& plugins
    );

private:
    /**
     * @brief Internal resolution context
     */
    struct ResolutionContext {
        const std::unordered_map<std::string, IPlugin*>& plugins;
        std::unordered_map<std::string, Version> plugin_versions;
        std::unordered_map<std::string, std::vector<PluginDependency>> dependency_map;
        std::unordered_set<std::string> visited;
        std::unordered_set<std::string> recursion_stack;
        std::vector<std::string> load_order;
        std::vector<DependencyError> errors;

        explicit ResolutionContext(
            const std::unordered_map<std::string, IPlugin*>& plugins
        );
    };

    // Algorithm steps
    static void build_dependency_map(ResolutionContext& ctx);
    static void detect_circular_dependencies(ResolutionContext& ctx);
    static void validate_version_constraints(ResolutionContext& ctx);
    static std::vector<std::string> topological_sort(ResolutionContext& ctx);
    static void filter_by_mode(
        ResolutionContext& ctx,
        DependencyResolutionMode mode,
        DependencyResolutionResult& result
    );

    // Helper for circular dependency detection
    static void detect_circular_recursive(
        ResolutionContext& ctx,
        const std::string& plugin_name,
        std::vector<std::string> path
    );
};

} // namespace DearTs::Core::Plugin
