/**
 * @file memory_events.hpp
 * @brief Memory Core 插件事件定义
 *
 * 定义所有与 Memory Core 相关的事件类型，包括：
 * - 事件确认机制（EventAck）
 * - 可追踪事件基类（TrackableEvent）
 * - 消息持久化事件
 * - RAG 查询事件
 * - 记忆提取事件
 * - 摘要生成事件
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <memory>

namespace DearTs::Plugins::MemoryCore::Events {

// ============ 事件同步机制 ============

/**
 * @brief 事件确认状态
 */
enum class AckStatus {
    Pending,    ///< 等待处理
    Accepted,   ///< 已接受
    Rejected,   ///< 已拒绝
    Completed,  ///< 已完成
    Failed      ///< 失败
};

/**
 * @brief 事件确认消息
 *
 * 用于确认事件已收到并处理，支持双向通信
 */
struct EventAck {
    std::string event_id;           ///< 关联的事件 ID
    AckStatus status;               ///< 确认状态
    std::string message;            ///< 附加消息
    std::chrono::system_clock::time_point timestamp;  ///< 时间戳

    /**
     * @brief 创建成功确认
     */
    static EventAck success(const std::string& event_id, const std::string& msg = "") {
        return EventAck{
            .event_id = event_id,
            .status = AckStatus::Completed,
            .message = msg,
            .timestamp = std::chrono::system_clock::now()
        };
    }

    /**
     * @brief 创建失败确认
     */
    static EventAck failure(const std::string& event_id, const std::string& msg) {
        return EventAck{
            .event_id = event_id,
            .status = AckStatus::Failed,
            .message = msg,
            .timestamp = std::chrono::system_clock::now()
        };
    }
};

// ============ 可追踪事件基类 ============

/**
 * @brief 可追踪事件基类
 *
 * 所有需要确认的事件都应继承此类
 */
struct TrackableEvent {
    std::string event_id;           ///< 事件唯一标识符
    std::string correlation_id;     ///< 关联 ID（用于事件链追踪）
    std::string source;             ///< 事件源（插件名）
    int sequence_number;            ///< 序列号（用于事件排序）
    std::chrono::system_clock::time_point timestamp;  ///< 时间戳

    /**
     * @brief 生成唯一事件 ID
     */
    static std::string generate_event_id() {
        // 简单实现：使用时间戳 + 随机数
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()
        ).count();
        return "evt_" + std::to_string(ms) + "_" + std::to_string(rand());
    }

    /**
     * @brief 初始化事件基础信息
     */
    void init_base(const std::string& src) {
        if (event_id.empty()) {
            event_id = generate_event_id();
        }
        source = src;
        timestamp = std::chrono::system_clock::now();
        sequence_number = 0;  // 将由发送方设置
    }

    /**
     * @brief 创建确认消息
     */
    EventAck create_ack(AckStatus status, const std::string& message = "") const {
        return EventAck{
            .event_id = event_id,
            .status = status,
            .message = message,
            .timestamp = std::chrono::system_clock::now()
        };
    }
};

// ============ 消息持久化事件 ============

/**
 * @brief 请求保存消息事件
 */
struct MessageSaveRequestedEvent : public TrackableEvent {
    std::string conversation_id;    ///< 会话 ID
    std::string message_uuid;       ///< 消息 UUID
    std::string role;               ///< 角色 (user/assistant/system)
    std::string content;            ///< 消息内容
    int64_t timestamp;              ///< 消息时间戳（Unix 毫秒）
    int tokens;                     ///< Token 数量
    std::string conversation_title; ///< 会话标题

    /**
     * @brief 创建确认
     */
    EventAck create_success_ack() const {
        return create_ack(AckStatus::Completed, "Message saved successfully");
    }

    EventAck create_failure_ack(const std::string& error) const {
        return create_ack(AckStatus::Failed, "Failed to save message: " + error);
    }
};

/**
 * @brief 消息保存完成事件
 */
struct MessageSavedEvent : public TrackableEvent {
    std::string conversation_id;    ///< 会话 ID
    std::string message_uuid;       ///< 消息 UUID
    int64_t database_id;            ///< 数据库中的 ID
    bool success;                   ///< 是否成功
    std::string error_message;      ///< 错误信息（如果失败）
};

