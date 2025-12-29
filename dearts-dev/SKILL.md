---
name: dearts-dev
description: DearTs Framework 开发技能（2025 最新版）。当用户开发基于 SDL3 + ImGui 的 C++20 现代应用框架、设计事件驱动架构、实现插件系统、创建多视图应用或使用 ImHex 风格设计时使用此技能。适用于 DearTs Framework 相关的所有开发任务，包括应用程序生命周期、类型安全事件系统、Content Registry、UI 组件开发和插件扩展。
---

# DearTs Framework 开发技能（2025 最新版）

## 概述

DearTs 是一个基于 SDL3 + ImGui 的现代 C++20 应用程序框架，参考 ImHex 优秀设计理念，提供完整的应用生命周期管理、类型安全事件系统和模块化插件架构。

**核心特性：**
- ✅ **C++20** - Concepts、Ranges、std::format、Modules
- ✅ **SDL3** - 最新跨平台多媒体库
- ✅ **ImGui 2.13.3** - 支持 Docking、Multi-viewport
- ✅ **类型安全事件系统** - 编译时类型检查的事件总线
- ✅ **Result 类型** - 统一错误处理，函数式编程支持
- ✅ **Content Registry** - 命令、工具、设置注册系统
- ✅ **多视图系统** - 参考 ImHex 的停靠窗口管理
- ✅ **插件架构** - API 版本检查，动态加载支持
- ✅ **异步日志** - 基于 liblogger 的高性能日志
- ✅ **源码直接编译** - 无中间库，简化构建

## 技术栈

### 核心技术
- **语言**: C++20 (GCC 11+ / Clang 13+ / MSVC 2022+)
- **图形**: SDL3 (静态库)
- **GUI**: ImGui 2.13.3 + SDL3 后端 + FreeType
- **字体**: FreeType 2.13.3
- **构建**: CMake 3.20+
- **日志**: liblogger (异步、线程安全)

### 可选依赖
- **SDL_image** - 图像格式支持
- **SDL_mixer** - 音频播放
- **SDL_ttf** - TrueType 字体
- **WinToast** - Windows 通知
- **cppjieba** - 中文分词

## 项目架构（2025 最新）

### 核心目录结构

```
DearTs/
├── core/                          # 核心源码（直接编译）
│   ├── app/                       # 应用程序生命周期
│   │   ├── application.h/cpp      # Application 基类
│   │   └── state.h                # 应用状态定义
│   ├── event/                     # 类型安全事件系统
│   │   ├── event_bus.h            # 事件总线
│   │   └── events.h               # 内置事件
│   ├── ui/                        # 用户界面系统
│   │   ├── imgui_layer.h          # SDL3 + ImGui 集成
│   │   ├── view.h                 # 视图基类
│   │   ├── view_manager.h         # 视图管理器
│   │   ├── command_palette.h      # 命令调色板
│   │   ├── title_bar.h            # 自定义标题栏
│   │   ├── theme_manager.h        # 主题管理
│   │   ├── shortcut_manager.h     # 快捷键管理
│   │   ├── layout_manager.h       # 布局管理
│   │   └── task_widget.h          # 任务小部件
│   ├── content/                   # Content Registry
│   │   ├── registry_base.h        # 注册表基类
│   │   ├── commands.h             # 命令系统
│   │   ├── tools.h                # 工具系统
│   │   ├── settings.h             # 设置系统
│   │   ├── project_manager.h      # 项目管理
│   │   └── callbacks.h            # 回调系统
│   ├── tasks/                     # 任务系统
│   │   └── task_manager.h         # 异步任务管理
│   ├── plugin/                    # 插件系统
│   │   └── plugin.h               # 插件接口
│   ├── config/                    # 配置管理
│   │   └── config_manager.h       # 配置文件管理
│   └── result.h                   # Result 类型
├── lib/                           # 外部库
│   └── liblogger/                 # 异步日志库
├── third_party/                   # 第三方依赖
│   ├── SDL/                       # SDL3
│   ├── imgui/                     # ImGui + 绑定
│   ├── freetype/                  # FreeType
│   └── ...
├── main/gui/                      # GUI 入口
│   ├── main.cpp
│   └── CMakeLists.txt
├── examples/                      # 示例代码
│   └── demo_imhex_style/          # ImHex 风格演示
├── docs/                          # 文档
│   ├── architecture_improvements.md
│   └── diagrams/                  # 架构图 PNG
└── .claude-skills/                # Claude Code 技能
    ├── dearts-app-generator
    ├── dearts-documentation
    └── ...
```

