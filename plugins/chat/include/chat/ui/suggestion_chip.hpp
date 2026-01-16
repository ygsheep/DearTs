/**
 * @file suggestion_chip.hpp
 * @brief AI 建议芯片组件
 */

#pragma once

#include "chat/models/ai_suggestion.hpp"
#include <string>
#include <functional>
#include <imgui.h>

namespace DearTs::Plugins::Chat::UI {

/**
 * @brief 建议芯片样式配置
 */
struct SuggestionChipStyle {
    ImVec4 bg_color = ImVec4(0.1f, 0.1f, 0.12f, 1.0f);
    ImVec4 bg_hovered_color = ImVec4(0.15f, 0.15f, 0.18f, 1.0f);
    ImVec4 bg_active_color = ImVec4(0.2f, 0.2f, 0.23f, 1.0f);
    ImVec4 text_color = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);

    float corner_radius = 8.0f;
    float padding_x = 12.0f;
    float padding_y = 6.0f;

    // 是否显示置信度
    bool show_confidence = false;

    // 是否显示索引编号
    bool show_index = true;
};

/**
 * @brief AI 建议芯片组件
 * @details 显示 AI 生成的建议回复，点击可填充到输入框
 */
class SuggestionChip {
public:
    /**
     * @brief 绘制建议芯片
     * @param suggestion 建议对象
     * @param index 索引编号（可选）
     * @param style 样式配置（可选）
     * @return 是否被点击
     */
    static bool draw(
        const AISuggestion& suggestion,
        int index = -1,
        const SuggestionChipStyle* style = nullptr
    );

    /**
     * @brief 绘制芯片列表（水平滚动）
     * @param suggestions 建议列表
     * @param on_click 点击回调
     * @param style 样式配置（可选）
     */
    static void draw_list(
        const std::vector<AISuggestion>& suggestions,
        std::function<void(const AISuggestion&)> on_click,
        const SuggestionChipStyle* style = nullptr
    );

    /**
     * @brief 绘制芯片列表（垂直排列）
     */
    static void draw_list_vertical(
        const std::vector<AISuggestion>& suggestions,
        std::function<void(const AISuggestion&)> on_click,
        const SuggestionChipStyle* style = nullptr
    );

private:
    /**
     * @brief 绘制单个芯片
     */
    static bool draw_chip(
        const AISuggestion& suggestion,
        int index,
        const SuggestionChipStyle& style
    );

    /**
     * @brief 计算芯片大小
     */
    static ImVec2 calc_size(const std::string& text, const SuggestionChipStyle& style);
};

} // namespace DearTs::Plugins::Chat::UI
