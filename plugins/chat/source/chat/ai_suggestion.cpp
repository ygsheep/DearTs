/**
 * @file ai_suggestion.cpp
 * @brief AI 建议数据模型实现
 */

#include "chat/models/ai_suggestion.hpp"
#include <algorithm>

namespace DearTs::Plugins::Chat {

std::string AISuggestion::get_preview() const {
    const size_t max_preview_length = 50;

    if (content.length() <= max_preview_length) {
        return content;
    }

    return content.substr(0, max_preview_length - 3) + "...";
}

} // namespace DearTs::Plugins::Chat
