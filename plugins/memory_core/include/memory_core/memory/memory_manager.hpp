/**
 * @file memory_manager.hpp
 * @brief 记忆管理器 - 管理全局记忆的 CRUD 操作
 *
 * 功能：
 * - 记忆的增删改查
 * - 按类型和重要性检索记忆
 * - 记忆重要性评分更新
 * - 访问频率统计
 */

#pragma once

#include "core/result.h"
#include <string>
#include <vector>
#include <optional>
#include <functional>

namespace DearTs::Plugins::MemoryCore {

// 前向声明
namespace Persistence {
    class SQLiteDatabase;
}

namespace Memory {

/**
 * @brief 记忆类型枚举
 */
enum class MemoryType {
    Preference,     ///< 用户偏好（如：代码风格、主题喜好）
    Fact,           ///< 事实知识（如：用户个人信息、工作背景）
    QA,             ///< 问答对（如：常见问题的解答）
    Context,        ///< 上下文信息（如：当前工作项目）
    Skill           ///< 技能信息（如：用户擅长的技术栈）
};

/**
 * @brief 记忆项结构体
 */
struct Memory {
    int64_t id;                      ///< 数据库 ID
    MemoryType type;                 ///< 记忆类型
    std::string content;             ///< 记忆内容
    std::optional<std::string> source_conversation_id;  ///< 来源会话 ID
    std::optional<int64_t> source_message_id;           ///< 来源消息 ID
    double importance;               ///< 重要性分数 [0-1]
    int64_t created_at;              ///< 创建时间（Unix 毫秒）
    int accessed_count;              ///< 访问次数
    std::optional<int64_t> last_accessed_at;  ///< 最后访问时间
    std::optional<int64_t> embedding_id;      ///< 向量嵌入 ID

    /**
     * @brief 将类型转换为字符串
     */
    static std::string type_to_string(MemoryType type) {
        switch (type) {
            case MemoryType::Preference: return "preference";
            case MemoryType::Fact: return "fact";
            case MemoryType::QA: return "qa";
            case MemoryType::Context: return "context";
            case MemoryType::Skill: return "skill";
            default: return "unknown";
        }
    }

    /**
     * @brief 从字符串解析类型
     */
    static MemoryType string_to_type(const std::string& str) {
        if (str == "preference") return MemoryType::Preference;
        if (str == "fact") return MemoryType::Fact;
        if (str == "qa") return MemoryType::QA;
        if (str == "context") return MemoryType::Context;
        if (str == "skill") return MemoryType::Skill;
        return MemoryType::Fact;  // 默认
    }
};

/**
 * @brief 记忆搜索过滤器
 */
struct MemoryFilter {
    std::optional<MemoryType> type;              ///< 按类型过滤
    std::optional<double> min_importance;        ///< 最小重要性
    std::optional<int> min_access_count;         ///< 最小访问次数
    std::optional<std::string> conversation_id;  ///< 按会话过滤
    std::string content_contains;                ///< 内容包含关键字
    int limit = 50;                              ///< 结果限制
    int offset = 0;                              ///< 偏移量
};

/**
 * @brief 记忆管理器
 *
 * 提供记忆的增删改查、搜索和更新功能
 */
class MemoryManager {
public:
    /**
     * @brief 获取单例实例
     */
    static MemoryManager& instance();

    /**
     * @brief 删除拷贝构造和赋值
     */
    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;

    // ============ CRUD 操作 ============

    /**
     * @brief 添加新记忆
     * @param memory 要添加的记忆
     * @return 新创建记忆的 ID 或错误信息
     */
    DearTs::Core::Result<int64_t, std::string> add_memory(const Memory& memory);

    /**
     * @brief 批量添加记忆
     * @param memories 要添加的记忆列表
     * @return 成功添加的数量或错误信息
     */
    DearTs::Core::Result<size_t, std::string> add_memories(const std::vector<Memory>& memories);

    /**
     * @brief 根据 ID 获取记忆
     * @param id 记忆 ID
     * @return 记忆对象或错误信息
     */
    DearTs::Core::Result<Memory, std::string> get_memory(int64_t id);

    /**
     * @brief 更新记忆
     * @param id 记忆 ID
     * @param memory 更新的记忆内容
     * @return 成功或错误信息
     */
    DearTs::Core::Result<void, std::string> update_memory(int64_t id, const Memory& memory);

    /**
     * @brief 删除记忆
     * @param id 记忆 ID
     * @return 成功或错误信息
     */
    DearTs::Core::Result<void, std::string> delete_memory(int64_t id);

    // ============ 搜索操作 ============

    /**
     * @brief 搜索记忆
     * @param filter 搜索过滤器
     * @return 记忆列表或错误信息
     */
    DearTs::Core::Result<std::vector<Memory>, std::string> search_memories(const MemoryFilter& filter);

    /**
     * @brief 获取所有记忆（带分页）
     * @param limit 结果限制
     * @param offset 偏移量
     * @return 记忆列表或错误信息
     */
    DearTs::Core::Result<std::vector<Memory>, std::string> get_all_memories(int limit = 100, int offset = 0);

    /**
     * @brief 按类型获取记忆
     * @param type 记忆类型
     * @param limit 结果限制
     * @return 记忆列表或错误信息
     */
    DearTs::Core::Result<std::vector<Memory>, std::string> get_memories_by_type(MemoryType type, int limit = 50);

    /**
     * @brief 按重要性获取记忆
     * @param min_importance 最小重要性
     * @param limit 结果限制
     * @return 记忆列表或错误信息
     */
    DearTs::Core::Result<std::vector<Memory>, std::string> get_memories_by_importance(double min_importance, int limit = 50);

    /**
     * @brief 获取最重要的记忆（按访问次数和重要性排序）
     * @param limit 结果限制
     * @return 记忆列表或错误信息
     */
    DearTs::Core::Result<std::vector<Memory>, std::string> get_top_memories(int limit = 20);

    // ============ 统计操作 ============

    /**
     * @brief 增加记忆访问计数
     * @param id 记忆 ID
     * @return 成功或错误信息
     */
    DearTs::Core::Result<void, std::string> increment_access_count(int64_t id);

    /**
     * @brief 更新记忆重要性
     * @param id 记忆 ID
     * @param importance 新的重要性分数 [0-1]
     * @return 成功或错误信息
     */
    DearTs::Core::Result<void, std::string> update_importance(int64_t id, double importance);

    /**
     * @brief 获取记忆总数
     * @return 记忆总数或错误信息
     */
    DearTs::Core::Result<size_t, std::string> get_memory_count();

    /**
     * @brief 按类型统计记忆数量
     * @return 类型到数量的映射或错误信息
     */
    DearTs::Core::Result<std::vector<std::pair<MemoryType, size_t>>, std::string> get_memory_count_by_type();

private:
    /**
     * @brief 私有构造函数（单例模式）
     */
    MemoryManager() = default;

    /**
     * @brief 析构函数
     */
    ~MemoryManager() = default;

    /**
     * @brief 将 Memory 对象转换为 SQL INSERT 语句
     */
    std::string build_insert_sql(const Memory& memory);

    /**
     * @brief 从数据库行构建 Memory 对象
     */
    DearTs::Core::Result<Memory, std::string> memory_from_db_row(void* stmt);
};

} // namespace Memory
} // namespace DearTs::Plugins::MemoryCore
