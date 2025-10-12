#pragma once

#include "../../layouts/layout_base.h"
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <imgui.h>
#include "text_segmenter.h"
#include "url_extractor.h"

namespace DearTs::Core::Window::Widgets::Clipboard {

// 使用text_segmenter.h中已定义的TextSegment结构
// 使用url_extractor.h中已定义的UrlInfo结构

/**
 * @brief 分词布局类
 *
 * 显示文本分词结果的布局组件，具有以下特性：
 * - URL优先显示在最前面
 * - 文本分词显示，每个词都有边框
 * - 鼠标悬停时改变背景颜色
 * - 点击选中效果
 * - 单击URL复制，双击打开浏览器
 */
class TextSegmentationLayout : public LayoutBase {
public:
    TextSegmentationLayout();
    ~TextSegmentationLayout() override;

    // LayoutBase 接口
    void render() override;
    void updateLayout(float width, float height) override;
    void handleEvent(const SDL_Event& event) override;

    // 设置要分析的文本
    void setText(const std::string& text);

    // 获取当前文本
    std::string getText() const;

    // 设置内容（别名，为了兼容性）
    void setContent(const std::string& text) { setText(text); }

    // 获取内容（别名，为了兼容性）
    std::string getContent() const { return getText(); }

    // 显示/隐藏窗口
    void showWindow();
    void hideWindow();
    bool isVisible() const { return is_visible_; }

private:
    // 渲染组件
    void renderTranslucentBackground();
    void renderUrlSection();              // URL区域（优先显示）
    void renderSegmentedText();           // 分词文本区域
    void renderToolbar();                 // 工具栏
    void renderTextSegment(const TextSegment& segment);
    void renderUrlItem(const UrlInfo& url_info);

    // 交互处理
    void handleMouseInteraction();
    void handleUrlInteraction();
    void handleTextSelection();
    void handleKeyboardInput();
    void handleContextMenu();

    // 颜色和样式管理
    void updateSegmentColors(TextSegment& segment);
    void updateUrlColors(UrlInfo& url_info);
    void resetSegmentStates();
    void resetUrlStates();

    // 功能操作
    void copySelectedText();
    void copyUrl(const std::string& url);
    void openUrlInBrowser(const std::string& url);
    void exportSegments();
    void togglePosTags();
    void selectAllSegments();

    // 布局计算
    void calculateLayout();
    ImVec2 calculateSegmentSize(const std::string& text);
    ImVec2 calculateUrlSize(const std::string& url);
    void arrangeTextSegments();
    void arrangeUrlItems();

    // 成员变量
    std::string original_text_;           // 原始文本
    std::vector<TextSegment> text_segments_;  // 文本片段列表
    std::vector<UrlInfo> url_infos_;     // URL信息列表

    std::unique_ptr<TextSegmenter> text_segmenter_; // 文本分词器实例
    bool is_segmenter_initialized_;             // 分词器是否已初始化

    // 状态管理
    bool is_visible_ = false;            // 窗口是否可见
    bool show_pos_tags_ = true;          // 是否显示词性标签
    bool show_urls_first_ = true;        // 是否优先显示URL
    int selected_segment_count_ = 0;     // 选中的片段数量
    int hovered_segment_ = -1;           // 悬停的片段索引
    int hovered_url_ = -1;               // 悬停的URL索引
    int clicked_url_ = -1;               // 点击的URL索引
    std::chrono::steady_clock::time_point last_click_time_; // 上次点击时间
    bool is_double_click_ = false;       // 是否双击

    // UI样式配置
    float window_opacity_ = 0.85f;       // 窗口透明度
    ImVec2 window_size_;                 // 窗口大小
    ImVec2 window_position_;             // 窗口位置
    ImVec2 content_margin_;              // 内容边距

    // 颜色配置
    struct Colors {
        ImVec4 window_bg;                // 窗口背景
        ImVec4 text_normal;              // 普通文本
        ImVec4 text_hovered;             // 悬停文本
        ImVec4 text_selected;            // 选中文本
        ImVec4 url_normal;               // 普通URL
        ImVec4 url_hovered;              // 悬停URL
        ImVec4 url_selected;             // 选中URL
        ImVec4 border_normal;            // 普通边框
        ImVec4 border_hovered;           // 悬停边框
        ImVec4 border_selected;          // 选中边框
        ImVec4 bg_normal;                // 普通背景
        ImVec4 bg_hovered;               // 悬停背景
        ImVec4 bg_selected;              // 选中背景
        ImVec4 tag_color;                // 词性标签颜色
    } colors_;

    // 布局参数
    struct Layout {
        float segment_spacing = 4.0f;    // 片段间距
        float line_spacing = 8.0f;       // 行间距
        float url_section_height = 120.0f; // URL区域高度
        float toolbar_height = 40.0f;     // 工具栏高度
        float corner_radius = 4.0f;      // 圆角半径
        float border_width = 1.0f;       // 默认边框宽度
        float padding = 8.0f;            // 内边距
    } layout_;

    // 初始化方法
    void initializeColors();
    void initializeTextSegmenter();
    void initializeLayout();

    // 文本处理
    void extractAndProcessText();
    void extractUrls();
    void performTextSegmentation();
};

} // namespace DearTs::Core::Window::Widgets::Clipboard