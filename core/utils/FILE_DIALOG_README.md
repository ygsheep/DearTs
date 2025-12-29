# 文件对话框系统 (File Dialog System)

## 概述

DearTs 框架提供了一个跨平台的文件对话框系统，使用原生 Windows API 实现，提供美观且用户友好的文件选择体验。

## 特性

✅ **Windows 原生对话框** - 使用 IFileOpenDialog/IFileSaveDialog COM 接口
✅ **同步和异步支持** - 支持阻塞和非阻塞模式
✅ **文件过滤器** - 自定义文件类型过滤
✅ **单选和多选** - 支持选择单个或多个文件
✅ **文件夹选择** - 支持文件夹浏览
✅ **默认路径** - 记住上次打开位置
✅ **错误处理** - 完善的错误信息反馈

## 快速使用

### 1. 打开单个文件（最简单）

```cpp
#include "core/utils/file_dialog.hpp"

using namespace DearTs::Core::Utils;

// 打开文件
auto path = open_single_file("打开文件", FileFilter("文本文件", "*.txt"));

if (!path.empty()) {
    LOG_INFO("Selected: {}", path.string());
}
```

### 2. 保存文件

```cpp
auto path = save_single_file("保存文件", FileFilter("日志文件", "*.log"));

if (!path.empty()) {
    // 保存文件...
}
```

### 3. 选择文件夹

```cpp
auto folder = select_single_folder("选择输出文件夹");

if (!folder.empty()) {
    LOG_INFO("Output folder: {}", folder.string());
}
```

### 4. 高级用法（自定义选项）

```cpp
FileDialogOptions options;
options.title = "选择日志文件";
options.filters = {
    FileFilter("日志文件", "*.log"),
    FileFilter("文本文件", "*.txt"),
    FileFilter("所有文件", "*.*")
};
options.default_path = "build/bin/logs";
options.allow_multiple = true;  // 允许多选

auto result = FileDialog::open_file(options);

if (result.success) {
    for (const auto& path : result.paths) {
        LOG_INFO("Selected: {}", path.string());
    }
}
```

### 5. 异步文件对话框（不阻塞UI）

```cpp
FileDialogOptions options;
options.title = "打开大文件";
options.filters = { FileFilter("数据文件", "*.dat") };

FileDialog::open_file_async(options, [](const FileDialogResult& result) {
    if (result.success) {
        LOG_INFO("File selected: {}", result.get_first_path().string());
        // 在后台加载文件...
    }
});

// UI 继续响应，不会被阻塞
```

## 在日志查看器中的使用

日志查看器集成了文件对话框系统：

1. **点击"浏览文件..."按钮** - 打开文件选择对话框
2. **选择日志文件** - 支持过滤 `.log` 和 `.txt` 文件
3. **自动加载** - 选择后自动加载并显示日志内容
4. **智能管理** - 新选择的文件会自动添加到下拉列表中

## API 参考

### FileFilter

```cpp
FileFilter(const std::string& name, const std::string& extension);
```

- **name**: 显示名称，如 "文本文件"
- **extension**: 文件扩展名，如 "*.txt" 或 "*.log;*.txt"

### FileDialogOptions

```cpp
struct FileDialogOptions {
    std::string title;                      // 对话框标题
    std::vector<FileFilter> filters;        // 文件过滤器
    std::filesystem::path default_path;     // 默认路径
    bool allow_multiple = false;            // 允许多选
    bool must_exist = true;                 // 文件必须存在
};
```

### FileDialogResult

```cpp
struct FileDialogResult {
    bool success = false;                   // 是否成功
    std::vector<std::filesystem::path> paths;  // 选择的文件路径
    std::string error_message;              // 错误信息

    std::filesystem::path get_first_path() const;  // 获取第一个路径
    bool has_paths() const;                      // 是否选择了文件
};
```

### FileDialog 类

```cpp
class FileDialog {
public:
    // 同步方法
    static FileDialogResult open_file(const FileDialogOptions& options);
    static FileDialogResult save_file(const FileDialogOptions& options);
    static FileDialogResult select_folder(const FileDialogOptions& options);

    // 异步方法（带回调）
    static void open_file_async(
        const FileDialogOptions& options,
        std::function<void(const FileDialogResult&)> callback
    );
    static void save_file_async(...);
    static void select_folder_async(...);
};
```

## 实现细节

### Windows 实现

- 使用 **COM IFileOpenDialog** 和 **IFileSaveDialog** 接口
- 自动 COM 初始化和清理
- 智能指针管理 COM 对象生命周期
- Unicode 支持（UTF-16 转换）

### 线程安全

- 同步方法：在调用线程中执行（阻塞）
- 异步方法：在后台线程中执行（不阻塞 UI）
- COM 初始化每个线程独立处理

## 错误处理

所有错误都会被捕获并返回：

```cpp
auto result = FileDialog::open_file(options);

if (!result.success && !result.error_message.empty()) {
    LOG_ERROR("Failed to open file: {}", result.error_message);
    // 显示错误消息给用户
}
```

## 最佳实践

1. **设置合理的默认路径** - 提升用户体验
2. **提供合适的文件过滤器** - 帮助用户快速定位文件
3. **使用异步方法** - 避免阻塞 UI 线程
4. **检查返回结果** - 处理用户取消和错误情况
5. **记录文件路径** - 便于下次打开时回到相同位置

## 示例：在插件中使用

```cpp
class MyPluginView : public Core::UI::ViewWindow {
public:
    void draw_content() override {
        if (ImGui::Button("打开文件")) {
            open_file_via_dialog();
        }

        // 显示当前文件
        if (!m_current_file.empty()) {
            ImGui::Text("当前文件: %s", m_current_file.string().c_str());
        }
    }

private:
    void open_file_via_dialog() {
        using namespace Core::Utils;

        auto path = open_single_file(
            "打开数据文件",
            FileFilter("CSV 文件", "*.csv")
        );

        if (!path.empty()) {
            m_current_file = path;
            load_and_process_file(path);
        }
    }

    std::filesystem::path m_current_file;
};
```

## 故障排除

### COM 初始化失败

确保 Windows COM 系统正常运行：

```cpp
// 每个线程首次使用时自动初始化
ComInitializer com_init;  // RAII 自动清理
```

### 文件对话框不显示

- 检查是否在 UI 线程中调用（同步方法需要）
- 尝试使用异步方法
- 检查 Windows 版本兼容性（Vista+）

### 路径显示乱码

确保使用持久的字符串变量：

```cpp
// ❌ 错误：临时对象
const char* preview = path.string().c_str();

// ✅ 正确：持久化字符串
static std::string preview_str;
preview_str = path.string();
const char* preview = preview_str.c_str();
```

## 未来改进

- [ ] Linux 实现（使用 GTK/zenity）
- [ ] macOS 实现（使用 NSOpenPanel/NSSavePanel）
- [ ] 最近文件列表
- [ ] 文件预览功能
- [ ] 更多自定义选项

## 相关文件

- `core/utils/file_dialog.hpp` - 头文件
- `core/utils/file_dialog.cpp` - 实现文件
- `plugins/logger_viewer/` - 使用示例

---

**DearTs Framework** - 现代化的 C++20 应用开发框架
