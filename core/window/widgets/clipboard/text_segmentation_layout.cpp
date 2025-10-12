#include "text_segmentation_layout.h"
#include <regex>
#include <algorithm>
#include <Windows.h>
#include <shellapi.h>
#include "../../utils/logger.h"

namespace DearTs::Core::Window::Widgets::Clipboard {

TextSegmentationLayout::TextSegmentationLayout()
    : LayoutBase("TextSegmentation")
    , is_segmenter_initialized_(false)
    , selected_segment_count_(0)
    , hovered_segment_(-1)
    , hovered_url_(-1)
    , clicked_url_(-1)
    , is_double_click_(false) {

    DEARTS_LOG_INFO("TextSegmentationLayout构造函数");

    initializeColors();
    initializeLayout();
    initializeTextSegmenter();
}

TextSegmentationLayout::~TextSegmentationLayout() {
    DEARTS_LOG_INFO("TextSegmentationLayout析构函数");
}

void TextSegmentationLayout::initializeColors() {
    // 初始化颜色配置
    colors_.window_bg = ImVec4(0.15f, 0.15f, 0.15f, 0.85f);
    colors_.text_normal = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
    colors_.text_hovered = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    colors_.text_selected = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    colors_.url_normal = ImVec4(0.4f, 0.6f, 1.0f, 1.0f);
    colors_.url_hovered = ImVec4(0.6f, 0.8f, 1.0f, 1.0f);
    colors_.url_selected = ImVec4(0.2f, 0.4f, 0.8f, 1.0f);
    colors_.border_normal = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
    colors_.border_hovered = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
    colors_.border_selected = ImVec4(0.2f, 0.4f, 0.8f, 1.0f);
    colors_.bg_normal = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors_.bg_hovered = ImVec4(0.2f, 0.2f, 0.3f, 0.8f);
    colors_.bg_selected = ImVec4(0.2f, 0.4f, 0.6f, 0.9f);
    colors_.tag_color = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
}

void TextSegmentationLayout::initializeLayout() {
    window_size_ = ImVec2(600, 500);
    content_margin_ = ImVec2(10.0f, 10.0f);
}

void TextSegmentationLayout::initializeTextSegmenter() {
    try {
        text_segmenter_ = std::make_unique<TextSegmenter>();

        if (text_segmenter_->initialize()) {
            is_segmenter_initialized_ = true;
            DEARTS_LOG_INFO("文本分词器初始化成功");
        } else {
            DEARTS_LOG_ERROR("文本分词器初始化失败");
            is_segmenter_initialized_ = false;
        }
    } catch (const std::exception& e) {
        DEARTS_LOG_ERROR("文本分词器初始化异常: " + std::string(e.what()));
        is_segmenter_initialized_ = false;
    }
}

void TextSegmentationLayout::render() {
    if (!is_visible_) {
        return;
    }

    // 设置窗口位置和大小 - 简单居中显示
    ImVec2 display_size = ImGui::GetIO().DisplaySize;
    window_position_ = ImVec2((display_size.x - window_size_.x) * 0.5f,
                              (display_size.y - window_size_.y) * 0.5f);

    ImGui::SetNextWindowPos(window_position_, ImGuiCond_Appearing);
    ImGui::SetNextWindowSize(window_size_, ImGuiCond_Appearing);

    // 设置窗口样式
    ImGui::PushStyleColor(ImGuiCol_WindowBg, colors_.window_bg);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, layout_.corner_radius);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, content_margin_);

    bool window_open = true;
    if (ImGui::Begin("文本分词分析", &window_open,
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar |
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {

        // 渲染透明背景
        renderTranslucentBackground();

        // 渲染工具栏
        renderToolbar();

        // 如果有URL，优先渲染URL区域
        if (!url_infos_.empty()) {
            renderUrlSection();
        }

        // 渲染分词文本
        renderSegmentedText();

        // 处理鼠标交互
        handleMouseInteraction();

    }
    ImGui::End();

    // 恢复样式
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();

    // 如果窗口被关闭，隐藏布局
    if (!window_open) {
        hideWindow();
    }
}

void TextSegmentationLayout::renderTranslucentBackground() {
    // 透明背景效果已经通过窗口样式设置
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.3f));
}

