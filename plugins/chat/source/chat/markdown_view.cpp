/**
 * @file markdown_view.cpp
 * @brief Markdown 预览视图实现
 */

#include "chat/views/markdown_view.hpp"
#include "chat/ui/markdown_renderer.hpp"
#include <imgui.h>

namespace DearTs::Plugins::Chat {

MarkdownView::MarkdownView(const std::string& content, const std::string& message_id)
    : ViewWindow(UnlocalizedString("Markdown##" + message_id), ICON_CODE)
    , m_content(content)
    , m_message_id(message_id)
{
    // 生成唯一的窗口名称
    m_window_name = "Markdown##" + m_message_id;
}

void MarkdownView::draw_content() {
    // 获取可用区域大小
    const ImVec2 avail_size = ImGui::GetContentRegionAvail();

    // 创建子窗口用于 Markdown 内容滚动
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 16.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.08f, 0.1f, 1.0f));

    if (ImGui::BeginChild("##markdown_content", avail_size, false, ImGuiWindowFlags_None)) {
        // 渲染 Markdown 内容
        UI::MarkdownRenderer::render(m_content);
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();  // ChildBg
    ImGui::PopStyleVar(2);    // WindowPadding, ChildRounding
}

} // namespace DearTs::Plugins::Chat