## 核心组件详解

### 1. Result 类型 - 统一错误处理

**文件**: `core/result.h`

```cpp
// 定义
template<typename T, typename E = std::string>
class Result {
    T m_value;
    E m_error;
    bool m_has_value;
public:
    static Result ok(T value);
    static Result err(E error);

    // 函数式编程
    template<typename F>
    auto map(F&& f) -> Result<std::invoke_result_t<F, T>, E>;

    template<typename F>
    auto and_then(F&& f) -> /* ... */;

    template<typename F>
    auto or_else(F&& f) -> /* ... */;

    bool is_ok() const;
    bool is_err() const;
    T& unwrap();
    E& unwrap_err();
};

// 使用示例
Result<int, std::string> divide(int a, int b) {
    if (b == 0) return Result::err("Division by zero");
    return Result::ok(a / b);
}

// 链式调用
auto result = divide(10, 2)
    .map([](int v) { return v * 2; })        // 乘以 2
    .and_then([](int v) { return save(v); }); // 保存
```

**优势**：
- 编译时类型安全
- 强制错误处理
- 支持函数式编程
- 无异常开销
- 提供 void 特化版本

**完整 API 手册**: `references/config_manager_api.md`

### 2. Logger - 高性能日志系统

**文件**: `liblogger/logger.h`

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

**特性**：
- ✅ 线程安全
- ✅ 异步文件写入
- ✅ 重复日志过滤
- ✅ std::format 格式化
- ✅ 六个日志级别（TRACE, DEBUG, INFO, WARN, ERROR, FATAL）

**完整 API 手册**: `references/logger_api.md`

### 3. TaskManager - 异步任务管理

**文件**: `core/tasks/task_manager.h`

```cpp
// 创建并启动任务
auto task = TaskManager::instance().launch(
    "加载文件",
    [](const std::atomic<bool>& should_cancel) {
        for (int i = 0; i < 100; i++) {
            if (should_cancel) {
                LOG_INFO("任务被取消");
                return;
            }

            // 执行工作...
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
);

// 检查状态
if (task->is_running()) {
    LOG_INFO("任务正在运行...");
}

// 取消任务
TaskManager::instance().cancel(task);
```

**特性**：
- ✅ 异步执行
- ✅ 进度跟踪
- ✅ 任务取消
- ✅ 多种任务类型（Normal, Background, Blocking, Critical）
- ✅ 完成回调

**完整 API 手册**: `references/task_manager_api.md`

### 4. ConfigManager - 配置管理

**文件**: `core/config/config_manager.h`

```cpp
// 设置配置值
ConfigManager::instance().set("app.window.width", 1280);
ConfigManager::instance().set("app.theme", "Dark");

// 获取配置值
auto width = ConfigManager::instance().get<int>("app.window.width");
if (width.isOk()) {
    int w = width.unwrap();
    LOG_INFO("Window width: {}", w);
}

// 使用默认值
int height = ConfigManager::instance().get_or("app.window.height", 720);

// 注册元数据
ConfigManager::instance().register_meta("app.window.width", {
    .description = "Window width",
    .default_value = 1280,
    .is_required = false,
    .validate_callback = [](const ConfigValue& value) {
        if (std::holds_alternative<int>(value)) {
            int w = std::get<int>(value);
            if (w < 640) {
                return Result::err("Width must be at least 640");
            }
        }
        return Result::ok();
    }
});

// 使用 ConfigScope（RAII）
ConfigScope scope{"app.window"};
scope.set("width", 1280);   // 实际键: "app.window.width"
```

**特性**：
- ✅ 类型安全（bool, int, double, string）
- ✅ 层级键（点号分隔）
- ✅ 元数据支持
- ✅ 验证和变更回调
- ✅ ConfigScope RAII 管理

**完整 API 手册**: `references/config_manager_api.md`

### 5. 类型安全事件系统

**文件**: `core/event/event_bus.h`

```cpp
// 定义事件（任意结构体）
struct WindowCloseEvent {
    int window_id;
    const char* title;
};

struct DataModifiedEvent {
    size_t offset;
    size_t size;
    std::vector<uint8_t> data;
};

// 订阅事件
auto token = EventBus::instance().subscribe<WindowCloseEvent>(
    [](const WindowCloseEvent& e) {
        LOG_INFO("Window {} closed", e.window_id);
    }
);

// 发布事件
EventBus::instance().publish(WindowCloseEvent{
    .window_id = 42,
    .title = "Main Window"
});

// RAII 自动取消订阅
{
    auto guard = make_event_guard<DataModifiedEvent>(
        [](const DataModifiedEvent& e) {
            // 处理事件
        }
    );
    // guard 析构时自动取消订阅
}
```

