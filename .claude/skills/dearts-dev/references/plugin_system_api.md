# Plugin System API 完全手册

## 概述

DearTs Framework 的插件系统提供了强大的扩展机制，支持动态加载、生命周期管理、API 版本检查等功能。

**核心特性：**
- ✅ **API 版本检查** - 确保插件与框架兼容
- ✅ **生命周期管理** - 加载、卸载、启用、禁用
- ✅ **类型安全** - 使用 Result 类型进行错误处理
- ✅ **单例模式** - PluginManager 全局管理
- ✅ **内置插件支持** - 编译时集成插件
- ✅ **动态加载** - 支持从动态库加载（预留）
- ✅ **事件驱动** - 与 Content Registry、EventBus 无缝集成

---

## 快速开始

### 创建第一个插件

```cpp
#include "core/plugin/plugin.h"
#include "core/content/commands.h"
#include "liblogger/logger.h"

// 1. 定义插件类
class MyPlugin : public DearTs::Core::Plugin::IPlugin {
public:
    // 必须实现：获取插件信息
    PluginInfo get_info() const override {
        return PluginInfo{
            .name = "MyPlugin",
            .author = "Your Name",
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

        LOG_INFO("MyPlugin: Loaded successfully");
        return Result::ok();
    }

    // 可选实现：插件卸载时调用
    void on_unload() override {
        LOG_INFO("MyPlugin: Unloading...");
        // 清理资源（框架会自动取消订阅等）
    }

    // 可选实现：插件启用时调用
    void on_enable() override {
        LOG_INFO("MyPlugin: Enabled");
    }

    // 可选实现：插件禁用时调用
    void on_disable() override {
        LOG_INFO("MyPlugin: Disabled");
    }
};

// 2. 注册插件（在应用初始化时）
auto result = PluginManager::instance().add_builtin(
    std::make_unique<MyPlugin>()
);

if (result.isErr()) {
    LOG_ERROR("Failed to load plugin: {}", result.error());
}
```

---

## 核心概念

### 插件生命周期

```
未加载 (Unloaded)
    ↓ 加载 (load)
已加载 (Loaded)
    ↓ 启用 (enable)
已启用 (Enabled)
    ↓ 禁用 (disable)
已加载 (Loaded)
    ↓ 卸载 (unload)
未加载 (Unloaded)
```

### 插件状态

```cpp
enum class PluginState {
    Unloaded,   // 未加载
    Loaded,     // 已加载（但未启用）
    Enabled,    // 已启用
    Disabled,   // 已禁用
    Error       // 错误状态
};
```

---

## IPlugin - 插件接口

所有插件必须继承此类并实现虚函数。

### 必须实现的方法

```cpp
virtual PluginInfo get_info() const = 0;
```

**返回值**: `PluginInfo` 结构，包含插件元数据

**示例**:
```cpp
PluginInfo get_info() const override {
    return PluginInfo{
        .name = "MyPlugin",
        .author = "DearTs Team",
        .description = "Plugin description",
        .version = "1.0.0",
        .api_version = "1.0.0"
    };
}
```

### 可选实现的方法

#### 1. on_load - 插件加载

```cpp
virtual Result<void, std::string> on_load()
```

**调用时机**: 插件被添加到 PluginManager 后立即调用

**用途**:
- 注册命令、视图、工具
- 订阅事件
- 初始化资源

**示例**:
```cpp
Result<void, std::string> on_load() override {
    LOG_INFO("Loading...");

    // 注册命令
    ContentRegistry::Commands::register_handler("my.action", ...);

    // 注册视图
    ContentRegistry::Views::add<MyView>();

    // 订阅事件
    m_eventToken = EventBus::instance().subscribe<Event>(handler);

    LOG_INFO("Loaded successfully");
    return Result::ok();
}
```

#### 2. on_unload - 插件卸载

```cpp
virtual void on_unload()
```

**调用时机**: 插件从 PluginManager 移除时调用

**用途**:
- 清理资源
- 保存数据

**注意**:
- 事件订阅会自动取消（RAII）
- 已注册的命令/视图会自动移除

**示例**:
```cpp
void on_unload() override {
    LOG_INFO("Unloading...");
    // 清理资源（大部分工作由框架自动完成）
}
```

#### 3. on_enable - 插件启用

```cpp
virtual void on_enable()
```

