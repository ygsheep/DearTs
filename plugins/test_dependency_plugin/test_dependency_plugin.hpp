/**
 * @file test_dependency_plugin.hpp
 * @brief 测试依赖插件 - 用于验证插件依赖管理功能
 * @details 这个插件声明了各种依赖关系，用于测试依赖解析系统
 */

#pragma once

#include "core/plugin/plugin.h"
#include "core/plugin/plugin_dependency.h"
#include "core/content/commands.h"
#include "liblogger/logger.h"

using namespace DearTs;
using namespace DearTs::Core;

namespace TestDependencyPlugin {

/**
 * @brief 测试依赖插件类
 * @details
 * 这个插件展示了如何声明各种类型的依赖：
 * - Required: 必需的依赖（缺失时错误）
 * - Optional: 可选的依赖（缺失时警告）
 * - Soft: 软依赖（缺失时静默）
 */
class TestDependencyPlugin : public DearTs::Core::Plugin::IPlugin {
public:
    /**
     * @brief 获取插件信息
     */
    [[nodiscard]] Plugin::PluginInfo get_info() const override {
        return Plugin::PluginInfo{
            .name = "TestDependencyPlugin",
            .author = "DearTs Team",
            .description = "Test plugin with dependencies for dependency management system",
            .version = "1.0.0",
            .api_version = "1.0.0"
        };
    }

    /**
     * @brief 声明插件依赖
     * @details
     * 依赖列表：
     * 1. Builtin (Required) - 内置插件总是存在
     * 2. TestPlugin (Optional) - 测试插件，可能不存在
     * 3. Live2D (Soft) - Live2D 插件，可能不存在
     */
    [[nodiscard]] std::vector<Plugin::PluginDependency> get_dependencies() const override {
        // 使用工厂方法创建依赖
        std::vector<Plugin::PluginDependency> deps;

        // 必需依赖：Builtin（总是存在）
        auto builtin_dep = Plugin::PluginDependency::required("Builtin", ">=1.0.0");
        if (builtin_dep.isOk()) {
            deps.push_back(builtin_dep.unwrap());
        }

        // 可选依赖：TestPlugin（可能不存在）
        auto test_dep = Plugin::PluginDependency::optional("TestPlugin", "^1.0.0");
        if (test_dep.isOk()) {
            deps.push_back(test_dep.unwrap());
        }

        // 软依赖：Live2D（可能不存在）
        auto live2d_dep = Plugin::PluginDependency::soft("Live2D", ">=1.0.0");
        if (live2d_dep.isOk()) {
            deps.push_back(live2d_dep.unwrap());
        }

        return deps;
    }

    /**
     * @brief 插件加载时调用
     */
    Result<void, std::string> on_load() override {
        LOG_INFO("TestDependencyPlugin: on_load() called");
        LOG_INFO("TestDependencyPlugin: All dependencies satisfied!");

        // 注意：由于 DLL 边界问题，动态插件无法直接向主应用的命令注册表注册命令
        // 这个插件主要用于测试依赖管理功能，不注册命令
        // 依赖检查结果已通过日志输出

        LOG_INFO("TestDependencyPlugin: Plugin loaded successfully");
        LOG_INFO("TestDependencyPlugin: Dependencies:");
        LOG_INFO("  - Builtin (Required): ✓ Available");
        LOG_INFO("  - TestPlugin (Optional): May or may not be available");
        LOG_INFO("  - Live2D (Soft): May or may not be available");

        return Result<void, std::string>::ok();
    }

    /**
     * @brief 插件卸载时调用
     */
    void on_unload() override {
        LOG_INFO("TestDependencyPlugin: on_unload() called");
    }

    /**
     * @brief 插件启用时调用
     */
    void on_enable() override {
        LOG_INFO("TestDependencyPlugin: on_enable() called - Plugin is now active");
        LOG_INFO("TestDependencyPlugin: Dependency verification passed - plugin is fully functional");
    }

    /**
     * @brief 插件禁用时调用
     */
    void on_disable() override {
        LOG_INFO("TestDependencyPlugin: on_disable() called - Plugin is now disabled");
    }
};

} // namespace TestDependencyPlugin
