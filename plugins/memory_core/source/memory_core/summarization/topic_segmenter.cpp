/**
 * @file topic_segmenter.cpp
 * @brief 主题分段器实现
 */

#include "memory_core/summarization/topic_segmenter.hpp"
#include "liblogger/logger.h"
#include <algorithm>
#include <sstream>
#include <set>
#include <map>
#include <cmath>
#include <cctype>

namespace DearTs::Plugins::MemoryCore::Summarization {

// ============ 单例实现 ============

TopicSegmenter& TopicSegmenter::instance() {
    static TopicSegmenter instance;
    return instance;
}

// ============ 初始化 ============

DearTs::Core::Result<void, std::string> TopicSegmenter::initialize(
    const TopicSegmenterConfig& config
) {
    if (m_initialized) {
        LOG_WARN("TopicSegmenter already initialized");
        return DearTs::Core::Result<void, std::string>::ok();
    }

    m_config = config;
    m_stats = SegmentationStats::empty();
    m_initialized = true;

    LOG_INFO("TopicSegmenter initialized: strategy={}, max_tokens={}, max_messages={}",
             static_cast<int>(config.strategy), config.max_tokens_per_segment,
             config.max_messages_per_segment);
    return DearTs::Core::Result<void, std::string>::ok();
}

void TopicSegmenter::shutdown() {
    if (!m_initialized) {
        return;
    }

    log_stats();
    LOG_INFO("TopicSegmenter shutdown");
    m_initialized = false;
}

// ============ 分段操作 ============

DearTs::Core::Result<std::vector<ConversationSegment>, std::string>
TopicSegmenter::segment(const std::vector<ConversationMessage>& messages) {
    if (!m_initialized) {
        return DearTs::Core::Result<std::vector<ConversationSegment>, std::string>::err(
            "TopicSegmenter not initialized"
        );
    }

    if (messages.empty()) {
        return DearTs::Core::Result<std::vector<ConversationSegment>, std::string>::ok(
            std::vector<ConversationSegment>{}
        );
    }

    LOG_INFO("Segmenting {} messages using strategy: {}",
             messages.size(), static_cast<int>(m_config.strategy));

    std::vector<ConversationSegment> segments;

    switch (m_config.strategy) {
        case SegmentationStrategy::TokenCount:
            segments = segment_by_token_count(messages);
            break;

        case SegmentationStrategy::MessageCount:
            segments = segment_by_message_count(messages);
            break;

        case SegmentationStrategy::TimeWindow:
            segments = segment_by_time_window(messages);
            break;

        case SegmentationStrategy::TopicChange:
            segments = segment_by_topic_change(messages);
            break;

        case SegmentationStrategy::Hybrid:
            segments = segment_hybrid(messages);
            break;

        default:
            segments = segment_by_message_count(messages);
            break;
    }

    // 合并小分段
    if (m_config.enable_merge_small_segments) {
        int before_merge = static_cast<int>(segments.size());
        segments = merge_small_segments(segments);
        int after_merge = static_cast<int>(segments.size());

        if (before_merge != after_merge) {
            m_stats.merged_segments = before_merge - after_merge;
            LOG_DEBUG("Merged {} small segments: {} -> {}",
                     m_stats.merged_segments, before_merge, after_merge);
        }
    }

    // 更新统计
    update_stats(segments);

    LOG_INFO("Segmented into {} segments", segments.size());
    return DearTs::Core::Result<std::vector<ConversationSegment>, std::string>::ok(segments);
}

bool TopicSegmenter::should_start_new_segment(
    const ConversationMessage& message,
    const ConversationSegment& current_segment
) {
    // 检查 token 限制
    size_t segment_tokens = current_segment.total_tokens + estimate_tokens(message.content);
    if (segment_tokens > static_cast<size_t>(m_config.max_tokens_per_segment)) {
        return true;
    }

    // 检查消息数量限制
    if (current_segment.message_count() >= m_config.max_messages_per_segment) {
        return true;
    }

    // 检查时间窗口
    int64_t time_diff = message.timestamp - current_segment.end_time;
    if (time_diff > m_config.max_time_window_seconds * 1000) {
        return true;
    }

    // 检查主题相似度
    if (!current_segment.messages.empty()) {
        double similarity = calculate_similarity(message, current_segment.messages.back());
        if (similarity < m_config.similarity_threshold) {
            return true;
        }
    }

    return false;
}

std::vector<ConversationSegment> TopicSegmenter::merge_small_segments(
    const std::vector<ConversationSegment>& segments
) {
    if (segments.size() <= 1) {
        return segments;
    }

    std::vector<ConversationSegment> merged;
    merged.reserve(segments.size());

    ConversationSegment current = segments[0];

    for (size_t i = 1; i < segments.size(); ++i) {
        // 检查是否应该合并（当前分段太小）
        if (current.message_count() < m_config.min_segment_size) {
            // 合并到当前分段
            for (const auto& msg : segments[i].messages) {
                current.messages.push_back(msg);
            }
            current.end_time = segments[i].end_time;
            current.total_tokens += segments[i].total_tokens;
        } else {
            // 完成当前分段
            merged.push_back(current);
            current = segments[i];
        }
    }

    // 添加最后一个分段
    merged.push_back(current);

    return merged;
}

// ============ 主题检测 ============

std::string TopicSegmenter::extract_topic_hint(const ConversationSegment& segment) {
    if (segment.messages.empty()) {
        return "";
    }

    // 提取所有关键词
    std::map<std::string, int> keyword_counts;

    for (const auto& msg : segment.messages) {
        if (msg.role == MessageRole::User) {
            auto keywords = extract_keywords(msg.content);
            for (const auto& kw : keywords) {
                keyword_counts[kw]++;
            }
        }
    }

    // 选择出现频率最高的关键词
    std::vector<std::pair<std::string, int>> sorted_keywords(
        keyword_counts.begin(), keyword_counts.end()
    );
    std::sort(sorted_keywords.begin(), sorted_keywords.end(),
        [](const auto& a, const auto& b) {
            return a.second > b.second;
        });

    // 构建主题提示（最多 3 个关键词）
    std::ostringstream oss;
    int count = 0;
    for (const auto& [keyword, freq] : sorted_keywords) {
        if (count >= 3) break;
        if (count > 0) oss << ", ";
        oss << keyword;
        count++;
    }

    return oss.str();
}

double TopicSegmenter::calculate_similarity(
    const ConversationMessage& msg1,
    const ConversationMessage& msg2
) {
    // 简单的关键词重叠相似度
    auto keywords1 = extract_keywords(msg1.content);
    auto keywords2 = extract_keywords(msg2.content);

    if (keywords1.empty() || keywords2.empty()) {
        return 0.0;
    }

    // 计算交集
    std::set<std::string> set1(keywords1.begin(), keywords1.end());
    std::set<std::string> set2(keywords2.begin(), keywords2.end());

    std::vector<std::string> intersection;
    std::set_intersection(set1.begin(), set1.end(),
                         set2.begin(), set2.end(),
                         std::back_inserter(intersection));

    // Jaccard 相似度
    std::set<std::string> union_set;
    union_set.insert(set1.begin(), set1.end());
    union_set.insert(set2.begin(), set2.end());

    if (union_set.empty()) {
        return 0.0;
    }

    return static_cast<double>(intersection.size()) / union_set.size();
}

std::vector<size_t> TopicSegmenter::detect_topic_changes(
    const std::vector<ConversationMessage>& messages
) {
    std::vector<size_t> change_points;

    if (messages.size() < 2) {
        return change_points;
    }

    // 计算相邻消息的相似度
    std::vector<double> similarities;
    for (size_t i = 1; i < messages.size(); ++i) {
        double sim = calculate_similarity(messages[i - 1], messages[i]);
        similarities.push_back(sim);
    }

    // 找出相似度显著下降的位置
    for (size_t i = 1; i < similarities.size(); ++i) {
        if (similarities[i] < m_config.similarity_threshold &&
            similarities[i - 1] >= m_config.similarity_threshold) {
            change_points.push_back(i + 1);
        }
    }

    LOG_DEBUG("Detected {} topic change points", change_points.size());
    return change_points;
}

// ============ 统计 ============

SegmentationStats TopicSegmenter::get_stats() const {
    return m_stats;
}

void TopicSegmenter::log_stats() const {
    LOG_INFO("TopicSegmenter Stats: segments={}, messages={}, tokens={}, "
             "avg_msgs/seg={:.1f}, avg_tokens/seg={:.1f}, merged={}",
             m_stats.total_segments, m_stats.total_messages, m_stats.total_tokens,
             m_stats.average_messages_per_segment, m_stats.average_tokens_per_segment,
             m_stats.merged_segments);
}

// ============ 私有辅助方法 ============

size_t TopicSegmenter::estimate_tokens(const std::string& text) const {
    // 简单估算：中文约 1.5 字符/token，英文约 4 字符/token
    size_t chinese_chars = 0;
    size_t other_chars = 0;

    for (char c : text) {
        if (static_cast<unsigned char>(c) > 127) {
            chinese_chars++;
        } else if (std::isalnum(c)) {
            other_chars++;
        }
    }

    return static_cast<size_t>(chinese_chars / 1.5 + other_chars / 4.0);
}

std::vector<ConversationSegment> TopicSegmenter::segment_by_token_count(
    const std::vector<ConversationMessage>& messages
) {
    std::vector<ConversationSegment> segments;
    ConversationSegment current;
    current.segment_id = 0;

    for (const auto& msg : messages) {
        size_t msg_tokens = estimate_tokens(msg.content);

        if (current.total_tokens + msg_tokens >
            static_cast<size_t>(m_config.max_tokens_per_segment) &&
            !current.messages.empty()) {
            // 完成当前分段
            current.topic_hint = extract_topic_hint(current);
            current.importance_score = calculate_importance(current);
            segments.push_back(current);

            // 开始新分段
            current = ConversationSegment();
            current.segment_id = static_cast<int>(segments.size());
        }

        // 添加消息
        current.messages.push_back(msg);
        current.total_tokens += msg_tokens;

        if (current.messages.empty()) {
            current.start_time = msg.timestamp;
        }
        current.end_time = msg.timestamp;
    }

    // 添加最后一个分段
    if (!current.messages.empty()) {
        current.topic_hint = extract_topic_hint(current);
        current.importance_score = calculate_importance(current);
        segments.push_back(current);
    }

    return segments;
}

std::vector<ConversationSegment> TopicSegmenter::segment_by_message_count(
    const std::vector<ConversationMessage>& messages
) {
    std::vector<ConversationSegment> segments;
    ConversationSegment current;
    current.segment_id = 0;

    for (size_t i = 0; i < messages.size(); ++i) {
        const auto& msg = messages[i];

        if (current.messages.size() >=
            static_cast<size_t>(m_config.max_messages_per_segment) &&
            !current.messages.empty()) {
            // 完成当前分段
            current.topic_hint = extract_topic_hint(current);
            current.importance_score = calculate_importance(current);
            segments.push_back(current);

            // 开始新分段
            current = ConversationSegment();
            current.segment_id = static_cast<int>(segments.size());
        }

        // 添加消息
        current.messages.push_back(msg);
        current.total_tokens += estimate_tokens(msg.content);

        if (current.messages.empty()) {
            current.start_time = msg.timestamp;
        }
        current.end_time = msg.timestamp;
    }

    // 添加最后一个分段
    if (!current.messages.empty()) {
        current.topic_hint = extract_topic_hint(current);
        current.importance_score = calculate_importance(current);
        segments.push_back(current);
    }

    return segments;
}

std::vector<ConversationSegment> TopicSegmenter::segment_by_time_window(
    const std::vector<ConversationMessage>& messages
) {
    std::vector<ConversationSegment> segments;
    ConversationSegment current;
    current.segment_id = 0;

    int64_t window_start = 0;

    for (const auto& msg : messages) {
        if (current.messages.empty()) {
            window_start = msg.timestamp;
        }

        int64_t time_diff = (msg.timestamp - window_start) / 1000; // 转换为秒

        if (time_diff > m_config.max_time_window_seconds &&
            !current.messages.empty()) {
            // 完成当前分段
            current.topic_hint = extract_topic_hint(current);
            current.importance_score = calculate_importance(current);
            segments.push_back(current);

            // 开始新分段
            current = ConversationSegment();
            current.segment_id = static_cast<int>(segments.size());
            window_start = msg.timestamp;
        }

        // 添加消息
        current.messages.push_back(msg);
        current.total_tokens += estimate_tokens(msg.content);

        if (current.messages.empty()) {
            current.start_time = msg.timestamp;
        }
        current.end_time = msg.timestamp;
    }

    // 添加最后一个分段
    if (!current.messages.empty()) {
        current.topic_hint = extract_topic_hint(current);
        current.importance_score = calculate_importance(current);
        segments.push_back(current);
    }

    return segments;
}

std::vector<ConversationSegment> TopicSegmenter::segment_by_topic_change(
    const std::vector<ConversationMessage>& messages
) {
    std::vector<ConversationSegment> segments;

    // 检测主题变化点
    auto change_points = detect_topic_changes(messages);

    // 按变化点分段
    size_t start = 0;
    int segment_id = 0;

    for (size_t change_point : change_points) {
        ConversationSegment segment;
        segment.segment_id = segment_id++;
        segment.messages.assign(messages.begin() + start,
                               messages.begin() + change_point);

        if (!segment.messages.empty()) {
            segment.start_time = segment.messages.front().timestamp;
            segment.end_time = segment.messages.back().timestamp;
            segment.total_tokens = 0;
            for (const auto& msg : segment.messages) {
                segment.total_tokens += estimate_tokens(msg.content);
            }
            segment.topic_hint = extract_topic_hint(segment);
            segment.importance_score = calculate_importance(segment);
            segments.push_back(segment);
        }

        start = change_point;
    }

    // 添加最后一个分段
    if (start < messages.size()) {
        ConversationSegment segment;
        segment.segment_id = segment_id;
        segment.messages.assign(messages.begin() + start, messages.end());

        if (!segment.messages.empty()) {
            segment.start_time = segment.messages.front().timestamp;
            segment.end_time = segment.messages.back().timestamp;
            segment.total_tokens = 0;
            for (const auto& msg : segment.messages) {
                segment.total_tokens += estimate_tokens(msg.content);
            }
            segment.topic_hint = extract_topic_hint(segment);
            segment.importance_score = calculate_importance(segment);
            segments.push_back(segment);
        }
    }

    return segments;
}

std::vector<ConversationSegment> TopicSegmenter::segment_hybrid(
    const std::vector<ConversationMessage>& messages
) {
    // 混合策略：综合多种条件
    std::vector<ConversationSegment> segments;
    ConversationSegment current;
    current.segment_id = 0;

    for (const auto& msg : messages) {
        // 检查是否需要开始新分段
        if (!current.messages.empty() && should_start_new_segment(msg, current)) {
            // 完成当前分段
            current.topic_hint = extract_topic_hint(current);
            current.importance_score = calculate_importance(current);
            segments.push_back(current);

            // 开始新分段
            current = ConversationSegment();
            current.segment_id = static_cast<int>(segments.size());
        }

        // 添加消息
        current.messages.push_back(msg);
        current.total_tokens += estimate_tokens(msg.content);

        if (current.messages.empty()) {
            current.start_time = msg.timestamp;
        }
        current.end_time = msg.timestamp;
    }

    // 添加最后一个分段
    if (!current.messages.empty()) {
        current.topic_hint = extract_topic_hint(current);
        current.importance_score = calculate_importance(current);
        segments.push_back(current);
    }

    return segments;
}

std::vector<std::string> TopicSegmenter::extract_keywords(const std::string& text) const {
    std::vector<std::string> keywords;
    std::set<std::string> seen;

    // 简单的关键词提取：提取中文词语和英文单词
    std::string current_word;
    bool in_chinese = false;

    for (char c : text) {
        if (static_cast<unsigned char>(c) > 127) {
            // 中文字符
            if (!in_chinese && !current_word.empty()) {
                // 完成英文单词
                if (current_word.length() >= 3) {
                    std::string lower = current_word;
                    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                    if (seen.find(lower) == seen.end()) {
                        keywords.push_back(lower);
                        seen.insert(lower);
                    }
                }
                current_word.clear();
            }
            in_chinese = true;
            current_word += c;

            // 单个中文字符作为关键词
            if (current_word.length() >= 1) {
                if (seen.find(current_word) == seen.end()) {
                    keywords.push_back(current_word);
                    seen.insert(current_word);
                }
                current_word.clear();
            }
        } else if (std::isalnum(c)) {
            // 英文字母或数字
            if (in_chinese && !current_word.empty()) {
                current_word.clear();
            }
            in_chinese = false;
            current_word += c;
        } else {
            // 分隔符
            if (!current_word.empty()) {
                if (current_word.length() >= 3) {
                    std::string lower = current_word;
                    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                    if (seen.find(lower) == seen.end()) {
                        keywords.push_back(lower);
                        seen.insert(lower);
                    }
                }
                current_word.clear();
            }
            in_chinese = false;
        }
    }

    // 处理最后一个词
    if (!current_word.empty() && current_word.length() >= 3) {
        std::string lower = current_word;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (seen.find(lower) == seen.end()) {
            keywords.push_back(lower);
        }
    }

    return keywords;
}

double TopicSegmenter::calculate_importance(const ConversationSegment& segment) {
    if (segment.messages.empty()) {
        return 0.0;
    }

    // 基于多个因素计算重要性
    double score = 0.0;

    // 1. 消息数量（0-0.3）
    double msg_score = std::min(1.0, segment.message_count() / 20.0) * 0.3;

    // 2. 时间跨度（0-0.2）
    double time_score = std::min(1.0, segment.duration_seconds() / 600.0) * 0.2;

    // 3. 用户消息比例（0-0.3）
    int user_msgs = 0;
    for (const auto& msg : segment.messages) {
        if (msg.role == MessageRole::User) {
            user_msgs++;
        }
    }
    double user_ratio = segment.messages.empty() ? 0.0
        : static_cast<double>(user_msgs) / segment.messages.size();
    double user_score = user_ratio * 0.3;

    // 4. 内容长度（0-0.2）
    double content_score = std::min(1.0, segment.total_tokens / 1000.0) * 0.2;

    score = msg_score + time_score + user_score + content_score;

    return std::min(1.0, score);
}

void TopicSegmenter::update_stats(const std::vector<ConversationSegment>& segments) {
    m_stats.total_segments = static_cast<int>(segments.size());
    m_stats.total_messages = 0;
    m_stats.total_tokens = 0;

    for (const auto& seg : segments) {
        m_stats.total_messages += static_cast<int>(seg.messages.size());
        m_stats.total_tokens += static_cast<int64_t>(seg.total_tokens);
    }

    if (m_stats.total_segments > 0) {
        m_stats.average_messages_per_segment =
            static_cast<double>(m_stats.total_messages) / m_stats.total_segments;
        m_stats.average_tokens_per_segment =
            static_cast<double>(m_stats.total_tokens) / m_stats.total_segments;
    }
}

} // namespace DearTs::Plugins::MemoryCore::Summarization
