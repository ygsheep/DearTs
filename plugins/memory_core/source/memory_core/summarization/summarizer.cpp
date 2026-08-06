/**
 * @file summarizer.cpp
 * @brief 摘要生成器实现
 */

#include "memory_core/summarization/summarizer.hpp"
#include "liblogger/logger.h"
#include <algorithm>
#include <sstream>
#include <set>
#include <map>
#include <regex>
#include <chrono>

namespace DearTs::Plugins::MemoryCore::Summarization {

// ============ 单例实现 ============

Summarizer& Summarizer::instance() {
    static Summarizer instance;
    return instance;
}

// ============ 初始化 ============

DearTs::Core::Result<void, std::string> Summarizer::initialize(
    const SummarizerConfig& config
) {
    if (m_initialized) {
        LOG_WARN("Summarizer already initialized");
        return DearTs::Core::Result<void, std::string>::ok();
    }

    m_config = config;
    m_trigger_condition = SummaryTriggerCondition::default_condition();
    m_stats = SummarizerStats::empty();
    m_cache.clear();
    m_initialized = true;

    LOG_INFO("Summarizer initialized: style={}, max_length={}, cache={}",
             static_cast<int>(config.default_style), config.max_summary_length,
             config.enable_cache);
    return DearTs::Core::Result<void, std::string>::ok();
}

void Summarizer::shutdown() {
    if (!m_initialized) {
        return;
    }

    log_stats();
    m_cache.clear();
    LOG_INFO("Summarizer shutdown");
    m_initialized = false;
}

// ============ 摘要生成 ============

DearTs::Core::Result<SummaryResult, std::string> Summarizer::generate_summary(
    int64_t conversation_id,
    const std::vector<ConversationMessage>& messages,
    SummaryStyle style,
    SummaryProgressCallback progress_callback
) {
    if (!m_initialized) {
        return DearTs::Core::Result<SummaryResult, std::string>::err(
            "Summarizer not initialized"
        );
    }

    if (messages.empty()) {
        return DearTs::Core::Result<SummaryResult, std::string>::err(
            "No messages to summarize"
        );
    }

    LOG_INFO("Generating summary for conversation {}: {} messages, style={}",
             conversation_id, messages.size(), static_cast<int>(style));

    // 报告进度
    if (progress_callback) {
        progress_callback("preparing", 0.0, "Preparing to generate summary");
    }

    // 检查缓存
    if (m_config.enable_cache) {
        std::string content_hash = calculate_content_hash(messages);
        std::string cache_key = build_cache_key(conversation_id, content_hash);

        auto it = m_cache.find(cache_key);
        if (it != m_cache.end()) {
            m_stats.cache_hits++;
            LOG_DEBUG("Summary cache hit for conversation {}", conversation_id);
            return DearTs::Core::Result<SummaryResult, std::string>::ok(it->second);
        }

        m_stats.cache_misses++;
    }

    // 创建临时分段用于生成摘要
    ConversationSegment segment;
    segment.segment_id = 0;
    segment.messages = messages;
    segment.start_time = messages.front().timestamp;
    segment.end_time = messages.back().timestamp;
    segment.total_tokens = 0;

    for (const auto& msg : messages) {
        // 简单估算 token 数
        segment.total_tokens += msg.content.length() / 2; // 粗略估算
    }

    segment.topic_hint = TopicSegmenter::instance().extract_topic_hint(segment);
    segment.importance_score = 0.5;

    // 生成摘要
    std::string summary;
    switch (style) {
        case SummaryStyle::Concise:
            summary = generate_concise_summary(segment);
            break;
        case SummaryStyle::Detailed:
            summary = generate_detailed_summary(segment);
            break;
        case SummaryStyle::BulletPoints:
            summary = generate_bullet_summary(segment);
            break;
        case SummaryStyle::QuestionAnswer:
            summary = generate_qa_summary(segment);
            break;
        case SummaryStyle::Timeline:
            summary = generate_timeline_summary(segment);
            break;
        default:
            summary = generate_concise_summary(segment);
            break;
    }

    // 截断到最大长度
    summary = truncate_summary(summary);

    // 构建结果
    SummaryResult result;
    result.summary = summary;
    result.topic_hint = segment.topic_hint;
    result.conversation_id = conversation_id;
    result.start_time = segment.start_time;
    result.end_time = segment.end_time;
    result.message_count = static_cast<int>(messages.size());
    result.importance_score = segment.importance_score;
    result.style = style;

    // 缓存结果
    if (m_config.enable_cache) {
        std::string content_hash = calculate_content_hash(messages);
        std::string cache_key = build_cache_key(conversation_id, content_hash);

        // 限制缓存大小
        if (static_cast<int>(m_cache.size()) >= m_config.cache_size) {
            // 移除最旧的条目（简单的 FIFO）
            auto first = m_cache.begin();
            m_cache.erase(first);
        }

        m_cache[cache_key] = result;
    }

    update_stats(summary);

    if (progress_callback) {
        progress_callback("completed", 1.0, "Summary generated successfully");
    }

    LOG_INFO("Summary generated: {} characters", summary.length());
    return DearTs::Core::Result<SummaryResult, std::string>::ok(result);
}

DearTs::Core::Result<std::vector<SummaryResult>, std::string>
Summarizer::generate_summaries_for_segments(
    int64_t conversation_id,
    const std::vector<ConversationSegment>& segments,
    SummaryStyle style
) {
    std::vector<SummaryResult> results;

    for (const auto& segment : segments) {
        SummaryResult result;
        result.conversation_id = conversation_id;
        result.start_time = segment.start_time;
        result.end_time = segment.end_time;
        result.message_count = static_cast<int>(segment.messages.size());
        result.importance_score = segment.importance_score;
        result.topic_hint = segment.topic_hint;
        result.style = style;

        // 生成摘要
        switch (style) {
            case SummaryStyle::Concise:
                result.summary = generate_concise_summary(segment);
                break;
            case SummaryStyle::Detailed:
                result.summary = generate_detailed_summary(segment);
                break;
            case SummaryStyle::BulletPoints:
                result.summary = generate_bullet_summary(segment);
                break;
            case SummaryStyle::QuestionAnswer:
                result.summary = generate_qa_summary(segment);
                break;
            case SummaryStyle::Timeline:
                result.summary = generate_timeline_summary(segment);
                break;
            default:
                result.summary = generate_concise_summary(segment);
                break;
        }

        result.summary = truncate_summary(result.summary);
        results.push_back(result);
    }

    LOG_INFO("Generated {} summaries for conversation {}", results.size(), conversation_id);
    return DearTs::Core::Result<std::vector<SummaryResult>, std::string>::ok(results);
}

DearTs::Core::Result<SummaryResult, std::string> Summarizer::update_summary(
    const SummaryResult& existing_summary,
    const std::vector<ConversationMessage>& new_messages
) {
    if (new_messages.empty()) {
        return DearTs::Core::Result<SummaryResult, std::string>::ok(existing_summary);
    }

    // 简单实现：将新摘要附加到现有摘要
    std::ostringstream oss;
    oss << existing_summary.summary << "\n\n";

    // 提取新消息的关键点
    ConversationSegment new_segment;
    new_segment.messages = new_messages;

    auto key_points = extract_key_points(new_segment);
    if (!key_points.empty()) {
        oss << "新增内容：\n";
        for (const auto& point : key_points) {
            oss << "- " << point << "\n";
        }
    }

    SummaryResult updated = existing_summary;
    updated.summary = oss.str();
    updated.end_time = new_messages.back().timestamp;
    updated.message_count += static_cast<int>(new_messages.size());

    LOG_INFO("Summary updated: {} -> {} messages",
             existing_summary.message_count, updated.message_count);

    return DearTs::Core::Result<SummaryResult, std::string>::ok(updated);
}

// ============ 触发检测 ============

bool Summarizer::should_trigger_summary(
    int64_t conversation_id,
    const std::vector<ConversationMessage>& messages,
    const SummaryTriggerCondition& condition
) {
    if (!condition.enable_auto_trigger) {
        return false;
    }

    if (messages.empty()) {
        return false;
    }

    // 检查消息数量
    if (static_cast<int>(messages.size()) >= condition.message_threshold) {
        LOG_DEBUG("Summary triggered: message count {} >= {}",
                 messages.size(), condition.message_threshold);
        return true;
    }

    // 检查时间跨度
    int64_t time_span = (messages.back().timestamp - messages.front().timestamp) / 1000;
    if (time_span >= condition.time_threshold_seconds) {
        LOG_DEBUG("Summary triggered: time span {} >= {} seconds",
                 time_span, condition.time_threshold_seconds);
        return true;
    }

    // TODO: 检查主题变化

    return false;
}

// ============ 缓存管理 ============

void Summarizer::clear_cache() {
    int size = static_cast<int>(m_cache.size());
    m_cache.clear();
    LOG_INFO("Cleared summary cache: {} entries removed", size);
}

double Summarizer::get_cache_hit_rate() const {
    int total = m_stats.cache_hits + m_stats.cache_misses;
    if (total == 0) {
        return 0.0;
    }
    return static_cast<double>(m_stats.cache_hits) / total;
}

// ============ 统计 ============

SummarizerStats Summarizer::get_stats() const {
    return m_stats;
}

void Summarizer::log_stats() const {
    double hit_rate = get_cache_hit_rate();
    LOG_INFO("Summarizer Stats: summaries={}, total_chars={}, avg_len={:.1f}, "
             "cache_hit_rate={:.2f}%",
             m_stats.total_summaries, m_stats.total_characters,
             m_stats.average_summary_length, hit_rate * 100);
}

// ============ 私有辅助方法 ============

std::string Summarizer::generate_concise_summary(const ConversationSegment& segment) {
    std::ostringstream oss;

    // 主题
    if (!segment.topic_hint.empty()) {
        oss << "主题: " << segment.topic_hint << "\n";
    }

    // 关键点
    auto key_points = extract_key_points(segment);
    if (!key_points.empty()) {
        oss << "要点: ";
        for (size_t i = 0; i < key_points.size() && i < 3; ++i) {
            if (i > 0) oss << "; ";
            oss << key_points[i];
        }
        oss << "\n";
    }

    // 统计
    oss << "包含 " << segment.message_count() << " 条消息";
    if (segment.duration_seconds() > 0) {
        oss << "，历时 " << (segment.duration_seconds() / 60) << " 分钟";
    }
    oss << "。";

    return oss.str();
}

std::string Summarizer::generate_detailed_summary(const ConversationSegment& segment) {
    std::ostringstream oss;

    oss << "## 对话摘要\n\n";

    // 基本信息
    oss << "**时间**: ";
    if (m_config.include_timestamps) {
        oss << segment.start_time << " - " << segment.end_time;
    } else {
        oss << segment.duration_seconds() / 60 << " 分钟";
    }
    oss << "\n";

    oss << "**消息数**: " << segment.message_count() << "\n";

    if (!segment.topic_hint.empty()) {
        oss << "**主题**: " << segment.topic_hint << "\n";
    }

    oss << "\n### 主要内容\n\n";

    // 提取并展示关键点
    auto key_points = extract_key_points(segment);
    for (const auto& point : key_points) {
        oss << "- " << point << "\n";
    }

    return oss.str();
}

std::string Summarizer::generate_bullet_summary(const ConversationSegment& segment) {
    std::ostringstream oss;

    auto key_points = extract_key_points(segment);

    for (const auto& point : key_points) {
        oss << "• " << point << "\n";
    }

    return oss.str();
}

std::string Summarizer::generate_qa_summary(const ConversationSegment& segment) {
    std::ostringstream oss;

    // 提取问答对
    std::vector<std::pair<std::string, std::string>> qa_pairs;

    for (size_t i = 0; i < segment.messages.size(); ++i) {
        if (segment.messages[i].role == MessageRole::User) {
            std::string question = segment.messages[i].content;
            std::string answer;

            // 查找下一个助手回复
            for (size_t j = i + 1; j < segment.messages.size(); ++j) {
                if (segment.messages[j].role == MessageRole::Assistant) {
                    answer = segment.messages[j].content;
                    break;
                }
            }

            if (!answer.empty()) {
                // 截断长内容
                if (question.length() > 50) {
                    question = question.substr(0, 50) + "...";
                }
                if (answer.length() > 100) {
                    answer = answer.substr(0, 100) + "...";
                }

                qa_pairs.push_back({question, answer});
            }
        }
    }

    // 输出问答对
    for (const auto& [q, a] : qa_pairs) {
        oss << "Q: " << q << "\n";
        oss << "A: " << a << "\n\n";
    }

    return oss.str();
}

std::string Summarizer::generate_timeline_summary(const ConversationSegment& segment) {
    std::ostringstream oss;

    oss << "## 时间线\n\n";

    for (const auto& msg : segment.messages) {
        const char* role = (msg.role == MessageRole::User) ? "用户" : "助手";

        if (m_config.include_timestamps) {
            oss << "[" << msg.timestamp << "] ";
        }

        oss << "**" << role << "**: ";

        // 截断内容
        std::string content = msg.content;
        if (content.length() > 80) {
            content = content.substr(0, 80) + "...";
        }

        oss << content << "\n\n";
    }

    return oss.str();
}

std::vector<std::string> Summarizer::extract_key_points(const ConversationSegment& segment) {
    std::vector<std::string> key_points;
    std::set<std::string> seen;

    // 简单的关键点提取：提取用户消息中的关键句
    for (const auto& msg : segment.messages) {
        if (msg.role != MessageRole::User) {
            continue;
        }

        // 按句子分割
        std::vector<std::string> sentences;
        std::string current;
        for (char c : msg.content) {
            current += c;
            if (c == '。' || c == '？' || c == '！' || c == '.') {
                if (current.length() > 5) {
                    sentences.push_back(current);
                }
                current.clear();
            }
        }

        if (!current.empty() && current.length() > 5) {
            sentences.push_back(current);
        }

        // 选择长句子作为关键点
        for (const auto& sentence : sentences) {
            if (sentence.length() >= 10 && sentence.length() <= 100) {
                std::string trimmed = sentence;
                // 去除首尾空格
                size_t start = trimmed.find_first_not_of(" \t\n\r");
                size_t end = trimmed.find_last_not_of(" \t\n\r");
                if (start != std::string::npos && end != std::string::npos) {
                    trimmed = trimmed.substr(start, end - start + 1);
                }

                if (seen.find(trimmed) == seen.end()) {
                    key_points.push_back(trimmed);
                    seen.insert(trimmed);

                    if (key_points.size() >= 5) {
                        break;
                    }
                }
            }
        }

        if (key_points.size() >= 5) {
            break;
        }
    }

    return key_points;
}

std::string Summarizer::build_cache_key(int64_t conversation_id, const std::string& content_hash) {
    std::ostringstream oss;
    oss << conversation_id << ":" << content_hash;
    return oss.str();
}

std::string Summarizer::calculate_content_hash(const std::vector<ConversationMessage>& messages) {
    // 简单哈希：基于消息数量和首尾内容
    std::ostringstream oss;
    oss << messages.size() << ":";
    if (!messages.empty()) {
        oss << messages.front().content.substr(0, 100) << ":";
        oss << messages.back().content.substr(0, 100);
    }
    return oss.str();
}

void Summarizer::update_stats(const std::string& summary) {
    m_stats.total_summaries++;
    m_stats.total_characters += static_cast<int64_t>(summary.length());
    m_stats.average_summary_length =
        static_cast<double>(m_stats.total_characters) / m_stats.total_summaries;
}

std::string Summarizer::truncate_summary(const std::string& summary) {
    if (static_cast<int>(summary.length()) <= m_config.max_summary_length) {
        return summary;
    }

    // 在句子边界截断
    std::string result = summary.substr(0, m_config.max_summary_length - 3);

    // 查找最后的句子结束符
    size_t last_period = result.rfind('。');
    if (last_period != std::string::npos && last_period > m_config.max_summary_length / 2) {
        result = result.substr(0, last_period + 1);
    } else {
        size_t last_newline = result.rfind('\n');
        if (last_newline != std::string::npos && last_newline > m_config.max_summary_length / 2) {
            result = result.substr(0, last_newline);
        }
    }

    return result + "...";
}

// ============ SummaryResult 序列化 ============

std::string SummaryResult::to_json() const {
    std::ostringstream oss;
    oss << "{"
        << "\"summary\":\"" << summary << "\","
        << "\"topic_hint\":\"" << topic_hint << "\","
        << "\"conversation_id\":" << conversation_id << ","
        << "\"start_time\":" << start_time << ","
        << "\"end_time\":" << end_time << ","
        << "\"message_count\":" << message_count << ","
        << "\"importance_score\":" << importance_score << ","
        << "\"style\":" << static_cast<int>(style)
        << "}";
    return oss.str();
}

DearTs::Core::Result<SummaryResult, std::string>
SummaryResult::from_json(const std::string& json) {
    // 简化实现
    SummaryResult result;
    // TODO: 完整的 JSON 解析
    return DearTs::Core::Result<SummaryResult, std::string>::ok(result);
}

} // namespace DearTs::Plugins::MemoryCore::Summarization
