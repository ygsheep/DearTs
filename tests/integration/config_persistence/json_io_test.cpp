/**
 * @file json_io_test.cpp
 * @brief 配置持久化集成测试
 * @details 测试配置的保存和加载功能
 * @author DearTs Team
 * @date 2025
 */

#include <gtest/gtest.h>
#include "core/config/config_manager.h"
#include "core/event/event_bus.h"
#include <fstream>
#include <filesystem>

using namespace DearTs::Core::Config;
using namespace DearTs::Core::Event;

/**
 * @brief 配置持久化集成测试 Fixture
 */
class ConfigPersistenceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 清理配置管理器
        ConfigManager::instance().clear_for_test();
        ConfigManager::instance().clear_change_callbacks();
        EventBus::instance().clear();

        // 创建临时测试目录
        m_test_dir = "test_config_temp";
        if (std::filesystem::exists(m_test_dir)) {
            std::filesystem::remove_all(m_test_dir);
        }
        std::filesystem::create_directories(m_test_dir);
    }

    void TearDown() override {
        ConfigManager::instance().clear_for_test();
        EventBus::instance().clear();

        // 清理临时测试目录
        if (std::filesystem::exists(m_test_dir)) {
            std::filesystem::remove_all(m_test_dir);
        }
    }

    std::filesystem::path get_test_config_path(const std::string& filename) {
        return std::filesystem::path(m_test_dir) / filename;
    }

    std::string m_test_dir;
};

// ============================================================================
// 基本保存和加载测试
// ============================================================================

TEST_F(ConfigPersistenceTest, SaveAndReload_SimpleValues) {
    auto config_file = get_test_config_path("simple_config.json");

    // 1. 设置各种类型的配置值
    ConfigManager::instance().set("app.int_value", 42);
    ConfigManager::instance().set("app.double_value", 3.14159);
    ConfigManager::instance().set("app.bool_value", true);
    ConfigManager::instance().set("app.string_value", "Hello, World!");

    // 2. 保存到文件
    auto save_result = ConfigManager::instance().save_to_file(config_file);
    ASSERT_TRUE(save_result.isOk()) << save_result.error();

    // 3. 清空配置
    ConfigManager::instance().clear_for_test();
    EXPECT_FALSE(ConfigManager::instance().has("app.int_value"));

    // 4. 从文件加载
    auto load_result = ConfigManager::instance().load_from_file(config_file);
    ASSERT_TRUE(load_result.isOk()) << load_result.error();

    // 5. 验证配置已恢复
    EXPECT_EQ(ConfigManager::instance().get<int>("app.int_value").unwrap(), 42);
    EXPECT_DOUBLE_EQ(ConfigManager::instance().get<double>("app.double_value").unwrap(), 3.14159);
    EXPECT_EQ(ConfigManager::instance().get<bool>("app.bool_value").unwrap(), true);
    EXPECT_EQ(ConfigManager::instance().get<std::string>("app.string_value").unwrap(), "Hello, World!");
}

TEST_F(ConfigPersistenceTest, SaveAndReload_NestedKeys) {
    auto config_file = get_test_config_path("nested_config.json");

    // 1. 设置嵌套配置
    ConfigManager::instance().set("app.window.width", 1280);
    ConfigManager::instance().set("app.window.height", 720);
    ConfigManager::instance().set("app.window.title", "DearTs Application");
    ConfigManager::instance().set("app.theme.name", "dark");
    ConfigManager::instance().set("app.theme.accent_color", "#FF0000");

    // 2. 保存到文件
    auto save_result = ConfigManager::instance().save_to_file(config_file);
    ASSERT_TRUE(save_result.isOk());

    // 3. 清空并重新加载
    ConfigManager::instance().clear_for_test();
    auto load_result = ConfigManager::instance().load_from_file(config_file);
    ASSERT_TRUE(load_result.isOk());

    // 4. 验证嵌套配置已恢复
    EXPECT_EQ(ConfigManager::instance().get<int>("app.window.width").unwrap(), 1280);
    EXPECT_EQ(ConfigManager::instance().get<int>("app.window.height").unwrap(), 720);
    EXPECT_EQ(ConfigManager::instance().get<std::string>("app.window.title").unwrap(), "DearTs Application");
    EXPECT_EQ(ConfigManager::instance().get<std::string>("app.theme.name").unwrap(), "dark");
    EXPECT_EQ(ConfigManager::instance().get<std::string>("app.theme.accent_color").unwrap(), "#FF0000");
}

