/**
 * @file memory_manager.cpp
 * @brief 记忆管理器实现
 */

#include "memory_core/memory/memory_manager.hpp"
#include "memory_core/persistence/database.hpp"
#include "memory_core/rag/embedding_provider.hpp"
#include "liblogger/logger.h"
#include <sstream>
#include <algorithm>
#include <format>

// SQLite3 头文件 - 条件编译
#ifdef SQLITE3_FOUND
    #include <sqlite3.h>
#endif

namespace DearTs::Plugins::MemoryCore::Memory {

// ============ 单例实现 ============

MemoryManager& MemoryManager::instance() {
    static MemoryManager instance;
    return instance;
}

// ============ CRUD 操作实现 ============

DearTs::Core::Result<int64_t, std::string> MemoryManager::add_memory(const Memory& memory) {
#ifdef SQLITE3_FOUND
    auto& db = Persistence::SQLiteDatabase::instance();

    if (!db.is_open()) {
        return DearTs::Core::Result<int64_t, std::string>::err("Database not initialized");
    }

    // 获取原始数据库指针
    sqlite3* sqlite_db = db.get_db();
    if (!sqlite_db) {
        return DearTs::Core::Result<int64_t, std::string>::err("Invalid database handle");
    }

    // 构建 INSERT SQL
    const char* sql = R"(
        INSERT INTO memories (type, content, source_conversation_id, source_message_id,
                           importance, created_at, accessed_count)
        VALUES (?, ?, ?, ?, ?, ?, ?)
    )";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(sqlite_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return DearTs::Core::Result<int64_t, std::string>::err(
            std::format("Failed to prepare statement: {}", sqlite3_errmsg(sqlite_db))
        );
    }

    // 绑定参数
    std::string type_str = Memory::type_to_string(memory.type);
    sqlite3_bind_text(stmt, 1, type_str.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, memory.content.c_str(), -1, SQLITE_TRANSIENT);

    // 可选参数：source_conversation_id
    if (memory.source_conversation_id.has_value()) {
        sqlite3_bind_text(stmt, 3, memory.source_conversation_id.value().c_str(), -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt, 3);
    }

    // 可选参数：source_message_id
    if (memory.source_message_id.has_value()) {
        sqlite3_bind_int64(stmt, 4, memory.source_message_id.value());
    } else {
        sqlite3_bind_null(stmt, 4);
    }

    sqlite3_bind_double(stmt, 5, memory.importance);
    sqlite3_bind_int64(stmt, 6, memory.created_at);
    sqlite3_bind_int(stmt, 7, memory.accessed_count);

    // 执行插入
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return DearTs::Core::Result<int64_t, std::string>::err(
            std::format("Failed to execute statement: {}", sqlite3_errmsg(sqlite_db))
        );
    }

    int64_t memory_id = sqlite3_last_insert_rowid(sqlite_db);
    LOG_INFO("Memory added with ID: {}", memory_id);

    // 异步生成嵌入向量（如果有提供者）
    if (m_embedding_provider) {
        // 在后台任务中生成嵌入
        auto embed_result = generate_and_store_embedding(memory_id);
        if (embed_result.isErr()) {
            LOG_WARN("Failed to generate embedding for memory {}: {}", memory_id, embed_result.error());
        }
    }

    return DearTs::Core::Result<int64_t, std::string>::ok(memory_id);

#else
    LOG_WARN("add_memory called but SQLite3 not available");
    return DearTs::Core::Result<int64_t, std::string>::ok(0);
#endif
}

DearTs::Core::Result<size_t, std::string> MemoryManager::add_memories(const std::vector<Memory>& memories) {
#ifdef SQLITE3_FOUND
    auto& db = Persistence::SQLiteDatabase::instance();

    if (!db.is_open()) {
        return DearTs::Core::Result<size_t, std::string>::err("Database not initialized");
    }

    size_t added_count = 0;

    // 使用事务批量添加（transaction 现在直接返回 Result<size_t, std::string>）
    auto result = db.transaction([&](Persistence::SQLiteDatabase&) -> DearTs::Core::Result<size_t, std::string> {
        for (const auto& memory : memories) {
            auto mem_result = add_memory(memory);
            if (mem_result.isOk()) {
                added_count++;
            }
        }
        return DearTs::Core::Result<size_t, std::string>::ok(added_count);
    });

    if (result.isErr()) {
        LOG_ERROR("Failed to add memories: {}", result.error());
        return result;
    }

    size_t count = result.unwrap();
    LOG_INFO("Added {} memories", count);
    return DearTs::Core::Result<size_t, std::string>::ok(count);

#else
    LOG_WARN("add_memories called but SQLite3 not available");
    return DearTs::Core::Result<size_t, std::string>::ok(memories.size());
#endif
}

