# Content Registry 详解

DearTs Framework 参考 ImHex 设计，提供了强大的 Content Registry 系统，用于管理命令、工具、设置等内容。

## 概述

**核心文件**:
- `core/content/registry_base.h` - 注册表基类
- `core/content/commands.h` - 命令系统
- `core/content/tools.h` - 工具系统
- `core/content/settings.h` - 设置系统
- `core/content/callbacks.h` - 回调系统

Content Registry 是一个中心化的内容管理系统，用于注册和访问应用的各种功能组件。

## 核心概念

### 注册表模式

```cpp
namespace ContentRegistry {

// 命令注册
namespace Commands {
    using Callback = std::function<void(const std::string& args)>;
    using QueryCallback = std::function<bool(const std::string& args)>;

    void registerHandler(
        const std::string& id,           // 命令 ID
        const std::string& name,         // 显示名称
        Callback callback,               // 执行回调
        QueryCallback query = nullptr    // 可用性查询
    );
}

// 工具注册
namespace Tools {
    using Callback = std::function<void()>;

    void add(
        const std::string& name,         // 工具名称
        Callback callback               // 渲染回调
    );
}

// 设置注册
namespace Settings {
    void add(
        const std::string& key,          // 设置键
        const std::string& name,         // 显示名称
        const T& defaultValue,            // 默认值
        const std::string& description   // 描述
    );
}

}  // namespace ContentRegistry
```

## 命令系统

### 注册命令

```cpp
// 注册简单命令
ContentRegistry::Commands::registerHandler(
    "file.save",                          // 命令 ID
    "Save current file",                  // 显示名称
    [](const auto& args) {
        LOG_INFO("Saving file...");
        saveFile();
    }
);

// 注册带查询的命令
ContentRegistry::Commands::registerHandler(
    "file.save",
    "Save current file",
    [](const auto& args) {
        saveFile();
    },
    [](const auto& args) -> bool {
        // 查询是否可用
        return hasUnsavedChanges();
    }
);

// 注册快捷键
ContentRegistry::Commands::registerHandler(
    "file.save",
    "Save current file",
    [](const auto& args) {
        saveFile();
    },
    nullptr,
    "Ctrl+S"  // 快捷键
);
```

### 执行命令

```cpp
// 直接执行
ContentRegistry::Commands::invoke("file.save");

// 带参数执行
ContentRegistry::Commands::invoke(
    "file.open",
    "/path/to/file.hex"
);

// 检查命令是否存在
bool exists = ContentRegistry::Commands::hasHandler("file.save");

// 获取命令名称
std::string name = ContentRegistry::Commands::getCommandName("file.save");
```

### 命令分组

```cpp
// 注册分组命令
ContentRegistry::Commands::registerGroup("file", "File");
ContentRegistry::Commands::registerGroup("edit", "Edit");
ContentRegistry::Commands::registerGroup("view", "View");

// 添加到分组
ContentRegistry::Commands::addToGroup("file", "file.new");
ContentRegistry::Commands::addToGroup("file", "file.open");
ContentRegistry::Commands::addToGroup("file", "file.save");
```

## 工具系统

### 注册工具

```cpp
// 注册简单工具
ContentRegistry::Tools::add(
    "Hex Editor",                         // 工具名称
    []() {
        if (ImGui::Begin("Hex Editor")) {
            ImGui::Text("Hex Editor Content");
            // ... 工具内容
        }
        ImGui::End();
    }
);

// 注册带图标的工具
ContentRegistry::Tools::add(
    ICON_FA_MEMORY " Data Inspector",     // 带图标
    []() {
        // ... 工具内容
    }
);
```

### 工具窗口管理

```cpp
// 显示工具
ContentRegistry::Tools::show("Hex Editor");

// 隐藏工具
ContentRegistry::Tools::hide("Hex Editor");

// 切换工具
ContentRegistry::Tools::toggle("Hex Editor");

// 检查工具是否可见
bool visible = ContentRegistry::Tools::isVisible("Hex Editor");
```

## 设置系统

### 注册设置

```cpp
// 注册布尔设置
ContentRegistry::Settings::add(
    "general.auto_save",                  // 设置键
    "Auto Save",                          // 显示名称
    true,                                 // 默认值
    "Automatically save files"            // 描述
);

// 注册整数设置
ContentRegistry::Settings::add(
    "editor.font_size",
    "Font Size",
    14,
    "Editor font size in pixels"
);

// 注册字符串设置
ContentRegistry::Settings::add(
    "editor.theme",
    "Theme",
    "Dark",
    "Editor color theme"
);

// 注册浮点数设置
ContentRegistry::Settings::add(
    "editor.scale",
    "Interface Scale",
    1.0f,
    "Interface scaling factor"
);
```

