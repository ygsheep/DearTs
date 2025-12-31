/**
 * @file full_lifecycle_test.cpp
 * @brief 插件完整生命周期集成测试
 * @details 测试插件从创建到卸载的完整生命周期
 * @author DearTs Team
 * @date 2025
 */

#include <gtest/gtest.h>
#include "core/plugin/plugin.h"
#include "core/config/config_manager.h"
#include "core/event/event_bus.h"
#include "tests/mocks/mock_plugin.hpp"

using namespace DearTs::Core::Plugin;
using namespace DearTs::Core::Config;
using namespace DearTs::Core::Event;
using DearTs::Tests::MockPlugin;

/**
 * @brief 插件生命周期集成测试 Fixture
 */
class PluginLifecycleTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 清理插件管理器状态
        PluginManager::instance().clear();

        // 清理配置
        ConfigManager::instance().clear_for_test();
        ConfigManager::instance().clear_change_callbacks();

        // 清理事件总线
        EventBus::instance().clear();
    }

    void TearDown() override {
        PluginManager::instance().clear();
        EventBus::instance().clear();
    }
};

// ============================================================================
// 完整生命周期测试
// ============================================================================

TEST_F(PluginLifecycleTest, FullCycle_BuiltinPlugin) {
    // 创建一个测试插件
    auto test_plugin = std::make_unique<MockPlugin>();
    test_plugin->custom_name = "TestPlugin";

    // 1. 添加插件（自动加载并启用）
    auto add_result = PluginManager::instance().add_builtin(std::move(test_plugin));
    ASSERT_TRUE(add_result.isOk()) << add_result.error();

    auto* plugin = PluginManager::instance().get_plugin("TestPlugin");
    ASSERT_NE(plugin, nullptr);

    auto state_result = PluginManager::instance().get_plugin_state("TestPlugin");
    ASSERT_TRUE(state_result.isOk());
    EXPECT_EQ(state_result.unwrap(), PluginState::Enabled);  // add_builtin 自动启用

    // 2. 禁用插件
    auto disable_result = PluginManager::instance().disable("TestPlugin");
    ASSERT_TRUE(disable_result.isOk()) << disable_result.error();

    state_result = PluginManager::instance().get_plugin_state("TestPlugin");
    ASSERT_TRUE(state_result.isOk());
    EXPECT_EQ(state_result.unwrap(), PluginState::Loaded);  // disable 返回 Loaded 状态

    // 3. 重新启用插件
    auto enable_result = PluginManager::instance().enable("TestPlugin");
    ASSERT_TRUE(enable_result.isOk()) << enable_result.error();

    state_result = PluginManager::instance().get_plugin_state("TestPlugin");
    ASSERT_TRUE(state_result.isOk());
    EXPECT_EQ(state_result.unwrap(), PluginState::Enabled);

    // 4. 卸载插件
    auto unload_success = PluginManager::instance().unload("TestPlugin");
    EXPECT_TRUE(unload_success);

    // 5. 验证插件已从管理器移除
    plugin = PluginManager::instance().get_plugin("TestPlugin");
    EXPECT_EQ(plugin, nullptr);
}

TEST_F(PluginLifecycleTest, FullCycle_WithConfigAccess) {
    // 测试插件在生命周期中访问配置
    auto test_plugin = std::make_unique<MockPlugin>();
    test_plugin->custom_name = "ConfigTestPlugin";

    PluginManager::instance().add_builtin(std::move(test_plugin));

    // 启用插件前设置配置
    ConfigScope cfg("config_test");
    cfg.set("enabled", true);
    cfg.set("version", "1.0.0");

    // 验证配置已写入
    EXPECT_TRUE(cfg.get_or<bool>("enabled", false));
    EXPECT_EQ(cfg.get_or<std::string>("version", ""), "1.0.0");

    // 禁用插件后修改配置
    cfg.set("enabled", false);

    // 验证配置已更新
    EXPECT_FALSE(cfg.get_or<bool>("enabled", true));
}

TEST_F(PluginLifecycleTest, FullCycle_WithEventSubscription) {
    // 测试插件在生命周期中订阅事件
    auto test_plugin = std::make_unique<MockPlugin>();
    test_plugin->custom_name = "EventTestPlugin";

    // 定义测试事件
    struct TestEvent {
        int data;
    };

    bool event_received = false;
    EventToken event_token;

    PluginManager::instance().add_builtin(std::move(test_plugin));

    // 手动订阅事件（模拟插件启用）
    event_token = EventBus::instance().subscribe<TestEvent>(
        [&event_received](const TestEvent& e) {
            event_received = true;
        }
    );

    // 启用插件
    PluginManager::instance().enable("EventTestPlugin");

    // 发布事件
    EventBus::instance().publish(TestEvent{ .data = 42 });
    EXPECT_TRUE(event_received);

    // 禁用插件
    event_received = false;
    PluginManager::instance().disable("EventTestPlugin");

    // 手动取消订阅（模拟插件禁用）
    EventBus::instance().unsubscribe<TestEvent>(event_token);

    // 发布事件，应该不再接收
    EventBus::instance().publish(TestEvent{ .data = 100 });
    EXPECT_FALSE(event_received);
}

