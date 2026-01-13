/**
 * @file message.hpp
 * @brief 聊天消息数据模型
 */

#pragma once

#include <string>
#include <chrono>
#include <functional>

namespace DearTs::Plugins::Chat {

/**
 * @brief 消息角色
 */
enum class MessageRole {
    User,       // 用户消息
    Assistant,  // AI 助手消息
    System,     // 系统消息
    Other       // 其他（如来自微信等外部消息）
};

/**
 * @brief 消息状态
 */
enum class MessageStatus {
    Pending,    // 待发送
    Sending,    // 发送中
    Sent,       // 已发送
    Failed,     // 发送失败
    Received    // 已接收
};

/**
 * @brief 聊天消息
 */
struct Message {
    std::string id;                                          // 消息唯一 ID
    std::string content;                                     // 消息内容
    MessageRole role = MessageRole::User;                    // 消息角色
    MessageStatus status = MessageStatus::Pending;           // 消息状态
    std::chrono::system_clock::time_point timestamp;         // 时间戳

    // 元数据
    std::string sender_id;                                   // 发送者 ID（用于多人聊天）
    std::string reply_to_id;                                 // 回复的消息 ID
    int32_t token_count = 0;                                 // 估算的 token 数量

    // 构造函数
    Message() : timestamp(std::chrono::system_clock::now()) {}

    Message(std::string content, MessageRole role)
        : content(std::move(content))
        , role(role)
        , timestamp(std::chrono::system_clock::now()) {}

    /**
     * @brief 估算消息的 token 数量
     * @details 粗略估算：1 token ≈ 4 个字符（英文）或 2 个汉字
     */
    void estimate_tokens();

    /**
     * @brief 获取格式化的时间字符串
     */
    [[nodiscard]] std::string get_time_string() const;

    /**
     * @brief 判断是否为用户消息
     */
    [[nodiscard]] bool is_user() const { return role == MessageRole::User; }

    /**
     * @brief 判断是否为 AI 消息
     */
    [[nodiscard]] bool is_assistant() const { return role == MessageRole::Assistant; }

    /**
     * @brief 判断消息是否已发送
     */
    [[nodiscard]] bool is_sent() const {
        return status == MessageStatus::Sent || status == MessageStatus::Received;
    }
};

} // namespace DearTs::Plugins::Chat
