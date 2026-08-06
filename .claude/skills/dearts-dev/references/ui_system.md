# UI 系统详解

DearTs Framework 提供了完整的 UI 系统，基于 ImGui 和 SDL3，参考 ImHex 设计，支持停靠窗口、多视图、命令调色板等现代 UI 特性。

## 概述

**核心文件**:
- `core/ui/imgui_layer.h` - SDL3 + ImGui 集成层
- `core/ui/view.h` - 视图基类
- `core/ui/view_manager.h` - 视图管理器
- `core/ui/command_palette.h` - 命令调色板
- `core/ui/title_bar.h` - 自定义标题栏
- `core/ui/theme_manager.h` - 主题管理
- `core/ui/shortcut_manager.h` - 快捷键管理

## ImGui Layer

### 初始化

```cpp
class ImGuiLayer : public IApplicationLayer {
public:
    bool onAttach() override {
        // 创建 ImGui 上下文
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        // 设置样式
        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::Style();
        style.WindowRounding = 5.0f;

        // 初始化 SDL3 后端
        ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
        ImGui_ImplOpenGL3_Init("#version 130");

        return true;
    }

    void onDetach() override {
        // 清理
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
    }

    void onEvent(const SDL_Event& event) override {
        // 转发 SDL 事件到 ImGui
        ImGui_ImplSDL3_ProcessEvent(&event);
    }

    void onRender() override {
        // 开始新帧
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // 渲染 UI
        renderUI();

        // 渲染 ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // 多视口支持
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            SDL_GL_MakeCurrentWindow(backup_current_window, gl_context);
        }
    }
};
```

## 视图系统

### 创建视图

```cpp
// 视图基类
class View {
public:
    virtual ~View() = default;

    virtual std::string getName() const = 0;
    virtual void drawContent() = 0;

    virtual bool hasWindow() const { return true; }
    virtual ImGuiWindowFlags getFlags() const { return 0; }
};

// 创建自定义视图
class HexEditorView : public View {
public:
    std::string getName() const override {
        return "Hex Editor";
    }

    void drawContent() override {
        ImGui::Text("Hex Editor Content");

        // 渲染十六进制编辑器
        renderHexEditor();
    }

private:
    void renderHexEditor();
};
```

### 视图管理器

```cpp
// 注册视图
ViewManager::instance().addView<HexEditorView>();
ViewManager::instance().addView<DataInspectorView>();
ViewManager::instance().addView<MemoryView>();

// 显示视图
ViewManager::instance().showView("Hex Editor");

// 隐藏视图
ViewManager::instance().hideView("Hex Editor");

// 切换视图
ViewManager::instance().toggleView("Hex Editor");

// 检查视图是否存在
bool exists = ViewManager::instance().hasView("Hex Editor");

// 获取视图
View* view = ViewManager::instance().getView("Hex Editor");
```

### 创建停靠布局

```cpp
void createDockingLayout() {
    // 设置停靠空间
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

    // 创建停靠节点
    ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
    ImGui::DockBuilderRemoveNode(dockspace_id);
    ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);

    ImGuiID dock_main_id = dockspace_id;
    ImGuiID dock_left = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.25f, nullptr, &dock_main_id);
    ImGuiID dock_bottom = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.25f, nullptr, &dock_main_id);

    // 分配窗口到节点
    ImGui::DockBuilderDockWindow("File Browser", dock_left);
    ImGui::DockBuilderDockWindow("Hex Editor", dock_main_id);
    ImGui::DockBuilderDockWindow("Data Inspector", dock_bottom);

    ImGui::DockBuilderFinish(dockspace_id);
}
```

## 命令调色板

### 打开命令调色板

```cpp
// 显示命令调色板
if (ImGui::IsKeyPressed(ImGuiKey_LeftCtrl) && ImGui::IsKeyPressed(ImGuiKey_P)) {
    CommandPalette::instance().open();
}

// 在渲染循环中
CommandPalette::instance().render();
```

### 注册命令

```cpp
// 添加命令到调色板
CommandPalette::instance().addCommand({
    .id = "file.save",
    .name = "File: Save",
    .shortcut = "Ctrl+S",
    .category = "File",
    .callback = []() {
        saveFile();
    }
});

CommandPalette::instance().addCommand({
    .id = "edit.undo",
    .name = "Edit: Undo",
    .shortcut = "Ctrl+Z",
    .category = "Edit",
    .callback = []() {
        undo();
    }
});
```

### 命令过滤

```cpp
// 命令调色板自动支持模糊搜索
// 用户输入 "fs" 可以匹配 "File: Save"
// 用户输入 "ed" 可以匹配 "Edit: Undo"
```

