/**
 * @file markdown_renderer.cpp
 * @brief Markdown 渲染器实现
 */

#include "chat/ui/markdown_renderer.hpp"
#include "chat/ui/chat_theme.hpp"
#include "liblogger/logger.h"
#include <regex>
#include <sstream>

namespace DearTs::Plugins::Chat::UI {

// 静态成员初始化
MarkdownRendererConfig MarkdownRenderer::s_config;
ImGui::MarkdownConfig MarkdownRenderer::s_md_config;
ImFont* MarkdownRenderer::s_h1_font = nullptr;
ImFont* MarkdownRenderer::s_h2_font = nullptr;
ImFont* MarkdownRenderer::s_h3_font = nullptr;
ImFont* MarkdownRenderer::s_code_font = nullptr;
bool MarkdownRenderer::s_initialized = false;

void MarkdownRenderer::initialize(const MarkdownRendererConfig& config) {
    s_config = config;

    // 配置 imgui_markdown
    s_md_config.linkCallback = link_callback;
    s_md_config.imageCallback = image_callback;
    s_md_config.userData = nullptr;

    // 配置标题格式
    s_md_config.headingFormats[0] = { s_h1_font, true };
    s_md_config.headingFormats[1] = { s_h2_font, true };
    s_md_config.headingFormats[2] = { s_h3_font, true };

    s_initialized = true;

    LOG_INFO("MarkdownRenderer: Initialized");
}

/**
 * @brief Markdown 段落类型
 */
enum class MarkdownSegmentType {
    Regular,   // 普通 Markdown（标题、列表、粗体等）
    CodeBlock  // 代码块
};

/**
 * @brief Markdown 段落
 */
struct MarkdownSegment {
    MarkdownSegmentType type;
    std::string content;      // 对于 Regular 是 markdown，对于 CodeBlock 是代码
    std::string language;     // 仅用于 CodeBlock：语言标识
};

/**
 * @brief 解析 Markdown 为段落列表
 */
static std::vector<MarkdownSegment> parse_markdown_segments(const std::string& markdown) {
    std::vector<MarkdownSegment> segments;
    std::string current_segment;
    bool in_code_block = false;
    std::string code_language;
    std::string code_content;

    std::istringstream stream(markdown);
    std::string line;

    while (std::getline(stream, line)) {
        // 检测代码块开始
        if (!in_code_block && line.find("```") == 0) {
            // 先保存当前段落（如果有）
            if (!current_segment.empty()) {
                segments.push_back({MarkdownSegmentType::Regular, current_segment, ""});
                current_segment.clear();
            }

            in_code_block = true;
            code_language = line.substr(3);  // 获取语言标识（如果有）
            // 去除语言标识前的空格
            while (!code_language.empty() && code_language[0] == ' ') {
                code_language.erase(0, 1);
            }
            code_content.clear();
            continue;
        }

        // 检测代码块结束
        if (in_code_block && line.find("```") == 0) {
            in_code_block = false;

            // 添加代码块段落
            segments.push_back({MarkdownSegmentType::CodeBlock, code_content, code_language});
            continue;
        }

        // 在代码块内，收集内容
        if (in_code_block) {
            if (!code_content.empty()) {
                code_content += "\n";
            }
            code_content += line;
        } else {
            // 普通文本
            current_segment += line + "\n";
        }
    }

    // 处理剩余内容
    if (in_code_block) {
        // 代码块未闭合
        segments.push_back({MarkdownSegmentType::CodeBlock, code_content, code_language});
    } else if (!current_segment.empty()) {
        segments.push_back({MarkdownSegmentType::Regular, current_segment, ""});
    }

    return segments;
}

void MarkdownRenderer::render(const std::string& markdown) {
    if (!s_initialized) {
        LOG_WARN("MarkdownRenderer: Not initialized, rendering as plain text");
        ImGui::Text("%s", markdown.c_str());
        return;
    }

    if (markdown.empty()) {
        return;
    }

    // 解析 Markdown 为段落
    std::vector<MarkdownSegment> segments = parse_markdown_segments(markdown);

    // 逐段渲染
    for (const auto& segment : segments) {
        if (segment.type == MarkdownSegmentType::CodeBlock) {
            // 渲染代码块
            render_code_block(segment.content, segment.language);
        } else {
            // 使用 imgui_markdown 渲染普通 Markdown
            ImGui::Markdown(segment.content.c_str(), segment.content.length(), s_md_config);
        }
    }
}

void MarkdownRenderer::cleanup() {
    s_initialized = false;
    LOG_INFO("MarkdownRenderer: Cleaned up");
}

void MarkdownRenderer::set_fonts(ImFont* h1_font, ImFont* h2_font, ImFont* h3_font, ImFont* code_font) {
    s_h1_font = h1_font;
    s_h2_font = h2_font;
    s_h3_font = h3_font;
    s_code_font = code_font;

    // 总是更新配置（无论是否已初始化）
    // 因为 set_fonts() 可能在 initialize() 之前被调用
    s_md_config.headingFormats[0] = { s_h1_font, true };
    s_md_config.headingFormats[1] = { s_h2_font, true };
    s_md_config.headingFormats[2] = { s_h3_font, true };
}

const MarkdownRendererConfig& MarkdownRenderer::get_config() {
    return s_config;
}

void MarkdownRenderer::render_code_block(const std::string& code, const std::string& language) {
    // 保存当前字体
    ImFont* current_font = ImGui::GetFont();
    if (s_code_font) {
        ImGui::PushFont(s_code_font);
    }

    // 计算代码块大小
    ImVec2 content_size = ImGui::CalcTextSize(code.c_str(), nullptr, false, ImGui::GetContentRegionAvail().x - s_config.code_padding * 2);

    // 绘制背景
    ImVec2 p_min = ImGui::GetCursorScreenPos();
    ImVec2 p_max = ImVec2(
        p_min.x + ImGui::GetContentRegionAvail().x,
        p_min.y + content_size.y + s_config.code_padding * 2
    );

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // 从 ChatTheme 获取代码块颜色
    ImVec4 code_bg = ChatTheme::getCodeBlockBg();
    ImVec4 code_text = ChatTheme::getCodeBlockText();
    const ImU32 bg_color = ImGui::ColorConvertFloat4ToU32(code_bg);

    draw_list->AddRectFilled(
        p_min,
        p_max,
        bg_color,
        s_config.code_corner_radius
    );

    // 绘制边框（使用更暗的背景色作为边框）
    ImVec4 border_color_vec = ChatTheme::darkenColor(code_bg, 0.1f);
    const ImU32 border_color = ImGui::ColorConvertFloat4ToU32(border_color_vec);
    draw_list->AddRect(
        p_min,
        p_max,
        border_color,
        s_config.code_corner_radius,
        0,
        1.0f
    );

    // 绘制语言标识（如果有）
    if (!language.empty()) {
        ImGui::SetCursorScreenPos(ImVec2(p_min.x + s_config.code_padding, p_min.y + s_config.code_padding));
        // 使用更淡的文本颜色作为语言标签
        ImVec4 label_color = ChatTheme::withAlpha(code_text, 0.7f);
        ImGui::PushStyleColor(ImGuiCol_Text, label_color);
        ImGui::Text("%s", language.c_str());
        ImGui::PopStyleColor();

        // 调整代码内容的起始位置
        ImGui::SetCursorScreenPos(ImVec2(p_min.x + s_config.code_padding, p_min.y + s_config.code_padding + ImGui::GetFontSize()));
    } else {
        ImGui::SetCursorScreenPos(ImVec2(p_min.x + s_config.code_padding, p_min.y + s_config.code_padding));
    }

    // 绘制代码内容
    ImGui::PushStyleColor(ImGuiCol_Text, code_text);
    ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x - s_config.code_padding);
    ImGui::Text("%s", code.c_str());
    ImGui::PopTextWrapPos();
    ImGui::PopStyleColor();

    // 恢复字体
    if (s_code_font) {
        ImGui::PopFont();
    }

    // 设置下一个光标位置
    ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorStartPos().x, p_max.y + 8.0f));
}

void MarkdownRenderer::link_callback(ImGui::MarkdownLinkCallbackData data) {
    std::string url(data.link, data.linkLength);
    LOG_INFO("MarkdownRenderer: Link clicked: {}", url);

    if (s_config.link_callback) {
        s_config.link_callback(url);
    } else {
        // 默认行为：输出日志
        LOG_WARN("MarkdownRenderer: No link callback set for URL: {}", url);
    }
}

ImGui::MarkdownImageData MarkdownRenderer::image_callback(ImGui::MarkdownLinkCallbackData data) {
    // 默认实现：返回占位符
    ImGui::MarkdownImageData imageData;
    imageData.isValid = false;
    imageData.useLinkCallback = false;

    std::string image_id(data.link, data.linkLength);
    LOG_DEBUG("MarkdownRenderer: Image requested: {}", image_id);

    // 可以在这里实现图片加载逻辑
    // 例如：从缓存加载图片，或使用占位符

    return imageData;
}

} // namespace DearTs::Plugins::Chat::UI
