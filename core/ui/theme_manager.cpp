/**
 * @file theme_manager.cpp
 * @brief 主题管理器实现
 */

#include "theme_manager.h"
#include "logger.h"
#include <fstream>
#include <sstream>

namespace DearTs::Core::UI {

ThemeManager::ThemeManager() {
    initializeThemes();
    applyChatManagerTheme();  // 初始化 ChatManager 颜色
    LOG_INFO("ThemeManager initialized");
}

void ThemeManager::setTheme(Theme theme) {
    if (m_current_theme != theme) {
        m_current_theme = theme;
        applyImGuiStyle();
        applyChatManagerTheme();  // 自动应用 ChatManager 主题颜色
        notifyThemeChanged();
        LOG_INFO("Theme changed to: {}", getThemeName(theme));
    }
}

void ThemeManager::applyImGuiStyle() {
    ImGuiStyle& style = ImGui::GetStyle();

    switch (m_current_theme) {
        case Theme::Dark:
            applyDarkTheme();
            break;
        case Theme::Light:
            applyLightTheme();
            break;
        case Theme::Classic:
            applyClassicTheme();
            break;
        case Theme::Custom:
            // 自定义主题使用已设置的颜色
            break;
    }

    // 应用自定义颜色覆盖
    for (const auto& [key, color] : m_colors) {
        if (key == "Background") {
            style.Colors[ImGuiCol_WindowBg] = color;
        } else if (key == "Text") {
            style.Colors[ImGuiCol_Text] = color;
        } else if (key == "Button") {
            style.Colors[ImGuiCol_Button] = color;
        } else if (key == "ButtonHovered") {
            style.Colors[ImGuiCol_ButtonHovered] = color;
        } else if (key == "ButtonActive") {
            style.Colors[ImGuiCol_ButtonActive] = color;
        } else if (key == "Header") {
            style.Colors[ImGuiCol_Header] = color;
        } else if (key == "HeaderHovered") {
            style.Colors[ImGuiCol_HeaderHovered] = color;
        } else if (key == "HeaderActive") {
            style.Colors[ImGuiCol_HeaderActive] = color;
        } else if (key == "TitleBg") {
            style.Colors[ImGuiCol_TitleBg] = color;
        } else if (key == "TitleBgActive") {
            style.Colors[ImGuiCol_TitleBgActive] = color;
        }
    }
}

bool ThemeManager::loadTheme(const std::string& json_path) {
    // TODO: 实现简单的主题文件格式
    // 当前简化版本暂不支持从文件加载主题
    LOG_WARN("loadTheme not implemented yet: {}", json_path);
    return false;
}

bool ThemeManager::saveTheme(const std::string& json_path) const {
    // TODO: 实现简单的主题文件格式
    // 当前简化版本暂不支持保存主题到文件
    LOG_WARN("saveTheme not implemented yet: {}", json_path);
    return false;
}

ImVec4 ThemeManager::getColor(const std::string& key) const {
    auto it = m_colors.find(key);
    if (it != m_colors.end()) {
        return it->second;
    }
    return ImVec4(1, 1, 1, 1); // 默认白色
}

void ThemeManager::setColor(const std::string& key, const ImVec4& color) {
    m_colors[key] = color;
    // 如果是自定义主题，立即应用
    if (m_current_theme == Theme::Custom) {
        applyImGuiStyle();
    }
}

const char* ThemeManager::getThemeName(Theme theme) {
    switch (theme) {
        case Theme::Dark:    return "暗色";
        case Theme::Light:   return "亮色";
        case Theme::Classic: return "经典";
        case Theme::Custom:  return "自定义";
        default:             return "未知";
    }
}

void ThemeManager::initializeThemes() {
    // 初始化默认颜色映射
    m_colors["Background"] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    m_colors["Text"] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
}

void ThemeManager::applyDarkTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // ===== Claude Dark Theme =====
    // 基于 Claude Desktop 暗色主题配色
    // 参考: --claude-primary: rgb(218, 119, 86), --claude-bg-dark: #262624

