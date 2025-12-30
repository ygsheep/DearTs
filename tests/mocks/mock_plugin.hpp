/**
 * @file mock_plugin.hpp
 * @brief Mock plugin for testing
 * @details Provides a simple IPlugin implementation for unit tests
 */

#pragma once

#include "core/plugin/plugin.h"
#include "core/result.h"
#include <string>

namespace DearTs::Tests {

/**
 * @brief Mock plugin for testing
 *
 * Provides flags to track which lifecycle methods were called
 */
class MockPlugin : public DearTs::Core::Plugin::IPlugin {
public:
    bool on_load_called = false;
    bool on_unload_called = false;
    bool on_enable_called = false;
    bool on_disable_called = false;

    Result<void, std::string> on_load_result = Result<void, std::string>::ok();
    bool should_fail_load = false;

    std::string custom_name = "MockPlugin";
    std::string custom_author = "Test Author";
    std::string custom_description = "Mock plugin for testing";
    std::string custom_version = "1.0.0";
    std::string custom_api_version = "1.0.0";

    [[nodiscard]] DearTs::Core::Plugin::PluginInfo get_info() const override {
        return DearTs::Core::Plugin::PluginInfo{
            .name = custom_name,
            .author = custom_author,
            .description = custom_description,
            .version = custom_version,
            .api_version = custom_api_version
        };
    }

    Result<void, std::string> on_load() override {
        on_load_called = true;

        if (should_fail_load) {
            return Result<void, std::string>::err("Mock plugin load failed");
        }

        return on_load_result;
    }

    void on_unload() override {
        on_unload_called = true;
    }

    void on_enable() override {
        on_enable_called = true;
    }

    void on_disable() override {
        on_disable_called = true;
    }

    /**
     * @brief Reset all flags to false
     */
    void reset() {
        on_load_called = false;
        on_unload_called = false;
        on_enable_called = false;
        on_disable_called = false;
        should_fail_load = false;
        on_load_result = Result<void, std::string>::ok();
    }
};

/**
 * @brief Mock plugin that fails to load
 */
class FailingMockPlugin : public MockPlugin {
public:
    FailingMockPlugin() {
        should_fail_load = true;
        custom_name = "FailingMockPlugin";
    }
};

/**
 * @brief Mock plugin with incompatible API version
 */
class IncompatibleApiVersionMockPlugin : public MockPlugin {
public:
    IncompatibleApiVersionMockPlugin() {
        custom_api_version = "0.0.1";  // Incompatible version
        custom_name = "IncompatibleApiVersionMockPlugin";
    }
};

} // namespace DearTs::Tests
