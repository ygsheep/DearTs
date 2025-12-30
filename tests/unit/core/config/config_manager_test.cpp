/**
 * @file config_manager_test.cpp
 * @brief Unit tests for ConfigManager
 */

#include <gtest/gtest.h>
#include "core/config/config_manager.h"
#include <fstream>

using namespace DearTs::Core::Config;
using DearTs::Core::Result;

/**
 * @brief Test fixture for ConfigManager tests
 */
class ConfigManagerTest : public ::testing::Test {
protected:
    // 使用测试实例级别的 counter 避免跨测试污染
    int m_test_counter = 0;

    // 每个测试开始前清除 ConfigManager 状态
    void SetUp() override {
        // 清除所有配置和元数据
        ConfigManager::instance().clear_for_test();
        // 清除所有全局回调
        ConfigManager::instance().clear_change_callbacks();
    }

    // 生成唯一测试键，使用测试实例隔离
    std::string get_unique_key(const std::string& suffix = "") {
        m_test_counter++;
        return "test_" + std::to_string(m_test_counter) + "_" + suffix;
    }
};

// ============================================================================
// Basic Operations Tests
// ============================================================================

TEST_F(ConfigManagerTest, SetGetInteger_ReturnsCorrectValue) {
    // Arrange
    const std::string key = get_unique_key("value");
    const int expected = 42;

    // Act
    auto set_result = ConfigManager::instance().set(key, expected);
    auto get_result = ConfigManager::instance().get<int>(key);

    // Assert
    ASSERT_TRUE(set_result.isOk()) << "Set should succeed";
    ASSERT_TRUE(get_result.isOk()) << "Get should succeed";
    EXPECT_EQ(get_result.unwrap(), expected);
}

TEST_F(ConfigManagerTest, SetGetString_ReturnsCorrectValue) {
    const std::string key = get_unique_key("message");
    const std::string expected = "Hello, World!";

    auto set_result = ConfigManager::instance().set(key, expected);
    auto get_result = ConfigManager::instance().get<std::string>(key);

    ASSERT_TRUE(set_result.isOk());
    ASSERT_TRUE(get_result.isOk());
    EXPECT_EQ(get_result.unwrap(), expected);
}

TEST_F(ConfigManagerTest, SetGetDouble_ReturnsCorrectValue) {
    const std::string key = get_unique_key("pi");
    const double expected = 3.14159;

    auto set_result = ConfigManager::instance().set(key, expected);
    auto get_result = ConfigManager::instance().get<double>(key);

    ASSERT_TRUE(set_result.isOk());
    ASSERT_TRUE(get_result.isOk());
    EXPECT_DOUBLE_EQ(get_result.unwrap(), expected);
}

TEST_F(ConfigManagerTest, SetGetBool_ReturnsCorrectValue) {
    const std::string key = get_unique_key("enabled");
    const bool expected = true;

    auto set_result = ConfigManager::instance().set(key, expected);
    auto get_result = ConfigManager::instance().get<bool>(key);

    ASSERT_TRUE(set_result.isOk());
    ASSERT_TRUE(get_result.isOk());
    EXPECT_EQ(get_result.unwrap(), expected);
}

TEST_F(ConfigManagerTest, GetNonExistentKey_ReturnsDefaultValue) {
    const std::string key = "non.existent.key";
    const int default_value = 99;

    auto result = ConfigManager::instance().get<int>(key, default_value);

    ASSERT_TRUE(result.isOk());
    EXPECT_EQ(result.unwrap(), default_value);
}

TEST_F(ConfigManagerTest, GetOr_ReturnsValueOrDefault) {
    const std::string key = get_unique_key("value");

    // Test with existing value
    ConfigManager::instance().set(key, 42);
    EXPECT_EQ(ConfigManager::instance().get_or<int>(key, 0), 42);

    // Test with non-existent key
    EXPECT_EQ(ConfigManager::instance().get_or<int>("non.existent", 99), 99);
}

// ============================================================================
// Type Mismatch Tests
// ============================================================================

TEST_F(ConfigManagerTest, GetTypeMismatch_ReturnsError) {
    const std::string key = get_unique_key("value");

    // Set as int
    ConfigManager::instance().set(key, 42);

    // Try to get as string
    auto result = ConfigManager::instance().get<std::string>(key);

    EXPECT_TRUE(result.isErr());
    EXPECT_TRUE(result.error().find("Type mismatch") != std::string::npos);
}