DearTs::Core::Result<Memory, std::string> MemoryManager::get_memory(int64_t id) {
#ifdef SQLITE3_FOUND
    auto& db = Persistence::SQLiteDatabase::instance();

    if (!db.is_open()) {
        return DearTs::Core::Result<Memory, std::string>::err("Database not initialized");
    }

    // 获取原始数据库指针
    sqlite3* sqlite_db = db.get_db();
    if (!sqlite_db) {
        return DearTs::Core::Result<Memory, std::string>::err("Invalid database handle");
    }

    const char* sql = R"(
        SELECT id, type, content, source_conversation_id, source_message_id,
               importance, created_at, accessed_count, last_accessed_at, embedding_id
        FROM memories
        WHERE id = ?
    )";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(sqlite_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return DearTs::Core::Result<Memory, std::string>::err(
            std::format("Failed to prepare statement: {}", sqlite3_errmsg(sqlite_db))
        );
    }

    // 绑定参数
    sqlite3_bind_int64(stmt, 1, id);

    // 执行查询
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        if (rc == SQLITE_DONE) {
            return DearTs::Core::Result<Memory, std::string>::err(
                std::format("Memory not found: {}", id)
            );
        }
        return DearTs::Core::Result<Memory, std::string>::err(
            std::format("Failed to execute statement: {}", sqlite3_errmsg(sqlite_db))
        );
    }

    // 提取数据
    Memory memory;
    memory.id = sqlite3_column_int64(stmt, 0);

    const char* type_str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    memory.type = Memory::string_to_type(type_str ? type_str : "fact");

    const char* content = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    memory.content = content ? content : "";

    // source_conversation_id (可选)
    if (sqlite3_column_type(stmt, 3) != SQLITE_NULL) {
        const char* conv_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        memory.source_conversation_id = std::string(conv_id ? conv_id : "");
    }

    // source_message_id (可选)
    if (sqlite3_column_type(stmt, 4) != SQLITE_NULL) {
        memory.source_message_id = sqlite3_column_int64(stmt, 4);
    }

    memory.importance = sqlite3_column_double(stmt, 5);
    memory.created_at = sqlite3_column_int64(stmt, 6);
    memory.accessed_count = sqlite3_column_int(stmt, 7);

    // last_accessed_at (可选)
    if (sqlite3_column_type(stmt, 8) != SQLITE_NULL) {
        memory.last_accessed_at = sqlite3_column_int64(stmt, 8);
    }

    // embedding_id (可选)
    if (sqlite3_column_type(stmt, 9) != SQLITE_NULL) {
        memory.embedding_id = sqlite3_column_int64(stmt, 9);
    }

    sqlite3_finalize(stmt);

    // 更新访问计数（异步）
    increment_access_count(id);

    LOG_INFO("Retrieved memory ID: {}, type: {}", id, Memory::type_to_string(memory.type));
    return DearTs::Core::Result<Memory, std::string>::ok(memory);

#else
    LOG_WARN("get_memory called but SQLite3 not available");
    Memory placeholder;
    placeholder.id = id;
    placeholder.type = MemoryType::Fact;
    placeholder.content = "SQLite3 not available";
    placeholder.importance = 0.0;
    placeholder.created_at = 0;
    placeholder.accessed_count = 0;
    return DearTs::Core::Result<Memory, std::string>::ok(placeholder);
#endif
}

DearTs::Core::Result<void, std::string> MemoryManager::update_memory(int64_t id, const Memory& memory) {
#ifdef SQLITE3_FOUND
    auto& db = Persistence::SQLiteDatabase::instance();

    if (!db.is_open()) {
        return DearTs::Core::Result<void, std::string>::err("Database not initialized");
    }

    // TODO: 实现数据库更新
    LOG_INFO("Updated memory ID: {}", id);
    return DearTs::Core::Result<void, std::string>::ok();

#else
    LOG_WARN("update_memory called but SQLite3 not available");
    return DearTs::Core::Result<void, std::string>::ok();
#endif
}

DearTs::Core::Result<void, std::string> MemoryManager::delete_memory(int64_t id) {
#ifdef SQLITE3_FOUND
    auto& db = Persistence::SQLiteDatabase::instance();

    if (!db.is_open()) {
        return DearTs::Core::Result<void, std::string>::err("Database not initialized");
    }

    // TODO: 实现数据库删除
    LOG_INFO("Deleted memory ID: {}", id);
    return DearTs::Core::Result<void, std::string>::ok();

#else
    LOG_WARN("delete_memory called but SQLite3 not available");
    return DearTs::Core::Result<void, std::string>::ok();
#endif
}

