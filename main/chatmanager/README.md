# ChatManager

> 基于 DearTs Framework 的现代化 AI 聊天应用程序

## 概述

ChatManager 是一个功能丰富的 AI 聊天 GUI 应用程序，具有智能对话管理、持久化记忆和 Markdown 渲染等功能。通过本地 LLM（如 Ollama）提供 AI 能力，支持跨会话的语义记忆检索。

### 核心特性

- **智能对话管理**：多会话支持、会话历史记录、消息编辑
- **本地 LLM 集成**：支持 Ollama 本地大模型，流式响应
- **持久化记忆**：跨会话记忆存储，语义检索（RAG）
- **Markdown 渲染**：完整的 Markdown 支持，代码高亮
- **现代化 UI**：玻璃态设计、主题切换、自定义标题栏
- **可扩展架构**：基于 DearTs Framework 插件系统

---

## 系统要求

### 最低配置

- **操作系统**: Windows 10+, Linux, macOS
- **编译器**: MSVC 2022 (C++20), GCC 11+, Clang 13+
- **CMake**: 3.20+
- **内存**: 4 GB RAM（推荐 8 GB+）
- **存储**: 500 MB 可用空间

### 依赖项

| 依赖 | 版本 | 用途 |
|------|------|------|
| SDL3 | Latest | 窗口和输入管理 |
| ImGui | 2.13.3+ | UI 渲染 |
| nlohmann/json | 3.11+ | JSON 序列化 |
| SQLite3 | 3.38+ | 数据库持久化（可选） |
| Ollama | Latest | 本地 LLM 服务（可选） |

---

## 快速开始

### 1. 安装 Ollama

ChatManager 使用 Ollama 作为本地 LLM 提供商。首先安装并启动 Ollama 服务。

#### Windows/Linux

```bash
# 下载并安装 Ollama
# https://ollama.ai/

# 拉取 Llama3.2 模型（聊天模型）
ollama pull llama3.2

# 拉取 nomic-embed-text 模型（嵌入模型，用于 RAG 语义检索）
ollama pull nomic-embed-text

# 启动 Ollama 服务
ollama serve
```

#### 验证 Ollama 连接

```bash
# 查看已安装的模型
ollama list

# 测试 Ollama API
curl http://localhost:11434/api/tags

# 测试聊天模型
curl http://localhost:11434/api/generate -d '{
  "model": "llama3.2",
  "prompt": "Hello, Ollama!"
}'

# 测试嵌入模型（用于 RAG）
curl http://localhost:11434/api/embed -d '{
  "model": "nomic-embed-text",
  "input": "Hello, world!"
}'
```

> **注意**: `nomic-embed-text` 是嵌入模型，专门用于生成文本向量，支持 Memory Core 的 RAG（检索增强生成）语义检索功能。如果只下载聊天模型而不下载嵌入模型，RAG 功能将不可用。

### 2. 安装 SQLite3（可选，用于持久化记忆）

#### Windows (vcpkg)

```bash
vcpkg install sqlite3:x64-windows
```

#### Linux

```bash
sudo apt-get install sqlite3 libsqlite3-dev
```

#### macOS

```bash
brew install sqlite3
```

### 3. 构建项目

```bash
# 配置构建（Release 模式）
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 构建所有目标
cmake --build build --config Release

# 或仅构建 ChatManager
cmake --build build --target ChatManager --config Release
```

### 4. 运行 ChatManager

```bash
# Windows
.\build\bin\ChatManager.exe

# Linux/macOS
./build/bin/ChatManager
```

---

## 配置

### Ollama 配置

#### 默认配置

ChatManager 默认连接到本地 Ollama 服务：

- **服务地址**: `http://localhost:11434`
- **默认模型**: `llama3.2`
- **嵌入模型**: `nomic-embed-text`（用于语义检索）

#### 自定义 Ollama 配置

在 ChatManager 中通过 **信息 > Memory** 面板配置：

1. 打开 ChatManager
2. 切换到 **信息** 选项卡
3. 选择 **Memory** 子选项卡
4. 在 **RAG 查询测试** 区域配置：
   - **Ollama 服务地址**: 修改 `http://localhost:11434`
   - **最大结果数**: 调整返回的记忆数量（1-20）
   - **最小相似度**: 设置相似度阈值（0.0-1.0）

