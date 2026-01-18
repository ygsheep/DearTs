/**
 * @file summarizer.hpp
 * @brief 摘要生成器 - 生成对话摘要
 *
 * 功能：
 * - 对话摘要生成
 * - 多种摘要风格
 * - 智能触发
 * - 摘要缓存
 */

#pragma once

#include "core/result.h"
#include "memory_core/summarization/topic_segmenter.hpp"
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <optional>
#include <cstdint>
#include <map>

namespace DearTs::Plugins::MemoryCore::Summarization {

/**
 * @brief 摘要风格
 */
enum class SummaryStyle {
    Concise,               ///< 简洁风格（要点列表）
    Detailed,              ///< 详细风格（完整叙述）
    BulletPoints,          ///< 项目符号
    QuestionAnswer,        ///< 问答形式
    Timeline               ///< 时间线形式
};

/**
 * @brief 摘要结果
 */
struct SummaryResult {
    std::string summary;                        ///< 摘要内容
    std::string topic_hint;                     ///< 主题提示
    int64_t conversation_id;                    ///< 会话 ID
    int64_t start_time;                         ///< 开始时间
    int64_t end_time;                           ///< 结束时间
    int message_count;                          ///< 消息数量
    double importance_score;                    ///< 重要性分数
    SummaryStyle style;                         ///< 摘要风格

    /**
     * @brief 序列化为 JSON
     */
    std::string to_json() const;

    /**
     * @brief 从 JSON 反序列化
     */
    static DearTs::Core::Result<SummaryResult, std::string> from_json(
        const std::string& json
    );
};

/**
 * @brief 摘要生成器配置
 */
struct SummarizerConfig {
    SummaryStyle default_style;                 ///< 默认风格
    int max_summary_length;                     ///< 最大摘要长度（字符）
    bool enable_cache;                          ///< 启用缓存
    bool include_timestamps;                    ///< 包含时间戳
    bool include_participants;                  ///< 包含参与者信息
    int cache_size;                             ///< 缓存大小

    /**
     * @brief 默认配置
     */
    static SummarizerConfig default_config() {
        return SummarizerConfig{
            .default_style = SummaryStyle::Concise,
            .max_summary_length = 500,
            .enable_cache = true,
            .include_timestamps = false,
            .include_participants = true,
            .cache_size = 100
        };
    }
};

/**
 * @brief 摘要触发条件
 */
struct SummaryTriggerCondition {
    int message_threshold;                      ///< 消息数量阈值
    int time_threshold_seconds;                 ///< 时间阈值（秒）
    double topic_change_threshold;              ///< 主题变化阈值
    bool enable_auto_trigger;                   ///< 启用自动触发

    /**
     * @brief 默认条件
     */
    static SummaryTriggerCondition default_condition() {
        return SummaryTriggerCondition{
            .message_threshold = 50,
            .time_threshold_seconds = 3600,     // 1 小时
            .topic_change_threshold = 0.3,
            .enable_auto_trigger = false
        };
    }

    /**
     * @brief 激进条件（频繁触发）
     */
    static SummaryTriggerCondition aggressive_condition() {
        return SummaryTriggerCondition{
            .message_threshold = 20,
            .time_threshold_seconds = 1800,     // 30 分钟
            .topic_change_threshold = 0.5,
            .enable_auto_trigger = true
        };
    }

    /**
     * @brief 保守条件（较少触发）
     */
    static SummaryTriggerCondition conservative_condition() {
        return SummaryTriggerCondition{
            .message_threshold = 100,
            .time_threshold_seconds = 7200,     // 2 小时
            .topic_change_threshold = 0.2,
            .enable_auto_trigger = false
        };
    }
};

/**
 * @brief 摘要统计
 */
struct SummarizerStats {
    int total_summaries;                        ///< 总摘要数
    int64_t total_characters;                   ///< 总字符数
    int cache_hits;                             ///< 缓存命中数
    int cache_misses;                           ///< 缓存未命中数
    double average_summary_length;              ///< 平均摘要长度

    /**
     * @brief 创建空统计
     */
    static SummarizerStats empty() {
        return SummarizerStats{
            .total_summaries = 0,
            .total_characters = 0,
            .cache_hits = 0,
            .cache_misses = 0,
            .average_summary_length = 0.0
        };
    }
};

/**
 * @brief 摘要进度回调
 *
 * @param stage 当前阶段
 * @param progress 进度 [0-1]
 * @param message 进度消息
 */
using SummaryProgressCallback = std::function<void(
    const std::string& stage,
    double progress,
    const std::string& message
)>;

/**
 * @brief 摘要生成器
 *
 * 负责生成对话摘要
 */
class Summarizer {
public:
    /**
     * @brief 获取单例实例
     */
    static Summarizer& instance();

    /**
     * @brief 删除拷贝构造和赋值
     */
    Summarizer(const Summarizer&) = delete;
    Summarizer& operator=(const Summarizer&) = delete;