// ============ 搜索操作实现 ============

DearTs::Core::Result<std::vector<Memory>, std::string> MemoryManager::search_memories(const MemoryFilter& filter) {
#ifdef SQLITE3_FOUND
    auto& db = Persistence::SQLiteDatabase::instance();

    if (!db.is_open()) {
        return DearTs::Core::Result<std::vector<Memory>, std::string>::err("Database not initialized");
    }

    // TODO: 实现带过滤器的搜索
    LOG_INFO("Searching memories with filter");
    std::vector<Memory> results;
    return DearTs::Core::Result<std::vector<Memory>, std::string>::ok(results);

#else
    LOG_WARN("search_memories called but SQLite3 not available");
    return DearTs::Core::Result<std::vector<Memory>, std::string>::ok({});
#endif
}

DearTs::Core::Result<std::vector<Memory>, std::string> MemoryManager::get_all_memories(int limit, int offset) {
#ifdef SQLITE3_FOUND
    auto& db = Persistence::SQLiteDatabase::instance();

    if (!db.is_open()) {
        return DearTs::Core::Result<std::vector<Memory>, std::string>::err("Database not initialized");
    }

    // TODO: 实现分页查询
    LOG_INFO("Getting all memories: limit={}, offset={}", limit, offset);
    std::vector<Memory> results;
    return DearTs::Core::Result<std::vector<Memory>, std::string>::ok(results);

#else
    LOG_WARN("get_all_memories called but SQLite3 not available");
    return DearTs::Core::Result<std::vector<Memory>, std::string>::ok({});
#endif
}

DearTs::Core::Result<std::vector<Memory>, std::string> MemoryManager::get_memories_by_type(MemoryType type, int limit) {
#ifdef SQLITE3_FOUND
    auto& db = Persistence::SQLiteDatabase::instance();

    if (!db.is_open()) {
        return DearTs::Core::Result<std::vector<Memory>, std::string>::err("Database not initialized");
    }

    // TODO: 实现按类型查询
    std::string type_str = Memory::type_to_string(type);
    LOG_INFO("Getting memories by type: {}", type_str);
    std::vector<Memory> results;
    return DearTs::Core::Result<std::vector<Memory>, std::string>::ok(results);

#else
    LOG_WARN("get_memories_by_type called but SQLite3 not available");
    return DearTs::Core::Result<std::vector<Memory>, std::string>::ok({});
#endif
}

DearTs::Core::Result<std::vector<Memory>, std::string> MemoryManager::get_memories_by_importance(double min_importance, int limit) {
#ifdef SQLITE3_FOUND
    auto& db = Persistence::SQLiteDatabase::instance();

    if (!db.is_open()) {
        return DearTs::Core::Result<std::vector<Memory>, std::string>::err("Database not initialized");
    }

    // TODO: 实现按重要性查询
    LOG_INFO("Getting memories with importance >= {}", min_importance);
    std::vector<Memory> results;
    return DearTs::Core::Result<std::vector<Memory>, std::string>::ok(results);

#else
    LOG_WARN("get_memories_by_importance called but SQLite3 not available");
    return DearTs::Core::Result<std::vector<Memory>, std::string>::ok({});
#endif
}

DearTs::Core::Result<std::vector<Memory>, std::string> MemoryManager::get_top_memories(int limit) {
#ifdef SQLITE3_FOUND
    auto& db = Persistence::SQLiteDatabase::instance();

    if (!db.is_open()) {
        return DearTs::Core::Result<std::vector<Memory>, std::string>::err("Database not initialized");
    }

    // TODO: 实现获取重要记忆（按访问次数和重要性排序）
    LOG_INFO("Getting top {} memories", limit);
    std::vector<Memory> results;
    return DearTs::Core::Result<std::vector<Memory>, std::string>::ok(results);

#else
    LOG_WARN("get_top_memories called but SQLite3 not available");
    return DearTs::Core::Result<std::vector<Memory>, std::string>::ok({});
#endif
}

// ============ 统计操作实现 ============

