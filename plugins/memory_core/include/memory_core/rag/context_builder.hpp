/**
 * @file context_builder.hpp
 * @brief 上下文构建器 - 为 RAG 构建优化的上下文
 *
 * 功能：
 * - 智能上下文组装
 * - 长度限制管理
 * - 重要性排序
 * - 去重和压缩
 */

#pragma once

#include "core/result.h"
#include "memory_core/rag/rag_service.hpp"
#include "memory_core/memory/memory_manager.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace DearTs::Plugins::MemoryCore::RAG {

/**
 * @brief 上下文构建选项
 */
struct ContextBuildOptions {
    int max_length;                     ///< 最大上下文长度（字符）
    int max_memories;                   ///< 最大记忆数量
    bool include_importance;            ///< 是否包含重要性信息
    bool include_source;                ///< 是否包含来源信息
    bool compress_duplicates;           ///< 是否压缩重复内容
    bool sort_by_relevance;             ///< 是否按相关性排序

    /**
     * @brief 默认选项
     */
    static ContextBuildOptions default_options() {
        return ContextBuildOptions{
            .max_length = 2000,
            .max_memories = 10,
            .include_importance = true,
            .include_source = false,
            .compress_duplicates = true,
            .sort_by_relevance = true
        };
    }

    /**
     * @brief 简洁选项（更短的上下文）
     */
    static ContextBuildOptions concise_options() {
        return ContextBuildOptions{
            .max_length = 1000,
            .max_memories = 5,
            .include_importance = false,
            .include_source = false,
            .compress_duplicates = true,
            .sort_by_relevance = true
        };
    }

    /**
     * @brief 详细选项（更长的上下文）
     */
    static ContextBuildOptions verbose_options() {
        return ContextBuildOptions{
            .max_length = 4000,
            .max_memories = 20,
            .include_importance = true,
            .include_source = true,
            .compress_duplicates = true,
            .sort_by_relevance = true
        };
    }
};

/**
 * @brief 上下文构建器
 *
 * 负责构建优化的 RAG 上下文
 */
class ContextBuilder {
public:
    /**
     * @brief 获取单例实例
     */
    static ContextBuilder& instance();

    /**
     * @brief 删除拷贝构造和赋值
     */
    ContextBuilder(const ContextBuilder&) = delete;
    ContextBuilder& operator=(const ContextBuilder&) = delete;

    // ============ 上下文构建 ============

    /**
     * @brief 从 RAG 结果构建上下文
     * @param results RAG 查询结果
     * @param options 构建选项
     * @return 构建的上下文字符串
     */
    DearTs::Core::Result<std::string, std::string> build_from_rag_results(
        const std::vector<RAGResult>& results,
        const ContextBuildOptions& options = ContextBuildOptions::default_options()
    );

    /**
     * @brief 从记忆列表构建上下文
     * @param memories 记忆列表
     * @param options 构建选项
     * @return 构建的上下文字符串
     */
    DearTs::Core::Result<std::string, std::string> build_from_memories(
        const std::vector<Memory::Memory>& memories,
        const ContextBuildOptions& options = ContextBuildOptions::default_options()
    );

    /**
     * @brief 为查询构建完整上下文（包含查询本身）
     * @param query 用户查询
     * @param results RAG 查询结果
     * @param options 构建选项
     * @return 完整的上下文字符串
     */
    DearTs::Core::Result<std::string, std::string> build_full_context(
        const std::string& query,
        const std::vector<RAGResult>& results,
        const ContextBuildOptions& options = ContextBuildOptions::default_options()
    );

    // ============ 上下文模板 ============

    /**
     * @brief 设置上下文模板
     * @param template_name 模板名称
     * @param template_content 模板内容
     * 支持占位符：{query}, {memories}, {context}
     */
    void set_template(const std::string& template_name, const std::string& template_content);

    /**
     * @brief 使用模板构建上下文
     * @param template_name 模板名称
     * @param query 查询
     * @param results RAG 结果
     * @param options 构建选项
     * @return 填充后的上下文
     */
    DearTs::Core::Result<std::string, std::string> build_with_template(
        const std::string& template_name,
        const std::string& query,
        const std::vector<RAGResult>& results,
        const ContextBuildOptions& options = ContextBuildOptions::default_options()
    );

    // ============ 高级功能 ============

    /**
     * @brief 压缩上下文（去除冗余）
     * @param context 原始上下文
     * @param max_length 最大长度
     * @return 压缩后的上下文
     */
    std::string compress_context(const std::string& context, int max_length);

    /**
     * @brief 提取关键信息
     * @param results RAG 结果
     * @param count 提取数量
     * @return 关键记忆列表
     */
    std::vector<RAGResult> extract_key_memories(
        const std::vector<RAGResult>& results,
        size_t count = 5
    );

    /**
     * @brief 按时间分组记忆
     * @param results RAG 结果
     * @return 时间分组的结果
     */
    std::vector<std::pair<std::string, std::vector<RAGResult>>> group_by_time(
        const std::vector<RAGResult>& results
    );

private:
    /**
     * @brief 私有构造函数（单例模式）
     */
    ContextBuilder() = default;

    /**
     * @brief 析构函数
     */
    ~ContextBuilder() = default;

    /**
     * @brief 格式化单个记忆为上下文
     */
    std::string format_memory(
        const Memory::Memory& memory,
        double relevance_score,
        const ContextBuildOptions& options
    );

    /**
     * @brief 去除重复记忆
     */
    std::vector<RAGResult> deduplicate_results(const std::vector<RAGResult>& results);

    /**
     * @brief 按相关性排序
     */
    std::vector<RAGResult> sort_by_relevance(std::vector<RAGResult> results);

    /**
     * @brief 截断到指定长度
     */
    std::string truncate_to_length(const std::string& text, int max_length);

    // ============ 成员变量 ============

    std::unordered_map<std::string, std::string> m_templates;  ///< 上下文模板

    // 内置模板
    static const char* DEFAULT_TEMPLATE;
    static const char* CONCISE_TEMPLATE;
    static const char* VERBOSE_TEMPLATE;
};

} // namespace DearTs::Plugins::MemoryCore::RAG