    // ============ 初始化 ============

    /**
     * @brief 初始化摘要生成器
     * @param config 生成器配置
     * @return 成功或错误信息
     */
    DearTs::Core::Result<void, std::string> initialize(
        const SummarizerConfig& config = SummarizerConfig::default_config()
    );

    /**
     * @brief 关闭摘要生成器
     */
    void shutdown();

    // ============ 摘要生成 ============

    /**
     * @brief 生成摘要
     * @param conversation_id 会话 ID
     * @param messages 消息列表
     * @param style 摘要风格
     * @param progress_callback 进度回调（可选）
     * @return 摘要结果或错误信息
     */
    DearTs::Core::Result<SummaryResult, std::string> generate_summary(
        int64_t conversation_id,
        const std::vector<ConversationMessage>& messages,
        SummaryStyle style = SummaryStyle::Concise,
        SummaryProgressCallback progress_callback = nullptr
    );

    /**
     * @brief 为分段生成摘要
     * @param conversation_id 会话 ID
     * @param segments 对话分段
     * @param style 摘要风格
     * @return 摘要结果列表或错误信息
     */
    DearTs::Core::Result<std::vector<SummaryResult>, std::string> generate_summaries_for_segments(
        int64_t conversation_id,
        const std::vector<ConversationSegment>& segments,
        SummaryStyle style = SummaryStyle::Concise
    );

    /**
     * @brief 增量更新摘要
     * @param existing_summary 现有摘要
     * @param new_messages 新消息
     * @return 更新后的摘要或错误信息
     */
    DearTs::Core::Result<SummaryResult, std::string> update_summary(
        const SummaryResult& existing_summary,
        const std::vector<ConversationMessage>& new_messages
    );

    // ============ 触发检测 ============

    /**
     * @brief 检查是否应该生成摘要
     * @param conversation_id 会话 ID
     * @param messages 消息列表
     * @param condition 触发条件
     * @return true 表示应该生成摘要
     */
    bool should_trigger_summary(
        int64_t conversation_id,
        const std::vector<ConversationMessage>& messages,
        const SummaryTriggerCondition& condition
    );

    /**
     * @brief 设置触发条件
     * @param condition 触发条件
     */
    void set_trigger_condition(const SummaryTriggerCondition& condition) {
        m_trigger_condition = condition;
    }

    // ============ 缓存管理 ============

    /**
     * @brief 清除缓存
     */
    void clear_cache();

    /**
     * @brief 获取缓存统计
     * @return 缓存命中率
     */
    double get_cache_hit_rate() const;

    // ============ 统计 ============

    /**
     * @brief 获取统计信息
     * @return 统计数据
     */
    SummarizerStats get_stats() const;

    /**
     * @brief 打印统计信息
     */
    void log_stats() const;

    // ============ 配置 ============

    /**
     * @brief 获取配置
     * @return 当前配置
     */
    const SummarizerConfig& get_config() const {
        return m_config;
    }

    /**
     * @brief 更新配置
     * @param config 新配置
     */
    void set_config(const SummarizerConfig& config) {
        m_config = config;
    }

private:
    /**
     * @brief 私有构造函数（单例模式）
     */
    Summarizer() = default;

    /**
     * @brief 析构函数
     */
    ~Summarizer() = default;

    /**
     * @brief 生成简洁摘要
     */
    std::string generate_concise_summary(const ConversationSegment& segment);

    /**
     * @brief 生成详细摘要
     */
    std::string generate_detailed_summary(const ConversationSegment& segment);

    /**
     * @brief 生成项目符号摘要
     */
    std::string generate_bullet_summary(const ConversationSegment& segment);

    /**
     * @brief 生成问答摘要
     */
    std::string generate_qa_summary(const ConversationSegment& segment);

    /**
     * @brief 生成时间线摘要
     */
    std::string generate_timeline_summary(const ConversationSegment& segment);

    /**
     * @brief 提取关键信息
     */
    std::vector<std::string> extract_key_points(const ConversationSegment& segment);

    /**
     * @brief 构建摘要缓存键
     */
    std::string build_cache_key(int64_t conversation_id, const std::string& content_hash);

    /**
     * @brief 计算内容哈希
     */
    std::string calculate_content_hash(const std::vector<ConversationMessage>& messages);

    /**
     * @brief 更新统计
     */
    void update_stats(const std::string& summary);

    /**
     * @brief 截断摘要到最大长度
     */
    std::string truncate_summary(const std::string& summary);

    // ============ 成员变量 ============

    SummarizerConfig m_config;                          ///< 配置
    SummaryTriggerCondition m_trigger_condition;       ///< 触发条件
    SummarizerStats m_stats;                           ///< 统计

    // 简单的内存缓存
    std::map<std::string, SummaryResult> m_cache;       ///< 摘要缓存

    bool m_initialized;                                 ///< 是否已初始化
};

} // namespace DearTs::Plugins::MemoryCore::Summarization
