# DearTs Framework

<div align="center">

**基于 SDL3 的现代 C++20 应用程序生命周期管理框架**

[![C++20](https://img.shields.io/badge/C++-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![SDL3](https://img.shields.io/badge/SDL-3-green.svg)](https://wiki.libsdl.org/SDL3/)
[![License](https://img.shields.io/badge/license-MIT-purple.svg)](LICENSE)

[快速开始](#-快速开始) • [架构说明](#-架构设计) • [API文档](#-application-类详解) • [开发指南](#-开发指南)

</div>

## 📖 项目简介

DearTs 是一个轻量级、模块化的 C++ 应用程序开发框架，专注于提供清晰的应用程序生命周期管理和现代化开发体验。框架采用**源码集成**的设计理念，核心模块直接编译到可执行文件中，避免了传统链接库的复杂性。

### ✨ 核心特性

#### 🎯 应用程序管理
- **完整生命周期管理** - 状态机驱动的初始化、运行、暂停、关闭流程
- **智能帧率控制** - 支持 VSync 和目标帧率，自动 FPS 统计
- **优雅退出机制** - 安全的资源清理和状态转换

#### 🚀 现代化架构
- **C++20** - 使用 Concepts、Ranges、std::format 等现代特性
- **类型安全事件系统** - 基于 EventBus 的发布-订阅模式，支持异步事件
- **模块化设计** - 清晰的模块边界，易于扩展和维护

#### 🎨 用户界面
- **ImGui 集成** - 完整的 SDL3 + ImGui 集成层
- **多视图系统** - 参考 ImHex 的停靠窗口管理，支持多种视图类型
- **主题支持** - 可定制的 UI 主题和样式

#### 🔧 开发工具
- **异步日志系统** - 基于 liblogger 的高性能日志，支持重复过滤
- **配置管理** - 灵活的配置文件管理和持久化
- **插件系统** - 支持动态插件加载（预留接口）

## 📁 项目结构

```
DearTs/
├── core/                              # 核心源代码（直接编译，非链接库）
│   ├── app/                          # 应用程序生命周期管理
│   │   ├── application.h             # 应用程序基类
│   │   └── application.cpp           # 应用程序实现
│   ├── event/                        # 类型安全的事件系统
│   │   ├── event_bus.h               # 事件总线（发布-订阅）
│   │   └── events.h                  # 内置事件定义
│   ├── ui/                           # 用户界面系统
│   │   ├── imgui_layer.h             # SDL3 + ImGui 集成层
│   │   ├── view.h                    # 视图系统
│   │   └── view_manager.h            # 多视图管理器
│   ├── config/                       # 配置管理
│   │   └── config_manager.h          # 配置文件管理器
│   └── plugin/                       # 插件系统（预留接口）
├── lib/                              # 外部依赖库
│   └── liblogger/                    # 异步日志库
├── third_party/                      # 第三方依赖
│   ├── SDL/                          # SDL3 - 跨平台多媒体库
│   ├── imgui/                        # ImGui - 立即模式 GUI
│   ├── freetype/                     # FreeType - 字体渲染
│   ├── SDL_image/                    # 图像加载库（可选）
│   ├── SDL_mixer/                    # 音频播放库（可选）
│   └── SDL_ttf/                      # TrueType 字体库（可选）
├── main/gui/                         # GUI 测试程序
│   ├── main.cpp                      # 生命周期测试示例
│   └── CMakeLists.txt                # 编译配置
├── examples/                         # 示例代码
├── docs/                             # 项目文档
│   └── 代码规范.md
├── CMakeLists.txt                    # 根构建配置
└── README.md                         # 本文件
```

### 🏗️ 架构设计理念

**core 不是链接库！** 这是 DearTs 的核心设计理念：

```
传统方式（复杂）:
core/ → 编译成 libdearts_core.a → 链接到可执行文件

DearTs 方式（简洁）:
core/ → 作为源代码 → 直接编译到可执行文件
```

**优势**：
- ✅ 简化构建流程，无需中间库
- ✅ 更好的编译器优化机会
- ✅ 减少链接复杂度
- ✅ 更容易调试和热重载

## 🏛️ 架构设计

### 应用程序生命周期

```
┌─────────────┐     ┌──────────────┐     ┌─────────┐
│UNINITIALIZED│ ──> │ INITIALIZING │ ──> │ RUNNING │
└─────────────┘     └──────────────┘     └────┬────┘
                                                │
                    ┌──────────────┐           │
                    │   STOPPED    │ <─────────┘
                    └──────────────┘
                           ▲
                           │
                    ┌──────────────┐
                    │  STOPPING    │
                    └──────────────┘
```

### 核心组件交互

```
┌─────────────────────────────────────────┐
│           Application                   │
│  ┌─────────────────────────────────┐   │
│  │    EventBus (事件总线)          │   │
│  │  ┌─────────┐  ┌─────────────┐  │   │
│  │  │ UI Layer│  │Input Manager│  │   │
│  │  └─────────┘  └─────────────┘  │   │
│  └─────────────────────────────────┘   │
│  ┌─────────────────────────────────┐   │
│  │   View Manager (视图管理)       │   │
│  └─────────────────────────────────┘   │
└─────────────────────────────────────────┘
         │                    │
         ▼                    ▼
    ┌─────────┐         ┌─────────┐
    │ SDL3    │         │ ImGui   │
    └─────────┘         └─────────┘
```

### 生命周期回调

```cpp
class MyApp : public Application {
protected:
    // 初始化阶段 - 资源加载和设置
    bool on_init() override {
        LOG_INFO("初始化应用程序");
        return true;  // 返回 false 将终止启动
    }

    // 每帧更新 - 游戏逻辑
    void on_update(double delta_time) override {
        // delta_time: 距离上一帧的时间（秒）
    }

    // 渲染阶段 - 绘制画面
    void on_render() override {
        // SDL3 渲染命令
    }

    // 事件处理 - 用户输入和系统事件
    void on_event(const SDL_Event& event) override {
        // 处理 SDL 事件
    }

    // 关闭阶段 - 资源清理
    void on_shutdown() override {
        LOG_INFO("清理资源");
    }
};
```

## 🚀 快速开始

### 前置要求

- **C++20 编译器**：GCC 11+ / Clang 13+ / MSVC 2022+
- **CMake 3.20+**
- **操作系统**：Windows 10+ / Linux / macOS 10.15+

### 构建步骤

```bash
# 1. 克隆仓库（包含子模块）
git clone --recursive https://github.com/your-org/dearts.git
cd DearTs

# 2. 配置项目
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 3. 构建
cmake --build build --config Release

# 4. 运行测试程序
./build/bin/deartsdl_gui      # Linux/macOS
# 或
build\bin\deartsdl_gui.exe    # Windows
```

### 构建流程详解

DearTs 采用三阶段构建流程：

```cmake
# 阶段 1: 编译第三方库为静态库
├─ SDL3          → SDL3-static.lib
├─ ImGui         → ImGui.lib
├─ FreeType      → libfreetype.a
└─ liblogger     → logger.lib

# 阶段 2: core 源代码直接参与可执行文件编译
add_executable(deartsdl_gui
    main.cpp                                    # 主程序
    ${CMAKE_SOURCE_DIR}/core/app/application.cpp # core 源码
    ${CMAKE_SOURCE_DIR}/core/event/event_bus.cpp
    # ... 更多 core 源文件
)

# 阶段 3: 链接第三方静态库
target_link_libraries(deartsdl_gui PRIVATE
    SDL3-static    # SDL3 静态库
    ImGui          # ImGui 静态库
    logger         # 日志库
)
```

### 基本使用示例

创建一个最小化的 DearTs 应用程序：

```cpp
#include "core/app/application.h"
#include <SDL3/SDL.h>

using namespace DearTs;
using namespace DearTs::Core::App;

class MyApp : public Application {
protected:
    bool on_init() override {
        // 初始化资源
        LOG_INFO("应用程序初始化中...");
        SDL_SetRenderDrawColor(m_renderer, 50, 50, 50, 255);
        return true;  // 返回 false 将终止启动
    }

    void on_update(double delta_time) override {
        // 每帧更新逻辑
        // delta_time: 距离上一帧的时间（秒）
    }

    void on_render() override {
        // 渲染逻辑
        SDL_RenderClear(m_renderer);
        // 添加你的绘制代码
        SDL_RenderPresent(m_renderer);
    }

    void on_event(const SDL_Event& event) override {
        // 事件处理
        if (event.type == SDL_EVENT_KEY_DOWN &&
            event.key.key == SDLK_ESCAPE) {
            request_exit(0);  // 按 ESC 退出
        }
    }

    void on_shutdown() override {
        // 清理资源
        LOG_INFO("应用程序关闭");
    }
};

int main() {
    // 配置应用程序
    ApplicationConfig config;
    config.name = "My DearTs App";
    config.version = "1.0.0";
    config.window_width = 1280;
    config.window_height = 720;
    config.enable_vsync = true;

    // 创建并运行应用程序
    auto app = std::make_unique<MyApp>();
    if (!app->initialize(config)) {
        return -1;
    }

    return app->run();
}
```

### 预期输出

运行测试程序后，你应该看到：
- ✅ 1280x720 的窗口
- ✅ 灰色背景（RGB: 50, 50, 50）
- ✅ 控制台输出日志信息
- ✅ 按 **ESC** 退出程序
- ✅ 按 **SPACE** 显示 FPS 统计

## 📚 API 文档

### ApplicationConfig 结构体

应用程序配置选项，在初始化时传递给 Application。

```cpp
struct ApplicationConfig {
    std::string name;           // 应用程序名称（显示在窗口标题）
    std::string version;        // 版本号
    int window_width;           // 窗口宽度（像素）
    int window_height;          // 窗口高度（像素）
    bool enable_vsync;          // 启用垂直同步（默认: true）
    bool enable_imgui;          // 启用 ImGui 支持（预留，默认: false）
};
```

### Application 公共方法

| 方法 | 说明 | 返回值 |
|------|------|--------|
| `initialize(config)` | 初始化应用程序，创建窗口和渲染器 | `bool` - 成功返回 true |
| `run()` | 启动主循环，阻塞直到程序退出 | `int` - 退出代码 |
| `shutdown()` | 手动关闭应用程序 | `void` |
| `request_exit(code)` | 请求优雅退出，完成当前帧后关闭 | `void` |
| `get_state()` | 获取当前应用状态 | `ApplicationState` |
| `get_config()` | 获取应用程序配置（只读） | `const ApplicationConfig&` |

### Application 状态枚举

```cpp
enum class ApplicationState {
    Uninitialized,   // 未初始化
    Initializing,    // 初始化中
    Running,         // 运行中
    Paused,          // 已暂停（失去焦点）
    Stopping,        // 停止中
    Stopped          // 已停止
};
```

### Application 保护成员

派生类可访问的成员变量：

```cpp
protected:
    // 配置和状态
    ApplicationConfig m_config;      // 应用程序配置
    ApplicationState  m_state;       // 当前状态

    // SDL 资源
    SDL_Window*   m_window;          // SDL 窗口（由基类管理）
    SDL_Renderer* m_renderer;        // SDL 渲染器（由基类管理）

    // 时间和性能统计
    double m_delta_time;             // 上一帧到当前帧的时间（秒）
    double m_current_fps;            // 当前 FPS（每秒更新）
    double m_average_fps;            // 平均 FPS（移动平均）
    uint64_t m_frame_count;          // 总帧数统计

    // ImGui（如果启用）
    ImGuiContext* m_imgui_context;   // ImGui 上下文
```

### EventBus 事件系统

DearTs 提供类型安全的事件总线，支持模块间通信。

```cpp
#include "core/event/event_bus.h"

// 1. 定义事件
struct MyEvent {
    int value;
    std::string message;
};

// 2. 订阅事件
auto guard = EventBus::subscribe<MyEvent>([](const MyEvent& e) {
    LOG_INFO("收到事件: {}", e.message);
});

// 3. 发布事件（同步）
EventBus::publish(MyEvent{42, "Hello"});

// 4. 发布事件（异步）
EventBus::publish_async(MyEvent{100, "World"});

// guard 离开作用域时自动取消订阅
```

**特性**：
- ✅ **类型安全** - 编译时类型检查
- ✅ **RAII 管理** - EventGuard 自动管理订阅生命周期
- ✅ **线程安全** - 支持多线程发布和订阅
- ✅ **异步支持** - 不阻塞主线程的事件处理

## 📖 开发指南

### 代码规范

项目严格遵循 `docs/代码规范.md`，确保代码一致性和可维护性。

#### 命名约定

| 类型 | 约定 | 示例 |
|------|------|------|
| 类名 | PascalCase | `Application`, `EventBus` |
| 函数名 | snake_case | `on_init()`, `get_config()` |
| 成员变量 | snake_case + `m_` 前缀 | `m_window`, `m_delta_time` |
| 常量 | UPPER_CASE | `MAX_FPS`, `DEFAULT_WIDTH` |
| 命名空间 | PascalCase | `DearTs::Core::App` |

#### 代码风格

- **缩进**：4 空格（不使用 Tab）
- **括号**：K&R 风格，左括号不换行
- **行长度**：不超过 120 字符
- **空格**：操作符周围加空格（`a = b + c`）
- **注释**：使用 `//` 进行单行注释，`/* */` 进行多行注释

#### 头文件保护

```cpp
#pragma once  // 首选 #pragma once

// 或使用传统 include guard（如果需要跨平台兼容）
#ifndef DEARTS_CORE_APP_APPLICATION_H
#define DEARTS_CORE_APP_APPLICATION_H
// ...
#endif
```

### 扩展 core 模块

由于 core 是源代码目录，添加新模块非常简单。

#### 步骤 1：创建新模块

```bash
# 在 core/ 下创建新模块目录
mkdir core/input
touch core/input/input_manager.h
touch core/input/input_manager.cpp
```

#### 步骤 2：更新 CMakeLists.txt

编辑 `main/gui/CMakeLists.txt`，添加新模块源文件：

```cmake
set(CORE_SOURCES
    ${CMAKE_SOURCE_DIR}/core/app/application.cpp
    ${CMAKE_SOURCE_DIR}/core/event/event_bus.cpp
    ${CMAKE_SOURCE_DIR}/core/input/input_manager.cpp  # 新增
    ${CMAKE_SOURCE_DIR}/core/input/input_manager.h    # 新增
)

add_executable(deartsdl_gui
    main.cpp
    ${CORE_SOURCES}
)
```

#### 步骤 3：在代码中使用

```cpp
#include "input/input_manager.h"  // 直接包含

class MyApp : public Application {
protected:
    void on_update(double delta_time) override {
        auto& input = InputManager::get_instance();
        if (input.is_key_pressed(SDLK_SPACE)) {
            LOG_INFO("空格键被按下");
        }
    }
};
```

### 添加新的测试程序

创建独立的测试程序来验证特定功能。

```bash
# 创建新测试目录
mkdir examples/event_demo
touch examples/event_demo/main.cpp
touch examples/event_demo/CMakeLists.txt
```

`examples/event_demo/CMakeLists.txt`:

```cmake
add_executable(event_demo
    main.cpp
    ${CMAKE_SOURCE_DIR}/core/app/application.cpp
    ${CMAKE_SOURCE_DIR}/core/event/event_bus.cpp
)

target_link_libraries(event_demo PRIVATE
    SDL3-static
    logger
)
```

### 性能优化建议

#### 1. 帧率控制

```cpp
// 启用 VSync（推荐，防止画面撕裂）
config.enable_vsync = true;

// 禁用 VSync（用于性能测试）
config.enable_vsync = false;
```

#### 2. 事件处理

```cpp
// 使用异步事件避免阻塞主循环
EventBus::publish_async(HeavyEvent{data});

// 在 on_update() 中批量处理事件
void on_update(double delta_time) override {
    process_event_queue();
}
```

#### 3. 日志优化

```cpp
// 使用格式化日志（性能优于字符串拼接）
LOG_INFO("FPS: {:.2f}, Frame: {}", m_current_fps, m_frame_count);

// 避免在热循环中使用高频日志
// 错误：
for (int i = 0; i < 1000000; ++i) {
    LOG_DEBUG("Processing item: {}", i);  // 性能杀手
}

// 正确：
LOG_DEBUG("开始处理 {} 个项目", total_items);
for (int i = 0; i < 1000000; ++i) {
    // 处理逻辑
}
LOG_DEBUG("完成处理，用时 {:.2f}s", elapsed_time);
```

### 调试技巧

#### 1. 启用详细日志

```cpp
// 在 main.cpp 中设置日志级别
Logger::get_instance().set_log_level(LogLevel::Debug);
```

#### 2. FPS 监控

```cpp
void on_update(double delta_time) override {
    static double timer = 0.0;
    timer += delta_time;

    if (timer >= 1.0) {  // 每秒输出一次
        LOG_INFO("FPS: {:.2f} (平均: {:.2f})",
                 m_current_fps, m_average_fps);
        timer = 0.0;
    }
}
```

#### 3. 内存泄漏检测（Linux/macOS）

```bash
# 使用 Valgrind 检测内存泄漏
valgrind --leak-check=full ./build/bin/deartsdl_gui
```

#### 4. 性能分析

```cpp
// 使用 SDL3 的高性能计时器
Uint64 start = SDL_GetTicks();
// ... 执行操作 ...
Uint64 elapsed = SDL_GetTicks() - start;
LOG_INFO("操作耗时: {} ms", elapsed);
```

## 🧪 测试程序

### 主测试程序 (main/gui/main.cpp)

完整的应用程序生命周期测试，涵盖以下功能：

| 功能 | 描述 |
|------|------|
| ✅ 初始化测试 | SDL3 窗口和渲染器创建 |
| ✅ 运行循环测试 | 主循环和帧率控制 |
| ✅ 事件处理测试 | 键盘输入响应 |
| ✅ 关闭流程测试 | 优雅退出和资源清理 |
| ✅ FPS 统计 | 实时帧率和平均帧率显示 |
| ✅ 生命周期验证 | 状态转换验证 |

### 快捷键

| 按键 | 功能 |
|------|------|
| `ESC` | 退出应用程序 |
| `SPACE` | 在控制台显示 FPS 统计信息 |

### 示例输出

```
[INFO] 正在初始化应用程序...
[INFO] SDL3 初始化成功
[INFO] 窗口创建成功: 1280x720
[INFO] 渲染器创建成功
[INFO] 应用程序启动成功
[INFO] FPS: 60.00 (平均: 59.87)
[INFO] 应用程序正在关闭...
[INFO] 退出代码: 0
```

## 🚧 开发路线图

### 已完成 ✅ (v1.0.0)

- [x] Application 生命周期管理系统
- [x] SDL3 窗口和渲染器管理
- [x] 类型安全的事件系统（EventBus）
- [x] 智能帧率控制和 VSync 支持
- [x] 实时 FPS 统计和性能监控
- [x] 异步日志系统集成
- [x] **core 作为源代码直接编译（非链接库）**
- [x] ImGui 集成层
- [x] 多视图管理系统

### 计划中 🚧

#### v1.1.0（输入系统）
- [ ] InputManager - 统一输入管理
- [ ] 键盘状态追踪
- [ ] 鼠标状态管理
- [ ] 手柄支持
- [ ] 输入映射和绑定

#### v1.2.0（渲染系统）
- [ ] RenderModule - 渲染抽象层
- [ ] 纹理管理器
- [ ] 精灵渲染系统
- [ ] 文本渲染优化
- [ ] 着色器管理

#### v1.3.0（资源管理）
- [ ] ResourceManager - 资源加载和缓存
- [ ] 异步资源加载
- [ ] 热重载支持
- [ ] 资源打包系统

#### v2.0.0（高级功能）
- [ ] 多窗口支持
- [ ] 音频系统（基于 SDL_mixer）
- [ ] 物理引擎集成
- [ ] 网络层
- [ ] 脚本系统（Lua/Python）
- [ ] 编辑器工具

### 长期愿景 🌟

- 跨平台移动端支持（iOS/Android）
- WebAssembly 支持
- VR/AR 支持
- 完整的编辑器
- 可视化调试工具

## 🔗 相关资源

### 官方文档

- [SDL3 官方文档](https://wiki.libsdl.org/SDL3/)
- [ImGui 文档](https://github.com/ocornut/imgui)
- [C++20 参考](https://en.cppreference.com/w/cpp/20)
- [CMake 文档](https://cmake.org/documentation/)

### 社区

- [GitHub Issues](https://github.com/your-org/dearts/issues) - 报告问题和请求功能
- [GitHub Discussions](https://github.com/your-org/dearts/discussions) - 社区讨论
- [Discord 服务器](https://discord.gg/your-server) - 实时聊天（如有）

### 参考项目

- [ImHex](https://github.com/WerWolv/ImHex) - 强大的十六进制编辑器（视图系统灵感来源）
- [SDL3 Game](https://github.com/libsdl-org/SDL/) - SDL3 官方示例
- [Oryol](https://github.com/floooh/oryol) - 跨平台游戏框架

## 📄 许可证

本项目采用 **MIT 许可证**。

```
MIT License

Copyright (c) 2024 DearTs Contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

## 🤝 贡献指南

欢迎贡献代码、报告问题或提出建议！

### 如何贡献

1. **Fork 本仓库**
2. **创建功能分支** (`git checkout -b feature/AmazingFeature`)
3. **提交更改** (`git commit -m 'Add some AmazingFeature'`)
4. **推送到分支** (`git push origin feature/AmazingFeature`)
5. **提交 Pull Request**

### 贡献要求

- ✅ 遵循项目的 [代码规范](docs/代码规范.md)
- ✅ 编写清晰的提交信息
- ✅ 添加必要的测试和文档
- ✅ 确保代码通过所有现有测试
- ✅ 保持提交历史清晰

### 代码审查流程

所有 Pull Request 需要至少一位维护者审查通过后才能合并。审查重点：
- 代码质量和规范性
- 功能正确性和完整性
- 性能影响评估
- 文档和注释的完整性

---

<div align="center">

**用 ❤️ 打造的 C++ SDL3 应用程序框架**

[⬆ 返回顶部](#dearts-framework)

</div>
