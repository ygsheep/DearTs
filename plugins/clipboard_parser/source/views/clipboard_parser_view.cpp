/**
 * @file clipboard_parser_view.cpp
 * @brief 剪切板解析视图实现
 */

#include "views/clipboard_parser_view.hpp"
#include "clipboard_parser_plugin.hpp"
#include "core/ui/icon_font.hpp"
#include "core/ui/shortcut_manager.h"
#include "core/tasks/task_manager.h"
#include "liblogger/logger.h"
#include <imgui.h>
#include <SDL3/SDL.h>
#include <algorithm>
#include <sstream>

using namespace DearTs::Core;
using DearTs::Core::ContentRegistry::UnlocalizedString;

// Windows 平台剪切板 API
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

// cppjieba 头文件
#include "cppjieba/Jieba.hpp"

namespace DearTs {
namespace Plugins {
namespace ClipboardParser {

// 正则表达式定义（支持多种格式）
const std::regex ClipboardParserView::URL_REGEX(
    R"(https?://[^\s<>"{}|\\^`\[\]]+|www\.[^\s<>"{}|\\^`\[\]]+)",
    std::regex::icase
);

const std::regex ClipboardParserView::FILE_PATH_REGEX(
    R"([A-Za-z]:\\[^\s<>:"|?*]+|(?:\./|\.\./|\~/)?[^\s<>:"|?*]+\/[^\s<>:"|?*]*)"
);

const std::regex ClipboardParserView::EMAIL_REGEX(
    R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})"
);

const std::regex ClipboardParserView::PHONE_REGEX(
    R"((?:\+?86)?1[3-9]\d{9}|(?:\+?86)?0\d{2,3}-?\d{7,8}|(?:\+?1)?[2-9]\d{2}-?[2-9]\d{2}-?\d{4})"
);

ClipboardParserView::ClipboardParserView()
    : ViewFloating(UnlocalizedString("剪切板解析器"), ICON_COPY)
    , m_config("clipboard_parser")
    , m_hotkey_string("Alt+V")
{
    // 从配置加载过滤设置
    m_filter_text = m_config.get_or<bool>("filter_text", true);
    m_filter_url = m_config.get_or<bool>("filter_url", true);
    m_filter_filepath = m_config.get_or<bool>("filter_filepath", true);
    m_filter_email = m_config.get_or<bool>("filter_email", true);
    m_filter_phone = m_config.get_or<bool>("filter_phone", true);
    m_auto_parse = m_config.get_or<bool>("auto_parse", true);

    // 加载自定义快捷键
    m_hotkey_string = m_config.get_or<std::string>("hotkey", "Alt+V");
}

ClipboardParserView::~ClipboardParserView() {
    // 保存配置
    m_config.set("filter_text", m_filter_text);
    m_config.set("filter_url", m_filter_url);
    m_config.set("filter_filepath", m_filter_filepath);
    m_config.set("filter_email", m_filter_email);
    m_config.set("filter_phone", m_filter_phone);
    m_config.set("auto_parse", m_auto_parse);
    m_config.set("hotkey", m_hotkey_string);
}

void ClipboardParserView::on_open() {
    LOG_DEBUG("ClipboardParserView: Opened");

    // jieba 在插件加载时已经在后台加载，这里不需要做任何事情
    // 如果启用了自动解析，直接解析剪切板内容
    if (m_auto_parse && ClipboardParserPlugin::is_jieba_ready()) {
        parse_clipboard_content();
    }
}

void ClipboardParserView::on_close() {
    LOG_DEBUG("ClipboardParserView: Closed");
}

