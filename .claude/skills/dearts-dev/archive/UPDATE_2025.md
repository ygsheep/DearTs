# DearTs 开发技能更新说明 (2025-12-28)

## 更新概述

本次更新添加了三个核心系统的完整 API 手册，让您可以直接使用这些功能，而无需每次阅读源码。

---

## 新增文档

### 1. ConfigManager API 完全手册

**文件**: `dearts-dev/references/config_manager_api.md`

**内容**:
- ✅ 配置管理概述
- ✅ 快速开始指南
- ✅ 完整 API 参考（11 个方法）
- ✅ ConfigScope RAII 作用域管理
- ✅ 实际应用示例（5 个完整示例）
- ✅ 最佳实践（DO's and DON'Ts）
- ✅ 完整项目示例

**核心功能**:
```cpp
// 设置配置
ConfigManager::instance().set("app.window.width", 1280);

// 获取配置
int width = ConfigManager::instance().get_or<int>("app.window.width", 1280);

// 使用 ConfigScope（推荐）
ConfigScope scope{"app.window"};
scope.set("height", 720);  // 自动添加前缀: "app.window.height"
```

**特性**:
- 类型安全（bool, int, double, string）
- 层级键（点号分隔）
- 元数据支持（描述、默认值、验证器）
- 变更回调
- 线程安全

---

### 2. TaskManager API 完全手册

**文件**: `dearts-dev/references/task_manager_api.md`

**内容**:
- ✅ 任务管理概述
- ✅ 快速开始指南
- ✅ 任务类型和状态说明
- ✅ TaskManager API（11 个方法）
- ✅ Task API（12 个方法）
- ✅ 实际应用示例（5 个完整示例）
- ✅ 最佳实践
- ✅ 完整应用示例

**核心功能**:
```cpp
// 启动任务
auto task = TaskManager::instance().launch(
    "加载文件",
    [](const std::atomic<bool>& should_cancel) {
        for (int i = 0; i < 100; i++) {
            if (should_cancel) return;  // 检查取消
            do_work(i);
        }
    }
);

// 检查进度
float progress = task->get_progress_percent();

// 取消任务
TaskManager::instance().cancel(task);
```

**特性**:
- 异步执行
- 进度跟踪
- 任务取消
- 多种任务类型（Normal, Background, Blocking, Critical）
- 完成回调
- 线程安全

---

### 3. Logger API 完全手册

**文件**: `dearts-dev/references/logger_api.md`

**内容**:
- ✅ 日志系统概述
- ✅ 快速开始指南
- ✅ 日志级别说明
- ✅ Logger API（6 个方法）
- ✅ 日志宏（12 个宏）
- ✅ 实际应用示例（6 个完整示例）
- ✅ 最佳实践
- ✅ 完整应用示例

**核心功能**:
```cpp
// 设置日志级别
Logger::get_instance().set_level(LogLevel::DEBUG);

// 启用文件输出
Logger::get_instance().enable_file_output("logs/app.log");

// 记录日志
LOG_INFO("Application started");
LOG_ERROR("Failed to load: {}", filename);
LOG_WARN("Memory usage: {} MB", usage);
```

**特性**:
- 线程安全
- 异步文件写入
- 重复日志过滤
- std::format 格式化
- 六个日志级别（TRACE, DEBUG, INFO, WARN, ERROR, FATAL）

---

## 技能文件更新

**文件**: `dearts-dev/SKILL.md`

### 新增内容

#### 1. 核心组件部分新增

在 SKILL.md 的"核心组件详解"部分添加了三个新章节：

- **Logger - 高性能日志系统** (第 2 节)
- **TaskManager - 异步任务管理** (第 3 节)
- **ConfigManager - 配置管理** (第 4 节)

每个章节包含：
- 功能概述
- 代码示例
- 特性列表
- 完整 API 文档链接

#### 2. 参考资源部分更新

在"参考资源"部分添加了：

