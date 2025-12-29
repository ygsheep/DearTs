# 文件对话框与任务系统集成

## 概述

文件对话框系统现在已完全集成到任务系统中，提供高效的异步文件处理，特别适合大文件操作。

## ✅ 主要改进

### 1. **与 TaskManager 集成**
```cpp
// 使用便捷函数
auto task = open_and_read_single_file(
    "打开日志文件",
    FileFilter("日志文件", "*.log"),
    [](const FileReadResult& result) {
        if (result.success) {
            LOG_INFO("Loaded {} bytes", result.file_size);
            // 处理文件内容...
        }
    },
    [](const std::string& error) {
        LOG_ERROR("Failed: {}", error);
    }
);

// 任务可以取消
TaskManager::instance().cancel(task);
```

### 2. **文件读取函数**

#### 同步读取
```cpp
// 读取整个文件
auto result = read_file("data.log", 100);  // 最大 100MB
if (result.success) {
    std::cout << result.content << std::endl;
}

// 逐行读取（适合文本文件）
auto result = read_file_lines("data.log", 10000);  // 最多 10000 行
```

#### 异步读取（推荐）
```cpp
// 异步读取文件
auto task = read_file_async(
    "large_file.log",
    [](const FileReadResult& result) {
        // 在 UI 线程调用
        LOG_INFO("File loaded: {} bytes", result.file_size);
        process_data(result.content);
    },
    [](const std::string& error) {
        LOG_ERROR("Error: {}", error);
    },
    [](size_t read, size_t total, float progress) {
        // 进度回调（每 4MB 调用一次）
        LOG_INFO("Progress: {}/{} bytes ({:.1f}%)", read, total, progress * 100);
    }
);
```

### 3. **高级功能**

#### 打开并读取文件（一步完成）
```cpp
using namespace Core::Utils;

// 在主线程显示对话框，后台读取文件
auto task = FileDialog::open_and_read_file(
    FileDialogOptions{
        .title = "选择日志文件",
        .filters = { FileFilter("日志文件", "*.log") }
    },
    [](const FileReadResult& result) {
        if (result.success) {
            LOG_INFO("Loaded {} from file", result.file_size);
            // 自动在 UI 线程调用，可以安全更新 UI
        }
    }
);
```

#### 批量读取多个文件
```cpp
FileDialogOptions options;
options.title = "选择多个日志文件";
options.filters = { FileFilter("日志文件", "*.log") };
options.allow_multiple = true;

auto task = FileDialog::open_and_read_files(
    options,
    [](const std::vector<FileReadResult>& results) {
        LOG_INFO("Loaded {} files", results.size());
        for (const auto& result : results) {
            LOG_INFO("  - {}: {} bytes", result.path.filename().string(), result.file_size);
        }
    }
);
```

### 4. **大文件处理特性**

#### 自动分块读取
- **块大小**: 4MB
- **进度更新**: 每块完成后触发
- **可取消**: 随时中断读取操作

#### 内存保护
```cpp
// 自动检查文件大小限制
auto result = read_file("huge.log", 500);  // 500 MB 限制

if (!result.success) {
    // 文件过大时的自动错误处理
    LOG_ERROR("{}", result.error_message);
    // 输出: "文件过大 (1024.5 MB)，超过限制 (500 MB)"
}
```

#### 行数限制
```cpp
// 只读取前 10000 行
auto result = read_file_lines("large.log", 10000);
```

## 📊 性能特性

### 1. **后台任务**
- ✅ 使用 `Tasks::TaskType::Background`
- ✅ 不阻塞 UI 线程
- ✅ 支持并发文件操作

### 2. **进度跟踪**
```cpp
// 实时进度更新
FileProgressCallback progress = [](size_t read, size_t total, float percent) {
    // 在 UI 线程安全地更新进度条
    ImGui::ProgressBar(percent, ImVec2(200, 0));
    ImGui::SameLine();
    ImGui::Text("%.1f%%", percent * 100);
};
```

### 3. **取消支持**
```cpp
auto task = read_file_async("huge.log", on_success, on_error);

// 用户点击"取消"按钮
if (ImGui::Button("取消")) {
    TaskManager::instance().cancel(task);
}
```

## 🎯 日志查看器集成示例

### 改进日志查看器使用异步加载

```cpp
class LoggerViewerView : public Core::UI::ViewWindow {
private:
    std::shared_ptr<Tasks::Task> m_load_task;

    void browse_and_load_log_async() {
        using namespace Core::Utils;

        // 配置文件对话框
        FileDialogOptions options;
        options.title = "选择日志文件";
        options.filters = {
            FileFilter("日志文件", "*.log"),
            FileFilter("文本文件", "*.txt"),
            FileFilter("所有文件", "*.*")
        };

        // 异步打开并读取文件
        m_load_task = FileDialog::open_and_read_file(
            options,
            [this](const FileReadResult& result) {
                // 成功回调（在 UI 线程）
                if (result.success) {
                    // 解析日志内容
                    parse_log_content(result.content);

                    // 更新 UI
                    m_log_content = std::move(result.content);
                    m_current_file = result.path;
                    m_file_size = result.file_size;
                }
            },
            [this](const std::string& error) {
                // 错误回调（在 UI 线程）
                LOG_ERROR("Failed to load log: {}", error);
                show_error_dialog(error);
            },
            [this](size_t read, size_t total, float progress) {
                // 进度回调（在 UI 线程）
                m_load_progress = progress;
                m_load_status = std::format("加载中... {:.1f}%", progress * 100);
            }
        );
    }

    void draw_content() override {
        // 绘制加载进度
        if (m_load_task && m_load_task->isRunning()) {
            ImGui::Text("%s", m_load_status.c_str());
            ImGui::ProgressBar(m_load_progress);

            if (ImGui::Button("取消")) {
                TaskManager::instance().cancel(m_load_task);
                m_load_task.reset();
            }
        }

        // 绘制日志内容...
    }
};
```

## 🔧 API 快速参考

### 同步方法（简单但阻塞）
```cpp
// 打开文件
auto path = open_single_file("打开", FileFilter("文本", "*.txt"));

// 读取文件
auto result = read_file(path, 100);  // 100 MB 限制
if (result) { /* 使用 result.content */ }
```

### 异步方法（推荐，不阻塞 UI）
```cpp
// 打开并读取
auto task = FileDialog::open_and_read_file(options, on_success, on_error);

// 只读取已知文件
auto task = read_file_async(path, on_success, on_error, progress_cb);

// 读取行
auto task = read_file_lines_async(path, on_success, on_error, progress_cb, max_lines);
```

## 📝 注意事项

1. **文件对话框必须在主线程显示**（Windows COM 要求）
2. **文件读取在后台线程执行**（不阻塞 UI）
3. **回调在主线程调用**（可安全更新 UI）
4. **始终检查 `should_cancel`**（尊重用户取消操作）
5. **设置合理的文件大小限制**（避免内存耗尽）

## 🚀 使用建议

### 小文件（< 10MB）
```cpp
// 使用同步方法即可
auto result = read_file("small.log");
process(result.content);
```

### 大文件（> 10MB）
```cpp
// 必须使用异步方法
auto task = read_file_async("large.log", on_success, on_error, progress_callback);
```

### 多个文件
```cpp
// 使用批量方法
auto task = FileDialog::open_and_read_files(options, on_success);
```

---

**DearTs Framework** - 现代化的 C++20 应用开发框架
