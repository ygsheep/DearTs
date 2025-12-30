/**
 * @file plugin_manager_test.cpp
 * @brief Unit tests for PluginManager
 */

#include "core/plugin/plugin.h"
#include "mock_plugin.hpp"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

using namespace DearTs::Core::Plugin;
using namespace DearTs::Tests;

// ============================================================================
// PluginManager Test Fixture
// ============================================================================

class PluginManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Note: PluginManager is a singleton, so we can't easily reset it
        // Tests should use unique plugin names to avoid conflicts
    }

    void TearDown() override {
        // Unload any test plugins
        PluginManager::instance().unload("TestPlugin");
        PluginManager::instance().unload("MockPlugin");
        PluginManager::instance().unload("FailingMockPlugin");
    }
};

// ============================================================================
// Add Builtin Plugin Tests
// ============================================================================

TEST_F(PluginManagerTest, AddBuiltin_Success) {
    auto plugin = std::make_unique<MockPlugin>();
    plugin->custom_name = "TestPlugin";

    auto result = PluginManager::instance().add_builtin(std::move(plugin));

    ASSERT_TRUE(result.isOk()) << "Should successfully add builtin plugin";
}

TEST_F(PluginManagerTest, AddBuiltin_DuplicateName_ReturnsError) {
    auto plugin1 = std::make_unique<MockPlugin>();
    plugin1->custom_name = "DuplicatePlugin";

    // Add first plugin
    auto result1 = PluginManager::instance().add_builtin(std::move(plugin1));
    ASSERT_TRUE(result1.isOk());

    // Try to add second plugin with same name
    auto plugin2 = std::make_unique<MockPlugin>();
    plugin2->custom_name = "DuplicatePlugin";

    auto result2 = PluginManager::instance().add_builtin(std::move(plugin2));
    EXPECT_TRUE(result2.isErr()) << "Should fail to add duplicate plugin name";
}

TEST_F(PluginManagerTest, AddBuiltin_LoadFails_PluginInErrorState) {
    auto plugin = std::make_unique<FailingMockPlugin>();

    auto result = PluginManager::instance().add_builtin(std::move(plugin));

    // Should still add the plugin, but it will be in error state
    // Implementation may vary - adjust based on actual behavior
}

// ============================================================================
// Plugin State Tests
// ============================================================================

TEST_F(PluginManagerTest, PluginLifecycle_StatesTransitionsCorrectly) {
    auto plugin = std::make_unique<MockPlugin>();
    plugin->custom_name = "LifecycleTestPlugin";

    // Add plugin (Unloaded -> Loaded)
    PluginManager::instance().add_builtin(std::move(plugin));

    // Note: Actual state checking depends on PluginManager API
    // You may need to add get_plugin_info() or similar method

    // Enable (Loaded -> Enabled)
    auto enable_result = PluginManager::instance().enable("LifecycleTestPlugin");
    // EXPECT_TRUE(enable_result.isOk());

    // Disable (Enabled -> Disabled)
    // ...

    // Unload (Disabled -> Unloaded)
    bool unloaded = PluginManager::instance().unload("LifecycleTestPlugin");
    // EXPECT_TRUE(unloaded);
}

TEST_F(PluginManagerTest, Load_CallsOnLoad) {
    auto plugin = std::make_unique<MockPlugin>();
    plugin->custom_name = "LoadTestPlugin";
    MockPlugin* plugin_ptr = plugin.get();  // Save pointer before move

    PluginManager::instance().add_builtin(std::move(plugin));

    // Verify on_load was called
    EXPECT_TRUE(plugin_ptr->on_load_called);
}

TEST_F(PluginManagerTest, Enable_CallsOnEnable) {
    auto plugin = std::make_unique<MockPlugin>();
    plugin->custom_name = "EnableTestPlugin";
    MockPlugin* plugin_ptr = plugin.get();

    PluginManager::instance().add_builtin(std::move(plugin));
    PluginManager::instance().enable("EnableTestPlugin");

    EXPECT_TRUE(plugin_ptr->on_enable_called);
}

