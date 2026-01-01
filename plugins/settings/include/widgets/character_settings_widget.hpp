/**
 * @file character_settings_widget.hpp
 * @brief 角色选择设置组件
 * @details 提供角色选择和配置界面
 */

#pragma once

#include "core/config/config_manager.h"
#include <string>
#include <vector>

namespace DearTs::Plugins::Settings {

/**
 * @brief 角色设置组件
 *
 * 负责渲染和管理角色相关的设置项：
 * - 角色选择（下拉列表）
 * - 角色位置选择
 * - 缩放比例调整
 * - 透明度调整
 * - 动画模式选择（仅动画模式）
 */
class CharacterSettingsWidget {
public:
    CharacterSettingsWidget() = default;
    ~CharacterSettingsWidget() = default;

    /**
     * @brief 渲染角色设置面板
     */
    void render();

    /**
     * @brief 获取已修改的配置键列表
     */
    [[nodiscard]] const std::vector<std::string>& get_modified_keys() const {
        return m_modified_keys;
    }

    /**
     * @brief 清空已修改配置键列表
     */
    void clear_modified_keys() {
        m_modified_keys.clear();
    }

private:
    /**
     * @brief 渲染角色选择区域
     */
    void render_character_selection();

    /**
     * @brief 渲染角色位置选择
     */
    void render_position_selection();

    /**
     * @brief 渲染角色缩放和透明度
     */
    void render_scale_and_opacity();

    /**
     * @brief 渲染动画设置（仅动画模式）
     */
    void render_animation_settings();

    /**
     * @brief 标记配置项为已修改
     */
    void mark_modified(const std::string& key);

private:
    std::vector<std::string> m_modified_keys;  ///< 已修改的配置键
};

} // namespace DearTs::Plugins::Settings
