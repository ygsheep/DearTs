/**
 * @file conversation.cpp
 * @brief 会话数据模型实现
 */

#include "chat/models/conversation.hpp"
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

        LOG_INFO("Deleted conversation {}", id);
        return true;
    }

    return false;
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
