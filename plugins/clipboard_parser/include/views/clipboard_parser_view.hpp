/**
 * @file clipboard_parser_view.hpp
 * @brief 剪切板解析视图
 * @details 卡片式 UI 界面，展示剪切板内容解析结果
 */

#pragma once

#include "core/ui/view.h"
#include "core/config/config_manager.h"
#include "core/ui/icon_font.hpp"
#include <string>
#include <vector>
#include <regex>
#include <memory>

// 前向声明 cppjieba 和插件
namespace cppjieba {
    class Jieba;
}

namespace DearTs::Plugins::ClipboardParser {
    class ClipboardParserPlugin;
}

namespace DearTs {
namespace Plugins {
namespace ClipboardParser {

using DearTs::Core::ContentRegistry::UnlocalizedString;

/**
 * @brief 解析结果类型
 */
enum class ParseResultType {
    Text,           // 普通文本
    URL,            // HTTP/HTTPS URL
    FilePath,       // 文件路径
    Email,          // 邮箱地址
    Phone,          // 电话号码
    Unknown         // 未知类型
};

/**
 * @brief 解析结果项
 */
struct ParseResultItem {
    std::string content;           // 内容
    ParseResultType type;          // 类型
    std::string type_label;        // 类型标签（用于显示）
    size_t original_index;         // 在原始文本中的索引
};

/**
 * @brief 剪切板解析视图
 *
 * 提供功能：
 * - 监听剪切板变化
 * - 智能分词（中文+英文）
 * - 提取 URL、文件路径、邮箱、电话号码
 * - 卡片式 UI 展示
 * - 一键复制功能
 */
class ClipboardParserView : public Core::UI::ViewFloating {
public:
    explicit ClipboardParserView();
    ~ClipboardParserView() override;

    void draw_content() override;

    ImVec2 get_min_size() const override {
        return ImVec2(500, 400);
    }

protected:
    void on_open() override;
    void on_close() override;

private:
    /**
     * @brief 绘制工具栏
     */
    void draw_toolbar();

    /**
     * @brief 绘制解析结果（卡片式布局）
     */
    void draw_parsed_results();

    /**
     * @brief 绘制单个结果卡片
     */
    void draw_result_card(const ParseResultItem& item, int index);

    /**
     * @brief 绘制设置面板
     */
    void draw_settings();

    /**
     * @brief 从剪切板获取文本
     */
    std::string get_clipboard_text();

    /**
     * @brief 解析剪切板内容
     */
    void parse_clipboard_content();

    /**
     * @brief 使用 jieba 进行智能分词
     */
    std::vector<std::string> tokenize_text(const std::string& text);

    /**
     * @brief 使用正则表达式提取特定类型内容
     */
    std::vector<ParseResultItem> extract_special_types(const std::string& text);

    /**
     * @brief 检测文本类型
     */
    ParseResultType detect_text_type(const std::string& text);

    /**
     * @brief 获取类型标签
     */
    std::string get_type_label(ParseResultType type);

    /**
     * @brief 获取类型颜色
     */
    ImVec4 get_type_color(ParseResultType type);

    /**
     * @brief 复制到剪切板
     */
    void copy_to_clipboard(const std::string& text);

    /**
     * @brief 显示复制成功提示
     */
    void show_copy_notification(const std::string& text);

    /**
     * @brief 解析输入框中的文本
     */
    void parse_input_text();

    /**
     * @brief 绘制文本输入区域
     */
    void draw_input_area();

    /**
     * @brief 复制选中的词组到剪切板
     */
    void copy_selected_items();

    /**
     * @brief 全选/取消全选
     */
    void toggle_select_all(bool select);

    /**
     * @brief 绘制文本标签（紧凑样式，横向流动）
     */
    void draw_text_tags();

    /**
     * @brief 绘制单个文本标签
     */
    bool draw_text_tag(const ParseResultItem& item, int index);

    /**
     * @brief 处理拖动多选
     */
    void handle_drag_selection(const ParseResultItem& item, int index, bool hovered);

private:
    // 配置
    Core::Config::ConfigScope m_config;

    // 当前剪切板内容
    std::string m_current_clipboard;

    // 解析结果
    std::vector<ParseResultItem> m_parse_results;

    // 选中状态（对应每个解析结果项）
    std::vector<bool> m_selected_items;

    // 过滤设置
    bool m_filter_text = true;
    bool m_filter_url = true;
    bool m_filter_filepath = true;
    bool m_filter_email = true;
    bool m_filter_phone = true;

    // UI 状态
    bool m_show_settings = false;
    bool m_auto_parse = true;
    int m_selected_card = -1;

    // 输入文本缓冲区（用于用户粘贴内容进行解析）
    char m_input_text_buffer[4096] = "";

    // 拖动选择状态
    bool m_is_dragging = false;
    int m_drag_start_index = -1;
    std::vector<int> m_dragged_indices;  // 记录已拖动过的标签索引

    // 复制通知
    std::string m_last_copied;
    float m_copy_notification_time = 0.0f;
    static constexpr float COPY_NOTIFICATION_DURATION = 2.0f;

    // 正则表达式（静态编译，提高性能）
    static const std::regex URL_REGEX;
    static const std::regex FILE_PATH_REGEX;
    static const std::regex EMAIL_REGEX;
    static const std::regex PHONE_REGEX;

    // 快捷键状态
    std::string m_hotkey_string;
    bool m_recording_hotkey = false;
};

} // namespace ClipboardParser
} // namespace Plugins
} // namespace DearTs
