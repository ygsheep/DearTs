# 命令面板UI功能 - PR实现计划

## 📋 功能概述

为DearTs框架添加类似VS Code/IntelliJ的命令面板（Command Palette），提供快速命令执行、搜索过滤和键盘导航功能。

**参考设计**：
- VS Code命令面板（Ctrl+Shift+P）
- IntelliJ IDEA Actions（Ctrl+Shift+A）
- ImHex命令面板

## 🎯 核心目标

1. **快速命令执行**：通过快捷键（Ctrl+Shift+P）打开命令面板
2. **模糊搜索**：实时过滤已注册命令
3. **键盘导航**：完整键盘支持（↑↓选择、Enter执行、Esc关闭）
4. **UI美观**：现代化ImGui设计，与框架风格一致
5. **可扩展**：插件可轻松注册命令到面板

## 📐 架构设计

### 1. 类设计

#### `CommandPaletteView`（主要视图类）

```cpp
// core/ui/command_palette_view.h
class CommandPaletteView : public ViewModal {
private:
    char m_searchBuffer[256];           // 搜索缓冲区
    std::vector<size_t> m_filtered;    // 过滤后的命令索引
    size_t m_selectedIndex;            // 当前选中项索引
    bool m_justOpened;                 // 刚打开状态标记

public:
    CommandPaletteView();
    ~CommandPaletteView() override = default;

    std::string getName() const override { return "Command Palette"; }
    void draw_content() override;
    void on_open() override;

private:
    void update_filter(const std::string& query);
    void execute_selected_command();
    void navigate_selection(int delta);
    bool matches_search(const CommandItem& cmd, const std::string& query);
};
```

### 2. 文件结构

```
core/ui/
├── command_palette_view.h          # 命令面板视图头文件
├── command_palette_view.cpp        # 命令面板视图实现
└── command_palette_commands.h      # 命令面板专用命令定义
```

### 3. 集成点

#### 3.1 Content Registry集成

利用现有的 `ContentRegistry::Commands` 系统：

```cpp
// 从 ContentRegistry::Commands::Registry::instance() 获取所有命令
// CommandPalette 作为命令的浏览器和执行器
```

#### 3.2 快捷键集成

在 `ShortcutManager` 中注册全局快捷键：

```cpp
// Ctrl+Shift+P (Windows/Linux)
// Cmd+Shift+P (macOS)
ShortcutManager::addShortcut(
    ShortcutType::Global,
    "command_palette.open",
    "Ctrl+Shift+P",
    []() {
        // 打开命令面板
        auto* palette = ContentRegistry::Views::get_by_name("Command Palette");
        if (palette) palette->get_window_open_state() = true;
    }
);
```

#### 3.3 TitleBar集成（可选）

在TitleBar添加命令按钮：

```cpp
TitleBar::instance().add_button(
    ICON_FA_SEARCH,
    "Command Palette (Ctrl+Shift+P)",
    []() {
        // 打开命令面板
    }
);
```

## 🎨 UI设计

### 命令面板布局

```
┌─────────────────────────────────────┐
│  Command Palette              × _ □ │  <- 模态窗口标题
├─────────────────────────────────────┤
│  🔍 [Search commands...__________]  │  <- 搜索输入框
├─────────────────────────────────────┤
│  ▶ Open File...        Ctrl+O       │  <- 命令列表（第一项选中）
│    Save File           Ctrl+S       │
│    Export Data         Ctrl+E       │
│    Settings                        │
│    Toggle Theme                     │
│    About                           │
└─────────────────────────────────────┘
```

### 视觉特性

1. **模态窗口**：使用 `ViewModal` 基类，始终置顶
2. **居中显示**：初始位置居中，尺寸固定（600x400）
3. **聚焦输入**：打开时自动聚焦搜索框
4. **高亮选中**：当前选中项使用不同背景色
5. **快捷键显示**：每个命令右侧显示快捷键（如有）
6. **图标提示**：命令前显示相应图标

### 交互逻辑

| 快捷键          | 动作                      |
|----------------|--------------------------|
| Ctrl+Shift+P   | 打开命令面板              |
| Esc            | 关闭面板                  |
| ↑ / ↓          | 选择上一/下一项           |
| Enter          | 执行选中命令              |
| Ctrl+J / Ctrl+K| 上一项（VS Code风格）     |
| Ctrl+K / Ctrl+J| 下一项（VS Code风格）     |
| 输入文字        | 实时过滤命令              |

## 🔧 技术实现细节

### 1. 搜索过滤算法

