/**
 * @file command_palette.h
 * @brief 命令面板 UI 组件
 * @details 提供类似 VS Code 的命令面板功能
 * @author DearTs Team
 * @date 2024
 * @version 1.0.0
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include "core/content/commands.h"
#include <imgui.h>

namespace DearTs::Core::UI {

/**
 * @brief 命令面板样式配置
 */
struct CommandPaletteStyle {
    ImVec4 popup_background = ImVec4(0.13f, 0.13f, 0.13f, 0.95f);  ///< 弹出窗口背景色
    ImVec4 item_hover = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);         ///< 悬停背景色
    ImVec4 item_active = ImVec4(0.26f, 0.59f, 0.98f, 0.60f);        ///< 选中背景色
    ImVec4 text_normal = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);        ///< 普通文本颜色
    ImVec4 text_dim = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);           ///< 暗淡文本颜色
    ImVec4 text_highlight = ImVec4(0.52f, 0.78f, 0.52f, 1.00f);     ///< 高亮文本颜色
    ImVec4 shortcut_text = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);      ///< 快捷键文本颜色
    ImVec4 separator = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);          ///< 分隔线颜色
    float rounding = 4.0f;                                          ///< 圆角半径
    float padding = 8.0f;                                           ///< 内边距
    float item_height = 32.0f;                                      ///< 列表项高度
};

/**
 * @brief 命令面板类
 *
 * @example
 * // 在应用中创建命令面板
 * auto palette = std::make_unique<CommandPalette>();
 *
 * // 注册命令
 * ContentRegistry::Commands::add("file.open", "Open File", []() {
 *     // 打开文件逻辑
 * });
 *
 * // 在渲染循环中
 * if (ImGui::IsKeyPressed(ImGuiKey_P) && ImGui::GetIO().KeyCtrl) {
 *     palette->open();
 * }
 * palette->render();
 */
class CommandPalette {
public:
    /**
     * @brief 构造函数
     */
    CommandPalette();

    /**
     * @brief 析构函数
     */
    ~CommandPalette() = default;

    /**
     * @brief 打开命令面板
     */
    void open();

    /**
     * @brief 关闭命令面板
     */
    void close();

    /**
     * @brief 切换命令面板状态
     */
    void toggle();

    /**
     * @brief 渲染命令面板
     * @details 应该在每帧调用
     */
    void render();

    /**
     * @brief 设置快捷键
     * @param key ImGui 键码
     * @param ctrl 是否需要 Ctrl 键
     * @param shift 是否需要 Shift 键
     * @param alt 是否需要 Alt 键
     */
    void set_shortcut(int key, bool ctrl = false, bool shift = false, bool alt = false);

    /**
     * @brief 检查是否应该打开
     */
    bool check_shortcut();

    /**
     * @brief 获取是否打开
     */
    [[nodiscard]] bool is_open() const { return m_is_open; }

    /**
     * @brief 设置样式
     */
    void set_style(const CommandPaletteStyle& style) { m_style = style; }

    /**
     * @brief 获取样式
     */
    [[nodiscard]] const CommandPaletteStyle& get_style() const { return m_style; }

    /**
     * @brief 设置占位符文本
     */
    void set_placeholder(const std::string& placeholder) { m_placeholder = placeholder; }

    /**
     * @brief 设置标题
     */
    void set_title(const std::string& title) { m_title = title; }

private:
    /**
     * @brief 渲染命令列表
     */
    void render_command_list();

    /**
     * @brief 渲染单个命令项
     */
    bool render_command_item(const ContentRegistry::Commands::CommandItem& command,
                             bool is_selected,
                             const std::string& search_query);

    /**
     * @brief 高亮搜索匹配的文本
     */
    void render_highlighted_text(const std::string& text, const std::string& query);

    /**
     * @brief 执行选中的命令
     */
    void execute_selected_command();

    /**
     * @brief 移动选择
     */
    void move_selection(int delta);

    /**
     * @brief 处理键盘导航
     */
    void handle_keyboard_navigation();

    /**
     * @brief 过滤命令
     */
    void filter_commands();

    /**
     * @brief 计算匹配分数
     * @return 匹配分数，越高表示匹配度越高
     */
    int calculate_match_score(const std::string& text, const std::string& query);

private:
    bool m_is_open = false;
    int m_selected_index = 0;
    char m_search_buffer[256] = "";
    std::vector<ContentRegistry::Commands::CommandItem> m_filtered_commands;
    CommandPaletteStyle m_style;

    // 快捷键配置
    int m_shortcut_key = ImGuiKey_P;
    bool m_shortcut_ctrl = true;
    bool m_shortcut_shift = false;
    bool m_shortcut_alt = false;

    // UI 文本
    std::string m_title = "Command Palette";
    std::string m_placeholder = "Type a command or search...";
};

} // namespace DearTs::Core::UI
