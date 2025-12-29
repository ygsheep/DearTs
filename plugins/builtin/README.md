# DearTs UI 插件开发指南

## 插件目录结构

```
DearTsd/
├── core/                          # 框架核心代码
├── lib/                           # 编译好的库（如 liblogger）
├── plugins/                       # ⭐ 插件目录（推荐位置）
│   └── builtin/                   # 内置插件
│       ├── include/               # 插件头文件
│       │   └── views/             # 视图定义
│       │       ├── hello_world_view.hpp
│       │       └── data_inspector_view.hpp
│       ├── source/                # 插件源码
│       │   └── builtin_plugin.cpp
│       ├── CMakeLists.txt
│       └── README.md
└── main/gui/                      # 应用入口
```

### ⚠️ 为什么插件放在 `plugins/` 而不是 `lib/`？

| 目录 | 用途 | 示例 |
|------|------|------|
| `lib/` | 编译好的**静态库**或**第三方库** | liblogger, SDL3, ImGui |
| `plugins/` | **动态插件**（可加载/卸载） | BuiltinPlugin, 第三方插件 |

---

## UI 插件示例

### 示例 1：Hello World 视图

```cpp
// plugins/builtin/include/views/hello_world_view.hpp

#pragma once

#include "core/ui/view.h"

namespace DearTs::Plugins::Builtin {

class HelloWorldView : public Core::UI::ViewWindow {
public:
    explicit HelloWorldView()
        : ViewWindow("Hello World") {
    }

    void draw_content() override {
        ImGui::Text("Hello from plugin!");

        if (ImGui::Button("点击我")) {
            m_click_count++;
        }

        ImGui::Text("点击次数: %d", m_click_count);
    }

private:
    int m_click_count = 0;
};

} // namespace DearTs::Plugins::Builtin
```

### 示例 2：数据检查器视图（实用工具）

```cpp
// plugins/builtin/include/views/data_inspector_view.hpp

#pragma once

#include "core/ui/view.h"
#include <vector>

namespace DearTs::Plugins::Builtin {

class DataInspectorView : public Core::UI::ViewWindow {
public:
    explicit DataInspectorView()
        : ViewWindow("Data Inspector") {
    }

    void draw_content() override {
        // 顶部工具栏
        if (ImGui::Button("刷新")) {
            refresh_data();
        }

        ImGui::SameLine();
        if (ImGui::Button("清除")) {
            m_data_input.clear();
        }

        ImGui::Separator();

        // 数据输入
        char buffer[256];
        if (ImGui::InputText("输入数据 (十六进制)", buffer, sizeof(buffer),
                             ImGuiInputTextFlags_CharsHexadecimal)) {
            m_data_input = buffer;
            parse_data();
        }

        ImGui::Separator();

        // 结果显示
        if (ImGui::BeginTable("Data", 2)) {
            ImGui::TableSetupColumn("偏移");
            ImGui::TableSetupColumn("值");
            ImGui::TableHeadersRow();

            for (const auto& item : m_parsed_data) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("0x%zX", item.offset);
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s", item.value.c_str());
            }

            ImGui::EndTable();
        }
    }

private:
    void parse_data() {
        // 解析十六进制数据...
    }

    void refresh_data() {
        parse_data();
    }

private:
    std::string m_data_input;
    struct { size_t offset; std::string value; };
    std::vector</* ... */> m_parsed_data;
};

} // namespace DearTs::Plugins::Builtin
```

---

## 插件主文件

