/**
 * @file live2d_settings_widget.hpp
 * @brief Live2D 设置界面组件
 * @details 提供 Live2D 模型的配置界面
 * @author DearTs Team
 * @date 2025
 * @version 1.0.0
 */

#pragma once

#include "plugins/settings/include/widgets/base_settings_widget.hpp"
#include "live2d_plugin.hpp"
#include <string>

namespace DearTs::Plugins::Live2D {

/**
 * @brief Live2D 设置界面组件
 *
 * 提供 Live2D 相关的设置功能：
 * - 模型设置（目录、活动模型、缩放、位置、透明度）
 * - 渲染设置（FBO、性能分析）
 * - 动画设置（动作速度、呼吸、眨眼、物理）
 * - 交互设置（鼠标跟随、拖拽）
 * - 调试设置（显示碰撞区域、显示参数）
 */
class Live2DSettingsWidget : public ::DearTs::Plugins::Settings::BaseSettingsWidget {
public:
    /**
     * @brief 构造函数
     * @param plugin Live2D 插件指针
     */
    explicit Live2DSettingsWidget(Live2DPlugin* plugin);

    /**
     * @brief 析构函数
     */
    ~Live2DSettingsWidget() override = default;

    /**
     * @brief 渲染设置界面
     */
    void render() override;

private:
    /**
     * @brief 渲染模型设置部分
     */
    void render_model_settings();

    /**
     * @brief 渲染渲染设置部分
     */
    void render_rendering_settings();

    /**
     * @brief 渲染动画设置部分
     */
    void render_animation_settings();

    /**
     * @brief 渲染交互设置部分
     */
    void render_interaction_settings();

    /**
     * @brief 渲染调试设置部分
     */
    void render_debug_settings();

    /**
     * @brief 获取位置名称
     * @param position 位置枚举值
     * @return 位置名称字符串
     */
    static const char* get_position_name(Live2DModelPosition position);

private:
    Live2DPlugin* m_plugin;  ///< Live2D 插件指针（非拥有）
};

} // namespace DearTs::Plugins::Live2D
