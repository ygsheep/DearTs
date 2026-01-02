/**
 * @file commands.h
 * @brief 命令注册表
 * @details 独立的命令面板管理模块
 * @author DearTs Team
 * @date 2025
 * @version 1.0.0
 */

#pragma once

#include "registry_base.h"
#include <vector>
#include <string>
#include <functional>

namespace DearTs::Core::ContentRegistry::Commands {

/**
 * @brief 命令项（用于命令面板）
 */
struct CommandItem {
    UnlocalizedString name;
    std::string description;
    std::string shortcut; // 快捷键

    // 命令回调
    Callback callback;

    // 命令是否启用
    using EnabledCallback = std::function<bool()>;
    EnabledCallback enabled_callback;
};

/**
 * @brief 命令注册表类
 */
class Registry final {  // 单例类，禁止继承
public:
    /**
     * @brief 获取单例实例（线程安全，Magic Statics）
     */
    static Registry& instance() noexcept {
        static Registry instance;
        return instance;
    }

    // 删除所有拷贝和移动操作
    Registry(const Registry&) = delete;
    Registry& operator=(const Registry&) = delete;
    Registry(Registry&&) = delete;
    Registry& operator=(Registry&&) = delete;

    /**
     * @brief 添加命令
     * @param name 命令名称
     * @param description 命令描述
     * @param callback 命令回调
     * @return 命令项引用，可继续配置快捷键和启用条件
     */
    CommandItem& add(const UnlocalizedString& name,
                     const std::string& description,
                     Callback callback);

    /**
     * @brief 获取所有命令
     */
    const std::vector<CommandItem>& get_all() const;

    /**
     * @brief 执行命令
     * @param name 命令名称
     * @return 成功返回 true，失败返回 false
     */
    bool execute(const UnlocalizedString& name);

    /**
     * @brief 搜索命令
     * @param query 搜索查询
     * @return 匹配的命令列表
     */
    std::vector<CommandItem> search(const std::string& query) const;

    /**
     * @brief 清空所有命令
     */
    void clear();

private:
    Registry() = default;
    ~Registry() = default;

    std::vector<CommandItem> m_commands;
};

/**
 * @brief 便捷函数：添加命令
 */
inline CommandItem& add(const UnlocalizedString& name,
                       const std::string& description,
                       Callback callback) {
    return Registry::instance().add(name, description, callback);
}

/**
 * @brief 便捷函数：获取所有命令
 */
inline const auto& get_all() {
    return Registry::instance().get_all();
}

/**
 * @brief 便捷函数：执行命令
 */
inline bool execute(const UnlocalizedString& name) {
    return Registry::instance().execute(name);
}

/**
 * @brief 便捷函数：搜索命令
 */
inline std::vector<CommandItem> search(const std::string& query) {
    return Registry::instance().search(query);
}

} // namespace DearTs::Core::ContentRegistry::Commands
