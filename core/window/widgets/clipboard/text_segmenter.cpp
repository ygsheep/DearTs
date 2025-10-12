#include "text_segmenter.h"
#include "../../utils/logger.h"
#include <sstream>
#include <cctype>
#include <algorithm>

namespace DearTs::Core::Window::Widgets::Clipboard {

// 正则表达式模式定义 - 修复Unicode转义序列错误
// 移除了有问题的Unicode正则表达式，改用字符检测函数

const std::regex TextSegmenter::ENGLISH_PATTERN(
    R"([a-zA-Z]+)"
);

const std::regex TextSegmenter::NUMBER_PATTERN(
    R"(\d+\.?\d*)"
);

// 移除了有问题的Unicode正则表达式 - 改用字符检测函数处理

const std::regex TextSegmenter::URL_PATTERN(
    R"(https?:\/\/[^\s]+)"
);

const std::regex TextSegmenter::EMAIL_PATTERN(
    R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})"
);

TextSegmenter::TextSegmenter() : is_initialized_(false) {
    DEARTS_LOG_INFO("TextSegmenter构造函数");
}

bool TextSegmenter::initialize() {
    is_initialized_ = true;
    DEARTS_LOG_INFO("文本分词器初始化成功");
    return true;
}

std::vector<TextSegment> TextSegmenter::segmentText(const std::string& text,
                                                      Method method) {
    if (!is_initialized_) {
        DEARTS_LOG_WARN("文本分词器未初始化");
        return {};
    }

    DEARTS_LOG_INFO("开始分词，方法: " + std::to_string(static_cast<int>(method)) +
                   "，文本长度: " + std::to_string(text.length()));

    std::vector<TextSegment> segments;

    switch (method) {
        case Method::SIMPLE_SPLIT:
            segments = simpleSegmentation(text);
            break;
        case Method::REGEX_BASED:
            segments = regexSegmentation(text);
            break;
        case Method::MIXED_MODE:
        default:
            segments = mixedSegmentation(text);
            break;
    }

    DEARTS_LOG_INFO("分词完成，共分得 " + std::to_string(segments.size()) + " 个片段");
    return segments;
}

std::string TextSegmenter::getWordTag(const std::string& word) {
    return basicPosTagging(word);
}

std::vector<TextSegment> TextSegmenter::simpleSegmentation(const std::string& text) {
    std::vector<TextSegment> segments;
    size_t start = 0;

    while (start < text.length()) {
        size_t end = start;

        // 跳过空白字符
        while (start < text.length() && isspace(static_cast<unsigned char>(text[start]))) {
            start++;
        }
        if (start >= text.length()) break;

        // 找到下一个空白字符或标点符号
        while (end < text.length() &&
               !isspace(static_cast<unsigned char>(text[end])) &&
               !isPunctuationChar(text[end])) {
            end++;
        }

        if (start < end) {
            std::string segment_text = text.substr(start, end - start);
            TextSegment segment(segment_text, start, end, getWordTag(segment_text));
            segments.push_back(segment);
        }

        start = end + 1;
    }

    return segments;
}