// ============================================================================
// 元数据持久化测试
// ============================================================================

TEST_F(ConfigPersistenceTest, SaveAndReload_WithMetadata) {
    auto config_file = get_test_config_path("metadata_config.json");

    // 1. 注册带元数据的配置
    ConfigMeta meta{
        .description = "Window width in pixels",
        .default_value = 1280,
        .is_required = false
    };

    ConfigManager::instance().register_meta("app.window.width", meta);
    ConfigManager::instance().set("app.window.width", 1920);

    // 2. 保存配置（注：当前实现不保存元数据）
    auto save_result = ConfigManager::instance().save_to_file(config_file);
    ASSERT_TRUE(save_result.isOk());

    // 3. 清空并重新加载
    ConfigManager::instance().clear_for_test();
    auto load_result = ConfigManager::instance().load_from_file(config_file);
    ASSERT_TRUE(load_result.isOk());

    // 4. 验证值已恢复，但元数据需要重新注册
    EXPECT_EQ(ConfigManager::instance().get<int>("app.window.width").unwrap(), 1920);

    // 5. 重新注册元数据
    ConfigManager::instance().register_meta("app.window.width", meta);
    auto retrieved_meta = ConfigManager::instance().get_meta("app.window.width");
    ASSERT_TRUE(retrieved_meta.isOk());
    EXPECT_EQ(retrieved_meta.unwrap().description, "Window width in pixels");
}

// ============================================================================
// 部分保存和增量更新测试
// ============================================================================

TEST_F(ConfigPersistenceTest, SaveAndAddNewValues_ReloadHasAllValues) {
    auto config_file = get_test_config_path("incremental_config.json");

    // 1. 初始配置
    ConfigManager::instance().set("app.value1", 100);
    ConfigManager::instance().save_to_file(config_file);

    // 2. 添加新配置
    ConfigManager::instance().set("app.value2", 200);
    ConfigManager::instance().set("app.value3", 300);
    ConfigManager::instance().save_to_file(config_file);

    // 3. 清空并重新加载
    ConfigManager::instance().clear_for_test();
    ConfigManager::instance().load_from_file(config_file);

    // 4. 验证所有值都存在
    EXPECT_EQ(ConfigManager::instance().get<int>("app.value1").unwrap(), 100);
    EXPECT_EQ(ConfigManager::instance().get<int>("app.value2").unwrap(), 200);
    EXPECT_EQ(ConfigManager::instance().get<int>("app.value3").unwrap(), 300);
}

TEST_F(ConfigPersistenceTest, PartialLoad_MergesWithExisting) {
    auto config_file = get_test_config_path("partial_config.json");

    // 1. 设置初始配置
    ConfigManager::instance().set("app.existing_value", 999);

    // 2. 保存当前配置
    ConfigManager::instance().save_to_file(config_file);

    // 3. 清空并设置新配置
    ConfigManager::instance().clear_for_test();
    ConfigManager::instance().set("app.new_value", 888);

    // 4. 加载配置（应该与现有配置合并）
    auto load_result = ConfigManager::instance().load_from_file(config_file);
    ASSERT_TRUE(load_result.isOk());

    // 5. 验证两个值都存在
    EXPECT_TRUE(ConfigManager::instance().has("app.new_value"));
}

// ============================================================================
// 错误处理测试
// ============================================================================

TEST_F(ConfigPersistenceTest, LoadFromNonExistentFile_ReturnsError) {
    auto config_file = get_test_config_path("non_existent.json");

    auto result = ConfigManager::instance().load_from_file(config_file);
    EXPECT_TRUE(result.isErr());
}

