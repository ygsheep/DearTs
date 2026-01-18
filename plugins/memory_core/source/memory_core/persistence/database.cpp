/**
 * @file database.cpp
 * @brief SQLite 数据库封装类实现
 */

#include "memory_core/persistence/database.hpp"
#include "memory_core/persistence/schema.hpp"
#include "liblogger/logger.h"
#include <filesystem>
#include <stdexcept>
#include <vector>
#include <sstream>
#include <cmath>
#include <cstring>
#include <algorithm>

// SQLite3 头文件 - 条件编译
#ifdef SQLITE3_FOUND
    #include <sqlite3.h>
#endif

namespace DearTs {
    namespace Plugins {
        namespace MemoryCore {
            namespace Persistence {

// ============ 内部辅助函数 ============

#ifdef SQLITE3_FOUND
/**
 * @brief 内部关闭函数（不获取锁，用于已经持有锁的情况）
 */
static void close_internal(sqlite3*& db) {
    if (db) {
        sqlite3_close(db);
        db = nullptr;
        LOG_INFO("Database closed");
    }
}
#else
static void close_internal(sqlite3*& db) {
    if (db) {
        db = nullptr;
        LOG_INFO("Database closed (placeholder)");
    }
}
#endif

// ============ 单例实现 ============

SQLiteDatabase& SQLiteDatabase::instance() {
    static SQLiteDatabase instance;
    return instance;
}

// ============ 析构函数 ============

SQLiteDatabase::~SQLiteDatabase() {
    close();
}

// ============ 初始化 ============

DearTs::Core::Result<void, std::string> SQLiteDatabase::initialize(const std::string& db_path) {
#ifdef SQLITE3_FOUND
    std::lock_guard lock(m_mutex);

    if (m_db != nullptr) {
        return DearTs::Core::Result<void, std::string>::err(
            "Database already initialized"
        );
    }

    LOG_INFO("Initializing database: {}", db_path);

    // 确保目录存在
    std::filesystem::path path(db_path);
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path());
    }

    // 打开数据库
    int rc = sqlite3_open_v2(
        db_path.c_str(),
        &m_db,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
        nullptr
    );

    if (rc != SQLITE_OK) {
        std::string error = sqlite3_errmsg(m_db);
        return DearTs::Core::Result<void, std::string>::err(
            "Failed to open database: " + error
        );
    }

    m_db_path = db_path;

    // 设置性能优化选项
    sqlite3_exec(m_db, "PRAGMA journal_mode = WAL;", nullptr, nullptr, nullptr);
    sqlite3_exec(m_db, "PRAGMA synchronous = NORMAL;", nullptr, nullptr, nullptr);
    sqlite3_exec(m_db, "PRAGMA cache_size = -64000;", nullptr, nullptr, nullptr);  // 64MB
    sqlite3_exec(m_db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);

    // 注册自定义 SQL 函数
    register_sql_functions();

    // 创建表
    auto create_result = create_tables();
    if (create_result.isErr()) {
        // 使用内部函数关闭，避免重复锁定互斥锁
        close_internal(m_db);
        return DearTs::Core::Result<void, std::string>::err(
            "Failed to create tables: " + create_result.error()
        );
    }

