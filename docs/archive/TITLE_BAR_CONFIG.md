# 标题栏配置说明 (使用 ConfigManager)

## 概述

标题栏配置现在通过 **ConfigManager** 统一管理，支持持久化、运行时修改和变更回调。

**配置键前缀**: `ui.titlebar.*`

## 配置项

所有标题栏配置项都注册在 `ConfigManager` 中，具有以下优势：

✅ **持久化**: 可以保存到 `demo_config.json`
✅ **运行时修改**: 可以在程序运行时动态调整
✅ **类型安全**: 编译时类型检查
✅ **默认值**: 未配置时使用默认值
✅ **验证**: 支持配置验证（可选）

### 完整配置列表

| 配置键 | 类型 | 默认值 | 描述 |
|--------|------|--------|------|
| `ui.titlebar.frame_padding_x` | double | 8.0 | MenuBar 水平内边距 |
| `ui.titlebar.frame_padding_y` | double | 10.0 | MenuBar 垂直内边距（决定标题栏高度） |
| `ui.titlebar.title_left_margin` | double | 10.0 | 标题文字左边距 |
| `ui.titlebar.button_width` | double | 36.0 | 按钮宽度 |
| `ui.titlebar.button_height` | double | 30.0 | 按钮高度 |
| `ui.titlebar.button_spacing` | double | 3.0 | 按钮间距 |
| `ui.titlebar.button_right_margin` | double | 10.0 | 按钮区域右边距 |
| `ui.titlebar.button_count` | int | 4 | 按钮数量 |
| `ui.titlebar.button_area_width` | double | 150.0 | 右侧按钮区域宽度（拖动检测时排除） |

## 配置文件

配置保存在 `demo_config.json` 文件中：

```json
{
  "ui": {
    "titlebar": {
      "frame_padding_x": 8.0,
      "frame_padding_y": 10.0,
      "title_left_margin": 10.0,
      "button_width": 36.0,
      "button_height": 30.0,
      "button_spacing": 3.0,
      "button_right_margin": 10.0,
      "button_count": 4,
      "button_area_width": 150.0
    }
  }
}
```

## 使用方式

### 1. 通过配置文件修改

直接编辑 `demo_config.json` 文件，然后重启程序：

```json
{
  "ui": {
    "titlebar": {
      "frame_padding_y": 15.0  // 增加标题栏高度
    }
  }
}
```

### 2. 运行时通过代码修改

```cpp
auto& config = Config::ConfigManager::instance();

// 修改标题栏高度
config.set("ui.titlebar.frame_padding_y", 15.0);

// 修改按钮大小
config.set("ui.titlebar.button_width", 40.0);
config.set("ui.titlebar.button_height", 35.0);

// 保存到文件
config.save_to_file("demo_config.json");

// 重新加载配置
m_title_bar_config.load_from_manager();
```

### 3. 读取配置

```cpp
auto& config = Config::ConfigManager::instance();

// 获取配置值
auto padding = config.get_or<double>("ui.titlebar.frame_padding_y", 10.0);
auto button_width = config.get_or<double>("ui.titlebar.button_width", 36.0);
```

## 配置注册

配置在 `setup_config()` 函数中注册（main.cpp:480-536）：

```cpp
// 注册配置元数据
config.register_meta("ui.titlebar.frame_padding_y", {
    .description = "Title bar vertical padding (controls height)",
    .default_value = 10.0,
    .is_required = false
});

// ... 注册其他配置

// 加载配置到本地结构
m_title_bar_config.load_from_manager();
```

## 计算属性

`TitleBarConfig` 结构体提供了便捷的计算方法：

```cpp
// 获取标题栏高度
float height = m_title_bar_config.get_title_bar_height();

// 获取按钮总宽度
float total_width = m_title_bar_config.get_buttons_total_width();

// 获取按钮起始 X 坐标
float start_x = m_title_bar_config.get_buttons_start_x(window_width);

// 检查是否在按钮区域
bool in_button_area = m_title_bar_config.is_in_button_area(x, window_width);
```

