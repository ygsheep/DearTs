/**
 * @file live2d_settings_view.hpp
 * @brief Live2D 设置视图
 * @details Live2D 插件的专用设置界面
 * @author DearTs Team
 * @date 2025
 * @version 1.0.0
 */

#pragma once

#include "core/ui/view.h"
#include "widgets/live2d_settings_widget.hpp"
#include <memory>

namespace DearTs::Plugins::Live2D {

/**
 * @brief Live2D 设置视图
 *
 * 提供完整的 Live2D 模型配置界面：
 * - 模型设置（目录、活动模型、缩放、位置、偏移、透明度）
 * - 渲染设置（FBO、降采样、性能分析）
 * - 动画设置（自动播放、速度、呼吸、眨眼、物理）
 * - 交互设置（鼠标跟随、拖拽）
 * - 调试设置（显示碰撞区域、参数列表）
 */
class Live2DSettingsView : public Core::UI::ViewWindow {
public:
    /**
     * @brief 构造函数
     * @param plugin Live2D 插件指针
     */
    explicit Live2DSettingsView(Live2DPlugin* plugin);

    /**
     * @brief 析构函数
     */
    ~Live2DSettingsView() override = default;

    /**
     * @brief 绘制视图内容
     */
    void draw_content() override;

    /**
     * @brief 获取最小窗口大小
     */
    ImVec2 get_min_size() const override {
        return ImVec2(500, 600);
    }

private:
    /**
     * @brief 绘制顶部工具栏
     */
    void draw_toolbar();

    /**
     * @brief 保存配置
     */
    void save_config();

    /**
     * @brief 重新加载配置
     */
    void reload_config();

private:
    Live2DPlugin* m_plugin;  ///< Live2D 插件指针（非拥有）
    std::unique_ptr<Live2DSettingsWidget> m_widget;  ///< 设置组件
};

} // namespace DearTs::Plugins::Live2D
