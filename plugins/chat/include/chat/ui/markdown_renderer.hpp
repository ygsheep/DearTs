/**
 * @file markdown_renderer.hpp
 * @brief Markdown 渲染器 - 基于 imgui_markdown
 * @details 支持 Markdown 语法渲染，包括标题、加粗、斜体、列表、链接等
 *          特别处理代码块（带深色背景的等宽字体）
 */

#pragma once

#include <string>
#include <functional>
#include <imgui.h>
#include <imgui_markdown.h>

namespace DearTs::Plugins::Chat::UI {

/**
 * @brief 链接点击回调类型
 * @param url URL 地址
 */
using LinkCallback = std::function<void(const std::string& url)>;

/**
 * @brief Markdown 渲染器配置
 */
struct MarkdownRendererConfig {
    LinkCallback link_callback = nullptr;  // 链接点击回调
    bool enable_html = false;              // 是否支持 HTML 标签
    float max_width = 0.0f;                // 最大宽度（0 = 自动）

    // 代码块样式
    ImVec4 code_bg_color = ImVec4(0.15f, 0.15f, 0.18f, 1.0f);  // 代码块背景色
    ImVec4 code_text_color = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);   // 代码块文字颜色
    float code_padding = 8.0f;                                 // 代码块内边距
    float code_corner_radius = 6.0f;                          // 代码块圆角
};

/**
 * @brief Markdown 渲染器
 * @details 使用 imgui_markdown 库渲染 Markdown 内容
 */
class MarkdownRenderer {
public:
    /**
     * @brief 初始化 Markdown 渲染器
     * @param config 渲染器配置
     */
    static void initialize(const MarkdownRendererConfig& config);

    /**
     * @brief 渲染 Markdown 内容
     * @param markdown Markdown 文本
     */
    static void render(const std::string& markdown);

    /**
     * @brief 清理资源
     */
    static void cleanup();

    /**
     * @brief 设置字体
     * @param h1_font H1 标题字体
     * @param h2_font H2 标题字体
     * @param h3_font H3 标题字体
     * @param code_font 代码块字体（等宽）
     */
    static void set_fonts(ImFont* h1_font, ImFont* h2_font, ImFont* h3_font, ImFont* code_font);

    /**
     * @brief 获取当前配置
     */
    static const MarkdownRendererConfig& get_config();

private:
    /**
     * @brief 预处理代码块
     * @details 将 ``` 代码块转换为带背景的等宽文本
     * @param markdown 原始 Markdown 文本
     * @return 处理后的文本
     */
    static std::string preprocess_code_blocks(const std::string& markdown);

    /**
     * @brief 渲染带背景的代码块
     * @param code 代码内容
     * @param language 语言标识（可选）
     */
    static void render_code_block(const std::string& code, const std::string& language = "");

    // imgui_markdown 回调函数
    static void link_callback(ImGui::MarkdownLinkCallbackData data);
    static ImGui::MarkdownImageData image_callback(ImGui::MarkdownLinkCallbackData data);

    // 静态成员
    static MarkdownRendererConfig s_config;
    static ImGui::MarkdownConfig s_md_config;
    static ImFont* s_h1_font;
    static ImFont* s_h2_font;
    static ImFont* s_h3_font;
    static ImFont* s_code_font;
    static bool s_initialized;
};

} // namespace DearTs::Plugins::Chat::UI