void ClipboardParserView::draw_content() {
    draw_toolbar();

    ImGui::Separator();

    if (m_show_settings) {
        draw_settings();
    } else {
        // 绘制输入区域
        draw_input_area();

        ImGui::Separator();

        // 绘制解析结果
        draw_parsed_results();
    }

    // 显示复制通知
    if (m_copy_notification_time > 0.0f) {
        m_copy_notification_time -= ImGui::GetIO().DeltaTime;
        if (m_copy_notification_time <= 0.0f) {
            m_last_copied.clear();
        } else {
            // 绘制通知
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_copy_notification_time / COPY_NOTIFICATION_DURATION);
            ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), 0, ImVec2(0.5f, 0.5f));
            if (ImGui::Begin("##CopyNotification", nullptr,
                           ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                           ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                           ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav |
                           ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration)) {

                // 推送图标字体（如果已加载）
                bool icon_font_loaded = UI::IconFont::isLoaded();
                if (icon_font_loaded) {
                    ImGui::PushFont(UI::IconFont::getFont());
                }

                ImGui::TextColored(ImVec4(0.0f, 0.8f, 0.0f, 1.0f), ICON_CHECK " 已复制: %s",
                                 m_last_copied.c_str());

                if (icon_font_loaded) {
                    ImGui::PopFont();
                }
            }
            ImGui::End();
            ImGui::PopStyleVar();
        }
    }
}

void ClipboardParserView::draw_toolbar() {
    // 刷新按钮
    if (ImGui::Button(ICON_REFRESH " 刷新")) {
        parse_clipboard_content();
    }

    ImGui::SameLine();

    // 设置按钮
    if (ImGui::Button(m_show_settings ? ICON_ARROW_BACK " 返回" : ICON_SETTINGS " 设置")) {
        m_show_settings = !m_show_settings;
    }

    ImGui::SameLine();

    // 自动解析开关
    ImGui::Checkbox("自动解析", &m_auto_parse);

    ImGui::SameLine();

    // 显示统计信息
    if (!m_parse_results.empty()) {
        ImGui::TextDisabled("(共 %zu 项)", m_parse_results.size());
    }
}

void ClipboardParserView::draw_parsed_results() {
    // Jieba 在插件加载时已经在后台加载，这里不需要显示加载动画

    if (m_parse_results.empty()) {
        // 空状态
        ImVec2 size = ImGui::GetContentRegionAvail();
        ImGui::SetCursorPosY(size.y / 2 - 20);
        ImGui::SetCursorPosX(size.x / 2 - 100);

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        ImGui::Text("剪切板为空或未找到解析内容");
        ImGui::PopStyleColor();

        if (ImGui::IsItemHovered()) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        }
        if (ImGui::IsItemClicked()) {
            parse_clipboard_content();
        }
        return;
    }

    // 分离文本项和 URL/路径项
    std::vector<size_t> text_indices;
    std::vector<size_t> card_indices;  // URL、路径、邮箱、电话等

    for (size_t i = 0; i < m_parse_results.size(); ++i) {
        const auto& item = m_parse_results[i];

        // 应用过滤器
        switch (item.type) {
            case ParseResultType::Text:
                if (m_filter_text) text_indices.push_back(i);
                break;
            case ParseResultType::URL:
            case ParseResultType::FilePath:
            case ParseResultType::Email:
            case ParseResultType::Phone:
                if ((item.type == ParseResultType::URL && m_filter_url) ||
                    (item.type == ParseResultType::FilePath && m_filter_filepath) ||
                    (item.type == ParseResultType::Email && m_filter_email) ||
                    (item.type == ParseResultType::Phone && m_filter_phone)) {
                    card_indices.push_back(i);
                }
                break;
            default:
                break;
        }
    }

    // 绘制文本标签（横向流动）
    if (!text_indices.empty()) {
        draw_text_tags();
    }

    // 绘制 URL/路径等卡片
    if (!card_indices.empty()) {
        if (!text_indices.empty()) {
            ImGui::Separator();
            ImGui::Text("链接和文件:");
        }
        for (size_t idx : card_indices) {
            draw_result_card(m_parse_results[idx], static_cast<int>(idx));
        }
    }

    if (text_indices.empty() && card_indices.empty()) {
        ImGui::TextDisabled("没有符合条件的解析结果（请检查过滤器设置）");
    }
}

