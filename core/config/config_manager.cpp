/**
 * @file config_manager.cpp
 * @brief 配置管理器实现
 */

#include "core/config/config_manager.h"
#include "liblogger/logger.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>

namespace DearTs::Core::Config {

ConfigManager& ConfigManager::instance() {
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::has(const std::string& key) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_values.find(key) != m_values.end();
}

void ConfigManager::remove(const std::string& key) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_values.erase(key);
    LOG_INFO("Removed config: {}", key);
}

void ConfigManager::register_meta(const std::string& key, ConfigMeta meta) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_metas[key] = std::move(meta);
    LOG_INFO("Registered config meta: {}", key);
}

Result<ConfigMeta, std::string> ConfigManager::get_meta(const std::string& key) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_metas.find(key);
    if (it == m_metas.end()) {
        return Result<ConfigMeta, std::string>::err(
            std::format("Config meta '{}' not found", key)
        );
    }
    return Result<ConfigMeta, std::string>::ok(it->second);
}

Result<void, std::string> ConfigManager::load_from_file(const std::filesystem::path& path) {
    std::lock_guard<std::mutex> lock(m_mutex);

    LOG_INFO("Loading config from: {}", path.string());

    // 检查文件是否存在
    if (!std::filesystem::exists(path)) {
        LOG_WARN("Config file not found: {}", path.string());
        return Result<void, std::string>::err("File not found");
    }

    // 打开文件
    std::ifstream file(path);
    if (!file.is_open()) {
        return Result<void, std::string>::err("Failed to open file");
    }

    try {
        // 使用 nlohmann/json 解析
        nlohmann::json j;
        file >> j;
        file.close();

        int loaded_count = 0;

        // 遍历 JSON 对象，转换为 ConfigValue
        for (auto& [key, value] : j.items()) {
            if (value.is_boolean()) {
                m_values[key] = value.get<bool>();
                loaded_count++;
            } else if (value.is_number_integer()) {
                m_values[key] = value.get<int>();
                loaded_count++;
            } else if (value.is_number()) {
                m_values[key] = value.get<double>();
                loaded_count++;
            } else if (value.is_string()) {
                m_values[key] = value.get<std::string>();
                loaded_count++;
            } else {
                LOG_WARN("Unsupported JSON type for key: {}", key);
            }
        }

        LOG_INFO("Loaded {} config items from {}", loaded_count, path.string());
        return Result<void, std::string>::ok();

    } catch (const nlohmann::json::parse_error& e) {
        file.close();
        return Result<void, std::string>::err(std::format("JSON parse error: {}", e.what()));
    } catch (const nlohmann::json::exception& e) {
        file.close();
        return Result<void, std::string>::err(std::format("JSON error: {}", e.what()));
    } catch (const std::exception& e) {
        file.close();
        return Result<void, std::string>::err(std::format("Error: {}", e.what()));
    }
}

Result<void, std::string> ConfigManager::save_to_file(const std::filesystem::path& path) {
    std::lock_guard<std::mutex> lock(m_mutex);

    LOG_INFO("Saving config to: {}", path.string());

    try {
        // 创建 JSON 对象
        nlohmann::json j;

        // 将 ConfigValue 转换为 JSON
        for (const auto& [key, value] : m_values) {
            if (std::holds_alternative<bool>(value)) {
                j[key] = std::get<bool>(value);
            } else if (std::holds_alternative<int>(value)) {
                j[key] = std::get<int>(value);
            } else if (std::holds_alternative<double>(value)) {
                j[key] = std::get<double>(value);
            } else if (std::holds_alternative<std::string>(value)) {
                j[key] = std::get<std::string>(value);
            }
        }

        // 打开文件并写入（使用缩进格式化）
        std::ofstream file(path);
        if (!file.is_open()) {
            return Result<void, std::string>::err("Failed to create file");
        }

        file << j.dump(4);  // 4 空格缩进
        file.close();

        LOG_INFO("Saved {} config items to {}", m_values.size(), path.string());
        return Result<void, std::string>::ok();

    } catch (const nlohmann::json::exception& e) {
        return Result<void, std::string>::err(std::format("JSON error: {}", e.what()));
    } catch (const std::exception& e) {
        return Result<void, std::string>::err(std::format("Error: {}", e.what()));
    }
}

void ConfigManager::add_change_callback(
    std::function<void(const std::string&, const ConfigValue&, const ConfigValue&)> callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_global_change_callbacks.push_back(std::move(callback));
}

void ConfigManager::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_values.clear();
    m_metas.clear();
}

std::vector<std::string> ConfigManager::get_all_keys() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::string> keys;
    keys.reserve(m_values.size());
    for (const auto& [key, _] : m_values) {
        keys.push_back(key);
    }
    return keys;
}

// ================ ConfigScope ================

ConfigScope::ConfigScope(const std::string& prefix)
    : m_prefix(prefix) {
    if (!m_prefix.empty() && m_prefix.back() != '.') {
        m_prefix += '.';
    }
}

ConfigScope::~ConfigScope() = default;

std::string ConfigScope::make_key(const std::string& key) const {
    return m_prefix + key;
}

} // namespace DearTs::Core::Config
