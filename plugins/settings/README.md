# Settings Plugin

DearTs 设置管理插件，提供图形化的配置编辑界面。

## 功能特性

### ✅ 已实现

- **配置视图** - 图形化查看和管理 ConfigManager 中的所有配置项
- **分类管理** - 将配置按功能分类（通用、日志、窗口、主题等）
- **实时编辑** - 直接修改配置值，支持多种数据类型
- **配置保存** - 保存配置到 dearts_config.json 文件
- **配置重载** - 从文件重新加载配置
- **默认重置** - 一键恢复所有配置为默认值
- **修改标记** - 显示已修改的配置项（* 标记）

### 🎯 配置分类

1. **通用设置** (General)
   - 应用程序通用配置

2. **日志设置** (Logger)
   - 日志级别 (TRACE/DEBUG/INFO/WARN/ERROR/FATAL)
   - 文件输出开关
   - 日志文件路径

3. **窗口设置** (Window)
   - 窗口大小、位置等配置（待扩展）

4. **主题设置** (Theme)
   - 主题配置（待扩展）

5. **快捷键设置** (Shortcuts)
   - 快捷键绑定（待扩展）

6. **高级设置** (Advanced)
   - 高级配置选项（待扩展）

## 使用方法

### 打开设置窗口

在应用程序中，设置窗口默认不会自动打开。你可以：

1. **通过命令面板** (Ctrl+P)
   - 输入 "Settings" 或 "设置"
   - 选择打开设置视图

2. **通过菜单** (如果已实现菜单集成)
   - 选择 "编辑" → "设置"

3. **编程方式打开**
   ```cpp
   auto* settings_view = ContentRegistry::Views::get_by_name(
       ContentRegistry::UnlocalizedString("Settings")
   );
   if (settings_view != nullptr) {
       settings_view->get_window_open_state() = true;
   }
   ```

### 修改配置

1. 在左侧选择配置分类
2. 在右侧面板中修改配置值
3. 已修改的项会显示 `*` 标记
4. 点击底部"保存更改"按钮保存到文件

### 保存配置

- 点击"保存更改"按钮将所有修改保存到 `dearts_config.json`
- 配置文件位于可执行文件所在目录

### 重载配置

- 点击"重新加载"按钮从文件重新加载配置
- 会放弃所有未保存的修改

### 重置默认值

- 点击"重置默认"按钮
- 在确认对话框中选择"确定"
- 所有配置将恢复为默认值

## 配置项说明

### 日志配置

| 配置键 | 类型 | 默认值 | 说明 |
|--------|------|--------|------|
| `logger.level` | int | 1 (DEBUG) | 日志级别 (0-5) |
| `logger.file_enabled` | bool | true | 是否启用文件输出 |
| `logger.file_path` | string | "logs/deartsdl_gui.log" | 日志文件路径 |

### 窗口配置

| 配置键 | 类型 | 默认值 | 说明 |
|--------|------|--------|------|
| `toolbox.titlebar.frame_padding_x` | double | 8.0 | 标题栏水平内边距 |
| `toolbox.titlebar.frame_padding_y` | double | 10.0 | 标题栏垂直内边距 |

## 文件结构

```
plugins/settings/
├── include/
│   ├── settings_plugin.hpp        # 插件主类
│   └── views/
│       └── settings_view.hpp      # 设置视图类
├── source/
│   ├── settings_plugin.cpp        # 插件实现
│   └── settings_view.cpp          # 视图实现
├── CMakeLists.txt                 # 构建配置
└── README.md                      # 本文件
```

## 开发指南

### 添加新的配置分类

1. 在 `settings_view.hpp` 中添加枚举值：
   ```cpp
   enum class ConfigCategory {
       // ... 现有分类
       MyCategory,  // 新分类
   };
   ```

2. 在 `get_category_name()` 中添加名称：
   ```cpp
   case ConfigCategory::MyCategory: return "我的分类";
   ```

3. 在 `draw_config_panel()` 中添加处理：
   ```cpp
   case ConfigCategory::MyCategory:
       draw_my_category_settings();
       break;
   ```

4. 实现绘制函数：
   ```cpp
   void SettingsView::draw_my_category_settings() {
       // 绘制配置项
   }
   ```

### 注册新的配置项

在应用程序初始化时使用 ConfigManager 注册：

```cpp
auto& config = ConfigManager::instance();

config.register_meta("my.category.setting", {
    .description = "设置说明",
    .default_value = 42,  // 默认值
    .is_required = false
});
```

## 技术细节

### 依赖

- DearTs Core (ConfigManager, View System)
- ImGui (UI 渲染)
- liblogger (日志记录)

### 线程安全

所有配置操作都在主线程执行，ConfigManager 内部使用互斥锁保护。

### 配置文件格式

配置使用 JSON 格式存储（`dearts_config.json`）：

```json
{
  "logger": {
    "level": 1,
    "file_enabled": true,
    "file_path": "logs/deartsdl_gui.log"
  },
  "toolbox": {
    "titlebar": {
      "frame_padding_x": 8.0,
      "frame_padding_y": 10.0
    }
  }
}
```

## 未来计划

- [ ] 添加搜索功能
- [ ] 支持配置项导入/导出
- [ ] 添加配置验证
- [ ] 实现配置预设
- [ ] 添加撤销/重做功能
- [ ] 支持配置项分组
- [ ] 添加配置项描述和帮助文本
- [ ] 实现配置项过滤器

## 贡献

欢迎提交 Issue 和 Pull Request！

## 许可证

MIT License - 详见项目根目录 LICENSE 文件
