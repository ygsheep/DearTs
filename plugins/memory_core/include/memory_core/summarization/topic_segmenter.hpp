/**
 * @file topic_segmenter.hpp
 * @brief 主题分段器 - 将对话按主题分段
 *
 * 功能：
 * - 主题变化检测
 * - 语义相似度分段
 * - 时间窗口分段
 * - 自适应分段
 */

#pragma once

#include "core/result.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <optional>
#include <cstdint>

namespace DearTs::Plugins::MemoryCore::Summarization {

/**
 * @brief 消息类型
 */
enum class MessageRole {
    User,                  ///< 用户消息
    Assistant,             ///< 助手消息
    System                 ///< 系统消息
};

/**
 * @brief 对话消息
 */
struct ConversationMessage {
    std::string content;                       ///< 消息内容
    MessageRole role;                          ///< 角色
    int64_t timestamp;                         ///< 时间戳
    std::optional<std::string> message_id;     ///< 消息 ID

    /**
     * @brief 获取消息长度
     */
    size_t length() const {
        return content.length();
    }
};

/**
 * @brief 对话分段
 */
struct ConversationSegment {
    int segment_id;                            ///< 分段 ID
    std::vector<ConversationMessage> messages; ///< 消息列表
    std::string topic_hint;                    ///< 主题提示
    int64_t start_time;                        ///< 开始时间
    int64_t end_time;                          ///< 结束时间
    size_t total_tokens;                       ///< 总 token 数（估算）
    double importance_score;                   ///< 重要性分数 [0-1]

    /**
     * @brief 获取消息数量
     */
    size_t message_count() const {
        return messages.size();
    }

    /**
     * @brief 获取持续时间（秒）
     */
    int64_t duration_seconds() const {
        return (end_time - start_time) / 1000;
    }
};

/**
 * @brief 分段策略
 */
enum class SegmentationStrategy {
    TokenCount,             ///< 按 token 数量
    MessageCount,           ///< 按消息数量
    TimeWindow,             ///< 按时间窗口
    TopicChange,            ///< 按主题变化
    Hybrid                  ///< 混合策略
};

/**
 * @brief 主题分段器配置
 */
struct TopicSegmenterConfig {
    SegmentationStrategy strategy;             ///< 分段策略
    int max_tokens_per_segment;                ///< 每段最大 token 数
    int max_messages_per_segment;              ///< 每段最大消息数
    int max_time_window_seconds;               ///< 最大时间窗口（秒）
    double similarity_threshold;               ///< 相似度阈值 [0-1]
    bool enable_merge_small_segments;          ///< 是否合并小分段
    int min_segment_size;                      ///< 最小分段大小

    /**
     * @brief 默认配置
     */
    static TopicSegmenterConfig default_config() {
        return TopicSegmenterConfig{
            .strategy = SegmentationStrategy::Hybrid,
            .max_tokens_per_segment = 2000,
            .max_messages_per_segment = 50,
            .max_time_window_seconds = 1800,  // 30 分钟
            .similarity_threshold = 0.6,
            .enable_merge_small_segments = true,
            .min_segment_size = 3
        };
    }

    /**
     * @brief 简洁配置（更小的分段）
     */
    static TopicSegmenterConfig concise_config() {
        return TopicSegmenterConfig{
            .strategy = SegmentationStrategy::TokenCount,
            .max_tokens_per_segment = 1000,
            .max_messages_per_segment = 20,
            .max_time_window_seconds = 600,   // 10 分钟
            .similarity_threshold = 0.7,
            .enable_merge_small_segments = true,
            .min_segment_size = 2
        };
    }

    /**
     * @brief 详细配置（更大的分段）
     */
    static TopicSegmenterConfig verbose_config() {
        return TopicSegmenterConfig{
            .strategy = SegmentationStrategy::Hybrid,
            .max_tokens_per_segment = 4000,
            .max_messages_per_segment = 100,
            .max_time_window_seconds = 3600,  // 1 小时
            .similarity_threshold = 0.5,
            .enable_merge_small_segments = false,
            .min_segment_size = 5
        };
    }
};

/**
 * @brief 分段统计
 */
struct SegmentationStats {
    int total_segments;                        ///< 总分段数
    int total_messages;                        ///< 总消息数
    int64_t total_tokens;                      ///< 总 token 数
    double average_messages_per_segment;       ///< 平均每段消息数
    double average_tokens_per_segment;         ///< 平均每段 token 数
    int merged_segments;                       ///< 合并的分段数

