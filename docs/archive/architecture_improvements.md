# DearTsd 架构改进文档

## 概述

本文档详细说明了对 DearTsd core 架构的改进，这些改进参考了 ImHex 的优秀设计理念，结合现代 C++20 特性和 SDL3/ImGui 最新 API，使框架更加灵活、可扩展、类型安全。

## 改进内容

### 1. Result 类型 - 统一错误处理 ⭐⭐⭐

**文件**: `core/result.h`

**解决的问题**:
- 原有架构中错误处理不统一（返回值、异常、静默失败）
- 缺乏类型安全的错误传播
- 错误信息丢失严重

**新特性**:
```cpp
// 使用示例
Result<int, std::string> divide(int a, int b) {
    if (b == 0) {
        return Result::err("Division by zero");
    }
    return Result::ok(a / b);
}

// 链式调用
auto result = divide(10, 2)
    .map([](int value) { return value * 2; })
    .and_then([](int value) { return save_to_file(value); });
```

**优势**:
- ✅ 编译时类型安全
- ✅ 强制错误处理
- ✅ 支持函数式编程（map, and_then）
- ✅ 无异常开销
- ✅ 提供 void 特化版本

### 2. 类型安全的事件系统 ⭐⭐⭐

**文件**: `core/event/event_bus.h`

**解决的问题**:
- 原有事件系统使用枚举手动分配类型 ID，容易冲突
- 缺乏编译时类型检查
- 回调函数类型不安全

**新特性**:
```cpp
// 定义事件
struct WindowCloseEvent {
    int window_id;
    const char* title;
};

struct DataChangedEvent {
    size_t offset;
    size_t size;
    std::vector<uint8_t> data;
};

// 订阅事件（类型安全）
auto token = EventBus::instance().subscribe<WindowCloseEvent>(
    [](const WindowCloseEvent& e) {
        LOG_INFO("Window {} ({}) closed", e.window_id, e.title);
    }
);

// 发布事件
EventBus::instance().publish(WindowCloseEvent{
    .window_id = 42,
    .title = "Main Window"
});

// RAII 自动管理
auto guard = make_event_guard<DataChangedEvent>([](const DataChangedEvent& e) {
    // 自动取消订阅
});
```

**优势**:
- ✅ 编译时类型检查
- ✅ 无需手动管理类型 ID
- ✅ 支持任意事件类型（POD、复杂结构体）
- ✅ RAII 自动管理订阅生命周期
- ✅ 线程安全（递归互斥锁）
- ✅ 支持异步事件队列

### 3. 模块化的 Content Registry ⭐⭐⭐

**文件**:
- `core/content/registry_base.h` - 基础定义
- `core/content/settings.h` - 设置管理
- `core/content/commands.h` - 命令注册
- `core/content/tools.h` - 工具注册
- `core/content/callbacks.h` - 生命周期回调

**解决的问题**:
- ContentRegistry 承担过多职责（违反单一职责原则）
- 模块间耦合度高
- 难以独立测试和维护

**新架构**:
```cpp
// 设置管理（独立模块）
using namespace ContentRegistry::Settings;

Settings::add("general", "theme", "dark");
Settings::add("general", "language", "en");

auto theme = Settings::get("general", "theme");
if (theme.isOk()) {
    LOG_INFO("Current theme: {}", theme.unwrap());
}

// 命令注册（独立模块）
using namespace ContentRegistry::Commands;

Commands::add("file.open", "Open File", []() {
    // 打开文件逻辑
}).shortcut = "Ctrl+O";

Commands::add("file.save", "Save File", []() {
    // 保存文件逻辑
}).enabled_callback = []() {
    return has_unsaved_changes();
};

// 工具注册（独立模块）
using namespace ContentRegistry::Tools;

Tools::add("calculator", "Simple Calculator", []() {
    show_calculator_window();
});

// 生命周期回调
using namespace ContentRegistry::Callbacks;

Callbacks::add_on_init([]() {
    LOG_INFO("Application initialized");
});

Callbacks::add_on_update([](double delta_time) {
    update_physics(delta_time);
});
```