```markdown
**核心 API 手册（新）：**
- `references/config_manager_api.md` - **ConfigManager 完全手册**
- `references/logger_api.md` - **Logger 完全手册**
- `references/task_manager_api.md` - **TaskManager 完全手册**
```

#### 3. 技能使用场景更新

添加了新的使用场景：

```markdown
- **使用 ConfigManager 管理应用配置**
- **使用 Logger 记录应用日志**
- **使用 TaskManager 执行异步任务**
- **查看 API 文档而非阅读源码**
```

#### 4. 快速 API 参考部分新增

添加了完整的快速参考章节，包含：

- ConfigManager - 配置管理
- Logger - 日志系统
- TaskManager - 任务管理
- Result 类型 - 错误处理
- EventBus - 事件系统
- Content Registry - 注册系统

每个系统都有：
- 头文件引用
- 快速示例代码
- 完整文档链接

---

## 使用指南

### 如何使用这些 API 文档

#### 方法 1：直接查阅

当您需要使用某个功能时，直接打开对应的 API 文档：

```bash
# 查看配置管理 API
cat dearts-dev/references/config_manager_api.md

# 查看日志系统 API
cat dearts-dev/references/logger_api.md

# 查看任务管理 API
cat dearts-dev/references/task_manager_api.md
```

#### 方法 2：通过技能文件

1. 打开 `dearts-dev/SKILL.md`
2. 查看"快速 API 参考"部分
3. 找到您需要的系统
4. 查看快速示例
5. 点击"完整文档"链接获取详细说明

#### 方法 3：在开发中参考

每个 API 文档都包含：

1. **概述** - 了解功能和特性
2. **快速开始** - 5 分钟上手
3. **API 参考** - 完整的方法签名和说明
4. **实际示例** - 真实场景的应用
5. **最佳实践** - DO's and DON'Ts
6. **快速参考** - 速查表

---

## 实际应用示例

### 示例 1：应用初始化

```cpp
#include "core/config/config_manager.h"
#include "liblogger/logger.h"
#include "core/tasks/task_manager.h"

class MyApp : public Application {
private:
    ConfigScope m_config{"app"};

public:
    bool on_init() override {
        // 1. 初始化日志
        Logger::get_instance().set_level(LogLevel::DEBUG);
        Logger::get_instance().enable_file_output("logs/app.log");
        LOG_INFO("Application starting...");

        // 2. 加载配置
        ConfigManager::instance().load_from_file("config.json");
        int window_width = m_config.get_or<int>("window.width", 1280);
        int window_height = m_config.get_or<int>("window.height", 720);

        // 3. 启动后台任务
        auto task = TaskManager::instance().launch(
            "加载资源",
            [](const auto& cancel) {
                load_resources(cancel);
            }
        );

        LOG_INFO("Application initialized successfully");
        return true;
    }

    void on_shutdown() override {
        // 等待所有任务完成
        TaskManager::instance().waitForAll();

        // 保存配置
        ConfigManager::instance().save_to_file("config.json");

        LOG_INFO("Application shutdown complete");
    }
};
```

### 示例 2：插件开发

```cpp
class MyPlugin : public IPlugin {
private:
    ConfigScope m_config{"plugins.myplugin"};

public:
    Result<void, std::string> on_load() override {
        LOG_INFO("MyPlugin: Loading...");

        // 注册配置
        ConfigManager::instance().register_meta("plugins.myplugin.auto_save", {
            .description = "Auto save data",
            .default_value = true,
            .is_required = false
        });

        // 读取配置
        bool auto_save = m_config.get_or<bool>("auto_save", true);

        // 启动后台任务
        if (auto_save) {
            TaskManager::instance().launch(
                "自动保存",
                [this](const auto& cancel) {
                    while (!cancel) {
                        std::this_thread::sleep_for(std::chrono::seconds(60));
                        save_data();
                        LOG_DEBUG("Auto saved");
                    }
                },
                TaskType::Background
            );
        }

        LOG_INFO("MyPlugin: Loaded successfully");
        return Result::ok();
    }

    void on_unload() override {
        LOG_INFO("MyPlugin: Unloading...");
        // 清理资源...
    }
};
```

