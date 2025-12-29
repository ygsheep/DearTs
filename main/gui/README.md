# DearTs 工具箱应用

基于 DearTs 框架的现代化工具箱应用程序，采用 ImGui 界面框架。

## 功能特性

### 核心功能
- ✅ **ImGui 集成** - 现代化即时模式 GUI
- ✅ **主题系统** - 支持暗色/亮色/经典主题切换
- ✅ **快捷键系统** - 全局快捷键支持
- ✅ **命令面板** - Ctrl+P 快速命令执行
- ✅ **自定义标题栏** - 美观的无边框窗口设计
- ✅ **可停靠窗口** - 灵活的界面布局
- ✅ **配置管理** - JSON 配置文件持久化
- ✅ **中文支持** - 完整的中文字体和 UI
- ✅ **任务管理** - 异步任务执行和进度跟踪
- ✅ **插件系统** - 可扩展的插件架构

### 快捷键列表

| 快捷键 | 功能 |
|--------|------|
| `Ctrl+O` | 打开文件 |
| `Ctrl+S` | 保存文件 |
| `Ctrl+Q` | 退出应用 |
| `Ctrl+P` | 命令面板 |
| `F11` | 切换全屏 |
| `Alt+Ctrl+1` | 暗色主题 |
| `Alt+Ctrl+2` | 亮色主题 |
| `Alt+Ctrl+3` | 经典主题 |

## 架构设计

### 文件结构

```
main/gui/
├── main.cpp                    # 应用入口
├── toolbox_application.hpp      # 应用类声明
├── toolbox_application.cpp      # 应用类实现
├── toolbox_plugin.hpp          # 示例插件
├── CMakeLists.txt             # 构建配置
└── README.md                   # 本文档
```

### 核心类

#### ToolboxApplication
主应用程序类，继承自 `Core::App::Application`，负责：

- **ImGui 初始化** - 设置 ImGui 上下文、字体、后端
- **配置管理** - 加载和保存应用配置
- **事件处理** - SDL 事件分发和窗口拖动
- **UI 渲染** - 菜单栏、工具窗口、DockSpace
- **快捷键** - 注册和处理全局快捷键
- **主题切换** - 动态主题切换
- **任务管理** - 异步任务执行和进度跟踪
- **插件系统** - 加载和管理插件

#### 任务管理

使用 `Core::Tasks::TaskManager` 管理异步任务：

```cpp
// 启动任务
TaskManager::instance().launch("任务名称", [](const std::atomic<bool>& should_cancel) {
    // 任务逻辑
    for (int i = 0; i <= 100 && !should_cancel; ++i) {
        // 执行工作
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
});

// 取消所有任务
TaskManager::instance().cancelAll();

// 获取运行中的任务
auto running_tasks = TaskManager::instance().getRunningTasks();
```

#### 插件系统

使用 `Core::Plugin::PluginManager` 管理插件：

```cpp
// 创建自定义插件
class MyPlugin : public Core::Plugin::Plugin {
public:
    Core::Result<void, std::string> on_load() override {
        // 插件初始化逻辑
        return Core::Result<void, std::string>::ok();
    }

    void on_unload() override {
        // 插件清理逻辑
    }

    std::string get_name() const override { return "MyPlugin"; }
    std::string get_version() const override { return "1.0.0"; }
    std::string get_author() const override { return "Your Name"; }
    std::string get_description() const override { return "My Plugin"; }
};

// 注册插件
auto plugin = std::make_unique<MyPlugin>();
PluginManager::instance().add_builtin(std::move(plugin));
```

### 配置文件

应用使用 `dearts_config.json` 存储配置：

```json
{
  "dearts.titlebar.frame_padding_x": 8.0,
  "dearts.titlebar.frame_padding_y": 10.0,
  "dearts.font.size": 16.0,
  "dearts.window.scale": 1.0
}
```

## 编译和运行

### 编译

使用 CMake 和 Ninja 构建：

```bash
cd build
cmake --build . --target deartsdl_gui --config Debug
```

或者使用提供的批处理文件：

```bash
# Windows
build_debug.bat
```

### 运行

编译完成后，可执行文件位于：
- Windows: `build/bin/deartsdl_gui.exe`

### 资源文件

应用需要以下资源文件（自动复制到输出目录）：
- `resources/fonts/NotoSansSC-Regular.ttf` - 中文字体
- `resources/fonts/MaterialSymbolsRounded-VariableFont_FILL,GRAD,opsz,wght.ttf` - 图标字体

## 扩展开发

### 添加新工具

1. **创建视图类**（继承 `Core::UI::ViewWindow`）：

```cpp
class MyToolView : public Core::UI::ViewWindow {
public:
    MyToolView() : ViewWindow("my_tool", ICON_TOOLS) {}

    void draw_content() override {
        ImGui::Text("我的工具");
        // 添加你的 UI 代码
    }
};
```

2. **在 setup_views() 中注册**：

```cpp
void ToolboxApplication::setup_views() {
    auto& view_manager = Core::UI::ViewManager::instance();
    view_manager.register_view(std::make_shared<MyToolView>());
}
```

### 添加新命令

```cpp
Commands::add("my_command", "我的命令", []() {
    LOG_INFO("执行我的命令");
});
```

### 添加新快捷键

```cpp
manager.addShortcut("my_shortcut", Shortcut(ImGuiKey_A, true, false, false), []() {
    LOG_INFO("快捷键触发: Ctrl+A");
}, ShortcutType::Global);
```

### 添加新菜单项

在 `render_menu_bar()` 中添加：

```cpp
if (ImGui::BeginMenu("我的菜单")) {
    if (ImGui::MenuItem("菜单项")) {
        // 菜单项点击处理
    }
    ImGui::EndMenu();
}
```

## 依赖项

- **SDL3** - 窗口和输入管理
- **ImGui** - 即时模式 GUI 框架
- **DearTs Core** - 应用框架
- **liblogger** - 日志系统
- **FreeType** - 字体渲染

## 已知问题

- [ ] 快捷键可能需要显式调用 `SDL_StartTextInput()`（已在 demo_imhex_style 中修复）
- [ ] 图标字体需要手动下载
- [ ] 任务进度显示需要优化

## 未来计划

- [ ] 添加更多内置工具（计算器、颜色选择器、记事本等）
- [ ] 完善插件系统（支持动态加载外部插件）
- [ ] 多语言支持
- [ ] 主题编辑器
- [ ] 布局保存和加载
- [ ] 任务历史记录和重试

## 许可证

MIT License

## 作者

DearTs Team

---

**相关文档：**
- [DearTs 框架文档](../../docs/)
- [ImGui 文档](https://github.com/ocornut/imgui)
- [SDL3 文档](https://wiki.libsdl.org/)
