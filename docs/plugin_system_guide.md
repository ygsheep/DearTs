# DearTs Framework 插件系统详解

## 概述

DearTs Framework 提供了一个强大而灵活的插件系统，参考了 ImHex 的设计理念。插件系统允许开发者动态扩展应用功能，而无需修改核心代码库。

### 核心特性

- ✅ **API 版本检查** - 确保插件与框架兼容
- ✅ **生命周期管理** - 加载、卸载、启用、禁用
- ✅ **类型安全** - 使用 C++20 类型系统和 Result 错误处理
- ✅ **单例模式** - PluginManager 全局管理
- ✅ **动态加载** - 支持从动态库加载插件（预留）
- ✅ **内置插件** - 支持编译时集成插件

## 架构设计

### 核心组件

```
┌─────────────────────────────────────────────┐
│           PluginManager (单例)             │
│  ┌────────────────────────────────────┐   │
│  │    PluginWrapper (插件包装)        │   │
│  │  ┌──────────────────────────────┐  │   │
│  │  │      IPlugin (插件接口)      │  │   │
│  │  └──────────────────────────────┘  │   │
│  └────────────────────────────────────┘   │
└─────────────────────────────────────────────┘
```

### 插件生命周期

```
未加载 (Unloaded)
    ↓ 加载
已加载 (Loaded)
    ↓ 启用
已启用 (Enabled)
    ↓ 禁用
已加载 (Loaded)
    ↓ 卸载
未加载 (Unloaded)
```

## 核心类详解

### 1. IPlugin - 插件接口

所有插件必须继承此类并实现虚函数。

```cpp
class IPlugin {
public:
    virtual ~IPlugin() = default;

    // 获取插件信息（必须实现）
    virtual PluginInfo get_info() const = 0;

    // 生命周期钩子（可选实现）
    virtual Result<void, std::string> on_load();
    virtual void on_unload();
    virtual void on_enable();
    virtual void on_disable();
};
```

**插件信息结构**：

```cpp
struct PluginInfo {
    std::string name;           // 插件名称（唯一标识）
    std::string author;         // 插件作者
    std::string description;    // 插件描述
    std::string version;        // 插件版本（如 "1.0.0"）
    std::string api_version;    // 需要的 API 版本（如 "1.0.0"）

    // 检查 API 版本兼容性
    bool is_api_compatible(const std::string& current_api_version) const;
};
```

### 2. PluginManager - 插件管理器

单例模式，全局管理所有插件。

```cpp
// 获取单例实例
PluginManager& manager = PluginManager::instance();

// 添加内置插件
Result<void, std::string> result = manager.add_builtin(
    std::make_unique<MyPlugin>()
);

// 从文件加载动态插件
Result<void, std::string> result = manager.load_from_file(
    "plugins/my_plugin.dll"
);

// 从目录加载所有插件
Result<size_t, std::string> result = manager.load_from_directory(
    "plugins/"
);

// 启用/禁用插件
manager.enable("MyPlugin");
manager.disable("MyPlugin");

// 卸载插件
bool success = manager.unload("MyPlugin");

// 获取插件信息
std::vector<PluginInfo> plugins = manager.get_all_plugins_info();

// 获取插件指针
IPlugin* plugin = manager.get_plugin("MyPlugin");

// 获取插件状态
Result<PluginState, std::string> state = manager.get_plugin_state("MyPlugin");
```

### 3. PluginWrapper - 插件包装

包装插件实例，管理插件状态和生命周期。

```cpp
class PluginWrapper {
public:
    PluginState get_state() const;
    const std::string& get_error() const;
    IPlugin* get() const;

    Result<void, std::string> load();
    void unload();
    void enable();
    void disable();
};
```

## 创建插件

### 方法 1: 内置插件（编译时集成）

适用于随主程序一起分发的插件。

