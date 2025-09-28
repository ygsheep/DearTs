/**
 * DearTs Configuration Manager - Simplified Version
 * 
 * 简化的配置管理器 - 只提供基本的配置加载和访问功能
 * 
 * @author DearTs Team
 * @version 1.0.0
 * @date 2025
 */

#pragma once

#ifndef DEARTS_CONFIG_MANAGER_H
#define DEARTS_CONFIG_MANAGER_H

// 直接包含必要的标准库头文件以避免预编译头文件问题
#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <type_traits>

namespace DearTs {
namespace Core {
namespace Utils {

/**
 * @brief 简化的配置管理器类
 * 
 * 提供基本的配置存储和访问功能
 */
class ConfigManager {
public:
    template<typename T>
    T getValue(const std::string &key, const T &defaultValue = T()) const {
        if constexpr (std::is_same_v<T, std::string>)
            return getString(key, defaultValue);
        else if constexpr (std::is_same_v<T, int>)
            return getInt(key, defaultValue);
        else if constexpr (std::is_same_v<T, bool>)
            return getBool(key, defaultValue);
        else if constexpr (std::is_same_v<T, double>)
            return getDouble(key, defaultValue);
        return defaultValue;
    }

    template<typename T>
    void setValue(const std::string &key, const T &value) {
        if constexpr (std::is_same_v<T, std::string>)
            setString(key, value);
        else if constexpr (std::is_same_v<T, int>)
            setInt(key, value);
        else if constexpr (std::is_same_v<T, bool>)
            setBool(key, value);
        else if constexpr (std::is_same_v<T, double>)
            setDouble(key, value);
    }

    void saveToFile(const std::string &path);
public:
    /**
     * @brief 获取单例实例
     * @return ConfigManager实例引用
     */
    static ConfigManager& getInstance();
    
    /**
     * @brief 从文件加载配置
     * @param filename 配置文件路径
     * @return 是否加载成功
     */
    bool loadFromFile(const std::string& filename);
    
    /**
     * @brief 获取字符串配置值
     * @param key 配置键
     * @param default_value 默认值
     * @return 配置值
     */
    std::string getString(const std::string& key, const std::string& default_value = "") const;
    
    /**
     * @brief 获取整数配置值
     * @param key 配置键
     * @param default_value 默认值
     * @return 配置值
     */
    int getInt(const std::string& key, int default_value = 0) const;
    
    /**
     * @brief 获取布尔配置值
     * @param key 配置键
     * @param default_value 默认值
     * @return 配置值
     */
    bool getBool(const std::string& key, bool default_value = false) const;
    
    /**
     * @brief 获取浮点配置值
     * @param key 配置键
     * @param default_value 默认值
     * @return 配置值
     */
    double getDouble(const std::string& key, double default_value = 0.0) const;
    
    /**
     * @brief 设置字符串配置值
     * @param key 配置键
     * @param value 配置值
     */
    void setString(const std::string& key, const std::string& value);
    
    /**
     * @brief 设置整数配置值
     * @param key 配置键
     * @param value 配置值
     */
    void setInt(const std::string& key, int value);
    
    /**
     * @brief 设置布尔配置值
     * @param key 配置键
     * @param value 配置值
     */
    void setBool(const std::string& key, bool value);
    
    /**
     * @brief 设置浮点配置值
     * @param key 配置键
     * @param value 配置值
     */
    void setDouble(const std::string& key, double value);
    
    /**
     * @brief 检查配置键是否存在
     * @param key 配置键
     * @return 是否存在
     */
    bool exists(const std::string& key) const;
    
    /**
     * @brief 清空所有配置
     */
    void clear();

private:
    ConfigManager() = default;
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    
    std::unordered_map<std::string, std::string> config_data_;  ///< 配置数据存储
    mutable std::mutex mutex_;                                   ///< 线程安全锁
};

} // namespace Utils
} // namespace Core
} // namespace DearTs

#endif // DEARTS_CONFIG_MANAGER_H