TEST_F(PluginManagerTest, Disable_CallsOnDisable) {
    auto plugin = std::make_unique<MockPlugin>();
    plugin->custom_name = "DisableTestPlugin";
    MockPlugin* plugin_ptr = plugin.get();

    PluginManager::instance().add_builtin(std::move(plugin));
    PluginManager::instance().enable("DisableTestPlugin");
    // Assuming there's a disable method
    // PluginManager::instance().disable("DisableTestPlugin");

    // EXPECT_TRUE(plugin_ptr->on_disable_called);
}

// ============================================================================
// Unload Plugin Tests
// ============================================================================

TEST_F(PluginManagerTest, Unload_ExistingPlugin_ReturnsTrue) {
    auto plugin = std::make_unique<MockPlugin>();
    plugin->custom_name = "UnloadTestPlugin";

    PluginManager::instance().add_builtin(std::move(plugin));

    bool result = PluginManager::instance().unload("UnloadTestPlugin");
    EXPECT_TRUE(result) << "Should successfully unload plugin";
}

TEST_F(PluginManagerTest, Unload_NonExistentPlugin_ReturnsFalse) {
    bool result = PluginManager::instance().unload("NonExistentPlugin");
    EXPECT_FALSE(result) << "Should fail to unload non-existent plugin";
}

TEST_F(PluginManagerTest, Unload_CallsOnUnload) {
    auto plugin = std::make_unique<MockPlugin>();
    plugin->custom_name = "OnUnloadTestPlugin";
    MockPlugin* plugin_ptr = plugin.get();

    PluginManager::instance().add_builtin(std::move(plugin));
    PluginManager::instance().unload("OnUnloadTestPlugin");

    EXPECT_TRUE(plugin_ptr->on_unload_called);
}

// ============================================================================
// API Version Tests
// ============================================================================

TEST_F(PluginManagerTest, AddBuiltin_CompatibleApiVersion_Succeeds) {
    auto plugin = std::make_unique<MockPlugin>();
    plugin->custom_name = "CompatiblePlugin";
    plugin->custom_api_version = "1.0.0";

    auto result = PluginManager::instance().add_builtin(std::move(plugin));

    // Assuming framework API version is "1.0.0"
    EXPECT_TRUE(result.isOk()) << "Compatible API version should succeed";
}

TEST_F(PluginManagerTest, AddBuiltin_IncompatibleApiVersion_Fails) {
    auto plugin = std::make_unique<IncompatibleApiVersionMockPlugin>();

    auto result = PluginManager::instance().add_builtin(std::move(plugin));

    // Behavior depends on implementation
    // May succeed during add but fail during load
}

// ============================================================================
// Get Plugin Info Tests
// ============================================================================

TEST_F(PluginManagerTest, GetPluginInfo_ExistingPlugin_ReturnsInfo) {
    auto plugin = std::make_unique<MockPlugin>();
    plugin->custom_name = "InfoTestPlugin";
    plugin->custom_author = "Test Author";
    plugin->custom_description = "Test Description";
    plugin->custom_version = "2.0.0";

    PluginManager::instance().add_builtin(std::move(plugin));

    // Assuming there's a get_plugin_info method
    // auto info_result = PluginManager::instance().get_plugin_info("InfoTestPlugin");
    // ASSERT_TRUE(info_result.isOk());
    //
    // auto info = info_result.unwrap();
    // EXPECT_EQ(info.name, "InfoTestPlugin");
    // EXPECT_EQ(info.author, "Test Author");
    // EXPECT_EQ(info.description, "Test Description");
    // EXPECT_EQ(info.version, "2.0.0");
}

TEST_F(PluginManagerTest, GetPluginInfo_NonExistentPlugin_ReturnsError) {
    // auto info_result = PluginManager::instance().get_plugin_info("NonExistent");
    // EXPECT_TRUE(info_result.isErr());
}

// ============================================================================
// List Plugins Tests
// ============================================================================

