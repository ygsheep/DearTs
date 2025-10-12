#pragma once

#include <string>
#include <vector>
#include <memory>
#include <regex>
#include <imgui.h>

namespace DearTs::Core::Window::Widgets::Clipboard {

/**
 * @brief 文本片段结构
 */
struct TextSegment {
    std::string text;                    // 文本内容
    std::string tag;                     // 词性标签
    size_t start_pos;                    // 起始位置
    size_t end_pos;                      // 结束位置
    float confidence;                    // 置信度（0.0-1.0）
    ImVec2 position;                     // 渲染位置
    ImVec2 size;                         // 渲染大小
    bool is_hovered = false;             // 是否悬停
    bool is_selected = false;            // 是否选中
    ImVec4 bg_color;                     // 背景颜色
    ImVec4 border_color;                 // 边框颜色
    float border_width = 1.0f;           // 边框宽度
    int index;                           // 在原文中的索引

    TextSegment() : start_pos(0), end_pos(0), confidence(0.0f) {}
    TextSegment(const std::string& t, size_t start, size_t end,
                const std::string& ttag = "", float conf = 1.0f)
        : text(t), tag(ttag), start_pos(start), end_pos(end), confidence(conf) {
        // 初始化UI交互状态
        position = ImVec2(0, 0);
        size = ImVec2(0, 0);
        bg_color = ImVec4(0, 0, 0, 0);
        border_color = ImVec4(0, 0, 0, 0);
        index = 0;
    }
};

/**
 * @brief 文本分词器类
 *
 * 简化版的文本分词器，支持中英文分词，不依赖外部库。
 * 提供基础的分词功能和词性标注。
 */
class TextSegmenter {
public:
    /**
     * @brief 分词方法
     */
    enum class Method {
        SIMPLE_SPLIT,      // 简单分词（按空格和标点）
        REGEX_BASED,       // 正则表达式分词
        MIXED_MODE         // 混合模式
    };

    /**
     * @brief 构造函数
     */
    TextSegmenter();

    /**
     * @brief 析构函数
     */
    ~TextSegmenter() = default;

    /**
     * @brief 初始化分词器
     * @return 是否初始化成功
     */
    bool initialize();

    /**
     * @brief 对文本进行分词
     * @param text 要分词的文本
     * @param method 分词方法
     * @return 分词结果列表
     */
    std::vector<TextSegment> segmentText(const std::string& text,
                                         Method method = Method::MIXED_MODE);

    /**
     * @brief 获取词性标签
     * @param word 词语
     * @return 词性标签
     */
    std::string getWordTag(const std::string& word);

    /**
     * @brief 判断是否为中文字符
     * @param c 字符
     * @return 是否为中文字符
     */
    static bool isChineseChar(char32_t c);

    /**
     * @brief 判断是否为英文字符
     * @param c 字符
     * @return 是否为英文字符
     */
    static bool isEnglishChar(char c);

    /**
     * @brief 判断是否为数字字符
     * @param c 字符
     * @return 是否为数字字符
     */
    static bool isDigitChar(char c);

    /**
     * @brief 判断是否为标点符号
     * @param c 字符
     * @return 是否为标点符号
     */
    static bool isPunctuationChar(char c);

private:
    /**
     * @brief 简单分词方法
     * @param text 文本
     * @return 分词结果
     */
    std::vector<TextSegment> simpleSegmentation(const std::string& text);

    /**
     * @brief 正则表达式分词方法
     * @param text 文本
     * @return 分词结果
     */
    std::vector<TextSegment> regexSegmentation(const std::string& text);

    /**
     * @brief 混合分词方法
     * @param text 文本
     * @return 分词结果
     */
    std::vector<TextSegment> mixedSegmentation(const std::string& text);

    /**
     * @brief 中文分词（基于字符）
     * @param text 文本
     * @param start_pos 起始位置
     * @return 分词结果
     */
    std::vector<TextSegment> segmentChinese(const std::string& text, size_t start_pos);

    /**
     * @brief 英文分词（基于单词）
     * @param text 文本
     * @param start_pos 起始位置
     * @return 分词结果
     */
    std::vector<TextSegment> segmentEnglish(const std::string& text, size_t start_pos);

    /**
     * @brief 提取数字
     * @param text 文本
     * @param start_pos 起始位置
     * @return 分词结果
     */
    std::vector<TextSegment> extractNumbers(const std::string& text, size_t start_pos);

    /**
     * @brief 提取标点符号
     * @param text 文本
     * @param start_pos 起始位置
     * @return 分词结果
     */
    std::vector<TextSegment> extractPunctuation(const std::string& text, size_t start_pos);

    /**
     * @brief UTF-8字符串转Unicode字符
     * @param utf8_str UTF-8字符串
     * @return Unicode字符列表
     */
    std::vector<char32_t> utf8ToUnicode(const std::string& utf8_str);

    /**
     * @brief Unicode字符转UTF-8字符串
     * @param unicode_chars Unicode字符列表
     * @return UTF-8字符串
     */
    std::string unicodeToUtf8(const std::vector<char32_t>& unicode_chars);

    /**
     * @brief 基础词性标注
     * @param word 词语
     * @return 词性标签
     */
    std::string basicPosTagging(const std::string& word);

    // 成员变量
    bool is_initialized_;                           // 是否已初始化

    // 正则表达式模式 - 移除了有问题的Unicode模式
    // static const std::regex CHINESE_PATTERN;   // 已移除，改用字符检测函数
    static const std::regex ENGLISH_PATTERN;       // 英文单词模式
    static const std::regex NUMBER_PATTERN;        // 数字模式
    // static const std::regex PUNCTUATION_PATTERN; // 已移除，改用字符检测函数
    static const std::regex URL_PATTERN;           // URL模式
    static const std::regex EMAIL_PATTERN;         // 邮箱模式
};

} // namespace DearTs::Core::Window::Widgets::Clipboard