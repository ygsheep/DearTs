/**
 * @file conversation_exporter.cpp
 * @brief 会话导出功能实现
 */

#include "chat/conversation_exporter.hpp"
#include "liblogger/logger.h"
#include <nlohmann/json.hpp>
#include <fmt/format.h>
#include <fstream>
#include <iomanip>
#include <ctime>

namespace DearTs::Plugins::Chat {

using json = nlohmann::json;

Result<void, std::string> ConversationExporter::export_conversation(
    const Conversation& conv,
    ExportFormat format,
    const std::string& output_path
) {
    try {
        switch (format) {
            case ExportFormat::JSON:
                return export_json(conv, output_path);
            case ExportFormat::Markdown:
                return export_markdown(conv, output_path);
            case ExportFormat::TXT:
                return export_txt(conv, output_path);
            case ExportFormat::HTML:
                return export_html(conv, output_path);
            default:
                return Result<void, std::string>::err("Unsupported export format");
        }
    } catch (const std::exception& e) {
        return Result<void, std::string>::err(fmt::format("Export failed: {}", e.what()));
    }
}

Result<void, std::string> ConversationExporter::export_json(
    const Conversation& conv,
    const std::string& output_path
) {
    try {
        json j;

        // 基本信息
        j["id"] = conv.id;
        j["title"] = conv.title;
        j["type"] = static_cast<int>(conv.type);
        j["created_at"] = format_timestamp(conv.created_at);
        j["updated_at"] = format_timestamp(conv.updated_at);

        // 消息列表
        j["messages"] = json::array();
        for (const auto& msg : conv.messages) {
            json msg_json;
            msg_json["id"] = msg.id;
            msg_json["content"] = msg.content;
            msg_json["role"] = static_cast<int>(msg.role);
            msg_json["status"] = static_cast<int>(msg.status);
            msg_json["timestamp"] = format_timestamp(msg.timestamp);
            msg_json["sender_id"] = msg.sender_id;
            msg_json["token_count"] = msg.token_count;

            j["messages"].push_back(msg_json);
        }

        // 写入文件
        std::ofstream file(output_path);
        if (!file) {
            return Result<void, std::string>::err("Failed to open file for writing");
        }

        file << j.dump(2);
        file.close();

        LOG_INFO("Exported conversation {} to JSON: {}", conv.id, output_path);
        return Result<void, std::string>::ok();

    } catch (const json::exception& e) {
        return Result<void, std::string>::err(fmt::format("JSON error: {}", e.what()));
    }
}

Result<void, std::string> ConversationExporter::export_markdown(
    const Conversation& conv,
    const std::string& output_path
) {
    try {
        std::ofstream file(output_path);
        if (!file) {
            return Result<void, std::string>::err("Failed to open file for writing");
        }

        // 标题
        file << "# " << conv.title << "\n\n";
        file << "**会话 ID:** " << conv.id << "\n\n";
        file << "**创建时间:** " << format_timestamp(conv.created_at) << "\n\n";
        file << "---\n\n";

        // 消息列表
        for (const auto& msg : conv.messages) {
            // 角色名称
            file << "## " << format_role(msg.role) << "\n\n";

            // 时间戳
            file << "*" << format_timestamp(msg.timestamp) << "*\n\n";

            // 内容（如果是用户消息，加粗）
            if (msg.role == MessageRole::User) {
                file << "> **" << msg.content << "**\n\n";
            } else {
                file << msg.content << "\n\n";
            }

            file << "---\n\n";
        }

        file.close();

        LOG_INFO("Exported conversation {} to Markdown: {}", conv.id, output_path);
        return Result<void, std::string>::ok();

    } catch (const std::exception& e) {
        return Result<void, std::string>::err(fmt::format("Markdown export error: {}", e.what()));
    }
}

Result<void, std::string> ConversationExporter::export_txt(
    const Conversation& conv,
    const std::string& output_path
) {
    try {
        std::ofstream file(output_path);
        if (!file) {
            return Result<void, std::string>::err("Failed to open file for writing");
        }

        // 标题
        file << "会话: " << conv.title << "\n";
        file << "ID: " << conv.id << "\n";
        file << "创建时间: " << format_timestamp(conv.created_at) << "\n";
        file << std::string(60, '=') << "\n\n";

        // 消息列表
        for (const auto& msg : conv.messages) {
            file << "[" << format_role(msg.role) << "] ";
            file << format_timestamp(msg.timestamp) << "\n";
            file << msg.content << "\n";
            file << "\n";
        }

        file.close();

        LOG_INFO("Exported conversation {} to TXT: {}", conv.id, output_path);
        return Result<void, std::string>::ok();

    } catch (const std::exception& e) {
        return Result<void, std::string>::err(fmt::format("TXT export error: {}", e.what()));
    }
}

Result<void, std::string> ConversationExporter::export_html(
    const Conversation& conv,
    const std::string& output_path
) {
    try {
        std::ofstream file(output_path);
        if (!file) {
            return Result<void, std::string>::err("Failed to open file for writing");
        }

        // HTML 头部
        file << "<!DOCTYPE html>\n";
        file << "<html lang=\"zh-CN\">\n";
        file << "<head>\n";
        file << "    <meta charset=\"UTF-8\">\n";
        file << "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
        file << "    <title>" << escape_html(conv.title) << "</title>\n";
        file << "    <style>\n";
        file << "        body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; max-width: 800px; margin: 0 auto; padding: 20px; }\n";
        file << "        .header { border-bottom: 2px solid #e0e0e0; padding-bottom: 20px; margin-bottom: 20px; }\n";
        file << "        .message { margin-bottom: 20px; padding: 15px; border-radius: 8px; }\n";
        file << "        .message.user { background-color: #e3f2fd; margin-left: 50px; }\n";
        file << "        .message.assistant { background-color: #f5f5f5; margin-right: 50px; }\n";
        file << "        .message.system { background-color: #fff3cd; text-align: center; }\n";
        file << "        .message-header { font-weight: bold; margin-bottom: 8px; color: #666; }\n";
        file << "        .message-content { white-space: pre-wrap; }\n";
        file << "        .timestamp { font-size: 0.85em; color: #999; }\n";
        file << "    </style>\n";
        file << "</head>\n";
        file << "<body>\n";

        // 头部
        file << "    <div class=\"header\">\n";
        file << "        <h1>" << escape_html(conv.title) << "</h1>\n";
        file << "        <p><strong>会话 ID:</strong> " << conv.id << "</p>\n";
        file << "        <p><strong>创建时间:</strong> " << format_timestamp(conv.created_at) << "</p>\n";
        file << "    </div>\n";

        // 消息列表
        for (const auto& msg : conv.messages) {
            std::string css_class;
            switch (msg.role) {
                case MessageRole::User:
                    css_class = "user";
                    break;
                case MessageRole::Assistant:
                    css_class = "assistant";
                    break;
                case MessageRole::System:
                    css_class = "system";
                    break;
                default:
                    css_class = "";
            }

            file << "    <div class=\"message " << css_class << "\">\n";
            file << "        <div class=\"message-header\">" << format_role(msg.role) << "</div>\n";
            file << "        <div class=\"timestamp\">" << format_timestamp(msg.timestamp) << "</div>\n";
            file << "        <div class=\"message-content\">" << escape_html(msg.content) << "</div>\n";
            file << "    </div>\n";
        }

        // HTML 尾部
        file << "</body>\n";
        file << "</html>\n";

        file.close();

        LOG_INFO("Exported conversation {} to HTML: {}", conv.id, output_path);
        return Result<void, std::string>::ok();

    } catch (const std::exception& e) {
        return Result<void, std::string>::err(fmt::format("HTML export error: {}", e.what()));
    }
}

std::string ConversationExporter::format_timestamp(const std::chrono::system_clock::time_point& tp) {
    const auto time_t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &time_t);
#else
    localtime_r(&time_t, &tm);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string ConversationExporter::format_role(MessageRole role) {
    switch (role) {
        case MessageRole::User:
            return "用户";
        case MessageRole::Assistant:
            return "AI 助手";
        case MessageRole::System:
            return "系统";
        case MessageRole::Other:
            return "其他";
        default:
            return "未知";
    }
}

std::string ConversationExporter::escape_json(const std::string& str) {
    std::string result;
    for (char c : str) {
        switch (c) {
            case '"':  result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    result += fmt::format("\\u{:04x}", static_cast<unsigned char>(c));
                } else {
                    result += c;
                }
        }
    }
    return result;
}

std::string ConversationExporter::escape_html(const std::string& str) {
    std::string result;
    for (char c : str) {
        switch (c) {
            case '&':  result += "&amp;"; break;
            case '<':  result += "&lt;"; break;
            case '>':  result += "&gt;"; break;
            case '"':  result += "&quot;"; break;
            case '\'': result += "&#39;"; break;
            default:   result += c; break;
        }
    }
    return result;
}

} // namespace DearTs::Plugins::Chat