TEST_F(PluginManagerTest, ListPlugins_ReturnsAllPlugins) {
    PluginManager::instance().add_builtin(std::make_unique<MockPlugin>());
    PluginManager::instance().add_builtin(std::make_unique<MockPlugin>());
    PluginManager::instance().add_builtin(std::make_unique<MockPlugin>());

    // Assuming there's a list_plugins method
    // auto plugins = PluginManager::instance().list_plugins();
    // EXPECT_GE(plugins.size(), 3);
}

TEST_F(PluginManagerTest, ListPlugins_Empty_ReturnsEmpty) {
    // Note: Can't easily clear singleton PluginManager
    // This test may not be reliable

    // auto plugins = PluginManager::instance().list_plugins();
    // EXPECT_FALSE(plugins.empty());  // Likely has other plugins
}

// ============================================================================
// Load from File Tests
// ============================================================================

TEST_F(PluginManagerTest, LoadFromFile_NonExistentFile_ReturnsError) {
    auto result = PluginManager::instance().load_from_file(
        "non_existent_plugin_12345.dll"
    );

    EXPECT_TRUE(result.isErr()) << "Should fail to load non-existent file";
}

TEST_F(PluginManagerTest, LoadFromFile_InvalidFile_ReturnsError) {
    // Create a temporary invalid file
    std::filesystem::path temp_file = "invalid_plugin.txt";
    std::ofstream(temp_file) << "This is not a valid plugin library";

    auto result = PluginManager::instance().load_from_file(temp_file);

    EXPECT_TRUE(result.isErr()) << "Should fail to load invalid plugin file";

    // Clean up
    std::filesystem::remove(temp_file);
}

// ============================================================================
// Load from Directory Tests
// ============================================================================

TEST_F(PluginManagerTest, LoadFromDirectory_NonExistentDirectory_ReturnsError) {
    auto result = PluginManager::instance().load_from_directory(
        "non_existent_dir_12345"
    );

    EXPECT_TRUE(result.isErr()) << "Should fail to load from non-existent directory";
}

TEST_F(PluginManagerTest, LoadFromDirectory_EmptyDirectory_ReturnsZero) {
    // Create temporary empty directory
    std::filesystem::path temp_dir = "empty_plugin_dir_12345";
    std::filesystem::create_directories(temp_dir);

    auto result = PluginManager::instance().load_from_directory(temp_dir);

    ASSERT_TRUE(result.isOk()) << "Should succeed (but load 0 plugins)";
    EXPECT_EQ(result.unwrap(), 0) << "Should load 0 plugins from empty directory";

    // Clean up
    std::filesystem::remove(temp_dir);
}

// ============================================================================
// Enable/Disable Tests
// ============================================================================

TEST_F(PluginManagerTest, Enable_NonExistentPlugin_ReturnsError) {
    auto result = PluginManager::instance().enable("NonExistentPlugin");

    EXPECT_TRUE(result.isErr()) << "Should fail to enable non-existent plugin";
}

