# DearTs Framework

<div align="center">

<img src="resources/icon.png" alt="DearTs Icon" width="128" height="128"/>

**基于 SDL3 + ImGui 的现代 C++20 应用程序框架，采用 ImHex 风格的插件架构**

[![C++20](https://img.shields.io/badge/C++-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![SDL3](https://img.shields.io/badge/SDL-3-green.svg)](https://wiki.libsdl.org/SDL3/)
[![License](https://img.shields.io/badge/license-MIT-purple.svg)](LICENSE)
[![Tests](https://img.shields.io/badge/tests-109%20passed-brightgreen.svg)](tests/)

[功能特性](#-核心特性) • [快速开始](#-快速开始) • [架构设计](#-架构设计) • [插件开发](#-插件开发) • [API 文档](#-api-文档)

</div>

---

## 📋 项目目的

### 🎯 我们要解决什么问题？

DearTs Framework 的诞生是为了解决现代 C++ GUI 应用开发中的以下痛点：

#### ❌ 传统 C++ 应用开发的问题

1. **繁琐的应用生命周期管理**
   - 手动管理 SDL/ImGui 初始化顺序
   - 资源清理容易遗漏导致内存泄漏
   - 状态转换逻辑混乱

2. **缺乏统一的插件架构**
   - 功能耦合严重，难以扩展
   - 插件加载和管理复杂
   - 模块间通信困难

3. **类型安全事件系统的缺失**
   - 依赖不安全的函数指针
   - 事件订阅/取消管理复杂
   - 容易产生内存泄漏和悬空指针

4. **配置管理混乱**
   - 缺乏统一的配置管理
   - JSON 序列化/反序列化繁琐
   - 配置验证和类型转换不安全

5. **测试基础设施不完善**
   - UI 自动化测试困难
   - 缺乏完整的单元测试框架
   - 测试覆盖率低

### ✅ DearTs 的解决方案

**DearTs Framework** 提供了一个完整的应用程序生命周期管理和插件系统，让开发者专注于业务逻辑而非基础设施：

- **🎯 完整的应用生命周期管理** - 清晰的状态机，自动化资源管理
- **🔌 ImHex 风格的插件系统** - 类型安全的插件接口，Content Registry 集中管理
- **📡 类型安全的事件总线** - 基于 C++20 的 EventBus，RAII 自动管理订阅
- **⚙️ 强大的配置管理** - 类型安全的 ConfigManager，JSON 持久化
- **🧪 完善的测试框架** - Google Test + ImGui Test Engine，109+ 测试用例
- **🎨 现代化 UI 系统** - SDL3 + ImGui v1.92，可停靠视图，主题系统

---

## ✨ 核心特性

### 🏗️ 应用程序生命周期

```
Uninitialized → Initializing → Running → Stopping → Stopped
                      ↑                            ↓
                      └────────── Paused ──────────┘
```

**特性**：
- ✅ 状态机驱动的生命周期
- ✅ 自动资源管理（RAII）
- ✅ 优雅退出机制
- ✅ FPS 统计和性能监控

### 🔌 ImHex 风格的插件系统

```cpp
class MyPlugin : public IPlugin {
    PluginInfo get_info() const override {
        return PluginInfo{
            .name = "MyPlugin",
            .version = "1.0.0"
        };
    }

    Result<void, std::string> on_load() override {
        // 注册命令
        ContentRegistry::Commands::register_handler("myplugin.action", ...);

        // 注册视图
        ContentRegistry::Views::add<MyView>();

        // 注册工具
        ContentRegistry::Tools::add("My Tool", ...);

        return Result::ok();
    }
};
```

**特性**：
- ✅ 清晰的插件生命周期 (load → enable → disable → unload)
- ✅ Content Registry 集中管理命令/视图/工具/设置
- ✅ 类型安全的 Result<T, E> 错误处理
- ✅ 插件间事件通信

### 📡 类型安全的事件总线

```cpp
// 定义事件
struct DataModifiedEvent {
    size_t offset;
    size_t size;
};

// 订阅事件（RAII 自动取消订阅）
EventBus::Token token = EventBus::instance().subscribe<DataModifiedEvent>(
    [](const DataModifiedEvent& e) {
        LOG_INFO("Data modified: {} bytes at offset {}", e.size, e.offset);
    }
);

// 发布事件
EventBus::instance().publish(DataModifiedEvent{0, 1024});
```

**特性**：
- ✅ 编译时类型检查
- ✅ RAII 自动管理订阅生命周期
- ✅ 支持多播和事件链
- ✅ 线程安全

### ⚙️ 配置管理

```cpp
// 使用 ConfigScope 自动添加前缀
ConfigScope config("myplugin");

// 类型安全的 get/set
config.set("enabled", true);
bool enabled = config.get_or<bool>("enabled", false);

// JSON 持久化
ConfigManager::instance().save_to_file("config.json");
```

**特性**：
- ✅ 类型安全的配置访问
- ✅ ConfigScope RAII 自动前缀
- ✅ JSON 导入/导出
- ✅ 配置验证和元数据

### 🧪 完整的测试框架

```
tests/
├── unit/          # 单元测试 (42 个测试)
│   ├── ConfigManager
│   ├── EventBus
│   ├── PluginManager
│   └── TaskManager
├── integration/   # 集成测试 (22 个测试)
└── ui/            # UI 自动化测试 (45 个测试)
```

**特性**：
- ✅ Google Test (单元测试 + 集成测试)
- ✅ ImGui Test Engine (UI 自动化测试)
- ✅ 109 个测试用例，全部通过
- ✅ 代码覆盖率 80%+

---

## 📦 依赖管理

DearTs 使用 **Git Submodules** 管理第三方依赖，确保版本可控和更新便捷。

### 🎮 SDL 家族 (4个)

| 库名 | 用途 | URL |
|------|------|-----|
| SDL3 | 跨平台多媒体核心 | [libsdl-org/SDL](https://github.com/libsdl-org/SDL) |
| SDL_image | 图像加载（PNG, JPG 等） | [libsdl-org/SDL_image](https://github.com/libsdl-org/SDL_image) |
| SDL_mixer | 音频播放 | [libsdl-org/SDL_mixer](https://github.com/libsdl-org/SDL_mixer) |
| SDL_ttf | TrueType 字体渲染 | [libsdl-org/SDL_ttf](https://github.com/libsdl-org/SDL_ttf) |

### 🎨 GUI 生态系统 (7个)

| 库名 | 用途 | URL |
|------|------|-----|
| imgui | 立即模式 GUI 框架 | [ocornut/imgui](https://github.com/ocornut/imgui) |
| freetype | 字体光栅化引擎 | [freetype/freetype](https://gitlab.freedesktop.org/freetype/freetype.git) |
| implot | 绘图和图表库 | [epezent/implot](https://github.com/epezent/implot.git) |
| imgui_markdown | Markdown 渲染支持 | [juliettef/imgui_markdown](https://github.com/juliettef/imgui_markdown.git) |
| imgui-node-editor | 节点编辑器 | [thedmd/imgui-node-editor](https://github.com/thedmd/imgui-node-editor.git) |
| imgui_test_engine | UI 自动化测试引擎 | [ocornut/imgui_test_engine](https://github.com/ocornut/imgui_test_engine.git) |
| lunasvg | SVG 字体支持 | [sammycage/lunasvg](https://github.com/sammycage/lunasvg.git) |

### 🧪 测试框架 (1个)

| 库名 | 用途 | URL |
|------|------|-----|
| googletest | 单元测试框架 | [google/googletest](https://github.com/google/googletest.git) |

### 📦 工具库 (3个)

| 库名 | 用途 | URL |
|------|------|-----|
| json | 现代 JSON 库 | [nlohmann/json](https://github.com/nlohmann/json.git) |
| cppjieba | C++ 中文分词 | [yanyiwu/cppjieba](https://github.com/yanyiwu/cppjieba.git) |
| llama.cpp | LLM 推理引擎 | [ggerganov/llama.cpp](https://github.com/ggerganov/llama.cpp.git) |

---

## 🚀 快速开始

### 前置要求

- **编译器**: MSVC 2022+ / GCC 11+ / Clang 13+
- **CMake**: 3.20+
- **操作系统**: Windows 10+ / Linux / macOS 10.15+

### 克隆和构建

```bash
# 1. 克隆仓库（包含子模块）
git clone --recursive https://github.com/ygsheep/DearTs.git
cd DearTs

# 如果忘记 --recursive，手动初始化子模块
git submodule update --init --recursive

# 2. 配置项目（Release 模式）
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 3. 构建
cmake --build build --config Release

# 4. 运行
./build/bin/DearTsApp.exe        # Windows
./build/bin/DearTsApp            # Linux/macOS
```

### 运行测试

```bash
# 运行所有测试
ctest --test-dir build --verbose

# 或直接运行测试可执行文件
./build/bin/dearts_unit_tests        # 单元测试
./build/bin/dearts_integration_tests # 集成测试
./build/bin/dearts_ui_tests          # UI 自动化测试
```

---

## 🏗️ 架构设计

### 核心设计原则

1. **插件优先** - 所有功能通过插件扩展，核心保持最小化
2. **事件驱动** - 组件间通过 EventBus 解耦通信
3. **类型安全** - Result<T, E> 替代异常，编译时类型检查
4. **RAII** - 自动资源管理（EventBus::Token, ConfigScope, Task 智能指针）
5. **现代 C++20** - Concepts, Ranges, Coroutines, std::format

### 系统架构

```
DearTs Application
├── Core Systems                (核心系统)
│   ├── Plugin System           (插件管理器)
│   ├── EventBus                (类型安全事件总线)
│   ├── Content Registry        (命令/视图/工具/设置注册表)
│   ├── ConfigManager           (配置管理器)
│   ├── TaskManager             (异步任务管理器)
│   └── Logger                  (异步日志系统)
│
├── UI Layer                    (UI 层)
│   ├── View System             (可停靠窗口视图)
│   ├── TitleBar                (自定义工具栏)
│   ├── CommandPalette          (命令面板)
│   └── Theme Manager           (主题管理)
│
└── Plugin Ecosystem            (插件生态)
    ├── Builtin Plugin          (内置插件)
    ├── Toast Notification      (通知插件)
    ├── Command Palette         (命令面板插件)
    └── Custom Plugins          (用户自定义插件)
```

### 插件生命周期

```
Unloaded
    ↓ add_builtin() / load_from_file()
Loaded
    ↓ enable()
Enabled
    ↓ disable()
Loaded
    ↓ unload()
Unloaded
```

### 数据流

```
User Action
    ↓
ContentRegistry::Commands::invoke()
    ↓
EventBus::publish(Event)
    ↓
Plugin Event Handlers (subscribed via EventBus::Token)
    ↓
UI Updates / Background Tasks
```

---

## 🔌 插件开发

### 最小化插件示例

```cpp
// my_plugin.hpp
#pragma once
#include "core/plugin/plugin.h"

class MyPlugin : public IPlugin {
public:
    PluginInfo get_info() const override {
        return PluginInfo{
            .name = "MyPlugin",
            .author = "Your Name",
            .description = "My awesome plugin",
            .version = "1.0.0",
            .api_version = "1.0.0"
        };
    }

    Result<void, std::string> on_load() override {
        // 注册命令
        ContentRegistry::Commands::register_handler(
            "myplugin.hello",
            "Say Hello",
            []() { LOG_INFO("Hello from MyPlugin!"); },
            nullptr,
            "Ctrl+Shift+H"
        );

        // 注册视图
        ContentRegistry::Views::add<MyView>();

        return Result::ok();
    }

    void on_unload() override {
        // 清理（RAII 自动处理大部分）
    }
};
```

### 创建自定义视图

```cpp
// my_view.hpp
#pragma once
#include "core/ui/view.h"

class MyView : public View {
public:
    std::string getName() const override {
        return "My View";
    }

    void draw_content() override {
        ImGui::Text("Hello from my view!");

        if (ImGui::Button("Click me")) {
            LOG_INFO("Button clicked!");
        }
    }
};
```

### 使用配置管理

```cpp
class MyPlugin : public IPlugin {
private:
    ConfigScope m_config{"myplugin"};  // 自动前缀 "myplugin."

public:
    Result<void, std::string> on_load() override {
        // 读取配置
        bool enabled = m_config.get_or<bool>("enabled", true);
        int interval = m_config.get_or<int>("interval", 60);

        LOG_INFO("Plugin enabled: {}, interval: {}", enabled, interval);

        // 保存配置
        m_config.set("last_run", std::time(nullptr));
        ConfigManager::instance().save_to_file("config.json");

        return Result::ok();
    }
};
```

### 事件订阅

```cpp
class MyPlugin : public IPlugin {
private:
    EventBus::Token m_eventToken;  // RAII 自动取消订阅

public:
    Result<void, std::string> on_load() override {
        // 订阅事件
        m_eventToken = EventBus::instance().subscribe<ApplicationReadyEvent>(
            [this](const ApplicationReadyEvent&) {
                LOG_INFO("Application is ready!");
                // 初始化插件
            }
        );

        return Result::ok();
    }

    void on_unload() override {
        // m_eventToken 自动取消订阅（RAII）
    }
};
```

---

## 📁 项目结构

```
DearTs/
├── core/                              # 核心系统（源码直接编译）
│   ├── plugin/                        # 插件系统
│   │   ├── plugin.h                   # IPlugin 接口
│   │   └── plugin_manager.h           # PluginManager
│   ├── events/                        # 事件系统
│   │   └── event_bus.h                # EventBus
│   ├── content/                       # Content Registry
│   │   ├── commands.h                 # 命令注册
│   │   ├── views.h                    # 视图注册
│   │   ├── tools.h                    # 工具注册
│   │   └── registry.h                 # 统一注册表
│   ├── config/                        # 配置管理
│   │   └── config_manager.h           # ConfigManager
│   ├── tasks/                         # 任务管理
│   │   └── task_manager.h             # TaskManager
│   └── ui/                            # UI 系统
│       ├── view.h                     # View 基类
│       ├── title_bar.h                # TitleBar
│       └── layout_manager.h           # 布局管理
│
├── plugins/                           # 插件实现
│   ├── builtin/                       # 内置插件
│   ├── toast_notification/            # 通知插件
│   └── command_palette/               # 命令面板插件
│
├── tests/                             # 测试套件
│   ├── unit/                          # 单元测试 (42)
│   ├── integration/                   # 集成测试 (22)
│   └── ui/                            # UI 自动化测试 (45)
│
├── lib/                               # 内部库
│   └── liblogger/                     # 异步日志库
│
├── third_party/                       # Git Submodules (15个)
│   ├── SDL/                           # SDL3
│   ├── imgui/                         # ImGui
│   ├── googletest/                    # Google Test
│   └── ...                            # 更多依赖
│
├── main/gui/                          # 主应用程序
│   └── main.cpp
│
├── docs/                              # 用户文档
├── dearts-dev/                        # 开发者文档
│   └── references/                    # API 参考手册
│       ├── plugin_system_api.md
│       ├── config_manager_api.md
│       ├── task_manager_api.md
│       └── logger_api.md
│
└── CMakeLists.txt                     # 构建配置
```

---

## 📊 测试覆盖

| 组件 | 覆盖率 | 测试数量 |
|------|--------|----------|
| ConfigManager | 90%+ | 7 |
| EventBus | 85%+ | 10 |
| PluginManager | 80%+ | 8 |
| TaskManager | 85%+ | 17 |
| Result<T,E> | 90%+ | 5 |
| UI 组件 | 关键路径 | 45 |
| **总计** | **80%+** | **109** |

---

## 🚧 开发路线图

### ✅ 已完成 (v0.1.0)

- [x] 插件系统 (IPlugin, PluginManager)
- [x] 类型安全事件总线 (EventBus)
- [x] Content Registry (命令/视图/工具/设置)
- [x] 配置管理 (ConfigManager, ConfigScope)
- [x] 任务管理 (TaskManager, async tasks)
- [x] 测试基础设施 (Google Test + ImGui Test Engine)
- [x] 109 个测试用例

### 🚧 进行中 (v0.2.0)

- [ ] 完善文档和示例
- [ ] 更多内置插件
- [ ] 性能优化
- [ ] CI/CD 集成

### 📅 计划中 (v0.3.0+)

- [ ] 多语言支持
- [ ] 主题系统增强
- [ ] 插件市场
- [ ] 远程插件加载

---

## 📚 文档

### 用户文档

- [插件开发指南](docs/plugin_system_guide.md)
- [API 参考手册](dearts-dev/references/)

### 开发者文档

- [插件系统 API](dearts-dev/references/plugin_system_api.md) (1038 行)
- [配置管理 API](dearts-dev/references/config_manager_api.md) (676 行)
- [任务管理 API](dearts-dev/references/task_manager_api.md) (917 行)
- [日志系统 API](dearts-dev/references/logger_api.md) (727 行)

### 测试文档

- [测试使用指南](tests/README.md)
- [测试计划](tests/TESTING_PLAN.md)

---

## 🤝 贡献指南

欢迎贡献代码、报告问题或提出建议！

### 如何贡献

1. **Fork 本仓库**
2. **创建功能分支** (`git checkout -b feature/AmazingFeature`)
3. **提交更改** (`git commit -m 'Add some AmazingFeature'`)
4. **推送到分支** (`git push origin feature/AmazingFeature`)
5. **提交 Pull Request**

### 代码规范

- ✅ 遵循 C++20 标准
- ✅ 使用 Result<T, E> 而非异常
- ✅ 使用 RAII 管理资源
- ✅ 添加必要的测试和文档
- ✅ 确保所有测试通过

---

## 📄 许可证

本项目采用 **MIT 许可证**。详见 [LICENSE](LICENSE) 文件。

---

## 🙏 致谢

DearTs Framework 的设计灵感来自以下优秀项目：

- **[ImHex](https://github.com/WerWolv/ImHex)** - 强大的十六进制编辑器，插件系统和 Content Registry 设计灵感
- **[SDL3](https://github.com/libsdl-org/SDL)** - 跨平台多媒体库
- **[ImGui](https://github.com/ocornut/imgui)** - 立即模式 GUI 框架
- **[Oryol](https://github.com/floooh/oryol)** - 跨平台游戏框架

---

<div align="center">

**用 ❤️ 打造的现代 C++20 应用程序框架**

[⬆ 返回顶部](#dearts-framework)

</div>