    // 文本颜色
    colors[ImGuiCol_Text]                   = ImVec4(0.941f, 0.941f, 0.941f, 1.00f);  // #f0f0f0
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.600f, 0.600f, 0.600f, 1.00f);  // #999999

    // 背景颜色
    colors[ImGuiCol_WindowBg]               = ImVec4(0.149f, 0.149f, 0.141f, 1.00f);  // #262624
    colors[ImGuiCol_ChildBg]                = ImVec4(0.149f, 0.149f, 0.141f, 1.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.188f, 0.188f, 0.180f, 0.98f);  // #30302E

    // 边框
    colors[ImGuiCol_Border]                 = ImVec4(0.239f, 0.239f, 0.239f, 1.00f);  // #3d3d3d
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    // 输入框/框架
    colors[ImGuiCol_FrameBg]                = ImVec4(0.200f, 0.200f, 0.200f, 1.00f);  // #333333
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.239f, 0.239f, 0.239f, 1.00f);  // #3d3d3d
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.855f, 0.467f, 0.337f, 0.20f);  // Claude primary alpha

    // 标题栏
    colors[ImGuiCol_TitleBg]                = ImVec4(0.188f, 0.188f, 0.180f, 1.00f);  // #30302E
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.200f, 0.200f, 0.200f, 1.00f);  // #333333
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.149f, 0.149f, 0.141f, 1.00f);

    // 菜单栏
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.188f, 0.188f, 0.180f, 1.00f);

    // 滚动条
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.149f, 0.149f, 0.141f, 1.00f);  // #262624
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.200f, 0.200f, 0.200f, 1.00f);  // #333333
    colors[ImGuiCol_ScrollbarGrabHovered]    = ImVec4(0.239f, 0.239f, 0.239f, 1.00f);  // #3d3d3d
    colors[ImGuiCol_ScrollbarGrabActive]     = ImVec4(0.855f, 0.467f, 0.337f, 1.00f);  // Claude primary

    // 复选框
    colors[ImGuiCol_CheckMark]              = ImVec4(0.855f, 0.467f, 0.337f, 1.00f);  // Claude primary

    // 滑块
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.239f, 0.239f, 0.239f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]        = ImVec4(0.855f, 0.467f, 0.337f, 1.00f);

    // 按钮 - Claude 主色
    colors[ImGuiCol_Button]                 = ImVec4(0.855f, 0.467f, 0.337f, 1.00f);  // #DA7656 Claude primary
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.741f, 0.365f, 0.227f, 1.00f);  // #BD5D3A primary hover
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.650f, 0.300f, 0.180f, 1.00f);

    // 表头
    colors[ImGuiCol_Header]                 = ImVec4(0.200f, 0.200f, 0.200f, 1.00f);  // #333333
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.855f, 0.467f, 0.337f, 0.15f);  // Claude hover
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.855f, 0.467f, 0.337f, 0.25f);  // Claude selection

    // 分隔符
    colors[ImGuiCol_Separator]              = ImVec4(0.239f, 0.239f, 0.239f, 1.00f);  // #3d3d3d
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.855f, 0.467f, 0.337f, 0.50f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.855f, 0.467f, 0.337f, 0.70f);

    // 调整大小
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.239f, 0.239f, 0.239f, 0.50f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.855f, 0.467f, 0.337f, 0.70f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.855f, 0.467f, 0.337f, 1.00f);

    // 标签页
    colors[ImGuiCol_Tab]                    = ImVec4(0.149f, 0.149f, 0.141f, 1.00f);  // #262624
    colors[ImGuiCol_TabHovered]             = ImVec4(0.855f, 0.467f, 0.337f, 0.15f);
    colors[ImGuiCol_TabActive]              = ImVec4(0.188f, 0.188f, 0.180f, 1.00f);  // #30302E
    colors[ImGuiCol_TabUnfocused]           = ImVec4(0.149f, 0.149f, 0.141f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.188f, 0.188f, 0.180f, 1.00f);

    // 图表
    colors[ImGuiCol_PlotLines]              = ImVec4(0.855f, 0.467f, 0.337f, 1.00f);  // Claude primary
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.000f, 0.600f, 0.400f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.855f, 0.467f, 0.337f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.000f, 0.600f, 0.400f, 1.00f);

    // 表格
    colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.188f, 0.188f, 0.180f, 1.00f);  // #30302E
    colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.239f, 0.239f, 0.239f, 1.00f);  // #3d3d3d
    colors[ImGuiCol_TableBorderLight]       = ImVec4(0.200f, 0.200f, 0.200f, 1.00f);
    colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]          = ImVec4(0.855f, 0.467f, 0.337f, 0.05f);

    // 设置样式
    style.WindowPadding         = ImVec2(8, 8);
    style.FramePadding          = ImVec2(4, 4);
    style.CellPadding           = ImVec2(4, 4);
    style.ItemSpacing           = ImVec2(8, 4);
    style.ItemInnerSpacing      = ImVec2(4, 4);
    style.TouchExtraPadding     = ImVec2(0, 0);
    style.IndentSpacing         = 21;
    style.ColumnsMinSpacing     = 6;
    style.ScrollbarSize         = 14;
    style.GrabMinSize           = 10;
    style.WindowBorderSize      = 1;
    style.ChildBorderSize       = 1;
    style.PopupBorderSize        = 1;
    style.FrameBorderSize        = 1;
    style.TabBorderSize          = 1;
    style.WindowRounding        = 4;
    style.ChildRounding         = 4;
    style.FrameRounding         = 4;
    style.PopupRounding          = 4;
    style.ScrollbarRounding     = 9;
    style.GrabRounding          = 4;
    style.LogSliderDeadzone     = 4;
    style.TabRounding           = 4;
    style.DisplaySafeAreaPadding = ImVec2(4, 4);
    style.DisplayWindowPadding  = ImVec2(8, 8);

    // 更新颜色映射
    m_colors["Background"] = colors[ImGuiCol_WindowBg];
    m_colors["Text"] = colors[ImGuiCol_Text];
    m_colors["Button"] = colors[ImGuiCol_Button];
    m_colors["ButtonHovered"] = colors[ImGuiCol_ButtonHovered];
    m_colors["ButtonActive"] = colors[ImGuiCol_ButtonActive];
    m_colors["Header"] = colors[ImGuiCol_Header];
    m_colors["HeaderHovered"] = colors[ImGuiCol_HeaderHovered];
    m_colors["HeaderActive"] = colors[ImGuiCol_HeaderActive];
}

