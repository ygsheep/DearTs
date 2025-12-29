# Toast Notification Plugin - 使用指南

## 概述

Toast Notification 插件为 DearTs Framework 提供了一个优雅的气泡消息通知系统。支持多种消息类型、动画效果、可配置选项和堆叠显示。

## 特性

✅ **多种消息类型** - 信息（蓝色）、成功（绿色）、警告（橙色）、错误（红色）
✅ **优雅的动画** - 平滑的进入/退出动画效果
✅ **可配置时长** - 自定义每个消息的显示时间
✅ **堆叠显示** - 支持同时显示多个消息
✅ **现代化 UI** - 美观的卡片式设计，带进度条
✅ **交互功能** - 悬停暂停、点击关闭、关闭按钮
✅ **配置持久化** - 所有设置自动保存到配置文件

## 快速开始

### 1. 基本使用

```cpp
#include "toast_manager.hpp"

// 显示信息提示
DearTs::Plugins::Toast::show_info("提示", "这是一条信息");

// 显示成功提示
DearTs::Plugins::Toast::show_success("成功", "操作已成功完成");

// 显示警告提示
DearTs::Plugins::Toast::show_warning("警告", "请注意可能存在的问题");

// 显示错误提示
DearTs::Plugins::Toast::show_error("错误", "操作失败，请重试");

// 关闭所有 Toast
DearTs::Plugins::Toast::close_all_toasts();
```

### 2. 高级用法

```cpp
using namespace DearTs::Plugins::Toast;

// 自定义时长（5秒）
ToastManager::instance().show(
    "自定义标题",
    "这条消息将显示5秒",
    ToastType::Info,
    std::chrono::milliseconds(5000)
);

// 获取 Toast ID 并控制关闭
int toast_id = ToastManager::instance().success("保存成功", "文件已保存");

// 稍后手动关闭
ToastManager::instance().close(toast_id);
```

### 3. 配置选项

```cpp
auto& config = ToastManager::instance().get_config();

// 动画设置
config.animation_speed = 3.0f;          // 动画速度（1.0 - 10.0）
config.enter_duration = 0.3f;           // 进入动画时长（秒）
config.exit_duration = 0.2f;            // 退出动画时长（秒）

// 尺寸设置
config.max_width = 400.0f;              // 最大宽度
config.min_width = 300.0f;              // 最小宽度
config.padding_x = 16.0f;               // 水平内边距
config.padding_y = 12.0f;               // 垂直内边距
config.spacing = 8.0f;                  // Toast 之间的间距

// 显示设置
config.max_toasts = 5;                  // 最大同时显示数量
config.show_progress_bar = true;        // 显示进度条
config.show_close_button = true;        // 显示关闭按钮
config.pause_on_hover = true;           // 悬停时暂停计时
config.click_to_close = false;          // 点击是否关闭

// 批量配置
ToastManager::instance().configure([](auto& cfg) {
    cfg.animation_speed = 5.0f;
    cfg.max_toasts = 3;
    cfg.show_progress_bar = false;
});
```

### 4. 在插件中使用

```cpp
class MyPlugin : public Core::Plugin::IPlugin {
public:
    Result<void, std::string> on_load() override {
        // 注册命令
        Core::ContentRegistry::Commands::register_handler(
            "myplugin.save",
            "保存文件",
            [this]() {
                save_file();
            }
        );

        return Result::ok();
    }

private:
    void save_file() {
        try {
            // 保存逻辑...

            // 显示成功提示
            ToastManager::instance().success(
                "保存成功",
                "文件已成功保存到磁盘"
            );

        } catch (const std::exception& e) {
            // 显示错误提示
            ToastManager::instance().error(
                "保存失败",
                std::string("无法保存文件: ") + e.what()
            );
        }
    }
};
```

## API 参考

### ToastManager 单例