void ClipboardParserView::draw_result_card(const ParseResultItem& item, int index) {
    ImGui::PushID(index);

    ImVec4 type_color = get_type_color(item.type);

    // 卡片背景
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 6.0f));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, type_color);

    // 卡片内容区域
    ImVec2 size = ImGui::GetContentRegionAvail();
    size.y = ImGui::GetTextLineHeight() * 2 + 16;

    ImGui::BeginChild("##card", size, true,
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    // 类型标签
    ImVec2 cursor = ImGui::GetCursorPos();
    ImGui::PushStyleColor(ImGuiCol_Text, type_color);
    ImGui::Text("[%s]", item.type_label.c_str());
    ImGui::PopStyleColor();

    // 内容文本
    ImGui::SameLine(0, 8.0f);
    ImGui::SetCursorPosY(cursor.y);

    // 截断过长的文本
    std::string display_text = item.content;
    const float max_text_width = ImGui::GetContentRegionAvail().x - 40.0f;
    ImVec2 text_size = ImGui::CalcTextSize(display_text.c_str());

    if (text_size.x > max_text_width) {
        size_t max_chars = static_cast<size_t>(display_text.size() * (max_text_width / text_size.x));
        if (max_chars > 3) {
            display_text = display_text.substr(0, max_chars - 3) + "...";
        }
    }

    ImGui::Text("%s", display_text.c_str());

    // 单项复制按钮（右侧）
    float button_x = ImGui::GetContentRegionAvail().x - 35;
    ImGui::SetCursorPosX(button_x);
    ImGui::SetCursorPosY(cursor.y);

    std::string copy_id = "##copy" + std::to_string(index);

    // 使用默认字体（已包含图标），所以不需要额外推送字体
    if (ImGui::Button(ICON_COPY, ImVec2(30, 0))) {
        copy_to_clipboard(item.content);
    }

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("复制此项");
    }

    ImGui::EndChild();

    // Pop style colors: 2 个颜色 + 2 个样式变量
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
    ImGui::PopID();
}

void ClipboardParserView::draw_text_tags() {
    ImGui::Text("分词结果:");

    // 检测鼠标左键释放，结束拖动
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        m_is_dragging = false;
        m_drag_start_index = -1;
        m_dragged_indices.clear();
    }

    // 收集所有文本类型的索引
    std::vector<size_t> text_indices;
    for (size_t i = 0; i < m_parse_results.size(); ++i) {
        if (m_parse_results[i].type == ParseResultType::Text && m_filter_text) {
            text_indices.push_back(i);
        }
    }

    if (text_indices.empty()) {
        ImGui::TextDisabled("无文本分词结果");
        return;
    }

    ImGui::Spacing();

    // 横向流动布局
    ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
    ImVec2 window_size = ImGui::GetContentRegionAvail();
    float current_x = 0.0f;
    float line_height = ImGui::GetTextLineHeight() * 1.8f;
    float spacing = 6.0f;

    for (size_t idx : text_indices) {
        const auto& item = m_parse_results[idx];
        ImVec2 tag_size = ImGui::CalcTextSize(item.content.c_str());
        tag_size.x += 16.0f;  // 左右内边距
        tag_size.y = line_height;

        // 检查是否需要换行
        if (current_x + tag_size.x > window_size.x && current_x > 0) {
            current_x = 0;
            cursor_pos.y += tag_size.y + spacing;
        }

        // 保存当前光标位置
        ImVec2 saved_pos = ImGui::GetCursorPos();

        // 设置标签位置
        ImGui::SetCursorPos(ImVec2(current_x, cursor_pos.y - ImGui::GetCursorScreenPos().y + ImGui::GetCursorPos().y));

        // 绘制标签
        draw_text_tag(item, static_cast<int>(idx));

        // 更新位置
        current_x += tag_size.x + spacing;

        // 恢复光标位置
        ImGui::SetCursorPos(saved_pos);
    }

    // 添加间距
    cursor_pos.y += line_height + spacing;
    ImGui::Dummy(ImVec2(0, line_height + spacing));
}

