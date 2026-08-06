# DearTs Framework - Reference Documentation

This directory contains comprehensive API documentation and reference guides for DearTs Framework.

## 📘 Core API Manuals（优先阅读）

These are the most important documents - complete API references with examples and best practices.

### [config_manager_api.md](config_manager_api.md) ⭐
**ConfigManager 完全手册** (17 KB, 676 lines)

配置管理系统，支持层级键、类型安全、JSON 持久化。

**关键 API:**
- `get<T>()`, `set()`, `get_or<T>()` - 类型安全的配置读写
- `load_from_file()`, `save_to_file()` - JSON 文件操作
- `register_meta()` - 配置元数据和验证
- `ConfigScope` - RAII 自动前缀管理

**包含:**
- 11 个方法完整说明
- 5 个实际应用示例
- 最佳实践 (DO's and DON'Ts)
- JSON 格式说明

---

### [logger_api.md](logger_api.md) ⭐
**Logger 完全手册** (18 KB, 727 lines)

高性能日志系统，线程安全、异步文件写入。

**关键 API:**
- `LOG_TRACE`, `LOG_DEBUG`, `LOG_INFO`, `LOG_WARN`, `LOG_ERROR`, `LOG_FATAL`
- `set_level()`, `enable_file_output()`
- 重复日志过滤
- std::format 格式化支持

**包含:**
- 6 个日志级别详解
- 6 个实际应用示例
- 性能优化建议
- 文件输出配置

---

### [task_manager_api.md](task_manager_api.md) ⭐
**TaskManager 完全手册** (22 KB, 917 lines)

异步任务管理系统，支持进度跟踪、任务取消。

**关键 API:**
- `launch()` - 启动异步任务
- Task 类型: Normal, Background, Blocking, Critical
- `update_progress()`, `cancel()`
- ImGui 进度条集成

**包含:**
- 11 个 TaskManager 方法
- 12 个 Task 方法
- 5 个完整示例（包括 ImGui 进度显示）
- 任务类型和状态详解

---

### [plugin_system_api.md](plugin_system_api.md) ⭐
**Plugin System 完全手册** (23 KB, 1038 lines)

插件开发完整指南，生命周期管理、API 集成。

**关键 API:**
- `IPlugin` 接口: `get_info()`, `on_load()`, `on_unload()`, `on_enable()`, `on_disable()`
- `PluginManager`: `add_builtin()`, `load_from_file()`, `enable()`, `disable()`
- Content Registry 集成
- EventBus 订阅

**包含:**
- 4 个完整插件示例
- 12 个 PluginManager 方法
- 插件开发检查清单
- 最佳实践和常见模式

---

## 📕 Core Systems

### [result_type.md](result_type.md)
**Result<T, E> 类型详解** (9 KB)

统一的错误处理类型，支持函数式编程。

- `ok()`, `err()`, `map()`, `and_then()`, `or_else()`
- 函数式编程模式
- void 特化版本

---

### [event_system.md](event_system.md)
**EventBus 事件系统详解** (11 KB)

类型安全的发布-订阅事件系统。

- 编译时类型检查
- `subscribe()`, `publish()`, `unsubscribe()`
- EventBus::Token RAII 管理
- 事件队列和异步处理

---

### [content_registry.md](content_registry.md)
**Content Registry 详解** (14 KB)

ImHex 风格的集中式命令/工具/设置注册系统。

- Commands 系统
- Tools 系统
- Settings 系统
- Views 系统

---

### [ui_system.md](ui_system.md)
**UI 系统详解** (15 KB)

ImGui 集成、视图管理、停靠窗口。

- ImGui Layer
- View 基类
- ViewManager
- Command Palette
- TitleBar
- Layout Management

---

## 📗 Application Layer

### [application_api.md](application_api.md)
**Application 类 API**

应用程序基类，生命周期管理。

---

### [task_system.md](task_system.md)
**任务系统详解**

任务系统架构和设计。

---

### [plugin_system.md](plugin_system.md)
**插件系统详解**

插件架构概述。

---

## 📙 Integration Guides

### [sdl3_init.md](sdl3_init.md)
**SDL3 初始化指南**

SDL3 库的初始化和配置。

---

### [imgui_integration.md](imgui_integration.md)
**ImGui 集成指南**

ImGui 与 SDL3 的集成。

---

### [cmake_config.md](cmake_config.md)
**CMake 配置详解**

CMake 构建系统配置。

---

### [logging_guide.md](logging_guide.md)
**日志系统使用指南**

日志系统的使用指南。

---

## 🎯 文档使用指南

### 按任务查找

| 任务 | 查阅文档 |
|------|---------|
| 创建插件 | `plugin_system_api.md` |
| 管理配置 | `config_manager_api.md` |
| 异步任务 | `task_manager_api.md` |
| 添加日志 | `logger_api.md` |
| 事件系统 | `event_system.md` |
| 注册命令 | `content_registry.md` |
| 创建视图 | `ui_system.md` |
| 错误处理 | `result_type.md` |
| CMake 配置 | `cmake_config.md` |

### 按学习路径

**初学者:**
1. 阅读 `config_manager_api.md` - 了解配置系统
2. 阅读 `logger_api.md` - 学会添加日志
3. 阅读 `plugin_system_api.md` - 创建第一个插件

**进阶:**
1. 阅读 `task_manager_api.md` - 异步任务处理
2. 阅读 `event_system.md` - 事件驱动架构
3. 阅读 `content_registry.md` - Content Registry 集成

**高级:**
1. 阅读 `result_type.md` - 函数式错误处理
2. 阅读 `ui_system.md` - 自定义视图和 UI
3. 深入源码实现

### 文档统计

| 类别 | 文档数 | 总行数 | 总大小 |
|------|--------|--------|--------|
| 核心 API 手册 | 4 | 3358 | 80 KB |
| 核心系统 | 4 | ~2500 | ~50 KB |
| 应用层 | 3 | ~500 | ~10 KB |
| 集成指南 | 4 | ~800 | ~15 KB |
| **总计** | **15** | **~7158** | **~155 KB** |

---

## 💡 提示

- **优先阅读** 4 个核心 API 手册（带 ⭐ 标记）
- 每个手册都有完整代码示例
- 查看最佳实践章节避免常见错误
- 参考 `../assets/` 获取代码模板
- 查看 `../../plugins/builtin/` 学习实际实现

---

**最后更新**: 2025-12-30
**文档版本**: 3.0.0