DearTs::Core::Result<void, std::string> MemoryManager::increment_access_count(int64_t id) {
#ifdef SQLITE3_FOUND
    auto& db = Persistence::SQLiteDatabase::instance();

    if (!db.is_open()) {
        return DearTs::Core::Result<void, std::string>::err("Database not initialized");
    }

    // 获取原始数据库指针
    sqlite3* sqlite_db = db.get_db();
    if (!sqlite_db) {
        return DearTs::Core::Result<void, std::string>::err("Invalid database handle");
    }

    // 获取当前时间戳（毫秒）
    auto now = std::chrono::system_clock::now();
    auto timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count();

    const char* sql = R"(
        UPDATE memories
        SET accessed_count = accessed_count + 1,
            last_accessed_at = ?
        WHERE id = ?
    )";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(sqlite_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return DearTs::Core::Result<void, std::string>::err(
            std::format("Failed to prepare statement: {}", sqlite3_errmsg(sqlite_db))
        );
    }

    // 绑定参数
    sqlite3_bind_int64(stmt, 1, timestamp_ms);
    sqlite3_bind_int64(stmt, 2, id);

    // 执行更新
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return DearTs::Core::Result<void, std::string>::err(
            std::format("Failed to execute statement: {}", sqlite3_errmsg(sqlite_db))
        );
    }

    LOG_DEBUG("Incremented access count for memory ID: {}", id);
    return DearTs::Core::Result<void, std::string>::ok();

#else
    LOG_WARN("increment_access_count called but SQLite3 not available");
    return DearTs::Core::Result<void, std::string>::ok();
#endif
}

DearTs::Core::Result<void, std::string> MemoryManager::update_importance(int64_t id, double importance) {
#ifdef SQLITE3_FOUND
    auto& db = Persistence::SQLiteDatabase::instance();

    if (!db.is_open()) {
        return DearTs::Core::Result<void, std::string>::err("Database not initialized");
    }

    // 验证重要性范围
    if (importance < 0.0 || importance > 1.0) {
        return DearTs::Core::Result<void, std::string>::err(
            "Importance must be between 0 and 1"
        );
    }

    // TODO: 实现重要性更新
    LOG_INFO("Updated importance for memory ID: {} to {}", id, importance);
    return DearTs::Core::Result<void, std::string>::ok();

#else
    LOG_WARN("update_importance called but SQLite3 not available");
    return DearTs::Core::Result<void, std::string>::ok();
#endif
}

DearTs::Core::Result<size_t, std::string> MemoryManager::get_memory_count() {
#ifdef SQLITE3_FOUND
    auto& db = Persistence::SQLiteDatabase::instance();

    if (!db.is_open()) {
        return DearTs::Core::Result<size_t, std::string>::err("Database not initialized");
    }

    // 获取原始数据库指针
    sqlite3* sqlite_db = db.get_db();
    if (!sqlite_db) {
        return DearTs::Core::Result<size_t, std::string>::err("Invalid database handle");
    }

    const char* sql = "SELECT COUNT(*) FROM memories";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(sqlite_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return DearTs::Core::Result<size_t, std::string>::err(
            std::format("Failed to prepare statement: {}", sqlite3_errmsg(sqlite_db))
        );
    }

    // 执行查询
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return DearTs::Core::Result<size_t, std::string>::err(
            std::format("Failed to execute statement: {}", sqlite3_errmsg(sqlite_db))
        );
    }

    size_t count = static_cast<size_t>(sqlite3_column_int64(stmt, 0));
    sqlite3_finalize(stmt);

    LOG_INFO("Memory count: {}", count);
    return DearTs::Core::Result<size_t, std::string>::ok(count);

#else
    LOG_WARN("get_memory_count called but SQLite3 not available");
    return DearTs::Core::Result<size_t, std::string>::ok(0);
#endif
}

DearTs::Core::Result<std::vector<std::pair<MemoryType, size_t>>, std::string> MemoryManager::get_memory_count_by_type() {
#ifdef SQLITE3_FOUND
    auto& db = Persistence::SQLiteDatabase::instance();

    if (!db.is_open()) {
        return DearTs::Core::Result<std::vector<std::pair<MemoryType, size_t>>, std::string>::err(
            "Database not initialized"
        );
    }

    // TODO: 实现按类型统计
    std::vector<std::pair<MemoryType, size_t>> counts;
    return DearTs::Core::Result<std::vector<std::pair<MemoryType, size_t>>, std::string>::ok(counts);

#else
    LOG_WARN("get_memory_count_by_type called but SQLite3 not available");
    return DearTs::Core::Result<std::vector<std::pair<MemoryType, size_t>>, std::string>::ok({});
#endif
}

// ============ 私有辅助方法 ============