void ClipboardParserView::handle_drag_selection(const ParseResultItem& item, int index, bool hovered) {
    // 如果没有在拖动，不做处理
    if (!m_is_dragging) {
        return;
    }

    // 检查是否已经拖动过这个标签
    bool already_dragged = false;
    for (int dragged_idx : m_dragged_indices) {
        if (dragged_idx == index) {
            already_dragged = true;
            break;
        }
    }

    // 如果正在拖动且鼠标悬停在标签上，且之前没有拖动过
    if (hovered && !already_dragged) {
        // 记录已拖动
        m_dragged_indices.push_back(index);

        // 切换选中状态
        if (static_cast<size_t>(index) >= m_selected_items.size()) {
            m_selected_items.resize(index + 1, false);
        }

        // 根据起始标签的选中状态决定行为
        bool start_selected = false;
        if (m_drag_start_index >= 0 && static_cast<size_t>(m_drag_start_index) < m_selected_items.size()) {
            start_selected = m_selected_items[m_drag_start_index];
        }

        // 如果起始标签被选中，则取消选中；否则选中
        m_selected_items[index] = !start_selected;
    }
}

bool ClipboardParserView::draw_text_tag(const ParseResultItem& item, int index) {
    ImGui::PushID(index);

    // 获取选中状态
    bool is_selected = (static_cast<size_t>(index) < m_selected_items.size() && m_selected_items[index]);

    // 计算标签大小
    ImVec2 label_size = ImGui::CalcTextSize(item.content.c_str());
    ImVec2 tag_size(label_size.x + 16.0f, ImGui::GetTextLineHeight() * 1.5f);

    // 检测点击和悬停
    ImVec2 cursor = ImGui::GetCursorScreenPos();
    ImVec2 tag_max(cursor.x + tag_size.x, cursor.y + tag_size.y);

    bool hovered = ImGui::IsMouseHoveringRect(cursor, tag_max);

    // 处理拖动多选
    handle_drag_selection(item, index, hovered);

    // 处理点击（如果不是拖动模式）
    bool clicked = hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !m_is_dragging;

    if (clicked) {
        // 开始拖动
        m_is_dragging = true;
        m_drag_start_index = index;
        m_dragged_indices.clear();
        m_dragged_indices.push_back(index);

        // 切换选中状态
        if (static_cast<size_t>(index) >= m_selected_items.size()) {
            m_selected_items.resize(index + 1, false);
        }
        m_selected_items[index] = !is_selected;
        is_selected = !is_selected;
    }

    // 绘制标签背景
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    if (is_selected) {
        // 选中状态：高亮背景 + 边框
        ImVec4 highlight_color = ImVec4(0.2f, 0.5f, 0.8f, 0.3f);
        draw_list->AddRectFilled(cursor, tag_max, ImGui::GetColorU32(highlight_color), 4.0f);
        draw_list->AddRect(cursor, tag_max, IM_COL32(100, 180, 255, 255), 4.0f, 0, 1.5f);
    } else {
        // 默认状态：浅色边框
        draw_list->AddRect(cursor, tag_max, IM_COL32(80, 80, 80, 80), 4.0f, 0, 1.0f);
    }

    if (hovered && !is_selected) {
        // 悬停状态：更亮的边框
        draw_list->AddRect(cursor, tag_max, IM_COL32(120, 120, 120, 120), 4.0f, 0, 1.0f);
    }

    // 绘制文本（居中）
    ImVec2 text_pos(
        cursor.x + (tag_size.x - label_size.x) * 0.5f,
        cursor.y + (tag_size.y - label_size.y) * 0.5f
    );
    draw_list->AddText(text_pos, IM_COL32(220, 220, 220, 255), item.content.c_str());

    // 处理复制按钮（右键）
    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        copy_to_clipboard(item.content);
    }

    if (hovered) {
        ImGui::SetTooltip("左键拖动: 多选 | 右键: 复制此项");
    }

    // 推进光标
    ImGui::Dummy(tag_size);

    ImGui::PopID();

    return clicked;
}