void ThemeManager::applyLightTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // 亮色主题
    colors[ImGuiCol_Text]                   = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_Border]                 = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]    = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]     = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]        = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_Button]                 = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);
    colors[ImGuiCol_Separator]              = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_Tab]                    = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);
    colors[ImGuiCol_TabHovered]             = ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
    colors[ImGuiCol_TabActive]              = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_TabUnfocused]           = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
    colors[ImGuiCol_PlotLines]              = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(0.50f, 0.50f, 0.80f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(0.50f, 0.50f, 0.80f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_TableBorderLight]       = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
    colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]          = ImVec4(0.00f, 0.00f, 0.00f, 0.06f);

    // 样式设置
    style.WindowPadding         = ImVec2(8, 8);
    style.FramePadding          = ImVec2(4, 4);
    style.CellPadding           = ImVec2(4, 4);
    style.ItemSpacing           = ImVec2(8, 4);
    style.ItemInnerSpacing      = ImVec2(4, 4);
    style.IndentSpacing         = 21;
    style.ColumnsMinSpacing     = 6;
    style.ScrollbarSize         = 14;
    style.WindowRounding        = 4;
    style.FrameRounding         = 4;
    style.PopupRounding         = 4;
    style.ScrollbarRounding     = 9;
    style.GrabRounding          = 4;
    style.TabRounding           = 4;
    style.DisplayWindowPadding  = ImVec2(8, 8);
    style.DisplaySafeAreaPadding = ImVec2(4, 4);

    // 更新颜色映射
    m_colors["Background"] = colors[ImGuiCol_WindowBg];
    m_colors["Text"] = colors[ImGuiCol_Text];
    m_colors["Button"] = colors[ImGuiCol_Button];
    m_colors["ButtonHovered"] = colors[ImGuiCol_ButtonHovered];
    m_colors["ButtonActive"] = colors[ImGuiCol_ButtonActive];
    m_colors["Header"] = colors[ImGuiCol_Header];
    m_colors["HeaderHovered"] = colors[ImGuiCol_HeaderHovered];
    m_colors["HeaderActive"] = colors[ImGuiCol_HeaderActive];
}

