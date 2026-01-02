/**
 * @file settings.cpp
 * @brief 设置注册表实现
 */

#include "core/content/settings.h"
#include "liblogger/logger.h"
#include <algorithm>

namespace DearTs::Core::ContentRegistry::Settings {

SettingItem& Registry::add(const UnlocalizedString& category,
                           const UnlocalizedString& name,
                           const std::string& default_value) {
    auto& category_map = m_settings[category];

    // 使用 try_emplace 或 insert 避免默认构造
    auto [it, inserted] = category_map.try_emplace(name);
    auto& item = it->second;

    if (inserted) {
        item.name = name;
        item.default_value = default_value;
        item.value = default_value;
        LOG_INFO("Added setting: {}.{} = {}", category.get(), name.get(), default_value);
    }

    return item;
}

Result<std::string, std::string> Registry::get(const UnlocalizedString& category,
                                                const UnlocalizedString& name) {
    auto cat_it = m_settings.find(category);
    if (cat_it == m_settings.end()) {
        return Result<std::string, std::string>::err(
            std::format("Category '{}' not found", category.get())
        );
    }

    auto& category_map = cat_it->second;
    auto item_it = category_map.find(name);
    if (item_it == category_map.end()) {
        return Result<std::string, std::string>::err(
            std::format("Setting '{}.{}' not found", category.get(), name.get())
        );
    }

    return Result<std::string, std::string>::ok(item_it->second.value);
}

Result<void, std::string> Registry::set(const UnlocalizedString& category,
                                        const UnlocalizedString& name,
                                        const std::string& value) {
    auto cat_it = m_settings.find(category);
    if (cat_it == m_settings.end()) {
        return Result<void, std::string>::err(
            std::format("Category '{}' not found", category.get())
        );
    }

    auto& category_map = cat_it->second;
    auto item_it = category_map.find(name);
    if (item_it == category_map.end()) {
        return Result<void, std::string>::err(
            std::format("Setting '{}.{}' not found", category.get(), name.get())
        );
    }

    auto& item = item_it->second;

    // 验证
    if (item.validate_callback) {
        if (!item.validate_callback(value)) {
            return Result<void, std::string>::err(
                std::format("Validation failed for '{}.{}'", category.get(), name.get())
            );
        }
    }

    // 设置值
    item.value = value;

    // 变更回调
    if (item.change_callback) {
        item.change_callback(value);
    }

    LOG_INFO("Set setting: {}.{} = {}", category.get(), name.get(), value);
    return Result<void, std::string>::ok();
}

const std::map<UnlocalizedString, std::map<UnlocalizedString, SettingItem>>&
Registry::get_all() const {
    return m_settings;
}

void Registry::clear() {
    m_settings.clear();
}

} // namespace DearTs::Core::ContentRegistry::Settings