```cpp
// 1. 定义插件类
class MyPlugin : public DearTs::Core::Plugin::IPlugin {
public:
    // 获取插件信息
    PluginInfo get_info() const override {
        return PluginInfo{
            .name = "My Plugin",
            .author = "Your Name",
            .description = "My awesome plugin",
            .version = "1.0.0",
            .api_version = "1.0.0"
        };
    }

    // 加载时调用
    Result<void, std::string> on_load() override {
        LOG_INFO("MyPlugin: Loading...");

        // 注册命令
        ContentRegistry::Commands::registerHandler(
            "myplugin.action",
            "My Action",
            []() {
                LOG_INFO("Action executed!");
            }
        );

        // 注册视图
        ViewManager::instance().addView<MyPluginView>();

        // 注册工具
        ContentRegistry::Tools::add(
            "My Tool",
            []() {
                ImGui::Text("My Tool Content");
            }
        );

        LOG_INFO("MyPlugin: Loaded successfully");
        return Result<void, std::string>::ok();
    }

    // 卸载时调用
    void on_unload() override {
        LOG_INFO("MyPlugin: Unloading...");

        // 清理资源（框架会自动取消订阅等）
    }

    // 启用时调用
    void on_enable() override {
        LOG_INFO("MyPlugin: Enabled");
    }

    // 禁用时调用
    void on_disable() override {
        LOG_INFO("MyPlugin: Disabled");
    }
};

// 2. 注册内置插件
void registerPlugins() {
    auto result = PluginManager::instance().add_builtin(
        std::make_unique<MyPlugin>()
    );

    if (result.isErr()) {
        LOG_ERROR("Failed to load plugin: {}", result.error());
    }
}
```

### 方法 2: 动态插件（运行时加载）

适用于可选的第三方插件（未来支持）。

```cpp
// 插件动态库需要导出创建和销毁函数
extern "C" {
    __declspec(dllexport) IPlugin* dearts_create_plugin() {
        return new MyPlugin();
    }

    __declspec(dllexport) void dearts_destroy_plugin(IPlugin* plugin) {
        delete plugin;
    }
}

// 主程序从文件加载
auto result = PluginManager::instance().load_from_file(
    "plugins/my_plugin.dll"
);
```

## 插件功能集成

### 1. 注册命令

```cpp
class CommandPlugin : public IPlugin {
    Result<void, std::string> on_load() override {
        ContentRegistry::Commands::registerHandler(
            "myplugin.hello",
            "Say Hello",
            []() {
                LOG_INFO("Hello from MyPlugin!");
            },
            nullptr,
            "Ctrl+Shift+H"  // 快捷键
        );

        return Result<void, std::string>::ok();
    }
};
```

### 2. 注册视图

```cpp
// 定义视图
class MyPluginView : public View {
public:
    std::string getName() const override {
        return "My Plugin View";
    }

    void drawContent() override {
        ImGui::Text("Hello from plugin view!");
    }
};

// 在插件中注册
class ViewPlugin : public IPlugin {
    Result<void, std::string> on_load() override {
        ViewManager::instance().addView<MyPluginView>();
        return Result<void, std::string>::ok();
    }
};
```

### 3. 注册工具

```cpp
class ToolPlugin : public IPlugin {
    Result<void, std::string> on_load() override {
        ContentRegistry::Tools::add(
            "My Plugin Tool",
            []() {
                if (ImGui::Begin("My Plugin Tool")) {
                    ImGui::Text("Tool content");
                }
                ImGui::End();
            }
        );

        return Result<void, std::string>::ok();
    }
};
```

### 4. 订阅事件

```cpp
class EventPlugin : public IPlugin {
private:
    EventBus::Token m_eventToken;

public:
    Result<void, std::string> on_load() override {
        // 订阅事件
        m_eventToken = EventBus::instance().subscribe<DataModifiedEvent>(
            [](const DataModifiedEvent& e) {
                LOG_INFO("Data modified: {} bytes at 0x{:X}",
                         e.size, e.offset);
            }
        );

        return Result<void, std::string>::ok();
    }

    void on_unload() override {
        // Token 会自动取消订阅
    }
};
```

### 5. 注册设置

```cpp
class SettingsPlugin : public IPlugin {
    Result<void, std::string> on_load() override {
        ContentRegistry::Settings::add(
            "myplugin.auto_save",
            "Auto Save",
            true
        );

        ContentRegistry::Settings::add(
            "myplugin.interval",
            "Save Interval (seconds)",
            60
        );

        return Result<void, std::string>::ok();
    }
};
```

## 完整示例

### 示例 1: 简单功能插件