**调用时机**: 插件被启用时调用

**用途**:
- 启用功能
- 恢复状态

#### 4. on_disable - 插件禁用

```cpp
virtual void on_disable()
```

**调用时机**: 插件被禁用时调用

**用途**:
- 暂停功能
- 保存状态

---

## PluginManager - 插件管理器

单例模式，全局管理所有插件。

### 1. 获取单例

```cpp
static PluginManager& PluginManager::instance()
```

**示例**:
```cpp
auto& pm = PluginManager::instance();
```

---

### 2. 添加内置插件

```cpp
Result<void, std::string> add_builtin(std::unique_ptr<IPlugin> plugin)
```

**参数**:
- `plugin` - 插件实例（unique_ptr）

**返回值**:
- 成功: `Result::ok()`
- 失败: `Result::err("错误信息")`

**示例**:
```cpp
auto result = PluginManager::instance().add_builtin(
    std::make_unique<MyPlugin>()
);

if (result.isErr()) {
    LOG_ERROR("Failed: {}", result.error());
}
```

**注意**:
- 插件名称必须唯一
- 会自动调用 `on_load()` 和 `on_enable()`

---

### 3. 从文件加载动态插件

```cpp
Result<void, std::string> load_from_file(const std::filesystem::path& path)
```

**参数**:
- `path` - 插件动态库路径（如 `plugins/myplugin.dll`）

**返回值**:
- 成功: `Result::ok()`
- 失败: `Result::err("错误信息")`

**示例**:
```cpp
auto result = PluginManager::instance().load_from_file(
    "plugins/myplugin.dll"
);

if (result.isOk()) {
    LOG_INFO("Plugin loaded successfully");
}
```

**注意**: 当前版本尚未实现动态加载

---

### 4. 从目录加载所有插件

```cpp
Result<size_t, std::string> load_from_directory(const std::filesystem::path& directory)
```

**参数**:
- `directory` - 插件目录路径

**返回值**:
- 成功: `Result<size_t, std::string>::ok(加载的插件数量)`
- 失败: `Result::err("错误信息")`

**示例**:
```cpp
auto result = PluginManager::instance().load_from_directory("plugins/");

if (result.isOk()) {
    size_t count = result.unwrap();
    LOG_INFO("Loaded {} plugins", count);
}
```

---

### 5. 卸载插件

```cpp
bool unload(const std::string& name)
```

**参数**:
- `name` - 插件名称

**返回值**:
- 成功: `true`
- 失败: `false`

**示例**:
```cpp
bool success = PluginManager::instance().unload("MyPlugin");
if (success) {
    LOG_INFO("Plugin unloaded");
}
```

---

### 6. 启用插件

```cpp
Result<void, std::string> enable(const std::string& name)
```

**参数**:
- `name` - 插件名称

**返回值**:
- 成功: `Result::ok()`
- 失败: `Result::err("错误信息")`

**示例**:
```cpp
auto result = PluginManager::instance().enable("MyPlugin");
if (result.isErr()) {
    LOG_ERROR("Failed to enable: {}", result.error());
}
```

---

### 7. 禁用插件

```cpp
Result<void, std::string> disable(const std::string& name)
```

**参数**:
- `name` - 插件名称

**返回值**:
- 成功: `Result::ok()`
- 失败: `Result::err("错误信息")`

**示例**:
```cpp
auto result = PluginManager::instance().disable("MyPlugin");
```

---

### 8. 重载插件

```cpp
Result<void, std::string> reload(const std::string& name)
```

**说明**: 重新加载插件（卸载后重新加载）

**注意**: 当前版本尚未实现

---

### 9. 获取插件指针

```cpp
IPlugin* get_plugin(const std::string& name)
```

**参数**:
- `name` - 插件名称

**返回值**:
- 成功: 插件指针
- 失败: `nullptr`

**示例**:
```cpp
auto* plugin = PluginManager::instance().get_plugin("MyPlugin");
if (plugin) {
    auto info = plugin->get_info();
    LOG_INFO("Plugin: {} v{}", info.name, info.version);
}
```

---

### 10. 获取所有插件信息

```cpp
std::vector<PluginInfo> get_all_plugins_info() const
```

**返回值**: 所有插件的 PluginInfo 列表