TEST_F(PluginLifecycleTest, StateTransitions_InvalidTransitions) {
    // 测试无效的状态转换
    auto test_plugin = std::make_unique<MockPlugin>();
    test_plugin->custom_name = "StateTestPlugin";

    PluginManager::instance().add_builtin(std::move(test_plugin));
    auto* plugin = PluginManager::instance().get_plugin("StateTestPlugin");
    ASSERT_NE(plugin, nullptr);

    // 插件已经被 add_builtin 自动启用，状态为 Enabled
    auto state_result = PluginManager::instance().get_plugin_state("StateTestPlugin");
    ASSERT_TRUE(state_result.isOk());
    EXPECT_EQ(state_result.unwrap(), PluginState::Enabled);

    // 禁用插件（从 Enabled → Loaded）
    auto disable_result = PluginManager::instance().disable("StateTestPlugin");
    ASSERT_TRUE(disable_result.isOk());

    state_result = PluginManager::instance().get_plugin_state("StateTestPlugin");
    EXPECT_EQ(state_result.unwrap(), PluginState::Loaded);

    // 再次禁用（已经是 Loaded 状态，操作被忽略但返回成功）
    disable_result = PluginManager::instance().disable("StateTestPlugin");
    ASSERT_TRUE(disable_result.isOk());  // 不返回错误，只是记录警告

    // 重新启用插件
    auto enable_result = PluginManager::instance().enable("StateTestPlugin");
    ASSERT_TRUE(enable_result.isOk());

    state_result = PluginManager::instance().get_plugin_state("StateTestPlugin");
    EXPECT_EQ(state_result.unwrap(), PluginState::Enabled);

    // 再次启用（已经是 Enabled 状态，操作被忽略但返回成功）
    enable_result = PluginManager::instance().enable("StateTestPlugin");
    ASSERT_TRUE(enable_result.isOk());  // 不返回错误，只是记录警告
}

TEST_F(PluginLifecycleTest, MultiplePlugins_InterdependentLifecycle) {
    // 测试多个插件的协同生命周期

    // 创建插件 A
    auto plugin_a = std::make_unique<MockPlugin>();
    plugin_a->custom_name = "PluginA";

    // 创建插件 B
    auto plugin_b = std::make_unique<MockPlugin>();
    plugin_b->custom_name = "PluginB";

    // 定义事件
    struct DataEvent {
        std::string data;
    };

    std::string received_data;
    EventToken event_token;

    // 插件 A 的订阅将在启用时手动设置
    PluginManager::instance().add_builtin(std::move(plugin_a));
    PluginManager::instance().add_builtin(std::move(plugin_b));

    // 启用插件 A
    PluginManager::instance().enable("PluginA");

    // 手动订阅事件（模拟插件 A 订阅）
    event_token = EventBus::instance().subscribe<DataEvent>(
        [&received_data](const DataEvent& e) {
            received_data = e.data;
        }
    );

    // 插件 B 在启用后发布事件（手动发布）
    PluginManager::instance().enable("PluginB");
    EventBus::instance().publish(DataEvent{ .data = "Hello from PluginB" });

    // 验证事件被接收
    EXPECT_EQ(received_data, "Hello from PluginB");

    // 禁用插件 A
    received_data.clear();
    PluginManager::instance().disable("PluginA");

    // 手动取消订阅（模拟插件 A 禁用）
    EventBus::instance().unsubscribe<DataEvent>(event_token);

    // 插件 B 再次发布事件，不应该被接收
    EventBus::instance().publish(DataEvent{ .data = "Test after disable" });
    EXPECT_TRUE(received_data.empty());
}

TEST_F(PluginLifecycleTest, UnloadAll_Cleanup) {
    // 创建多个插件
    for (int i = 0; i < 3; ++i) {
        auto plugin = std::make_unique<MockPlugin>();
        plugin->custom_name = "Plugin" + std::to_string(i);
        PluginManager::instance().add_builtin(std::move(plugin));
    }

    // 启用所有插件
    for (int i = 0; i < 3; ++i) {
        PluginManager::instance().enable("Plugin" + std::to_string(i));
    }

    // 验证所有插件都已启用
    auto plugins_info = PluginManager::instance().get_all_plugins_info();
    EXPECT_EQ(plugins_info.size(), 3);

    for (const auto& info : plugins_info) {
        auto state_result = PluginManager::instance().get_plugin_state(info.name);
        ASSERT_TRUE(state_result.isOk());
        EXPECT_EQ(state_result.unwrap(), PluginState::Enabled);
    }

    // 卸载所有插件
    PluginManager::instance().clear();

    // 验证所有插件都已卸载
    plugins_info = PluginManager::instance().get_all_plugins_info();
    EXPECT_TRUE(plugins_info.empty());
}

// ============================================================================
// 边界情况测试
// ============================================================================

TEST_F(PluginLifecycleTest, EnableNonExistentPlugin_ReturnsError) {
    auto result = PluginManager::instance().enable("NonExistent");
    EXPECT_TRUE(result.isErr());
}

TEST_F(PluginLifecycleTest, DisableNonExistentPlugin_ReturnsError) {
    auto result = PluginManager::instance().disable("NonExistent");
    EXPECT_TRUE(result.isErr());
}

TEST_F(PluginLifecycleTest, UnloadNonExistentPlugin_ReturnsError) {
    auto result = PluginManager::instance().unload("NonExistent");
    EXPECT_FALSE(result);  // unload() returns bool, false means failed
}