    LOG_INFO("Database initialized successfully: {}", db_path);
    return DearTs::Core::Result<void, std::string>::ok();
#else
    // SQLite3 未找到时返回占位符
    LOG_WARN("SQLite3 not available, database functions will be placeholders");
    LOG_INFO("Database path (not initialized): {}", db_path);
    m_db_path = db_path;
    return DearTs::Core::Result<void, std::string>::ok();
#endif
}

void SQLiteDatabase::close() {
    std::lock_guard lock(m_mutex);
    close_internal(m_db);
}

// ============ 事务管理 ============

DearTs::Core::Result<void, std::string> SQLiteDatabase::begin_transaction() {
#ifdef SQLITE3_FOUND
    std::lock_guard lock(m_mutex);

    if (!m_db) {
        return DearTs::Core::Result<void, std::string>::err(
            "Database not initialized"
        );
    }

    char* error_msg = nullptr;
    int rc = sqlite3_exec(m_db, "BEGIN TRANSACTION;", nullptr, nullptr, &error_msg);

    if (rc != SQLITE_OK) {
        std::string error(error_msg ? error_msg : "Unknown error");
        sqlite3_free(error_msg);
        return DearTs::Core::Result<void, std::string>::err(
            "Failed to begin transaction: " + error
        );
    }

    return DearTs::Core::Result<void, std::string>::ok();
#else
    LOG_WARN("begin_transaction called but SQLite3 not available");
    return DearTs::Core::Result<void, std::string>::ok();
#endif
}

DearTs::Core::Result<void, std::string> SQLiteDatabase::commit() {
#ifdef SQLITE3_FOUND
    std::lock_guard lock(m_mutex);

    if (!m_db) {
        return DearTs::Core::Result<void, std::string>::err(
            "Database not initialized"
        );
    }

    char* error_msg = nullptr;
    int rc = sqlite3_exec(m_db, "COMMIT;", nullptr, nullptr, &error_msg);

    if (rc != SQLITE_OK) {
        std::string error(error_msg ? error_msg : "Unknown error");
        sqlite3_free(error_msg);
        return DearTs::Core::Result<void, std::string>::err(
            "Failed to commit transaction: " + error
        );
    }

    return DearTs::Core::Result<void, std::string>::ok();
#else
    LOG_WARN("commit called but SQLite3 not available");
    return DearTs::Core::Result<void, std::string>::ok();
#endif
}

DearTs::Core::Result<void, std::string> SQLiteDatabase::rollback() {
#ifdef SQLITE3_FOUND
    std::lock_guard lock(m_mutex);

    if (!m_db) {
        return DearTs::Core::Result<void, std::string>::err(
            "Database not initialized"
        );
    }

    char* error_msg = nullptr;
    int rc = sqlite3_exec(m_db, "ROLLBACK;", nullptr, nullptr, &error_msg);

    if (rc != SQLITE_OK) {
        std::string error(error_msg ? error_msg : "Unknown error");
        sqlite3_free(error_msg);
        return DearTs::Core::Result<void, std::string>::err(
            "Failed to rollback transaction: " + error
        );
    }

    return DearTs::Core::Result<void, std::string>::ok();
#else
    LOG_WARN("rollback called but SQLite3 not available");
    return DearTs::Core::Result<void, std::string>::ok();
#endif
}

// ============ 私有方法 ============

DearTs::Core::Result<void, std::string> SQLiteDatabase::create_tables() {
#ifdef SQLITE3_FOUND
    // 0. 先删除旧的 FTS5 触发器（如果存在），避免引用不存在的 FTS5 表
    const std::vector<const char*> drop_trigger_sqls = {
        "DROP TRIGGER IF EXISTS messages_fts_insert;",
        "DROP TRIGGER IF EXISTS messages_fts_delete;",
        "DROP TRIGGER IF EXISTS messages_fts_update;"
    };

    for (const char* sql : drop_trigger_sqls) {
        char* error_msg = nullptr;
        int rc = sqlite3_exec(m_db, sql, nullptr, nullptr, &error_msg);
        if (rc != SQLITE_OK) {
            // 忽略错误（触发器可能不存在）
            sqlite3_free(error_msg);
        }
    }
    LOG_INFO("Old FTS5 triggers dropped (if any existed)");

    // 1. 先创建基础表（不依赖 FTS5）
    auto basic_sql = DatabaseSchema::get_basic_init_sql();
    for (const auto& sql : basic_sql) {
        char* error_msg = nullptr;
        int rc = sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &error_msg);

        if (rc != SQLITE_OK) {
            std::string error(error_msg ? error_msg : "Unknown error");
            sqlite3_free(error_msg);
            return DearTs::Core::Result<void, std::string>::err(
                "Failed to execute basic SQL: " + error + "\nSQL: " + sql
            );
        }
    }

    LOG_INFO("Basic tables and indexes created successfully");

    // 2. 尝试创建 FTS5 表和触发器
    bool fts5_unavailable = false;

    // 尝试创建 FTS5 表
    const char* fts5_table_sql = DatabaseSchema::messages_fts_table();
    char* error_msg = nullptr;
    int rc = sqlite3_exec(m_db, fts5_table_sql, nullptr, nullptr, &error_msg);

