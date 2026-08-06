/**
 * @file sse_parser.cpp
 * @brief Server-Sent Events (SSE) 流式解析器实现
 */

#include "sse_parser.hpp"
#include "liblogger/logger.h"
#include <algorithm>

namespace DearTs::Core::Network {

// ==================== SSEParser 实现 ====================

SSEParser::SSEParser(EventCallback callback)
    : m_callback(std::move(callback))
{
}

void SSEParser::parse(const std::string& chunk) {
    if (chunk.empty()) {
        return;
    }

    // 将新数据追加到缓冲区
    m_buffer += chunk;

    // 处理缓冲区中的完整事件
    process_events();
}

void SSEParser::process_events() {
    if (m_buffer.empty()) {
        return;
    }

    size_t pos = 0;
    size_t last_event_end = 0;

    while (pos < m_buffer.length()) {
        // 查找事件分隔符（双换行）
        // SSE 支持多种换行格式：\n\n, \r\r, \r\n\r\n
        size_t event_end = std::string::npos;

        // 优先查找 \n\n
        size_t nn_pos = m_buffer.find("\n\n", pos);
        if (nn_pos != std::string::npos) {
            event_end = nn_pos + 2;
        }

        // 查找 \r\n\r\n（Windows 风格）
        if (event_end == std::string::npos) {
            size_t rn_pos = m_buffer.find("\r\n\r\n", pos);
            if (rn_pos != std::string::npos) {
                event_end = rn_pos + 4;
            }
        }

        // 查找 \r\r
        if (event_end == std::string::npos) {
            size_t rr_pos = m_buffer.find("\r\r", pos);
            if (rr_pos != std::string::npos) {
                event_end = rr_pos + 2;
            }
        }

        if (event_end == std::string::npos) {
            // 没有找到完整事件，剩余数据不完整
            break;
        }

        // 提取事件文本（不包含分隔符）
        std::string event_text = m_buffer.substr(pos, event_end - pos - 2);

        // 去除事件文本末尾的 \r
        if (!event_text.empty() && event_text.back() == '\r') {
            event_text.pop_back();
        }

        // 解析并回调事件
        if (!event_text.empty()) {
            SSEEvent event = parse_event(event_text);
            if (event.has_data()) {
                m_callback(event);
                m_event_count++;
            }
        }

        last_event_end = event_end;
        pos = event_end;
    }

    // 保留未处理的数据
    if (last_event_end > 0) {
        m_buffer = m_buffer.substr(last_event_end);
    }
}

SSEEvent SSEParser::parse_event(const std::string& event_text) {
    SSEEvent event;

    std::istringstream stream(event_text);
    std::string line;

    while (std::getline(stream, line)) {
        // 去除行尾的 \r
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line.empty()) {
            continue;
        }

        // 解析 "field: value" 格式
        size_t colon_pos = line.find(':');
        if (colon_pos == std::string::npos) {
            // 无冒号，整行作为字段名，值为空
            // 忽略
            continue;
        }

        std::string field = line.substr(0, colon_pos);
        std::string value;

        if (colon_pos + 1 < line.length()) {
            // 检查冒号后是否有空格
            if (line[colon_pos + 1] == ' ') {
                value = line.substr(colon_pos + 2);
            } else {
                value = line.substr(colon_pos + 1);
            }
        }

        // 解析不同字段
        if (field == "data") {
            // 多个 data 字段需要用 \n 连接
            if (event.has_data()) {
                event.data += "\n" + value;
            } else {
                event.data = value;
            }
        } else if (field == "event") {
            event.event = value;
        } else if (field == "id") {
            event.id = value;
        } else if (field == "retry") {
            try {
                event.retry = std::stoi(value);
            } catch (...) {
                // 忽略无效的 retry 值
            }
        }
    }

    return event;
}

void SSEParser::reset() {
    m_buffer.clear();
    m_event_count = 0;
}

// ==================== NDJSONParser 实现 ====================

NDJSONParser::NDJSONParser(LineCallback callback)
    : m_callback(std::move(callback))
{
}

void NDJSONParser::parse(const std::string& chunk) {
    if (chunk.empty()) {
        return;
    }

    // 将新数据追加到缓冲区
    m_buffer += chunk;

    // 处理缓冲区中的完整行
    process_lines();
}

void NDJSONParser::process_lines() {
    if (m_buffer.empty()) {
        return;
    }

    size_t pos = 0;
    size_t last_line_end = 0;

    while (pos < m_buffer.length()) {
        // 查找换行符
        size_t line_end = m_buffer.find('\n', pos);

        if (line_end == std::string::npos) {
            // 没有找到完整行，剩余数据不完整
            break;
        }

        // 提取行内容
        std::string line = m_buffer.substr(pos, line_end - pos);

        // 去除行尾的 \r
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        // 回调非空行
        if (!line.empty() && m_callback) {
            m_callback(line);
        }

        last_line_end = line_end + 1;
        pos = line_end + 1;
    }

    // 保留未处理的数据
    if (last_line_end > 0) {
        m_buffer = m_buffer.substr(last_line_end);
    }
}

void NDJSONParser::reset() {
    m_buffer.clear();
}

} // namespace DearTs::Core::Network