std::vector<TextSegment> TextSegmenter::regexSegmentation(const std::string& text) {
    std::vector<TextSegment> segments;
    std::vector<std::pair<std::regex, std::string>> patterns = {
        {URL_PATTERN, "url"},
        {EMAIL_PATTERN, "email"},
        {NUMBER_PATTERN, "num"},
        {ENGLISH_PATTERN, "en"}
    };

    size_t pos = 0;
    while (pos < text.length()) {
        bool matched = false;

        // 首先检查是否为中文字符（不使用正则表达式）
        if (pos < text.length()) {
            char32_t current_char = 0;
            size_t char_len = 1;

            // 处理UTF-8字符
            if ((text[pos] & 0x80) == 0) {
                current_char = static_cast<char32_t>(text[pos]);
            } else if ((text[pos] & 0xE0) == 0xC0) {
                if (pos + 1 < text.length()) {
                    current_char = ((text[pos] & 0x1F) << 6) | (text[pos + 1] & 0x3F);
                    char_len = 2;
                }
            } else if ((text[pos] & 0xF0) == 0xE0) {
                if (pos + 2 < text.length()) {
                    current_char = ((text[pos] & 0x0F) << 12) |
                                   ((text[pos + 1] & 0x3F) << 6) |
                                   (text[pos + 2] & 0x3F);
                    char_len = 3;
                }
            } else if ((text[pos] & 0xF8) == 0xF0) {
                if (pos + 3 < text.length()) {
                    current_char = ((text[pos] & 0x07) << 18) |
                                   ((text[pos + 1] & 0x3F) << 12) |
                                   ((text[pos + 2] & 0x3F) << 6) |
                                   (text[pos + 3] & 0x3F);
                    char_len = 4;
                }
            }

            if (current_char != 0) {
                if (isChineseChar(current_char)) {
                    std::string char_str = text.substr(pos, char_len);
                    TextSegment segment(char_str, pos, pos + char_len, "zh");
                    segments.push_back(segment);
                    pos += char_len;
                    matched = true;
                    continue;
                } else if (isPunctuationChar(text[pos])) {
                    std::string punct_str(1, text[pos]);
                    TextSegment segment(punct_str, pos, pos + 1, "punct");
                    segments.push_back(segment);
                    pos += 1;
                    matched = true;
                    continue;
                }
            }
        }

        // 然后尝试正则表达式匹配
        for (auto& [pattern, tag] : patterns) {
            std::sregex_iterator iter(text.begin() + pos, text.end(), pattern);
            std::sregex_iterator end;

            if (iter != end && iter->position() == 0) {
                std::string match_text = iter->str();
                TextSegment segment(match_text, pos, pos + match_text.length(), tag);
                segments.push_back(segment);
                pos += match_text.length();
                matched = true;
                break;
            }
        }

        if (!matched) {
            // 如果没有匹配任何模式，创建一个单字符片段
            TextSegment segment(std::string(1, text[pos]), pos, pos + 1, "unknown");
            segments.push_back(segment);
            pos++;
        }
    }

    return segments;
}

std::vector<TextSegment> TextSegmenter::mixedSegmentation(const std::string& text) {
    std::vector<TextSegment> segments;
    size_t pos = 0;

    while (pos < text.length()) {
        // 跳过空白字符
        while (pos < text.length() && isspace(static_cast<unsigned char>(text[pos]))) {
            pos++;
        }
        if (pos >= text.length()) break;

        char32_t current_char = 0;
        size_t char_len = 1;

        // 处理UTF-8字符
        if ((text[pos] & 0x80) == 0) {
            // ASCII字符
            current_char = static_cast<char32_t>(text[pos]);
        } else if ((text[pos] & 0xE0) == 0xC0) {
            // 2字节UTF-8
            if (pos + 1 < text.length()) {
                current_char = ((text[pos] & 0x1F) << 6) | (text[pos + 1] & 0x3F);
                char_len = 2;
            }
        } else if ((text[pos] & 0xF0) == 0xE0) {
            // 3字节UTF-8
            if (pos + 2 < text.length()) {
                current_char = ((text[pos] & 0x0F) << 12) |
                               ((text[pos + 1] & 0x3F) << 6) |
                               (text[pos + 2] & 0x3F);
                char_len = 3;
            }
        } else if ((text[pos] & 0xF8) == 0xF0) {
            // 4字节UTF-8
            if (pos + 3 < text.length()) {
                current_char = ((text[pos] & 0x07) << 18) |
                               ((text[pos + 1] & 0x3F) << 12) |
                               ((text[pos + 2] & 0x3F) << 6) |
                               (text[pos + 3] & 0x3F);
                char_len = 4;
            }
        }

        if (current_char == 0) {
            pos++;
            continue;
        }

        // 根据字符类型进行分段
        if (isChineseChar(current_char)) {
            // 中文字符，单独分词
            std::string char_str = text.substr(pos, char_len);
            TextSegment segment(char_str, pos, pos + char_len, "zh");
            segments.push_back(segment);
            pos += char_len;
        } else if (isEnglishChar(text[pos])) {
            // 英文单词，收集连续的英文字符
            auto eng_segments = segmentEnglish(text, pos);
            segments.insert(segments.end(), eng_segments.begin(), eng_segments.end());
            pos += eng_segments.back().end_pos - pos;
        } else if (isdigit(static_cast<unsigned char>(text[pos]))) {
            // 数字，收集连续的数字字符
            auto num_segments = extractNumbers(text, pos);
            segments.insert(segments.end(), num_segments.begin(), num_segments.end());
            pos += num_segments.back().end_pos - pos;
        } else if (isPunctuationChar(text[pos])) {
            // 标点符号
            auto punct_segments = extractPunctuation(text, pos);
            segments.insert(segments.end(), punct_segments.begin(), punct_segments.end());
            pos += punct_segments.back().end_pos - pos;
        } else {
            // 其他字符，单个分词
            std::string char_str = text.substr(pos, char_len);
            TextSegment segment(char_str, pos, pos + char_len, "unknown");
            segments.push_back(segment);
            pos += char_len;
        }
    }

    return segments;
}