**优势**：
- 编译时类型检查（无需手动 ID）
- RAII 自动管理
- 线程安全
- 支持异步事件队列

参见 `references/event_system.md` 详细文档。

### 3. Content Registry

**文件**: `core/content/registry_base.h`

ImHex 风格的内容注册系统，用于管理命令、工具、设置等。

#### 3.1 命令系统

```cpp
// 注册命令
ContentRegistry::Commands::registerHandler(
    "dearts.file.save",                   // 命令 ID
    "Save current file",                  // 显示名称
    [](const auto& params) {
        // 执行保存
        saveFile();
    },
    [](const auto& params) -> bool {
        // 查询是否可用
        return hasUnsavedChanges();
    }
);

// 执行命令
ContentRegistry::Commands::invoke("dearts.file.save");
```

#### 3.2 工具系统

```cpp
// 注册工具
ContentRegistry::Tools::add(
    "Hex Editor",                         // 工具名称
    []() {
        // 创建工具窗口
        ImGui::Begin("Hex Editor");
        // ... 工具内容
        ImGui::End();
    }
);
```

#### 3.3 设置系统

```cpp
// 注册设置
ContentRegistry::Settings::add(
    "dearts.general.auto_save",           // 设置键
    "Auto Save",                          // 显示名称
    true,                                 // 默认值
    "Automatically save files"            // 描述
);

// 读取设置
bool autoSave = ContentRegistry::Settings::read(
    "dearts.general.auto_save", true
);

// 写入设置
ContentRegistry::Settings::write(
    "dearts.general.auto_save", false
);
```

**完整 API 手册**: `references/content_registry_api.md`

### 6. Plugin System - 插件系统

**文件**: `core/plugin/plugin.h`

```cpp
// 定义插件类
class MyPlugin : public IPlugin {
public:
    // 必须实现：获取插件信息
    PluginInfo get_info() const override {
        return PluginInfo{
            .name = "MyPlugin",
            .author = "DearTs Team",
            .description = "My awesome plugin",
            .version = "1.0.0",
            .api_version = "1.0.0"
        };
    }

    // 可选实现：插件加载时调用
    Result<void, std::string> on_load() override {
        LOG_INFO("MyPlugin: Loading...");

        // 注册命令
        ContentRegistry::Commands::register_handler(
            "myplugin.action",
            "My Action",
            []() {
                LOG_INFO("Action executed!");
            }
        );

        // 注册视图
        ContentRegistry::Views::add<MyView>();

        return Result::ok();
    }

    // 可选实现：插件卸载时调用
    void on_unload() override {
        LOG_INFO("MyPlugin: Unloading...");
    }
};

// 注册插件
PluginManager::instance().add_builtin(
    std::make_unique<MyPlugin>()
);
```

**特性**：
- ✅ API 版本检查
- ✅ 生命周期管理（加载、卸载、启用、禁用）
- ✅ 类型安全（Result 错误处理）
- ✅ 与 Content Registry、EventBus 集成
- ✅ 支持内置插件和动态插件
- ✅ 自动资源管理（RAII）

**完整 API 手册**: `references/plugin_system_api.md`

#### 4.1 ImGui Layer

**文件**: `core/ui/imgui_layer.h`

SDL3 + ImGui 集成层，管理 ImGui 生命周期。

```cpp
class ImGuiLayer : public IApplicationLayer {
public:
    bool onAttach() override;
    void onDetach() override;
    void onEvent(const SDL_Event& event) override;
    void onRender() override;

private:
    ImGuiContext* m_context;
    SDL_Window* m_window;
};
```

#### 4.2 视图系统

**文件**: `core/ui/view.h`, `core/ui/view_manager.h`

```cpp
// 创建视图
class HexEditorView : public View {
public:
    std::string getName() const override {
        return "Hex Editor";
    }

    void drawContent() override {
        ImGui::Text("Hex Editor Content");
        // ... 视图内容
    }
};

// 注册视图
ViewManager::instance().addView<HexEditorView>();

// 显示/隐藏视图
ViewManager::instance().showView("Hex Editor");
```

