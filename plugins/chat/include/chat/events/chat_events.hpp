/**
 * @file chat_events.hpp
 * @brief 聊天相关事件定义
 */

#pragma once

#include "chat/models/message.hpp"
#include "chat/models/conversation.hpp"
#include "chat/models/ai_suggestion.hpp"

namespace DearTs::Plugins::Chat::Events {

// ============================================================================
// 消息事件
// ============================================================================

/**
 * @brief 消息已发送事件
 */
struct MessageSentEvent {
    std::string conversation_id;
    Message message;
};

/**
 * @brief 消息已接收事件
 */
struct MessageReceivedEvent {
    std::string conversation_id;
    Message message;
};

/**
 * @brief 消息状态更新事件
 */
struct MessageStatusUpdateEvent {
    std::string conversation_id;
    std::string message_id;
    MessageStatus old_status;
    MessageStatus new_status;
};

// ============================================================================
// 会话事件
// ============================================================================

/**
 * @brief 会话创建事件
 */
struct ConversationCreatedEvent {
    std::shared_ptr<Conversation> conversation;
};

/**
 * @brief 会话选中事件
 */
struct ConversationSelectedEvent {
    std::shared_ptr<Conversation> conversation;
};

/**
 * @brief 会话删除事件
 */
struct ConversationDeletedEvent {
    std::string conversation_id;
};

/**
 * @brief 会话更新事件
 */
struct ConversationUpdatedEvent {
    std::string conversation_id;
};

// ============================================================================
// AI 相关事件
// ============================================================================

/**
 * @brief AI 分析请求事件
 */
struct AIAnalysisRequestEvent {
    std::string conversation_id;
    std::vector<Message> context;              // 上下文消息
    Message current_message;                   // 当前需要分析的消息
    int suggestion_count = 3;                  // 需要生成的建议数量
};

/**
 * @brief AI 响应事件
 */
struct AIResponseEvent {
    std::string conversation_id;
    AIAnalysisResult result;
};

/**
 * @brief 建议准备就绪事件
 */
struct SuggestionsReadyEvent {
    std::string conversation_id;
    std::vector<AISuggestion> suggestions;
};

/**
 * @brief 流式响应更新事件
 */
struct StreamingUpdateEvent {
    std::string conversation_id;
    std::string partial_content;               // 部分内容
    bool is_complete = false;                  // 是否完成
};

/**
 * @brief AI 分析开始事件
 */
struct AIAnalysisStartedEvent {
    std::string conversation_id;
    std::string task_id;                       // TaskManager 任务 ID
};

/**
 * @brief AI 分析完成事件
 */
struct AIAnalysisCompletedEvent {
    std::string conversation_id;
    std::string task_id;
    bool success;
    std::string error_message;
};

// ============================================================================
// UI 相关事件
// ============================================================================

/**
 * @brief 滚动到底部请求事件
 */
struct ScrollToBottomEvent {
    std::string conversation_id;
};

/**
 * @brief 输入框焦点请求事件
 */
struct InputFocusEvent {
    std::string conversation_id;
};

/**
 * @brief 建议芯片点击事件
 */
struct SuggestionChipClickedEvent {
    std::string conversation_id;
    AISuggestion suggestion;
};

// ============================================================================
// LLM Provider 事件
// ============================================================================

/**
 * @brief LLM Provider 切换事件
 */
struct LLMProviderChangedEvent {
    std::string old_provider;
    std::string new_provider;
};

/**
 * @brief LLM 模型切换事件
 */
struct LLMModelChangedEvent {
    std::string old_model;
    std::string new_model;
};

/**
 * @brief Ollama 模型列表更新事件
 */
struct OllamaModelsUpdatedEvent {
    std::vector<std::string> models;
    std::string base_url;
};

/**
 * @brief Ollama 连接状态事件
 */
struct OllamaConnectionStatusEvent {
    bool is_connected;
    std::string base_url;
    std::string error_message;
};

// ============================================================================
// 配置相关事件
// ============================================================================

/**
 * @brief 配置更新事件
 */
struct ConfigUpdatedEvent {
    std::string config_key;
    std::string old_value;
    std::string new_value;
};

// ============================================================================
// 导出相关事件
// ============================================================================

/**
 * @brief 导出请求事件
 */
struct ExportRequestEvent {
    std::string conversation_id;
    std::string format;                       // 导出格式：json, markdown, txt
    std::string output_path;                  // 输出路径
};

/**
 * @brief 导出完成事件
 */
struct ExportCompletedEvent {
    std::string conversation_id;
    std::string format;
    std::string output_path;
    bool success;
    std::string error_message;
};

} // namespace DearTs::Plugins::Chat::Events