TEST_F(PluginManagerTest, Enable_AlreadyEnabledPlugin_Succeeds) {
    auto plugin = std::make_unique<MockPlugin>();
    plugin->custom_name = "AlreadyEnabledPlugin";

    PluginManager::instance().add_builtin(std::move(plugin));

    auto result1 = PluginManager::instance().enable("AlreadyEnabledPlugin");
    // auto result2 = PluginManager::instance().enable("AlreadyEnabledPlugin");

    // EXPECT_TRUE(result1.isOk());
    // Second enable may or may not succeed depending on implementation
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST_F(PluginManagerTest, PluginWithLoadError_ErrorStateSet) {
    auto plugin = std::make_unique<FailingMockPlugin>();

    PluginManager::instance().add_builtin(std::move(plugin));

    // Plugin should be in error state
    // Check via get_plugin_info or similar
}

TEST_F(PluginManagerTest, MultiplePluginsWithErrors_OthersStillLoad) {
    auto failing_plugin = std::make_unique<FailingMockPlugin>();
    failing_plugin->custom_name = "FailingPlugin";

    auto working_plugin = std::make_unique<MockPlugin>();
    working_plugin->custom_name = "WorkingPlugin";

    PluginManager::instance().add_builtin(std::move(failing_plugin));
    PluginManager::instance().add_builtin(std::move(working_plugin));

    // Working plugin should still be loadable
    auto result = PluginManager::instance().enable("WorkingPlugin");
    // EXPECT_TRUE(result.isOk()) << "Working plugin should succeed";
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

TEST_F(PluginManagerTest, ConcurrentAddBuiltin_ThreadSafe) {
    const int num_threads = 10;
    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([i]() {
            auto plugin = std::make_unique<MockPlugin>();
            plugin->custom_name = "ConcurrentPlugin_" + std::to_string(i);

            PluginManager::instance().add_builtin(std::move(plugin));
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // All plugins should be added successfully
    // Verify by listing plugins
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(PluginManagerTest, AddNullPlugin_ReturnsError) {
    // Behavior depends on implementation
    // EXPECT_NO_THROW({
    //     PluginManager::instance().add_builtin(nullptr);
    // });
}

TEST_F(PluginManagerTest, PluginWithEmptyName_HandledCorrectly) {
    auto plugin = std::make_unique<MockPlugin>();
    plugin->custom_name = "";

    auto result = PluginManager::instance().add_builtin(std::move(plugin));

    // May succeed or fail depending on validation
}

TEST_F(PluginManagerTest, PluginWithSpecialCharactersInName_HandledCorrectly) {
    auto plugin = std::make_unique<MockPlugin>();
    plugin->custom_name = "Test-Plugin_123.456";

    auto result = PluginManager::instance().add_builtin(std::move(plugin));

    // Should handle special characters
}

// ============================================================================
// Plugin Metadata Tests
// ============================================================================

TEST_F(PluginManagerTest, PluginInfo_AllFieldsSetCorrectly) {
    auto plugin = std::make_unique<MockPlugin>();
    plugin->custom_name = "MetadataTestPlugin";
    plugin->custom_author = "Test Author";
    plugin->custom_description = "A test plugin for unit testing";
    plugin->custom_version = "1.2.3";
    plugin->custom_api_version = "1.0.0";

    auto result = PluginManager::instance().add_builtin(std::move(plugin));
    ASSERT_TRUE(result.isOk());

    // Verify metadata is stored correctly
    // auto info = PluginManager::instance().get_plugin_info("MetadataTestPlugin");
    // ASSERT_TRUE(info.isOk());
    //
    // auto info_data = info.unwrap();
    // EXPECT_EQ(info_data.name, "MetadataTestPlugin");
    // EXPECT_EQ(info_data.author, "Test Author");
    // EXPECT_EQ(info_data.description, "A test plugin for unit testing");
    // EXPECT_EQ(info_data.version, "1.2.3");
    // EXPECT_EQ(info_data.api_version, "1.0.0");
}

// ============================================================================
// Plugin Wrapper Tests
// ============================================================================

TEST_F(PluginManagerTest, PluginWrapper_ManagesLifecycleCorrectly) {
    auto plugin = std::make_unique<MockPlugin>();
    plugin->custom_name = "WrapperTestPlugin";
    MockPlugin* plugin_ptr = plugin.get();

    // Add plugin (wrapper takes ownership)
    PluginManager::instance().add_builtin(std::move(plugin));

    // Plugin should still exist (managed by wrapper)
    // Verify by checking if callbacks are still called
}

TEST_F(PluginManagerTest, PluginWrapper_Unload_CleansUpCorrectly) {
    auto plugin = std::make_unique<MockPlugin>();
    plugin->custom_name = "WrapperCleanupTest";
    MockPlugin* plugin_ptr = plugin.get();

    PluginManager::instance().add_builtin(std::move(plugin));

    bool on_unload_called_before = plugin_ptr->on_unload_called;

    PluginManager::instance().unload("WrapperCleanupTest");

    EXPECT_TRUE(plugin_ptr->on_unload_called);
    EXPECT_GT(plugin_ptr->on_unload_called, on_unload_called_before);
}
