# DearTsd 新架构快速入门

## 已完成的改进 ✅

### 1. 移除了旧 API
- ❌ 删除 `core/event/event.h` 和 `event.cpp`
- ❌ 删除 `core/content/content_registry.h` 和 `content_registry.cpp`
- ✅ 更新 `core/ui/view.h` 引用新的 `registry_base.h`

### 2. 新架构组件（全部实现）

| 组件 | 头文件 | 实现文件 | 状态 |
|------|--------|----------|------|
| Result 类型 | `core/result.h` | - | ✅ |
| EventBus | `core/event/event_bus.h` | - | ✅ |
| Settings | `core/content/settings.h` | `core/content/settings.cpp` | ✅ |
| Commands | `core/content/commands.h` | `core/content/commands.cpp` | ✅ |
| Tools | `core/content/tools.h` | `core/content/tools.cpp` | ✅ |
| Callbacks | `core/content/callbacks.h` | `core/content/callbacks.cpp` | ✅ |
| Plugin | `core/plugin/plugin.h` | `core/plugin/plugin.cpp` | ✅ |
| ConfigManager | `core/config/config_manager.h` | `core/config/config_manager.cpp` | ✅ |
| CommandPalette | `core/ui/command_palette.h` | `core/ui/command_palette.cpp` | ✅ |

### 3. 修复的编译错误
- ✅ 添加 `#include <imgui.h>` 到 `command_palette.h`
- ✅ 修复 `ImGuiKey` 类型问题（使用 `int` 代替）
- ✅ 更新示例代码中的快捷键设置

## 编译项目

### 要求
- C++20 编译器（MSVC 2022+, GCC 11+, Clang 13+）
- CMake 3.20+
- SDL3 3.3.3（已包含在 `third_party/`）
- ImGui（已包含在 `third_party/`）

### 编译步骤

#### Windows (Visual Studio)
```bash
# 1. 创建构建目录
mkdir build
cd build

# 2. 配置 CMake
cmake .. -G "Visual Studio 17 2022" -A x64

# 3. 编译
cmake --build . --config Release

# 4. 运行
cd bin\Release
demo_imhex_style.exe
```

#### Linux/macOS
```bash
# 1. 创建构建目录
mkdir build
cd build

# 2. 配置 CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# 3. 编译
make -j$(nproc)

# 4. 运行
cd bin
./demo_imhex_style
```

## 快速示例

### 使用 Result 类型
```cpp
#include "core/result.h"

Result<int, std::string> divide(int a, int b) {
    if (b == 0) {
        return Result::err("Division by zero");
    }
    return Result::ok(a / b);
}

// 使用
auto result = divide(10, 2);
if (result.isOk()) {
    LOG_INFO("Result: {}", result.unwrap());
} else {
    LOG_ERROR("Error: {}", result.error());
}
```

### 使用事件系统
```cpp
#include "core/event/event_bus.h"

// 定义事件
struct MyEvent {
    int data;
    const char* message;
};

// 订阅事件
auto guard = make_event_guard<MyEvent>([](const MyEvent& e) {
    LOG_INFO("Received: {} - {}", e.data, e.message);
});

// 发布事件
EventBus::instance().publish(MyEvent{42, "Hello"});
```

### 使用命令系统
```cpp
#include "core/content/commands.h"

using namespace ContentRegistry::Commands;

// 注册命令
Commands::add("myapp.save", "Save File", []() {
    LOG_INFO("Saving file...");
});

// 执行命令
Commands::execute("myapp.save");

// 搜索命令
auto results = Commands::search("save");
```

### 使用配置管理
```cpp
#include "core/config/config_manager.h"

using namespace Config;

// 设置配置
ConfigManager::instance().set("app.window.width", 1280);
ConfigManager::instance().set("app.theme", "dark");

// 获取配置
auto width = ConfigManager::instance().get<int>("app.window.width");
auto theme = ConfigManager::instance().get_or("app.theme", std::string("light"));

// 使用作用域
ConfigScope scope("app.window");
scope.set("width", 1920);  // 实际键为 "app.window.width"
```

### 创建插件
```cpp
#include "core/plugin/plugin.h"

class MyPlugin : public Plugin::IPlugin {
public:
    PluginInfo get_info() const override {
        return {
            .name = "My Plugin",
            .author = "Your Name",
            .description = "My awesome plugin",
            .version = "1.0.0",
            .api_version = "1.0.0"
        };
    }

    Result<void, std::string> on_load() override {
        // 注册命令、工具等
        Commands::add("myplugin.action", "Do Something", []() {
            LOG_INFO("Action executed!");
        });
        return Result<void, std::string>::ok();
    }
};

// 注册为内置插件
DEARTS_BUILTIN_PLUGIN(MyPlugin)
```

### 使用命令面板
```cpp
#include "core/ui/command_palette.h"

auto palette = std::make_unique<UI::CommandPalette>();
palette->set_shortcut(static_cast<int>(ImGuiKey_P), true, false, false);  // Ctrl+P

// 在渲染循环中
if (palette->check_shortcut()) {
    palette->open();
}

palette->render();
```

## 项目结构

```
DearTsd/
├── core/
│   ├── result.h                 # Result 类型
│   ├── app/
│   │   ├── application.h
│   │   └── application.cpp
│   ├── event/
│   │   └── event_bus.h         # 新事件系统
│   ├── content/
│   │   ├── registry_base.h
│   │   ├── settings.h/.cpp    # 设置管理
│   │   ├── commands.h/.cpp    # 命令注册
│   │   ├── tools.h/.cpp       # 工具注册
│   │   └── callbacks.h/.cpp   # 生命周期回调
│   ├── plugin/
│   │   └── plugin.h/.cpp      # 插件系统
│   ├── config/
│   │   └── config_manager.h/.cpp  # 配置管理
│   └── ui/
│       ├── view.h/.cpp
│       └── command_palette.h/.cpp  # 命令面板
├── examples/
│   └── demo_imhex_style/
│       └── main.cpp            # 完整示例程序
└── docs/
    ├── quickstart.md           # 本文档
    ├── architecture_improvements.md
    └── imhex_style_refactoring_summary.md
```

## 下一步

1. **运行示例**: 编译并运行 `examples/demo_imhex_style/main.cpp`
2. **阅读文档**: 查看 `docs/architecture_improvements.md` 了解详细设计
3. **创建插件**: 尝试编写自己的插件
4. **自定义**: 根据需要扩展和自定义架构

## 常见问题

### Q: 为什么使用 C++20？
A: C++20 提供了概念、std::format、std::variant 等现代特性，大大简化了代码并提高了类型安全性。

### Q: 与旧版本兼容吗？
A: 不兼容。这是一个全新的架构，完全移除了旧的 API。

### Q: 如何调试？
A: 所有核心组件都有详细的日志输出，使用 `LOG_INFO`、`LOG_ERROR` 等宏进行调试。

### Q: 性能如何？
A: 所有抽象都是零成本的，编译器会优化掉不必要的开销。

## 获取帮助

- 查看文档: `docs/`
- 查看示例: `examples/demo_imhex_style/main.cpp`
- 查看测试: `tests/` (如果有)

---

**版本**: 1.0.0
**更新日期**: 2024-12-28