void TextSegmentationLayout::renderUrlSection() {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.15f, 0.5f));

    if (ImGui::BeginChild("URLSection", ImVec2(0, layout_.url_section_height), true)) {
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "🔗 检测到的链接 (%d)", url_infos_.size());
        ImGui::Separator();

        for (size_t i = 0; i < url_infos_.size(); ++i) {
            renderUrlItem(url_infos_[i]);
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void TextSegmentationLayout::renderUrlItem(const UrlInfo& url_info) {
    // 更新URL颜色状态
    const_cast<UrlInfo&>(url_info) = const_cast<const UrlInfo&>(url_info);
    UrlInfo& mutable_url = const_cast<UrlInfo&>(url_info);
    updateUrlColors(mutable_url);

    // 设置URL项目样式
    ImGui::PushStyleColor(ImGuiCol_Button, mutable_url.bg_color);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors_.bg_hovered);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, colors_.bg_selected);
    ImGui::PushStyleColor(ImGuiCol_Text, mutable_url.text_color);
    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, mutable_url.border_width);

    // 渲染URL按钮
    std::string display_text = mutable_url.url;
    if (display_text.length() > 50) {
        display_text = display_text.substr(0, 47) + "...";
    }

    ImVec2 button_size = calculateUrlSize(display_text);
    if (ImGui::Button(display_text.c_str(), button_size)) {
        // 单击复制URL
        copyUrl(mutable_url.url);
        DEARTS_LOG_INFO("复制URL: " + mutable_url.url);
    }

    // 绘制边框
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 button_min = ImGui::GetItemRectMin();
    ImVec2 button_max = ImGui::GetItemRectMax();
    draw_list->AddRect(button_min, button_max,
                      ImGui::ColorConvertFloat4ToU32(mutable_url.border_color),
                      layout_.corner_radius, 0, mutable_url.border_width);

    // 恢复样式
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(4);

    // 添加提示文本
    ImGui::SameLine();
    ImGui::TextColored(colors_.tag_color, "[单击复制 双击打开]");
}

void TextSegmentationLayout::renderSegmentedText() {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.2f));

    float remaining_height = ImGui::GetContentRegionAvail().y;
    if (ImGui::BeginChild("SegmentedText", ImVec2(0, remaining_height), true)) {
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.4f, 1.0f), "📝 文本分词结果");
        ImGui::Separator();

        // 渲染文本片段
        for (auto& segment : text_segments_) {
            renderTextSegment(segment);
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void TextSegmentationLayout::renderTextSegment(const TextSegment& segment) {
    // 更新片段颜色状态
    TextSegment& mutable_segment = const_cast<TextSegment&>(segment);
    updateSegmentColors(mutable_segment);

    // 设置片段样式
    ImGui::PushStyleColor(ImGuiCol_Button, mutable_segment.bg_color);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors_.bg_hovered);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, colors_.bg_selected);
    ImGui::PushStyleColor(ImGuiCol_Text, mutable_segment.is_selected ? colors_.text_selected : colors_.text_normal);
    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, mutable_segment.border_width);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 2.0f));

    // 构建显示文本
    std::string display_text = segment.text;
    if (show_pos_tags_ && !segment.tag.empty()) {
        display_text += "/" + segment.tag;
    }

    // 渲染片段按钮
    ImVec2 button_size = calculateSegmentSize(display_text);
    if (ImGui::Button(display_text.c_str(), button_size)) {
        // 切换选中状态
        mutable_segment.is_selected = !mutable_segment.is_selected;
        if (mutable_segment.is_selected) {
            selected_segment_count_++;
        } else {
            selected_segment_count_--;
        }
        DEARTS_LOG_INFO("文本片段选中状态变更: " + segment.text + " -> " +
                       (mutable_segment.is_selected ? "选中" : "取消选中"));
    }

    // 绘制边框
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 button_min = ImGui::GetItemRectMin();
    ImVec2 button_max = ImGui::GetItemRectMax();
    draw_list->AddRect(button_min, button_max,
                      ImGui::ColorConvertFloat4ToU32(mutable_segment.border_color),
                      layout_.corner_radius, 0, mutable_segment.border_width);

    // 恢复样式
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(4);

    // 添加间距（在同一行内）
    ImGui::SameLine(0.0f, layout_.segment_spacing);
}

void TextSegmentationLayout::renderToolbar() {
    ImGui::Separator();

    // 工具栏按钮
    if (ImGui::Button("📋 复制选中")) {
        copySelectedText();
    }
    ImGui::SameLine();

    if (ImGui::Button(show_pos_tags_ ? "🏷️ 隐藏词性" : "🏷️ 显示词性")) {
        togglePosTags();
    }
    ImGui::SameLine();

    if (ImGui::Button("🔄 全选")) {
        selectAllSegments();
    }
    ImGui::SameLine();

    if (ImGui::Button("💾 导出")) {
        exportSegments();
    }

    ImGui::SameLine();
    ImGui::Text("选中: %d 个片段", selected_segment_count_);

    ImGui::Separator();
}

