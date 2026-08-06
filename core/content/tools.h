/**
 * @file tools.h
 * @brief 工具注册表
 * @details 独立的工具管理模块
 * @author DearTs Team
 * @date 2025
 * @version 1.0.0
 */

#pragma once

#include "registry_base.h"
#include <vector>
#include <string>
#include <functional>

namespace DearTs::Core::ContentRegistry::Tools {

/**
 * @brief 工具项
 */
struct ToolItem {
    UnlocalizedString name;
    std::string description;

    // 工具回调
    Callback callback;
};

/**
 * @brief 工具注册表类
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
     * @brief 添加工具
     * @param name 工具名称
     * @param description 工具描述
     * @param callback 工具回调
     * @return 工具项引用
     */
    ToolItem& add(const UnlocalizedString& name,
                  const std::string& description,
                  Callback callback);

    /**
     * @brief 获取所有工具
     */
    const std::vector<ToolItem>& get_all() const;

    /**
     * @brief 清空所有工具
     */
    void clear();

private:
    Registry() = default;
    ~Registry() = default;

    std::vector<ToolItem> m_tools;
};

/**
 * @brief 便捷函数：添加工具
 */
inline ToolItem& add(const UnlocalizedString& name,
                    const std::string& description,
                    Callback callback) {
    return Registry::instance().add(name, description, callback);
}

/**
 * @brief 便捷函数：获取所有工具
 */
inline const auto& get_all() {
    return Registry::instance().get_all();
}

} // namespace DearTs::Core::ContentRegistry::Tools