---

## 文档结构

```
dearts-dev/
├── SKILL.md                              # 主技能文件（已更新）
└── references/
    ├── config_manager_api.md             # ✨ 新增 - ConfigManager 完全手册
    ├── logger_api.md                     # ✨ 新增 - Logger 完全手册
    ├── task_manager_api.md               # ✨ 新增 - TaskManager 完全手册
    ├── result_type.md                    # Result 类型详解
    ├── event_system.md                   # 事件系统详解
    ├── content_registry.md               # Content Registry 详解
    ├── ui_system.md                      # UI 系统详解
    └── ...                               # 其他参考文档
```

---

## 关键改进

### 1. 无需阅读源码

**之前**：
```cpp
// 想使用配置管理，需要阅读源码
// core/config/config_manager.h - 289 行
// core/config/config_manager.cpp - 96 行
// 需要理解实现细节...
```

**现在**：
```cpp
// 直接查阅 API 文档
// dearts-dev/references/config_manager_api.md
// 包含完整的 API 说明、示例和最佳实践

// 快速开始
ConfigManager::instance().set("app.window.width", 1280);
int width = ConfigManager::instance().get_or<int>("app.window.width", 1280);
```

### 2. 完整的示例代码

每个 API 文档都包含：
- ✅ 快速开始（5 分钟上手）
- ✅ 基本用法（常见场景）
- ✅ 实际示例（真实应用）
- ✅ 最佳实践（DO's and DON'Ts）
- ✅ 完整项目示例（可直接复制）

### 3. 统一的文档格式

所有 API 文档遵循统一结构：
1. 概述
2. 快速开始
3. 核心概念
4. API 参考
5. 实际示例
6. 最佳实践
7. 快速参考

---

## 下一步

### 推荐学习路径

1. **第一步**: 阅读 SKILL.md 的"快速 API 参考"部分
2. **第二步**: 遇到具体需求时，查阅对应的 API 文档
3. **第三步**: 复制示例代码，修改为你的需求
4. **第四步**: 参考"最佳实践"部分改进代码

### 常见场景快速索引

| 需求 | 查阅文档 | 关键 API |
|------|---------|---------|
| 管理应用配置 | `config_manager_api.md` | `ConfigManager::instance().set/get_or` |
| 记录应用日志 | `logger_api.md` | `LOG_INFO/ERROR/WARN` |
| 执行异步任务 | `task_manager_api.md` | `TaskManager::instance().launch` |
| 处理错误 | `result_type.md` | `Result::ok/err` |
| 事件通信 | `event_system.md` | `EventBus::instance().subscribe/publish` |
| 注册命令/工具 | `content_registry.md` | `ContentRegistry::Commands::register_handler` |

---

## 总结

本次更新提供了三个核心系统的完整 API 文档：

1. **ConfigManager** - 350 行完整文档
2. **Logger** - 300 行完整文档
3. **TaskManager** - 400 行完整文档

总计 **1050+ 行**的详细 API 文档，包含：
- 完整的 API 参考
- 丰富的代码示例
- 实际应用场景
- 最佳实践指导

**现在您可以直接使用这些功能，无需每次阅读源码！** 🎉

---

## 相关文件

- **主技能文件**: `dearts-dev/SKILL.md`
- **ConfigManager API**: `dearts-dev/references/config_manager_api.md`
- **Logger API**: `dearts-dev/references/logger_api.md`
- **TaskManager API**: `dearts-dev/references/task_manager_api.md`
- **插件系统指南**: `docs/plugin_system_guide.md`
- **插件快速开始**: `plugins/QUICKSTART.md`

---

**更新日期**: 2025-12-28
**版本**: 2.0.0
