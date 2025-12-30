/**
 * @file dearts_application.hpp
 * @brief DearTs 应用程序主类
 * @details 基于 DearTs 框架的 DearTs 应用，集成 ImGui、主题、快捷键等功能
 * @author DearTs Team
 * @date 2024
 * @version 1.0.0
 */

#pragma once

#include "core/app/application.h"
#include "core/ui/theme_manager.h"
#include "core/ui/shortcut_manager.h"
#include "core/ui/command_palette.h"
#include "core/ui/title_bar.h"
#include "core/ui/view_manager.h"
#include "core/ui/icon_font.hpp"
#include "core/ui/task_widget.h"
#include "core/tasks/task_manager.h"
#include "core/plugin/plugin.h"
#include "core/config/config_manager.h"
#include <memory>
#include <vector>

namespace DearTs::Main::GUI {

/**
 * @brief DearTs 应用配置
 */
struct DearTsConfig {
    float frame_padding_x = 8.0f;
    float frame_padding_y = 10.0f;
    float title_left_margin = 10.0f;
    float button_width = 30.0f;
    float button_height = 30.0f;  // 按钮高度
    float button_spacing = 6.0f;
    float button_right_margin = 10.0f;
    int button_count = 3;

    float get_title_bar_height() const {
        return frame_padding_y * 2.0f + 16.0f;
    }

    bool is_in_button_area(float x, float window_width) const {
        float button_area_width = button_count * button_width +
                                  (button_count - 1) * button_spacing +
                                  button_right_margin;
        return x >= (window_width - button_area_width);
    }
};

/**
 * @brief DearTs 应用程序类
 *
 * @details 提供完整的 DearTs 应用功能：
 * - ImGui 集成
 * - 主题切换
 * - 快捷键系统
 * - 命令面板
 * - 视图管理
 * - 自定义标题栏
 * - 配置管理
 */
class DearTsApplication : public Core::App::Application {
public:
    DearTsApplication() = default;
    ~DearTsApplication() override = default;

protected:
    /**
     * @brief 初始化应用
     */
    bool on_init() override;

    /**
     * @brief 更新逻辑
     */
    void on_update(double delta_time) override;

    /**
     * @brief 渲染界面
     */
    void on_render() override;

    /**
     * @brief 处理事件
     */
    void on_event(const SDL_Event& event) override;

    /**
     * @brief 关闭应用
     */
    void on_shutdown() override;

private:
    /**
     * @brief 设置 ImGui
     */
    bool setup_imgui();

    /**
     * @brief 设置配置管理器
     */
    void setup_config();

    /**
     * @brief 设置事件监听器
     */
    void setup_events();

    /**
     * @brief 设置命令和工具
     */
    void setup_commands_and_tools();

    /**
     * @brief 设置快捷键
     */
    void setup_shortcuts();

    /**
     * @brief 设置插件
     */
    void setup_plugins();

    /**
     * @brief 设置视图
     */
    void setup_views();

    /**
     * @brief 渲染菜单栏
     */
    void render_menu_bar();

    /**
     * @brief 渲染主窗口
     */
    void render_main_window();

    /**
     * @brief 渲染工具窗口
     */
    void render_tool_windows();

    /**
     * @brief 渲染自定义标题栏
     * @return 标题栏高度
     */
    float render_title_bar();

    /**
     * @brief 创建和渲染 DockSpace
     * @param title_bar_height 标题栏高度
     */
    void render_dock_space(float title_bar_height);

    /**
     * @brief 渲染所有视图
     */
    void render_views();

private:
    // ImGui 相关
    ImGuiContext* m_imgui_context = nullptr;

    // UI 组件
    DearTsConfig m_title_bar_config;
    Core::UI::TitleBar m_title_bar;
    std::unique_ptr<Core::UI::CommandPalette> m_command_palette;

    // 窗口状态
    bool m_show_demo_window = false;
    bool m_show_metrics_window = false;
    bool m_show_about_window = false;
    bool m_show_style_editor = false;
    bool m_show_task_plugin_window = false;

    // 窗口拖动状态
    bool m_is_dragging = false;
    bool m_is_maximized = false;  // 窗口是否最大化
    ImVec2 m_drag_start_pos;
    ImVec2 m_window_start_pos;

    // 统计信息
    int m_last_log_time = 0;
};

} // namespace DearTs::Main::GUI