TEST_F(ConfigManagerTest, SetAndGetDifferentTypes_WorkCorrectly) {
    const std::string int_key = get_unique_key("int");
    const std::string str_key = get_unique_key("str");
    const std::string bool_key = get_unique_key("bool");

    ASSERT_TRUE(ConfigManager::instance().set(int_key, 42).isOk());
    ASSERT_TRUE(ConfigManager::instance().set(str_key, "hello").isOk());
    ASSERT_TRUE(ConfigManager::instance().set(bool_key, true).isOk());

    EXPECT_EQ(ConfigManager::instance().get<int>(int_key).unwrap(), 42);
    EXPECT_EQ(ConfigManager::instance().get<std::string>(str_key).unwrap(), "hello");
    EXPECT_EQ(ConfigManager::instance().get<bool>(bool_key).unwrap(), true);
}

// ============================================================================
// Has and Remove Tests
// ============================================================================

TEST_F(ConfigManagerTest, Has_ExistingKey_ReturnsTrue) {
    const std::string key = get_unique_key("value");
    ConfigManager::instance().set(key, 42);

    EXPECT_TRUE(ConfigManager::instance().has(key));
}

TEST_F(ConfigManagerTest, Has_NonExistentKey_ReturnsFalse) {
    EXPECT_FALSE(ConfigManager::instance().has("non.existent.key"));
}

TEST_F(ConfigManagerTest, Remove_ExistingKey_RemovesValue) {
    const std::string key = get_unique_key("value");
    ConfigManager::instance().set(key, 42);

    ConfigManager::instance().remove(key);

    EXPECT_FALSE(ConfigManager::instance().has(key));
    EXPECT_FALSE(ConfigManager::instance().get<int>(key).isOk());
}

TEST_F(ConfigManagerTest, Remove_NonExistentKey_DoesNothing) {
    // Should not throw or crash
    EXPECT_NO_THROW({
        ConfigManager::instance().remove("non.existent.key");
    });
}

// ============================================================================
// Metadata Tests
// ============================================================================

TEST_F(ConfigManagerTest, RegisterMeta_GetReturnsCorrectMeta) {
    const std::string key = get_unique_key("value");
    ConfigMeta meta{
        .description = "Test value",
        .default_value = 42,
        .is_required = false
    };

    ConfigManager::instance().register_meta(key, meta);

    auto result = ConfigManager::instance().get_meta(key);
    ASSERT_TRUE(result.isOk());
    auto retrieved_meta = result.unwrap();
    EXPECT_EQ(retrieved_meta.description, "Test value");
    EXPECT_TRUE(std::holds_alternative<int>(retrieved_meta.default_value));
    EXPECT_EQ(std::get<int>(retrieved_meta.default_value), 42);
    EXPECT_FALSE(retrieved_meta.is_required);
}

TEST_F(ConfigManagerTest, GetMeta_NonExistentKey_ReturnsError) {
    auto result = ConfigManager::instance().get_meta("non.existent.key");
    EXPECT_TRUE(result.isErr());
}

TEST_F(ConfigManagerTest, Get_UsesMetaDefaultValue) {
    const std::string key = get_unique_key("value");
    ConfigMeta meta{
        .description = "Test value",
        .default_value = 100,
        .is_required = false
    };

    ConfigManager::instance().register_meta(key, meta);

    // Get without setting value should return meta default
    auto result = ConfigManager::instance().get<int>(key);
    ASSERT_TRUE(result.isOk());
    EXPECT_EQ(result.unwrap(), 100);
}

// ============================================================================
// Validation Callback Tests
// ============================================================================

TEST_F(ConfigManagerTest, ValidateCallback_ValidValue_Succeeds) {
    const std::string key = get_unique_key("value");
    ConfigMeta meta;
    meta.description = "Test value";
    meta.default_value = 50;
    meta.is_required = false;
    meta.validate_callback = [](const ConfigValue& value) {
        if (std::holds_alternative<int>(value)) {
            int int_value = std::get<int>(value);
            if (int_value >= 0 && int_value <= 100) {
                return Result<void, std::string>::ok();
            }
            return Result<void, std::string>::err("Value must be between 0 and 100");
        }
        return Result<void, std::string>::err("Value must be an integer");
    };

    ConfigManager::instance().register_meta(key, meta);

    // Valid value
    auto result = ConfigManager::instance().set(key, 75);
    EXPECT_TRUE(result.isOk()) << "Valid value should pass validation";
}

TEST_F(ConfigManagerTest, ValidateCallback_InvalidValue_Fails) {
    const std::string key = get_unique_key("value");
    const ConfigMeta meta{
    .description = "Test value",
    .default_value = 50,
    .is_required = false,
    .validate_callback = [](const ConfigValue& value) {
        if (std::holds_alternative<int>(value)) {
            int int_value = std::get<int>(value);
            if (int_value >= 0 && int_value <= 100) {
                return Result<void, std::string>::ok();
            }
            return Result<void, std::string>::err("Value must be between 0 and 100");
        }
        return Result<void, std::string>::err("Value must be an integer");
        }
    };

    ConfigManager::instance().register_meta(key, meta);

    // Invalid value
    auto result = ConfigManager::instance().set(key, 150);
    EXPECT_TRUE(result.isErr()) << "Invalid value should fail validation";
    EXPECT_TRUE(result.error().find("Validation failed") != std::string::npos);
}

