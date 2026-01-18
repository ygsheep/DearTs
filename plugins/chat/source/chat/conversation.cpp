/**
 * @file conversation.cpp
 * @brief 会话数据模型实现
 */

#include "chat/models/conversation.hpp"
#include "chat/events/chat_events.hpp"
#include "memory_core/persistence/database.hpp"
#include "core/event/event_bus.h"
#include "liblogger/logger.h"
#include <format>
#include <algorithm>
#include <random>

namespace DearTs::Plugins::Chat {

void Conversation::add_message(const Message& message) {
    messages.push_back(message);
    touch();

    // 更新最后消息预览
    if (!message.content.empty()) {
        const std::string& content = message.content;
        last_message_preview = content.substr(0, std::min(size_t(50), content.length()));
        if (content.length() > 50) {
            last_message_preview += "...";
        }
    }
}

std::optional<std::reference_wrapper<const Message>> Conversation::get_last_message() const {
    if (messages.empty()) {
        return std::nullopt;
    }
    return messages.back();
}

std::string Conversation::get_last_message_preview() const {
    if (last_message_preview.empty() && !messages.empty()) {
        const std::string& content = messages.back().content;
        std::string preview = content.substr(0, std::min(size_t(50), content.length()));
        if (content.length() > 50) {
            preview += "...";
        }
        return preview;
    }
    return last_message_preview;
}

std::string Conversation::get_display_title() const {
    if (!title.empty()) {
        return title;
    }

    // 如果没有标题，使用最后一条消息的前 20 个字符
    if (!messages.empty()) {
        const std::string& content = messages.back().content;
        std::string short_title = content.substr(0, std::min(size_t(20), content.length()));
        if (content.length() > 20) {
            short_title += "...";
        }
        return short_title;
    }

    return "新对话";
}

size_t Conversation::estimate_total_tokens() const {
    size_t total = 0;
    for (const auto& msg : messages) {
        total += msg.token_count;
    }
    return total;
}

std::vector<Message> Conversation::build_context(size_t max_tokens) const {
    std::vector<Message> context;
    size_t total_tokens = 0;

    // 从最新消息开始，倒序添加
    for (auto it = messages.rbegin(); it != messages.rend(); ++it) {
        const size_t tokens = it->token_count > 0 ? it->token_count : (it->content.length() / 4 + 1);

        if (total_tokens + tokens > max_tokens) {
            break;
        }

        context.insert(context.begin(), *it);
        total_tokens += tokens;
    }

    return context;
}

void Conversation::clear_messages() {
    messages.clear();
    last_message_preview.clear();
    touch();
    LOG_INFO("Cleared all messages in conversation {}", id);
}

bool Conversation::delete_message(const std::string& message_id) {
    auto it = std::remove_if(messages.begin(), messages.end(),
        [&message_id](const Message& msg) {
            return msg.id == message_id;
        });

    if (it != messages.end()) {
        messages.erase(it, messages.end());
        touch();
        LOG_INFO("Deleted message {} from conversation {}", message_id, id);
        return true;
    }

    return false;
}

// ConversationManager 实现
std::shared_ptr<Conversation> ConversationManager::create_conversation(
    const std::string& title,
    ConversationType type
) {
    auto conv = std::make_shared<Conversation>();
    conv->id = m_next_id;
    conv->title = title;
    conv->type = type;

    // 生成下一个 ID
    try {
        int id_num = std::stoi(m_next_id);
        m_next_id = std::to_string(id_num + 1);
    } catch (...) {
        m_next_id = "1";
    }

    m_conversations.push_back(conv);

    // ✅ 发布会话创建事件，通知其他插件（如 Memory Core）同步到数据库
    DearTs::Core::Event::EventBus::instance().publish(Events::ConversationCreatedEvent{
        conv->id,
        title,
        type
    });

    LOG_INFO("Created conversation {}: {}", conv->id, title);
    return conv;
}

std::shared_ptr<Conversation> ConversationManager::get_conversation(const std::string& id) {
    for (auto& conv : m_conversations) {
        if (conv->id == id) {
            return conv;
        }
    }
    return nullptr;
}

bool ConversationManager::delete_conversation(const std::string& id) {
    auto it = std::remove_if(m_conversations.begin(), m_conversations.end(),
        [&id](const std::shared_ptr<Conversation>& conv) {
            return conv->id == id;
        });

    if (it != m_conversations.end()) {
        m_conversations.erase(it, m_conversations.end());

        // 如果删除的是当前会话，清空当前会话
        if (m_current_conversation && m_current_conversation->id == id) {
            m_current_conversation.reset();
        }

        // ✅ 发布删除事件，通知其他插件（如 Memory Core）
        DearTs::Core::Event::EventBus::instance().publish(Events::ConversationDeletedEvent{ id });

        LOG_INFO("Deleted conversation {}", id);
        return true;
    }

    return false;
}

bool ConversationManager::rename_conversation(const std::string& id, const std::string& new_title) {
    auto conv = find_by_id(id);
    if (!conv) {
        LOG_WARN("Conversation not found for renaming: {}", id);
        return false;
    }

    // 更新会话标题
    std::string old_title = conv->title;
    conv->title = new_title;
    conv->touch();

    // ✅ 发布更新事件，通知其他插件（如 Memory Core）
    DearTs::Core::Event::EventBus::instance().publish(
        Events::ConversationUpdatedEvent{
            id,
            Events::ConversationUpdateType::TitleChanged,
            new_title
        }
    );

    LOG_INFO("Renamed conversation {} from '{}' to '{}'", id, old_title, new_title);
    return true;
}

std::shared_ptr<Conversation> ConversationManager::load_conversation(
    const std::string& id,
    const std::string& title,
    ConversationType type,
    int64_t created_at,
    int64_t updated_at
) {
    // 检查是否已存在
    auto existing = find_by_id(id);
    if (existing) {
        LOG_DEBUG("Conversation {} already loaded, returning existing", id);
        return existing;
    }

    // 创建新会话对象（不发布事件）
    auto conv = std::make_shared<Conversation>();
    conv->id = id;
    conv->title = title;
    conv->type = type;

    // 转换 int64_t 时间戳（毫秒）到 time_point
    auto created_duration = std::chrono::milliseconds(created_at);
    auto updated_duration = std::chrono::milliseconds(updated_at);
    conv->created_at = std::chrono::system_clock::time_point(created_duration);
    conv->updated_at = std::chrono::system_clock::time_point(updated_duration);

    // 更新 next_id 以避免 ID 冲突（只在必要时）
    try {
        int id_num = std::stoi(id);
        int next_id_num = std::stoi(m_next_id);
        if (id_num >= next_id_num) {
            m_next_id = std::to_string(id_num + 1);
        }
    } catch (...) {
        // ID 解析失败，忽略
    }

    m_conversations.push_back(conv);

    LOG_INFO("Loaded conversation {} from database: {}", id, title);
    return conv;
}

DearTs::Core::Result<size_t, std::string> ConversationManager::load_messages(const std::string& conversation_id) {
    // 查找会话
    auto conv = find_by_id(conversation_id);
    if (!conv) {
        return DearTs::Core::Result<size_t, std::string>::err(
            std::format("Conversation {} not found", conversation_id)
        );
    }

    LOG_DEBUG("load_messages: conversation_id={}, current message count={}",
              conversation_id, conv->messages.size());

    // ✅ 移除跳过逻辑，总是尝试从数据库加载消息
    // 如果内存中已有消息，add_message 会追加到列表末尾
    // 这样可以确保显示最新的历史记录

    // 从数据库加载消息
    using namespace DearTs::Plugins::MemoryCore::Persistence;
    auto& db = SQLiteDatabase::instance();
    auto result = db.get_messages_by_conversation(conversation_id);

    if (result.isErr()) {
        LOG_ERROR("Failed to get messages from database: {}", result.error());
        return DearTs::Core::Result<size_t, std::string>::err(result.error());
    }

    auto& message_records = result.unwrap();
    LOG_INFO("Retrieved {} message records from database for conversation {}",
             message_records.size(), conversation_id);

    // ✅ 如果数据库返回的消息数量和内存中的一样，说明已经加载过了，跳过
    if (message_records.size() == conv->messages.size() && !conv->messages.empty()) {
        LOG_DEBUG("Messages already loaded ({} in memory, {} in DB), skipping",
                  conv->messages.size(), message_records.size());
        return DearTs::Core::Result<size_t, std::string>::ok(conv->messages.size());
    }

    // ✅ 清空现有消息（避免重复）
    if (!conv->messages.empty()) {
        LOG_DEBUG("Clearing {} existing messages before loading from database",
                  conv->messages.size());
        conv->messages.clear();
    }

    // 转换数据库记录为 Message 对象
    for (const auto& record : message_records) {
        // 转换角色字符串到 MessageRole
        MessageRole role = MessageRole::User;
        if (record.role == "user") {
            role = MessageRole::User;
        } else if (record.role == "assistant") {
            role = MessageRole::Assistant;
        } else if (record.role == "system") {
            role = MessageRole::System;
        }

        // 创建消息对象
        Message msg(record.content, role);
        msg.status = MessageStatus::Sent;

        // 设置时间戳
        auto timestamp_duration = std::chrono::milliseconds(record.timestamp);
        msg.timestamp = std::chrono::system_clock::time_point(timestamp_duration);

        if (record.tokens) {
            msg.token_count = *record.tokens;
        }

        // 设置消息 ID（使用 message_uuid）
        msg.id = record.message_uuid;

        conv->add_message(msg);
        LOG_DEBUG("Added message: role={}, content='{}'", record.role,
                  record.content.substr(0, std::min(size_t(30), record.content.length())));
    }

    LOG_INFO("Loaded {} messages for conversation {} from database (now has {} in memory)",
             message_records.size(), conversation_id, conv->messages.size());

    return DearTs::Core::Result<size_t, std::string>::ok(message_records.size());
}

std::shared_ptr<Conversation> ConversationManager::find_by_id(const std::string& id) const {
    for (const auto& conv : m_conversations) {
        if (conv->id == id) {
            return conv;
        }
    }
    return nullptr;
}

std::vector<std::shared_ptr<Conversation>> ConversationManager::search_by_title(
    const std::string& query
) const {
    std::vector<std::shared_ptr<Conversation>> results;

    for (const auto& conv : m_conversations) {
        const std::string& title = conv->get_display_title();
        if (title.find(query) != std::string::npos) {
            results.push_back(conv);
        }
    }

    return results;
}

} // namespace DearTs::Plugins::Chat
