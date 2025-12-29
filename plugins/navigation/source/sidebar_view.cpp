/**
 * @file sidebar_view.cpp
 * @brief 侧边栏视图实现
 */

#include "views/sidebar_view.hpp"
#include "core/ui/view.h"
#include "core/ui/icon_font.hpp"
#include "core/content/registry_base.h"
#include "liblogger/logger.h"
#include <algorithm>
#include <imgui.h>

using namespace DearTs::Core;

namespace DearTs::Plugins::Navigation {

SidebarView::SidebarView()
    : ViewWindow("侧边栏") {
    LOG_INFO("SidebarView initialized");

    // 设置默认显示
    m_window_open = true;
}

void SidebarView::draw_content() {
    // 绘制工具栏
    draw_toolbar();

    ImGui::Separator();

    // 绘制搜索框
    draw_search_box();

    // 绘制筛选器
    draw_filter();

    ImGui::Separator();

    // 绘制视图列表
    if (m_current_category == ViewCategory::All) {
        // 显示所有视图
        draw_view_list("所有视图");
    } else {
        // 按分类显示
        const char* category_name = get_category_name(m_current_category);
        draw_view_list(category_name);
    }

    // 统计信息
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
        "显示: %d / 总计: %d", m_visible_count, m_total_count);
}

void SidebarView::draw_toolbar() {
    // 获取所有视图
    const auto& all_views = ContentRegistry::Views::get_all();

    // 工具栏样式
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.1f, 0.1f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.2f, 0.2f, 0.4f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0.3f, 0.6f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 6));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));

    // 全部显示按钮
    if (ImGui::Button("全部显示")) {
        for (const auto& [name, view] : all_views) {
            view->get_window_open_state() = true;
        }
    }

    ImGui::SameLine();

    // 全部隐藏按钮
    if (ImGui::Button("全部隐藏")) {
        for (const auto& [name, view] : all_views) {
            view->get_window_open_state() = false;
        }
    }

    ImGui::SameLine();

    // 重置为默认按钮
    if (ImGui::Button("重置默认")) {
        // 这里可以添加重置逻辑
        // 由于没有存储默认状态，暂时不实现
        LOG_INFO("Reset to defaults (not implemented)");
    }

    // 恢复样式
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);
}

void SidebarView::draw_search_box() {
    // 搜索框样式
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.15f, 0.15f, 0.15f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.2f, 0.2f, 0.2f, 0.9f));

    // 使用图标字体显示搜索图标
    if (UI::IconFont::isLoaded()) {
        ImGui::PushFont(UI::IconFont::getFont());
    }

    if (ImGui::InputText(ICON_SEARCH " 搜索", m_search_buffer, sizeof(m_search_buffer),
        ImGuiInputTextFlags_EnterReturnsTrue)) {
        apply_filter();
    }

    // 恢复原来的字体
    if (UI::IconFont::isLoaded()) {
        ImGui::PopFont();
    }

    // 实时搜索
    if (ImGui::IsItemEdited()) {
        apply_filter();
    }

    ImGui::PopStyleColor(3);
}

void SidebarView::draw_filter() {
    // 分类筛选 - 使用下拉框
    ImGui::AlignTextToFramePadding();
    ImGui::Text("分类:");
    ImGui::SameLine();

    // 分类选项数组
    static const char* category_items[] = { "全部", "核心", "工具", "插件" };
    static int current_category_idx = 0;

    // 根据当前分类设置索引
    if (m_current_category == ViewCategory::All) current_category_idx = 0;
    else if (m_current_category == ViewCategory::Core) current_category_idx = 1;
    else if (m_current_category == ViewCategory::Tools) current_category_idx = 2;
    else if (m_current_category == ViewCategory::Plugins) current_category_idx = 3;

    // 绘制下拉框
    if (ImGui::Combo("##category", &current_category_idx, category_items, IM_ARRAYSIZE(category_items))) {
        // 更新当前分类
        switch (current_category_idx) {
            case 0: m_current_category = ViewCategory::All; break;
            case 1: m_current_category = ViewCategory::Core; break;
            case 2: m_current_category = ViewCategory::Tools; break;
            case 3: m_current_category = ViewCategory::Plugins; break;
        }
        apply_filter();
    }
}