// ============================================================================
// Change Callback Tests
// ============================================================================

TEST_F(ConfigManagerTest, ChangeCallback_CalledOnValueChange) {
    const std::string key = get_unique_key("value");
    bool callback_called = false;
    ConfigValue new_value;

    ConfigMeta meta{
        .description = "Test value",
        .default_value = 0,
        .is_required = false,
        .change_callback = [&](const ConfigValue& value) {
            callback_called = true;
            new_value = value;
        }
    };
    

    ConfigManager::instance().register_meta(key, meta);
    ConfigManager::instance().set(key, 42);

    EXPECT_TRUE(callback_called);
    EXPECT_TRUE(std::holds_alternative<int>(new_value));
    EXPECT_EQ(std::get<int>(new_value), 42);
}

TEST_F(ConfigManagerTest, GlobalChangeCallback_CalledOnAnyChange) {
    bool callback_called = false;
    std::string captured_key;
    const std::string test_key = get_unique_key("value");

    ConfigManager::instance().add_change_callback(
        [&](const std::string& key, const ConfigValue& old_val, const ConfigValue& new_val) {
            callback_called = true;
            captured_key = key;
        }
    );

    ConfigManager::instance().set(test_key, 42);

    EXPECT_TRUE(callback_called);
    EXPECT_EQ(captured_key, test_key);
}

// ============================================================================
// Clear Tests
// ============================================================================

TEST_F(ConfigManagerTest, Clear_RemovesAllValues) {
    std::string prefix = get_unique_key("clear");
    ConfigManager::instance().set(prefix + "1", 1);
    ConfigManager::instance().set(prefix + "2", 2);
    ConfigManager::instance().set(prefix + "3", 3);

    EXPECT_TRUE(ConfigManager::instance().has(prefix + "1"));
    EXPECT_TRUE(ConfigManager::instance().has(prefix + "2"));
    EXPECT_TRUE(ConfigManager::instance().has(prefix + "3"));

    ConfigManager::instance().clear();

    EXPECT_FALSE(ConfigManager::instance().has(prefix + "1"));
    EXPECT_FALSE(ConfigManager::instance().has(prefix + "2"));
    EXPECT_FALSE(ConfigManager::instance().has(prefix + "3"));
}

// ============================================================================
// GetAllKeys Tests
// ============================================================================

TEST_F(ConfigManagerTest, GetAllKeys_ReturnsAllKeys) {
    std::string key1 = get_unique_key("1");
    std::string key2 = get_unique_key("2");
    std::string key3 = get_unique_key("3");

    ConfigManager::instance().set(key1, 1);
    ConfigManager::instance().set(key2, 2);
    ConfigManager::instance().set(key3, 3);

    auto keys = ConfigManager::instance().get_all_keys();

    EXPECT_EQ(keys.size(), 3);
    // Note: order might vary due to unordered_map
    std::sort(keys.begin(), keys.end());
    EXPECT_EQ(keys[0], key1);
    EXPECT_EQ(keys[1], key2);
    EXPECT_EQ(keys[2], key3);
}

TEST_F(ConfigManagerTest, GetAllKeys_Empty_ReturnsEmptyVector) {
    auto keys = ConfigManager::instance().get_all_keys();
    EXPECT_TRUE(keys.empty());
}

// ============================================================================
// Hierarchical Keys Tests
// ============================================================================

TEST_F(ConfigManagerTest, HierarchicalKeys_WorkCorrectly) {
    std::string prefix = get_unique_key("app");
    ConfigManager::instance().set(prefix + ".window.width", 1280);
    ConfigManager::instance().set(prefix + ".window.height", 720);
    ConfigManager::instance().set(prefix + ".theme.name", "dark");

    EXPECT_EQ(ConfigManager::instance().get<int>(prefix + ".window.width").unwrap(), 1280);
    EXPECT_EQ(ConfigManager::instance().get<int>(prefix + ".window.height").unwrap(), 720);
    EXPECT_EQ(ConfigManager::instance().get<std::string>(prefix + ".theme.name").unwrap(), "dark");
}

// ============================================================================
// ConfigScope Tests
// ============================================================================

TEST_F(ConfigManagerTest, ConfigScope_PrefixesKeysCorrectly) {
    std::string prefix = get_unique_key("app.window");
    ConfigScope scope(prefix);

    scope.set("width", 1280);
    scope.set("height", 720);

    // Keys should be prefixed
    std::string width_key = prefix + ".width";
    std::string height_key = prefix + ".height";
    EXPECT_TRUE(ConfigManager::instance().has(width_key));
    EXPECT_TRUE(ConfigManager::instance().has(height_key));

    EXPECT_EQ(ConfigManager::instance().get<int>(width_key).unwrap(), 1280);
    EXPECT_EQ(ConfigManager::instance().get<int>(height_key).unwrap(), 720);
}