#### 远程 Ollama 服务器

如需连接远程 Ollama 服务器：

1. 在远程服务器上启动 Ollama 并设置环境变量：
   ```bash
   export OLLAMA_HOST=0.0.0.0:11434
   ollama serve
   ```

2. 在 ChatManager 信息面板中配置远程服务器地址：
   ```
   http://your-server-ip:11434
   ```

#### 可用模型列表

```bash
# 查看已安装的模型
ollama list

# 下载其他模型
ollama pull mistral
ollama pull codellama
ollama pull nomic-embed-text  # 嵌入模型（用于 RAG）
```

### 数据库配置

#### SQLite3 数据库路径

数据库文件默认存储在项目根目录下的 `data/` 文件夹：

```
DearTsd/
├── data/
│   ├── memory.db          # 记忆数据库
│   ├── memory.db-wal      # WAL 日志
│   └── memory.db-shm      # 共享内存
```

#### 数据库性能优化

数据库初始化时自动应用以下优化：

```sql
-- WAL 模式（提高并发性能）
PRAGMA journal_mode = WAL;

-- 同步模式（平衡性能和安全）
PRAGMA synchronous = NORMAL;

-- 缓存大小（64 MB）
PRAGMA cache_size = -64000;

-- 外键约束
PRAGMA foreign_keys = ON;
```

#### 数据库架构

| 表名 | 用途 |
|------|------|
| `memories` | 存储持久化记忆 |
| `embeddings` | 存储向量嵌入（用于语义检索） |
| `messages` | 存储聊天消息 |
| `messages_fts` | FTS5 全文搜索索引 |
| `conversations` | 存储会话元数据 |

#### 数据库初始化

数据库在应用启动时自动初始化。如需手动重建：

```cpp
// 删除旧数据库
rm data/memory.db

// 重启 ChatManager，数据库将自动重建
```

#### 禁用 SQLite3

如果编译时未找到 SQLite3，ChatManager 会显示友好提示：

```
Memory Core 功能未启用
编译时未找到 SQLite3，Memory Core 功能不可用。
请在 CMake 配置时启用 SQLite3 支持。
```

在这种情况下，ChatManager 仍可正常使用聊天功能，但不支持：
- 跨会话记忆存储
- RAG 语义检索
- 消息持久化

---

## 功能说明

### 对话管理

- **新建会话**: `Ctrl+N` 或点击左上角 "+" 按钮
- **切换会话**: 在左侧会话列表中选择
- **删除会话**: 右键点击会话 > 删除
- **导出会话**: 信息面板 > 导出

### AI 设置

在 **信息** 选项卡中配置 AI 参数：

| 参数 | 默认值 | 说明 |
|------|--------|------|
| LLM 提供商 | Ollama | 本地或远程 LLM 服务 |
| 模型名称 | llama3.2 | 使用的 LLM 模型 |
| Temperature | 0.7 | 控制输出随机性（0.0-1.0） |
| Max Tokens | 2048 | 最大生成长度 |

### Markdown 支持

ChatRenderer 支持完整的 Markdown 语法：

- 标题（H1-H6）
- 粗体/斜体
- 代码块（语法高亮）
- 列表（有序/无序）
- 引用块
- 链接和图片
- 表格

### 主题切换

支持深色/浅色主题切换，在 **设置** 面板中配置。

---

## 调试

### Memory Debug UI

ChatManager 集成了 Memory Core 调试面板，位于 **信息 > Memory** 选项卡。

#### 功能区域

1. **记忆统计**
   - 总记忆数量
   - 按类型分布（偏好、事实、问答、上下文、技能）

2. **数据库状态**
   - 连接状态指示器（绿色=已连接，红色=未连接）
   - 数据库路径

3. **RAG 查询测试**
   - 输入查询文本
   - 调整最大结果数和最小相似度
   - 查看检索结果

4. **事件监控**
   - 事件统计表格
   - 实时事件日志

5. **一致性管理**
   - 离线队列状态
   - 同步操作按钮

#### 导出统计数据

点击 **导出统计** 按钮将 Memory Debug 数据导出为 JSON：

