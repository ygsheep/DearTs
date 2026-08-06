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

## 插件依赖管理系统

### 概述

DearTs Framework 提供完整的插件依赖管理功能，支持：
- 三种依赖类型（必需、可选、软依赖）
- 语义化版本范围约束（SemVer 2.0.0 + npm 风格）
- 自动拓扑排序确定加载顺序
- 循环依赖检测
- 版本冲突验证

### PluginDependency - 插件依赖声明

```cpp
struct PluginDependency {
    std::string plugin_name;      // 依赖的插件名称
    VersionRange version_range;   // 版本范围
    DependencyType type;          // 依赖类型

    // 工厂方法
    static Result<PluginDependency, std::string> required(
        std::string name,
        std::string version_range
    );

    static Result<PluginDependency, std::string> optional(
        std::string name,
        std::string version_range
    );

    static Result<PluginDependency, std::string> soft(
        std::string name,
        std::string version_range
    );
};
```

### 依赖类型

```cpp
enum class DependencyType {
    Required,   // 必需依赖 - 缺失或版本不匹配时加载失败
    Optional,   // 可选依赖 - 缺失时警告，继续加载
    Soft        // 软依赖 - 缺失时静默忽略
};
```

### 在插件中声明依赖

```cpp
class MyPlugin : public IPlugin {
public:
    // 新增：声明插件依赖
    std::vector<PluginDependency> get_dependencies() const override {
        return {
            // 必需依赖：CorePlugin >= 2.0.0
            PluginDependency::required("CorePlugin", ">=2.0.0").unwrap(),

            // 可选依赖：UIPlugin ^1.5.0
            PluginDependency::optional("UIPlugin", "^1.5.0").unwrap(),

            // 软依赖：AnalyticsPlugin ~1.2.0
            PluginDependency::soft("AnalyticsPlugin", "~1.2.0").unwrap()
        };
    }
};
```

### 版本范围语法（npm 风格）

| 语法 | 说明 | 示例 | 匹配版本 |
|------|------|------|----------|
| `1.2.3` | 精确版本 | `1.2.3` | `1.2.3` |
| `^1.2.3` | 兼容版本 | `^1.2.3` | `>=1.2.3 <2.0.0` |
| `~1.2.3` | 补丁级更新 | `~1.2.3` | `>=1.2.3 <1.3.0` |
| `1.2.*` | 通配符 | `1.2.*` | `>=1.2.0 <1.3.0` |
| `>=1.2.3` | 大于等于 | `>=1.2.3` | `1.2.3`, `1.3.0`, `2.0.0` |
| `>1.2.3` | 大于 | `>1.2.3` | `1.2.4`, `1.3.0`, `2.0.0` |
| `<=1.2.3` | 小于等于 | `<=1.2.3` | `1.2.3`, `1.2.2`, `1.0.0` |
| `<1.2.3` | 小于 | `<1.2.3` | `1.2.2`, `1.1.0`, `0.9.0` |
| `1.2.3 - 2.3.4` | 范围 | `1.2.3 - 2.3.4` | `>=1.2.3 <=2.3.4` |
| 复合 | 组合条件 | `>=1.0.0 <2.0.0` | `1.0.0` - `1.999.999` |

---

## PluginManager 依赖管理 API

### 1. 设置依赖解析模式

```cpp
void set_dependency_mode(DependencyResolutionMode mode)
```

**参数**:
- `mode` - 依赖解析模式

**模式说明**:
```cpp
enum class DependencyResolutionMode {
    Lenient,    // 宽松模式 - 跳过缺失的可选依赖，继续加载
    Strict      // 严格模式 - 遇到依赖错误立即停止
};
```

**示例**:
```cpp
// 设置严格模式
PluginManager::instance().set_dependency_mode(
    DependencyResolutionMode::Strict
);
```

---

### 2. 获取依赖解析模式

```cpp
DependencyResolutionMode get_dependency_mode() const
```

**返回值**: 当前的依赖解析模式

---

### 3. 获取最后一次依赖解析结果

```cpp
DependencyResolutionResult get_last_resolution_result() const
```

**返回值**: 依赖解析结果详情

