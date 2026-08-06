/**
 * @file database.hpp
 * @brief SQLite 数据库封装类
 */

#pragma once

#include "core/result.h"
// TODO: 添加 SQLite3 amalgamation 或通过 CMake 查找
// #include <sqlite3.h>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <optional>

// SQLite3 前向声明（临时方案）
struct sqlite3;
struct sqlite3_stmt;

namespace DearTs {
    namespace Plugins {
        namespace MemoryCore {
            namespace Persistence {

/**
 * @brief SQLite 数据库类
 *
 * 提供数据库连接管理、表创建和基础 CRUD 操作
 */
class SQLiteDatabase {
public:
    /**
     * @brief 获取单例实例
     */
    static SQLiteDatabase& instance();

    /**
     * @brief 删除拷贝构造和赋值
     */
    SQLiteDatabase(const SQLiteDatabase&) = delete;
    SQLiteDatabase& operator=(const SQLiteDatabase&) = delete;

    // ============ 初始化和连接管理 ============

    /**
     * @brief 初始化数据库
     * @param db_path 数据库文件路径
     * @return 成功返回 Result::ok()，失败返回错误信息
     */
    DearTs::Core::Result<void, std::string> initialize(const std::string& db_path);

    /**
     * @brief 关闭数据库连接
     */
    void close();

    /**
     * @brief 检查数据库是否在线
     * @return 数据库连接是否有效
     */
    bool is_open() const { return m_db != nullptr; }

    /**
     * @brief 获取原始数据库指针
     * @return sqlite3 指针
     */
    sqlite3* get_db() const { return m_db; }

    /**
     * @brief 获取数据库路径
     * @return 数据库文件路径
     */
    const std::string& get_db_path() const { return m_db_path; }

    // ============ 事务管理 ============

    /**
     * @brief 开始事务
     */
    DearTs::Core::Result<void, std::string> begin_transaction();

    /**
     * @brief 提交事务
     */
    DearTs::Core::Result<void, std::string> commit();

    /**
     * @brief 回滚事务
     */
    DearTs::Core::Result<void, std::string> rollback();

    /**
     * @brief 事务包装器 - 自动管理事务
     * @param func 要执行的函数
     * @return 函数执行结果（如果函数返回 Result<T,E>，则返回 Result<T,E>；否则返回 Result<Ret,E>）
     */
    template<typename Func>
    auto transaction(Func&& func) {
        using RetType = decltype(func(*this));
        using ResultType = DearTs::Core::Result<RetType, std::string>;

        auto begin_result = begin_transaction();
        if (begin_result.isErr()) {
            if constexpr (DearTs::Core::is_result<RetType>::value) {
                // 如果 lambda 返回 Result，返回错误的 Result
                return RetType::err("Failed to begin transaction: " + begin_result.error());
            } else {
                return ResultType::err("Failed to begin transaction: " + begin_result.error());
            }
        }

        auto result = func(*this);

        // 检查是否返回 Result 类型
        constexpr bool returns_result = DearTs::Core::is_result<RetType>::value;

        if constexpr (returns_result) {
            // Lambda 返回 Result<T, E>
            if (result.isErr()) {
                rollback();
                return result;  // 直接返回 Result<T, E>
            }

            auto commit_result = commit();
            if (commit_result.isErr()) {
                rollback();
                return RetType::err("Failed to commit transaction: " + commit_result.error());
            }

            return result;  // 直接返回 Result<T, E>
        } else {
            // Lambda 返回普通类型
            auto commit_result = commit();
            if (commit_result.isErr()) {
                rollback();
                return ResultType::err("Failed to commit transaction: " + commit_result.error());
            }

            return ResultType::ok(result);
        }
    }

    // ============ 消息操作（阶段 1.2 实现） ============

    /**
     * @brief 消息搜索结果结构体
     */
    struct MessageSearchResult {
        int64_t id;                  ///< 消息 ID
        std::string message_uuid;    ///< 消息 UUID
        std::string conversation_id; ///< 会话 ID
        std::string role;            ///< 角色 (user/assistant)
        std::string content;         ///< 内容
        int64_t timestamp;           ///< 时间戳
        double rank;                 ///< FTS5 排序分值
    };

    /**
     * @brief 使用 FTS5 全文搜索消息
     * @param query 搜索查询字符串（支持 FTS5 查询语法）
     * @param conversation_id 可选的会话 ID 限制（为空时搜索所有会话）
     * @param limit 结果数量限制
     * @return 搜索结果列表或错误信息
     */
    DearTs::Core::Result<std::vector<MessageSearchResult>, std::string> search_messages(
        const std::string& query,
        const std::string& conversation_id = "",
        int limit = 50
    );

    /**
     * @brief 使用 FTS5 高亮搜索结果
     * @param query 搜索查询字符串
     * @param content 原始内容
     * @param context_chars 高亮上下文字符数
     * @return 带有 <mark> 标签的高亮文本
     */
    static std::string highlight_fts_result(
        const std::string& query,
        const std::string& content,
        int context_chars = 50
    );