    if (rc != SQLITE_OK) {
        std::string error(error_msg ? error_msg : "Unknown error");

        // 检查是否是 FTS5 不可用错误
        if (error.find("no such module: fts5") != std::string::npos ||
            error.find("fts5") != std::string::npos) {
            LOG_WARN("FTS5 module not available in this SQLite build. "
                     "Full-text search features will be disabled.");
            fts5_unavailable = true;
            sqlite3_free(error_msg);
        } else {
            sqlite3_free(error_msg);
            return DearTs::Core::Result<void, std::string>::err(
                "Failed to create FTS5 table: " + error
            );
        }
    } else {
        LOG_INFO("FTS5 table created successfully");

        // 3. 只有在 FTS5 表成功创建后才创建触发器
        auto fts5_triggers = DatabaseSchema::fts5_triggers();
        for (const auto& trigger_sql : fts5_triggers) {
            error_msg = nullptr;
            rc = sqlite3_exec(m_db, trigger_sql, nullptr, nullptr, &error_msg);

            if (rc != SQLITE_OK) {
                std::string error(error_msg ? error_msg : "Unknown error");
                sqlite3_free(error_msg);
                return DearTs::Core::Result<void, std::string>::err(
                    "Failed to create FTS5 trigger: " + error
                );
            }
        }

        LOG_INFO("FTS5 triggers created successfully");
    }

    if (fts5_unavailable) {
        LOG_INFO("Database initialization completed (FTS5 disabled)");
    } else {
        LOG_INFO("All database objects created successfully (including FTS5)");
    }

    return DearTs::Core::Result<void, std::string>::ok();
#else
    LOG_WARN("create_tables called but SQLite3 not available");
    return DearTs::Core::Result<void, std::string>::ok();
#endif
}

// ============ 自定义 SQL 函数 ============

#ifdef SQLITE3_FOUND

/**
 * @brief 解析 JSON 数组字符串为向量
 * @param json_str JSON 数组字符串，如 "[0.1, 0.2, 0.3]"
 * @return 解析后的向量
 */
static std::vector<double> parse_json_array(const std::string& json_str) {
    std::vector<double> result;

    // 跳过开头的 '[' 和空白
    size_t start = json_str.find('[');
    if (start == std::string::npos) {
        return result;
    }
    start++;

    // 解析数字
    std::string num_str;
    for (size_t i = start; i < json_str.size(); i++) {
        char c = json_str[i];

        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            continue;  // 跳过空白
        } else if (c == ',' || c == ']') {
            if (!num_str.empty()) {
                try {
                    result.push_back(std::stod(num_str));
                    num_str.clear();
                } catch (...) {
                    // 解析失败，跳过
                }
            }
            if (c == ']') {
                break;  // 结束
            }
        } else if (c == '-' || c == '.' || (c >= '0' && c <= '9')) {
            num_str += c;  // 数字字符
        }
    }

    return result;
}

/**
 * @brief SQLite3 自定义函数：余弦相似度
 * @param context SQLite3 函数上下文
 * @param argc 参数数量（应为 2）
 * @param argv 参数值（两个向量字符串）
 */
static void cosine_similarity_func(sqlite3_context* context, int argc, sqlite3_value** argv) {
    if (argc != 2) {
        sqlite3_result_error(context, "cosine_similarity requires exactly 2 arguments", -1);
        return;
    }

    // 获取两个向量字符串
    const char* vec1_str = reinterpret_cast<const char*>(sqlite3_value_text(argv[0]));
    const char* vec2_str = reinterpret_cast<const char*>(sqlite3_value_text(argv[1]));

    if (!vec1_str || !vec2_str) {
        sqlite3_result_null(context);
        return;
    }

    // 解析向量
    std::vector<double> vec1 = parse_json_array(vec1_str);
    std::vector<double> vec2 = parse_json_array(vec2_str);

    // 检查向量维度
    if (vec1.empty() || vec2.empty() || vec1.size() != vec2.size()) {
        sqlite3_result_null(context);
        return;
    }

    // 计算点积和模长
    double dot_product = 0.0;
    double norm1 = 0.0;
    double norm2 = 0.0;

    for (size_t i = 0; i < vec1.size(); i++) {
        dot_product += vec1[i] * vec2[i];
        norm1 += vec1[i] * vec1[i];
        norm2 += vec2[i] * vec2[i];
    }

    // 计算余弦相似度
    norm1 = std::sqrt(norm1);
    norm2 = std::sqrt(norm2);

    if (norm1 < 1e-10 || norm2 < 1e-10) {
        // 零向量，返回 0
        sqlite3_result_double(context, 0.0);
        return;
    }

    double cosine_sim = dot_product / (norm1 * norm2);

    // 确保结果在 [-1, 1] 范围内
    if (cosine_sim > 1.0) cosine_sim = 1.0;
    if (cosine_sim < -1.0) cosine_sim = -1.0;

    sqlite3_result_double(context, cosine_sim);
}

