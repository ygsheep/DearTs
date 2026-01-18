# Memory Core Plugin

> DearTs Framework 的持久化记忆管理和 RAG（检索增强生成）服务插件

## 📋 目录

- [概述](#概述)
- [核心功能](#核心功能)
- [架构设计](#架构设计)
- [快速开始](#快速开始)
- [API 参考](#api-参考)
- [事件系统](#事件系统)
- [配置选项](#配置选项)
- [开发指南](#开发指南)

---

## 概述

Memory Core 插件为 DearTs Framework 提供跨会话的持久化记忆管理能力。通过 SQLite 数据库存储、向量嵌入语义检索和智能记忆提取，实现 AI 助手的长期记忆功能。

### 主要特性

- **持久化存储**：基于 SQLite3 + FTS5 全文搜索
- **语义检索**：通过 Ollama 嵌入模型实现 RAG 检索
- **智能提取**：LLM 驱动的记忆提取，支持规则引擎降级
- **事件驱动**：与 Chat 等插件通过 EventBus 松耦合集成
- **多类型记忆**：支持偏好、事实、问答、上下文、技能五种记忆类型
- **重要性评分**：自动评估记忆重要性，支持访问频率统计
- **离线队列**：网络故障时支持离线操作，恢复后自动同步

---

## 核心功能

### 1. 记忆管理 (MemoryManager)

```cpp
using Mem = DearTs::Plugins::MemoryCore::Memory::Memory;
using MemType = DearTs::Plugins::MemoryCore::Memory::MemoryType;

// 添加记忆
Mem memory{
    .type = MemType::Preference,
    .content = "用户偏好深色主题",
    .importance = 0.8
};
auto result = MemoryManager::instance().add_memory(memory);

// 搜索记忆
MemoryFilter filter;
filter.type = MemType::Preference;
filter.min_importance = 0.5;
auto memories = MemoryManager::instance().search_memories(filter);
```

### 2. RAG 语义检索 (RAGService)

```cpp
// 执行语义检索
RAGQuery query{
    .text = "用户喜欢什么样的界面主题？",
    .top_k = 5,
    .min_similarity = 0.6
};
auto results = RAGService::instance().query(query);

// 带上下文增强
auto context = ContextBuilder::instance().build(
    query.text,
    results.memories,
    2000  // 最大 token 数
);
```

### 3. 记忆提取 (LLMMemoryExtractor)

```cpp
// 从对话消息中提取记忆
LLMMemoryExtractor extractor;
std::vector<std::string> messages = {
    "我更喜欢深色的编辑器主题",
    "字体大小设置为 14 比较合适"
};

auto extracted = extractor.extract_memories(
    messages,
    "conv_123",
    true  // 使用 LLM 提取
);

// 自动降级到规则引擎（LLM 不可用时）
for (const auto& mem : extracted) {
    MemoryManager::instance().add_memory(mem);
}
```

### 4. 数据库持久化 (SQLiteDatabase)

```cpp
// 初始化数据库
auto db_result = SQLiteDatabase::instance().initialize(
    "data/memory.db"
);

// 事务操作
SQLiteDatabase::instance().begin_transaction();
// ... 执行多个 SQL 操作 ...
SQLiteDatabase::instance().commit();
```

---

## 架构设计

### 系统架构图

```
┌─────────────────────────────────────────────────────────────────┐
│                      Memory Core Plugin                         │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │                    Event Layer                           │   │
│  │  EventBus: MessageSaveRequested │ RAGQueryRequested      │   │
│  │           │ MemoryExtractRequested │ SummaryRequested     │   │
│  └──────────────────────┬───────────────────────────────────┘   │
│                         │                                       │
│  ┌──────────────────────▼───────────────────────────────────┐   │
│  │                  Service Layer                           │   │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │   │
│  │  │ MemoryManager│  │  RAGService  │  │  Summarizer  │  │   │
│  │  │              │  │              │  │              │  │   │
│  │  │ - CRUD      │  │ - Embedding  │  │ - LLM Summ. │  │   │
│  │  │ - Search    │  │ - Semantic   │  │ - Topic Seg. │  │   │
│  │  │ - Import.   │  │   Retrieval  │  │              │  │   │
│  │  └──────────────┘  └──────────────┘  └──────────────┘  │   │
│  │                                                          │   │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │   │
│  │  │   Extractor  │  │  Consistency │  │   Context    │  │   │
│  │  │              │  │   Manager    │  │   Builder    │  │   │
│  │  │ - LLM       │  │              │  │              │  │   │
│  │  │ - Rules     │  │ - Offline Q  │  │ - Token Limit│  │   │
│  │  │ - Fallback  │  │ - Retry      │  │ - Priority   │  │   │
│  │  └──────────────┘  └──────────────┘  └──────────────┘  │   │
│  └──────────────────────┬───────────────────────────────────┘   │
│                         │                                       │
│  ┌──────────────────────▼───────────────────────────────────┐   │
│  │                  Persistence Layer                        │   │
│  │  ┌──────────────────────────────────────────────────┐   │   │
│  │  │              SQLiteDatabase                      │   │   │
│  │  │  ┌──────────┐ ┌──────────┐ ┌──────────┐        │   │   │
│  │  │  │ Memories │ │Embeddings│ │ Summaries│        │   │   │
│  │  │  │  Table   │ │  Table   │ │  Table   │        │   │   │
│  │  │  └──────────┘ └──────────┘ └──────────┘        │   │   │
│  │  │                                                  │   │   │
│  │  │  • FTS5 Full-Text Search                        │   │   │
│  │  │  • WAL Mode for Concurrency                     │   │   │
│  │  │  • Custom SQL Functions (cosine similarity)     │   │   │
│  │  └──────────────────────────────────────────────────┘   │   │
│  └───────────────────────────────────────────────────────────┘   │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│                     External Dependencies                        │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐      │
│  │  SQLite3 │  │  Ollama  │  │   LLM    │  │  DearTs  │      │
│  │          │  │   API    │  │ Provider │  │  Core    │      │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘      │
└─────────────────────────────────────────────────────────────────┘
```

### 数据流程图

```
┌──────────┐
│  User    │
│ Message  │
└────┬─────┘
     │
     ▼
┌─────────────────────────────────────────────────────────────┐
│                     Chat Plugin                              │
│  ┌─────────────────────────────────────────────────────┐   │
│  │  Publish: MessageSaveRequestedEvent                  │   │
│  └─────────────────────────────────────────────────────┘   │
└────────────────────┬────────────────────────────────────────┘
                     │ EventBus
                     ▼
┌─────────────────────────────────────────────────────────────┐
│                  Memory Core Plugin                          │
│  ┌─────────────────────────────────────────────────────┐   │
│  │  Subscribe: MessageSaveRequestedEvent               │   │
│  │  Handler: handle_message_save_requested()           │   │
│  └────────────────────────┬────────────────────────────┘   │
│                           │                                  │
│  ┌────────────────────────▼────────────────────────────┐   │
│  │        LLMMemoryExtractor                           │   │
│  │  ┌──────────────────────────────────────────────┐  │   │
│  │  │ 1. Call Ollama LLM for extraction            │  │   │
│  │  │    Prompt: "Extract memories from chat..."   │  │   │
│  │  │                                              │  │   │
│  │  │ 2. Parse LLM response (JSON format)          │  │   │
│  │  │    {                                         │  │   │
│  │  │      "memories": [                           │  │   │
│  │  │        {"type": "preference",                │  │   │
│  │  │         "content": "用户偏好深色主题",        │  │   │
│  │  │         "importance": 0.8}                   │  │   │
│  │  │      ]                                        │  │   │
│  │  │    }                                         │  │   │
│  │  │                                              │  │   │
│  │  │ 3. Fallback to Rule Engine if LLM fails     │  │   │
│  │  └──────────────────────────────────────────────┘  │   │
│  └────────────────────────┬────────────────────────────┘   │
│                           │                                  │
│  ┌────────────────────────▼────────────────────────────┐   │
│  │        MemoryManager                                │   │
│  │  ┌──────────────────────────────────────────────┐  │   │
│  │  │ 1. Validate memory content                   │  │   │
│  │  │ 2. Calculate importance score                │  │   │
│  │  │ 3. Store to SQLiteDatabase                  │  │   │
│  │  │ 4. Return memory ID                          │  │   │
│  │  └──────────────────────────────────────────────┘  │   │
│  └────────────────────────┬────────────────────────────┘   │
│                           │                                  │
│  ┌────────────────────────▼────────────────────────────┐   │
│  │        SQLiteDatabase                              │   │
│  │  ┌──────────────────────────────────────────────┐  │   │
│  │  │ INSERT INTO memories (                       │  │   │
│  │  │   type, content, importance,                 │  │   │
│  │  │   created_at, source_conversation_id         │  │   │
│  │  │ ) VALUES (?, ?, ?, ?, ?)                     │  │   │
│  │  └──────────────────────────────────────────────┘  │   │
│  │                                                      │   │
│  │  ┌──────────────────────────────────────────────┐  │   │
│  │  │  RAG: Generate Embedding                     │  │   │
│  │  │  1. Call Ollama /api/embed                  │  │   │
│  │  │  2. Store vector in embeddings table         │  │   │
│  │  └──────────────────────────────────────────────┘  │   │
│  └────────────────────────┬────────────────────────────┘   │
└───────────────────────────┼──────────────────────────────────┘
                            │
                            ▼
                   ┌─────────────────┐
                   │  SQLite Database │
                   │  - memories      │
                   │  - embeddings    │
                   │  - summaries     │
                   └─────────────────┘
```

### 组件交互图

```
┌──────────────────┐
│  Chat Plugin     │
└────────┬─────────┘
         │ RAGQueryRequestedEvent
         │ { query: "用户喜欢什么主题？" }
         ▼
┌───────────────────────────────────────────────────────────┐
│                   RAGService                               │
│  ┌─────────────────────────────────────────────────────┐  │
│  │  1. Generate query embedding                       │  │
│  │     OllamaLLMProvider::generate_embedding(query)   │  │
│  └─────────────────────────────────────────────────────┘  │
│  ┌─────────────────────────────────────────────────────┐  │
│  │  2. Search similar memories                        │  │
│  │     SELECT * FROM memories                         │  │
│  │     WHERE cosine_similarity(embedding, ?) > 0.7    │  │
│  └─────────────────────────────────────────────────────┘  │
│  ┌─────────────────────────────────────────────────────┐  │
│  │  3. Build context with ContextBuilder              │  │
│  │     - Prioritize by importance                     │  │
│  │     - Limit token count                            │  │
│  └─────────────────────────────────────────────────────┘  │
│  ┌─────────────────────────────────────────────────────┐  │
│  │  4. Publish RAGQueryCompletedEvent                 │  │
│  └─────────────────────────────────────────────────────┘  │
└───────────────────────────┬───────────────────────────────┘
                            │ RAGQueryCompletedEvent
                            │ { results: [...], context: "..." }
                            ▼
┌──────────────────┐
│  Chat Plugin     │
│  (Use context    │
│   for LLM call)  │
└──────────────────┘
```

---

## 快速开始

### 1. 初始化插件

Memory Core 插件会在 `on_load()` 时自动初始化：

```cpp
auto result = plugin.on_load();
if (result.isOk()) {
    // 数据库已初始化
    // 事件监听器已注册
}
```

### 2. 基础使用

#### 添加记忆

```cpp
#include "memory_core/memory/memory_manager.hpp"

using namespace DearTs::Plugins::MemoryCore;

Memory::Memory mem{
    .id = 0,  // 自动生成
    .type = Memory::MemoryType::Preference,
    .content = "用户偏好深色 IDE 主题",
    .importance = 0.8
};

auto add_result = Memory::MemoryManager::instance().add_memory(mem);
if (add_result.isOk()) {
    int64_t memory_id = add_result.unwrap();
    LOG_INFO("记忆已添加，ID: {}", memory_id);
}
```

#### 检索记忆

```cpp
Memory::MemoryFilter filter;
filter.content_contains = "主题";
filter.min_importance = 0.5;
filter.limit = 10;

auto search_result = Memory::MemoryManager::instance().search_memories(filter);
if (search_result.isOk()) {
    auto memories = search_result.unwrap();
    for (const auto& mem : memories) {
        LOG_INFO("找到: {} (重要性: {})", mem.content, mem.importance);
    }
}
```

#### RAG 查询

```cpp
#include "memory_core/rag/rag_service.hpp"

RAG::RAGQuery query{
    .text = "用户喜欢什么样的界面配置？",
    .top_k = 5,
    .min_similarity = 0.6
};

auto result = RAG::RAGService::instance().query(query);
if (result.isOk()) {
    auto rag_result = result.unwrap();
    LOG_INFO("检索到 {} 条相关记忆", rag_result.memories.size());
    LOG_INFO("上下文: {}", rag_result.context);
}
```

---

## API 参考

### MemoryManager

#### 单例访问

```cpp
static MemoryManager& instance();
```

#### 添加记忆

```cpp
Result<int64_t, std::string> add_memory(const Memory& memory);
```

#### 获取记忆

```cpp
Result<Memory, std::string> get_memory(int64_t id);
```

#### 更新记忆

```cpp
Result<void, std::string> update_memory(int64_t id, const Memory& memory);
```

#### 删除记忆

```cpp
Result<void, std::string> delete_memory(int64_t id);
```

#### 搜索记忆

```cpp
Result<std::vector<Memory>, std::string> search_memories(const MemoryFilter& filter);
```

#### 获取统计信息

```cpp
Result<MemoryStats, std::string> get_stats();
```

### RAGService

#### 单例访问

```cpp
static RAGService& instance();
```

#### 语义查询

```cpp
Result<RAGResult, std::string> query(const RAGQuery& query);
```

#### 批量生成嵌入

```cpp
Result<void, std::string> generate_embeddings(const std::vector<int64_t>& memory_ids);
```

### SQLiteDatabase

#### 单例访问

```cpp
static SQLiteDatabase& instance();
```

#### 初始化

```cpp
Result<void, std::string> initialize(const std::string& db_path);
```

#### 事务操作

```cpp
Result<void, std::string> begin_transaction();
Result<void, std::string> commit();
Result<void, std::string> rollback();
```

---

## 事件系统

### 发布的事件

#### MemoryExtractedEvent

```cpp
struct MemoryExtractedEvent {
    std::string conversation_id;
    std::vector<Memory> memories;
    bool success;
    std::string error_message;
};
```

#### RAGQueryCompletedEvent

```cpp
struct RAGQueryCompletedEvent {
    std::string query_id;
    std::string query_text;
    std::vector<Memory> memories;
    std::string context;
    bool success;
};
```

#### MessageSavedEvent

```cpp
struct MessageSavedEvent {
    std::string conversation_id;
    int64_t message_id;
    bool success;
};
```

### 订阅的事件

#### MemoryExtractRequestedEvent

```cpp
struct MemoryExtractRequestedEvent {
    std::string conversation_id;
    std::vector<std::string> message_contents;
    bool use_llm;
};
```

#### RAGQueryRequestedEvent

```cpp
struct RAGQueryRequestedEvent {
    std::string query_id;
    std::string query_text;
    int top_k;
    double min_similarity;
};
```

---

## 配置选项

### 数据库配置

```cpp
// 数据库文件路径
"memory_core.database.path" = "data/memory.db"

// WAL 模式启用
"memory_core.database.wal_enabled" = true

// FTS5 分词器
"memory_core.database.fts_tokenizer" = "porter"
```

### RAG 配置

```cpp
// Ollama 服务地址
"memory_core.rag.ollama_url" = "http://localhost:11434"

// 嵌入模型
"memory_core.rag.embedding_model" = "nomic-embed-text"

// 默认 top_k
"memory_core.rag.default_top_k" = 5

// 最小相似度阈值
"memory_core.rag.min_similarity" = 0.6
```

### 提取器配置

```cpp
// LLM 提取启用
"memory_core.extractor.use_llm" = true

// 规则引擎降级
"memory_core.extractor.fallback_to_rules" = true

// 最大记忆数
"memory_core.extractor.max_memories_per_extraction" = 10
```

---

## 开发指南

### 添加新的记忆类型

1. 在 `MemoryType` 枚举中添加新类型
2. 更新 `type_to_string()` 和 `string_to_type()` 方法
3. 更新数据库 schema（如需要）

### 自定义提取规则

继承 `RuleBasedExtractor` 并重写 `extract_memories()`：

```cpp
class CustomExtractor : public RuleBasedExtractor {
protected:
    std::vector<Memory> extract_memories_impl(
        const std::vector<std::string>& messages
    ) override {
        // 自定义提取逻辑
    }
};
```

### 扩展 RAG 检索

实现自定义的相似度计算：

```cpp
// 在 SQLiteDatabase 中注册自定义 SQL 函数
sqlite3_create_function(
    db,
    "custom_similarity",
    2,
    SQLITE_UTF8,
    nullptr,
    custom_similarity_func,
    nullptr,
    nullptr
);
```

---

## 技术栈

- **C++20**: Concepts, Ranges, Coroutines
- **SQLite3**: 持久化存储 + FTS5 全文搜索
- **Ollama**: 本地 LLM 和嵌入模型
- **DearTs Framework**: 插件系统、事件总线、结果类型

---

## 许可证

MIT License - 详见项目根目录 LICENSE 文件

---

## 贡献指南

欢迎提交 Issue 和 Pull Request！

1. Fork 项目
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 开启 Pull Request

---

## 联系方式

- 项目主页: [DearTs Framework](https://github.com/dearts/framework)
- 问题反馈: [GitHub Issues](https://github.com/dearts/framework/issues)