```cpp
#pragma once

#include "core/plugin/plugin.h"
#include "core/content/commands.h"
#include "liblogger/logger.h"

class HelloWorldPlugin : public DearTs::Core::Plugin::IPlugin {
public:
    PluginInfo get_info() const override {
        return PluginInfo{
            .name = "HelloWorld",
            .author = "DearTs Team",
            .description = "A simple hello world plugin",
            .version = "1.0.0",
            .api_version = "1.0.0"
        };
    }

    Result<void, std::string> on_load() override {
        LOG_INFO("HelloWorldPlugin: Loading...");

        // 注册命令
        ContentRegistry::Commands::registerHandler(
            "helloworld.say",
            "Say Hello",
            []() {
                ImGui::OpenPopup("Hello Popup");
                LOG_INFO("Hello from plugin!");
            }
        );

        // 渲染弹窗
        m_renderToken = ImGui::GetIO().MetricsActiveTotal++;
        ImGui::GetIO().MetricsActiveTotal--;

        LOG_INFO("HelloWorldPlugin: Loaded");
        return Result<void, std::string>::ok();
    }

    void on_unload() override {
        LOG_INFO("HelloWorldPlugin: Unloaded");
    }

    void on_enable() override {
        LOG_INFO("HelloWorldPlugin: Enabled");
    }

    void on_disable() override {
        LOG_INFO("HelloWorldPlugin: Disabled");
    }

    // 可选：渲染钩子
    void onRender() {
        if (ImGui::BeginPopup("Hello Popup")) {
            ImGui::Text("Hello from HelloWorld Plugin!");
            if (ImGui::Button("Close")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

private:
    int m_renderToken = 0;
};
```

### 示例 2: 数据处理插件

```cpp
class DataProcessorPlugin : public IPlugin {
public:
    PluginInfo get_info() const override {
        return PluginInfo{
            .name = "DataProcessor",
            .author = "Data Team",
            .description = "Advanced data processing plugin",
            .version = "2.0.0",
            .api_version = "1.0.0"
        };
    }

    Result<void, std::string> on_load() override {
        // 注册命令
        ContentRegistry::Commands::registerHandler(
            "dataprocessor.analyze",
            "Analyze Data",
            [this]() {
                analyzeData();
            }
        );

        ContentRegistry::Commands::registerHandler(
            "dataprocessor.export",
            "Export Data",
            [this]() {
                exportData();
            }
        );

        // 订阅事件
        m_dataEventToken = EventBus::instance().subscribe<DataModifiedEvent>(
            [this](const DataModifiedEvent& e) {
                onDataModified(e);
            }
        );

        // 注册视图
        ViewManager::instance().addView<DataAnalysisView>();

        // 注册设置
        ContentRegistry::Settings::add(
            "dataprocessor.auto_analyze",
            "Auto Analyze",
            true
        );

        LOG_INFO("DataProcessorPlugin: Loaded successfully");
        return Result<void, std::string>::ok();
    }

    void on_unload() override {
        LOG_INFO("DataProcessorPlugin: Cleanup");
    }

private:
    void analyzeData() {
        LOG_INFO("Analyzing data...");
        // 分析逻辑
    }

    void exportData() {
        LOG_INFO("Exporting data...");
        // 导出逻辑
    }

    void onDataModified(const DataModifiedEvent& event) {
        if (auto_analyze) {
            analyzeData();
        }
    }

    EventBus::Token m_dataEventToken;
    bool auto_analyze = true;
};
```

## 插件最佳实践

### 1. 插件命名规范

```cpp
// ✅ 推荐 - PascalCase，清晰描述功能
class HexEditorPlugin : public IPlugin { };
class DataInspectorPlugin : public IPlugin { };

// ❌ 不推荐 - 模糊的名称
class Plugin1 : public IPlugin { };
class MyPlugin : public IPlugin { };
```

### 2. 错误处理

```cpp
// ✅ 推荐 - 使用 Result 类型
Result<void, std::string> on_load() override {
    if (!initializeResources()) {
        return Result::err("Failed to initialize resources");
    }
    return Result::ok();
}

// ❌ 不推荐 - 异常（可能破坏稳定性）
void on_load() override {
    if (!initializeResources()) {
        throw std::runtime_error("Failed");
    }
}
```

### 3. 资源管理