void ClipboardParserView::draw_settings() {
    ImGui::Text("过滤器设置");
    ImGui::Separator();

    ImGui::Checkbox("显示普通文本", &m_filter_text);
    ImGui::Checkbox("显示 URL", &m_filter_url);
    ImGui::Checkbox("显示文件路径", &m_filter_filepath);
    ImGui::Checkbox("显示邮箱地址", &m_filter_email);
    ImGui::Checkbox("显示电话号码", &m_filter_phone);

    ImGui::Separator();
    ImGui::Text("快捷键设置");

    ImGui::Text("当前快捷键: %s", m_hotkey_string.c_str());

    if (ImGui::Button("录制新快捷键")) {
        m_recording_hotkey = true;
    }

    if (m_recording_hotkey) {
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f),
                          "按下按键组合...");

        // 检测按键组合
        auto& io = ImGui::GetIO();
        if (io.KeyCtrl || io.KeyShift || io.KeyAlt) {
            for (int key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; ++key) {
                if (ImGui::IsKeyPressed(static_cast<ImGuiKey>(key))) {
                    // 构建快捷键字符串
                    std::string new_hotkey;
                    if (io.KeyCtrl) new_hotkey += "Ctrl+";
                    if (io.KeyShift) new_hotkey += "Shift+";
                    if (io.KeyAlt) new_hotkey += "Alt+";

                    // 添加按键名称
                    if (key >= ImGuiKey_A && key <= ImGuiKey_Z) {
                        new_hotkey += ('A' + (key - ImGuiKey_A));
                    } else if (key >= ImGuiKey_0 && key <= ImGuiKey_9) {
                        new_hotkey += ('0' + (key - ImGuiKey_0));
                    } else if (key == ImGuiKey_V) {
                        new_hotkey += "V";
                    }

                    if (!new_hotkey.empty()) {
                        m_hotkey_string = new_hotkey;
                        m_config.set("hotkey", m_hotkey_string);
                        LOG_INFO("ClipboardParser: Hotkey changed to: {}", m_hotkey_string);
                    }

                    m_recording_hotkey = false;
                    break;
                }
            }
        }

        // 按 ESC 取消
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            m_recording_hotkey = false;
        }
    }

    ImGui::Separator();
    ImGui::TextDisabled("提示: 也可以在 config.json 中直接修改 'clipboard_parser.hotkey'");
}

std::string ClipboardParserView::get_clipboard_text() {
    std::string result;

#ifdef _WIN32
    if (!OpenClipboard(nullptr)) {
        LOG_WARN("ClipboardParserView: Failed to open clipboard");
        return result;
    }

    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (hData != nullptr) {
        const wchar_t* pwstr = static_cast<const wchar_t*>(GlobalLock(hData));
        if (pwstr != nullptr) {
            // 转换为 UTF-8
            int size = WideCharToMultiByte(CP_UTF8, 0, pwstr, -1, nullptr, 0, nullptr, nullptr);
            if (size > 0) {
                result.resize(size - 1);
                WideCharToMultiByte(CP_UTF8, 0, pwstr, -1, &result[0], size, nullptr, nullptr);
            }
            GlobalUnlock(hData);
        }
    }

    CloseClipboard();
#else
    // SDL3 跨平台方案
    if (SDL_HasClipboardText()) {
        char* text = SDL_GetClipboardText();
        if (text) {
            result = text;
            SDL_free(text);
        }
    }
#endif

    return result;
}