std::vector<TextSegment> TextSegmenter::segmentChinese(const std::string& text, size_t start_pos) {
    std::vector<TextSegment> segments;
    auto unicode_chars = utf8ToUnicode(text.substr(start_pos));

    for (size_t i = 0; i < unicode_chars.size(); ++i) {
        if (isChineseChar(unicode_chars[i])) {
            std::string char_str = unicodeToUtf8({unicode_chars[i]});
            size_t actual_pos = start_pos; // 这里需要更精确的位置计算
            TextSegment segment(char_str, actual_pos, actual_pos + char_str.length(), "zh");
            segments.push_back(segment);
        }
    }

    return segments;
}

std::vector<TextSegment> TextSegmenter::segmentEnglish(const std::string& text, size_t start_pos) {
    std::vector<TextSegment> segments;
    size_t end = start_pos;

    while (end < text.length() && isEnglishChar(text[end])) {
        end++;
    }

    if (start_pos < end) {
        std::string word = text.substr(start_pos, end - start_pos);
        TextSegment segment(word, start_pos, end, getWordTag(word));
        segments.push_back(segment);
    }

    return segments;
}

std::vector<TextSegment> TextSegmenter::extractNumbers(const std::string& text, size_t start_pos) {
    std::vector<TextSegment> segments;
    size_t end = start_pos;

    while (end < text.length() && (isdigit(static_cast<unsigned char>(text[end])) || text[end] == '.')) {
        end++;
    }

    if (start_pos < end) {
        std::string number = text.substr(start_pos, end - start_pos);
        TextSegment segment(number, start_pos, end, "num");
        segments.push_back(segment);
    }

    return segments;
}

std::vector<TextSegment> TextSegmenter::extractPunctuation(const std::string& text, size_t start_pos) {
    std::vector<TextSegment> segments;

    std::string punct_str(1, text[start_pos]);
    TextSegment segment(punct_str, start_pos, start_pos + 1, "punct");
    segments.push_back(segment);

    return segments;
}

std::vector<char32_t> TextSegmenter::utf8ToUnicode(const std::string& utf8_str) {
    std::vector<char32_t> unicode_chars;

    for (size_t i = 0; i < utf8_str.length();) {
        char32_t code_point = 0;
        size_t char_len = 1;

        if ((utf8_str[i] & 0x80) == 0) {
            // 1字节
            code_point = static_cast<char32_t>(utf8_str[i]);
            char_len = 1;
        } else if ((utf8_str[i] & 0xE0) == 0xC0) {
            // 2字节
            if (i + 1 < utf8_str.length()) {
                code_point = ((utf8_str[i] & 0x1F) << 6) | (utf8_str[i + 1] & 0x3F);
                char_len = 2;
            }
        } else if ((utf8_str[i] & 0xF0) == 0xE0) {
            // 3字节
            if (i + 2 < utf8_str.length()) {
                code_point = ((utf8_str[i] & 0x0F) << 12) |
                           ((utf8_str[i + 1] & 0x3F) << 6) |
                           (utf8_str[i + 2] & 0x3F);
                char_len = 3;
            }
        } else if ((utf8_str[i] & 0xF8) == 0xF0) {
            // 4字节
            if (i + 3 < utf8_str.length()) {
                code_point = ((utf8_str[i] & 0x07) << 18) |
                           ((utf8_str[i + 1] & 0x3F) << 12) |
                           ((utf8_str[i + 2] & 0x3F) << 6) |
                           (utf8_str[i + 3] & 0x3F);
                char_len = 4;
            }
        }

        if (code_point != 0) {
            unicode_chars.push_back(code_point);
        }

        i += char_len;
    }

    return unicode_chars;
}