void SidebarView::draw_view_list(const std::string& category) {
    // 获取所有已注册的视图
    const auto& all_views = ContentRegistry::Views::get_all();

    // 更新统计
    m_total_count = static_cast<int>(all_views.size());
    m_visible_count = 0;

    // 如果列表为空
    if (all_views.empty()) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "暂无视图");
        return;
    }

    // 遍历所有视图
    for (const auto& [name, view] : all_views) {
        // 过滤掉侧边栏自己
        if (view->get_name() == this->get_name()) {
            continue;
        }

        // 应用搜索筛选
        if (m_search_buffer[0] != '\0') {
            std::string search_term = m_search_buffer;
            std::string view_name = view->get_name();

            // 转换为小写进行不区分大小写搜索
            std::transform(search_term.begin(), search_term.end(),
                          search_term.begin(), ::tolower);
            std::string lower_view_name = view_name;
            std::transform(lower_view_name.begin(), lower_view_name.end(),
                          lower_view_name.begin(), ::tolower);

            if (lower_view_name.find(search_term) == std::string::npos) {
                continue;
            }
        }

        // 统计可见视图
        if (view->get_window_open_state()) {
            m_visible_count++;
        }

        // 绘制视图项
        draw_view_item(view.get());
    }
}

void SidebarView::draw_view_item(UI::View* view) {
    if (!view) return;

    // 获取视图信息
    std::string name = view->get_name();
    const char* icon = view->get_icon();
    bool is_visible = view->get_window_open_state();

    // 可用性检查
    bool should_process = view->should_process();

    // 视图项样式
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.1f, 0.1f, 0.3f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.15f, 0.15f, 0.15f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.2f, 0.2f, 0.2f, 0.7f));

    // 调整 FramePadding 实现垂直居中
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 8));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));

    // 使用固定高度
    float item_height = 35.0f;

    // 使用 Selectable 创建可点击项
    ImGuiSelectableFlags flags = ImGuiSelectableFlags_AllowDoubleClick;

    // 为每个视图创建唯一的 ID（使用视图名称）
    std::string item_id = "##view_" + name;

    // 绘制选择项（只显示名称）
    if (ImGui::Selectable(item_id.c_str(), is_visible, flags, ImVec2(0, item_height))) {
        // 切换视图可见性
        view->get_window_open_state() = !view->get_window_open_state();
    }

    // 在 Selectable 上面绘制内容
    ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
    ImVec2 item_min = ImGui::GetItemRectMin();
    ImVec2 item_max = ImGui::GetItemRectMax();
    float item_height_actual = item_max.y - item_min.y;

    // 计算垂直居中位置
    float text_y = item_min.y + (item_height_actual - ImGui::GetFontSize()) * 0.5f;

    // 使用 DrawList 绘制图标和文本
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // 绘制图标（如果有）
    float x_offset = item_min.x + 8.0f;
    if (icon && UI::IconFont::isLoaded()) {
        ImGui::PushFont(UI::IconFont::getFont());
        draw_list->AddText(
            ImVec2(x_offset, text_y),
            IM_COL32(255, 255, 255, 255),
            icon
        );
        ImGui::PopFont();
        x_offset += ImGui::GetFontSize() + 4.0f; // 图标宽度 + 间距
    }

    // 绘制名称
    draw_list->AddText(
        ImVec2(x_offset, text_y),
        IM_COL32(255, 255, 255, 255),
        name.c_str()
    );

    // 右侧状态指示器
    float status_x = item_max.x - 60.0f;
    float status_y = item_min.y + item_height_actual * 0.5f;

    if (is_visible) {
        // 绿色点（可见）
        draw_list->AddCircleFilled(
            ImVec2(status_x, status_y),
            6.0f,
            IM_COL32(100, 200, 100, 255)
        );
    } else {
        // 灰色点（隐藏）
        draw_list->AddCircleFilled(
            ImVec2(status_x, status_y),
            6.0f,
            IM_COL32(100, 100, 100, 255)
        );
    }

    // 不可用状态
    if (!should_process) {
        draw_list->AddText(
            ImVec2(status_x + 15.0f, text_y),
            IM_COL32(128, 128, 128, 255),
            "(锁定)"
        );
    }

    // 描述文本
    draw_list->AddText(
        ImVec2(status_x + 60.0f, text_y),
        IM_COL32(153, 153, 153, 255),
        "点击切换"
    );

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);
}

void SidebarView::apply_filter() {
    // 筛选逻辑在 draw_view_list 中实时处理
    // 这里可以添加额外的筛选逻辑
}

const char* SidebarView::get_category_name(ViewCategory category) const {
    switch (category) {
        case ViewCategory::All:     return "所有视图";
        case ViewCategory::Core:    return "核心视图";
        case ViewCategory::Tools:   return "工具视图";
        case ViewCategory::Plugins: return "插件视图";
        default:                    return "未知分类";
    }
}

void SidebarView::draw_settings_button() {
    // 设置按钮（可以展开高级选项）
    if (ImGui::Button("⚙ 设置")) {
        // TODO: 打开设置对话框
    }
}

} // namespace DearTs::Plugins::Navigation