#endif // SQLITE3_FOUND

void SQLiteDatabase::register_sql_functions() {
#ifdef SQLITE3_FOUND
    if (!m_db) {
        LOG_WARN("Cannot register SQL functions: database not initialized");
        return;
    }

    // 注册余弦相似度函数: cosine_similarity(vec1, vec2)
    int rc = sqlite3_create_function(
        m_db,                          // 数据库连接
        "cosine_similarity",           // 函数名
        2,                             // 参数数量
        SQLITE_UTF8,                   // 文本编码
        nullptr,                       // 用户数据
        cosine_similarity_func,        // 函数实现
        nullptr,                       // step 函数（聚合函数用）
        nullptr                        // final 函数（聚合函数用）
    );

    if (rc != SQLITE_OK) {
        LOG_ERROR("Failed to register cosine_similarity function: {}", sqlite3_errmsg(m_db));
    } else {
        LOG_INFO("Custom SQL function 'cosine_similarity' registered successfully");
    }
#else
    LOG_INFO("Custom SQL functions registration skipped (SQLite3 not available)");
#endif
}

// ============ FTS5 全文搜索 ============

DearTs::Core::Result<std::vector<SQLiteDatabase::MessageSearchResult>, std::string>
SQLiteDatabase::search_messages(
    const std::string& query,
    const std::string& conversation_id,
    int limit
) {
#ifdef SQLITE3_FOUND
    std::lock_guard lock(m_mutex);

    if (!m_db) {
        return DearTs::Core::Result<std::vector<MessageSearchResult>, std::string>::err(
            "Database not initialized"
        );
    }

    // 构建 FTS5 搜索查询
    // FTS5 语法: SELECT ... FROM table WHERE table MATCH 'query' ORDER BY rank
    std::string sql;
    if (conversation_id.empty()) {
        sql = "SELECT m.id, m.message_uuid, m.conversation_id, m.role, m.content, m.timestamp, "
              "messages_fts.rank "
              "FROM messages m "
              "INNER JOIN messages_fts ON m.id = messages_fts.rowid "
              "WHERE messages_fts MATCH ? "
              "ORDER BY messages_fts.rank "
              "LIMIT ?";
    } else {
        sql = "SELECT m.id, m.message_uuid, m.conversation_id, m.role, m.content, m.timestamp, "
              "messages_fts.rank "
              "FROM messages m "
              "INNER JOIN messages_fts ON m.id = messages_fts.rowid "
              "WHERE messages_fts MATCH ? AND m.conversation_id = ? "
              "ORDER BY messages_fts.rank "
              "LIMIT ?";
    }

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        return DearTs::Core::Result<std::vector<MessageSearchResult>, std::string>::err(
            "Failed to prepare search query: " + std::string(sqlite3_errmsg(m_db))
        );
    }

    // 绑定参数
    int param_idx = 1;
    sqlite3_bind_text(stmt, param_idx++, query.c_str(), -1, SQLITE_TRANSIENT);
    if (!conversation_id.empty()) {
        sqlite3_bind_text(stmt, param_idx++, conversation_id.c_str(), -1, SQLITE_TRANSIENT);
    }
    sqlite3_bind_int(stmt, param_idx, limit);

    // 执行查询并收集结果
    std::vector<MessageSearchResult> results;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        MessageSearchResult result;
        result.id = sqlite3_column_int64(stmt, 0);
        result.message_uuid = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        result.conversation_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        result.role = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        result.content = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        result.timestamp = sqlite3_column_int64(stmt, 5);
        result.rank = sqlite3_column_double(stmt, 6);
        results.push_back(std::move(result));
    }

    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return DearTs::Core::Result<std::vector<MessageSearchResult>, std::string>::err(
            "Failed to execute search query: " + std::string(sqlite3_errmsg(m_db))
        );
    }

    LOG_INFO("FTS5 search completed: {} results for query '{}'", results.size(), query);
    return DearTs::Core::Result<std::vector<MessageSearchResult>, std::string>::ok(results);

#else
    LOG_WARN("search_messages called but SQLite3 not available");
    return DearTs::Core::Result<std::vector<MessageSearchResult>, std::string>::ok({});