void ThemeManager::applyClassicTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // 经典主题（接近 ImGui 默认）
    colors[ImGuiCol_Text]                   = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    colors[ImGuiCol_Border]                 = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]    = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]     = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]        = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
    colors[ImGuiCol_Button]                 = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
    colors[ImGuiCol_Separator]              = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);
    colors[ImGuiCol_Tab]                    = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_TabHovered]             = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
    colors[ImGuiCol_TabActive]              = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_TabUnfocused]           = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotLines]              = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.85f, 0.60f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.85f, 0.60f, 1.00f);

    // 样式设置
    style.WindowPadding         = ImVec2(8, 8);
    style.FramePadding          = ImVec2(4, 4);
    style.ItemSpacing           = ImVec2(8, 4);
    style.ItemInnerSpacing      = ImVec2(4, 4);
    style.IndentSpacing         = 21;
    style.ColumnsMinSpacing     = 6;
    style.ScrollbarSize         = 16;
    style.WindowRounding        = 5;
    style.FrameRounding         = 4;
    style.PopupRounding         = 4;
    style.ScrollbarRounding     = 9;
    style.GrabRounding          = 4;
    style.TabRounding           = 4;
    style.DisplayWindowPadding  = ImVec2(8, 8);
    style.DisplaySafeAreaPadding = ImVec2(4, 4);

    // 更新颜色映射
    m_colors["Background"] = colors[ImGuiCol_WindowBg];
    m_colors["Text"] = colors[ImGuiCol_Text];
    m_colors["Button"] = colors[ImGuiCol_Button];
    m_colors["ButtonHovered"] = colors[ImGuiCol_ButtonHovered];
    m_colors["ButtonActive"] = colors[ImGuiCol_ButtonActive];
    m_colors["Header"] = colors[ImGuiCol_Header];
    m_colors["HeaderHovered"] = colors[ImGuiCol_HeaderHovered];
    m_colors["HeaderActive"] = colors[ImGuiCol_HeaderActive];
}

void ThemeManager::notifyThemeChanged() {
    for (const auto& callback : m_theme_changed_callbacks) {
        callback(m_current_theme);
    }
}

void ThemeManager::applyGlassmorphismStyle() {
    ImGuiStyle& style = ImGui::GetStyle();

    // Claude 玻璃态颜色
    // 使用 Claude 的背景色 (#262624) 和主色 (#DA7656)
    ImVec4 glass_bg = ImVec4(0.149f, 0.149f, 0.141f, m_glass_alpha);  // #262624
    ImVec4 accent = m_accent_color;  // Claude Amber (#f59e0b or #DA7656)

    // 设置窗口背景
    style.Colors[ImGuiCol_WindowBg] = glass_bg;
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.149f, 0.149f, 0.141f, 0.5f);

    // 按钮样式 - Claude 主色
    style.Colors[ImGuiCol_Button] = accent;
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.741f, 0.365f, 0.227f, 0.9f);  // #BD5D3A
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.650f, 0.300f, 0.180f, 1.0f);

    // 文本样式
    style.Colors[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.98f, 1.0f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.6f, 0.6f, 0.65f, 1.0f);

    // 边框
    style.Colors[ImGuiCol_Border] = ImVec4(0.3f, 0.3f, 0.35f, 0.3f);
    style.Colors[ImGuiCol_Separator] = ImVec4(0.3f, 0.3f, 0.35f, 0.3f);

    // 滚动条
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.1f, 0.1f, 0.12f, 0.5f);
    style.Colors[ImGuiCol_ScrollbarGrab] = accent;
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(accent.x + 0.1f, accent.y + 0.1f, accent.z + 0.1f, 0.8f);

    // 圆角
    style.WindowRounding = m_border_radius;
    style.ChildRounding = m_border_radius;
    style.FrameRounding = m_border_radius - 2.0f;
    style.PopupRounding = m_border_radius;
    style.ScrollbarRounding = m_border_radius - 1.0f;
    style.GrabRounding = m_border_radius - 2.0f;
    style.TabRounding = m_border_radius - 2.0f;

    LOG_INFO("ThemeManager: Applied glassmorphism style (alpha={:.2f}, radius={:.1f})",
             m_glass_alpha, m_border_radius);
}