// ============ RAG 查询事件 ============

/**
 * @brief RAG 查询请求事件
 */
struct RAGQueryRequestedEvent : public TrackableEvent {
    std::string query;              ///< 查询文本
    std::string conversation_id;    ///< 当前会话 ID
    int max_results;                ///< 最大结果数
    double min_similarity;          ///< 最小相似度阈值 [0-1]

    /**
     * @brief 默认参数构造
     */
    RAGQueryRequestedEvent()
        : max_results(5), min_similarity(0.5) {}
};

/**
 * @brief RAG 查询结果项
 */
struct RAGResultItem {
    std::string content;            ///< 相关内容
    std::string source_conversation_id;  ///< 来源会话 ID
    double similarity;              ///< 相似度分数 [0-1]
    std::string memory_type;        ///< 记忆类型
    int64_t timestamp;              ///< 时间戳
};

/**
 * @brief RAG 查询完成事件
 */
struct RAGQueryCompletedEvent : public TrackableEvent {
    std::string query;              ///< 原始查询
    std::vector<RAGResultItem> results;  ///< 查询结果
    bool success;                   ///< 是否成功
    std::string error_message;      ///< 错误信息
};

// ============ 记忆提取事件 ============

/**
 * @brief 记忆提取请求事件
 */
struct MemoryExtractRequestedEvent : public TrackableEvent {
    std::string conversation_id;    ///< 会话 ID
    std::vector<std::string> message_contents;  ///< 要分析的消息内容
    bool use_llm;                   ///< 是否使用 LLM 提取
};

/**
 * @brief 提取的记忆项
 */
struct ExtractedMemory {
    std::string type;               ///< 记忆类型 (preference, fact, qa)
    std::string content;            ///< 记忆内容
    double importance;              ///< 重要性分数 [0-1]
    std::string source_message_id;  ///< 来源消息 ID
};

/**
 * @brief 记忆提取完成事件
 */
struct MemoryExtractedEvent : public TrackableEvent {
    std::string conversation_id;    ///< 会话 ID
    std::vector<ExtractedMemory> memories;  ///< 提取的记忆
    bool success;                   ///< 是否成功
    std::string error_message;      ///< 错误信息
};

// ============ 摘要生成事件 ============

/**
 * @brief 会话摘要生成请求事件
 */
struct SummaryRequestedEvent : public TrackableEvent {
    std::string conversation_id;    ///< 会话 ID
    int64_t start_message_id;       ///< 起始消息 ID
    int64_t end_message_id;         ///< 结束消息 ID
    std::string summary_type;       ///< 摘要类型 (incremental, topic_based, milestone)
};

/**
 * @brief 会话摘要完成事件
 */
struct SummaryCompletedEvent : public TrackableEvent {
    std::string conversation_id;    ///< 会话 ID
    std::string summary_text;       ///< 摘要文本
    std::string summary_type;       ///< 摘要类型
    int64_t start_message_id;       ///< 覆盖的起始消息 ID
    int64_t end_message_id;         ///< 覆盖的结束消息 ID
    bool success;                   ///< 是否成功
    std::string error_message;      ///< 错误信息
};

// ============ 事件订阅配置 ============

/**
 * @brief 事件订阅配置
 *
 * 用于配置 Memory Core 插件订阅哪些 Chat 插件事件
 */
struct EventSubscriptionConfig {
    bool subscribe_message_sent;        ///< 订阅消息发送事件
    bool subscribe_conversation_selected;///< 订阅会话选择事件
    bool subscribe_ai_response;          ///< 订阅 AI 响应事件

    /**
     * @brief 默认配置：订阅所有事件
     */
    static EventSubscriptionConfig all_enabled() {
        return EventSubscriptionConfig{
            .subscribe_message_sent = true,
            .subscribe_conversation_selected = true,
            .subscribe_ai_response = true
        };
    }

    /**
     * @brief 最小配置：仅订阅消息发送
     */
    static EventSubscriptionConfig minimal() {
        return EventSubscriptionConfig{
            .subscribe_message_sent = true,
            .subscribe_conversation_selected = false,
            .subscribe_ai_response = false
        };
    }
};

} // namespace DearTs::Plugins::MemoryCore::Events