```cpp
// 获取实例
static ToastManager& instance();

// 显示 Toast
int show(const std::string& title, const std::string& message,
         ToastType type, std::chrono::milliseconds duration);

// 便捷方法
int info(const std::string& title, const std::string& message);
int success(const std::string& title, const std::string& message);
int warning(const std::string& title, const std::string& message);
int error(const std::string& title, const std::string& message);

// 关闭 Toast
void close(int id);
void close_all();

// 更新和渲染（由 Application 自动调用）
void update(float delta_time);
void render();

// 配置
ToastConfig& get_config();
void set_config(const ToastConfig& config);
void configure(ConfigCallback callback);
```

### ToastType 枚举

```cpp
enum class ToastType {
    Info,       // 信息提示（蓝色）
    Success,    // 成功提示（绿色）
    Warning,    // 警告提示（橙色）
    Error,      // 错误提示（红色）
    None        // 无类型（默认样式）
};
```

### ToastConfig 结构

```cpp
struct ToastConfig {
    float animation_speed = 3.0f;
    float enter_duration = 0.3f;
    float exit_duration = 0.2f;
    float max_width = 400.0f;
    float min_width = 300.0f;
    float padding_x = 16.0f;
    float padding_y = 12.0f;
    float spacing = 8.0f;
    int max_toasts = 5;
    bool show_progress_bar = true;
    bool show_close_button = true;
    bool pause_on_hover = true;
    bool click_to_close = false;
};
```

## 测试视图

插件提供了一个 "Toast Tester" 测试视图，可以通过以下方式打开：

1. 在菜单栏选择：View → Toast Tester
2. 或使用命令面板：Ctrl+P → 输入 "Toast Tester"

测试视图功能：
- 自定义标题和消息
- 选择消息类型
- 调整显示时长
- 预设消息按钮
- 配置选项调整
- 实时统计信息

## 已注册的命令

插件自动注册以下命令：

- `toast.info` - 显示信息提示
- `toast.success` - 显示成功提示
- `toast.warning` - 显示警告提示
- `toast.error` - 显示错误提示
- `toast.close_all` - 关闭所有提示

可以在命令调色板（Ctrl+P）中使用这些命令。

## 配置持久化

Toast 插件会自动保存配置到配置文件。配置在 `toast_notification` 命名空间下：

```json
{
  "toast_notification": {
    "animation_speed": 3.0,
    "enter_duration": 0.3,
    "exit_duration": 0.2,
    "max_width": 400.0,
    "min_width": 300.0,
    "padding_x": 16.0,
    "padding_y": 12.0,
    "spacing": 8.0,
    "max_toasts": 5,
    "show_progress_bar": true,
    "show_close_button": true,
    "pause_on_hover": true,
    "click_to_close": false
  }
}
```

## 最佳实践

### ✅ DO

1. **使用合适的类型**
   ```cpp
   // 成功操作
   ToastManager::instance().success("保存成功", "文件已保存");

   // 错误处理
   ToastManager::instance().error("加载失败", "文件不存在");
   ```

2. **提供清晰的消息**
   ```cpp
   // ✅ 好的消息
   ToastManager::instance().info("下载完成", "文件已保存到下载文件夹");

   // ❌ 模糊的消息
   ToastManager::instance().info("完成", "好了");
   ```

3. **合理的显示时长**
   ```cpp
   // 短消息 - 2-3秒
   ToastManager::instance().info("已复制", "文本已复制到剪贴板", 2000ms);

   // 长消息 - 5-10秒
   ToastManager::instance().warning("磁盘空间不足", "请清理磁盘空间", 8000ms);
   ```

### ❌ DON'T

1. **不要滥用 Toast**
   - 不要每秒都显示 Toast
   - 不要堆叠太多 Toast（超过 5 个）
   - 不要在 Toast 中显示大量文本

2. **不要阻塞操作**
   ```cpp
   // ❌ 错误：阻塞等待
   auto id = ToastManager::instance().info("处理中", "请稍候...");
   while (condition) { /* 等待 */ }

   // ✅ 正确：异步处理
   task = TaskManager::instance().launch("处理", [&]() {
       // 处理完成后
       ToastManager::instance().success("完成", "处理已完成");
   });
   ```