## 自定义标题栏

### 渲染标题栏

```cpp
// 在渲染循环中
TitleBar::instance().render();
```

### 添加按钮

```cpp
// 添加标题栏按钮
TitleBar::instance().addButton({
    .icon = ICON_FA_SAVE,
    .tooltip = "Save",
    .onClick = []() {
        ContentRegistry::Commands::invoke("file.save");
    }
});

TitleBar::instance().addButton({
    .icon = ICON_FA_FOLDER_OPEN,
    .tooltip = "Open",
    .onClick = []() {
        ContentRegistry::Commands::invoke("file.open");
    }
});
```

### 自定义样式

```cpp
// 设置标题栏样式
TitleBar::instance().setHeight(30.0f);
TitleBar::instance().setBackgroundColor(ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
TitleBar::instance().setTextColor(ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
```

## 主题管理

### 应用主题

```cpp
// 应用内置主题
ThemeManager::instance().applyTheme("Dark");
ThemeManager::instance().applyTheme("Light");
ThemeManager::instance().applyTheme("Classic");
```

### 自定义主题

```cpp
// 创建自定义主题
ImGuiStyle style;
style.WindowRounding = 5.0f;
style.FrameRounding = 3.0f;
style.GrabRounding = 3.0f;
style.Alpha = 1.0f;
style.WindowPadding = ImVec2(8, 8);

// 颜色设置
style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
style.Colors[ImGuiCol_TitleBg] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
style.Colors[ImGuiCol_Button] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);

// 应用自定义主题
ThemeManager::instance().applyCustomStyle(style);
```

### 主题切换

```cpp
// 在设置窗口中
if (ImGui::Begin("Settings")) {
    if (ImGui::CollapsingHeader("Theme")) {
        static int currentTheme = 0;
        const char* themes[] = {"Dark", "Light", "Classic"};

        if (ImGui::Combo("Theme", &currentTheme, themes, IM_ARRAYSIZE(themes))) {
            ThemeManager::instance().applyTheme(themes[currentTheme]);
        }
    }
}
ImGui::End();
```

## 快捷键管理

### 注册快捷键

```cpp
// 注册快捷键
ShortcutManager::instance().registerShortcut(
    "file.save",              // 命令 ID
    ImGuiKey_LeftCtrl,         // 修饰键
    ImGuiKey_S                 // 主键
);

ShortcutManager::instance().registerShortcut(
    "edit.redo",
    ImGuiMod_Ctrl | ImGuiMod_Shift,
    ImGuiKey_Z
);

// 注册回调
ShortcutManager::instance().setCallback(
    "file.save",
    []() {
        LOG_INFO("Shortcut: File Save");
        saveFile();
    }
);
```

### 处理快捷键

```cpp
// 在事件处理中
void onEvent(const SDL_Event& event) override {
    // 转发到快捷键管理器
    ShortcutManager::instance().handleEvent(event);
}
```

## 实际应用示例

### 创建 ImHex 风格界面

```cpp
class ImHexStyleApp : public Application {
    bool onInitialize() override {
        // 添加 ImGui Layer
        addLayer(std::make_shared<ImGuiLayer>());

        // 注册视图
        ViewManager::instance().addView<HexEditorView>();
        ViewManager::instance().addView<DataInspectorView>();
        ViewManager::instance().addView<MemoryView>();

        // 注册命令
        registerCommands();

        // 设置标题栏按钮
        setupTitleBar();

        return true;
    }

    void onRender() override {
        // 创建停靠空间
        ImGui::DockSpaceOverViewport();

        // 渲染标题栏
        TitleBar::instance().render();

        // 渲染所有视图
        ViewManager::instance().render();

        // 渲染命令调色板（如果打开）
        CommandPalette::instance().render();
    }

    void onEvent(SDL_Event& event) override {
        // 处理快捷键
        ShortcutManager::instance().handleEvent(event);
    }

private:
    void registerCommands() {
        // 文件命令
        CommandPalette::instance().addCommand({
            .id = "file.new",
            .name = "File: New",
            .shortcut = "Ctrl+N",
            .callback = []() { createNewFile(); }
        });

        // 编辑命令
        CommandPalette::instance().addCommand({
            .id = "edit.undo",
            .name = "Edit: Undo",
            .shortcut = "Ctrl+Z",
            .callback = []() { undo(); }
        });
    }

    void setupTitleBar() {
        TitleBar::instance().addButton({
            .icon = ICON_FA_FOLDER_OPEN,
            .tooltip = "Open File",
            .onClick = []() { openFileDialog(); }
        });

        TitleBar::instance().addButton({
            .icon = ICON_FA_SAVE,
            .tooltip = "Save File",
            .onClick = []() { saveFile(); }
        });
    }
};
```