void ClipboardParserView::parse_clipboard_content() {
    m_current_clipboard = get_clipboard_text();

    if (m_current_clipboard.empty()) {
        LOG_DEBUG("ClipboardParserView: Clipboard is empty");
        m_parse_results.clear();
        return;
    }

    LOG_DEBUG("ClipboardParserView: Parsing clipboard content (length: {})", m_current_clipboard.size());

    // 使用正则表达式提取特殊类型
    m_parse_results = extract_special_types(m_current_clipboard);

    // 如果没有找到特殊类型，使用 jieba 进行智能分词
    if (m_parse_results.empty() && ClipboardParserPlugin::is_jieba_ready()) {
        std::vector<std::string> words = tokenize_text(m_current_clipboard);

        for (const auto& word : words) {
            if (word.empty()) continue;

            ParseResultItem item;
            item.content = word;
            item.type = detect_text_type(word);
            item.type_label = get_type_label(item.type);
            item.original_index = 0;

            // 只添加有意义的词（过滤单字符和标点）
            if (word.size() > 1 || (word.size() == 1 && isalnum(word[0]))) {
                m_parse_results.push_back(item);
            }
        }
    }

    LOG_INFO("ClipboardParserView: Parsed {} items from clipboard", m_parse_results.size());
}

std::vector<std::string> ClipboardParserView::tokenize_text(const std::string& text) {
    std::vector<std::string> words;

    // 获取插件的 jieba 实例
    auto* jieba = ClipboardParserPlugin::get_jieba();
    if (!jieba) {
        // 降级方案：按空格和标点分割
        std::stringstream ss(text);
        std::string token;
        while (std::getline(ss, token, ' ')) {
            if (!token.empty()) {
                words.push_back(token);
            }
        }
        return words;
    }

    try {
        // 使用 jieba 分词（搜索模式，更精确）
        jieba->CutForSearch(text, words, true);
    } catch (const std::exception& e) {
        LOG_ERROR("ClipboardParserView: Jieba tokenization failed: {}", e.what());
        // 降级方案
        std::stringstream ss(text);
        std::string token;
        while (std::getline(ss, token, ' ')) {
            if (!token.empty()) {
                words.push_back(token);
            }
        }
    }

    return words;
}

std::vector<ParseResultItem> ClipboardParserView::extract_special_types(const std::string& text) {
    std::vector<ParseResultItem> results;

    std::istringstream stream(text);
    std::string line;

    while (std::getline(stream, line)) {
        if (line.empty()) continue;

        // 提取 URL
        std::sregex_token_iterator url_begin(line.begin(), line.end(), URL_REGEX), url_end;
        for (auto it = url_begin; it != url_end; ++it) {
            ParseResultItem item;
            item.content = it->str();
            item.type = ParseResultType::URL;
            item.type_label = get_type_label(item.type);
            results.push_back(item);
        }

        // 提取文件路径
        std::sregex_token_iterator file_begin(line.begin(), line.end(), FILE_PATH_REGEX), file_end;
        for (auto it = file_begin; it != file_end; ++it) {
            std::string path = it->str();
            // 排除 URL（避免重复）
            if (path.find("http") != 0 && path.find("www") != 0) {
                ParseResultItem item;
                item.content = path;
                item.type = ParseResultType::FilePath;
                item.type_label = get_type_label(item.type);
                results.push_back(item);
            }
        }

        // 提取邮箱
        std::sregex_token_iterator email_begin(line.begin(), line.end(), EMAIL_REGEX), email_end;
        for (auto it = email_begin; it != email_end; ++it) {
            ParseResultItem item;
            item.content = it->str();
            item.type = ParseResultType::Email;
            item.type_label = get_type_label(item.type);
            results.push_back(item);
        }

        // 提取电话号码
        std::sregex_token_iterator phone_begin(line.begin(), line.end(), PHONE_REGEX), phone_end;
        for (auto it = phone_begin; it != phone_end; ++it) {
            ParseResultItem item;
            item.content = it->str();
            item.type = ParseResultType::Phone;
            item.type_label = get_type_label(item.type);
            results.push_back(item);
        }
    }

    return results;
}

