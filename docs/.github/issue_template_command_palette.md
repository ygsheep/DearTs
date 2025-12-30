# GitHub Issue 模板 - 命令面板功能

你可以直接复制以下内容到GitHub创建Issue：

---

## 🎯 功能概述

为DearTs框架添加类似VS Code/IntelliJ的命令面板（Command Palette），提供快速命令执行、搜索过滤和键盘导航功能。

**参考设计**：
- VS Code命令面板（Ctrl+Shift+P）
- IntelliJ IDEA Actions（Ctrl+Shift+A）

## 📋 核心功能

### MVP功能（Phase 1）

1. **快速命令执行**
   - 快捷键打开（Ctrl+Shift+P / Cmd+Shift+P）
   - 命令列表显示
   - 选中命令执行

2. **模糊搜索**
   - 实时过滤已注册命令
   - 按名称和描述匹配
   - 空结果友好提示

3. **键盘导航**
   - `↑` / `↓` 选择命令
   - `Enter` 执行选中命令
   - `Esc` 关闭面板
   - 自动聚焦搜索框

4. **UI设计**
   - 模态窗口（始终置顶）
   - 居中显示（600x400）
   - 选中项高亮
   - 快捷键显示

### 增强功能（Phase 2）

- 命令分类和标签
- 搜索历史
- TitleBar集成（可选）
- 配置支持（快捷键自定义、尺寸记忆）

## 🏗️ 架构设计

### 新增文件

```
core/ui/
├── command_palette_view.h          # 命令面板视图头文件
├── command_palette_view.cpp        # 命令面板视图实现
└── command_palette_commands.h      # 命令面板专用命令定义
```

### 集成点

1. **Content Registry**: 利用现有的 `ContentRegistry::Commands` 系统
2. **View系统**: 继承 `ViewModal` 基类
3. **ShortcutManager**: 注册全局快捷键 `Ctrl+Shift+P`
4. **BuiltinPlugin**: 注册命令面板视图

### 类设计

```cpp
class CommandPaletteView : public ViewModal {
private:
    char m_searchBuffer[256];
    std::vector<size_t> m_filtered;
    size_t m_selectedIndex;
    bool m_justOpened;

public:
    void draw_content() override;
    void on_open() override;

private:
    void update_filter(const std::string& query);
    void execute_selected_command();
    void navigate_selection(int delta);
    bool matches_search(const CommandItem& cmd, const std::string& query);
};
```

## 📊 技术细节

### 搜索算法

- 模糊匹配（按名称和描述）
- 实时过滤更新
- 大小写不敏感

### 键盘交互

- 利用 ImGui 的键盘输入处理
- 支持 `ImGuiKey_UpArrow`, `ImGuiKey_DownArrow`, `ImGuiKey_Enter`, `ImGuiKey_Escape`
- 自动聚焦搜索框

### UI布局

```
┌─────────────────────────────────────┐
│  Command Palette              ×     │
├─────────────────────────────────────┤
│  🔍 [Search commands...__________]  │
├─────────────────────────────────────┤
│  ▶ Open File...        Ctrl+O       │
│    Save File           Ctrl+S       │
│    Export Data         Ctrl+E       │
│    Settings                        │
└─────────────────────────────────────┘
```

## 📦 实现任务

### Phase 1: MVP (核心功能)

- [ ] 创建 `CommandPaletteView` 类
- [ ] 实现命令列表显示
- [ ] 实现搜索过滤
- [ ] 实现键盘交互
- [ ] 注册视图和快捷键

### Phase 2: 增强功能

- [ ] 高级搜索（分类、标签、历史）
- [ ] 视觉优化（动画、图标）
- [ ] TitleBar集成
- [ ] 配置支持

### Phase 3: 文档和测试

- [ ] API文档
- [ ] 用户指南
- [ ] 单元测试
- [ ] 示例插件

## ✅ 成功指标

- **性能**: 打开时间 < 100ms，搜索响应 < 16ms
- **可用性**: 支持至少 1000 个命令
- **代码质量**: 单元测试覆盖率 >80%

## 📚 参考资料

- **完整PR计划**: 查看 `docs/command_palette_pr_plan.md` 获取详细实现计划
- **VS Code命令面板**: https://code.visualstudio.com/docs/getstarted/tips-and-tricks#_command-palette
- **ImGui输入处理**: https://github.com/ocornut/imgui/blob/master/docs/FAQ.md

## 🔗 相关文件

- **设计文档**: `docs/command_palette_pr_plan.md`
- **UI系统**: `core/ui/view.h`
- **命令系统**: `core/content/commands.h`
- **快捷键管理**: `core/ui/shortcut_manager.h`

## 📝 额外说明

- **向后兼容**: ✅ 不破坏现有API，完全基于现有 ContentRegistry
- **插件兼容**: ✅ 现有插件自动支持命令面板
- **可配置性**: ✅ 可通过配置禁用或自定义快捷键

---

**预计里程碑**: v1.1.0
**优先级**: High（核心UI功能）
**复杂度**: Medium（基于现有系统）
**预计工作量**: 3-5天

---

## 📋 使用说明

### 方法1: 使用GitHub网页界面（推荐）

1. 访问 https://github.com/ygsheep/DearTs/issues/new
2. 将上面的内容复制粘贴到Issue描述中
3. 设置标题：`[Feature] 添加命令面板UI (Command Palette)`
4. 添加标签：`enhancement`, `ui`, `good first issue`
5. 点击 "Submit new issue"

### 方法2: 使用gh CLI（需要认证）

```bash
# 登录GitHub（首次使用）
gh auth login

# 创建Issue
gh issue create \
  --title "[Feature] 添加命令面板UI (Command Palette)" \
  --label "enhancement,ui,good first issue" \
  --body-file docs/.github/issue_template_command_palette.md
```

### 方法3: 使用Git命令（通过MCP工具）

如果你已经配置了GitHub MCP工具的认证，可以直接使用：
```bash
# MCP工具会自动创建Issue
# 无需手动操作
```

---

**下一步**: Issue创建后，就可以开始实现功能了！按照 `docs/command_palette_pr_plan.md` 中的任务清单逐步完成。