**优势**:
- ✅ 清晰的职责分离
- ✅ 每个模块可独立使用
- ✅ 更好的命名空间组织
- ✅ 易于扩展和测试
- ✅ 支持 Result 类型错误处理

### 4. 插件系统 ⭐⭐⭐

**文件**: `core/plugin/plugin.h`

**解决的问题**:
- 所有功能编译时静态链接
- 无法动态扩展功能
- 第三方扩展困难

**新特性**:
```cpp
// 定义插件
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
        // 注册命令
        Commands::add("myplugin.action", "Do Something", []() {
            // 执行操作
        });

        // 注册视图
        // 注册工具
        // 等等...

        return Result<void, std::string>::ok();
    }

    void on_unload() override {
        // 清理资源
    }
};

// 内置插件
DEARTS_BUILTIN_PLUGIN(MyPlugin)

// 加载内置插件
PluginManager::instance().add_builtin(std::make_unique<MyPlugin>());

// 从文件加载插件
PluginManager::instance().load_from_file("plugins/myplugin.dll");
PluginManager::instance().load_from_directory("plugins");
```

**优势**:
- ✅ 标准化的插件接口
- ✅ 支持内置插件和动态库插件
- ✅ 插件生命周期管理（load/unload/enable/disable）
- ✅ API 版本兼容性检查
- ✅ 统一的错误处理

### 5. 配置管理器 ⭐⭐

**文件**: `core/config/config_manager.h`

**解决的问题**:
- 配置分散在各个模块
- 缺乏统一的配置管理接口
- 不支持配置验证和变更通知

**新特性**:
```cpp
// 设置配置
ConfigManager::instance().set("app.window.width", 1280);
ConfigManager::instance().set("app.window.height", 720);
ConfigManager::instance().set("app.theme", "dark");

// 获取配置
auto width = ConfigManager::instance().get<int>("app.window.width")
    .unwrap_or(1280);

auto theme = ConfigManager::instance().get_or("app.theme", std::string("light"));

// 注册配置元数据
ConfigManager::instance().register_meta("app.window.width", {
    .description = "Main window width",
    .default_value = 1280,
    .is_required = false,
    .validate_callback = [](const ConfigValue& value) {
        if (std::holds_alternative<int>(value)) {
            int width = std::get<int>(value);
            if (width < 640 || width > 7680) {
                return Result<void, std::string>::err(
                    "Window width must be between 640 and 7680"
                );
            }
        }
        return Result<void, std::string>::ok();
    },
    .change_callback = [](const ConfigValue& value) {
        if (std::holds_alternative<int>(value)) {
            resize_window(std::get<int>(value));
        }
    }
});

// 作用域管理
{
    ConfigScope scope("app.window");
    scope.set("width", 1280);  // 实际键为 "app.window.width"
    auto height = scope.get<int>("height");
}

// 持久化
ConfigManager::instance().save_to_file("config.json");
ConfigManager::instance().load_from_file("config.json");
```

**优势**:
- ✅ 统一的配置管理接口
- ✅ 支持分层配置（点号分隔）
- ✅ 类型安全
- ✅ 配置验证
- ✅ 变更通知
- ✅ 持久化支持
- ✅ 作用域管理（RAII）

### 6. 命令面板 ⭐⭐

**文件**: `core/ui/command_palette.h`

**解决的问题**:
- 缺乏统一的命令执行界面
- 快捷键冲突和分散管理
- 命令发现困难