ParseResultType ClipboardParserView::detect_text_type(const std::string& text) {
    if (std::regex_search(text, URL_REGEX)) {
        return ParseResultType::URL;
    }
    if (std::regex_search(text, FILE_PATH_REGEX)) {
        return ParseResultType::FilePath;
    }
    if (std::regex_search(text, EMAIL_REGEX)) {
        return ParseResultType::Email;
    }
    if (std::regex_search(text, PHONE_REGEX)) {
        return ParseResultType::Phone;
    }
    return ParseResultType::Text;
}

std::string ClipboardParserView::get_type_label(ParseResultType type) {
    switch (type) {
        case ParseResultType::URL:
            return "URL";
        case ParseResultType::FilePath:
            return "路径";
        case ParseResultType::Email:
            return "邮箱";
        case ParseResultType::Phone:
            return "电话";
        case ParseResultType::Text:
            return "文本";
        default:
            return "未知";
    }
}

ImVec4 ClipboardParserView::get_type_color(ParseResultType type) {
    switch (type) {
        case ParseResultType::URL:
            return ImVec4(0.2f, 0.6f, 1.0f, 1.0f);  // 蓝色
        case ParseResultType::FilePath:
            return ImVec4(1.0f, 0.6f, 0.2f, 1.0f);  // 橙色
        case ParseResultType::Email:
            return ImVec4(0.2f, 0.8f, 0.4f, 1.0f);  // 绿色
        case ParseResultType::Phone:
            return ImVec4(0.8f, 0.2f, 0.8f, 1.0f);  // 紫色
        case ParseResultType::Text:
            return ImVec4(0.7f, 0.7f, 0.7f, 1.0f);  // 灰色
        default:
            return ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
    }
}

void ClipboardParserView::copy_to_clipboard(const std::string& text) {
#ifdef _WIN32
    if (!OpenClipboard(nullptr)) {
        LOG_WARN("ClipboardParserView: Failed to open clipboard for copying");
        return;
    }

    EmptyClipboard();

    // 转换为宽字符
    int size = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    HGLOBAL hglb = GlobalAlloc(GMEM_MOVEABLE, size * sizeof(wchar_t));
    if (hglb) {
        wchar_t* lptstr = static_cast<wchar_t*>(GlobalLock(hglb));
        if (lptstr) {
            MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, lptstr, size);
            GlobalUnlock(hglb);
            SetClipboardData(CF_UNICODETEXT, hglb);
        }
    }

    CloseClipboard();
#else
    SDL_SetClipboardText(text.c_str());
#endif

    LOG_DEBUG("ClipboardParserView: Copied to clipboard: {}", text);
    show_copy_notification(text);
}

void ClipboardParserView::show_copy_notification(const std::string& text) {
    m_last_copied = text;
    if (m_last_copied.size() > 30) {
        m_last_copied = m_last_copied.substr(0, 30) + "...";
    }
    m_copy_notification_time = COPY_NOTIFICATION_DURATION;
}