### 读取和写入设置

```cpp
// 读取设置
bool autoSave = ContentRegistry::Settings::read(
    "general.auto_save",
    true  // 默认值
);

int fontSize = ContentRegistry::Settings::read(
    "editor.font_size",
    14
);

// 写入设置
ContentRegistry::Settings::write(
    "general.auto_save",
    false
);

// 检查设置是否存在
bool exists = ContentRegistry::Settings::has("general.auto_save");
```

### 设置持久化

```cpp
// 保存设置到文件
ContentRegistry::Settings::save("settings.json");

// 从文件加载设置
ContentRegistry::Settings::load("settings.json");

// 重置为默认值
ContentRegistry::Settings::reset();
```

## 回调系统

### 注册生命周期回调

```cpp
// 启动回调
ContentRegistry::Callbacks::add(
    CallbackType::Startup,
    []() {
        LOG_INFO("Application starting");
        initializePlugins();
    }
);

// 关闭回调
ContentRegistry::Callbacks::add(
    CallbackType::Shutdown,
    []() {
        LOG_INFO("Application shutting down");
        cleanupPlugins();
    }
);

// 文件打开回调
ContentRegistry::Callbacks::add(
    CallbackType::FileOpened,
    [](const std::string& path) {
        LOG_INFO("File opened: {}", path);
        onFileOpened(path);
    }
);
```

### 触发回调

```cpp
// 触发启动回调
ContentRegistry::Callbacks::trigger(CallbackType::Startup);

// 触发文件打开回调
ContentRegistry::Callbacks::trigger(
    CallbackType::FileOpened,
    "/path/to/file.hex"
);

// 触发关闭回调
ContentRegistry::Callbacks::trigger(CallbackType::Shutdown);
```

## 实际应用示例

### 1. 创建菜单系统

```cpp
void registerMenuItems() {
    // File 菜单
    ContentRegistry::Commands::registerHandler(
        "file.new",
        "New",
        []() { createNewFile(); },
        nullptr,
        "Ctrl+N"
    );

    ContentRegistry::Commands::registerHandler(
        "file.open",
        "Open",
        []() { openFileDialog(); },
        nullptr,
        "Ctrl+O"
    );

    // Edit 菜单
    ContentRegistry::Commands::registerHandler(
        "edit.undo",
        "Undo",
        []() { undo(); },
        []() { return canUndo(); },  // 查询是否可用
        "Ctrl+Z"
    );

    ContentRegistry::Commands::registerHandler(
        "edit.redo",
        "Redo",
        []() { redo(); },
        []() { return canRedo(); },
        "Ctrl+Y"
    );
}

// 渲染菜单
void renderMenu() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New", "Ctrl+N")) {
                ContentRegistry::Commands::invoke("file.new");
            }
            if (ImGui::MenuItem("Open", "Ctrl+O")) {
                ContentRegistry::Commands::invoke("file.open");
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) {
                ContentRegistry::Commands::invoke("app.exit");
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {
            bool canUndo = ContentRegistry::Commands::query("edit.undo");
            bool canRedo = ContentRegistry::Commands::query("edit.redo");

            if (ImGui::MenuItem("Undo", "Ctrl+O", false, canUndo)) {
                ContentRegistry::Commands::invoke("edit.undo");
            }
            if (ImGui::MenuItem("Redo", "Ctrl+Y", false, canRedo)) {
                ContentRegistry::Commands::invoke("edit.redo");
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}
```

### 2. 创建工具窗口

```cpp
void registerTools() {
    // Hex 编辑器工具
    ContentRegistry::Tools::add(
        "Hex Editor",
        []() {
            static std::array<uint8_t, 256> data = {0};

            if (ImGui::Begin("Hex Editor")) {
                if (ImGui::BeginTable("HexTable", 16)) {
                    for (size_t i = 0; i < data.size(); i++) {
                        ImGui::TableNextColumn();
                        ImGui::Text("%02X", data[i]);
                    }
                    ImGui::EndTable();
                }
            }
            ImGui::End();
        }
    );

    // 数据检查器工具
    ContentRegistry::Tools::add(
        "Data Inspector",
        []() {
            if (ImGui::Begin("Data Inspector")) {
                ImGui::Text("Data Analysis");
                // ... 数据分析内容
            }
            ImGui::End();
        }
    );
}
```

### 3. 配置管理

