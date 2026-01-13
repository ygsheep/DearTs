/**
 * @file message.cpp
 * @brief 消息数据模型实现
 */

#include "chat/models/message.hpp"
#include <fmt/format.h>
#include <cctype>

namespace DearTs::Plugins::Chat {

void Message::estimate_tokens() {
    // 粗略估算：1 token ≈ 4 个英文字符 或 2 个汉字
    size_t total_chars = content.length();
    size_t chinese_chars = 0;

    for (char c : content) {
        // 判断是否为非 ASCII 字符（包括中文）
        if (static_cast<unsigned char>(c) > 127) {
            chinese_chars++;
        }
    }

    const size_t english_chars = total_chars - chinese_chars;
    token_count = static_cast<int32_t>((english_chars / 4) + (chinese_chars / 2) + 1);
}

std::string Message::get_time_string() const {
    const auto time_t = std::chrono::system_clock::to_time_t(timestamp);
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &time_t);
#else
    localtime_r(&time_t, &tm);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M:%S");
    return oss.str();
}

} // namespace DearTs::Plugins::Chat