void ClipboardParserView::draw_input_area() {
    ImGui::Text("粘贴或输入要解析的文本:");

    // 多行文本输入框
    ImVec2 size = ImGui::GetContentRegionAvail();
    size.y = 100.0f;  // 固定高度

    if (ImGui::InputTextMultiline("##input_text", m_input_text_buffer,
                                   sizeof(m_input_text_buffer), size,
                                   ImGuiInputTextFlags_AllowTabInput)) {
        // 文本变化时不需要立即解析
    }

    ImGui::Spacing();

    // 第一行按钮：解析、粘贴、清空
    if (ImGui::Button(ICON_SEARCH " 解析文本")) {
        parse_input_text();
    }

    ImGui::SameLine();

    if (ImGui::Button(ICON_COPY " 从剪切板粘贴")) {
        std::string clipboard_text = get_clipboard_text();
        if (!clipboard_text.empty()) {
            // 复制到输入缓冲区
            size_t copy_len = std::min(clipboard_text.size(), sizeof(m_input_text_buffer) - 1);
            memcpy(m_input_text_buffer, clipboard_text.c_str(), copy_len);
            m_input_text_buffer[copy_len] = '\0';
            LOG_INFO("ClipboardParserView: Pasted {} characters from clipboard", copy_len);
        } else {
            LOG_WARN("ClipboardParserView: Clipboard is empty");
        }
    }

    ImGui::SameLine();

    if (ImGui::Button(ICON_DELETE " 清空")) {
        m_input_text_buffer[0] = '\0';
        m_parse_results.clear();
        m_selected_items.clear();
    }

    ImGui::Spacing();

    // 第二行按钮：复制选中项
    // 统计选中数量
    size_t selected_count = 0;
    for (bool selected : m_selected_items) {
        if (selected) selected_count++;
    }

    std::string copy_label = ICON_COPY " 复制选中项";
    if (selected_count > 0) {
        copy_label += " (";
        copy_label += std::to_string(selected_count);
        copy_label += ")";
    }

    if (ImGui::Button(copy_label.c_str())) {
        copy_selected_items();
    }

    ImGui::SameLine();

    ImGui::TextDisabled("提示: 点击词组进行选择，按住Ctrl可多选");

    ImGui::Spacing();
    ImGui::Separator();
}

void ClipboardParserView::parse_input_text() {
    std::string input_text(m_input_text_buffer);

    if (input_text.empty()) {
        LOG_DEBUG("ClipboardParserView: Input text is empty");
        m_parse_results.clear();
        m_selected_items.clear();
        return;
    }

    LOG_DEBUG("ClipboardParserView: Parsing input text (length: {})", input_text.size());

    // 清空之前的结果和选中状态
    m_parse_results.clear();
    m_selected_items.clear();

    // 使用正则表达式提取特殊类型
    m_parse_results = extract_special_types(input_text);

    // 如果没有找到特殊类型，使用 jieba 进行智能分词
    if (m_parse_results.empty() && ClipboardParserPlugin::is_jieba_ready()) {
        std::vector<std::string> words = tokenize_text(input_text);

        for (const auto& word : words) {
            if (word.empty()) continue;

            ParseResultItem item;
            item.content = word;
            item.type = detect_text_type(word);
            item.type_label = get_type_label(item.type);
            item.original_index = 0;

            // 只添加有意义的词（过滤单字符和标点）
            if (word.size() > 1 || (word.size() == 1 && isalnum(word[0]))) {
                m_parse_results.push_back(item);
            }
        }
    }

    // 初始化选中状态（全部不选中）
    m_selected_items.resize(m_parse_results.size(), false);

    LOG_INFO("ClipboardParserView: Parsed {} items from input text", m_parse_results.size());
}

void ClipboardParserView::copy_selected_items() {
    if (m_parse_results.empty()) {
        LOG_WARN("ClipboardParserView: No items to copy");
        return;
    }

    // 拼接选中的词组（无分隔符）
    std::string result;
    size_t count = 0;

    for (size_t i = 0; i < m_parse_results.size(); ++i) {
        if (i < m_selected_items.size() && m_selected_items[i]) {
            result += m_parse_results[i].content;
            count++;
        }
    }

    if (result.empty()) {
        LOG_WARN("ClipboardParserView: No items selected");
        return;
    }

    // 复制到剪切板
    copy_to_clipboard(result);

    LOG_INFO("ClipboardParserView: Copied {} selected items", count);
}

void ClipboardParserView::toggle_select_all(bool select) {
    m_selected_items.resize(m_parse_results.size(), select);

    for (size_t i = 0; i < m_selected_items.size(); ++i) {
        m_selected_items[i] = select;
    }

    LOG_DEBUG("ClipboardParserView: {} all items", select ? "Selected" : "Deselected");
}

} // namespace ClipboardParser
} // namespace Plugins
} // namespace DearTs