**示例**:
```cpp
auto plugins = PluginManager::instance().get_all_plugins_info();

for (const auto& plugin : plugins) {
    LOG_INFO("Plugin: {} by {} (v{})",
        plugin.name, plugin.author, plugin.version);
}
```

---

### 11. 获取插件状态

```cpp
Result<PluginState, std::string> get_plugin_state(const std::string& name) const
```

**参数**:
- `name` - 插件名称

**返回值**:
- 成功: `Result<PluginState>::ok(state)`
- 失败: `Result::err("错误信息")`

**示例**:
```cpp
auto result = PluginManager::instance().get_plugin_state("MyPlugin");
if (result.isOk()) {
    auto state = result.unwrap();
    switch (state) {
        case PluginState::Enabled:
            LOG_INFO("Plugin is enabled");
            break;
        case PluginState::Disabled:
            LOG_INFO("Plugin is disabled");
            break;
        case PluginState::Error:
            LOG_INFO("Plugin has errors");
            break;
        default:
            break;
    }
}
```

---

### 12. 清空所有插件

```cpp
void clear()
```

**示例**:
```cpp
PluginManager::instance().clear();
```

---

## 实际应用示例

### 示例 1：Hello World 插件

```cpp
class HelloWorldPlugin : public IPlugin {
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
        ContentRegistry::Commands::register_handler(
            "helloworld.say",
            "Say Hello",
            []() {
                LOG_INFO("Hello from plugin!");
                ImGui::OpenPopup("Hello Popup");
            }
        );

        // 注册视图
        ContentRegistry::Views::add<HelloWorldView>();

        LOG_INFO("HelloWorldPlugin: Loaded");
        return Result::ok();
    }

    void on_unload() override {
        LOG_INFO("HelloWorldPlugin: Unloaded");
    }
};

// 注册插件
PluginManager::instance().add_builtin(
    std::make_unique<HelloWorldPlugin>()
);
```

---

### 示例 2：数据处理插件

```cpp
class DataProcessorPlugin : public IPlugin {
private:
    EventBus::Token m_dataEventToken;

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
        LOG_INFO("DataProcessorPlugin: Loading...");

        // 注册命令
        ContentRegistry::Commands::register_handler(
            "dataprocessor.analyze",
            "Analyze Data",
            [this]() { analyzeData(); }
        );

        ContentRegistry::Commands::register_handler(
            "dataprocessor.export",
            "Export Data",
            [this]() { exportData(); }
        );

        // 订阅事件
        m_dataEventToken = EventBus::instance().subscribe<DataModifiedEvent>(
            [this](const DataModifiedEvent& e) {
                onDataModified(e);
            }
        );

        // 注册视图
        ContentRegistry::Views::add<DataAnalysisView>();

        // 注册工具
        ContentRegistry::Tools::add(
            "Data Processor",
            []() {
                ImGui::Begin("Data Processor");
                // 工具内容
                ImGui::End();
            }
        );

        // 注册设置
        ContentRegistry::Settings::add(
            "dataprocessor.auto_analyze",
            "Auto Analyze",
            true
        );

        LOG_INFO("DataProcessorPlugin: Loaded");
        return Result::ok();
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
        bool auto_analyze = ContentRegistry::Settings::read(
            "dataprocessor.auto_analyze", true
        );

        if (auto_analyze) {
            analyzeData();
        }
    }
};
```

---

### 示例 3：带标题栏按钮的插件

```cpp
class ToolbarPlugin : public IPlugin {
public:
    PluginInfo get_info() const override {
        return PluginInfo{
            .name = "ToolbarButtons",
            .author = "DearTs Team",
            .description = "Adds custom toolbar buttons",
            .version = "1.0.0",
            .api_version = "1.0.0"
        };
    }

    Result<void, std::string> on_load() override {
        LOG_INFO("ToolbarPlugin: Loading...");

        // 添加标题栏按钮
        TitleBar::instance().add_button(
            ICON_FA_SAVE,
            "Save (Ctrl+S)",
            []() {
                LOG_INFO("Save button clicked");
                ContentRegistry::Commands::invoke("builtin.file.save");
            },
            ImVec4(0.2f, 0.6f, 1.0f, 1.0f)
        );

        TitleBar::instance().add_button(
            ICON_FA_FOLDER_OPEN,
            "Open File (Ctrl+O)",
            []() {
                LOG_INFO("Open button clicked");
                ContentRegistry::Commands::invoke("builtin.file.open");
            },
            ImVec4(1.0f, 0.8f, 0.2f, 1.0f)
        );

        LOG_INFO("ToolbarPlugin: Loaded");
        return Result::ok();
    }
};
```

