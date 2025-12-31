# DearTs Framework - 文档中心

<div align="center">

**用户文档和开发指南**

[🚀 快速开始](#快速开始) • [🔌 插件系统](#插件系统) • [📖 架构设计](#架构设计) • [📝 代码规范](#代码规范)

</div>

---

## 📚 文档导航

### 🚀 快速开始

| 文档 | 描述 | 适用对象 |
|------|------|----------|
| **[快速开始指南](quickstart.md)** | 5分钟上手 DearTs Framework | 新手用户 |
| **[构建说明](#)** | 详细的构建和编译说明 | 所有开发者 |

### 🔌 插件系统

| 文档 | 描述 | 适用对象 |
|------|------|----------|
| **[插件系统指南](plugin_system_guide.md)** | 完整的插件开发教程 | 插件开发者 |
| **[Content Registry](#)** | 命令、视图、工具注册 | 插件开发者 |
| **[示例插件](#)** | 示例插件代码 | 学习者 |

### 📖 架构设计

| 文档 | 描述 | 适用对象 |
|------|------|----------|
| **[系统架构](#)** | 核心系统架构设计 | 架构师 |
| **[事件系统](#)** | EventBus 事件总线 | 开发者 |
| **[配置管理](#)** | ConfigManager 配置系统 | 开发者 |

### 📝 代码规范

| 文档 | 描述 | 适用对象 |
|------|------|----------|
| **[代码规范](代码规范.md)** | C++20 编码规范 | 所有贡献者 |
| **[命名约定](#)** | 命名风格指南 | 所有贡献者 |
| **[最佳实践](#)** | 推荐的编程实践 | 所有开发者 |

### 🔧 开发者文档

**完整的开发者文档在 [dearts-dev/](../dearts-dev/) 目录：**

- **[API 参考手册](../dearts-dev/references/)** - 详细的 API 文档
- **[开发技能](../dearts-dev/SKILL.md)** - Claude Code 技能包
- **[示例代码](../dearts-dev/examples/)** - 示例和教程
- **[工具脚本](../dearts-dev/scripts/)** - 开发辅助工具

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

# 2. 配置项目
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 3. 构建
cmake --build build --config Release

# 4. 运行
./build/bin/DearTsApp.exe
```

### 运行测试

```bash
# 运行所有测试
ctest --test-dir build --verbose

# 或运行单个测试套件
./build/bin/dearts_unit_tests        # 单元测试
./build/bin/dearts_integration_tests # 集成测试
./build/bin/dearts_ui_tests          # UI 自动化测试
```

---

## 🔌 插件系统

### 最小化插件示例

```cpp
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
            []() { LOG_INFO("Hello from MyPlugin!"); }
        );

        // 注册视图
        ContentRegistry::Views::add<MyView>();

        return Result::ok();
    }
};
```

**更多内容**: [插件系统指南](plugin_system_guide.md)

---

## 📖 核心概念

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

### Content Registry

Content Registry 是 DearTs 的核心概念，提供统一的接口来注册：

- **Commands** - 命令和快捷键
- **Views** - 可停靠窗口视图
- **Tools** - 工具和实用程序
- **Settings** - 配置选项

### 事件系统

```cpp
// 定义事件
struct MyEvent {
    int value;
};

// 订阅事件（RAII 自动管理）
EventBus::Token token = EventBus::instance().subscribe<MyEvent>(
    [](const MyEvent& e) {
        LOG_INFO("Received: {}", e.value);
    }
);

// 发布事件
EventBus::instance().publish(MyEvent{42});
```

---

## 📝 代码规范

### 命名约定

| 类型 | 约定 | 示例 |
|------|------|------|
| 类名 | PascalCase | `PluginManager`, `EventBus` |
| 函数名 | snake_case | `on_load()`, `get_info()` |
| 成员变量 | snake_case + `m_` 前缀 | `m_plugins`, `m_eventBus` |
| 常量 | UPPER_CASE | `MAX_PLUGINS`, `API_VERSION` |

### 错误处理

```cpp
// ✅ 正确：使用 Result<T, E>
Result<void, std::string> on_load() override {
    if (!init()) {
        return Result::err("Initialization failed");
    }
    return Result::ok();
}

// ❌ 错误：使用异常
void on_load() override {
    if (!init()) {
        throw std::runtime_error("Failed");
    }
}
```

### RAII 资源管理

```cpp
// ✅ 正确：使用 RAII
class MyPlugin : public IPlugin {
private:
    EventBus::Token m_eventToken;  // 自动取消订阅
    ConfigScope m_config{"myplugin"};  // 自动前缀
};

// ❌ 错误：手动管理资源
class MyPlugin : public IPlugin {
private:
    EventBus::Token* m_eventToken;  // 需要手动 delete
};
```

**完整规范**: [代码规范](代码规范.md)

---

## 🔗 相关资源

### 官方文档

- **[开发者文档](../dearts-dev/)** - 完整的 API 参考和开发指南
- **[GitHub 仓库](https://github.com/ygsheep/DearTs)** - 源代码和问题追踪
- **[示例代码](../dearts-dev/examples/)** - 示例和教程

### 参考项目

- **[ImHex](https://github.com/WerWolv/ImHex)** - 插件系统和 Content Registry 灵感来源
- **[SDL3](https://github.com/libsdl-org/SDL)** - 跨平台多媒体库
- **[ImGui](https://github.com/ocornut/imgui)** - 立即模式 GUI 框架

---

## 🤝 贡献

欢迎贡献代码、报告问题或提出建议！

### 如何贡献

1. **Fork 本仓库**
2. **创建功能分支** (`git checkout -b feature/AmazingFeature`)
3. **提交更改** (`git commit -m 'Add some AmazingFeature'`)
4. **推送到分支** (`git push origin feature/AmazingFeature`)
5. **提交 Pull Request**

### 贡献要求

- ✅ 遵循项目的 [代码规范](代码规范.md)
- ✅ 添加必要的测试和文档
- ✅ 确保所有测试通过
- ✅ 保持提交历史清晰

---

<div align="center">

**需要帮助？** 查看 [开发者文档](../dearts-dev/) 或提交 [GitHub Issue](https://github.com/ygsheep/DearTs/issues)

[⬆ 返回顶部](#dearts-framework--文档中心)

</div>