std::string MemoryManager::build_insert_sql(const Memory& memory) {
    std::ostringstream sql;

    sql << "INSERT INTO memories (type, content";

    if (memory.source_conversation_id.has_value()) {
        sql << ", source_conversation_id";
    }
    if (memory.source_message_id.has_value()) {
        sql << ", source_message_id";
    }

    sql << ", importance, created_at, accessed_count";

    if (memory.last_accessed_at.has_value()) {
        sql << ", last_accessed_at";
    }
    if (memory.embedding_id.has_value()) {
        sql << ", embedding_id";
    }

    sql << ") VALUES ('"
        << Memory::type_to_string(memory.type) << "', '"
        << memory.content << "'";

    if (memory.source_conversation_id.has_value()) {
        sql << ", '" << memory.source_conversation_id.value() << "'";
    }
    if (memory.source_message_id.has_value()) {
        sql << ", " << memory.source_message_id.value();
    }

    sql << ", " << memory.importance
        << ", " << memory.created_at
        << ", " << memory.accessed_count;

    if (memory.last_accessed_at.has_value()) {
        sql << ", " << memory.last_accessed_at.value();
    }
    if (memory.embedding_id.has_value()) {
        sql << ", " << memory.embedding_id.value();
    }

    sql << ");";

    return sql.str();
}

DearTs::Core::Result<Memory, std::string> MemoryManager::memory_from_db_row(void* stmt) {
    // TODO: 从 sqlite3_stmt 读取数据并构建 Memory 对象
    Memory memory;
    return DearTs::Core::Result<Memory, std::string>::ok(memory);
}

// ============ 嵌入向量操作实现 ============

void MemoryManager::set_embedding_provider(std::unique_ptr<RAG::IEmbeddingProvider> provider) {
    m_embedding_provider = std::move(provider);
    LOG_INFO("MemoryManager: Embedding provider set");
}

DearTs::Core::Result<int64_t, std::string> MemoryManager::generate_and_store_embedding(int64_t memory_id) {
#ifdef SQLITE3_FOUND
    // 检查提供者
    if (!m_embedding_provider) {
        return DearTs::Core::Result<int64_t, std::string>::err("No embedding provider available");
    }

    auto& db = Persistence::SQLiteDatabase::instance();
    if (!db.is_open()) {
        return DearTs::Core::Result<int64_t, std::string>::err("Database not initialized");
    }

    // 获取记忆内容
    auto memory_result = get_memory(memory_id);
    if (memory_result.isErr()) {
        return DearTs::Core::Result<int64_t, std::string>::err(memory_result.error());
    }

    auto& memory = memory_result.unwrap();

    // 生成嵌入向量
    auto embedding_result = m_embedding_provider->generate_embedding(memory.content);
    if (embedding_result.isErr()) {
        return DearTs::Core::Result<int64_t, std::string>::err(
            "Failed to generate embedding: " + embedding_result.error()
        );
    }

    auto& embedding_vector = embedding_result.unwrap();

    // 存储嵌入向量到数据库
    auto now = std::chrono::system_clock::now();
    auto timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count();

    auto json_data = embedding_vector.to_json();

    auto embed_id_result = db.insert_embedding(
        embedding_vector.model,
        json_data,
        embedding_vector.dimension,
        timestamp_ms
    );

    if (embed_id_result.isErr()) {
        return DearTs::Core::Result<int64_t, std::string>::err(
            "Failed to store embedding: " + embed_id_result.error()
        );
    }

    int64_t embedding_id = embed_id_result.unwrap();

    // 更新记忆的 embedding_id
    sqlite3* sqlite_db = db.get_db();
    if (!sqlite_db) {
        return DearTs::Core::Result<int64_t, std::string>::err("Invalid database handle");
    }

    const char* update_sql = R"(
        UPDATE memories SET embedding_id = ? WHERE id = ?
    )";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(sqlite_db, update_sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return DearTs::Core::Result<int64_t, std::string>::err(
            std::format("Failed to prepare statement: {}", sqlite3_errmsg(sqlite_db))
        );
    }

    sqlite3_bind_int64(stmt, 1, embedding_id);
    sqlite3_bind_int64(stmt, 2, memory_id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return DearTs::Core::Result<int64_t, std::string>::err(
            std::format("Failed to update memory: {}", sqlite3_errmsg(sqlite_db))
        );
    }

    LOG_INFO("Generated and stored embedding for memory {}: embedding_id={}", memory_id, embedding_id);
    return DearTs::Core::Result<int64_t, std::string>::ok(embedding_id);

#else
    return DearTs::Core::Result<int64_t, std::string>::err("SQLite3 not available");
#endif
}

} // namespace DearTs::Plugins::MemoryCore::Memory
