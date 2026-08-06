/**
 * @file memory_extractor.hpp
 * @brief 记忆提取器 - 从对话中提取记忆
 *
 * 功能：
 * - 基于规则的记忆提取（模式匹配）
 * - 用户偏好检测
 * - 事实知识提取
 * - 问答对识别
 */

#pragma once

#include "core/result.h"
#include "memory_core/memory/memory_manager.hpp"
#include <string>
#include <vector>
#include <regex>
#include <memory>

namespace DearTs::Plugins::MemoryCore::Memory {

/**
 * @brief 提取规则配置
 */
struct ExtractionRule {
    std::string name;                    ///< 规则名称
    MemoryType target_type;              ///< 目标记忆类型
    std::vector<std::regex> patterns;    ///< 匹配模式列表
    double default_importance;           ///< 默认重要性 [0-1]
    bool enabled;                        ///< 是否启用

    ExtractionRule() : default_importance(0.5), enabled(true) {}
};

/**
 * @brief 提取结果
 */
struct ExtractionResult {
    Memory memory;                       ///< 提取的记忆
    std::string matched_text;            ///< 匹配的文本
    std::string rule_name;               ///< 匹配的规则名称
    double confidence;                   ///< 置信度 [0-1]
};

/**
 * @brief 记忆提取器
 *
 * 提供基于规则的记忆提取功能
 */
class MemoryExtractor {
public:
    /**
     * @brief 获取单例实例
     */
    static MemoryExtractor& instance();

    /**
     * @brief 删除拷贝构造和赋值
     */
    MemoryExtractor(const MemoryExtractor&) = delete;
    MemoryExtractor& operator=(const MemoryExtractor&) = delete;

    // ============ 规则管理 ============

    /**
     * @brief 初始化默认提取规则
     */
    void initialize_default_rules();

    /**
     * @brief 添加自定义规则
     * @param rule 要添加的规则
     */
    void add_rule(const ExtractionRule& rule);

    /**
     * @brief 移除规则
     * @param name 规则名称
     */
    void remove_rule(const std::string& name);

    /**
     * @brief 启用/禁用规则
     * @param name 规则名称
     * @param enabled 是否启用
     */
    void set_rule_enabled(const std::string& name, bool enabled);

    /**
     * @brief 获取所有规则
     */
    const std::vector<ExtractionRule>& get_rules() const { return m_rules; }

    // ============ 记忆提取 ============

    /**
     * @brief 从单个消息中提取记忆
     * @param message 消息内容
     * @param conversation_id 会话 ID（可选）
     * @param source_message_id 源消息 ID（可选）
     * @return 提取的记忆列表
     */
    DearTs::Core::Result<std::vector<ExtractionResult>, std::string> extract_from_message(
        const std::string& message,
        const std::string& conversation_id = "",
        int64_t source_message_id = 0
    );

    /**
     * @brief 从多个消息中提取记忆
     * @param messages 消息列表
     * @param conversation_id 会话 ID（可选）
     * @return 提取的记忆列表
     */
    DearTs::Core::Result<std::vector<ExtractionResult>, std::string> extract_from_messages(
        const std::vector<std::string>& messages,
        const std::string& conversation_id = ""
    );

    /**
     * @brief 提取并保存记忆
     * @param message 消息内容
     * @param conversation_id 会话 ID
     * @param source_message_id 源消息 ID
     * @return 保存的记忆 ID 列表或错误信息
     */
    DearTs::Core::Result<std::vector<int64_t>, std::string> extract_and_save(
        const std::string& message,
        const std::string& conversation_id,
        int64_t source_message_id
    );

    // ============ 专用提取器 ============

    /**
     * @brief 提取用户偏好
     * @param message 消息内容
     * @return 提取的偏好列表
     */
    std::vector<ExtractionResult> extract_preferences(const std::string& message);

    /**
     * @brief 提取事实知识
     * @param message 消息内容
     * @return 提取的事实列表
     */
    std::vector<ExtractionResult> extract_facts(const std::string& message);

    /**
     * @brief 提取问答对
     * @param user_message 用户消息
     * @param assistant_message 助手消息
     * @return 提取的问答对列表
     */
    std::vector<ExtractionResult> extract_qa_pairs(
        const std::string& user_message,
        const std::string& assistant_message
    );

private:
    /**
     * @brief 私有构造函数（单例模式）
     */
    MemoryExtractor() = default;

    /**
     * @brief 析构函数
     */
    ~MemoryExtractor() = default;

    /**
     * @brief 应用规则提取记忆
     * @param text 输入文本
     * @param rule 要应用的规则
     * @param conversation_id 会话 ID
     * @param source_message_id 源消息 ID
     * @return 提取结果
     */
    std::vector<ExtractionResult> apply_rule(
        const std::string& text,
        const ExtractionRule& rule,
        const std::string& conversation_id = "",
        int64_t source_message_id = 0
    );

    /**
     * @brief 计算提取置信度
     * @param matched_text 匹配的文本
     * @param pattern 匹配的模式
     * @return 置信度 [0-1]
     */
    double calculate_confidence(const std::string& matched_text, const std::regex& pattern);

    /**
     * @brief 清理提取的文本
     * @param text 原始文本
     * @return 清理后的文本
     */
    std::string clean_extracted_text(const std::string& text);

    // ============ 成员变量 ============

    std::vector<ExtractionRule> m_rules;   ///< 提取规则列表
};

// ============ 内置规则工厂 ============

namespace BuiltInRules {

/**
 * @brief 创建用户偏好规则
 */
ExtractionRule create_preference_rules();

/**
 * @brief 创建事实知识规则
 */
ExtractionRule create_fact_rules();

/**
 * @brief 创建技能信息规则
 */
ExtractionRule create_skill_rules();

/**
 * @brief 创建上下文信息规则
 */
ExtractionRule create_context_rules();

} // namespace BuiltInRules

} // namespace DearTs::Plugins::MemoryCore::Memory