```json
{
  "total_memories": 42,
  "memory_type_counts": [
    {"count": 10, "name": "Preference"},
    {"count": 15, "name": "Fact"},
    {"count": 8, "name": "QA"},
    {"count": 5, "name": "Context"},
    {"count": 4, "name": "Skill"}
  ],
  "last_query": "用户喜欢什么主题？",
  "event_stats": {
    "RAGQueryCompleted": 15
  }
}
```

---

## 故障排除

### Ollama 连接失败

**症状**: 提示 "Ollama 未连接"

**解决方案**:

1. 验证 Ollama 服务是否运行：
   ```bash
   curl http://localhost:11434/api/tags
   ```

2. 确认模型已下载：
   ```bash
   ollama list
   ```

3. 检查防火墙设置

### RAG 功能不可用

**症状**: 语义检索返回空结果或失败

**解决方案**:

RAG 功能需要嵌入模型 `nomic-embed-text`。验证模型是否已下载：

```bash
# 查看已安装的模型
ollama list
```

确认输出中包含 `nomic-embed-text`。如果没有，下载它：

```bash
# 下载嵌入模型
ollama pull nomic-embed-text

# 验证嵌入模型工作正常
curl http://localhost:11434/api/embed -d '{
  "model": "nomic-embed-text",
  "input": "Hello, world!"
}'
```

### SQLite3 未找到

**症状**: 编译时警告 "SQLite3 not found"

**解决方案**:

#### Windows (vcpkg)

```bash
vcpkg install sqlite3:x64-windows
```

#### Linux

```bash
sudo apt-get install sqlite3 libsqlite3-dev
```

然后重新配置 CMake：

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
```

### 字体显示问题

**症状**: 中文字符显示为方框

**解决方案**:

1. 确保 `resources/fonts/` 目录下有中文字体：
   - `OPPOSans-M.ttf`
   - `NotoSansSC-Regular.ttf`

2. ChatManager 会自动回退到系统字体：
   - Windows: 微软雅黑（`C:/Windows/Fonts/msyh.ttc`）
   - Linux: WenQuanYi Zen Hei

---

## 技术栈

- **框架**: DearTs Framework (SDL3 + ImGui)
- **语言**: C++20
- **UI**: ImGui 2.13.3+ (SDL3 GPU 后端)
- **持久化**: SQLite3 + FTS5 全文搜索
- **LLM**: Ollama (本地 LLM 服务)
- **JSON**: nlohmann/json 3.11+
- **日志**: liblogger (异步日志)

---

## 项目结构

```
main/chatmanager/
├── CMakeLists.txt          # CMake 构建配置
├── chatmanager.rc          # Windows 资源文件
├── README.md               # 本文档
├── include/chatmanager/
│   └── application.hpp     # 应用程序主类
└── source/
    ├── main.cpp            # 入口点
    └── application.cpp     # 应用程序实现
```

### 加载的插件

ChatManager 自动加载以下内置插件：

1. **Builtin Plugin** - 基础 UI 组件
2. **Memory Core Plugin** - 持久化记忆和 RAG
3. **Chat Plugin** - 聊天功能和 Markdown 渲染

---

## 开发

### 构建类型

```bash
# Debug 模式（包含调试符号）
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Release 模式（优化）
cmake -B build -DCMAKE_BUILD_TYPE=Release

# RelWithDebInfo 模式（优化 + 调试符号）
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
```

### 编译选项

- MSVC: `/W4 /std:c++20 /MP`
- GCC/Clang: `-Wall -Wextra -Wpedantic`

### 添加新功能

ChatManager 基于 DearTs Framework 插件系统，扩展功能请参考：

- [DearTs Framework API](../../dearts-dev/SKILL.md)
- [插件开发指南](../../plugins/QUICKSTART.md)

---

## 许可证

MIT License - 详见项目根目录 LICENSE 文件

---

## 贡献

欢迎提交 Issue 和 Pull Request！

1. Fork 项目
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 开启 Pull Request

---

## 相关文档

- [DearTs Framework 完整文档](../../dearts-dev/SKILL.md)
- [Chat Plugin 文档](../../plugins/chat/README.md)
- [Memory Core Plugin 文档](../../plugins/memory_core/README.md)
- [插件系统 API 参考](../../dearts-dev/references/plugin_system_api.md)