**结果结构**:
```cpp
struct DependencyResolutionResult {
    bool success;                           // 是否成功
    std::vector<PluginLoadOrder> orders;    // 加载顺序
    std::vector<std::string> errors;        // 错误列表
    std::vector<std::string> warnings;      // 警告列表
};
```

**示例**:
```cpp
auto result = PluginManager::instance().get_last_resolution_result();

if (!result.success) {
    for (const auto& error : result.errors) {
        LOG_ERROR("Dependency error: {}", error);
    }
}

for (const auto& warning : result.warnings) {
    LOG_WARN("Dependency warning: {}", warning);
}
```

---

### 4. 使用依赖关系加载所有插件

```cpp
Result<void, std::string> load_all_with_dependencies()
```

**说明**: 自动解析依赖关系并按正确顺序加载所有已添加的插件

**示例**:
```cpp
// 1. 添加所有插件
PluginManager::instance().add_builtin(std::make_unique<CorePlugin>());
PluginManager::instance().add_builtin(std::make_unique<UIPlugin>());
PluginManager::instance().add_builtin(std::make_unique<MyPlugin>());

// 2. 解析依赖并按正确顺序加载
auto result = PluginManager::instance().load_all_with_dependencies();

if (result.isErr()) {
    LOG_ERROR("Failed to load plugins: {}", result.error());
} else {
    LOG_INFO("All plugins loaded successfully");
}
```

---

### 5. 初始化依赖配置

```cpp
void initialize_dependency_config()
```

**说明**: 初始化依赖配置系统（通常在应用启动时调用一次）

---

### 6. 检查插件是否为内置插件

```cpp
bool is_plugin_builtin(const std::string& name) const
```

**参数**:
- `name` - 插件名称

**返回值**:
- 内置插件: `true`
- 动态加载插件: `false`

**示例**:
```cpp
bool is_builtin = PluginManager::instance().is_plugin_builtin("MyPlugin");
if (is_builtin) {
    LOG_INFO("This is a builtin plugin");
}
```

---

## 版本控制系统

### Version 类

语义化版本 2.0.0 实现：

```cpp
class Version {
public:
    uint32_t major = 0;      // 主版本号
    uint32_t minor = 0;      // 次版本号
    uint32_t patch = 0;      // 修订号
    std::string prerelease;  // 预发布标识（如 "alpha", "beta.1"）
    std::string build;       // 构建元数据

    // 解析版本字符串
    static Result<Version, std::string> parse(const std::string& version_str);

    // 版本比较（C++20 spaceship operator）
    auto operator<=>(const Version& other) const = default;

    // 转换为字符串
    std::string to_string() const;
};
```

**示例**:
```cpp
// 解析版本
auto v1 = Version::parse("1.2.3");
auto v2 = Version::parse("2.0.0-alpha.1+build.123");

if (v1.isOk()) {
    Version version = v1.unwrap();
    LOG_INFO("Version: {}.{}.{}", version.major, version.minor, version.patch);
}

// 版本比较
if (Version::parse("1.2.3") < Version::parse("1.2.4")) {
    LOG_INFO("1.2.3 is older");
}
```

### VersionRange 类

版本范围规范实现：

```cpp
class VersionRange {
public:
    // 解析版本范围
    static Result<VersionRange, std::string> parse(const std::string& range_str);

    // 检查版本是否在范围内
    bool satisfies(const Version& version) const;

    // 转换为字符串
    std::string to_string() const;
};
```

**示例**:
```cpp
// 解析版本范围
auto range = VersionRange::parse("^1.2.3");
if (range.isOk()) {
    VersionRange vr = range.unwrap();

    // 检查版本是否满足
    Version v1 = Version::parse("1.2.5").unwrap();
    Version v2 = Version::parse("2.0.0").unwrap();

    LOG_INFO("1.2.5 satisfies: {}", vr.satisfies(v1)); // true
    LOG_INFO("2.0.0 satisfies: {}", vr.satisfies(v2)); // false
}
```

---

## 动态库加载系统

### 跨平台抽象

```cpp
class DynamicLibraryLoader {
public:
    virtual ~DynamicLibraryLoader() = default;

    virtual Result<void, std::string> load(const std::filesystem::path& path) = 0;
    virtual Result<void*, std::string> get_symbol(const char* name) = 0;
    virtual void unload() = 0;

    static std::unique_ptr<DynamicLibraryLoader> create();
};
```