```cpp
void initializeSettings() {
    // 通用设置
    ContentRegistry::Settings::add("general.auto_save", "Auto Save", true);
    ContentRegistry::Settings::add("general.auto_save_interval", "Auto Save Interval", 60);
    ContentRegistry::Settings::add("general.recent_files", "Recent Files", std::vector<std::string>{});

    // 编辑器设置
    ContentRegistry::Settings::add("editor.font_size", "Font Size", 14);
    ContentRegistry::Settings::add("editor.show_line_numbers", "Show Line Numbers", true);
    ContentRegistry::Settings::add("editor.word_wrap", "Word Wrap", false);

    // 界面设置
    ContentRegistry::Settings::add("ui.theme", "Theme", std::string("Dark"));
    ContentRegistry::Settings::add("ui.scale", "Scale", 1.0f);
    ContentRegistry::Settings::add("ui.language", "Language", std::string("English"));

    // 加载设置
    ContentRegistry::Settings::load("settings.json");
}

void renderSettingsWindow() {
    if (ImGui::Begin("Settings")) {
        if (ImGui::CollapsingHeader("General")) {
            bool autoSave = ContentRegistry::Settings::read("general.auto_save", true);
            if (ImGui::Checkbox("Auto Save", &autoSave)) {
                ContentRegistry::Settings::write("general.auto_save", autoSave);
            }
        }

        if (ImGui::CollapsingHeader("Editor")) {
            int fontSize = ContentRegistry::Settings::read("editor.font_size", 14);
            if (ImGui::SliderInt("Font Size", &fontSize, 10, 24)) {
                ContentRegistry::Settings::write("editor.font_size", fontSize);
            }
        }
    }
    ImGui::End();
}
```

## 最佳实践

### 1. 命令 ID 命名规范

```cpp
// ✅ 推荐 - 层级化命名
"file.new"
"file.open"
"file.save"
"edit.undo"
"edit.redo"
"view.hex_editor"
"view.data_inspector"

// ❌ 不推荐 - 扁平命名
"new_file"
"open_file"
"save_file"
"undo_edit"
```

### 2. 设置键命名规范

```cpp
// ✅ 推荐 - 分类.属性
"general.auto_save"
"editor.font_size"
"ui.theme"
"network.proxy_url"

// ❌ 不推荐 - 扁平命名
"auto_save"
"font_size"
"theme"
"proxy_url"
```

### 3. 使用 lambda 捕获避免全局状态

```cpp
// ✅ 推荐
class FileManager {
public:
    void registerCommands() {
        ContentRegistry::Commands::registerHandler(
            "file.save",
            "Save File",
            [this]() { saveFile(); }  // 捕获 this
        );
    }

private:
    void saveFile();
};

// ❌ 不推荐 - 全局变量
std::string currentFile;

void registerCommands() {
    ContentRegistry::Commands::registerHandler(
        "file.save",
        "Save File",
        []() { saveFile(currentFile); }  // 依赖全局变量
    );
}
```

### 4. 提供有意义的默认值

```cpp
// ✅ 推荐
ContentRegistry::Settings::add(
    "editor.font_size",
    "Font Size",
    14,  // 合理的默认值
    "Editor font size in pixels"
);

// ❌ 不推荐
ContentRegistry::Settings::add(
    "editor.font_size",
    "Font Size",
    0,  // 无效的默认值
    "Editor font size in pixels"
);
```

## API 参考

### Commands

```cpp
namespace Commands {
    void registerHandler(
        const std::string& id,
        const std::string& name,
        Callback callback,
        QueryCallback query = nullptr,
        const std::string& shortcut = ""
    );

    void invoke(const std::string& id, const Args& args = {});
    bool hasHandler(const std::string& id);
    bool query(const std::string& id, const Args& args = {});
    std::string getCommandName(const std::string& id);
}
```

### Tools

```cpp
namespace Tools {
    void add(const std::string& name, Callback callback);
    void show(const std::string& name);
    void hide(const std::string& name);
    void toggle(const std::string& name);
    bool isVisible(const std::string& name);
}
```

### Settings

```cpp
namespace Settings {
    template<typename T>
    void add(const std::string& key, const std::string& name,
             const T& defaultValue, const std::string& description = "");

    template<typename T>
    T read(const std::string& key, const T& defaultValue = T{});

    template<typename T>
    void write(const std::string& key, const T& value);

    bool has(const std::string& key);
    void save(const std::string& path);
    void load(const std::string& path);
    void reset();
}
```

## 参考资源

- ImHex ContentRegistry: https://github.com/WerWolv/ImHex
- 源码: `core/content/`
- 示例: `examples/demo_imhex_style/`
