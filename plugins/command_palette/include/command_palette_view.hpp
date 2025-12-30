/**
 * @file command_palette_view.hpp
 * @brief 命令面板视图
 * @details 命令面板的UI实现
 * @author DearTs Team
 * @date 2025
 * @version 1.0.0
 */

#pragma once

#include "core/ui/view.h"
#include "core/content/commands.h"
#include "core/content/registry_base.h"
#include "core/ui/icon_font.hpp"
#include <string>
#include <vector>

namespace DearTs::Plugins::CommandPalette {

using namespace DearTs::Core;
using namespace ContentRegistry;

/**
 * @brief 命令面板视图类
 *
 * 提供命令面板的UI界面：
 * - 模态窗口
 * - 搜索输入框
 * - 命令列表
 * - 键盘导航
 */
class CommandPaletteView : public UI::ViewModal {
public:
    /**
     * @brief 构造函数
     */
    CommandPaletteView()
        : ViewModal(UnlocalizedString("命令面板"), ICON_SEARCH)
        , m_selected_index(0)
        , m_just_opened(true)
    {
        m_search_buffer[0] = '\0';
    }

    /**
     * @brief 析构函数
     */
    ~CommandPaletteView() override = default;

    /**
     * @brief 获取最小窗口大小
     */
    [[nodiscard]] ImVec2 get_min_size() const override {
        return ImVec2(400, 300);
    }

    /**
     * @brief 获取最大窗口大小
     */
    [[nodiscard]] ImVec2 get_max_size() const override {
        return ImVec2(800, 600);
    }

    /**
     * @brief 不在侧边栏菜单中显示
     */
    [[nodiscard]] bool has_view_menu_entry() const override {
        return false;
    }

    /**
     * @brief 获取窗口标志（无标题栏、不可移动、不可调整大小）
     */
    [[nodiscard]] ImGuiWindowFlags get_window_flags() const override {
        return ImGuiWindowFlags_NoTitleBar |          // 无标题栏
               ImGuiWindowFlags_NoMove |              // 不可移动
               ImGuiWindowFlags_NoResize |            // 不可调整大小
               ImGuiWindowFlags_NoCollapse;           // 不可折叠
    }

    /**
     * @brief 绘制视图（覆盖父类以实现自定义布局）
     */
    void draw(ImGuiWindowFlags extra_flags = ImGuiWindowFlags_None) override;

protected:
    /**
     * @brief 绘制内容
     */
    void draw_content() override;

    /**
     * @brief 窗口打开时调用
     */
    void on_open() override;

    /**
     * @brief 窗口关闭时调用
     */
    void on_close() override;

private:
    /**
     * @brief 渲染命令列表
     */
    void render_command_list();

    /**
     * @brief 渲染单个命令项
     */
    bool render_command_item(const ContentRegistry::Commands::CommandItem& command,
                             bool is_selected);

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

private:
    char m_search_buffer[256];                                              ///< 搜索缓冲区
    std::vector<ContentRegistry::Commands::CommandItem> m_filtered_commands; ///< 过滤后的命令
    int m_selected_index;                                                   ///< 当前选中索引
    bool m_just_opened;                                                     ///< 刚打开标记
};

} // namespace DearTs::Plugins::CommandPalette
