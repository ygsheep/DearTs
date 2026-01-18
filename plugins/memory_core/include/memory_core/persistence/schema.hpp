/**
 * @file schema.hpp
 * @brief 数据库表结构定义和 SQL 语句
 */

#pragma once

#include <string>
#include <vector>

namespace DearTs {
    namespace Plugins {
        namespace MemoryCore {
            namespace Persistence {

/**
 * @brief 数据库表结构定义
 *
 * 包含所有表的 SQL 创建语句、索引和触发器
 */
class DatabaseSchema {
public:
    // ============ 表创建 SQL ============

    /**
     * @brief 会话表
     */
    static const char* conversations_table() {
        return R"(
            CREATE TABLE IF NOT EXISTS conversations (
                id TEXT PRIMARY KEY,
                title TEXT NOT NULL,
                type TEXT NOT NULL,
                created_at INTEGER NOT NULL,
                updated_at INTEGER NOT NULL,
                llm_model TEXT,
                temperature REAL,
                max_tokens INTEGER,
                is_pinned INTEGER DEFAULT 0,
                tags TEXT,
                metadata TEXT
            );
        )";
    }

    /**
     * @brief 消息表
     */
    static const char* messages_table() {
        return R"(
            CREATE TABLE IF NOT EXISTS messages (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                message_uuid TEXT UNIQUE NOT NULL,
                conversation_id TEXT NOT NULL,
                role TEXT NOT NULL,
                content TEXT NOT NULL,
                timestamp INTEGER NOT NULL,
                tokens INTEGER,
                embedding_id INTEGER,
                is_key_message INTEGER DEFAULT 0,
                metadata TEXT,
                FOREIGN KEY (conversation_id) REFERENCES conversations(id) ON DELETE CASCADE,
                FOREIGN KEY (embedding_id) REFERENCES embeddings(id)
            );
        )";
    }

    /**
     * @brief 全局记忆表
     */
    static const char* memories_table() {
        return R"(
            CREATE TABLE IF NOT EXISTS memories (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                type TEXT NOT NULL,
                content TEXT NOT NULL,
                source_conversation_id TEXT,
                source_message_id INTEGER,
                importance REAL DEFAULT 0.5,
                created_at INTEGER NOT NULL,
                accessed_count INTEGER DEFAULT 0,
                last_accessed_at INTEGER,
                embedding_id INTEGER,
                FOREIGN KEY (source_conversation_id) REFERENCES conversations(id) ON DELETE SET NULL,
                FOREIGN KEY (embedding_id) REFERENCES embeddings(id)
            );
        )";
    }

    /**
     * @brief 会话摘要表
     */
    static const char* conversation_summaries_table() {
        return R"(
            CREATE TABLE IF NOT EXISTS conversation_summaries (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                conversation_id TEXT NOT NULL,
                summary_text TEXT NOT NULL,
                message_range_start INTEGER,
                message_range_end INTEGER,
                summary_type TEXT NOT NULL,
                created_at INTEGER NOT NULL,
                FOREIGN KEY (conversation_id) REFERENCES conversations(id) ON DELETE CASCADE
            );
        )";
    }

    /**
     * @brief 向量嵌入表
     */
    static const char* embeddings_table() {
        return R"(
            CREATE TABLE IF NOT EXISTS embeddings (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                vector TEXT NOT NULL,
                model TEXT NOT NULL,
                dimension INTEGER NOT NULL,
                created_at INTEGER NOT NULL
            );
        )";
    }

    /**
     * @brief FTS5 全文搜索虚拟表
     */
    static const char* messages_fts_table() {
        return R"(
            CREATE VIRTUAL TABLE IF NOT EXISTS messages_fts USING fts5(
                content,
                conversation_id,
                tokenize = 'porter unicode61'
            );
        )";
    }

    // ============ 索引创建 SQL ============

    /**
     * @brief 创建所有索引
     */
    static std::vector<const char*> indexes() {
        return {
            "CREATE INDEX IF NOT EXISTS idx_messages_conversation ON messages(conversation_id, timestamp);",
            "CREATE INDEX IF NOT EXISTS idx_messages_uuid ON messages(message_uuid);",
            "CREATE INDEX IF NOT EXISTS idx_memories_type_importance ON memories(type, importance DESC);",
            "CREATE INDEX IF NOT EXISTS idx_summaries_conversation ON conversation_summaries(conversation_id);",
            "CREATE INDEX IF NOT EXISTS idx_embeddings_model ON embeddings(model);",
        };
    }

    // ============ 触发器 SQL ============

    /**
     * @brief 创建 FTS5 同步触发器（需要 FTS5 支持）
     */
    static std::vector<const char*> fts5_triggers() {
        return {
            // 消息插入时同步到 FTS5
            R"(
                CREATE TRIGGER IF NOT EXISTS messages_fts_insert
                AFTER INSERT ON messages BEGIN
                    INSERT INTO messages_fts(rowid, content, conversation_id)
                    VALUES (new.id, new.content, new.conversation_id);
                END;
            )",

            // 消息删除时从 FTS5 删除
            R"(
                CREATE TRIGGER IF NOT EXISTS messages_fts_delete
                AFTER DELETE ON messages BEGIN
                    DELETE FROM messages_fts WHERE rowid = old.id;
                END;
            )",

            // 消息更新时同步到 FTS5
            R"(
                CREATE TRIGGER IF NOT EXISTS messages_fts_update
                AFTER UPDATE ON messages BEGIN
                    UPDATE messages_fts SET content = new.content, conversation_id = new.conversation_id
                    WHERE rowid = new.id;
                END;
            )",
        };
    }

    /**
     * @brief 获取所有初始化 SQL
     */
    static std::vector<std::string> get_init_sql() {
        std::vector<std::string> sql;

        // 创建表
        sql.push_back(conversations_table());
        sql.push_back(messages_table());
        sql.push_back(memories_table());
        sql.push_back(conversation_summaries_table());
        sql.push_back(embeddings_table());
        sql.push_back(messages_fts_table());

        // 创建索引
        for (const char* index_sql : indexes()) {
            sql.push_back(index_sql);
        }

        // 创建触发器
        for (const char* trigger_sql : fts5_triggers()) {
            sql.push_back(trigger_sql);
        }

        return sql;
    }

    /**
     * @brief 获取不依赖 FTS5 的初始化 SQL（基础表和索引）
     */
    static std::vector<std::string> get_basic_init_sql() {
        std::vector<std::string> sql;

        // 创建基础表（不包括 FTS5）
        sql.push_back(conversations_table());
        sql.push_back(messages_table());
        sql.push_back(memories_table());
        sql.push_back(conversation_summaries_table());
        sql.push_back(embeddings_table());

        // 创建索引
        for (const char* index_sql : indexes()) {
            sql.push_back(index_sql);
        }

        return sql;
    }

    /**
     * @brief 数据库版本
     */
    static constexpr int CURRENT_VERSION = 1;
};

            } // namespace Persistence
        } // namespace MemoryCore
    } // namespace Plugins
} // namespace DearTs
