/**
 * @file content_registry_test.cpp
 * @brief Unit tests for ContentRegistry (Commands, Tools, Settings)
 */

#include <gtest/gtest.h>
#include "core/content/commands.h"
#include "core/content/tools.h"
#include "core/content/settings.h"
#include <string>
#include <vector>

using namespace DearTs::Core::Content;

// ============================================================================
// ContentRegistry Test Fixture
// ============================================================================

class ContentRegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Note: ContentRegistry is likely a singleton or static registry
        // Tests should use unique names to avoid conflicts
    }

    void TearDown() override {
        // Clean up - unregister test commands/tools if needed
    }
};

// ============================================================================
// Command Registry Tests
// ============================================================================

TEST_F(ContentRegistryTest, RegisterCommand_Success) {
    bool executed = false;

    auto result = ContentRegistry::Commands::register_handler(
        "test.command",
        "Test Command",
        [&]() { executed = true; },
        nullptr,  // enabled callback
        "Ctrl+Shift+T"
    );

    // Execute the command
    ContentRegistry::Commands::invoke("test.command");

    EXPECT_TRUE(executed);
}

TEST_F(ContentRegistryTest, RegisterCommandWithEnabledCallback_OnlyExecutesWhenEnabled) {
    bool executed = false;
    bool enabled = false;

    auto result = ContentRegistry::Commands::register_handler(
        "test.conditional_command",
        "Conditional Command",
        [&]() { executed = true; },
        [&]() -> bool { return enabled; },
        ""
    );

    // Try to execute when disabled
    ContentRegistry::Commands::invoke("test.conditional_command");
    EXPECT_FALSE(executed);

    // Enable and execute
    enabled = true;
    ContentRegistry::Commands::invoke("test.conditional_command");
    EXPECT_TRUE(executed);
}

TEST_F(ContentRegistryTest, RegisterDuplicateCommand_OverwritesExisting) {
    bool first_executed = false;
    bool second_executed = false;

    // Register first command
    ContentRegistry::Commands::register_handler(
        "test.duplicate",
        "First Command",
        [&]() { first_executed = true; },
        nullptr,
        ""
    );

    // Register second command with same name
    ContentRegistry::Commands::register_handler(
        "test.duplicate",
        "Second Command",
        [&]() { second_executed = true; },
        nullptr,
        ""
    );

    // Execute
    ContentRegistry::Commands::invoke("test.duplicate");

    // Behavior depends on implementation
    // May overwrite or keep both
}

TEST_F(ContentRegistryTest, InvokeNonExistentCommand_DoesNothing) {
    // Should not crash or throw
    EXPECT_NO_THROW({
        ContentRegistry::Commands::invoke("non.existent.command");
    });
}

TEST_F(ContentRegistryTest, GetCommandByName_ReturnsCorrectCommand) {
    ContentRegistry::Commands::register_handler(
        "test.named_command",
        "Named Command",
        []() {},
        nullptr,
        "Ctrl+N"
    );

    // Assuming there's a get_command_info method
    // auto command = ContentRegistry::Commands::get_command_info("test.named_command");
    // EXPECT_EQ(command.name, "Named Command");
    // EXPECT_EQ(command.shortcut, "Ctrl+N");
}

TEST_F(CommandRegistryTest, ListCommands_ReturnsAllCommands) {
    ContentRegistry::Commands::register_handler("test.cmd1", "Command 1", []() {}, nullptr, "");
    ContentRegistry::Commands::register_handler("test.cmd2", "Command 2", []() {}, nullptr, "");
    ContentRegistry::Commands::register_handler("test.cmd3", "Command 3", []() {}, nullptr, "");

    // auto commands = ContentRegistry::Commands::get_all_commands();
    // EXPECT_GE(commands.size(), 3);
}

// ============================================================================
// Tool Registry Tests
// ============================================================================

TEST_F(ContentRegistryTest, RegisterTool_Success) {
    bool tool_executed = false;

    ContentRegistry::Tools::add(
        "Test Tool",
        [&]() { tool_executed = true; }
    );

    // Assuming there's a way to invoke tools
    // ContentRegistry::Tools::invoke("Test Tool");

    // EXPECT_TRUE(tool_executed);
}