```cpp
// plugins/builtin/source/builtin_plugin.cpp

#include "views/hello_world_view.hpp"
#include "views/data_inspector_view.hpp"
#include "core/plugin/plugin.h"
#include "core/content/commands.h"
#include "core/ui/title_bar.h"
#include "liblogger/logger.h"

namespace DearTs::Plugins::Builtin {

class BuiltinPlugin : public Core::Plugin::IPlugin {
public:
    // 1. 获取插件信息（必须实现）
    Core::Plugin::PluginInfo get_info() const override {
        return Core::Plugin::PluginInfo{
            .name = "Builtin",
            .author = "DearTs Team",
            .description = "DearTs 内置插件",
            .version = "1.0.0",
            .api_version = "1.0.0"
        };
    }

    // 2. 插件加载时调用
    Core::Result<void, std::string> on_load() override {
        LOG_INFO("BuiltinPlugin: Loading...");

        // 注册视图
        ContentRegistry::Views::add<HelloWorldView>();
        ContentRegistry::Views::add<DataInspectorView>();

        // 注册命令
        ContentRegistry::Commands::register_handler(
            "builtin.file.save",
            "Save",
            []() {
                LOG_INFO("Saving...");
            },
            nullptr,
            "Ctrl+S"
        );

        // 注册标题栏按钮
        TitleBar::instance().add_button(
            ICON_FA_SAVE,
            "Save",
            []() {
                ContentRegistry::Commands::invoke("builtin.file.save");
            }
        );

        LOG_INFO("BuiltinPlugin: Loaded");
        return Core::Result<void, std::string>::ok();
    }

    // 3. 插件卸载时调用
    void on_unload() override {
        LOG_INFO("BuiltinPlugin: Unloaded");
    }

    // 4. 插件启用时调用
    void on_enable() override {
        LOG_INFO("BuiltinPlugin: Enabled");
    }

    // 5. 插件禁用时调用
    void on_disable() override {
        LOG_INFO("BuiltinPlugin: Disabled");
    }
};

} // namespace DearTs::Plugins::Builtin

// 导出函数（用于动态加载）
extern "C" {

__declspec(dllexport) DearTs::Core::Plugin::IPlugin* dearts_create_plugin() {
    return new DearTs::Plugins::Builtin::BuiltinPlugin();
}

__declspec(dllexport) void dearts_destroy_plugin(DearTs::Core::Plugin::IPlugin* plugin) {
    delete plugin;
}

__declspec(dllexport) const char* dearts_get_api_version() {
    return "1.0.0";
}

}
```

---

## 在应用中使用插件

### 方法 1：内置插件（编译时集成）

```cpp
// main/gui/main.cpp

#include "plugins/builtin/source/builtin_plugin.cpp"  // 直接包含

int main() {
    auto& app = ToolboxApplication::instance();

    // 初始化应用
    if (!app.init()) {
        return -1;
    }

    // 注册内置插件
    auto result = DearTs::Core::Plugin::PluginManager::instance()
        .add_builtin(std::make_unique<DearTs::Plugins::Builtin::BuiltinPlugin>());

    if (result.isErr()) {
        LOG_ERROR("Failed to load builtin plugin: {}", result.error());
    }

    // 运行应用
    app.run();

    return 0;
}
```

### 方法 2：动态插件（运行时加载）

```cpp
// main/gui/main.cpp

int main() {
    auto& app = ToolboxApplication::instance();

    // 初始化应用
    if (!app.init()) {
        return -1;
    }

    // 从文件加载插件
    auto result = DearTs::Core::Plugin::PluginManager::instance()
        .load_from_file("plugins/builtin.dll");

    if (result.isErr()) {
        LOG_ERROR("Failed to load plugin: {}", result.error());
    }

    // 或从目录加载所有插件
    auto count = DearTs::Core::Plugin::PluginManager::instance()
        .load_from_directory("plugins/");

    if (count.isOk()) {
        LOG_INFO("Loaded {} plugins", count.unwrap());
    }

    // 运行应用
    app.run();

    return 0;
}
```

---

## CMake 配置

### 顶层 CMakeLists.txt

```cmake
# DearTsd/CMakeLists.txt

cmake_minimum_required(VERSION 3.20)
project(DearTs VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)

# 添加核心库
add_subdirectory(core)
add_subdirectory(lib/liblogger)

# 添加插件
add_subdirectory(plugins/builtin)

# 主程序
add_subdirectory(main/gui)
```

### 插件 CMakeLists.txt

```cmake
# plugins/builtin/CMakeLists.txt

cmake_minimum_required(VERSION 3.20)
project(BuiltinPlugin VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)

# 插件源文件
set(PLUGIN_SOURCES
    source/builtin_plugin.cpp
)

# 创建动态库
add_library(${PROJECT_NAME} SHARED ${PLUGIN_SOURCES})

# 包含目录
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/core
        ${CMAKE_SOURCE_DIR}/third_party/imgui
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 链接库
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ImGui::ImGui
        SDL3::SDL3
)

# Windows 导出设置
if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE BUILDING_DLL)
endif()

# 输出到 plugins 目录
set_target_properties(${PROJECT_NAME} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
)
```

---

## 插件开发清单

### 创建新插件步骤