**新特性**:
```cpp
// 创建命令面板
auto palette = std::make_unique<CommandPalette>();
palette->set_shortcut(ImGuiKey_P, true, false, false);  // Ctrl+P

// 注册命令
Commands::add("file.open", "Open File", []() {
    // 打开文件
}).enabled_callback = []() { return true; };

Commands::add("file.save", "Save File", []() {
    // 保存文件
}).enabled_callback = []() { return has_unsaved_changes(); };

Commands::add("view.toggle_fullscreen", "Toggle Fullscreen", []() {
    // 切换全屏
});

// 在渲染循环中
if (palette->check_shortcut()) {
    palette->open();
}

palette->render();
```

**优势**:
- ✅ 类似 VS Code 的命令面板体验
- ✅ 支持搜索和过滤
- ✅ 快捷键高亮
- ✅ 动态启用/禁用
- ✅ 可自定义样式

## 迁移指南

### 从旧事件系统迁移

**旧代码**:
```cpp
enum EventType {
    WindowClose = 1
};

class WindowCloseEvent : public Event {
    size_t get_type_id() const override {
        return WindowClose;
    }
};

auto sub = subscribe_event<WindowCloseEvent>([](const WindowCloseEvent& e) {
    // 处理事件
});
```

**新代码**:
```cpp
struct WindowCloseEvent {
    int window_id;
};

auto guard = make_event_guard<WindowCloseEvent>([](const WindowCloseEvent& e) {
    // 处理事件
});
```

### 从旧 ContentRegistry 迁移

**旧代码**:
```cpp
ContentRegistry::Settings::add("general", "theme", "dark");
```

**新代码**:
```cpp
using namespace ContentRegistry::Settings;
Settings::add("general", "theme", "dark");
// 或者
auto& setting = Settings::add("general", "theme", "dark");
setting.validate_callback = [](const std::string& value) {
    return value == "dark" || value == "light";
};
```

## 最佳实践

### 1. 错误处理

```cpp
// 推荐：使用 Result 类型
Result<void, std::string> do_something() {
    TRY_RESULT(load_config());
    TRY_RESULT(init_resources());
    return Result<void, std::string>::ok();
}

// 不推荐：使用异常
void do_something() {
    try {
        load_config();
        init_resources();
    } catch (const std::exception& e) {
        // 错误处理
    }
}
```

### 2. 事件定义

```cpp
// 推荐：使用简单结构体
struct MyEvent {
    int id;
    std::string message;
    double timestamp;
};

// 不推荐：复杂的继承层次
class MyEvent : public Event {
    // 复杂的实现
};
```

### 3. 配置管理

```cpp
// 推荐：使用作用域
void init_window() {
    ConfigScope scope("app.window");
    auto width = scope.get_or<int>("width", 1280);
    auto height = scope.get_or<int>("height", 720);
    // ...
}

// 不推荐：重复前缀
void init_window() {
    auto width = ConfigManager::instance().get_or("app.window.width", 1280);
    auto height = ConfigManager::instance().get_or("app.window.height", 720);
    // ...
}
```

## 性能考虑

1. **Result 类型**: 零成本抽象，编译器优化后与直接返回值性能相同
2. **事件系统**: 使用 type_index 作为键，查找性能为 O(log n)
3. **插件系统**: 动态加载仅在启动时，运行时无性能损失
4. **配置管理**: 使用 unordered_map，查找性能为 O(1)

## 兼容性

- **C++ 标准**: C++20
- **编译器**:
  - GCC 11+
  - Clang 13+
  - MSVC 2022+
- **平台**:
  - Windows 10+
  - macOS 11+
  - Linux (主流发行版)

## 未来计划

- [ ] 添加配置文件热重载
- [ ] 实现插件沙箱
- [ ] 添加插件依赖管理
- [ ] 支持远程配置同步
- [ ] 实现事件性能分析工具

## 参考资料

- [ImHex Architecture](https://github.com/WerWolv/ImHex)
- [SDL3 Documentation](https://wiki.libsdl.org/SDL3/)
- [ImGui Documentation](https://github.com/ocornut/imgui)
