/**
 * @file config_manager.h
 * @brief 配置管理器
 * @details 提供统一的配置管理接口，支持分层配置、类型安全、持久化
 * @author DearTs Team
 * @date 2025
 * @version 1.0.0
 */

#pragma once

#include "core/result.h"
#include <string>
#include <variant>
#include <unordered_map>
#include <filesystem>
#include <functional>
#include <mutex>

namespace DearTs::Core::Config {

/**
 * @brief 配置值类型
 */
using ConfigValue = std::variant<
    bool,
    int,
    double,
    std::string
>;

/**
 * @brief 配置项元数据
 */
struct ConfigMeta {
    std::string description;        ///< 配置项描述
    ConfigValue default_value;      ///< 默认值
    bool is_required;               ///< 是否必需

    /**
     * @brief 验证回调
     * @return 成功返回 void，失败返回错误信息
     */
    using ValidateCallback = std::function<Result<void, std::string>(const ConfigValue&)>;
    ValidateCallback validate_callback;

    /**
     * @brief 变更回调
     */
    using ChangeCallback = std::function<void(const ConfigValue&)>;
    ChangeCallback change_callback;
};

/**
 * @brief 配置管理器类
 *
 * @example
 * // 设置配置值
 * ConfigManager::instance().set("app.window.width", 1280);
 *
 * // 获取配置值
 * auto width = ConfigManager::instance().get<int>("app.window.width").unwrap_or(1280);
 *
 * // 注册配置元数据
 * ConfigManager::instance().register_meta("app.window.width", {
 *     .description = "Window width",
 *     .default_value = 1280,
 *     .is_required = false
 * });
 */
class ConfigManager {
public:
    /**
     * @brief 获取单例实例
     */
    static ConfigManager& instance();

    /**
     * @brief 设置配置值
     * @tparam T 值类型
     * @param key 配置键（支持点号分隔的层级，如 "app.window.width"）
     * @param value 配置值
     * @return 成功返回 void，失败返回错误信息
     */
    template<typename T>
    Result<void, std::string> set(const std::string& key, T value) {
        std::lock_guard<std::mutex> lock(m_mutex);

        ConfigValue config_value = value;

        // 验证配置值
        auto it = m_metas.find(key);
        if (it != m_metas.end()) {
            auto& meta = it->second;
            if (meta.validate_callback) {
                auto result = meta.validate_callback(config_value);
                if (result.isErr()) {
                    return Result<void, std::string>::err(
                        std::format("Validation failed for '{}': {}", key, result.error())
                    );
                }
            }
        }

        // 设置值
        auto old_value_it = m_values.find(key);
        ConfigValue old_value = (old_value_it != m_values.end())
            ? old_value_it->second
            : config_value;

        m_values[key] = config_value;

        // 触发变更回调
        if (it != m_metas.end() && it->second.change_callback) {
            it->second.change_callback(config_value);
        }

        // 触发全局变更回调
        for (auto& callback : m_global_change_callbacks) {
            callback(key, old_value, config_value);
        }

        return Result<void, std::string>::ok();
    }

    /**
     * @brief 获取配置值
     * @tparam T 值类型
     * @param key 配置键
     * @param default_value 默认值
     * @return 成功返回配置值，失败返回默认值
     */
    template<typename T>
    [[nodiscard]] Result<T, std::string> get(const std::string& key, T default_value = T{}) const {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_values.find(key);
        if (it == m_values.end()) {
            // 尝试从元数据获取默认值
            auto meta_it = m_metas.find(key);
            if (meta_it != m_metas.end()) {
                if (std::holds_alternative<T>(meta_it->second.default_value)) {
                    return Result<T, std::string>::ok(std::get<T>(meta_it->second.default_value));
                }
            }
            return Result<T, std::string>::ok(default_value);
        }

        if (std::holds_alternative<T>(it->second)) {
            return Result<T, std::string>::ok(std::get<T>(it->second));
        }

        return Result<T, std::string>::err(
            std::format("Type mismatch for config key '{}'", key)
        );
    }

    /**
     * @brief 获取配置值（带默认值）
     * @tparam T 值类型
     * @param key 配置键
     * @param default_value 默认值
     * @return 配置值
     */
    template<typename T>
    [[nodiscard]] T get_or(const std::string& key, T default_value) const {
        auto result = get<T>(key);
        if (result.isOk()) {
            return result.unwrap();
        }
        return default_value;
    }

    /**
     * @brief 检查配置键是否存在
     */
    [[nodiscard]] bool has(const std::string& key) const;

    /**
     * @brief 删除配置项
     */
    void remove(const std::string& key);

    /**
     * @brief 注册配置元数据
     * @param key 配置键
     * @param meta 配置元数据
     */
    void register_meta(const std::string& key, ConfigMeta meta);

    /**
     * @brief 获取配置元数据
     */
    [[nodiscard]] Result<ConfigMeta, std::string> get_meta(const std::string& key) const;

    /**
     * @brief 从文件加载配置
     * @param path 文件路径（支持 JSON、TOML 等格式）
     * @return 成功返回 void，失败返回错误信息
     */
    Result<void, std::string> load_from_file(const std::filesystem::path& path);

    /**
     * @brief 保存配置到文件
     * @param path 文件路径
     * @return 成功返回 void，失败返回错误信息
     */
    Result<void, std::string> save_to_file(const std::filesystem::path& path);

    /**
     * @brief 添加全局变更回调
     * @param callback 回调函数，参数为 (key, old_value, new_value)
     */
    void add_change_callback(std::function<void(const std::string&, const ConfigValue&, const ConfigValue&)> callback);

    /**
     * @brief 清空所有配置
     */
    void clear();

    /**
     * @brief 获取所有配置键
     */
    [[nodiscard]] std::vector<std::string> get_all_keys() const;

private:
    ConfigManager() = default;
    ~ConfigManager() = default;

    // 删除拷贝和移动
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    ConfigManager(ConfigManager&&) = delete;
    ConfigManager& operator=(ConfigManager&&) = delete;

    mutable std::mutex m_mutex;
    std::unordered_map<std::string, ConfigValue> m_values;
    std::unordered_map<std::string, ConfigMeta> m_metas;
    std::vector<std::function<void(const std::string&, const ConfigValue&, const ConfigValue&)>> m_global_change_callbacks;
};

/**
 * @brief 配置作用域守卫（RAII）
 * @details 自动管理配置作用域的生命周期
 */
class ConfigScope {
public:
    explicit ConfigScope(const std::string& prefix);
    ~ConfigScope();

    // 删除拷贝
    ConfigScope(const ConfigScope&) = delete;
    ConfigScope& operator=(const ConfigScope&) = delete;

    /**
     * @brief 获取完整配置键
     */
    [[nodiscard]] std::string make_key(const std::string& key) const;

    /**
     * @brief 设置配置值
     */
    template<typename T>
    Result<void, std::string> set(const std::string& key, T value) {
        return ConfigManager::instance().set(make_key(key), std::move(value));
    }

    /**
     * @brief 获取配置值
     */
    template<typename T>
    [[nodiscard]] Result<T, std::string> get(const std::string& key, T default_value = T{}) const {
        return ConfigManager::instance().get<T>(make_key(key), default_value);
    }

    /**
     * @brief 获取配置值（带默认值）
     */
    template<typename T>
    [[nodiscard]] T get_or(const std::string& key, T default_value) const {
        return ConfigManager::instance().get_or(make_key(key), default_value);
    }

private:
    std::string m_prefix;
};

} // namespace DearTs::Core::Config