#### 4.3 命令调色板

**文件**: `core/ui/command_palette.h`

类似 VS Code 的命令调色板。

```cpp
// 显示命令调色板
CommandPalette::instance().open();

// 注册命令到调色板
CommandPalette::instance().addCommand({
    .id = "file.save",
    .name = "File: Save",
    .shortcut = "Ctrl+S",
    .callback = []() { saveFile(); }
});
```

#### 4.4 自定义标题栏

**文件**: `core/ui/title_bar.h`

```cpp
TitleBar::instance().render();

// 自定义按钮
TitleBar::instance().addButton({
    .icon = ICON_FA_SAVE,
    .tooltip = "Save",
    .onClick = []() { saveFile(); }
});
```

参见 `references/ui_system.md` 详细文档。

### 5. 插件系统

**文件**: `core/plugin/plugin.h`

```cpp
// 定义插件
class MyPlugin : public Plugin {
public:
    std::string getName() const override {
        return "My Plugin";
    }

    std::string getVersion() const override {
        return "1.0.0";
    }

    void onLoad() override {
        // 注册命令、工具等
        ContentRegistry::Commands::registerHandler(...);
    }

    void onUnload() override {
        // 清理资源
    }
};

// 注册插件
REGISTER_PLUGIN(MyPlugin, "1.0.0");
```

参见 `references/plugin_system.md` 详细文档。

### 6. 任务系统

**文件**: `core/tasks/task_manager.h`

```cpp
// 创建任务
auto task = TaskManager::instance().createTask(
    "Loading File",                       // 任务名称
    [](Task& task) {
        for (int i = 0; i < 100; i++) {
            task.updateProgress(i);
            // ... 处理逻辑
        }
    }
);

// 异步执行
task.executeAsync();

// 取消任务
task.cancel();
```

## Application 生命周期

### 状态机

```
UNINITIALIZED → INITIALIZING → RUNNING ←→ PAUSED
                            ↓         ↓
                         STOPPING ←──┘
                            ↓
                        STOPPED
```

### 生命周期钩子

```cpp
class MyApp : public dearts::Application {
public:
    // 1. 初始化阶段
    bool onInitialize() override {
        LOG_INFO("Initializing application");
        return true;  // 返回 false 将中止启动
    }

    // 2. 每帧更新
    void onUpdate(float deltaTime) override {
        // deltaTime: 距离上一帧的时间（秒）
    }

    // 3. 渲染阶段
    void onRender() override {
        // ImGui 渲染
        ImGui::Begin("Main Window");
        ImGui::Text("FPS: %.2f", getFPS());
        ImGui::End();
    }

    // 4. 事件处理
    void onEvent(SDL_Event& event) override {
        // 处理 SDL 事件
    }

    // 5. 关闭阶段
    void onShutdown() override {
        LOG_INFO("Shutting down");
    }
};
```

## 开发规范

### 命名约定

- **类名**: `PascalCase` (例如 `Application`)
- **函数名**: `snake_case` (例如 `on_initialize`)
- **成员变量**: `m_camelCase` (例如 `m_window`)
- **常量**: `kPascalCase` (例如 `kMaxFPS`)
- **文件名**: `snake_case.h` (例如 `application.h`)

### 代码格式

使用 `.clang-format`：
- 4 空格缩进
- K&R 括号风格
- 行长度 ≤ 120 字符

### C++20 特性

```cpp
// Concepts
template<typename T>
concept Integral = std::is_integral_v<T>;

template<Integral T>
T add(T a, T b) { return a + b; }

// Ranges
std::vector<int> numbers = {1, 2, 3, 4, 5};
auto evens = numbers | std::views::filter([](int n) {
    return n % 2 == 0;
});

// std::format
std::string s = std::format("FPS: {:.2f}", fps);

// 结构化绑定
for (const auto& [key, value] : settings) {
    // ...
}

// std::optional
std::optional<int> parseInt(std::string_view s);
```

## 构建系统

### CMake 配置

```bash
# Debug 构建
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug

# Release 构建
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# 运行
./build/bin/deartsdl_gui      # Linux/macOS
build\bin\deartsdl_gui.exe    # Windows
```

### 添加新模块

