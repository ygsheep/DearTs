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
    LOG_INFO("ThemeManager initialized");
}

void ThemeManager::setTheme(Theme theme) {
    if (m_current_theme != theme) {
        m_current_theme = theme;
        applyImGuiStyle();
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

    // 基于 ImGui 默认暗色主题进行优化
    colors[ImGuiCol_Text]                   = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_Border]                 = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]    = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]     = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]        = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_Button]                 = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);
    colors[ImGuiCol_Separator]              = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);
    colors[ImGuiCol_Tab]                    = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_TabHovered]             = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    colors[ImGuiCol_TabActive]              = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_TabUnfocused]           = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_PlotLines]              = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.80f, 0.50f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.80f, 0.50f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_TableBorderLight]       = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);

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

    // 玻璃态颜色
    ImVec4 glass_bg = ImVec4(0.08f, 0.08f, 0.10f, m_glass_alpha);
    ImVec4 accent = m_accent_color;

    // 设置窗口背景
    style.Colors[ImGuiCol_WindowBg] = glass_bg;
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.1f, 0.1f, 0.12f, 0.5f);

    // 按钮样式
    style.Colors[ImGuiCol_Button] = accent;
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(accent.x + 0.1f, accent.y + 0.1f, accent.z + 0.1f, 0.9f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(accent.x + 0.15f, accent.y + 0.15f, accent.z + 0.15f, 1.0f);

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

} // namespace DearTs::Core::UI
