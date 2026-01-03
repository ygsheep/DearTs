/**
 * @file test_plugin.hpp
 * @brief 测试插件 - 用于验证插件自动发现功能
 */

#pragma once

#include "core/plugin/plugin.h"

namespace TestPlugin {

/**
 * @brief 测试插件类
 */
class TestPlugin : public DearTs::Core::Plugin::IPlugin {
public:
    /**
     * @brief 获取插件信息
     */
    [[nodiscard]] DearTs::Core::Plugin::PluginInfo get_info() const override {
        return DearTs::Core::Plugin::PluginInfo{
            .name = "TestPlugin",
            .author = "DearTs Team",
            .description = "Test plugin for auto-discovery",
            .version = "1.0.0",
            .api_version = "1.0.0"
        };
    }

    /**
     * @brief 插件加载时调用
     */
    DearTs::Core::Result<void, std::string> on_load() override {
        LOG_INFO("TestPlugin: on_load() called");

        // 注册一个测试命令
        Core::ContentRegistry::Commands::add(
            "testplugin.hello",
            "测试插件问候",
            []() {
                LOG_INFO("Hello from TestPlugin!");
            }
        );

        LOG_INFO("TestPlugin: Registered test command 'testplugin.hello'");
        return DearTs::Core::Result<void, std::string>::ok();
    }

    /**
     * @brief 插件卸载时调用
     */
    void on_unload() override {
        LOG_INFO("TestPlugin: on_unload() called");
    }

    /**
     * @brief 插件启用时调用
     */
    void on_enable() override {
        LOG_INFO("TestPlugin: on_enable() called - Plugin is now active");
    }

    /**
     * @brief 插件禁用时调用
     */
    void on_disable() override {
        LOG_INFO("TestPlugin: on_disable() called - Plugin is now disabled");
    }
};

} // namespace TestPlugin