```cmake
# main/gui/CMakeLists.txt
add_executable(deartsdl_gui
    main.cpp
    ${CMAKE_SOURCE_DIR}/core/app/application.cpp
    ${CMAKE_SOURCE_DIR}/core/event/event_bus.cpp  # 新增
    ${CMAKE_SOURCE_DIR}/core/ui/view_manager.cpp  # 新增
)

target_include_directories(deartsdl_gui
    PRIVATE
        ${CMAKE_SOURCE_DIR}/core/app
        ${CMAKE_SOURCE_DIR}/core/event  # 新增
        ${CMAKE_SOURCE_DIR}/core/ui     # 新增
)
```

## 快速示例

### 创建 ImHex 风格应用

参见 `examples/demo_imhex_style/` 完整示例。

```cpp
class ImHexStyleApp : public dearts::Application {
    bool onInitialize() override {
        // 初始化 ImGui Layer
        addLayer(std::make_shared<ImGuiLayer>());

        // 注册视图
        ContentRegistry::Views::registerView<HexEditorView>();
        ContentRegistry::Views::registerView<DataInspectorView>();

        // 注册命令
        ContentRegistry::Commands::registerHandler("file.open", ...);
        ContentRegistry::Commands::registerHandler("file.save", ...);

        // 添加标题栏按钮
        TitleBar::instance().addButton(...);

        return true;
    }

    void onRender() override {
        // 渲染停靠窗口
        ImGui::DockSpaceOverViewport();

        // 渲染标题栏
        TitleBar::instance().render();

        // 渲染视图
        ViewManager::instance().render();
    }
};
```

## 参考资源

### references/

详细技术文档：

**核心 API 手册（新）：**
- `references/config_manager_api.md` - **ConfigManager 完全手册** - 配置管理 API
- `references/logger_api.md` - **Logger 完全手册** - 日志系统 API
- `references/task_manager_api.md` - **TaskManager 完全手册** - 任务管理 API
- `references/plugin_system_api.md` - **Plugin System 完全手册** - 插件系统 API

**其他参考文档：**
- `references/result_type.md` - Result 类型详解
- `references/event_system.md` - 事件系统详解
- `references/content_registry.md` - Content Registry 详解
- `references/ui_system.md` - UI 系统详解
- `references/plugin_system.md` - 插件系统详解
- `references/task_system.md` - 任务系统详解
- `references/sdl3_init.md` - SDL3 初始化指南
- `references/imgui_integration.md` - ImGui 集成指南
- `references/application_api.md` - Application 类 API
- `references/cmake_config.md` - CMake 配置详解
- `references/logging_guide.md` - 日志系统使用指南

### assets/

代码模板：

- `assets/app_template.cpp` - 应用程序模板
- `assets/view_template.cpp` - 视图模板
- `assets/plugin_template.cpp` - 插件模板
- `assets/cmake_template.txt` - CMake 模板

### docs/diagrams/

14 个高分辨率架构图 PNG：

1. 目录结构图
2. 应用程序生命周期
3. 核心类关系图
4. 主循环流程
5. 模块依赖关系
6. 事件处理流程
7. 渲染管线
8. 数据流向
9. 构建系统
10. 内存管理架构
11. 性能监控系统
12. 错误处理流程
13. 扩展模块规划
14. 版本演进路线

## 最佳实践

### 1. 使用 Result 类型处理错误

```cpp
// ✅ 推荐
Result<std::string, Error> loadFile(const std::string& path) {
    if (!exists(path)) {
        return Result::err(Error::FileNotFound);
    }
    return Result::ok(readFile(path));
}

// ❌ 不推荐
std::string loadFile(const std::string& path) {
    if (!exists(path)) {
        return "";  // 错误信息丢失
    }
    return readFile(path);
}
```

### 2. 使用事件系统解耦

```cpp
// ✅ 推荐
EventBus::instance().publish(FileLoadedEvent{
    .path = filePath,
    .content = content
});

// ❌ 不推荐
fileLoadedCallback(filePath, content);  // 紧耦合
```

### 3. 使用 Content Registry

```cpp
// ✅ 推荐
ContentRegistry::Commands::registerHandler("file.save", ...);

// ❌ 不推荐
if (key == "Ctrl+S") {
    saveFile();
}
```

### 4. 使用 RAII 管理资源

```cpp
// ✅ 推荐
{
    auto subscription = EventBus::instance().subscribe<Event>(handler);
    // ...
}  // 自动取消订阅

// ❌ 不推荐
auto token = EventBus::instance().subscribe<Event>(handler);
// ...
EventBus::instance().unsubscribe(token);  // 容易忘记
```

## 相关链接