#endif
}

// ============ 消息和会话插入操作 ============

DearTs::Core::Result<int64_t, std::string> SQLiteDatabase::insert_message(
    const std::string& conversation_id,
    const std::string& message_uuid,
    const std::string& role,
    const std::string& content,
    int64_t timestamp,
    std::optional<int> tokens
) {
#ifdef SQLITE3_FOUND
    std::lock_guard lock(m_mutex);

    if (!m_db) {
        return DearTs::Core::Result<int64_t, std::string>::err("Database not initialized");
    }

    const char* sql = R"(
        INSERT INTO messages (message_uuid, conversation_id, role, content, timestamp, tokens)
        VALUES (?, ?, ?, ?, ?, ?)
    )";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return DearTs::Core::Result<int64_t, std::string>::err(
            std::format("Failed to prepare statement: {}", sqlite3_errmsg(m_db))
        );
    }

    // 绑定参数
    sqlite3_bind_text(stmt, 1, message_uuid.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, conversation_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, role.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, content.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 5, timestamp);
    sqlite3_bind_int(stmt, 6, tokens.value_or(0));

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::string error = sqlite3_errmsg(m_db);
        sqlite3_finalize(stmt);
        return DearTs::Core::Result<int64_t, std::string>::err(
            std::format("Failed to execute statement: {}", error)
        );
    }

    int64_t row_id = sqlite3_last_insert_rowid(m_db);
    sqlite3_finalize(stmt);

    LOG_INFO("Message inserted: id={}, uuid={}", row_id, message_uuid);
    return DearTs::Core::Result<int64_t, std::string>::ok(row_id);
#else
    return DearTs::Core::Result<int64_t, std::string>::err("SQLite3 not available");
#endif
}

DearTs::Core::Result<void, std::string> SQLiteDatabase::insert_conversation(
    const std::string& conversation_id,
    const std::string& title,
    const std::string& type,
    int64_t timestamp
) {
#ifdef SQLITE3_FOUND
    std::lock_guard lock(m_mutex);

    if (!m_db) {
        return DearTs::Core::Result<void, std::string>::err("Database not initialized");
    }

    const char* sql = R"(
        INSERT OR REPLACE INTO conversations (id, title, type, created_at, updated_at)
        VALUES (?, ?, ?, ?, ?)
    )";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return DearTs::Core::Result<void, std::string>::err(
            std::format("Failed to prepare statement: {}", sqlite3_errmsg(m_db))
        );
    }

    sqlite3_bind_text(stmt, 1, conversation_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, title.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 4, timestamp);
    sqlite3_bind_int64(stmt, 5, timestamp);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::string error = sqlite3_errmsg(m_db);
        sqlite3_finalize(stmt);
        return DearTs::Core::Result<void, std::string>::err(
            std::format("Failed to execute statement: {}", error)
        );
    }

    sqlite3_finalize(stmt);
    LOG_INFO("Conversation inserted: id={}", conversation_id);
    return DearTs::Core::Result<void, std::string>::ok();
#else
    return DearTs::Core::Result<void, std::string>::err("SQLite3 not available");
#endif
}

DearTs::Core::Result<void, std::string> SQLiteDatabase::delete_conversation(
    const std::string& conversation_id
) {
#ifdef SQLITE3_FOUND
    std::lock_guard lock(m_mutex);

    if (!m_db) {
        return DearTs::Core::Result<void, std::string>::err("Database not initialized");
    }

    const char* sql = R"(
        DELETE FROM conversations WHERE id = ?
    )";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return DearTs::Core::Result<void, std::string>::err(
            std::format("Failed to prepare statement: {}", sqlite3_errmsg(m_db))
        );
    }

    sqlite3_bind_text(stmt, 1, conversation_id.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::string error = sqlite3_errmsg(m_db);
        sqlite3_finalize(stmt);
        return DearTs::Core::Result<void, std::string>::err(
            std::format("Failed to execute statement: {}", error)
        );
    }

    sqlite3_finalize(stmt);
    LOG_INFO("Conversation deleted: id={}", conversation_id);
    return DearTs::Core::Result<void, std::string>::ok();
#else
    return DearTs::Core::Result<void, std::string>::err("SQLite3 not available");
#endif
}

