/**
 * @file conversation_exporter.hpp
 * @brief 会话导出功能
 */

#pragma once

#include "chat/models/conversation.hpp"
#include "core/result.h"
#include <string>
#include <fstream>

namespace DearTs::Plugins::Chat {

/**
 * @brief 导出格式
 */
enum class ExportFormat {
    JSON,       // JSON 格式
    Markdown,   // Markdown 格式
    TXT,        // 纯文本格式
    HTML        // HTML 格式
};

/**
 * @brief 会话导出器
 */
class ConversationExporter {
public:
    /**
     * @brief 导出会话
     * @param conv 会话对象
     * @param format 导出格式
     * @param output_path 输出文件路径
     * @return Result 包含成功或错误信息
     */
    static DearTs::Core::Result<void, std::string> export_conversation(
        const Conversation& conv,
        ExportFormat format,
        const std::string& output_path
    );

    /**
     * @brief 导出为 JSON
     */
    static DearTs::Core::Result<void, std::string> export_json(
        const Conversation& conv,
        const std::string& output_path
    );

    /**
     * @brief 导出为 Markdown
     */
    static DearTs::Core::Result<void, std::string> export_markdown(
        const Conversation& conv,
        const std::string& output_path
    );

    /**
     * @brief 导出为纯文本
     */
    static DearTs::Core::Result<void, std::string> export_txt(
        const Conversation& conv,
        const std::string& output_path
    );

    /**
     * @brief 导出为 HTML
     */
    static DearTs::Core::Result<void, std::string> export_html(
        const Conversation& conv,
        const std::string& output_path
    );

private:
    /**
     * @brief 格式化时间戳
     */
    static std::string format_timestamp(const std::chrono::system_clock::time_point& tp);

    /**
     * @brief 格式化角色名称
     */
    static std::string format_role(MessageRole role);

    /**
     * @brief 转义 JSON 字符串
     */
    static std::string escape_json(const std::string& str);

    /**
     * @brief 转义 HTML 字符串
     */
    static std::string escape_html(const std::string& str);
};

} // namespace DearTs::Plugins::Chat