## 快速调整指南

### 增加标题栏高度

**方法 1 - 修改配置文件**:
```json
{
  "ui": {
    "titlebar": {
      "frame_padding_y": 15.0
    }
  }
}
```

**方法 2 - 代码修改**:
```cpp
config.set("ui.titlebar.frame_padding_y", 15.0);
config.save_to_file("demo_config.json");
```

**自动影响**:
- ✅ MenuBar 渲染高度
- ✅ 拖动检测区域
- ✅ DockSpace 位置计算

### 调整按钮大小

```json
{
  "ui": {
    "titlebar": {
      "button_width": 40.0,
      "button_height": 35.0
    }
  }
}
```

### 调整按钮间距

```json
{
  "ui": {
    "titlebar": {
      "button_spacing": 5.0
    }
  }
}
```

### 调整标题位置

```json
{
  "ui": {
    "titlebar": {
      "title_left_margin": 20.0
    }
  }
}
```

## 工作流程

### 启动时

1. **加载配置文件**: `ConfigManager::instance().load_from_file("demo_config.json")`
2. **注册配置元数据**: 为每个配置项设置默认值和描述
3. **加载到内存**: `m_title_bar_config.load_from_manager()` 从 ConfigManager 读取配置

### 运行时修改

1. **修改配置**: `config.set("ui.titlebar.frame_padding_y", 15.0)`
2. **重新加载**: `m_title_bar_config.load_from_manager()`
3. **立即生效**: 下一帧渲染时使用新配置

### 退出时

1. **保存配置**: `config.save_to_file("demo_config.json")`
2. **持久化**: 配置写入文件，下次启动时自动加载

## 架构优势

### 与之前的区别

**之前** (静态结构体):
```cpp
struct TitleBarConfig {
    float frame_padding_y = 10.0;  // 硬编码，需要重新编译
};
```

**现在** (ConfigManager):
```cpp
// 从配置文件读取，无需重新编译
config.set("ui.titlebar.frame_padding_y", 15.0);
```

### 主要优势

| 特性 | 之前 | 现在 |
|------|------|------|
| 修改方式 | 修改代码，重新编译 | 修改配置文件或运行时修改 |
| 持久化 | ❌ | ✅ 保存到 JSON |
| 运行时修改 | ❌ | ✅ 动态调整 |
| 用户自定义 | ❌ | ✅ 用户可编辑配置文件 |
| 热重载 | ❌ | ✅ 支持配置热重载 |
| 变更回调 | ❌ | ✅ 支持配置变更回调 |
| 类型安全 | ✅ | ✅ |

## 注意事项

1. **修改配置文件后需要重启程序**，或者在代码中调用 `load_from_manager()` 重新加载
2. **配置键名区分大小写**：`ui.titlebar.frame_padding_y` 而不是 `UI.TITLEBAR.FRAME_PADDING_Y`
3. **类型必须匹配**：double 类型使用浮点数，int 类型使用整数
4. **修改后建议保存**：使用 `config.save_to_file("demo_config.json")` 保存更改

## 扩展：添加变更回调

可以监听配置变更，在配置改变时执行自定义逻辑：

```cpp
config.register_meta("ui.titlebar.frame_padding_y", {
    .description = "Title bar vertical padding",
    .default_value = 10.0,
    .is_required = false,
    .change_callback = [](const Config::ConfigValue& value) {
        if (std::holds_alternative<double>(value)) {
            double padding = std::get<double>(value);
            LOG_INFO("标题栏高度已修改为: {}", padding);
            // 执行其他逻辑，如通知 UI 刷新
        }
    }
});
```

## 总结

使用 ConfigManager 管理标题栏配置提供了：

✅ **更好的用户体验**：用户可以通过配置文件自定义界面
✅ **开发更方便**：无需重新编译即可调整界面
✅ **更符合框架架构**：统一使用 ConfigManager 管理所有配置
✅ **易于扩展**：可以轻松添加验证、回调等功能
✅ **生产环境友好**：支持配置持久化和运行时调整
