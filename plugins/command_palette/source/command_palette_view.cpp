/**
 * @file command_palette_view.cpp
 * @brief 命令面板视图实现
 */

#include "command_palette_view.hpp"
#include "liblogger/logger.h"
#include <imgui.h>
#include <algorithm>

using namespace DearTs::Core;
using namespace ContentRegistry;

namespace DearTs::Plugins::CommandPalette {

void CommandPaletteView::on_open() {
    LOG_INFO("CommandPaletteView::on_open() - Window opening!");
    m_search_buffer[0] = '\0';
    m_selected_index = 0;
    m_just_opened = true;
    filter_commands();
}

void CommandPaletteView::on_close() {
    LOG_INFO("命令面板已关闭");
}

void CommandPaletteView::draw(ImGuiWindowFlags extra_flags) {
    if (!m_window_open) {
        return;
    }

    // 获取视口信息
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 work_pos = viewport->WorkPos;
    ImVec2 work_size = viewport->WorkSize;

    // 计算居中位置，稍微偏上 2%
    float window_width = 600;  // 固定宽度
    ImVec2 window_pos(
        work_pos.x + (work_size.x - window_width) * 0.5f,
        work_pos.y + work_size.y * 0.02f  // 偏上 2%
    );

    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always);
    ImGui::SetNextWindowSizeConstraints(
        ImVec2(window_width, 200),   // 最小：宽度固定，高度最小 200
        ImVec2(window_width, 600)    // 最大：宽度固定，高度最大 600
    );
    ImGui::SetNextWindowSize(ImVec2(window_width, 0), ImGuiCond_FirstUseEver);  // 初始高度自动适应

    // 获取窗口标志
    auto flags = get_window_flags() | extra_flags;

    auto window_name = View::to_window_name(m_unlocalized_name);

    // 先渲染半透明遮罩背景
    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
    ImU32 dim_color = ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.3f));  // 30% 不透明度的黑色
    ImVec2 work_end(work_pos.x + work_size.x, work_pos.y + work_size.y);  // 计算右下角
    draw_list->AddRectFilled(work_pos, work_end, dim_color);

    // 渲染窗口
    bool was_window_hovered = false;
    if (ImGui::Begin(window_name.c_str(), &m_window_open, flags)) {
        m_focused = ImGui::IsWindowFocused();
        was_window_hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
        draw_content();
    }

    ImGui::End();

    // 检测点击遮罩关闭窗口
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        if (!was_window_hovered) {
            m_window_open = false;
            LOG_DEBUG("点击遮罩关闭命令面板");
        }
    }

    // 手动跟踪状态
    m_window_just_opened = (m_window_open && !m_prev_window_open);
    m_window_just_closed = (!m_window_open && m_prev_window_open);

    if (m_window_just_opened) {
        on_open();
    } else if (m_window_just_closed) {
        on_close();
    }

    m_prev_window_open = m_window_open;
}

void CommandPaletteView::draw_content() {
    // 搜索框
    if (m_just_opened) {
        ImGui::SetKeyboardFocusHere(0);
        m_just_opened = false;
    }

    ImGui::PushItemWidth(-1);
    bool input_submitted = ImGui::InputTextWithHint(
        "##search",
        " 输入命令或搜索...",
        m_search_buffer,
        sizeof(m_search_buffer),
        ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue
    );
    ImGui::PopItemWidth();

    // 过滤命令
    if (input_submitted) {
        filter_commands();
        m_selected_index = 0;

        // 如果按了Enter，执行选中命令
        if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) {
            execute_selected_command();
        }
    }

    // 实时过滤（输入时）
    if (ImGui::IsItemEdited()) {
        filter_commands();
        m_selected_index = 0;
    }

    // 处理键盘导航
    handle_keyboard_navigation();

    // 分隔线
    ImGui::Separator();

    // 命令列表
    render_command_list();
}