    /**
     * @brief 插入消息到数据库
     * @param conversation_id 会话 ID
     * @param message_uuid 消息 UUID
     * @param role 角色 (user/assistant)
     * @param content 消息内容
     * @param timestamp 时间戳
     * @param tokens Token 数量（可选）
     * @return 成功返回插入的记录 ID，失败返回错误信息
     */
    DearTs::Core::Result<int64_t, std::string> insert_message(
        const std::string& conversation_id,
        const std::string& message_uuid,
        const std::string& role,
        const std::string& content,
        int64_t timestamp,
        std::optional<int> tokens = std::nullopt
    );

    /**
     * @brief 插入会话到数据库
     * @param conversation_id 会话 ID
     * @param title 会话标题
     * @param type 会话类型
     * @param timestamp 时间戳
     * @return 成功返回 void，失败返回错误信息
     */
    DearTs::Core::Result<void, std::string> insert_conversation(
        const std::string& conversation_id,
        const std::string& title,
        const std::string& type,
        int64_t timestamp
    );

    /**
     * @brief 删除会话及其所有相关数据
     * @param conversation_id 会话 ID
     * @return 成功返回 void，失败返回错误信息
     * @note 由于外键约束 ON DELETE CASCADE，会自动删除相关消息
     */
    DearTs::Core::Result<void, std::string> delete_conversation(
        const std::string& conversation_id
    );

    /**
     * @brief 更新会话标题
     * @param conversation_id 会话 ID
     * @param new_title 新标题
     * @return 成功返回 void，失败返回错误信息
     */
    DearTs::Core::Result<void, std::string> update_conversation_title(
        const std::string& conversation_id,
        const std::string& new_title
    );

    /**
     * @brief 会话记录结构
     */
    struct ConversationRecord {
        std::string id;
        std::string title;
        std::string type;
        int64_t created_at;
        int64_t updated_at;
    };

    /**
     * @brief 消息记录结构
     */
    struct MessageRecord {
        int64_t id;
        std::string conversation_id;
        std::string message_uuid;
        std::string role;
        std::string content;
        int64_t timestamp;
        std::optional<int> tokens;
    };

    /**
     * @brief 获取指定时间范围内的会话列表
     * @param start_time_ms 开始时间戳（毫秒）
     * @param end_time_ms 结束时间戳（毫秒）
     * @return 成功返回会话列表，每个会话包含 id, title, type, created_at, updated_at
     */
    DearTs::Core::Result<std::vector<ConversationRecord>, std::string> get_conversations_by_date_range(
        int64_t start_time_ms,
        int64_t end_time_ms
    );

    /**
     * @brief 获取所有会话（不限制时间范围）
     * @return 成功返回所有会话列表
     */
    DearTs::Core::Result<std::vector<ConversationRecord>, std::string> get_all_conversations();

    /**
     * @brief 获取指定会话的所有消息
     * @param conversation_id 会话 ID
     * @return 成功返回消息列表，每条消息包含完整信息
     */
    DearTs::Core::Result<std::vector<MessageRecord>, std::string> get_messages_by_conversation(
        const std::string& conversation_id
    );

    // ============ 嵌入向量操作 ============

    /**
     * @brief 嵌入向量记录结构
     */
    struct EmbeddingRecord {
        int64_t id;
        std::string model;
        int dimension;
        std::string vector_data;      ///< JSON 序列化的向量数据
        int64_t created_at;
    };

    /**
     * @brief 存储嵌入向量到数据库
     * @param model 模型名称
     * @param vector_data 向量数据（JSON 格式）
     * @param dimension 向量维度
     * @param created_at 创建时间戳
     * @return 成功返回嵌入 ID，失败返回错误信息
     */
    DearTs::Core::Result<int64_t, std::string> insert_embedding(
        const std::string& model,
        const std::string& vector_data,
        int dimension,
        int64_t created_at
    );

    /**
     * @brief 获取嵌入向量
     * @param embedding_id 嵌入 ID
     * @return 嵌入记录或错误信息
     */
    DearTs::Core::Result<EmbeddingRecord, std::string> get_embedding(int64_t embedding_id);

    /**
     * @brief 删除嵌入向量
     * @param embedding_id 嵌入 ID
     * @return 成功或错误信息
     */
    DearTs::Core::Result<void, std::string> delete_embedding(int64_t embedding_id);

    // ============ 会话操作（TODO: 阶段 1 实现） ============

    // ============ 记忆操作（TODO: 阶段 3 实现） ============

private:
    /**
     * @brief 私有构造函数（单例模式）
     */
    SQLiteDatabase() = default;

    /**
     * @brief 析构函数
     */
    ~SQLiteDatabase();

    /**
     * @brief 创建所有表
     */
    DearTs::Core::Result<void, std::string> create_tables();

    /**
     * @brief 注册自定义 SQL 函数
     */
    void register_sql_functions();

    // ============ 成员变量 ============

    sqlite3* m_db = nullptr;           ///< SQLite 数据库指针
    std::string m_db_path;             ///< 数据库文件路径
    mutable std::mutex m_mutex;        ///< 线程安全互斥锁
    bool m_isOnline = true;            ///< 数据库是否在线
};

            } // namespace Persistence
        } // namespace MemoryCore
    } // namespace Plugins
} // namespace DearTs
