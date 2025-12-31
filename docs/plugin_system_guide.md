# DearTs Framework - 插件开发指南

<div align="center">

**从零开始创建你的第一个 DearTs 插件**

[前置要求](#前置要求) • [快速示例](#快速示例) • [核心概念](#核心概念) • [进阶主题](#进阶主题)

</div>

---

## 📋 前置要求

### 知识储备

- ✅ C++20 基础（概念、auto、lambda）
- ✅ 面向对象编程（类、继承、虚函数）
- ✅ 智能指针和 RAII
- ✅ Git 基础操作

### 开发环境

- **编译器**: MSVC 2022+ / GCC 11+ / Clang 13+
- **CMake**: 3.20+
- **IDE**: Visual Studio 2022 / VS Code / CLion

---

## 🚀 快速示例

### 最小化插件（5 分钟）

#### 1. 创建插件类

```cpp
// my_plugin.hpp
#pragma once
#include "core/plugin/plugin.h"

class MyPlugin : public IPlugin {
public:
    // 插件信息
    PluginInfo get_info() const override {
        return PluginInfo{
            .name = "MyPlugin",
            .author = "Your Name",
            .description = "我的第一个插件",
            .version = "1.0.0",
            .api_version = "1.0.0"
        };
    }

    // 插件加载时调用
    Result<void, std::string> on_load() override {
        LOG_INFO("MyPlugin loaded!");
        return Result::ok();
    }

    // 插件卸载时调用
    void on_unload() override {
        LOG_INFO("MyPlugin unloaded!");
    }
};
```

#### 2. 注册插件

```cpp
// main.cpp 或在 builtin plugin 中
#include "my_plugin.hpp"

int main() {
    // ...

    // 注册内置插件
    PluginManager::instance().add_builtin(
        std::make_unique<MyPlugin>()
    );

    // ...
}
```

#### 3. 编译运行

```bash
cmake --build build --config Release
./build/bin/DearTsApp.exe
```

输出：
```
[INFO] MyPlugin loaded!
```

---

## 📖 核心概念

### 1. 插件生命周期

```
Unloaded (未加载)
    ↓ add_builtin() / load_from_file()
Loaded (已加载)
    ↓ enable()
Enabled (已启用)
    ↓ disable()
Loaded (已加载)
    ↓ unload()
Unloaded (未加载)
```

**生命周期方法**：

| 方法 | 调用时机 | 用途 |
|------|----------|------|
| `get_info()` | 加载时 | 返回插件信息 |
| `on_load()` | 加载时 | 注册命令、视图等 |
| `on_unload()` | 卸载时 | 清理资源 |
| `on_enable()` | 启用时 | 激活插件功能 |
| `on_disable()` | 禁用时 | 停用插件功能 |

### 2. Content Registry

Content Registry 是 DearTs 的核心，提供统一的接口注册：

#### 注册命令

```cpp
Result<void, std::string> on_load() override {
    ContentRegistry::Commands::register_handler(
        "myplugin.hello",      // 命令 ID
        "Say Hello",           // 显示名称
        []() {                 // 回调函数
            LOG_INFO("Hello from MyPlugin!");
        },
        nullptr,               // 启用回调（可选）
        "Ctrl+Shift+H"         // 快捷键（可选）
    );

    return Result::ok();
}
```

#### 注册视图

```cpp
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

// 在 on_load() 中注册
ContentRegistry::Views::add<MyView>();
```

#### 注册工具

```cpp
ContentRegistry::Tools::add(
    "My Tool",              // 工具名称
    []() {                  // 回调函数
        LOG_INFO("Tool activated");
    }
);
```

#### 注册设置

```cpp
ContentRegistry::Settings::add(
    "myplugin.enabled",     // 设置键
    "Enable MyPlugin",      // 显示名称
    true                    // 默认值
);
```

### 3. 事件系统

#### 定义事件

```cpp
struct MyCustomEvent {
    int value;
    std::string message;
};
```

#### 订阅事件

```cpp
class MyPlugin : public IPlugin {
private:
    EventBus::Token m_eventToken;  // RAII 自动管理

public:
    Result<void, std::string> on_load() override {
        // 订阅事件
        m_eventToken = EventBus::instance().subscribe<MyCustomEvent>(
            [](const MyCustomEvent& e) {
                LOG_INFO("Received: {} - {}", e.value, e.message);
            }
        );

        return Result::ok();
    }
};
```

#### 发布事件

```cpp
EventBus::instance().publish(MyCustomEvent{
    42,
    "Hello from event!"
});
```

### 4. 配置管理

```cpp
class MyPlugin : public IPlugin {
private:
    ConfigScope m_config{"myplugin"};  // 自动添加 "myplugin." 前缀

public:
    Result<void, std::string> on_load() override {
        // 读取配置
        bool enabled = m_config.get_or<bool>("enabled", true);
        int interval = m_config.get_or<int>("interval", 60);

        LOG_INFO("Plugin enabled: {}, interval: {}", enabled, interval);

        // 写入配置
        m_config.set("last_run", std::time(nullptr));

        // 保存到文件
        ConfigManager::instance().save_to_file("config.json");

        return Result::ok();
    }
};
```

**配置文件 (config.json)**:
```json
{
    "myplugin": {
        "enabled": true,
        "interval": 60,
        "last_run": 1704067200
    }
}
```

---

## 🔧 进阶主题

### 1. 插件间通信

```cpp
// 插件 A 发布事件
EventBus::instance().publish(DataModifiedEvent{});

// 插件 B 订阅事件
m_eventToken = EventBus::instance().subscribe<DataModifiedEvent>(
    [](const DataModifiedEvent& e) {
        // 处理数据修改
    }
);
```

### 2. 异步任务

```cpp
#include "core/tasks/task_manager.h"

Result<void, std::string> on_load() override {
    auto task = TaskManager::instance().launch(
        "My Background Task",
        [](const auto& cancel) {
            for (int i = 0; i < 100 && !cancel.is_cancelled(); i++) {
                task.update_progress(i, 100);
                // 执行工作...
            }
            return Result::ok();
        }
    );

    return Result::ok();
}
```

### 3. 视图高级特性

```cpp
class MyView : public View {
public:
    std::string getName() const override {
        return "My Advanced View";
    }

    void draw_content() override {
        // 菜单栏
        if (ImGui::BeginMenuBar()) {
            if (ImGui::MenuItem("Refresh")) {
                refresh_data();
            }
            ImGui::EndMenuBar();
        }

        // 状态栏
        ImGui::Text("Status: %s", m_status.c_str());

        // 可调整大小的分割
        ImGui::Separator();

        // 自定义绘制
        draw_custom_content();
    }

private:
    void refresh_data() {
        LOG_INFO("Refreshing data...");
    }

    void draw_custom_content() {
        // ...
    }

    std::string m_status = "Ready";
};
```

### 4. 插件依赖

```cpp
PluginInfo get_info() const override {
    return PluginInfo{
        .name = "MyPlugin",
        // ...
        .dependencies = {"RequiredPlugin1", "RequiredPlugin2"}
    };
}
```

---

## 🧪 测试插件

### 单元测试

```cpp
// tests/unit/plugins/my_plugin_test.cpp
#include <gtest/gtest.h>
#include "my_plugin.hpp"

TEST(MyPluginTest, GetInfo) {
    MyPlugin plugin;
    auto info = plugin.get_info();

    EXPECT_EQ(info.name, "MyPlugin");
    EXPECT_EQ(info.version, "1.0.0");
}

TEST(MyPluginTest, OnLoad) {
    MyPlugin plugin;
    auto result = plugin.on_load();

    EXPECT_TRUE(result.isOk());
}
```

### 手动测试

1. 编译插件
2. 运行应用
3. 检查日志输出
4. 验证功能正常

---

## 📚 最佳实践

### ✅ DO

1. **使用 Result<T, E> 处理错误**
   ```cpp
   Result<void, std::string> on_load() override {
       if (!init()) {
           return Result::err("Initialization failed");
       }
       return Result::ok();
   }
   ```

2. **使用 RAII 管理资源**
   ```cpp
   class MyPlugin : public IPlugin {
   private:
       EventBus::Token m_eventToken;  // 自动取消订阅
       ConfigScope m_config{"myplugin"};  // 自动前缀
   };
   ```

3. **使用前缀避免命名冲突**
   ```cpp
   // 命令
   "myplugin.action"

   // 设置
   "myplugin.setting_name"
   ```

4. **记录重要事件**
   ```cpp
   LOG_INFO("Plugin loading...");
   LOG_ERROR("Failed to load: {}", error);
   ```

### ❌ DON'T

1. **不要使用异常处理**
   ```cpp
   // ❌ 错误
   void on_load() override {
       throw std::runtime_error("Failed");
   }

   // ✅ 正确
   Result<void, std::string> on_load() override {
       return Result::err("Failed");
   }
   ```

2. **不要忘记 API 版本**
   ```cpp
   PluginInfo{
       // ...
       .api_version = "1.0.0"  // 必须匹配框架
   }
   ```

3. **不要使用全局状态**
   ```cpp
   // ❌ 错误
   static int g_counter;

   // ✅ 正确
   class MyPlugin : public IPlugin {
   private:
       int m_counter;
   };
   ```

4. **不要在 on_load() 中执行耗时操作**
   ```cpp
   // ❌ 错误
   Result<void, std::string> on_load() override {
       // 阻塞加载
       load_huge_file();
       return Result::ok();
   }

   // ✅ 正确
   Result<void, std::string> on_load() override {
       // 异步加载
       TaskManager::instance().launch("Load File", []() {
           load_huge_file();
       });
       return Result::ok();
   }
   ```

---

## 🎯 完整示例

### 完整的插件示例

```cpp
#pragma once
#include "core/plugin/plugin.h"
#include "core/events/event_bus.h"
#include "core/content/commands.h"
#include "core/content/views.h"
#include "core/config/config_manager.h"

class HelloWorldPlugin : public IPlugin {
private:
    EventBus::Token m_eventToken;
    ConfigScope m_config{"helloworld"};

public:
    PluginInfo get_info() const override {
        return PluginInfo{
            .name = "HelloWorld",
            .author = "Your Name",
            .description = "A simple hello world plugin",
            .version = "1.0.0",
            .api_version = "1.0.0"
        };
    }

    Result<void, std::string> on_load() override {
        // 读取配置
        bool show_message = m_config.get_or<bool>("show_message", true);

        if (show_message) {
            LOG_INFO("Hello, World!");
        }

        // 注册命令
        ContentRegistry::Commands::register_handler(
            "helloworld.say",
            "Say Hello",
            []() {
                LOG_INFO("Hello from command!");
            },
            nullptr,
            "Ctrl+Shift+H"
        );

        // 订阅事件
        m_eventToken = EventBus::instance().subscribe<ApplicationReadyEvent>(
            [](const ApplicationReadyEvent&) {
                LOG_INFO("Application is ready!");
            }
        );

        return Result::ok();
    }

    void on_unload() override {
        LOG_INFO("Goodbye, World!");
        // m_eventToken 自动取消订阅（RAII）
    }
};
```

---

## 🔗 相关资源

### API 文档

- **[插件系统 API](../dearts-dev/references/plugin_system_api.md)** - 完整的 Plugin API 参考
- **[ConfigManager API](../dearts-dev/references/config_manager_api.md)** - 配置管理 API
- **[EventBus API](../dearts-dev/references/event_system.md)** - 事件系统 API

### 示例代码

- **[内置插件](../plugins/builtin/)** - BuiltinPlugin 示例
- **[Toast 通知插件](../plugins/toast_notification/)** - 实用插件示例
- **[命令面板插件](../plugins/command_palette/)** - 复杂插件示例

### 社区

- **[GitHub Issues](https://github.com/ygsheep/DearTs/issues)** - 报告问题
- **[GitHub Discussions](https://github.com/ygsheep/DearTs/discussions)** - 社区讨论

---

<div align="center">

**开始创建你的插件吧！** 🚀

[⬆ 返回顶部](#dearts-framework---插件开发指南)

</div>