// ==================== ChatManager 主题支持 ====================

void ThemeManager::applyChatManagerTheme() {
    // ChatManager 颜色键名定义
    static const char* chat_color_keys[] = {
        "chat.background",           // 聊天区域背景
        "chat.surface",              // 表面颜色
        "chat.surface_highlight",    // 高亮表面颜色
        "chat.user_message_bg",      // 用户消息气泡背景
        "chat.user_message_text",    // 用户消息文本颜色
        "chat.user_message_border",  // 用户消息边框
        "chat.user_border_hover",    // 用户消息悬停边框
        "chat.ai_message_bg",        // AI 消息气泡背景
        "chat.ai_message_text",      // AI 消息文本颜色
        "chat.ai_message_border",    // AI 消息边框
        "chat.ai_border_hover",      // AI 消息悬停边框
        "chat.input_bg",             // 输入框背景
        "chat.input_border",         // 输入框边框
        "chat.input_focus",          // 输入框聚焦颜色
        "chat.input_placeholder",    // 输入框占位符颜色
        "chat.suggestion_bg",        // 建议芯片背景
        "chat.suggestion_hover",     // 建议芯片悬停
        "chat.suggestion_text",      // 建议芯片文本
        "chat.code_bg",              // 代码块背景
        "chat.code_text",            // 代码块文本
        "chat.system_text",          // 系统消息文本
        "chat.timestamp",            // 时间戳颜色
        "chat.link_color",           // 链接颜色
        "chat.error_bg",             // 错误消息背景
        "chat.error_text",           // 错误消息文本
        "chat.button_bg",            // 按钮背景
        "chat.button_hover",         // 按钮悬停
        "chat.button_active",        // 按钮按下
        "chat.button_icon",          // 按钮图标
        "chat.button_icon_hover",    // 按钮图标悬停
    };

    // 根据当前主题设置 ChatManager 颜色
    if (m_current_theme == Theme::Dark) {
        // ===== Claude 暗色主题配色 =====
        // 基于 Claude CSS: --claude-bg-dark: #262624, --claude-primary: rgb(218, 119, 86)
        m_colors["chat.background"]         = ImVec4(0.149f, 0.149f, 0.141f, 1.00f);  // #262624
        m_colors["chat.surface"]            = ImVec4(0.188f, 0.188f, 0.180f, 1.00f);  // #30302E
        m_colors["chat.surface_highlight"]  = ImVec4(0.200f, 0.200f, 0.200f, 1.00f);  // #333333

        // 用户消息 - Claude 主色 (#DA7656)
        m_colors["chat.user_message_bg"]    = ImVec4(0.855f, 0.467f, 0.337f, 1.00f);  // #DA7656 Claude primary
        m_colors["chat.user_message_text"]  = ImVec4(0.941f, 0.941f, 0.941f, 1.00f);  // #f0f0f0
        m_colors["chat.user_message_border"]= ImVec4(0.741f, 0.365f, 0.227f, 1.00f);  // #BD5D3A primary hover
        m_colors["chat.user_border_hover"]  = ImVec4(0.920f, 0.550f, 0.420f, 1.00f);  // 更亮的主色

        // AI 消息 - 深色表面
        m_colors["chat.ai_message_bg"]      = ImVec4(0.188f, 0.188f, 0.180f, 1.00f);  // #30302E
        m_colors["chat.ai_message_text"]    = ImVec4(0.941f, 0.941f, 0.941f, 1.00f);  // #f0f0f0
        m_colors["chat.ai_message_border"]  = ImVec4(0.239f, 0.239f, 0.239f, 1.00f);  // #3d3d3d
        m_colors["chat.ai_border_hover"]    = ImVec4(0.290f, 0.290f, 0.290f, 1.00f);

        // 输入框
        m_colors["chat.input_bg"]           = ImVec4(0.149f, 0.149f, 0.141f, 1.00f);  // #262624
        m_colors["chat.input_border"]       = ImVec4(0.239f, 0.239f, 0.239f, 1.00f);  // #3d3d3d
        m_colors["chat.input_focus"]        = ImVec4(0.855f, 0.467f, 0.337f, 0.80f);  // Claude primary
        m_colors["chat.input_placeholder"]  = ImVec4(0.600f, 0.600f, 0.600f, 1.00f);  // #999999

        // 建议芯片
        m_colors["chat.suggestion_bg"]      = ImVec4(0.200f, 0.200f, 0.200f, 1.00f);  // #333333
        m_colors["chat.suggestion_hover"]   = ImVec4(0.855f, 0.467f, 0.337f, 0.10f);  // Claude hover
        m_colors["chat.suggestion_text"]    = ImVec4(0.800f, 0.800f, 0.800f, 1.00f);  // #cccccc

        // 代码块
        m_colors["chat.code_bg"]            = ImVec4(0.200f, 0.200f, 0.200f, 1.00f);  // #333333
        m_colors["chat.code_text"]          = ImVec4(0.941f, 0.941f, 0.941f, 1.00f);  // #f0f0f0

        // 其他
        m_colors["chat.system_text"]        = ImVec4(0.600f, 0.600f, 0.600f, 1.00f);  // #999999
        m_colors["chat.timestamp"]          = ImVec4(0.600f, 0.600f, 0.600f, 1.00f);
        m_colors["chat.link_color"]         = ImVec4(0.855f, 0.467f, 0.337f, 1.00f);  // Claude primary
        m_colors["chat.error_bg"]           = ImVec4(0.400f, 0.120f, 0.120f, 1.00f);
        m_colors["chat.error_text"]         = ImVec4(1.000f, 0.700f, 0.700f, 1.00f);

        // 按钮颜色（使用浅灰色）
        m_colors["chat.button_bg"]          = ImVec4(0.180f, 0.180f, 0.180f, 1.00f);  // 浅灰色背景
        m_colors["chat.button_hover"]       = ImVec4(0.250f, 0.250f, 0.250f, 1.00f);  // 悬停时更亮
        m_colors["chat.button_active"]      = ImVec4(0.300f, 0.300f, 0.300f, 1.00f);  // 按下时更亮
        m_colors["chat.button_icon"]        = ImVec4(0.650f, 0.650f, 0.650f, 1.00f);  // 图标颜色
        m_colors["chat.button_icon_hover"]  = ImVec4(0.850f, 0.850f, 0.850f, 1.00f);  // 图标悬停颜色

    } else if (m_current_theme == Theme::Light) {
        // ===== Claude 亮色主题配色 =====
        // 基于 Claude CSS 推导亮色版本
        m_colors["chat.background"]         = ImVec4(0.996f, 0.996f, 0.996f, 1.00f);  // #fefefe
        m_colors["chat.surface"]            = ImVec4(0.945f, 0.961f, 0.976f, 1.00f);  // #f1f5f9
        m_colors["chat.surface_highlight"]  = ImVec4(0.880f, 0.880f, 0.880f, 1.00f);

        // 用户消息 - Claude 主色亮色版
        m_colors["chat.user_message_bg"]    = ImVec4(0.855f, 0.467f, 0.337f, 1.00f);  // #DA7656
        m_colors["chat.user_message_text"]  = ImVec4(1.000f, 1.000f, 1.000f, 1.00f);  // 白色文本
        m_colors["chat.user_message_border"]= ImVec4(0.741f, 0.365f, 0.227f, 1.00f);  // #BD5D3A
        m_colors["chat.user_border_hover"]  = ImVec4(0.920f, 0.550f, 0.420f, 1.00f);

        // AI 消息 - 浅色表面
        m_colors["chat.ai_message_bg"]      = ImVec4(0.945f, 0.961f, 0.976f, 1.00f);  // #f1f5f9
        m_colors["chat.ai_message_text"]    = ImVec4(0.235f, 0.235f, 0.263f, 1.00f);  // #3c3c43
        m_colors["chat.ai_message_border"]  = ImVec4(0.800f, 0.800f, 0.800f, 1.00f);
        m_colors["chat.ai_border_hover"]    = ImVec4(0.700f, 0.700f, 0.700f, 1.00f);

        // 输入框
        m_colors["chat.input_bg"]           = ImVec4(0.970f, 0.970f, 0.970f, 1.00f);
        m_colors["chat.input_border"]       = ImVec4(0.800f, 0.800f, 0.800f, 1.00f);
        m_colors["chat.input_focus"]        = ImVec4(0.855f, 0.467f, 0.337f, 0.60f);  // Claude primary
        m_colors["chat.input_placeholder"]  = ImVec4(0.550f, 0.550f, 0.600f, 1.00f);

        // 建议芯片
        m_colors["chat.suggestion_bg"]      = ImVec4(0.920f, 0.920f, 0.920f, 1.00f);
        m_colors["chat.suggestion_hover"]   = ImVec4(0.855f, 0.467f, 0.337f, 0.15f);  // 淡主色悬停
        m_colors["chat.suggestion_text"]    = ImVec4(0.300f, 0.300f, 0.350f, 1.00f);

        // 代码块
        m_colors["chat.code_bg"]            = ImVec4(0.920f, 0.920f, 0.920f, 1.00f);
        m_colors["chat.code_text"]          = ImVec4(0.235f, 0.235f, 0.263f, 1.00f);  // #3c3c43

        // 其他
        m_colors["chat.system_text"]        = ImVec4(0.550f, 0.550f, 0.600f, 1.00f);
        m_colors["chat.timestamp"]          = ImVec4(0.500f, 0.500f, 0.550f, 1.00f);
        m_colors["chat.link_color"]         = ImVec4(0.855f, 0.467f, 0.337f, 1.00f);  // Claude primary
        m_colors["chat.error_bg"]           = ImVec4(1.000f, 0.920f, 0.920f, 1.00f);
        m_colors["chat.error_text"]         = ImVec4(0.800f, 0.200f, 0.200f, 1.00f);

        // 按钮颜色（亮色主题）
        m_colors["chat.button_bg"]          = ImVec4(0.850f, 0.850f, 0.870f, 1.00f);  // 浅灰色背景
        m_colors["chat.button_hover"]       = ImVec4(0.750f, 0.750f, 0.770f, 1.00f);  // 悬停时更深
        m_colors["chat.button_active"]      = ImVec4(0.650f, 0.650f, 0.670f, 1.00f);  // 按下时更深
        m_colors["chat.button_icon"]        = ImVec4(0.350f, 0.350f, 0.380f, 1.00f);  // 图标颜色
        m_colors["chat.button_icon_hover"]  = ImVec4(0.200f, 0.200f, 0.230f, 1.00f);  // 图标悬停颜色
    }

    LOG_INFO("ThemeManager: Applied ChatManager theme colors");
}

const char* ThemeManager::getChatColorName(size_t index) {
    static const char* chat_color_keys[] = {
        "chat.background",
        "chat.surface",
        "chat.surface_highlight",
        "chat.user_message_bg",
        "chat.user_message_text",
        "chat.user_message_border",
        "chat.user_border_hover",
        "chat.ai_message_bg",
        "chat.ai_message_text",
        "chat.ai_message_border",
        "chat.ai_border_hover",
        "chat.input_bg",
        "chat.input_border",
        "chat.input_focus",
        "chat.input_placeholder",
        "chat.suggestion_bg",
        "chat.suggestion_hover",
        "chat.suggestion_text",
        "chat.code_bg",
        "chat.code_text",
        "chat.system_text",
        "chat.timestamp",
        "chat.link_color",
        "chat.error_bg",
        "chat.error_text",
    };

    if (index < sizeof(chat_color_keys) / sizeof(chat_color_keys[0])) {
        return chat_color_keys[index];
    }
    return "unknown";
}

} // namespace DearTs::Core::UI