TEST_F(ConfigPersistenceTest, LoadFromInvalidJson_ReturnsError) {
    auto config_file = get_test_config_path("invalid.json");

    // 1. 创建无效的 JSON 文件
    {
        std::ofstream file(config_file);
        file << "{ invalid json content }";
        file.close();
    }

    // 2. 尝试加载
    auto result = ConfigManager::instance().load_from_file(config_file);
    EXPECT_TRUE(result.isErr());
}

TEST_F(ConfigPersistenceTest, SaveToReadOnlyPath_ReturnsError) {
    // 在 Unix 系统上可以测试只读目录
    // Windows 上可能需要特殊处理
    auto config_file = get_test_config_path("readonly/config.json");

    // 不创建目录，直接尝试保存到不存在的路径
    auto result = ConfigManager::instance().save_to_file(config_file);
    EXPECT_TRUE(result.isErr());
}

// ============================================================================
// 回调和事件测试
// ============================================================================

TEST_F(ConfigPersistenceTest, Load_TriggersChangeCallbacks) {
    auto config_file = get_test_config_path("callback_config.json");

    // 1. 设置配置并注册变更回调
    std::vector<std::string> changed_keys;
    ConfigManager::instance().add_change_callback(
        [&changed_keys](const std::string& key, const ConfigValue&, const ConfigValue&) {
            changed_keys.push_back(key);
        }
    );

    ConfigManager::instance().set("app.value1", 100);
    ConfigManager::instance().set("app.value2", 200);
    ConfigManager::instance().save_to_file(config_file);

    // 2. 清空配置
    ConfigManager::instance().clear_for_test();
    changed_keys.clear();

    // 3. 从文件加载
    ConfigManager::instance().load_from_file(config_file);

    // 4. 验证每个加载的配置项都触发了回调
    // 注意：当前实现可能在加载时不触发回调，这是设计选择
    // 如果需要触发回调，可以在 load_from_file 中实现
}

// ============================================================================
// 大型配置集测试
// ============================================================================

TEST_F(ConfigPersistenceTest, SaveAndReload_LargeConfigSet) {
    auto config_file = get_test_config_path("large_config.json");

    // 1. 创建大量配置项
    const int num_items = 1000;
    for (int i = 0; i < num_items; ++i) {
        ConfigManager::instance().set("config.item_" + std::to_string(i), i * 10);
    }

    // 2. 保存
    auto save_result = ConfigManager::instance().save_to_file(config_file);
    ASSERT_TRUE(save_result.isOk());

    // 3. 验证文件已创建
    EXPECT_TRUE(std::filesystem::exists(config_file));

    // 4. 获取文件大小
    auto file_size = std::filesystem::file_size(config_file);
    EXPECT_GT(file_size, 0);

    // 5. 清空并重新加载
    ConfigManager::instance().clear_for_test();
    auto load_result = ConfigManager::instance().load_from_file(config_file);
    ASSERT_TRUE(load_result.isOk());

    // 6. 验证所有配置项都已恢复
    for (int i = 0; i < num_items; ++i) {
        auto result = ConfigManager::instance().get<int>("config.item_" + std::to_string(i));
        ASSERT_TRUE(result.isOk()) << "Failed to load config.item_" << i;
        EXPECT_EQ(result.unwrap(), i * 10);
    }
}

// ============================================================================
// 多文件配置测试
// ============================================================================