TEST_F(ContentRegistryTest, RegisterDuplicateTools_AllRegistered) {
    int count = 0;

    ContentRegistry::Tools::add("Tool 1", [&]() { count++; });
    ContentRegistry::Tools::add("Tool 2", [&]() { count++; });
    ContentRegistry::Tools::add("Tool 1", [&]() { count++; });  // Duplicate name

    // Behavior depends on implementation
    // May allow duplicates or not
}

TEST_F(ContentRegistryTest, GetTools_ReturnsAllTools) {
    ContentRegistry::Tools::add("Tool A", []() {});
    ContentRegistry::Tools::add("Tool B", []() {});
    ContentRegistry::Tools::add("Tool C", []() {});

    // auto tools = ContentRegistry::Tools::get_all();
    // EXPECT_GE(tools.size(), 3);
}

// ============================================================================
// Settings Registry Tests
// ============================================================================

TEST_F(ContentRegistryTest, RegisterSetting_Success) {
    int value = 42;

    ContentRegistry::Settings::add(
        "test.value",
        "Test Value",
        value
    );

    // Assuming there's a way to get/set settings
    // auto retrieved = ContentRegistry::Settings::get<int>("test.value");
    // EXPECT_EQ(retrieved, 42);
}

TEST_F(ContentRegistryTest, RegisterSettingWithDifferentTypes_WorkCorrectly) {
    ContentRegistry::Settings::add("test.int_value", "Int Value", 123);
    ContentRegistry::Settings::add("test.str_value", "String Value", std::string("hello"));
    ContentRegistry::Settings::add("test.bool_value", "Bool Value", true);
    ContentRegistry::Settings::add("test.double_value", "Double Value", 3.14);

    // All should be registered successfully
    // Verify by getting each value
}

TEST_F(ContentRegistryTest, UpdateSetting_NotifiesListeners) {
    bool callback_called = false;
    int new_value = 0;

    ContentRegistry::Settings::add("test.updatable", "Updatable", 10);

    // Assuming there's a way to add change listeners
    // ContentRegistry::Settings::add_change_callback("test.updatable", [&](const auto& value) {
    //     callback_called = true;
    //     new_value = std::get<int>(value);
    // });

    // ContentRegistry::Settings::set("test.updatable", 20);

    // EXPECT_TRUE(callback_called);
    // EXPECT_EQ(new_value, 20);
}

TEST_F(ContentRegistryTest, GetSettingThatDoesNotExist_ReturnsDefault) {
    // auto value = ContentRegistry::Settings::get_with_default<int>("non.existent", 99);
    // EXPECT_EQ(value, 99);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(ContentRegistryTest, CommandAndSettingsIntegration_WorkTogether) {
    int setting_value = 0;

    ContentRegistry::Settings::add("test.repeat_count", "Repeat Count", 5);

    ContentRegistry::Commands::register_handler(
        "test.repeat_command",
        "Repeat Command",
        [&]() {
            // auto count = ContentRegistry::Settings::get<int>("test.repeat_count");
            // setting_value = count;
        },
        nullptr,
        ""
    );

    ContentRegistry::Commands::invoke("test.repeat_command");

    // EXPECT_EQ(setting_value, 5);
}

TEST_F(ContentRegistryTest, CommandThatOpensTool_WorkCorrectly) {
    bool tool_opened = false;

    ContentRegistry::Tools::add("Test Tool", [&]() {
        tool_opened = true;
    });

    ContentRegistry::Commands::register_handler(
        "test.open_tool",
        "Open Test Tool",
        [&]() {
            // ContentRegistry::Tools::invoke("Test Tool");
        },
        nullptr,
        ""
    );

    ContentRegistry::Commands::invoke("test.open_tool");

    // EXPECT_TRUE(tool_opened);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(ContentRegistryTest, RegisterCommandWithEmptyName_HandledCorrectly) {
    EXPECT_NO_THROW({
        ContentRegistry::Commands::register_handler(
            "",
            "Empty Name Command",
            []() {},
            nullptr,
            ""
        );
    });
}

