/**
 * @file imgui_extensions.cpp
 * @brief ImGui 扩展工具实现
 */

#include "imgui_extensions.h"

namespace DearTs::Core::UI {

namespace ImGuiExt {

bool TitleBarButton(const char* label, const ImVec2& size) {
    // 使用公共 API 实现标题栏按钮
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.5f, 0.5f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.6f, 0.6f, 0.6f));

    bool result = ImGui::Button(label, size);

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);

    return result;
}

bool ButtonColored(const char* label, const ImVec4& color, const ImVec2& size) {
    ImGui::PushStyleColor(ImGuiCol_Button, color);
    bool result = ImGui::Button(label, size);
    ImGui::PopStyleColor();

    return result;
}

bool ButtonColoredEx(const char* label, const ImVec4& bg_color, const ImVec4& text_color) {
    ImGui::PushStyleColor(ImGuiCol_Button, bg_color);
    ImGui::PushStyleColor(ImGuiCol_Text, text_color);
    bool result = ImGui::Button(label);
    ImGui::PopStyleColor(2);

    return result;
}

bool IconButton(const char* icon_id, const ImVec2& size, const char* tooltip) {
    ImGui::PushID(icon_id);
    bool result = TitleBarButton(icon_id, size);

    if (tooltip && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("%s", tooltip);
    }

    ImGui::PopID();

    return result;
}

void HelpMarker(const char* desc) {
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void BulletText(const char* text) {
    ImGui::Bullet();
    ImGui::SameLine();
    ImGui::Text("%s", text);
}

void Spacing(float width) {
    ImGui::Dummy(ImVec2(width, 0));
}

} // namespace ImGuiExt

} // namespace DearTs::Core::UI
