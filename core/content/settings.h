/**
 * @file settings.h
 * @brief 设置注册表
 * @details 独立的设置管理模块
 * @author DearTs Team
 * @date 2025
 * @version 1.0.0
 */

#pragma once

#include "registry_base.h"
#include <map>
#include <string>
#include <functional>
#include "core/result.h"

namespace DearTs::Core::ContentRegistry::Settings {

/**
 * @brief 设置项
 */
struct SettingItem {
    UnlocalizedString name;
    std::string value;
    std::string default_value;

    // 验证回调
    using ValidateCallback = std::function<bool(const std::string&)>;
    ValidateCallback validate_callback;

    // 变更回调
    using ChangeCallback = std::function<void(const std::string&)>;
    ChangeCallback change_callback;
};

/**
 * @brief 设置注册表类
 */
class Registry {
public:
    /**
     * @brief 获取单例实例
     */
    static Registry& instance();

    /**
     * @brief 添加设置项
     * @param category 设置类别
     * @param name 设置名称
     * @param default_value 默认值
     * @return 设置项引用，可继续配置验证和变更回调
     */
    SettingItem& add(const UnlocalizedString& category,
                     const UnlocalizedString& name,
                     const std::string& default_value);

    /**
     * @brief 获取设置值
     * @param category 设置类别
     * @param name 设置名称
     * @return 设置值
     */
    Result<std::string, std::string> get(const UnlocalizedString& category,
                                        const UnlocalizedString& name);

    /**
     * @brief 设置值
     * @param category 设置类别
     * @param name 设置名称
     * @param value 新值
     * @return 成功返回 void，失败返回错误信息
     */
    Result<void, std::string> set(const UnlocalizedString& category,
                                  const UnlocalizedString& name,
                                  const std::string& value);

    /**
     * @brief 获取所有设置
     */
    const std::map<UnlocalizedString, std::map<UnlocalizedString, SettingItem>>& get_all() const;

    /**
     * @brief 清空所有设置
     */
    void clear();

private:
    Registry() = default;
    ~Registry() = default;

    // 删除拷贝和移动
    Registry(const Registry&) = delete;
    Registry& operator=(const Registry&) = delete;
    Registry(Registry&&) = delete;
    Registry& operator=(Registry&&) = delete;

    std::map<UnlocalizedString, std::map<UnlocalizedString, SettingItem>> m_settings;
};

/**
 * @brief 便捷函数：添加设置项
 */
inline SettingItem& add(const UnlocalizedString& category,
                       const UnlocalizedString& name,
                       const std::string& default_value) {
    return Registry::instance().add(category, name, default_value);
}

/**
 * @brief 便捷函数：获取设置值
 */
inline Result<std::string, std::string> get(const UnlocalizedString& category,
                                           const UnlocalizedString& name) {
    return Registry::instance().get(category, name);
}

/**
 * @brief 便捷函数：设置值
 */
inline Result<void, std::string> set(const UnlocalizedString& category,
                                     const UnlocalizedString& name,
                                     const std::string& value) {
    return Registry::instance().set(category, name, value);
}

/**
 * @brief 便捷函数：获取所有设置
 */
inline const auto& get_all() {
    return Registry::instance().get_all();
}

} // namespace DearTs::Core::ContentRegistry::Settings