- SDL3 文档: https://wiki.libsdl.org/SDL3/
- ImGui GitHub: https://github.com/ocornut/imgui
- ImHex GitHub: https://github.com/WerWolv/ImHex
- CMake 文档: https://cmake.org/documentation/
- C++ 参考: https://en.cppreference.com/w/cpp
- 项目文档: `docs/` 目录

## 技能使用场景

当您需要：
- 创建基于 SDL3 + ImGui 的 C++ 应用
- 实现类型安全的事件系统
- 使用 Content Registry 管理命令/工具/设置
- 开发插件系统
- 创建多视图停靠窗口
- 使用 Result 类型进行错误处理
- 实现命令调色板
- 参考 ImHex 架构设计
- **使用 ConfigManager 管理应用配置**
- **使用 Logger 记录应用日志**
- **使用 TaskManager 执行异步任务**
- **开发插件系统**
- **创建 UI 插件（视图、命令、工具）**
- **查看 API 文档而非阅读源码**

此技能将自动激活并提供专业指导！

---

## 快速 API 参考

### ConfigManager - 配置管理

```cpp
#include "core/config/config_manager.h"

// 设置配置
ConfigManager::instance().set("app.window.width", 1280);

// 获取配置（带默认值）
int width = ConfigManager::instance().get_or<int>("app.window.width", 1280);

// 使用 ConfigScope
ConfigScope scope{"app.window"};
scope.set("height", 720);
```

**完整文档**: `references/config_manager_api.md`

### Logger - 日志系统

```cpp
#include "liblogger/logger.h"

// 设置日志级别
Logger::get_instance().set_level(LogLevel::DEBUG);

// 启用文件输出
Logger::get_instance().enable_file_output("logs/app.log");

// 记录日志
LOG_INFO("Application started");
LOG_ERROR("Failed: {}", error);
```

**完整文档**: `references/logger_api.md`

### TaskManager - 任务管理

```cpp
#include "core/tasks/task_manager.h"

// 启动任务
auto task = TaskManager::instance().launch("任务名", [](auto& cancel) {
    for (int i = 0; i < 100; i++) {
        if (cancel) return;
        do_work(i);
    }
});

// 检查状态
if (task->is_running()) {
    LOG_INFO("Progress: {:.1f}%", task->get_progress_percent() * 100);
}
```

**完整文档**: `references/task_manager_api.md`

### Result 类型 - 错误处理

```cpp
#include "core/result.h"

// 使用 Result
Result<Data, std::string> load_data() {
    if (error) {
        return Result::err("Failed to load");
    }
    return Result::ok(data);
}

// 链式调用
auto result = load_data()
    .map([](Data d) { return process(d); })
    .and_then([](Data d) { return save(d); });

if (result.isErr()) {
    LOG_ERROR("Error: {}", result.error());
}
```

**完整文档**: `references/result_type.md`

### EventBus - 事件系统

```cpp
#include "core/event/event_bus.h"

// 订阅事件
auto token = EventBus::instance().subscribe<DataModifiedEvent>(
    [](const DataModifiedEvent& e) {
        LOG_INFO("Data modified: {} bytes", e.size);
    }
);

// 发布事件
EventBus::instance().publish(DataModifiedEvent{ .size = 1024 });
```

**完整文档**: `references/event_system.md`

### Content Registry - 注册系统

```cpp
#include "core/content/commands.h"

// 注册命令
ContentRegistry::Commands::register_handler(
    "file.save",
    "Save File",
    []() { save_file(); },
    nullptr,
    "Ctrl+S"
);

// 注册视图
ContentRegistry::Views::add<MyView>();
```

**完整文档**: `references/content_registry.md`

### Plugin System - 插件系统

```cpp
#include "core/plugin/plugin.h"

// 定义插件
class MyPlugin : public IPlugin {
public:
    PluginInfo get_info() const override {
        return PluginInfo{
            .name = "MyPlugin",
            .author = "DearTs Team",
            .description = "My plugin",
            .version = "1.0.0",
            .api_version = "1.0.0"
        };
    }

    Result<void, std::string> on_load() override {
        // 注册命令
        ContentRegistry::Commands::register_handler("my.action", ...);

        // 注册视图
        ContentRegistry::Views::add<MyView>();

        return Result::ok();
    }
};

// 注册插件
PluginManager::instance().add_builtin(
    std::make_unique<MyPlugin>()
);
```

**完整文档**: `references/plugin_system_api.md`
