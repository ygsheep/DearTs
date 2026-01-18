/**
 * @file memory_manager.cpp
 * @brief 记忆管理器实现
 */

#include "memory_core/memory/memory_manager.hpp"
#include "memory_core/persistence/database.hpp"
#include "liblogger/logger.h"
#include <sstream>
#include <algorithm>

// SQLite3 前向声明（避免硬依赖）
struct sqlite3;
struct sqlite3_stmt;
struct sqlite3_value;

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

    // 构建 SQL
    std::string sql = build_insert_sql(memory);

    // 使用事务包装器执行（transaction 现在直接返回 Result<int64_t, std::string>）
    auto result = db.transaction([&](Persistence::SQLiteDatabase&) -> DearTs::Core::Result<int64_t, std::string> {
        // TODO: 实现 SQL 执行并返回新 ID
        // 当前占位符实现
        return DearTs::Core::Result<int64_t, std::string>::ok(1);
    });

    if (result.isErr()) {
        LOG_ERROR("Failed to add memory: {}", result.error());
        return result;
    }

    int64_t memory_id = result.unwrap();
    LOG_INFO("Memory added with ID: {}", memory_id);
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

    // TODO: 实现数据库查询
    // 占位符：返回一个空记忆
    Memory memory;
    memory.id = id;
    memory.type = MemoryType::Fact;
    memory.content = "Placeholder memory";
    memory.importance = 0.5;
    memory.created_at = 0;
    memory.accessed_count = 0;

    LOG_INFO("Retrieved memory ID: {}", id);
    return DearTs::Core::Result<Memory, std::string>::ok(memory);

#else
    LOG_WARN("get_memory called but SQLite3 not available");
    Memory placeholder;
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

    // TODO: 实现访问计数增加
    LOG_INFO("Incremented access count for memory ID: {}", id);
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

    // TODO: 实现计数查询
    return DearTs::Core::Result<size_t, std::string>::ok(0);

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

} // namespace DearTs::Plugins::MemoryCore::Memory
