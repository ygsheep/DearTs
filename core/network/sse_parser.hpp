/**
 * @file sse_parser.hpp
 * @brief Server-Sent Events (SSE) 流式解析器
 *
 * SSE 格式规范:
 * - 每行格式: "field: value\r\n"
 * - 事件由空行分隔: "\n\n"
 * - 常见字段: data, event, id, retry
 *
 * 示例:
 *   data: {"content": "hello"}\n
 *   \n
 *   event: message\n
 *   data: {"content": "world"}\n
 *   \n
 */

#pragma once

#include <string>
#include <functional>
#include <sstream>
#include <map>

namespace DearTs::Core::Network {

/**
 * @brief SSE 事件结构
 */
struct SSEEvent {
    std::string data;      // 事件数据（最常用）
    std::string event;     // 事件类型（可选）
    std::string id;        // 事件 ID（可选）
    int retry = 0;         // 重连间隔（可选，毫秒）

    bool has_data() const { return !data.empty(); }
    bool has_event() const { return !event.empty(); }
    bool has_id() const { return !id.empty(); }
    bool has_retry() const { return retry > 0; }
};

/**
 * @brief SSE 流式解析器
 *
 * 处理 SSE 格式的流式数据，支持：
 * - 缓冲不完整的数据块
 * - 按事件边界解析
 * - 自动处理各种换行格式
 */
class SSEParser {
public:
    /**
     * @brief 事件回调函数类型
     */
    using EventCallback = std::function<void(const SSEEvent&)>;

    /**
     * @brief 构造函数
     * @param callback 收到完整事件时的回调
     */
    explicit SSEParser(EventCallback callback);

    /**
     * @brief 解析数据块
     * @param chunk 收到的原始数据块
     *
     * 注意：数据块可能包含：
     * - 0 个完整事件（数据不完整，会缓冲）
     * - 1 个完整事件
     * - 多个完整事件
     * - 多个完整事件 + 不完整的结尾（会缓冲）
     */
    void parse(const std::string& chunk);

    /**
     * @brief 重置解析器状态
     */
    void reset();

    /**
     * @brief 获取当前缓冲的数据
     */
    const std::string& get_buffer() const { return m_buffer; }

    /**
     * @brief 获取已解析的事件数量
     */
    size_t get_event_count() const { return m_event_count; }

private:
    EventCallback m_callback;
    std::string m_buffer;
    size_t m_event_count = 0;

    /**
     * @brief 处理缓冲区中的完整事件
     */
    void process_events();

    /**
     * @brief 解析单个事件
     * @param event_text 事件文本（不包含分隔空行）
     * @return 解析后的事件
     */
    static SSEEvent parse_event(const std::string& event_text);
};

/**
 * @brief NDJSON 流式解析器
 *
 * 用于 Ollama 等使用 NDJSON（每行一个 JSON）的服务
 */
class NDJSONParser {
public:
    using LineCallback = std::function<void(const std::string&)>;

    explicit NDJSONParser(LineCallback callback);

    /**
     * @brief 解析数据块
     * @param chunk 收到的原始数据块
     */
    void parse(const std::string& chunk);

    /**
     * @brief 重置解析器状态
     */
    void reset();

private:
    LineCallback m_callback;
    std::string m_buffer;

    /**
     * @brief 处理缓冲区中的完整行
     */
    void process_lines();
};

} // namespace DearTs::Core::Network