---

### 示例 4：插件管理视图

```cpp
class PluginManagerView : public View {
public:
    std::string getName() const override {
        return "Plugin Manager";
    }

    void draw_content() override {
        ImGui::Text("插件管理器");
        ImGui::Separator();

        // 获取所有插件
        auto plugins = PluginManager::instance().get_all_plugins_info();

        if (plugins.empty()) {
            ImGui::TextDisabled("无已加载插件");
            return;
        }

        // 插件列表
        for (const auto& plugin : plugins) {
            draw_plugin_info(plugin);
            ImGui::Separator();
        }

        // 操作按钮
        if (ImGui::Button("刷新")) {
            // 刷新插件列表
        }

        ImGui::SameLine();
        if (ImGui::Button("加载插件...")) {
            // 打开文件选择对话框
        }
    }

private:
    void draw_plugin_info(const PluginInfo& plugin) {
        // 插件名称和版本
        ImGui::Text("%s v%s by %s",
            plugin.name.c_str(),
            plugin.version.c_str(),
            plugin.author.c_str());

        // 描述
        ImGui::TextDisabled("%s", plugin.description.c_str());

        // API 版本
        ImGui::Text("API: %s", plugin.api_version.c_str());

        // 状态
        auto state_result = PluginManager::instance().get_plugin_state(plugin.name);
        if (state_result.isOk()) {
            auto state = state_result.unwrap();
            const char* state_text = "";
            ImColor color;

            switch (state) {
                case PluginState::Unloaded:
                    state_text = "未加载";
                    color = ImColor(128, 128, 128);
                    break;
                case PluginState::Loaded:
                    state_text = "已加载";
                    color = ImColor(255, 255, 0);
                    break;
                case PluginState::Enabled:
                    state_text = "已启用";
                    color = ImColor(0, 255, 0);
                    break;
                case PluginState::Disabled:
                    state_text = "已禁用";
                    color = ImColor(255, 128, 0);
                    break;
                case PluginState::Error:
                    state_text = "错误";
                    color = ImColor(255, 0, 0);
                    break;
            }

            ImGui::SameLine();
            ImGui::TextColored(color, "[%s]", state_text);

            // 操作按钮
            if (state == PluginState::Enabled) {
                if (ImGui::Button(std::format("禁用##{}", plugin.name).c_str())) {
                    PluginManager::instance().disable(plugin.name);
                }
            } else if (state == PluginState::Disabled) {
                if (ImGui::Button(std::format("启用##{}", plugin.name).c_str())) {
                    PluginManager::instance().enable(plugin.name);
                }
            }

            if (ImGui::Button(std::format("卸载##{}", plugin.name).c_str())) {
                PluginManager::instance().unload(plugin.name);
            }
        }
    }
};
```

---

## 插件功能集成

### 1. 注册命令

```cpp
ContentRegistry::Commands::register_handler(
    "myplugin.action",           // 命令 ID
    "My Action",                 // 显示名称
    []() {                       // 回调函数
        LOG_INFO("Action executed!");
    },
    nullptr,                     // 可用性查询（可选）
    "Ctrl+Shift+M"               // 快捷键（可选）
);
```

### 2. 注册视图

```cpp
class MyView : public View {
public:
    std::string getName() const override {
        return "My View";
    }

    void draw_content() override {
        ImGui::Text("Hello from my view!");
    }
};

// 在插件中注册
ContentRegistry::Views::add<MyView>();
```

### 3. 注册工具

```cpp
ContentRegistry::Tools::add(
    "My Tool",
    []() {
        if (ImGui::Begin("My Tool")) {
            ImGui::Text("Tool content");
        }
        ImGui::End();
    }
);
```

### 4. 订阅事件

```cpp
class MyPlugin : public IPlugin {
private:
    EventBus::Token m_eventToken;

public:
    Result<void, std::string> on_load() override {
        // 订阅事件
        m_eventToken = EventBus::instance().subscribe<DataModifiedEvent>(
            [](const DataModifiedEvent& e) {
                LOG_INFO("Data modified: {} bytes", e.size);
            }
        );

        return Result::ok();
    }

    // Token 析构时会自动取消订阅
};
```