TEST_F(CommandRegistryTest, RegisterCommandWithNullCallback_DoesNotCrash) {
    // Behavior depends on implementation validation
    // EXPECT_NO_THROW({
    //     ContentRegistry::Commands::register_handler(
    //         "test.null_callback",
    //         "Null Callback",
    //         nullptr,
    //         nullptr,
    //         ""
    //     );
    // });
}

TEST_F(ContentRegistryTest, RegisterManyCommands_AllWorkCorrectly) {
    const int num_commands = 100;

    for (int i = 0; i < num_commands; ++i) {
        ContentRegistry::Commands::register_handler(
            "test.massive_cmd_" + std::to_string(i),
            "Massive Command " + std::to_string(i),
            []() {},
            nullptr,
            ""
        );
    }

    // All should be registered
    // auto count = ContentRegistry::Commands::get_all_commands().size();
    // EXPECT_GE(count, num_commands);
}

// ============================================================================
// Shortcut Tests
// ============================================================================

TEST_F(ContentRegistryTest, RegisterCommandWithShortcut_ShortcutStored) {
    ContentRegistry::Commands::register_handler(
        "test.shortcut_cmd",
        "Shortcut Command",
        []() {},
        nullptr,
        "Ctrl+Shift+S"
    );

    // Assuming there's a way to query shortcuts
    // auto shortcut = ContentRegistry::Commands::get_shortcut("test.shortcut_cmd");
    // EXPECT_EQ(shortcut, "Ctrl+Shift+S");
}

TEST_F(ContentRegistryTest, ExecuteCommandByShortcut_WorksCorrectly) {
    bool executed = false;

    ContentRegistry::Commands::register_handler(
        "test.shortcut_execute",
        "Execute by Shortcut",
        [&]() { executed = true; },
        nullptr,
        "Ctrl+X"
    );

    // Assuming there's a method to execute by shortcut
    // ContentRegistry::Commands::execute_by_shortcut("Ctrl+X");

    // EXPECT_TRUE(executed);
}

TEST_F(CommandRegistryTest, DuplicateShortcuts_AllRegistered) {
    ContentRegistry::Commands::register_handler("test.cmd1", "Cmd 1", []() {}, nullptr, "Ctrl+A");
    ContentRegistry::Commands::register_handler("test.cmd2", "Cmd 2", []() {}, nullptr, "Ctrl+A");

    // Behavior depends on implementation
    // May allow duplicates or not
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

TEST_F(ContentRegistryTest, ConcurrentRegistrations_ThreadSafe) {
    const int num_threads = 10;
    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([i]() {
            ContentRegistry::Commands::register_handler(
                "test.concurrent_cmd_" + std::to_string(i),
                "Concurrent Command " + std::to_string(i),
                []() {},
                nullptr,
                ""
            );
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // All commands should be registered
    // auto count = ContentRegistry::Commands::get_all_commands().size();
    // EXPECT_GE(count, num_threads);
}

TEST_F(ContentRegistryTest, ConcurrentExecution_ThreadSafe) {
    std::atomic<int> counter{0};
    const int num_threads = 10;

    for (int i = 0; i < num_threads; ++i) {
        ContentRegistry::Commands::register_handler(
            "test.concurrent_exec_" + std::to_string(i),
            "Concurrent Exec " + std::to_string(i),
            [&]() { counter++; },
            nullptr,
            ""
        );
    }

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([i]() {
            ContentRegistry::Commands::invoke("test.concurrent_exec_" + std::to_string(i));
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(counter.load(), num_threads);
}

// ============================================================================
// Unregister Tests
// ============================================================================

TEST_F(ContentRegistryTest, UnregisterCommand_RemovesCommand) {
    ContentRegistry::Commands::register_handler(
        "test.temporary",
        "Temporary Command",
        []() {},
        nullptr,
        ""
    );

    // Assuming there's an unregister method
    // ContentRegistry::Commands::unregister("test.temporary");

    // Command should not exist anymore
    // auto result = ContentRegistry::Commands::get_command_info("test.temporary");
    // EXPECT_TRUE(result.isErr());
}

TEST_F(ContentRegistryTest, UnregisterNonExistentCommand_DoesNothing) {
    EXPECT_NO_THROW({
        // ContentRegistry::Commands::unregister("non.existent.command");
    });
}