```cpp
bool CommandPaletteView::matches_search(
    const CommandItem& cmd,
    const std::string& query
) {
    if (query.empty()) return true;

    std::string lowerQuery = to_lower(query);
    std::string lowerName = to_lower(cmd.name);
    std::string lowerDesc = to_lower(cmd.description);

    // 模糊匹配：名称或描述包含查询词
    return lowerName.find(lowerQuery) != std::string::npos ||
           lowerDesc.find(lowerQuery) != std::string::npos;
}

void CommandPaletteView::update_filter(const std::string& query) {
    m_filtered.clear();
    auto& commands = ContentRegistry::Commands::Registry::instance().get_commands();

    for (size_t i = 0; i < commands.size(); i++) {
        if (matches_search(commands[i], query)) {
            m_filtered.push_back(i);
        }
    }

    // 调整选中索引
    if (m_selectedIndex >= m_filtered.size()) {
        m_selectedIndex = m_filtered.empty() ? 0 : m_filtered.size() - 1;
    }
}
```

### 2. 键盘导航

```cpp
void CommandPaletteView::draw_content() {
    // 搜索框
    if (m_justOpened) {
        ImGui::SetKeyboardFocusHere(0);
        m_justOpened = false;
    }

    if (ImGui::InputTextWithHint(
        "##search",
        ICON_FA_SEARCH " Search commands...",
        m_searchBuffer,
        sizeof(m_searchBuffer),
        ImGuiInputTextFlags_EnterReturnsTrue
    )) {
        execute_selected_command();
    }

    // 处理键盘导航
    if (ImGui::IsItemFocused()) {
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            m_window_open = false;
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
            navigate_selection(-1);
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
            navigate_selection(1);
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_Enter)) {
            execute_selected_command();
        }
    }

    // 更新过滤
    std::string query(m_searchBuffer);
    if (query != m_lastQuery) {
        update_filter(query);
        m_lastQuery = query;
    }

    // 命令列表
    draw_command_list();
}
```

### 3. 命令执行

```cpp
void CommandPaletteView::execute_selected_command() {
    if (m_selectedIndex < m_filtered.size()) {
        size_t cmdIndex = m_filtered[m_selectedIndex];
        auto& commands = ContentRegistry::Commands::Registry::instance().get_commands();
        auto& cmd = commands[cmdIndex];

        // 检查命令是否可用
        if (!cmd.enabled_callback || cmd.enabled_callback()) {
            cmd.callback();
            m_window_open = false; // 执行后关闭
        } else {
            LOG_WARN("Command '{}' is currently disabled", cmd.name);
        }
    }
}
```

## 📦 实现任务清单

### Phase 1: 核心功能（MVP）

- [ ] **Task 1.1**: 创建 `CommandPaletteView` 类
  - [ ] 继承 `ViewModal`
  - [ ] 实现基本UI布局
  - [ ] 搜索输入框

- [ ] **Task 1.2**: 实现命令列表显示
  - [ ] 从 ContentRegistry 获取命令
  - [ ] 渲染命令列表
  - [ ] 显示快捷键

- [ ] **Task 1.3**: 实现搜索过滤
  - [ ] 模糊匹配算法
  - [ ] 实时过滤更新
  - [ ] 空结果处理

- [ ] **Task 1.4**: 实现键盘交互
  - [ ] Esc 关闭
  - [ ] Enter 执行
  - [ ] ↑↓ 导航

- [ ] **Task 1.5**: 注册命令面板视图
  - [ ] 在 BuiltinPlugin 中注册
  - [ ] 绑定快捷键 Ctrl+Shift+P
  - [ ] 测试基本功能

### Phase 2: 增强功能

- [ ] **Task 2.1**: 高级搜索
  - [ ] 支持命令分类
  - [ ] 支持标签搜索
  - [ ] 搜索历史

- [ ] **Task 2.2**: 视觉优化
  - [ ] 选中项高亮动画
  - [ ] 命令分类分隔线
  - [ ] 图标支持

- [ ] **Task 2.3**: TitleBar集成
  - [ ] 添加命令按钮
  - [ ] 工具提示
  - [ ] 可配置显示/隐藏

- [ ] **Task 2.4**: 配置支持
  - [ ] 快捷键自定义
  - [ ] 面板尺寸记忆
  - [ ] 主题适配

### Phase 3: 文档和测试

- [ ] **Task 3.1**: 编写文档
  - [ ] API 文档
  - [ ] 用户指南
  - [ ] 插件开发指南

- [ ] **Task 3.2**: 单元测试
  - [ ] 搜索算法测试
  - [ ] 命令执行测试
  - [ ] 键盘导航测试

- [ ] **Task 3.3**: 示例插件
  - [ ] 命令面板使用示例
  - [ ] 自定义命令示例