void TextSegmentationLayout::updateSegmentColors(TextSegment& segment) {
    if (segment.is_selected) {
        segment.bg_color = colors_.bg_selected;
        segment.border_color = colors_.border_selected;
        segment.border_width = 2.0f;
    } else if (segment.is_hovered) {
        segment.bg_color = colors_.bg_hovered;
        segment.border_color = colors_.border_hovered;
        segment.border_width = 1.5f;
    } else {
        segment.bg_color = colors_.bg_normal;
        segment.border_color = colors_.border_normal;
        segment.border_width = layout_.border_width;
    }
}

void TextSegmentationLayout::updateUrlColors(UrlInfo& url_info) {
    if (url_info.is_selected) {
        url_info.bg_color = colors_.bg_selected;
        url_info.border_color = colors_.border_selected;
        url_info.text_color = colors_.text_selected;
        url_info.border_width = 2.0f;
    } else if (url_info.is_hovered) {
        url_info.bg_color = colors_.bg_hovered;
        url_info.border_color = colors_.border_hovered;
        url_info.text_color = colors_.url_hovered;
        url_info.border_width = 1.5f;
    } else {
        url_info.bg_color = colors_.bg_normal;
        url_info.border_color = colors_.url_normal;
        url_info.text_color = colors_.url_normal;
        url_info.border_width = layout_.border_width;
    }
}

ImVec2 TextSegmentationLayout::calculateSegmentSize(const std::string& text) {
    ImVec2 text_size = ImGui::CalcTextSize(text.c_str());
    return ImVec2(text_size.x + 8.0f, text_size.y + 4.0f); // 加上内边距
}

ImVec2 TextSegmentationLayout::calculateUrlSize(const std::string& url) {
    ImVec2 text_size = ImGui::CalcTextSize(url.c_str());
    return ImVec2(text_size.x + 12.0f, text_size.y + 6.0f); // URL按钮稍大一些
}

void TextSegmentationLayout::handleMouseInteraction() {
    ImVec2 mouse_pos = ImGui::GetMousePos();

    // 重置悬停状态
    resetSegmentStates();
    resetUrlStates();

    // 检查文本片段悬停
    for (auto& segment : text_segments_) {
        ImVec2 segment_min = segment.position;
        ImVec2 segment_max = ImVec2(segment.position.x + segment.size.x,
                                  segment.position.y + segment.size.y);

        if (mouse_pos.x >= segment_min.x && mouse_pos.x <= segment_max.x &&
            mouse_pos.y >= segment_min.y && mouse_pos.y <= segment_max.y) {
            segment.is_hovered = true;
            hovered_segment_ = segment.index;
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            break;
        }
    }

    // 检查URL悬停
    for (size_t i = 0; i < url_infos_.size(); ++i) {
        auto& url_info = url_infos_[i];
        ImVec2 url_min = url_info.position;
        ImVec2 url_max = ImVec2(url_info.position.x + url_info.size.x,
                              url_info.position.y + url_info.size.y);

        if (mouse_pos.x >= url_min.x && mouse_pos.x <= url_max.x &&
            mouse_pos.y >= url_min.y && mouse_pos.y <= url_max.y) {
            url_info.is_hovered = true;
            hovered_url_ = static_cast<int>(i);
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            break;
        }
    }

    // 处理双击URL
    if (ImGui::IsMouseDoubleClicked(0) && hovered_url_ >= 0) {
        openUrlInBrowser(url_infos_[hovered_url_].url);
        DEARTS_LOG_INFO("双击打开URL: " + url_infos_[hovered_url_].url);
    }
}

void TextSegmentationLayout::resetSegmentStates() {
    for (auto& segment : text_segments_) {
        segment.is_hovered = false;
    }
    hovered_segment_ = -1;
}

void TextSegmentationLayout::resetUrlStates() {
    for (auto& url_info : url_infos_) {
        url_info.is_hovered = false;
    }
    hovered_url_ = -1;
}

void TextSegmentationLayout::copyUrl(const std::string& url) {
    if (OpenClipboard(nullptr)) {
        EmptyClipboard();

        HGLOBAL hClipboardData = GlobalAlloc(GMEM_MOVEABLE, url.length() + 1);
        if (hClipboardData) {
            char* pchData = static_cast<char*>(GlobalLock(hClipboardData));
            if (pchData) {
                strcpy_s(pchData, url.length() + 1, url.c_str());
                GlobalUnlock(hClipboardData);
                SetClipboardData(CF_TEXT, hClipboardData);
                DEARTS_LOG_INFO("URL已复制到剪贴板: " + url);
            }
        }
        CloseClipboard();
    }
}

