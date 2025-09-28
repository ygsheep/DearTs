/**
 * DearTs Configuration Manager - Simplified Implementation
 * 
 * 简化的配置管理器实现 - 只提供基本的配置加载和访问功能
 * 
 * @author DearTs Team
 * @version 1.0.0
 * @date 2025
 */

#include "config_manager.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace DearTs {
namespace Core {
namespace Utils {

/**
 * @brief 获取单例实例
 * @return ConfigManager实例引用
 */
ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

/**
 * @brief 从文件加载配置
 * @param filename 配置文件路径
 * @return 是否加载成功
 */
bool ConfigManager::loadFromFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        // 跳过空行和注释行
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }
        
        // 查找等号分隔符
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            
            // 去除首尾空白
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            if (!key.empty()) {
                config_data_[key] = value;
            }
        }
    }
    
    return true;
}

/**
 * @brief 获取字符串配置值
 * @param key 配置键
 * @param default_value 默认值
 * @return 配置值
 */
std::string ConfigManager::getString(const std::string& key, const std::string& default_value) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = config_data_.find(key);
    if (it != config_data_.end()) {
        return it->second;
    }
    return default_value;
}

/**
 * @brief 获取整数配置值
 * @param key 配置键
 * @param default_value 默认值
 * @return 配置值
 */
int ConfigManager::getInt(const std::string& key, int default_value) const {
    std::string value = getString(key);
    if (value.empty()) {
        return default_value;
    }
    
    try {
        return std::stoi(value);
    } catch (const std::exception&) {
        return default_value;
    }
}

/**
 * @brief 获取布尔配置值
 * @param key 配置键
 * @param default_value 默认值
 * @return 配置值
 */
bool ConfigManager::getBool(const std::string& key, bool default_value) const {
    std::string value = getString(key);
    if (value.empty()) {
        return default_value;
    }
    
    // 转换为小写
    std::transform(value.begin(), value.end(), value.begin(), ::tolower);
    
    if (value == "true" || value == "1" || value == "yes" || value == "on") {
        return true;
    } else if (value == "false" || value == "0" || value == "no" || value == "off") {
        return false;
    }
    
    return default_value;
}

/**
 * @brief 获取浮点配置值
 * @param key 配置键
 * @param default_value 默认值
 * @return 配置值
 */
double ConfigManager::getDouble(const std::string& key, double default_value) const {
    std::string value = getString(key);
    if (value.empty()) {
        return default_value;
    }
    
    try {
        return std::stod(value);
    } catch (const std::exception&) {
        return default_value;
    }
}

/**
 * @brief 设置字符串配置值
 * @param key 配置键
 * @param value 配置值
 */
void ConfigManager::setString(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_data_[key] = value;
}

/**
 * @brief 设置整数配置值
 * @param key 配置键
 * @param value 配置值
 */
void ConfigManager::setInt(const std::string& key, int value) {
    setString(key, std::to_string(value));
}

/**
 * @brief 设置布尔配置值
 * @param key 配置键
 * @param value 配置值
 */
void ConfigManager::setBool(const std::string& key, bool value) {
    setString(key, value ? "true" : "false");
}

/**
 * @brief 设置浮点配置值
 * @param key 配置键
 * @param value 配置值
 */
void ConfigManager::setDouble(const std::string& key, double value) {
    setString(key, std::to_string(value));
}

/**
 * @brief 检查配置键是否存在
 * @param key 配置键
 * @return 是否存在
 */
bool ConfigManager::exists(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return config_data_.find(key) != config_data_.end();
}

/**
 * @brief 清空所有配置
 */
void ConfigManager::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    config_data_.clear();
}

/**
 * @brief 保存配置到文件
 * @param path 文件路径
 */
void ConfigManager::saveToFile(const std::string &path) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::ofstream file(path);
    if (!file.is_open()) {
        return;
    }
    
    for (const auto& [key, value] : config_data_) {
        file << key << "=" << value << "\n";
    }
}

} // namespace Utils
} // namespace Core
} // namespace DearTs