    /**
     * @brief 创建空统计
     */
    static SegmentationStats empty() {
        return SegmentationStats{
            .total_segments = 0,
            .total_messages = 0,
            .total_tokens = 0,
            .average_messages_per_segment = 0.0,
            .average_tokens_per_segment = 0.0,
            .merged_segments = 0
        };
    }
};

/**
 * @brief 主题分段器
 *
 * 负责将长对话按主题分段
 */
class TopicSegmenter {
public:
    /**
     * @brief 获取单例实例
     */
    static TopicSegmenter& instance();

    /**
     * @brief 删除拷贝构造和赋值
     */
    TopicSegmenter(const TopicSegmenter&) = delete;
    TopicSegmenter& operator=(const TopicSegmenter&) = delete;

    // ============ 初始化 ============

    /**
     * @brief 初始化分段器
     * @param config 分段器配置
     * @return 成功或错误信息
     */
    DearTs::Core::Result<void, std::string> initialize(
        const TopicSegmenterConfig& config = TopicSegmenterConfig::default_config()
    );

    /**
     * @brief 关闭分段器
     */
    void shutdown();

    // ============ 分段操作 ============

    /**
     * @brief 对对话进行分段
     * @param messages 消息列表
     * @return 分段结果或错误信息
     */
    DearTs::Core::Result<std::vector<ConversationSegment>, std::string> segment(
        const std::vector<ConversationMessage>& messages
    );

    /**
     * @brief 增量添加消息并检查是否需要分段
     * @param message 新消息
     * @param current_segment 当前分段
     * @return true 表示需要开始新分段
     */
    bool should_start_new_segment(
        const ConversationMessage& message,
        const ConversationSegment& current_segment
    );

    /**
     * @brief 合并小分段
     * @param segments 原始分段列表
     * @return 合并后的分段列表
     */
    std::vector<ConversationSegment> merge_small_segments(
        const std::vector<ConversationSegment>& segments
    );

    // ============ 主题检测 ============

    /**
     * @brief 提取主题提示
     * @param segment 对话分段
     * @return 主题提示文本
     */
    std::string extract_topic_hint(const ConversationSegment& segment);

    /**
     * @brief 计算两个消息的相似度
     * @param msg1 消息 1
     * @param msg2 消息 2
     * @return 相似度 [0-1]
     */
    double calculate_similarity(
        const ConversationMessage& msg1,
        const ConversationMessage& msg2
    );

    /**
     * @brief 检测主题变化
     * @param messages 消息列表
     * @return 主题变化的索引位置列表
     */
    std::vector<size_t> detect_topic_changes(
        const std::vector<ConversationMessage>& messages
    );

    // ============ 统计 ============

    /**
     * @brief 获取统计信息
     * @return 统计数据
     */
    SegmentationStats get_stats() const;

    /**
     * @brief 打印统计信息
     */
    void log_stats() const;

    // ============ 配置 ============

    /**
     * @brief 获取配置
     * @return 当前配置
     */
    const TopicSegmenterConfig& get_config() const {
        return m_config;
    }

    /**
     * @brief 更新配置
     * @param config 新配置
     */
    void set_config(const TopicSegmenterConfig& config) {
        m_config = config;
    }

private:
    /**
     * @brief 私有构造函数（单例模式）
     */
    TopicSegmenter() = default;

    /**
     * @brief 析构函数
     */
    ~TopicSegmenter() = default;

    /**
     * @brief 估算 token 数量
     * @param text 文本
     * @return 估算的 token 数量
     */
    size_t estimate_tokens(const std::string& text) const;

    /**
     * @brief 按 token 数量分段
     */
    std::vector<ConversationSegment> segment_by_token_count(
        const std::vector<ConversationMessage>& messages
    );

    /**
     * @brief 按消息数量分段
     */
    std::vector<ConversationSegment> segment_by_message_count(
        const std::vector<ConversationMessage>& messages
    );

    /**
     * @brief 按时间窗口分段
     */
    std::vector<ConversationSegment> segment_by_time_window(
        const std::vector<ConversationMessage>& messages
    );

    /**
     * @brief 按主题变化分段
     */
    std::vector<ConversationSegment> segment_by_topic_change(
        const std::vector<ConversationMessage>& messages
    );

    /**
     * @brief 混合策略分段
     */
    std::vector<ConversationSegment> segment_hybrid(
        const std::vector<ConversationMessage>& messages
    );

    /**
     * @brief 提取关键词
     */
    std::vector<std::string> extract_keywords(const std::string& text) const;

    /**
     * @brief 计算重要性分数
     */
    double calculate_importance(const ConversationSegment& segment);

    /**
     * @brief 更新统计
     */
    void update_stats(const std::vector<ConversationSegment>& segments);

    // ============ 成员变量 ============

    TopicSegmenterConfig m_config;                      ///< 配置
    SegmentationStats m_stats;                          ///< 统计
    bool m_initialized;                                 ///< 是否已初始化
};

} // namespace DearTs::Plugins::MemoryCore::Summarization