void TextSegmentationLayout::openUrlInBrowser(const std::string& url) {
    ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    DEARTS_LOG_INFO("在浏览器中打开URL: " + url);
}

void TextSegmentationLayout::copySelectedText() {
    std::string selected_text;
    for (const auto& segment : text_segments_) {
        if (segment.is_selected) {
            if (!selected_text.empty()) {
                selected_text += " ";
            }
            selected_text += segment.text;
        }
    }

    if (!selected_text.empty() && OpenClipboard(nullptr)) {
        EmptyClipboard();

        HGLOBAL hClipboardData = GlobalAlloc(GMEM_MOVEABLE, selected_text.length() + 1);
        if (hClipboardData) {
            char* pchData = static_cast<char*>(GlobalLock(hClipboardData));
            if (pchData) {
                strcpy_s(pchData, selected_text.length() + 1, selected_text.c_str());
                GlobalUnlock(hClipboardData);
                SetClipboardData(CF_TEXT, hClipboardData);
                DEARTS_LOG_INFO("选中文本已复制到剪贴板: " + selected_text);
            }
        }
        CloseClipboard();
    }
}

void TextSegmentationLayout::togglePosTags() {
    show_pos_tags_ = !show_pos_tags_;
    DEARTS_LOG_INFO("词性标签显示: " + std::string(show_pos_tags_ ? "开启" : "关闭"));
}

void TextSegmentationLayout::selectAllSegments() {
    selected_segment_count_ = 0;
    for (auto& segment : text_segments_) {
        segment.is_selected = true;
        selected_segment_count_++;
    }
    DEARTS_LOG_INFO("全选所有文本片段，共 " + std::to_string(selected_segment_count_) + " 个");
}

void TextSegmentationLayout::exportSegments() {
    // 导出功能实现（可以导出到文件或剪贴板）
    std::string export_text = "=== 文本分词导出 ===\n";
    export_text += "原始文本: " + original_text_ + "\n\n";

    export_text += "URL链接:\n";
    for (const auto& url_info : url_infos_) {
        export_text += "- " + url_info.url + "\n";
    }

    export_text += "\n分词结果:\n";
    for (const auto& segment : text_segments_) {
        export_text += segment.text;
        if (show_pos_tags_ && !segment.tag.empty()) {
            export_text += "/" + segment.tag;
        }
        export_text += " ";
    }

    if (OpenClipboard(nullptr)) {
        EmptyClipboard();
        HGLOBAL hClipboardData = GlobalAlloc(GMEM_MOVEABLE, export_text.length() + 1);
        if (hClipboardData) {
            char* pchData = static_cast<char*>(GlobalLock(hClipboardData));
            if (pchData) {
                strcpy_s(pchData, export_text.length() + 1, export_text.c_str());
                GlobalUnlock(hClipboardData);
                SetClipboardData(CF_TEXT, hClipboardData);
                DEARTS_LOG_INFO("分词结果已导出到剪贴板");
            }
        }
        CloseClipboard();
    }
}

void TextSegmentationLayout::setText(const std::string& text) {
    DEARTS_LOG_INFO("设置分词窗口文本，长度: " + std::to_string(text.length()));
    original_text_ = text;

    // 处理文本
    extractAndProcessText();

    // 显示窗口
    showWindow();

    DEARTS_LOG_DEBUG("分词处理完成 - URL数量: " + std::to_string(url_infos_.size()) +
                    ", 文本片段数量: " + std::to_string(text_segments_.size()) +
                    ", 窗口可见性: " + (is_visible_ ? "可见" : "隐藏"));
}

void TextSegmentationLayout::extractAndProcessText() {
    text_segments_.clear();
    url_infos_.clear();

    // 提取URL
    extractUrls();

    // 执行文本分词
    performTextSegmentation();

    // 计算布局
    calculateLayout();
}

void TextSegmentationLayout::extractUrls() {
    static const std::regex url_regex(
        R"(https?:\/\/(www\.)?[-a-zA-Z0-9@:%._\+~#=]{1,256}\.[a-zA-Z0-9()]{1,6}\b([-a-zA-Z0-9()@:%_\+.~#?&//=]*))"
    );

    std::sregex_iterator iter(original_text_.begin(), original_text_.end(), url_regex);
    std::sregex_iterator end;

    int url_index = 0;
    for (; iter != end; ++iter) {
        UrlInfo url_info;
        url_info.url = iter->str();
        url_info.domain = url_info.url;
        url_info.discovered_time = std::chrono::system_clock::now();
        url_info.index = url_index++;
        url_infos_.push_back(url_info);
    }

    DEARTS_LOG_INFO("提取到 " + std::to_string(url_infos_.size()) + " 个URL");
}

