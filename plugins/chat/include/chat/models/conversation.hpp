/**
 * @file conversation.hpp
 * @brief 会话数据模型
 */

#pragma once

#include "message.hpp"
#include <vector>
#include <string>
#include <chrono>
#include <memory>
#include <optional>

namespace DearTs::Plugins::Chat {

/**
 * @brief 会话类型
 */
enum class ConversationType {
    Chat,       // 单聊
    Group,      // 群聊
    AI,         // AI 对话
    System      // 系统会话
};

/**
 * @brief 会话
 */
struct Conversation {
    std::string id;                                          // 会话唯一 ID
    std::string title;                                       // 会话标题
    ConversationType type = ConversationType::Chat;          // 会话类型
    std::vector<Message> messages;                           // 消息列表
    std::chrono::system_clock::time_point created_at;        // 创建时间
    std::chrono::system_clock::time_point updated_at;        // 更新时间

    // 会话设置
    std::string llm_provider = "http";                       // LLM 提供商
    std::string llm_model = "llama3.2";                      // LLM 模型
    float temperature = 0.7f;                                // 温度参数
    int max_tokens = 2048;                                   // 最大 token 数

    // 元数据
    std::string description;                                 // 会话描述
    int unread_count = 0;                                    // 未读消息数
    bool is_pinned = false;                                  // 是否置顶
    bool is_archived = false;                                // 是否归档
    std::string last_message_preview;                        // 最后一条消息预览

    // 构造函数
    Conversation()
        : created_at(std::chrono::system_clock::now())
        , updated_at(std::chrono::system_clock::now()) {}

    Conversation(std::string id, std::string title)
        : id(std::move(id))
        , title(std::move(title))
        , created_at(std::chrono::system_clock::now())
        , updated_at(std::chrono::system_clock::now()) {}

    /**
     * @brief 添加消息到会话
     */
    void add_message(const Message& message);

    /**
     * @brief 获取最后一条消息
     */
    [[nodiscard]] std::optional<std::reference_wrapper<const Message>> get_last_message() const;

    /**
     * @brief 获取最后一条消息的预览文本
     */
    [[nodiscard]] std::string get_last_message_preview() const;

    /**
     * @brief 获取会话的显示标题
     */
    [[nodiscard]] std::string get_display_title() const;

    /**
     * @brief 判断会话是否为空
     */
    [[nodiscard]] bool is_empty() const { return messages.empty(); }

    /**
     * @brief 获取消息数量
     */
    [[nodiscard]] size_t get_message_count() const { return messages.size(); }

    /**
     * @brief 估算会话的总 token 数量
     */
    [[nodiscard]] size_t estimate_total_tokens() const;

    /**
     * @brief 构建用于 LLM 的上下文
     * @param max_tokens 最大 token 数量限制
     * @return 上下文消息列表（从最旧到最新）
     */
    [[nodiscard]] std::vector<Message> build_context(size_t max_tokens = 4096) const;

    /**
     * @brief 清空所有消息
     */
    void clear_messages();

    /**
     * @brief 删除指定消息
     */
    bool delete_message(const std::string& message_id);

    /**
     * @brief 更新会话的更新时间
     */
    void touch() {
        updated_at = std::chrono::system_clock::now();
    }
};

/**
 * @brief 会话管理器
 */
class ConversationManager {
public:
    /**
     * @brief 创建新会话
     */
    [[nodiscard]] std::shared_ptr<Conversation> create_conversation(
        const std::string& title,
        ConversationType type = ConversationType::AI
    );

    /**
     * @brief 获取会话
     */
    [[nodiscard]] std::shared_ptr<Conversation> get_conversation(const std::string& id);

    /**
     * @brief 获取所有会话
     */
    [[nodiscard]] const std::vector<std::shared_ptr<Conversation>>& get_conversations() const {
        return m_conversations;
    }

    /**
     * @brief 删除会话
     */
    bool delete_conversation(const std::string& id);

    /**
     * @brief 获取当前选中的会话
     */
    [[nodiscard]] std::shared_ptr<Conversation> get_current_conversation() const {
        return m_current_conversation;
    }

    /**
     * @brief 设置当前会话
     */
    void set_current_conversation(std::shared_ptr<Conversation> conv) {
        m_current_conversation = std::move(conv);
    }

    /**
     * @brief 按 ID 查找会话
     */
    [[nodiscard]] std::shared_ptr<Conversation> find_by_id(const std::string& id) const;

    /**
     * @brief 按标题搜索会话
     */
    [[nodiscard]] std::vector<std::shared_ptr<Conversation>> search_by_title(
        const std::string& query
    ) const;

private:
    std::vector<std::shared_ptr<Conversation>> m_conversations;
    std::shared_ptr<Conversation> m_current_conversation;
    std::string m_next_id = "1";
};

} // namespace DearTs::Plugins::Chat
