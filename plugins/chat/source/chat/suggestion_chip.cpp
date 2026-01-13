/**
 * @file suggestion_chip.cpp
 * @brief AI 建议芯片组件实现
 */

#include "chat/ui/suggestion_chip.hpp"
#include <fmt/format.h>

namespace DearTs::Plugins::Chat::UI {

bool SuggestionChip::draw(
    const AISuggestion& suggestion,
    int index,
    const SuggestionChipStyle* style_ptr
) {
    SuggestionChipStyle default_style;
    const SuggestionChipStyle& style = style_ptr ? *style_ptr : default_style;

    return draw_chip(suggestion, index, style);
}

void SuggestionChip::draw_list(
    const std::vector<AISuggestion>& suggestions,
    std::function<void(const AISuggestion&)> on_click,
    const SuggestionChipStyle* style_ptr
) {
    SuggestionChipStyle default_style;
    const SuggestionChipStyle& style = style_ptr ? *style_ptr : default_style;

    if (suggestions.empty()) {
        return;
    }

    // 水平滚动布局
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));

    for (size_t i = 0; i < suggestions.size(); i++) {
        if (i > 0) {
            ImGui::SameLine();
        }

        const bool clicked = draw_chip(suggestions[i], static_cast<int>(i), style);
        if (clicked && on_click) {
            on_click(suggestions[i]);
        }
    }

    ImGui::PopStyleVar();
}

void SuggestionChip::draw_list_vertical(
    const std::vector<AISuggestion>& suggestions,
    std::function<void(const AISuggestion&)> on_click,
    const SuggestionChipStyle* style_ptr
) {
    SuggestionChipStyle default_style;
    const SuggestionChipStyle& style = style_ptr ? *style_ptr : default_style;

    if (suggestions.empty()) {
        return;
    }

    for (size_t i = 0; i < suggestions.size(); i++) {
        const bool clicked = draw_chip(suggestions[i], static_cast<int>(i), style);
        if (clicked && on_click) {
            on_click(suggestions[i]);
        }
    }
}

bool SuggestionChip::draw_chip(
    const AISuggestion& suggestion,
    int index,
    const SuggestionChipStyle& style
) {
    // 构建标签
    const std::string label = fmt::format("##suggestion_{}", suggestion.id);

    // 计算大小
    const ImVec2 chip_size = calc_size(suggestion.content, style);

    // 设置样式
    ImGui::PushStyleColor(ImGuiCol_Button, style.bg_color);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, style.bg_hovered_color);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, style.bg_active_color);
    ImGui::PushStyleColor(ImGuiCol_Text, style.text_color);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, style.corner_radius);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(style.padding_x, style.padding_y));

    // 绘制按钮
    const bool clicked = ImGui::Button(label.c_str(), chip_size);

    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(2);

    // 绘制文本（居中）
    if (style.show_index && index >= 0) {
        // 显示索引 + 内容
        const std::string text = fmt::format("{}. {}", index + 1, suggestion.content);
        const ImVec2 text_pos = ImGui::GetItemRectMin() + ImVec2(style.padding_x, style.padding_y);
        ImGui::GetWindowDrawList()->AddText(
            ImGui::GetFont(),
            ImGui::GetFontSize(),
            text_pos,
            IM_COL32(
                static_cast<int>(style.text_color.x * 255),
                static_cast<int>(style.text_color.y * 255),
                static_cast<int>(style.text_color.z * 255),
                static_cast<int>(style.text_color.w * 255)
            ),
            text.c_str()
        );
    } else {
        // 只显示内容
        const ImVec2 text_pos = ImGui::GetItemRectMin() + ImVec2(style.padding_x, style.padding_y);
        ImGui::GetWindowDrawList()->AddText(
            ImGui::GetFont(),
            ImGui::GetFontSize(),
            text_pos,
            IM_COL32(
                static_cast<int>(style.text_color.x * 255),
                static_cast<int>(style.text_color.y * 255),
                static_cast<int>(style.text_color.z * 255),
                static_cast<int>(style.text_color.w * 255)
            ),
            suggestion.content.c_str()
        );
    }

    // 绘制置信度（如果启用）
    if (style.show_confidence && suggestion.confidence > 0.0f) {
        const std::string conf_str = fmt::format("{:.0f}%", suggestion.confidence * 100);
        const ImVec2 conf_pos = ImGui::GetItemRectMax() - ImVec2(
            ImGui::CalcTextSize(conf_str.c_str()).x + 5,
            ImGui::GetFontSize() + 2
        );

        ImGui::GetWindowDrawList()->AddText(
            ImGui::GetFont(),
            ImGui::GetFontSize() * 0.8f,
            conf_pos,
            IM_COL32(150, 150, 150, 255),
            conf_str.c_str()
        );
    }

    return clicked;
}

ImVec2 SuggestionChip::calc_size(const std::string& text, const SuggestionChipStyle& style) {
    const ImVec2 text_size = ImGui::CalcTextSize(text.c_str());
    return ImVec2(
        text_size.x + style.padding_x * 2,
        text_size.y + style.padding_y * 2
    );
}

} // namespace DearTs::Plugins::Chat::UI