## 示例场景

### 1. 文件操作

```cpp
void save_file(const std::string& path) {
    try {
        // 保存逻辑...
        write_file(path, data);

        // 显示成功提示
        ToastManager::instance().success(
            "保存成功",
            "文件已保存: " + path
        );

    } catch (const std::exception& e) {
        // 显示错误提示
        ToastManager::instance().error(
            "保存失败",
            e.what()
        );
    }
}
```

### 2. 表单验证

```cpp
void submit_form() {
    if (username.empty()) {
        ToastManager::instance().warning(
            "验证失败",
            "请输入用户名"
        );
        return;
    }

    if (password.length() < 8) {
        ToastManager::instance().warning(
            "验证失败",
            "密码长度至少需要8位"
        );
        return;
    }

    // 提交表单...
    ToastManager::instance().success("提交成功", "表单已提交");
}
```

### 3. 异步任务

```cpp
void load_data_async() {
    // 显示开始提示
    ToastManager::instance().info("加载中", "正在加载数据...");

    auto task = TaskManager::instance().launch("加载数据", [&](auto& cancel) {
        try {
            auto data = fetch_data();

            // 成功完成
            ToastManager::instance().success(
                "加载完成",
                "成功加载 " + std::to_string(data.size()) + " 条数据"
            );

        } catch (const std::exception& e) {
            // 加载失败
            ToastManager::instance().error(
                "加载失败",
                e.what()
            );
        }
    });
}
```

## 技术细节

### 动画系统

- **进入动画**: 使用 `ease_out_cubic` 缓动函数，从右侧滑入并淡入
- **退出动画**: 淡出并向上移动，同样使用缓动效果
- **动画时长**: 可通过 `enter_duration` 和 `exit_duration` 配置

### 布局系统

- Toast 显示在屏幕右上角
- 自动堆叠，新的 Toast 在上方
- 每个Toast 的位置根据动画进度动态计算

### 性能优化

- 使用对象池模式管理 Toast
- 自动清理过期和完成的 Toast
- 线程安全的更新机制（使用 mutex）

## 故障排除

### Toast 不显示

**检查清单**:
1. 插件是否已加载？查看日志："ToastPlugin loaded successfully"
2. ToastManager::update() 和 render() 是否在主循环中调用？
3. 配置的 max_toasts 是否为 0？

### 动画卡顿

**解决方案**:
1. 降低 `animation_speed` 值
2. 减少 max_toasts 数量
3. 检查是否在主线程中调用

### 配置未保存

**检查**:
1. ConfigManager 是否正确初始化？
2. 是否调用了 `save_to_file()`？
3. 配置文件路径是否可写？

## 文件结构

```
plugins/toast_notification/
├── include/
│   ├── toast.hpp           # Toast 数据结构和类型
│   ├── toast_manager.hpp   # ToastManager 管理器
│   ├── toast_view.hpp      # Toast 测试视图
│   └── toast_plugin.hpp    # Toast 插件
├── source/
│   ├── toast_manager.cpp   # ToastManager 实现
│   └── toast_plugin.cpp    # Toast 插件实现
├── CMakeLists.txt          # 构建配置
└── README.md               # 本文档
```

## 版本历史

### v1.0.0 (当前版本)

- ✅ 基础 Toast 通知系统
- ✅ 四种消息类型（信息、成功、警告、错误）
- ✅ 优雅的进入/退出动画
- ✅ 可配置的显示选项
- ✅ 堆叠显示多个消息
- ✅ 进度条和关闭按钮
- ✅ 悬停暂停功能
- ✅ 配置持久化
- ✅ 测试视图
- ✅ 命令集成

## 贡献

欢迎提交 Issue 和 Pull Request！

## 许可证

MIT License - 详见项目根目录 LICENSE 文件