DearTs::Core::Result<void, std::string> SQLiteDatabase::update_conversation_title(
    const std::string& conversation_id,
    const std::string& new_title
) {
#ifdef SQLITE3_FOUND
    std::lock_guard lock(m_mutex);

    if (!m_db) {
        return DearTs::Core::Result<void, std::string>::err("Database not initialized");
    }

    const char* sql = R"(
        UPDATE conversations
        SET title = ?, updated_at = ?
        WHERE id = ?
    )";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return DearTs::Core::Result<void, std::string>::err(
            std::format("Failed to prepare statement: {}", sqlite3_errmsg(m_db))
        );
    }

    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count();

    sqlite3_bind_text(stmt, 1, new_title.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, timestamp);
    sqlite3_bind_text(stmt, 3, conversation_id.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::string error = sqlite3_errmsg(m_db);
        sqlite3_finalize(stmt);
        return DearTs::Core::Result<void, std::string>::err(
            std::format("Failed to execute statement: {}", error)
        );
    }

    sqlite3_finalize(stmt);
    LOG_INFO("Conversation title updated: id={}, title={}", conversation_id, new_title);
    return DearTs::Core::Result<void, std::string>::ok();
#else
    return DearTs::Core::Result<void, std::string>::err("SQLite3 not available");
#endif
}

DearTs::Core::Result<std::vector<SQLiteDatabase::ConversationRecord>, std::string>
SQLiteDatabase::get_conversations_by_date_range(int64_t start_time_ms, int64_t end_time_ms) {
#ifdef SQLITE3_FOUND
    std::lock_guard lock(m_mutex);

    if (!m_db) {
        return DearTs::Core::Result<std::vector<ConversationRecord>, std::string>::err(
            "Database not initialized"
        );
    }

    // ✅ DEBUG: 输出查询的时间范围
    LOG_INFO("DEBUG: Querying conversations with created_at BETWEEN {} AND {} (ms)",
             start_time_ms, end_time_ms);
    LOG_INFO("DEBUG: This is approximately {} to {} (seconds)",
             start_time_ms / 1000, end_time_ms / 1000);

    const char* sql = R"(
        SELECT id, title, type, created_at, updated_at
        FROM conversations
        WHERE created_at >= ? AND created_at <= ?
        ORDER BY created_at DESC
    )";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return DearTs::Core::Result<std::vector<ConversationRecord>, std::string>::err(
            std::format("Failed to prepare statement: {}", sqlite3_errmsg(m_db))
        );
    }

    sqlite3_bind_int64(stmt, 1, start_time_ms);
    sqlite3_bind_int64(stmt, 2, end_time_ms);

    std::vector<ConversationRecord> conversations;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        ConversationRecord record;
        record.id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        record.title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        record.type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        record.created_at = sqlite3_column_int64(stmt, 3);
        record.updated_at = sqlite3_column_int64(stmt, 4);

        // ✅ DEBUG: 输出每个会话的时间戳
        LOG_INFO("DEBUG: Found conversation: id={}, created_at={} ms ({} days since epoch)",
                 record.id, record.created_at, record.created_at / 86400000);

        conversations.push_back(std::move(record));
    }

    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return DearTs::Core::Result<std::vector<ConversationRecord>, std::string>::err(
            std::format("Failed to execute statement: {}", sqlite3_errmsg(m_db))
        );
    }

    LOG_INFO("Retrieved {} conversations from database ({} to {})",
             conversations.size(), start_time_ms, end_time_ms);
    return DearTs::Core::Result<std::vector<ConversationRecord>, std::string>::ok(conversations);
#else
    return DearTs::Core::Result<std::vector<ConversationRecord>, std::string>::err("SQLite3 not available");
#endif
}