### 平台特定实现

**Windows**:
```cpp
class WindowsLibraryLoader : public DynamicLibraryLoader {
    HMODULE m_handle = nullptr;

    Result<void, std::string> load(const std::filesystem::path& path) override {
        m_handle = LoadLibraryW(path.c_str());
        if (!m_handle) {
            return Result::err("Failed to load library");
        }
        return Result::ok();
    }

    Result<void*, std::string> get_symbol(const char* name) override {
        void* symbol = (void*)GetProcAddress(m_handle, name);
        if (!symbol) {
            return Result::err("Symbol not found");
        }
        return Result::ok(symbol);
    }

    void unload() override {
        if (m_handle) {
            FreeLibrary(m_handle);
            m_handle = nullptr;
        }
    }
};
```

**Linux/macOS**:
```cpp
class UnixLibraryLoader : public DynamicLibraryLoader {
    void* m_handle = nullptr;

    Result<void, std::string> load(const std::filesystem::path& path) override {
        m_handle = dlopen(path.c_str(), RTLD_LAZY);
        if (!m_handle) {
            return Result::err(std::string(dlerror()));
        }
        return Result::ok();
    }

    Result<void*, std::string> get_symbol(const char* name) override {
        void* symbol = dlsym(m_handle, name);
        if (!symbol) {
            return Result::err(std::string(dlerror()));
        }
        return Result::ok(symbol);
    }

    void unload() override {
        if (m_handle) {
            dlclose(m_handle);
            m_handle = nullptr;
        }
    }
};
```

---

## 依赖管理示例

### 示例 1：带依赖的插件

```cpp
class AdvancedAnalyticsPlugin : public IPlugin {
public:
    PluginInfo get_info() const override {
        return PluginInfo{
            .name = "AdvancedAnalytics",
            .author = "Data Team",
            .description = "Advanced analytics with charting",
            .version = "1.0.0",
            .api_version = "1.0.0"
        };
    }

    // 声明依赖
    std::vector<PluginDependency> get_dependencies() const override {
        return {
            // 必需：基础数据插件
            PluginDependency::required("BaseData", ">=2.0.0").unwrap(),

            // 可选：图表插件（用于可视化）
            PluginDependency::optional("ChartPlugin", "^1.5.0").unwrap(),

            // 软依赖：日志插件（用于调试）
            PluginDependency::soft("DebugLogger", ">=1.0.0").unwrap()
        };
    }

    Result<void, std::string> on_load() override {
        LOG_INFO("AdvancedAnalytics: Loading...");

        // 检查可选依赖是否可用
        auto* chart_plugin = PluginManager::instance().get_plugin("ChartPlugin");
        if (chart_plugin) {
            LOG_INFO("Chart plugin available - enabling visualization");
            m_hasCharting = true;
        } else {
            LOG_WARN("Chart plugin not available - visualization disabled");
        }

        // 注册命令
        ContentRegistry::Commands::register_handler(
            "analytics.analyze",
            "Analyze Data",
            [this]() { performAnalysis(); }
        );

        return Result::ok();
    }

private:
    bool m_hasCharting = false;
};
```

### 示例 2：严格模式依赖解析

```cpp
void load_plugins_strict() {
    auto& pm = PluginManager::instance();

    // 设置严格模式
    pm.set_dependency_mode(DependencyResolutionMode::Strict);

    // 添加插件
    pm.add_builtin(std::make_unique<BaseDataPlugin>());
    pm.add_builtin(std::make_unique<ChartPlugin>());
    pm.add_builtin(std::make_unique<AdvancedAnalyticsPlugin>());

    // 解析依赖并加载
    auto result = pm.load_all_with_dependencies();

    if (result.isErr()) {
        LOG_ERROR("Failed to load plugins: {}", result.error());

        // 检查详细错误
        auto resolution = pm.get_last_resolution_result();
        for (const auto& error : resolution.errors) {
            LOG_ERROR("  - {}", error);
        }
    } else {
        LOG_INFO("All plugins loaded successfully");

        // 检查警告
        auto resolution = pm.get_last_resolution_result();
        for (const auto& warning : resolution.warnings) {
            LOG_WARN("  - {}", warning);
        }
    }
}
```