void CommandPaletteView::render_command_list() {
    using namespace ContentRegistry::Commands;

    if (m_filtered_commands.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::Text("未找到命令");
        ImGui::PopStyleColor();
        return;
    }

    // 列表
    ImVec2 list_size = ImGui::GetContentRegionAvail();
    list_size.y = std::min(list_size.y, 400.0f);

    if (ImGui::BeginListBox("##commands", list_size)) {
        for (int i = 0; i < m_filtered_commands.size(); i++) {
            bool is_selected = (i == static_cast<size_t>(m_selected_index));

            ImGui::PushID(i);
            if (is_selected) {
                ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.26f, 0.59f, 0.98f, 0.6f));
            }

            bool clicked = ImGui::Selectable("##command_item", is_selected);

            if (is_selected) {
                ImGui::PopStyleColor();
            }

            ImGui::SameLine();

            // 渲染命令名称和描述
            ImGui::BeginGroup();
            render_highlighted_text(m_filtered_commands[i].name.get(), m_search_buffer);

            // 显示快捷键
            if (!m_filtered_commands[i].shortcut.empty()) {
                ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::CalcTextSize(m_filtered_commands[i].shortcut.c_str()).x - 20.0f);
                ImGui::TextDisabled("%s", m_filtered_commands[i].shortcut.c_str());
            } else {
                ImGui::SameLine();
                ImGui::TextDisabled("- %s", m_filtered_commands[i].description.c_str());
            }

            ImGui::EndGroup();
            ImGui::PopID();

            if (clicked) {
                execute_selected_command();
                break;
            }
        }
        ImGui::EndListBox();
    }
}

bool CommandPaletteView::render_command_item(const ContentRegistry::Commands::CommandItem& command,
                                              bool is_selected) {
    // 这个方法现在在 render_command_list 中内联实现
    (void)command;
    (void)is_selected;
    return false;
}

void CommandPaletteView::render_highlighted_text(const std::string& text, const std::string& query) {
    if (query[0] == '\0') {
        ImGui::Text("%s", text.c_str());
        return;
    }

    // 简单实现：直接显示文本（TODO: 实现搜索高亮）
    ImGui::Text("%s", text.c_str());
}

void CommandPaletteView::execute_selected_command() {
    if (m_selected_index >= 0 &&
        m_selected_index < static_cast<int>(m_filtered_commands.size())) {

        auto& command = m_filtered_commands[m_selected_index];

        // 检查命令是否启用
        if (!command.enabled_callback || command.enabled_callback()) {
            if (command.callback) {
                command.callback();
                LOG_INFO("从命令面板执行命令: {}", command.name.get());
            }
            m_window_open = false;  // 执行后关闭
        } else {
            LOG_WARN("命令 '{}' 当前已禁用", command.name.get());
        }
    }
}

void CommandPaletteView::move_selection(int delta) {
    if (m_filtered_commands.empty()) {
        return;
    }

    m_selected_index += delta;
    if (m_selected_index < 0) {
        m_selected_index = static_cast<int>(m_filtered_commands.size()) - 1;
    } else if (m_selected_index >= static_cast<int>(m_filtered_commands.size())) {
        m_selected_index = 0;
    }
}

void CommandPaletteView::handle_keyboard_navigation() {
    const ImGuiIO& io = ImGui::GetIO();

    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
        move_selection(-1);
    } else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
        move_selection(1);
    } else if (ImGui::IsKeyPressed(ImGuiKey_Tab)) {
        // Tab: 向下移动，Shift+Tab: 向上移动
        if (io.KeyShift) {
            move_selection(-1);
        } else {
            move_selection(1);
        }
    } else if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) {
        execute_selected_command();
    } else if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        m_window_open = false;
    }
}

void CommandPaletteView::filter_commands() {
    using namespace ContentRegistry::Commands;
    m_filtered_commands = Registry::instance().search(m_search_buffer);
}

} // namespace DearTs::Plugins::CommandPalette