TEST_F(ConfigManagerTest, ConfigScope_GetOr_PrefixedKeys) {
    std::string prefix = get_unique_key("app");
    ConfigScope scope(prefix);

    ConfigManager::instance().set(prefix + ".version", "1.0.0");

    EXPECT_EQ(scope.get_or<std::string>("version", "0.0.0"), "1.0.0");
}

TEST_F(ConfigManagerTest, ConfigScope_MakeKey_ReturnsCorrectKey) {
    ConfigScope scope("app.window");
    EXPECT_EQ(scope.make_key("width"), "app.window.width");
    EXPECT_EQ(scope.make_key("height"), "app.window.height");
}

// ============================================================================
// Thread Safety Tests (Basic)
// ============================================================================

TEST_F(ConfigManagerTest, ThreadSafety_ConcurrentSets_DoNotCrash) {
    const int num_threads = 10;
    const int operations_per_thread = 100;
    const std::string test_prefix = get_unique_key("thread");

    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([i, operations_per_thread, &test_prefix]() {
            for (int j = 0; j < operations_per_thread; ++j) {
                std::string key = test_prefix + "_" + std::to_string(i) + "_value_" + std::to_string(j);
                ConfigManager::instance().set(key, j);
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // Verify all values were set
    for (int i = 0; i < num_threads; ++i) {
        for (int j = 0; j < operations_per_thread; ++j) {
            std::string key = test_prefix + "_" + std::to_string(i) + "_value_" + std::to_string(j);
            EXPECT_TRUE(ConfigManager::instance().has(key));
        }
    }
}

// ============================================================================
// Persistence Tests (Basic)
// ============================================================================

TEST_F(ConfigManagerTest, SaveAndLoadToFile_PreservesValues) {
    // Create a temporary file with unique name
    std::string test_prefix = get_unique_key("config");
    std::filesystem::path temp_file = test_prefix + ".json";

    // Set some values
    std::string int_key = test_prefix + ".int";
    std::string str_key = test_prefix + ".str";
    std::string bool_key = test_prefix + ".bool";

    ConfigManager::instance().set(int_key, 42);
    ConfigManager::instance().set(str_key, "hello");
    ConfigManager::instance().set(bool_key, true);

    // Save to file
    auto save_result = ConfigManager::instance().save_to_file(temp_file);
    if (save_result.isOk()) {
        // Remove keys to test loading
        ConfigManager::instance().remove(int_key);
        ConfigManager::instance().remove(str_key);
        ConfigManager::instance().remove(bool_key);

        EXPECT_FALSE(ConfigManager::instance().has(int_key));

        // Load from file
        auto load_result = ConfigManager::instance().load_from_file(temp_file);
        if (load_result.isOk()) {
            EXPECT_EQ(ConfigManager::instance().get<int>(int_key).unwrap(), 42);
            EXPECT_EQ(ConfigManager::instance().get<std::string>(str_key).unwrap(), "hello");
            EXPECT_EQ(ConfigManager::instance().get<bool>(bool_key).unwrap(), true);
        }

        // Clean up
        std::filesystem::remove(temp_file);
    }
}

TEST_F(ConfigManagerTest, LoadFromNonExistentFile_ReturnsError) {
    auto result = ConfigManager::instance().load_from_file("non_existent_file_12345.json");
    EXPECT_TRUE(result.isErr());
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(ConfigManagerTest, EmptyKey_HandledCorrectly) {
    // Empty key should be valid (though unusual)
    auto result = ConfigManager::instance().set("", 42);
    // Behavior depends on implementation - just ensure it doesn't crash
    EXPECT_NO_THROW({
        ConfigManager::instance().set("", 42);
    });
}

TEST_F(ConfigManagerTest, VeryLongKey_HandledCorrectly) {
    std::string long_key(1000, 'a');
    EXPECT_NO_THROW({
        ConfigManager::instance().set(long_key, 42);
    });
}

TEST_F(ConfigManagerTest, SpecialCharactersInKey_WorkCorrectly) {
    EXPECT_NO_THROW({
        ConfigManager::instance().set("test.key-with_special.chars", 42);
    });
}

TEST_F(ConfigManagerTest, OverwriteValue_Succeeds) {
    const std::string key = get_unique_key("value");

    ConfigManager::instance().set(key, 10);
    EXPECT_EQ(ConfigManager::instance().get<int>(key).unwrap(), 10);

    ConfigManager::instance().set(key, 20);
    EXPECT_EQ(ConfigManager::instance().get<int>(key).unwrap(), 20);
}