### 示例 3：依赖图可视化

```cpp
// 获取依赖图（调试用）
auto graph = DependencyResolver::visualize_dependency_graph(plugins);
LOG_INFO("Dependency Graph:\n{}", graph);
```

输出示例：
```
Dependency Graph:
BaseData (2.0.0)
  ├─ No dependencies
ChartPlugin (1.5.2)
  ├─ BaseData (>=2.0.0) ✓
AdvancedAnalytics (1.0.0)
  ├─ BaseData (>=2.0.0) ✓
  ├─ ChartPlugin (^1.5.0) ✓ (optional)
  └─ DebugLogger (>=1.0.0) ✗ (soft)

Load Order:
1. BaseData
2. ChartPlugin
3. AdvancedAnalytics
```

---

## API 快速参考（更新）

### PluginManager 方法

| 方法 | 说明 | 新增 |
|------|------|------|
| `instance()` | 获取单例 | |
| `add_builtin(plugin)` | 添加内置插件 | |
| `load_from_file(path)` | 从文件加载动态插件 | ✅ |
| `load_from_directory(dir)` | 从目录加载所有插件 | |
| `unload(name)` | 卸载插件 | |
| `enable(name)` | 启用插件 | |
| `disable(name)` | 禁用插件 | |
| `reload(name)` | 重载插件 | |
| `get_plugin(name)` | 获取插件指针 | |
| `get_all_plugins_info()` | 获取所有插件信息 | |
| `get_plugin_state(name)` | 获取插件状态 | |
| `is_plugin_builtin(name)` | 检查是否为内置插件 | ✅ |
| `clear()` | 清空所有插件 | |
| `set_dependency_mode(mode)` | 设置依赖解析模式 | ✅ |
| `get_dependency_mode()` | 获取依赖解析模式 | ✅ |
| `get_last_resolution_result()` | 获取依赖解析结果 | ✅ |
| `load_all_with_dependencies()` | 使用依赖关系加载所有插件 | ✅ |
| `initialize_dependency_config()` | 初始化依赖配置 | ✅ |

### IPlugin 方法

| 方法 | 说明 | 新增 |
|------|------|------|
| `get_info()` | 获取插件信息（必须实现） | |
| `get_dependencies()` | 获取插件依赖列表 | ✅ |
| `on_load()` | 插件加载时调用 | |
| `on_unload()` | 插件卸载时调用 | |
| `on_enable()` | 插件启用时调用 | |
| `on_disable()` | 插件禁用时调用 | |

### 新增类型

| 类型 | 说明 | 头文件 |
|------|------|--------|
| `PluginDependency` | 插件依赖声明 | `plugin_dependency.h` |
| `DependencyType` | 依赖类型枚举 | `plugin_dependency.h` |
| `DependencyResolutionMode` | 依赖解析模式 | `dependency_resolver.h` |
| `DependencyResolutionResult` | 依赖解析结果 | `dependency_resolver.h` |
| `Version` | 语义化版本 | `version.h` |
| `VersionRange` | 版本范围 | `version_range.h` |
| `DynamicLibraryLoader` | 动态库加载器 | `plugin_loader.h` |

---

## 更新日志

### Version 2.0.0 (当前)
- ✅ 添加插件依赖管理系统
- ✅ 添加语义化版本控制（SemVer 2.0.0）
- ✅ 添加 npm 风格版本范围支持
- ✅ 添加跨平台动态库加载
- ✅ 添加依赖解析和拓扑排序
- ✅ 添加循环依赖检测
- ✅ 添加版本冲突验证

### Version 1.0.0
- ✅ 基础插件系统
- ✅ 生命周期管理
- ✅ API 版本检查
- ✅ 内置插件支持

---

**文件**: `core/plugin/plugin.h`
**源码**: `core/plugin/plugin.cpp`
**新增**: `plugin_dependency.h/cpp`, `dependency_resolver.h/cpp`, `version.h/cpp`, `version_range.h/cpp`, `plugin_loader.h/cpp`
**相关**: Result 类型, Logger, EventBus, Content Registry, TaskManager, ConfigManager
**插件指南**: `docs/plugin_system_guide.md`
**快速开始**: `plugins/QUICKSTART.md`