std::string TextSegmenter::unicodeToUtf8(const std::vector<char32_t>& unicode_chars) {
    std::string utf8_str;

    for (char32_t code_point : unicode_chars) {
        if (code_point <= 0x7F) {
            utf8_str += static_cast<char>(code_point);
        } else if (code_point <= 0x7FF) {
            utf8_str += static_cast<char>(0xC0 | (code_point >> 6));
            utf8_str += static_cast<char>(0x80 | (code_point & 0x3F));
        } else if (code_point <= 0xFFFF) {
            utf8_str += static_cast<char>(0xE0 | (code_point >> 12));
            utf8_str += static_cast<char>(0x80 | ((code_point >> 6) & 0x3F));
            utf8_str += static_cast<char>(0x80 | (code_point & 0x3F));
        } else if (code_point <= 0x10FFFF) {
            utf8_str += static_cast<char>(0xF0 | (code_point >> 18));
            utf8_str += static_cast<char>(0x80 | ((code_point >> 12) & 0x3F));
            utf8_str += static_cast<char>(0x80 | ((code_point >> 6) & 0x3F));
            utf8_str += static_cast<char>(0x80 | (code_point & 0x3F));
        }
    }

    return utf8_str;
}

std::string TextSegmenter::basicPosTagging(const std::string& word) {
    if (word.empty()) return "unknown";

    // 检查是否为数字
    if (std::all_of(word.begin(), word.end(), [](char c) { return std::isdigit(static_cast<unsigned char>(c)); })) {
        return "num";
    }

    // 检查是否包含数字（可能是混合数字词）
    if (std::any_of(word.begin(), word.end(), [](char c) { return std::isdigit(static_cast<unsigned char>(c)); })) {
        return "mixed";
    }

    // 检查是否为英文字母
    if (std::all_of(word.begin(), word.end(), [](char c) { return std::isalpha(static_cast<unsigned char>(c)); })) {
        return "en";
    }

    // 检查是否为中文字符
    auto unicode_chars = utf8ToUnicode(word);
    if (!unicode_chars.empty() && std::all_of(unicode_chars.begin(), unicode_chars.end(),
                                               [](char32_t c) { return isChineseChar(c); })) {
        return "zh";
    }

    // 检查长度进行简单分类
    if (word.length() == 1) {
        return "char";
    } else if (word.length() <= 2) {
        return "short";
    } else if (word.length() <= 4) {
        return "medium";
    } else {
        return "long";
    }
}

bool TextSegmenter::isChineseChar(char32_t c) {
    return (c >= 0x4E00 && c <= 0x9FFF) ||  // CJK统一汉字
           (c >= 0x3400 && c <= 0x4DBF) ||  // CJK扩展A
           (c >= 0x20000 && c <= 0x2A6DF) || // CJK扩展B
           (c >= 0x2A700 && c <= 0x2B73F) || // CJK扩展C
           (c >= 0x2B740 && c <= 0x2B81F) || // CJK扩展D
           (c >= 0x2B820 && c <= 0x2CEAF) || // CJK扩展E
           (c >= 0xF900 && c <= 0xFAFF) ||  // CJK兼容汉字
           (c >= 0x2F800 && c <= 0x2FA1F);   // CJK兼容汉字补充
}

bool TextSegmenter::isEnglishChar(char c) {
    return std::isalpha(static_cast<unsigned char>(c));
}

bool TextSegmenter::isDigitChar(char c) {
    return std::isdigit(static_cast<unsigned char>(c));
}

bool TextSegmenter::isPunctuationChar(char c) {
    return std::ispunct(static_cast<unsigned char>(c)) || c == '，' || c == '。' ||
           c == '！' || c == '？' || c == '；' || c == '：' || c == '"' || c == '"' ||
           c == '\'' || c == '\'' || c == '（' || c == '）' || c == '【' || c == '】';
}

} // namespace DearTs::Core::Window::Widgets::Clipboard