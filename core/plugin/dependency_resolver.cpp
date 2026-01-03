/**
 * @file dependency_resolver.cpp
 * @brief Plugin dependency resolver implementation
 */

#include "core/plugin/dependency_resolver.h"
#include "core/plugin/version.h"
#include "liblogger/logger.h"
#include <algorithm>
#include <format>
#include <queue>

namespace DearTs::Core::Plugin {

// ================ DependencyError ================

std::string DependencyError::to_string() const {
    std::string result = std::format("{}: {} -> {}", type, plugin_name, dependency_name);

    if (!details.empty()) {
        result += std::format("\n  Details: {}", details);
    }

    if (!dependency_chain.empty()) {
        result += "\n  Dependency chain:";
        for (const auto& name : dependency_chain) {
            result += std::format("\n    -> {}", name);
        }
    }

    return result;
}

// ================ LoadOrderEntry ================

std::string LoadOrderEntry::to_string() const {
    std::string deps_str;
    for (size_t i = 0; i < dependencies.size(); ++i) {
        if (i > 0) deps_str += ", ";
        deps_str += dependencies[i];
    }

    std::string state_str;
    switch (target_state) {
        case PluginState::Enabled:  state_str = "Enabled";  break;
        case PluginState::Loaded:   state_str = "Loaded";   break;
        case PluginState::Disabled: state_str = "Disabled"; break;
        default:                    state_str = "Unknown";  break;
    }

    return std::format("{}. {} ({}) [deps: {}]",
                       load_order, plugin_name, state_str, deps_str);
}

// ================ DependencyResolutionResult ================

std::string DependencyResolutionResult::to_string() const {
    std::string result = std::format("Success: {}\n", success);

    result += std::format("Load Order ({} plugins):\n", load_order.size());
    for (const auto& entry : load_order) {
        result += std::format("  {}\n", entry.to_string());
    }

    if (!errors.empty()) {
        result += std::format("\nErrors ({}):\n", errors.size());
        for (const auto& error : errors) {
            result += std::format("  - {}\n", error.to_string());
        }
    }

    if (!disabled_plugins.empty()) {
        result += std::format("\nDisabled Plugins ({}):\n", disabled_plugins.size());
        for (const auto& name : disabled_plugins) {
            result += std::format("  - {}\n", name);
        }
    }

    if (!missing_plugins.empty()) {
        result += std::format("\nMissing Plugins ({}):\n", missing_plugins.size());
        for (const auto& name : missing_plugins) {
            result += std::format("  - {}\n", name);
        }
    }

    return result;
}

// ================ ResolutionContext ================

DependencyResolver::ResolutionContext::ResolutionContext(
    const std::unordered_map<std::string, IPlugin*>& plugins)
    : plugins(plugins) {
    // Extract plugin versions
    for (const auto& [name, plugin] : plugins) {
        auto info = plugin->get_info();
        auto version_result = Version::parse(info.version);
        if (version_result.isOk()) {
            plugin_versions[name] = version_result.unwrap();
        } else {
            plugin_versions[name] = Version{0, 0, 0};  // Fallback
        }
    }
}

// ================ DependencyResolver ================

DependencyResolutionResult DependencyResolver::resolve(
    const std::unordered_map<std::string, IPlugin*>& available_plugins,
    DependencyResolutionMode mode) {

    LOG_INFO("Resolving dependencies for {} plugins", available_plugins.size());

    DependencyResolutionResult result;
    result.success = true;

    // Early exit if no plugins
    if (available_plugins.empty()) {
        return result;
    }

    // Build resolution context
    ResolutionContext ctx(available_plugins);

    // Step 1: Build dependency map
    build_dependency_map(ctx);

    // Step 2: Detect circular dependencies
    detect_circular_dependencies(ctx);

    // Step 3: Validate version constraints
    validate_version_constraints(ctx);

    // Step 4: Topological sort
    auto sorted_plugins = topological_sort(ctx);

    // Step 5: Filter by mode and build load order
    filter_by_mode(ctx, mode, result);

    // Determine overall success
    result.success = (mode == DependencyResolutionMode::Lenient) ||
                     (result.errors.empty() && result.disabled_plugins.empty());

    LOG_INFO("Dependency resolution complete: success={}, errors={}, disabled={}",
             result.success, result.errors.size(), result.disabled_plugins.size());

    return result;
}

std::string DependencyResolver::visualize_dependency_graph(
    const std::unordered_map<std::string, IPlugin*>& plugins) {

    std::string dot_graph = "digraph PluginDependencies {\n";
    dot_graph += "  rankdir=BT;\n";
    dot_graph += "  node [shape=box, style=rounded];\n\n";

    // Add nodes
    for (const auto& [name, plugin] : plugins) {
        auto info = plugin->get_info();
        dot_graph += std::format("  \"{}\" [label=\"{}\\n{}\"];\n",
                                 name, name, info.version);
    }

    dot_graph += "\n";

    // Add edges
    for (const auto& [name, plugin] : plugins) {
        auto deps = plugin->get_dependencies();
        for (const auto& dep : deps) {
            std::string style = "";
            if (dep.type == DependencyType::Optional) {
                style = " [style=dashed, label=\"optional\"]";
            } else if (dep.type == DependencyType::Soft) {
                style = " [style=dotted, label=\"soft\"]";
            } else {
                style = std::format(" [label=\"{}\"]", dep.version_range.to_string());
            }

            dot_graph += std::format("  \"{}\" -> \"{}\"{};\n",
                                     name, dep.plugin_name, style);
        }
    }

    dot_graph += "}\n";

    return dot_graph;
}

void DependencyResolver::build_dependency_map(ResolutionContext& ctx) {
    LOG_DEBUG("Building dependency map...");

    for (const auto& [name, plugin] : ctx.plugins) {
        auto deps = plugin->get_dependencies();
        ctx.dependency_map[name] = deps;

        LOG_DEBUG("Plugin '{}' has {} dependencies", name, deps.size());
        for (const auto& dep : deps) {
            LOG_DEBUG("  - {} ({})", dep.plugin_name, dep.to_string());
        }
    }
}

void DependencyResolver::detect_circular_dependencies(ResolutionContext& ctx) {
    LOG_DEBUG("Detecting circular dependencies...");

    ctx.visited.clear();
    ctx.recursion_stack.clear();

    for (const auto& [plugin_name, _] : ctx.plugins) {
        if (!ctx.visited.contains(plugin_name)) {
            detect_circular_recursive(ctx, plugin_name, {});
        }
    }

    LOG_DEBUG("Found {} circular dependencies", ctx.errors.size());
}

void DependencyResolver::detect_circular_recursive(
    ResolutionContext& ctx,
    const std::string& plugin_name,
    std::vector<std::string> path) {

    ctx.visited.insert(plugin_name);
    ctx.recursion_stack.insert(plugin_name);
    path.push_back(plugin_name);

    if (!ctx.dependency_map.contains(plugin_name)) {
        ctx.recursion_stack.erase(plugin_name);
        path.pop_back();
        return;
    }

    for (const auto& dep : ctx.dependency_map[plugin_name]) {
        if (!ctx.plugins.contains(dep.plugin_name)) {
            continue;  // Missing dependency handled elsewhere
        }

        if (!ctx.visited.contains(dep.plugin_name)) {
            detect_circular_recursive(ctx, dep.plugin_name, path);
        } else if (ctx.recursion_stack.contains(dep.plugin_name)) {
            // Found circular dependency
            std::vector<std::string> chain = path;
            chain.push_back(dep.plugin_name);

            ctx.errors.push_back(DependencyError{
                .type = DependencyErrorType::CircularDependency,
                .plugin_name = plugin_name,
                .dependency_name = dep.plugin_name,
                .details = std::format(
                    "Circular dependency detected: {} -> {}",
                    plugin_name, dep.plugin_name
                ),
                .dependency_chain = chain
            });

            LOG_WARN("Circular dependency detected: {} -> {}",
                     plugin_name, dep.plugin_name);
        }
    }

    ctx.recursion_stack.erase(plugin_name);
    path.pop_back();
}

void DependencyResolver::validate_version_constraints(ResolutionContext& ctx) {
    LOG_DEBUG("Validating version constraints...");

    for (const auto& [plugin_name, deps] : ctx.dependency_map) {
        for (const auto& dep : deps) {
            if (!ctx.plugin_versions.contains(dep.plugin_name)) {
                // Missing dependency - add error for required deps
                if (dep.type == DependencyType::Required) {
                    ctx.errors.push_back(DependencyError{
                        .type = DependencyErrorType::MissingDependency,
                        .plugin_name = plugin_name,
                        .dependency_name = dep.plugin_name,
                        .details = std::format(
                            "Required dependency '{}' not found",
                            dep.plugin_name
                        )
                    });

                    LOG_WARN("Missing required dependency: {} needs {}",
                             plugin_name, dep.plugin_name);
                }
                continue;
            }

            Version available_version = ctx.plugin_versions[dep.plugin_name];

            if (!dep.is_satisfied_by(available_version)) {
                ctx.errors.push_back(DependencyError{
                    .type = DependencyErrorType::VersionConflict,
                    .plugin_name = plugin_name,
                    .dependency_name = dep.plugin_name,
                    .details = std::format(
                        "Version conflict: {} requires {}, but {} is available",
                        plugin_name,
                        dep.version_range.to_string(),
                        available_version.to_string()
                    )
                });

                LOG_WARN("Version conflict: {} needs {} but {} is available",
                         plugin_name, dep.version_range.to_string(), available_version.to_string());
            }
        }
    }

    LOG_DEBUG("Found {} version conflicts", ctx.errors.size());
}

std::vector<std::string> DependencyResolver::topological_sort(ResolutionContext& ctx) {
    LOG_DEBUG("Performing topological sort...");

    std::vector<std::string> result;
    std::unordered_set<std::string> visited;
    std::unordered_set<std::string> temp_visited;

    std::function<void(const std::string&)> visit = [&](const std::string& name) {
        if (temp_visited.contains(name)) {
            return;  // Skip circular deps (already reported)
        }
        if (visited.contains(name)) {
            return;
        }

        temp_visited.insert(name);

        // Visit dependencies first
        if (ctx.dependency_map.contains(name)) {
            for (const auto& dep : ctx.dependency_map[name]) {
                if (ctx.plugins.contains(dep.plugin_name)) {
                    visit(dep.plugin_name);
                }
            }
        }

        temp_visited.erase(name);
        visited.insert(name);
        result.push_back(name);  // Add after dependencies (post-order)
    };

    // Visit all plugins
    for (const auto& [name, _] : ctx.plugins) {
        if (!visited.contains(name)) {
            visit(name);
        }
    }

    LOG_DEBUG("Topological sort complete: {} plugins", result.size());

    // Reverse to get dependencies first
    std::reverse(result.begin(), result.end());
    return result;
}

void DependencyResolver::filter_by_mode(
    ResolutionContext& ctx,
    DependencyResolutionMode mode,
    DependencyResolutionResult& result) {

    LOG_DEBUG("Filtering by mode: {}", (mode == DependencyResolutionMode::Lenient) ? "Lenient" : "Strict");

    // Get load order from topological sort
    auto sorted_plugins = topological_sort(ctx);

    // Build load order entries
    int order = 0;
    for (const auto& plugin_name : sorted_plugins) {
        LoadOrderEntry entry;
        entry.plugin_name = plugin_name;
        entry.load_order = order++;
        entry.target_state = PluginState::Enabled;

        // Collect dependencies
        if (ctx.dependency_map.contains(plugin_name)) {
            for (const auto& dep : ctx.dependency_map[plugin_name]) {
                entry.dependencies.push_back(dep.plugin_name);
            }
        }

        // Check if plugin has errors
        bool has_errors = false;
        for (const auto& error : ctx.errors) {
            if (error.plugin_name == plugin_name) {
                // For lenient mode, disable on errors
                // For strict mode, this will be caught by success check
                if (mode == DependencyResolutionMode::Lenient) {
                    if (error.type == DependencyErrorType::CircularDependency ||
                        (error.type == DependencyErrorType::MissingDependency &&
                         std::any_of(ctx.dependency_map[plugin_name].begin(),
                                     ctx.dependency_map[plugin_name].end(),
                                     [&](const PluginDependency& d) {
                                         return d.plugin_name == error.dependency_name &&
                                                d.type == DependencyType::Required;
                                     }))) {
                        entry.target_state = PluginState::Disabled;
                        result.disabled_plugins.push_back(plugin_name);
                        has_errors = true;
                    }
                }
            }
        }

        result.load_order.push_back(entry);
    }

    // Copy all errors
    result.errors = ctx.errors;

    // Collect missing plugins
    std::unordered_set<std::string> available_plugins;
    for (const auto& [name, _] : ctx.plugins) {
        available_plugins.insert(name);
    }

    for (const auto& [name, deps] : ctx.dependency_map) {
        for (const auto& dep : deps) {
            if (!available_plugins.contains(dep.plugin_name)) {
                result.missing_plugins.push_back(dep.plugin_name);
            }
        }
    }

    LOG_DEBUG("Filtering complete: {} plugins to load, {} disabled, {} missing",
              result.load_order.size(), result.disabled_plugins.size(), result.missing_plugins.size());
}

} // namespace DearTs::Core::Plugin
