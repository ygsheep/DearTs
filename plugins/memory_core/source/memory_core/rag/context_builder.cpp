/**
 * @file context_builder.cpp
 * @brief 上下文构建器实现
 */

#include "memory_core/rag/context_builder.hpp"
#include "liblogger/logger.h"
#include <sstream>
#include <algorithm>
#include <set>
#include <map>

namespace DearTs::Plugins::MemoryCore::RAG {

// ============ 内置模板定义 ============

const char* ContextBuilder::DEFAULT_TEMPLATE =
    "基于以下相关记忆回答问题：\n\n"
    "{memories}\n\n"
    "问题：{query}";

const char* ContextBuilder::CONCISE_TEMPLATE =
    "相关记忆：{memories}\n问题：{query}";

const char* ContextBuilder::VERBOSE_TEMPLATE =
    "===== 相关记忆 =====\n\n"
    "{memories}\n\n"
    "===== 用户问题 =====\n"
    "{query}\n\n"
    "请根据以上记忆内容，提供准确和有用的回答。";

// ============ 单例实现 ============

ContextBuilder& ContextBuilder::instance() {
    static ContextBuilder instance;
    return instance;
}

// ============ 上下文构建 ============

DearTs::Core::Result<std::string, std::string>
ContextBuilder::build_from_rag_results(
    const std::vector<RAGResult>& results,
    const ContextBuildOptions& options
) {
    std::ostringstream context;

    // 去重（如果启用）
    std::vector<RAGResult> processed_results = results;
    if (options.compress_duplicates) {
        processed_results = deduplicate_results(results);
    }

    // 排序（如果启用）
    if (options.sort_by_relevance) {
        processed_results = sort_by_relevance(std::move(processed_results));
    }

    // 限制记忆数量
    if (processed_results.size() > static_cast<size_t>(options.max_memories)) {
        processed_results.resize(options.max_memories);
    }

    // 构建上下文
    int current_length = 0;
    for (const auto& result : processed_results) {
        std::string memory_text = format_memory(
            result.memory,
            result.combined_score,
            options
        );

        // 检查长度限制
        if (current_length + memory_text.length() > static_cast<size_t>(options.max_length)) {
            break;
        }

        context << memory_text << "\n";
        current_length += memory_text.length() + 1;
    }

    LOG_INFO("Built context from RAG results: {} chars, {} memories",
             current_length, processed_results.size());

    return DearTs::Core::Result<std::string, std::string>::ok(context.str());
}

DearTs::Core::Result<std::string, std::string>
ContextBuilder::build_from_memories(
    const std::vector<Memory::Memory>& memories,
    const ContextBuildOptions& options
) {
    // 将 Memory 转换为 RAGResult
    std::vector<RAGResult> results;
    for (const auto& memory : memories) {
        RAGResult result;
        result.memory = memory;
        result.similarity = memory.importance;  // 使用重要性作为相似度
        result.keyword_score = 0.5;
        result.calculate_combined_score();
        results.push_back(result);
    }

    return build_from_rag_results(results, options);
}

DearTs::Core::Result<std::string, std::string>
ContextBuilder::build_full_context(
    const std::string& query,
    const std::vector<RAGResult>& results,
    const ContextBuildOptions& options
) {
    // 构建记忆部分
    auto memories_result = build_from_rag_results(results, options);
    if (memories_result.isErr()) {
        return DearTs::Core::Result<std::string, std::string>::err(memories_result.error());
    }

    // 构建完整上下文
    std::ostringstream full_context;
    full_context << memories_result.unwrap() << "\n";
    full_context << "当前问题：" << query;

    return DearTs::Core::Result<std::string, std::string>::ok(full_context.str());
}

// ============ 上下文模板 ============

void ContextBuilder::set_template(const std::string& template_name, const std::string& template_content) {
    m_templates[template_name] = template_content;
    LOG_INFO("Set context template: {}", template_name);
}

DearTs::Core::Result<std::string, std::string>
ContextBuilder::build_with_template(
    const std::string& template_name,
    const std::string& query,
    const std::vector<RAGResult>& results,
    const ContextBuildOptions& options
) {
    // 获取模板
    std::string template_content;

    if (template_name == "default") {
        template_content = DEFAULT_TEMPLATE;
    } else if (template_name == "concise") {
        template_content = CONCISE_TEMPLATE;
    } else if (template_name == "verbose") {
        template_content = VERBOSE_TEMPLATE;
    } else {
        auto it = m_templates.find(template_name);
        if (it != m_templates.end()) {
            template_content = it->second;
        } else {
            return DearTs::Core::Result<std::string, std::string>::err(
                "Template not found: " + template_name
            );
        }
    }

    // 构建记忆部分
    auto memories_result = build_from_rag_results(results, options);
    if (memories_result.isErr()) {
        return DearTs::Core::Result<std::string, std::string>::err(memories_result.error());
    }

    // 填充模板
    std::string context = template_content;
    size_t pos;

    // 替换 {query}
    pos = context.find("{query}");
    if (pos != std::string::npos) {
        context.replace(pos, 7, query);
    }

    // 替换 {memories}
    pos = context.find("{memories}");
    if (pos != std::string::npos) {
        context.replace(pos, 10, memories_result.unwrap());
    }

    // 替换 {context}（完整上下文）
    pos = context.find("{context}");
    if (pos != std::string::npos) {
        std::string full_context = build_full_context(query, results, options).unwrap();
        context.replace(pos, 9, full_context);
    }

    LOG_INFO("Built context with template: {}", template_name);
    return DearTs::Core::Result<std::string, std::string>::ok(context);
}

// ============ 高级功能 ============

std::string ContextBuilder::compress_context(const std::string& context, int max_length) {
    if (static_cast<int>(context.length()) <= max_length) {
        return context;
    }

    // 简单截断（可以改进为智能截断）
    return truncate_to_length(context, max_length);
}

std::vector<RAGResult> ContextBuilder::extract_key_memories(
    const std::vector<RAGResult>& results,
    size_t count
) {
    // 按重要性排序并提取前 N 个
    auto sorted = sort_by_relevance(results);

    if (sorted.size() > count) {
        sorted.resize(count);
    }

    LOG_INFO("Extracted {} key memories", sorted.size());
    return sorted;
}

std::vector<std::pair<std::string, std::vector<RAGResult>>>
ContextBuilder::group_by_time(const std::vector<RAGResult>& results) {
    std::map<std::string, std::vector<RAGResult>> grouped;

    for (const auto& result : results) {
        // 按时间分组（今天、本周、更早）
        std::string time_group;

        auto now = std::chrono::system_clock::now();
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()
        ).count();

        int64_t diff_hours = (now_ms - result.memory.created_at) / (1000 * 60 * 60);

        if (diff_hours < 24) {
            time_group = "今天";
        } else if (diff_hours < 24 * 7) {
            time_group = "本周";
        } else if (diff_hours < 24 * 30) {
            time_group = "本月";
        } else {
            time_group = "更早";
        }

        grouped[time_group].push_back(result);
    }