```cpp
// ✅ 推荐 - RAII
class MyPlugin : public IPlugin {
private:
    std::unique_ptr<Resource> m_resource;

public:
    void on_unload() override {
        // 自动释放
        m_resource.reset();
    }
};

// ❌ 不推荐 - 手动管理
class MyPlugin : public IPlugin {
public:
    void on_unload() override {
        delete m_resource;  // 容易忘记
    }
};
```

### 4. API 版本兼容性

```cpp
// ✅ 推荐 - 声明支持的 API 版本
PluginInfo get_info() const override {
    return PluginInfo{
        .api_version = "1.0.0"  // 明确版本
    };
}

// ✅ 推荐 - 版本比较
bool is_api_compatible(const std::string& current) const {
    // 支持语义版本比较
    return api_version == current || is_backward_compatible(current);
}
```

### 5. 日志记录

```cpp
// ✅ 推荐 - 使用统一的日志系统
Result<void, std::string> on_load() override {
    LOG_INFO("MyPlugin: Loading...");

    try {
        // 初始化逻辑
        LOG_INFO("MyPlugin: Loaded successfully");
        return Result::ok();
    } catch (const std::exception& e) {
        LOG_ERROR("MyPlugin: Failed to load - {}", e.what());
        return Result::err(e.what());
    }
}
```

## 插件系统 API 参考

### PluginManager

```cpp
class PluginManager {
public:
    // 单例访问
    static PluginManager& instance();

    // 添加插件
    Result<void, std::string> add_builtin(std::unique_ptr<IPlugin> plugin);
    Result<void, std::string> load_from_file(const std::filesystem::path& path);
    Result<size_t, std::string> load_from_directory(const std::filesystem::path& directory);

    // 卸载插件
    bool unload(const std::string& name);

    // 启用/禁用
    Result<void, std::string> enable(const std::string& name);
    Result<void, std::string> disable(const std::string& name);

    // 重载
    Result<void, std::string> reload(const std::string& name);

    // 查询
    IPlugin* get_plugin(const std::string& name);
    std::vector<PluginInfo> get_all_plugins_info() const;
    Result<PluginState, std::string> get_plugin_state(const std::string& name) const;

    // 清空
    void clear();
};
```

### IPlugin

```cpp
class IPlugin {
public:
    virtual ~IPlugin() = default;

    // 必须实现
    virtual PluginInfo get_info() const = 0;

    // 可选实现
    virtual Result<void, std::string> on_load();
    virtual void on_unload();
    virtual void on_enable();
    virtual void on_disable();
};
```

## 插件调试

### 查看已加载插件

```cpp
// 获取所有插件信息
auto plugins = PluginManager::instance().get_all_plugins_info();

ImGui::Begin("Plugins");
for (const auto& plugin : plugins) {
    ImGui::Text("Name: {}", plugin.name);
    ImGui::Text("Author: {}", plugin.author);
    ImGui::Text("Version: {}", plugin.version);
    ImGui::Text("API: {}", plugin.api_version);
    ImGui::Separator();
}
ImGui::End();
```

### 查看插件状态

```cpp
auto state = PluginManager::instance().get_plugin_state("MyPlugin");
if (state.isOk()) {
    auto s = state.unwrap();
    switch (s) {
        case PluginState::Unloaded:   /* ... */ break;
        case PluginState::Loaded:     /* ... */ break;
        case PluginState::Enabled:    /* ... */ break;
        case PluginState::Disabled:   /* ... */ break;
        case PluginState::Error:      /* ... */ break;
    }
}
```

## 插件开发检查清单

- [ ] 实现 `get_info()` 返回完整的 PluginInfo
- [ ] 实现 `on_load()` 返回 Result 类型
- [ ] 实现 `on_unload()` 清理资源
- [ ] 使用 RAII 管理资源生命周期
- [ ] 使用 `LOG_*` 宏记录日志
- [ ] 使用 Result 类型进行错误处理
- [ ] 注册命令、工具、视图时使用前缀避免冲突
- [ ] API 版本号与框架兼容
- [ ] 线程安全（如需要）

## 参考资源

- **源码**: `core/plugin/plugin.h`, `core/plugin/plugin.cpp`
- **模板**: `assets/plugin_template.cpp`
- **示例**: ImHex 插件 (`.demo/ImHex/plugins/`)
- **文档**: `dearts-dev/references/plugin_system.md`