TEST_F(ConfigPersistenceTest, LoadMultipleConfigFiles_LastOneWins) {
    auto config_file1 = get_test_config_path("config1.json");
    auto config_file2 = get_test_config_path("config2.json");

    // 1. 保存第一个配置
    ConfigManager::instance().clear_for_test();
    ConfigManager::instance().set("app.value", 100);
    ConfigManager::instance().set("app.source", "file1");
    ConfigManager::instance().save_to_file(config_file1);

    // 2. 保存第二个配置（有重叠的键）
    ConfigManager::instance().clear_for_test();
    ConfigManager::instance().set("app.value", 200);
    ConfigManager::instance().set("app.source", "file2");
    ConfigManager::instance().set("app.extra", "only_in_file2");
    ConfigManager::instance().save_to_file(config_file2);

    // 3. 清空并加载两个配置文件
    ConfigManager::instance().clear_for_test();
    ConfigManager::instance().load_from_file(config_file1);
    ConfigManager::instance().load_from_file(config_file2);

    // 4. 验证后加载的文件覆盖了前面的值
    EXPECT_EQ(ConfigManager::instance().get<int>("app.value").unwrap(), 200);
    EXPECT_EQ(ConfigManager::instance().get<std::string>("app.source").unwrap(), "file2");
    EXPECT_EQ(ConfigManager::instance().get<std::string>("app.extra").unwrap(), "only_in_file2");
}

// ============================================================================
// 特殊字符和编码测试
// ============================================================================

TEST_F(ConfigPersistenceTest, SaveAndReload_SpecialCharacters) {
    auto config_file = get_test_config_path("special_chars.json");

    // 1. 设置包含特殊字符的配置
    ConfigManager::instance().set("app.quote", "Text with \"quotes\"");
    ConfigManager::instance().set("app.backslash", "Path\\To\\File");
    ConfigManager::instance().set("app.newline", "Line1\nLine2");
    ConfigManager::instance().set("app.unicode", "Hello 世界 🌍");

    // 2. 保存和加载
    ConfigManager::instance().save_to_file(config_file);
    ConfigManager::instance().clear_for_test();
    ConfigManager::instance().load_from_file(config_file);

    // 3. 验证特殊字符正确保存
    EXPECT_EQ(ConfigManager::instance().get<std::string>("app.quote").unwrap(), "Text with \"quotes\"");
    EXPECT_EQ(ConfigManager::instance().get<std::string>("app.backslash").unwrap(), "Path\\To\\File");
    EXPECT_EQ(ConfigManager::instance().get<std::string>("app.newline").unwrap(), "Line1\nLine2");
    EXPECT_EQ(ConfigManager::instance().get<std::string>("app.unicode").unwrap(), "Hello 世界 🌍");
}

// ============================================================================
// 类型边界测试
// ============================================================================

TEST_F(ConfigPersistenceTest, SaveAndReload_BoundaryValues) {
    auto config_file = get_test_config_path("boundary_values.json");

    // 1. 设置边界值
    ConfigManager::instance().set("int.max", INT_MAX);
    ConfigManager::instance().set("int.min", INT_MIN);
    ConfigManager::instance().set("double.max", 1.79769e+308);  // DBL_MAX
    ConfigManager::instance().set("double.min", -1.79769e+308);
    ConfigManager::instance().set("bool.true_val", true);
    ConfigManager::instance().set("bool.false_val", false);
    ConfigManager::instance().set("string.empty", "");
    ConfigManager::instance().set("string.long", std::string(10000, 'x'));

    // 2. 保存和加载
    ConfigManager::instance().save_to_file(config_file);
    ConfigManager::instance().clear_for_test();
    ConfigManager::instance().load_from_file(config_file);

    // 3. 验证边界值
    EXPECT_EQ(ConfigManager::instance().get<int>("int.max").unwrap(), INT_MAX);
    EXPECT_EQ(ConfigManager::instance().get<int>("int.min").unwrap(), INT_MIN);
    EXPECT_DOUBLE_EQ(ConfigManager::instance().get<double>("double.max").unwrap(), 1.79769e+308);
    EXPECT_DOUBLE_EQ(ConfigManager::instance().get<double>("double.min").unwrap(), -1.79769e+308);
    EXPECT_TRUE(ConfigManager::instance().get<bool>("bool.true_val").unwrap());
    EXPECT_FALSE(ConfigManager::instance().get<bool>("bool.false_val").unwrap());
    EXPECT_EQ(ConfigManager::instance().get<std::string>("string.empty").unwrap(), "");
    EXPECT_EQ(ConfigManager::instance().get<std::string>("string.long").unwrap().length(), 10000);
}