### 创建自定义视图

```cpp
class DataInspectorView : public View {
public:
    std::string getName() const override {
        return "Data Inspector";
    }

    ImGuiWindowFlags getFlags() const override {
        return ImGuiWindowFlags_MenuBar;
    }

    void drawContent() override {
        // 菜单栏
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("Format")) {
                if (ImGui::MenuItem("Hexadecimal")) m_format = Format::Hex;
                if (ImGui::MenuItem("Decimal")) m_format = Format::Decimal;
                if (ImGui::MenuItem("Binary")) m_format = Format::Binary;
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        // 显示当前选择的数据
        if (!m_selectedData.empty()) {
            ImGui::Text("Selected Data:");

            switch (m_format) {
                case Format::Hex:
                    ImGui::Text("0x%02X", m_selectedData[0]);
                    break;
                case Format::Decimal:
                    ImGui::Text("%d", m_selectedData[0]);
                    break;
                case Format::Binary:
                    ImGui::Text("0b%08b", m_selectedData[0]);
                    break;
            }
        }
    }

    void setData(const std::vector<uint8_t>& data) {
        m_selectedData = data;
    }

private:
    enum class Format { Hex, Decimal, Binary };
    Format m_format = Format::Hex;
    std::vector<uint8_t> m_selectedData;
};
```

## 最佳实践

### 1. 视图命名规范

```cpp
// ✅ 推荐 - 清晰的名称
class HexEditorView : public View {
    std::string getName() const override {
        return "Hex Editor";
    }
};

// ❌ 不推荐 - 模糊的名称
class View1 : public View {
    std::string getName() const override {
        return "View 1";
    }
};
```

### 2. 分离逻辑和渲染

```cpp
// ✅ 推荐
class DataView : public View {
    void drawContent() override {
        renderUI();
    }

private:
    void renderUI() {
        ImGui::Text("Data: {}", m_data);
    }

    std::string m_data;
};

// ❌ 不推荐 - 所有逻辑在 drawContent 中
class DataView : public View {
    void drawContent() override {
        std::string data = fetchData();  // 逻辑混在渲染中
        process(data);
        ImGui::Text("Data: {}", data);
    }
};
```

### 3. 使用 RAII 管理资源

```cpp
// ✅ 推荐
class TextureView : public View {
    TextureView() {
        m_texture = loadTexture("icon.png");
    }

    ~TextureView() {
        if (m_texture) {
            SDL_DestroyTexture(m_texture);
        }
    }

private:
    SDL_Texture* m_texture = nullptr;
};
```

### 4. 提供有意义的工具提示

```cpp
// ✅ 推荐
if (ImGui::Button("Save")) {
    saveFile();
}
if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Save current file (Ctrl+S)");
}
```

## API 参考

### ViewManager

```cpp
class ViewManager {
public:
    static ViewManager& instance();

    template<typename T>
    void addView();

    void showView(const std::string& name);
    void hideView(const std::string& name);
    void toggleView(const std::string& name);
    bool hasView(const std::string& name) const;
    View* getView(const std::string& name);

    void render();
};
```

### CommandPalette

```cpp
class CommandPalette {
public:
    static CommandPalette& instance();

    void addCommand(const Command& cmd);
    void open();
    void close();
    void render();
};
```

### TitleBar

```cpp
class TitleBar {
public:
    static TitleBar& instance();

    void render();
    void addButton(const Button& button);
    void setHeight(float height);
    void setBackgroundColor(const ImVec4& color);
};
```

### ThemeManager

```cpp
class ThemeManager {
public:
    static ThemeManager& instance();

    void applyTheme(const std::string& name);
    void applyCustomStyle(const ImGuiStyle& style);
    ImGuiStyle getCurrentStyle() const;
};
```

### ShortcutManager

```cpp
class ShortcutManager {
public:
    static ShortcutManager& instance();

    void registerShortcut(const std::string& id, ImGuiMod mods, ImGuiKey key);
    void setCallback(const std::string& id, std::function<void()> callback);
    void handleEvent(const SDL_Event& event);
};
```

## 参考资源

- ImGui 官方文档: https://github.com/ocornut/imgui
- ImGui Docking: https://github.comocornut/imgui/wiki/Docking
- ImHex UI 设计: https://github.com/WerWolv/ImHex
- 源码: `core/ui/`
- 示例: `examples/demo_imhex_style/`
