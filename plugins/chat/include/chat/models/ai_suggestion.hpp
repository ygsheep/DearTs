/**
 * @file ai_suggestion.hpp
 * @brief AI 建议回复数据模型
 */

#pragma once

#include <string>
#include <vector>

namespace DearTs::Plugins::Chat {

/**
 * @brief AI 建议回复
 */
struct AISuggestion {
    std::string id;                          // 建议 ID
    std::string content;                     // 建议内容
    float confidence = 0.0f;                 // 置信度 (0.0 - 1.0)

    // 元数据
    std::string reasoning;                   // 推理过程（可选）
    int token_count = 0;                     // 估算的 token 数量

    // 构造函数
    AISuggestion() = default;

    AISuggestion(std::string content, float confidence = 0.8f)
        : content(std::move(content))
        , confidence(confidence) {}

    /**
     * @brief 判断建议是否有效
     */
    [[nodiscard]] bool is_valid() const {
        return !content.empty() && confidence > 0.0f;
    }

    /**
     * @brief 获取简短的预览文本（最多 50 字符）
     */
    [[nodiscard]] std::string get_preview() const;
};

/**
 * @brief AI 分析结果
 */
struct AIAnalysisResult {
    std::string response;                    // AI 的主要回复
    std::vector<AISuggestion> suggestions;   // 建议回复列表（多个选项）

    // 元数据
    bool is_complete = false;                // 是否完整响应
    std::string error;                       // 错误信息（如果有）
    int total_tokens = 0;                    // 使用的总 token 数
    double duration_ms = 0.0;                // 处理耗时（毫秒）

    /**
     * @brief 判断分析是否成功
     */
    [[nodiscard]] bool is_success() const {
        return is_complete && error.empty();
    }

    /**
     * @brief 获取最佳建议（置信度最高的）
     */
    [[nodiscard]] const AISuggestion* get_best_suggestion() const {
        if (suggestions.empty()) return nullptr;

        const AISuggestion* best = &suggestions[0];
        for (const auto& suggestion : suggestions) {
            if (suggestion.confidence > best->confidence) {
                best = &suggestion;
            }
        }
        return best;
    }

    /**
     * @brief 清空结果
     */
    void clear() {
        response.clear();
        suggestions.clear();
        is_complete = false;
        error.clear();
        total_tokens = 0;
        duration_ms = 0.0;
    }
};

/**
 * @brief AI 上下文配置
 */
struct AIContextConfig {
    int max_context_messages = 10;           // 最大上下文消息数
    int max_tokens = 4096;                   // 最大 token 数
    float temperature = 0.7f;                // 温度
    float top_p = 1.0f;                      // Top-p 采样
    std::string system_prompt;               // 系统提示词

    // 默认配置
    static AIContextConfig default_chat() {
        AIContextConfig config;
        config.system_prompt = "你是一个友好、专业的 AI 助手。";
        return config;
    }

    static AIContextConfig creative() {
        AIContextConfig config;
        config.temperature = 0.9f;
        config.system_prompt = "你是一个富有创造力的 AI 助手。";
        return config;
    }

    static AIContextConfig precise() {
        AIContextConfig config;
        config.temperature = 0.3f;
        config.system_prompt = "你是一个精确、专业的 AI 助手。";
        return config;
    }
};

} // namespace DearTs::Plugins::Chat