## 🧪 测试计划

### 单元测试

```cpp
// tests/command_palette_test.cpp
TEST(CommandPalette, FilterEmpty) {
    CommandPaletteView palette;
    palette.update_filter("");
    EXPECT_GT(palette.get_filtered_count(), 0);
}

TEST(CommandPalette, FilterSpecific) {
    CommandPaletteView palette;
    palette.update_filter("save");
    auto count = palette.get_filtered_count();
    EXPECT_GT(count, 0);
}

TEST(CommandPalette, NavigateSelection) {
    CommandPaletteView palette;
    palette.navigate_selection(1); // 下
    palette.navigate_selection(-1); // 上
    EXPECT_EQ(palette.get_selected_index(), 0);
}
```

### 集成测试

1. **命令注册测试**：确保所有已注册命令都出现在面板中
2. **快捷键测试**：Ctrl+Shift+P 正确打开面板
3. **命令执行测试**：选中命令能正确执行
4. **插件命令测试**：插件注册的命令也显示在面板中

### 手动测试清单

- [ ] 打开/关闭命令面板
- [ ] 搜索命令（中文、英文、特殊字符）
- [ ] 键盘导航（↑↓EnterEsc）
- [ ] 执行各种命令（启用/禁用状态）
- [ ] 空搜索结果显示
- [ ] 无匹配搜索结果显示
- [ ] 快捷键显示正确性
- [ ] 多次快速打开/关闭

## 📝 API文档示例

### 为插件开发者

```cpp
// 插件如何添加命令到命令面板
class MyPlugin : public IPlugin {
    Result<void, std::string> on_load() override {
        // 注册命令（自动出现在命令面板）
        ContentRegistry::Commands::register_handler(
            "myplugin.do_something",
            "Do Something",
            []() {
                LOG_INFO("Doing something...");
            },
            []() { return true; }, // 启用条件
            "Ctrl+Shift+D"          // 快捷键
        );

        return Result::ok();
    }
};
```

### 为框架开发者

```cpp
// 如何自定义命令面板行为
class CustomCommandPalette : public CommandPaletteView {
protected:
    bool matches_search(const CommandItem& cmd, const std::string& query) override {
        // 自定义搜索算法
        return custom_match(cmd, query);
    }

    void draw_command_item(const CommandItem& cmd, bool isSelected) override {
        // 自定义命令项渲染
    }
};
```

## 🚀 发布计划

### Milestone 1: MVP（最小可行产品）
**时间**: v1.1.0
- Phase 1 所有任务完成
- 基本命令面板功能可用
- 基本文档完成

### Milestone 2: 增强版
**时间**: v1.2.0
- Phase 2 所有任务完成
- 高级搜索功能
- TitleBar集成
- 完整文档

### Milestone 3: 稳定版
**时间**: v1.3.0
- Phase 3 所有任务完成
- 单元测试覆盖 >80%
- 性能优化
- 生产就绪

## 🔄 向后兼容性

- ✅ **不破坏现有API**：完全基于现有 ContentRegistry
- ✅ **可选功能**：默认启用，可通过配置禁用
- ✅ **插件兼容**：现有插件自动支持命令面板

## 📊 成功指标

1. **性能**：
   - 打开时间 < 100ms
   - 搜索响应 < 16ms（60fps）
   - 支持至少 1000 个命令

2. **可用性**：
   - 键盘导航流畅无延迟
   - 搜索结果准确率 >95%
   - 用户学习曲线 < 5分钟

3. **代码质量**：
   - 单元测试覆盖率 >80%
   - 无内存泄漏
   - 符合框架编码规范

## 📚 参考资料

- VS Code命令面板设计：https://code.visualstudio.com/docs/getstarted/tips-and-tricks#_command-palette
- ImGui输入处理：https://github.com/ocornut/imgui/blob/master/docs/FAQ.md
- ImHex命令面板实现：https://github.com/WerWolv/ImHex
- 模糊搜索算法：https://www.forrestthewoods.com/blog/reverse_engineering_sublime_texts_fuzzy_match/

## ✅ 检查清单

在提交PR前确保：

- [ ] 所有功能已实现且测试通过
- [ ] 代码符合框架编码规范
- [ ] 文档完整且准确
- [ ] 性能指标满足要求
- [ ] 无编译警告
- [ ] 内存泄漏检查通过
- [ ] 向后兼容性验证
- [ ] 插件集成测试通过

---

**文档版本**: 1.0
**创建日期**: 2025-12-30
**预计PR编号**: #XXX
**预计里程碑**: v1.1.0