DearTs::Core::Result<std::vector<SQLiteDatabase::ConversationRecord>, std::string>
SQLiteDatabase::get_all_conversations() {
#ifdef SQLITE3_FOUND
    std::lock_guard lock(m_mutex);

    if (!m_db) {
        return DearTs::Core::Result<std::vector<ConversationRecord>, std::string>::err(
            "Database not initialized"
        );
    }

    const char* sql = R"(
        SELECT id, title, type, created_at, updated_at
        FROM conversations
        ORDER BY created_at DESC
    )";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return DearTs::Core::Result<std::vector<ConversationRecord>, std::string>::err(
            std::format("Failed to prepare statement: {}", sqlite3_errmsg(m_db))
        );
    }

    std::vector<ConversationRecord> conversations;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        ConversationRecord record;
        record.id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        record.title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        record.type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        record.created_at = sqlite3_column_int64(stmt, 3);
        record.updated_at = sqlite3_column_int64(stmt, 4);

        // ✅ DEBUG: 输出每个会话的时间戳
        LOG_INFO("DEBUG: Found conversation: id={}, title={}, created_at={} ms ({} days since epoch, ~{})",
                 record.id, record.title, record.created_at,
                 record.created_at / 86400000,
                 (record.created_at / 86400000) / 365 + 1970);

        conversations.push_back(std::move(record));
    }

    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return DearTs::Core::Result<std::vector<ConversationRecord>, std::string>::err(
            std::format("Failed to execute statement: {}", sqlite3_errmsg(m_db))
        );
    }

    LOG_INFO("Retrieved all {} conversations from database", conversations.size());
    return DearTs::Core::Result<std::vector<ConversationRecord>, std::string>::ok(conversations);
#else
    return DearTs::Core::Result<std::vector<ConversationRecord>, std::string>::err("SQLite3 not available");
#endif
}

DearTs::Core::Result<std::vector<SQLiteDatabase::MessageRecord>, std::string>
SQLiteDatabase::get_messages_by_conversation(const std::string& conversation_id) {
#ifdef SQLITE3_FOUND
    std::lock_guard lock(m_mutex);

    if (!m_db) {
        return DearTs::Core::Result<std::vector<MessageRecord>, std::string>::err(
            "Database not initialized"
        );
    }

    const char* sql = R"(
        SELECT id, conversation_id, message_uuid, role, content, timestamp, tokens
        FROM messages
        WHERE conversation_id = ?
        ORDER BY timestamp ASC
    )";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return DearTs::Core::Result<std::vector<MessageRecord>, std::string>::err(
            std::format("Failed to prepare statement: {}", sqlite3_errmsg(m_db))
        );
    }

    sqlite3_bind_text(stmt, 1, conversation_id.c_str(), -1, SQLITE_TRANSIENT);

    std::vector<MessageRecord> messages;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        MessageRecord record;
        record.id = sqlite3_column_int64(stmt, 0);
        record.conversation_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        record.message_uuid = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        record.role = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        record.content = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        record.timestamp = sqlite3_column_int64(stmt, 5);

        // tokens 可能为 NULL
        if (sqlite3_column_type(stmt, 6) != SQLITE_NULL) {
            record.tokens = sqlite3_column_int(stmt, 6);
        }

        messages.push_back(std::move(record));
    }

    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return DearTs::Core::Result<std::vector<MessageRecord>, std::string>::err(
            std::format("Failed to execute statement: {}", sqlite3_errmsg(m_db))
        );
    }

    LOG_INFO("Retrieved {} messages for conversation {}", messages.size(), conversation_id);
    return DearTs::Core::Result<std::vector<MessageRecord>, std::string>::ok(messages);
#else
    return DearTs::Core::Result<std::vector<MessageRecord>, std::string>::err("SQLite3 not available");
#endif
}

std::string SQLiteDatabase::highlight_fts_result(
    const std::string& query,
    const std::string& content,
    int context_chars
) {
    // 简化版高亮实现
    // 在实际应用中，可以使用 SQLite FTS5 的 snippet() 函数或 bm25() 函数

    // 查找查询词在内容中的位置
    std::string lower_content = content;
    std::string lower_query = query;

    // 转换为小写（简化处理）
    std::transform(lower_content.begin(), lower_content.end(), lower_content.begin(), ::tolower);
    std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);

    size_t pos = lower_content.find(lower_query);
    if (pos == std::string::npos) {
        return content;  // 未找到，返回原内容
    }

    // 计算高亮范围
    size_t start = (pos > static_cast<size_t>(context_chars)) ? pos - context_chars : 0;
    size_t end = std::min(pos + query.length() + context_chars, content.length());

    // 构建高亮结果
    std::string result;
    if (start > 0) {
        result += "...";
    }
    result += content.substr(start, pos - start);
    result += "<mark>" + content.substr(pos, query.length()) + "</mark>";
    result += content.substr(pos + query.length(), end - pos - query.length());
    if (end < content.length()) {
        result += "...";
    }

    return result;
}

            } // namespace Persistence
        } // namespace MemoryCore
    } // namespace Plugins
} // namespace DearTs
