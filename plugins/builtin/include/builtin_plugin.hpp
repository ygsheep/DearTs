/**
 * @file builtin_plugin.hpp
 * @brief 内置插件声明
 * @details DearTs 的默认插件，提供基础 UI 组件和功能
 */

#pragma once

#include "core/plugin/plugin.h"

namespace DearTs::Plugins::Builtin {

/**
 * @brief 内置插件类
 *
 * 提供 DearTs 的默认功能：
 * - Hello World 视图
 * - Data Inspector 视图
 */
class BuiltinPlugin : public Core::Plugin::IPlugin {
public:
    /**
     * @brief 获取插件信息
     */
    Core::Plugin::PluginInfo get_info() const override;

    /**
     * @brief 插件加载时调用
     */
    Core::Result<void, std::string> on_load() override;

    /**
     * @brief 插件卸载时调用
     */
    void on_unload() override;

    /**
     * @brief 插件启用时调用
     */
    void on_enable() override;

    /**
     * @brief 插件禁用时调用
     */
    void on_disable() override;

private:
    /**
     * @brief 注册所有视图
     */
    void register_views();
};

} // namespace DearTs::Plugins::Builtin