    // 转换为 vector
    std::vector<std::pair<std::string, std::vector<RAGResult>>> result;
    for (auto it = grouped.begin(); it != grouped.end(); ++it) {
        result.push_back({it->first, it->second});
    }

    return result;
}

// ============ 私有辅助方法 ============

std::string ContextBuilder::format_memory(
    const Memory::Memory& memory,
    double relevance_score,
    const ContextBuildOptions& options
) {
    std::ostringstream oss;

    // 类型标签
    oss << "[" << Memory::Memory::type_to_string(memory.type) << "] ";

    // 内容
    oss << memory.content;

    // 重要性（如果启用）
    if (options.include_importance && memory.importance > 0.5) {
        oss << " (重要性: " << static_cast<int>(memory.importance * 100) << "%";
        if (relevance_score >= 0) {
            oss << ", 相关性: " << static_cast<int>(relevance_score * 100) << "%";
        }
        oss << ")";
    }

    // 来源（如果启用）
    if (options.include_source && memory.source_conversation_id.has_value()) {
        oss << " [来自: " << memory.source_conversation_id.value() << "]";
    }

    return oss.str();
}

std::vector<RAGResult> ContextBuilder::deduplicate_results(const std::vector<RAGResult>& results) {
    std::vector<RAGResult> deduplicated;
    std::set<std::string> seen_contents;

    for (const auto& result : results) {
        // 使用内容作为去重键
        if (seen_contents.find(result.memory.content) == seen_contents.end()) {
            deduplicated.push_back(result);
            seen_contents.insert(result.memory.content);
        }
    }

    if (deduplicated.size() != results.size()) {
        LOG_INFO("Deduplicated: {} -> {} results", results.size(), deduplicated.size());
    }

    return deduplicated;
}

std::vector<RAGResult> ContextBuilder::sort_by_relevance(std::vector<RAGResult> results) {
    std::sort(results.begin(), results.end(),
        [](const RAGResult& a, const RAGResult& b) {
            return a.combined_score > b.combined_score;
        });

    return results;
}

std::string ContextBuilder::truncate_to_length(const std::string& text, int max_length) {
    if (static_cast<int>(text.length()) <= max_length) {
        return text;
    }

    // 尝试在句子边界截断
    std::string result = text.substr(0, max_length - 3);

    // 查找最后的句子结束符
    size_t last_period = result.rfind('。');
    if (last_period != std::string::npos && last_period > max_length / 2) {
        result = result.substr(0, last_period + 1);
    } else {
        size_t last_space = result.rfind(' ');
        if (last_space != std::string::npos && last_space > max_length / 2) {
            result = result.substr(0, last_space);
        }
    }

    return result + "...";
}

} // namespace DearTs::Plugins::MemoryCore::RAG