### 5. 注册设置

```cpp
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
```

### 6. 添加标题栏按钮

```cpp
TitleBar::instance().add_button(
    ICON_FA_SAVE,                   // 图标
    "Save (Ctrl+S)",                // 工具提示
    []() {                          // 点击回调
        save_file();
    },
    ImVec4(0.2f, 0.6f, 1.0f, 1.0f)  // 颜色
);
```

---

## 最佳实践

### ✅ DO

1. **使用 Result 类型处理错误**
   ```cpp
   Result<void, std::string> on_load() override {
       if (!initResources()) {
           return Result::err("Failed to initialize");
       }
       return Result::ok();
   }
   ```

2. **使用 RAII 管理资源**
   ```cpp
   class MyPlugin : public IPlugin {
   private:
       EventBus::Token m_token;  // 自动取消订阅

   public:
       Result<void, std::string> on_load() override {
           m_token = EventBus::instance().subscribe<Event>(handler);
           return Result::ok();
       }
   };
   ```

3. **记录日志**
   ```cpp
   Result<void, std::string> on_load() override {
       LOG_INFO("MyPlugin: Loading...");
       // ...
       LOG_INFO("MyPlugin: Loaded");
       return Result::ok();
   }
   ```

4. **使用命名空间**
   ```cpp
   namespace DearTs::Plugins::MyPlugin {
       class MyPlugin : public IPlugin { };
   }
   ```

5. **API 版本兼容性**
   ```cpp
   PluginInfo get_info() const override {
       return PluginInfo{
           .api_version = "1.0.0"  // 明确版本
       };
   }
   ```

### ❌ DON'T

1. **不要在析构函数中抛出异常**
   ```cpp
   // ❌ 不好
   ~MyPlugin() {
       throw std::runtime_error("Error");
   }

   // ✅ 好
   ~MyPlugin() {
       LOG_ERROR("Cleanup failed");
   }
   ```

2. **不要使用全局状态**
   ```cpp
   // ❌ 不好
   static std::vector<int> g_data;

   // ✅ 好
   class MyPlugin : public IPlugin {
       std::vector<int> m_data;
   };
   ```

3. **不要忘记检查 API 版本**
   ```cpp
   // ❌ 不好
   PluginInfo{ .api_version = "" }

   // ✅ 好
   PluginInfo{ .api_version = "1.0.0" }
   ```

---

## 插件开发检查清单

开发插件时，确保完成以下步骤：

- [ ] 实现 `get_info()` 返回完整的 PluginInfo
- [ ] 实现 `on_load()` 返回 Result 类型
- [ ] 实现 `on_unload()` 清理资源
- [ ] 使用 RAII 管理资源生命周期
- [ ] 使用 `LOG_*` 宏记录日志
- [ ] 使用 Result 类型进行错误处理
- [ ] 注册命令、工具、视图时使用前缀避免冲突
- [ ] API 版本号与框架兼容
- [ ] 线程安全（如需要）

---

## API 快速参考

### PluginManager 方法

| 方法 | 说明 |
|------|------|
| `instance()` | 获取单例 |
| `add_builtin(plugin)` | 添加内置插件 |
| `load_from_file(path)` | 从文件加载动态插件 |
| `load_from_directory(dir)` | 从目录加载所有插件 |
| `unload(name)` | 卸载插件 |
| `enable(name)` | 启用插件 |
| `disable(name)` | 禁用插件 |
| `reload(name)` | 重载插件 |
| `get_plugin(name)` | 获取插件指针 |
| `get_all_plugins_info()` | 获取所有插件信息 |
| `get_plugin_state(name)` | 获取插件状态 |
| `clear()` | 清空所有插件 |

### IPlugin 方法

| 方法 | 说明 |
|------|------|
| `get_info()` | 获取插件信息（必须实现） |
| `on_load()` | 插件加载时调用 |
| `on_unload()` | 插件卸载时调用 |
| `on_enable()` | 插件启用时调用 |
| `on_disable()` | 插件禁用时调用 |

---

**文件**: `core/plugin/plugin.h`
**源码**: `core/plugin/plugin.cpp`
**相关**: Result 类型, Logger, EventBus, Content Registry, TaskManager, ConfigManager
**插件指南**: `docs/plugin_system_guide.md`
**快速开始**: `plugins/QUICKSTART.md`