void TextSegmentationLayout::performTextSegmentation() {
    if (!is_segmenter_initialized_) {
        DEARTS_LOG_WARN("文本分词器未初始化，跳过分词处理");
        return;
    }

    try {
        auto segments = text_segmenter_->segmentText(original_text_, TextSegmenter::Method::MIXED_MODE);

        int segment_index = 0;
        for (const auto& seg : segments) {
            TextSegment segment;
            segment.text = seg.text;
            segment.tag = seg.tag;
            segment.start_pos = seg.start_pos;
            segment.end_pos = seg.end_pos;
            segment.index = segment_index++;
            segment.is_selected = false;
            segment.is_hovered = false;

            text_segments_.push_back(segment);
        }

        DEARTS_LOG_INFO("文本分词完成，共分得 " + std::to_string(text_segments_.size()) + " 个词");

    } catch (const std::exception& e) {
        DEARTS_LOG_ERROR("文本分词过程出错: " + std::string(e.what()));
    }
}

void TextSegmentationLayout::calculateLayout() {
    // 检查是否在渲染上下文中
    if (!ImGui::GetCurrentContext()) {
        DEARTS_LOG_WARN("ImGui上下文未初始化，跳过布局计算");
        return;
    }

    arrangeUrlItems();
    arrangeTextSegments();
}

void TextSegmentationLayout::arrangeUrlItems() {
    // 检查ImGui上下文是否有效
    if (!ImGui::GetCurrentContext()) {
        return;
    }

    // URL布局计算
    ImVec2 current_pos = ImGui::GetCursorScreenPos();
    current_pos.x += layout_.padding;
    current_pos.y += layout_.padding;

    for (auto& url_info : url_infos_) {
        url_info.position = current_pos;
        url_info.size = calculateUrlSize(url_info.url);

        // 简单的换行布局
        current_pos.x += url_info.size.x + layout_.segment_spacing;
        if (current_pos.x > window_size_.x - 100) { // 预留边距
            current_pos.x = layout_.padding;
            current_pos.y += url_info.size.y + layout_.line_spacing;
        }
    }
}

void TextSegmentationLayout::arrangeTextSegments() {
    // 检查ImGui上下文是否有效
    if (!ImGui::GetCurrentContext()) {
        return;
    }

    // 文本片段布局计算
    ImVec2 current_pos = ImGui::GetCursorScreenPos();
    current_pos.x += layout_.padding;
    current_pos.y += layout_.padding;

    for (auto& segment : text_segments_) {
        segment.size = calculateSegmentSize(
            segment.text + (show_pos_tags_ && !segment.tag.empty() ? "/" + segment.tag : "")
        );

        // 简单的换行布局
        if (current_pos.x + segment.size.x > window_size_.x - 50) { // 预留边距
            current_pos.x = layout_.padding;
            current_pos.y += segment.size.y + layout_.line_spacing;
        }

        segment.position = current_pos;
        current_pos.x += segment.size.x + layout_.segment_spacing;
    }
}

void TextSegmentationLayout::updateLayout(float width, float height) {
    setSize(width, height);
    window_size_ = ImVec2(width, height);
    calculateLayout();
}

void TextSegmentationLayout::handleEvent(const SDL_Event& event) {
    // 处理SDL事件
    if (event.type == SDL_KEYDOWN) {
        handleKeyboardInput();
    }
}

void TextSegmentationLayout::handleKeyboardInput() {
    // 键盘快捷键处理
    const Uint8* state = SDL_GetKeyboardState(nullptr);

    if (state[SDL_SCANCODE_LCTRL] || state[SDL_SCANCODE_RCTRL]) {
        if (state[SDL_SCANCODE_A]) {
            selectAllSegments();
        } else if (state[SDL_SCANCODE_C]) {
            copySelectedText();
        }
    }
}

void TextSegmentationLayout::showWindow() {
    is_visible_ = true;
    DEARTS_LOG_INFO("显示分词窗口，可见性设置为: " + std::string(is_visible_ ? "可见" : "隐藏"));
}

void TextSegmentationLayout::hideWindow() {
    is_visible_ = false;
    DEARTS_LOG_INFO("隐藏分词窗口");
}

std::string TextSegmentationLayout::getText() const {
    return original_text_;
}

} // namespace DearTs::Core::Window::Widgets::Clipboard