1. ✅ **创建插件目录**
   ```bash
   mkdir -p plugins/myplugin/{include,source}
   ```

2. ✅ **实现视图类**
   ```cpp
   // 继承 ViewWindow 或 View
   class MyView : public Core::UI::ViewWindow {
       void draw_content() override {
           // 绘制 UI
       }
   };
   ```

3. ✅ **实现插件类**
   ```cpp
   class MyPlugin : public Core::Plugin::IPlugin {
       PluginInfo get_info() const override;
       Result<void, std::string> on_load() override;
   };
   ```

4. ✅ **注册功能**
   - 视图: `ContentRegistry::Views::add<MyView>()`
   - 命令: `ContentRegistry::Commands::register_handler(...)`
   - 按钮: `TitleBar::instance().add_button(...)`

5. ✅ **导出插件**
   ```cpp
   extern "C" {
       IPlugin* dearts_create_plugin();
       void dearts_destroy_plugin(IPlugin*);
   }
   ```

6. ✅ **配置 CMake**
   - 添加插件到构建系统
   - 配置输出目录

7. ✅ **测试插件**
   - 加载插件
   - 验证视图显示
   - 测试命令和按钮

---

## 插件功能扩展

### 注册工具窗口

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

### 注册设置项

```cpp
ContentRegistry::Settings::add(
    "myplugin.auto_save",
    "Auto Save",
    true
);

ContentRegistry::Settings::add(
    "myplugin.interval",
    "Interval (seconds)",
    60
);
```

### 订阅事件

```cpp
EventBus::Token m_eventToken;

Result<void, std::string> on_load() override {
    m_eventToken = EventBus::instance().subscribe<DataModifiedEvent>(
        [](const DataModifiedEvent& e) {
            LOG_INFO("Data modified: {} bytes", e.size);
        }
    );

    return Result::ok();
}
```

---

## 插件最佳实践

### ✅ DO

1. **使用命名空间** - 避免符号冲突
   ```cpp
   namespace DearTs::Plugins::MyPlugin {
       class MyView { };
   }
   ```

2. **使用 Result 类型** - 类型安全的错误处理
   ```cpp
   Result<void, std::string> on_load() override {
       if (!init()) {
           return Result::err("Initialization failed");
       }
       return Result::ok();
   }
   ```

3. **使用 RAII** - 自动资源管理
   ```cpp
   class MyPlugin {
       EventBus::Token m_token;  // 自动取消订阅
   };
   ```

4. **记录日志** - 便于调试
   ```cpp
   LOG_INFO("MyPlugin: Loading...");
   LOG_ERROR("MyPlugin: Failed - {}", error);
   ```

### ❌ DON'T

1. **不要在插件中使用全局状态**
   ```cpp
   // ❌ 不好
   static std::vector<int> g_data;

   // ✅ 好
   class MyPlugin {
       std::vector<int> m_data;
   };
   ```

2. **不要在析构函数中抛出异常**
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

3. **不要忘记检查 API 版本**
   ```cpp
   PluginInfo get_info() const override {
       return PluginInfo{
           .api_version = "1.0.0"  // 明确版本
       };
   }
   ```

---

## 调试插件

### 查看已加载插件

```cpp
auto plugins = PluginManager::instance().get_all_plugins_info();

ImGui::Begin("Plugins");
for (const auto& plugin : plugins) {
    ImGui::Text("Name: %s", plugin.name.c_str());
    ImGui::Text("Version: %s", plugin.version.c_str());
    ImGui::Separator();
}
ImGui::End();
```

### 查看插件状态

```cpp
auto state = PluginManager::instance().get_plugin_state("Builtin");
if (state.isOk()) {
    auto s = state.unwrap();
    switch (s) {
        case PluginState::Enabled:
            LOG_INFO("Plugin is enabled");
            break;
        case PluginState::Error:
            LOG_ERROR("Plugin has errors");
            break;
    }
}
```

---

## 参考资源

- **插件系统文档**: `docs/plugin_system_guide.md`
- **ImHex 插件**: `.demo/ImHex/plugins/builtin/`
- **View 基类**: `core/ui/view.h`
- **Plugin 接口**: `core/plugin/plugin.h`
- **Content Registry**: `core/content/registry_base.h`

---

现在您可以在 `plugins/builtin/` 目录下创建自己的 UI 插件了！🚀
