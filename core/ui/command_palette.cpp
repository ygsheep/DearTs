/**
 * @file command_palette.cpp
 * @brief 命令面板 UI 组件实现
 */

#include "core/ui/command_palette.h"
#include "liblogger/logger.h"
#include <algorithm>

namespace DearTs::Core::UI {

CommandPalette::CommandPalette() {
    m_search_buffer[0] = '\0';
}

void CommandPalette::open() {
    if (!m_is_open) {
        m_is_open = true;
        m_search_buffer[0] = '\0';
        m_selected_index = 0;
        filter_commands();
        ImGui::SetKeyboardFocusHere(0);
        LOG_INFO("Command palette opened");
    }
}

void CommandPalette::close() {
    if (m_is_open) {
        m_is_open = false;
        LOG_INFO("Command palette closed");
    }
}

void CommandPalette::toggle() {
    if (m_is_open) {
        close();
    } else {
        open();
    }
}

void CommandPalette::render() {
    if (!m_is_open) {
        return;
    }

    // 计算窗口大小
    ImGuiIO& io = ImGui::GetIO();
    float width = 600.0f;
    float height = 400.0f;
    if (width > io.DisplaySize.x * 0.8f) width = io.DisplaySize.x * 0.8f;
    if (height > io.DisplaySize.y * 0.6f) height = io.DisplaySize.y * 0.6f;

    ImVec2 window_size(width, height);
    ImVec2 window_pos((io.DisplaySize.x - window_size.x) * 0.5f,
                      (io.DisplaySize.y - window_size.y) * 0.3f);

    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(window_size);
    ImGui::SetNextWindowFocus();

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
                            ImGuiWindowFlags_NoMove |
                            ImGuiWindowFlags_NoSavedSettings;

    if (ImGui::Begin(m_title.c_str(), &m_is_open, flags)) {
        // 搜索框
        ImGui::PushItemWidth(-1);
        bool input_active = ImGui::InputText("##search", m_search_buffer,
                                            sizeof(m_search_buffer),
                                            ImGuiInputTextFlags_AutoSelectAll);
        ImGui::PopItemWidth();

        // 过滤命令
        if (input_active) {
            filter_commands();
            m_selected_index = 0;
        }

        // 命令列表
        render_command_list();
    }

    ImGui::End();

    // ESC 关闭
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        close();
    }
}

void CommandPalette::set_shortcut(int key, bool ctrl, bool shift, bool alt) {
    m_shortcut_key = key;
    m_shortcut_ctrl = ctrl;
    m_shortcut_shift = shift;
    m_shortcut_alt = alt;
}

bool CommandPalette::check_shortcut() {
    ImGuiIO& io = ImGui::GetIO();

    if (ImGui::IsKeyPressed(static_cast<ImGuiKey>(m_shortcut_key))) {
        if ((!m_shortcut_ctrl || io.KeyCtrl) &&
            (!m_shortcut_shift || io.KeyShift) &&
            (!m_shortcut_alt || io.KeyAlt)) {
            return true;
        }
    }

    return false;
}

void CommandPalette::render_command_list() {
    using namespace ContentRegistry::Commands;

    if (m_filtered_commands.empty()) {
        ImGui::TextDisabled("No commands found");
        return;
    }

    // 列表
    ImVec2 list_size = ImGui::GetContentRegionAvail();
    list_size.y -= m_style.item_height;

    if (ImGui::BeginListBox("##commands", list_size)) {
        for (size_t i = 0; i < m_filtered_commands.size(); i++) {
            bool is_selected = (i == m_selected_index);

            if (render_command_item(m_filtered_commands[i], is_selected, m_search_buffer)) {
                execute_selected_command();
                close();
                break;
            }
        }
        ImGui::EndListBox();
    }

    // 键盘导航
    handle_keyboard_navigation();
}

bool CommandPalette::render_command_item(const ContentRegistry::Commands::CommandItem& command,
                                        bool is_selected,
                                        const std::string& search_query) {
    ImGui::PushID(&command);

    if (is_selected) {
        ImGui::PushStyleColor(ImGuiCol_Header, m_style.item_active);
    }

    bool clicked = ImGui::Selectable("", is_selected, ImGuiSelectableFlags_AllowDoubleClick);

    if (is_selected) {
        ImGui::PopStyleColor();
    }

    ImGui::SameLine();

    // 渲染命令名称和描述
    ImGui::BeginGroup();
    render_highlighted_text(command.name.get(), search_query);
    ImGui::SameLine();
    ImGui::TextDisabled("- %s", command.description.c_str());
    ImGui::EndGroup();

    ImGui::PopID();

    return clicked;
}

void CommandPalette::render_highlighted_text(const std::string& text, const std::string& query) {
    if (query.empty()) {
        ImGui::Text("%s", text.c_str());
        return;
    }

    // TODO: 实现搜索高亮
    ImGui::Text("%s", text.c_str());
}

void CommandPalette::execute_selected_command() {
    if (m_selected_index < m_filtered_commands.size()) {
        auto& command = m_filtered_commands[m_selected_index];
        if (command.callback) {
            command.callback();
            LOG_INFO("Executed command from palette: {}", command.name.get());
        }
    }
}

void CommandPalette::move_selection(int delta) {
    m_selected_index += delta;
    if (m_selected_index < 0) {
        m_selected_index = static_cast<int>(m_filtered_commands.size()) - 1;
    } else if (m_selected_index >= static_cast<int>(m_filtered_commands.size())) {
        m_selected_index = 0;
    }
}

void CommandPalette::handle_keyboard_navigation() {
    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
        move_selection(-1);
    } else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
        move_selection(1);
    } else if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) {
        execute_selected_command();
        close();
    }
}

void CommandPalette::filter_commands() {
    using namespace ContentRegistry::Commands;
    m_filtered_commands = Registry::instance().search(m_search_buffer);
}

int CommandPalette::calculate_match_score(const std::string& text, const std::string& query) {
    // TODO: 实现模糊匹配评分
    return 0;
}

} // namespace DearTs::Core::UI
