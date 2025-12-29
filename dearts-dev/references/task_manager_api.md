# TaskManager API 完全手册

## 概述

TaskManager 是 DearTs Framework 的异步任务管理系统，提供线程安全、进度跟踪、任务取消等功能。

**核心特性：**
- ✅ **异步执行** - 在后台线程执行耗时操作
- ✅ **进度跟踪** - 实时更新任务进度
- ✅ **任务取消** - 支持中途取消正在执行的任务
- ✅ **多种任务类型** - Normal, Background, Blocking, Critical
- ✅ **线程安全** - 自动管理线程生命周期
- ✅ **状态管理** - Pending, Running, Completed, Cancelled, Failed
- ✅ **完成回调** - 任务完成时触发回调

---

## 快速开始

### 基本用法

```cpp
#include "core/tasks/task_manager.h"

using namespace DearTs::Core::Tasks;

// 创建并启动任务
auto task = TaskManager::instance().launch(
    "加载文件",
    [](const std::atomic<bool>& should_cancel) {
        for (int i = 0; i < 100; i++) {
            if (should_cancel) {
                LOG_INFO("任务被取消");
                return;
            }

            // 执行工作...
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
);

// 检查状态
if (task->is_running()) {
    LOG_INFO("任务正在运行...");
}
```

---

## 核心概念

### 任务类型 (TaskType)

```cpp
enum class TaskType {
    Normal,     // 普通任务 - 异步执行（默认）
    Background, // 后台任务 - 不影响 UI
    Blocking,   // 阻塞任务 - 在当前线程执行
    Critical    // 关键任务 - 高优先级，不会被自动清理
};
```

**使用场景：**
- `Normal` - 大部分异步任务
- `Background` - 不需要用户关心的后台任务
- `Blocking` - 必须立即完成的关键操作
- `Critical` - 不能被清理的重要任务

### 任务状态 (TaskStatus)

```
Pending    →  Running  →  Completed
    ↓            ↓
Cancelled ←─────┘
    ↓
Failed
```

---

## TaskManager API

### 1. 获取单例

```cpp
TaskManager& TaskManager::instance()
```

**示例：**
```cpp
auto& tm = TaskManager::instance();
```

---

### 2. 创建任务（不立即执行）

```cpp
std::shared_ptr<Task> create(
    const std::string& name,
    Task::TaskFunc func,
    TaskType type = TaskType::Normal
)
```

**参数：**
- `name` - 任务名称
- `func` - 任务函数（签名为 `void(const std::atomic<bool>&)`）
- `type` - 任务类型（默认 Normal）

**返回：**
- `std::shared_ptr<Task>` - 任务对象

**示例：**
```cpp
auto task = TaskManager::instance().create(
    "处理数据",
    [](const std::atomic<bool>& should_cancel) {
        // 任务逻辑
    },
    TaskType::Background
);

// 稍后启动
TaskManager::instance().start(task);
```

---

### 3. 创建并立即启动任务

```cpp
std::shared_ptr<Task> launch(
    const std::string& name,
    Task::TaskFunc func,
    TaskType type = TaskType::Normal
)
```

**参数：** 同 `create()`

**示例：**
```cpp
auto task = TaskManager::instance().launch(
    "保存文件",
    [](const std::atomic<bool>& should_cancel) {
        save_file();
    }
);

// 任务已经启动
```

---

### 4. 启动任务

```cpp
void start(std::shared_ptr<Task> task)
```

**示例：**
```cpp
auto task = TaskManager::instance().create("工作", work_func);
// ... 稍后启动
TaskManager::instance().start(task);
```

---

### 5. 取消任务

```cpp
void cancel(std::shared_ptr<Task> task)
```

**示例：**
```cpp
auto task = TaskManager::instance().launch("长时间任务", ...);

// 用户点击取消按钮
if (ImGui::Button("取消")) {
    TaskManager::instance().cancel(task);
}
```

---

### 6. 获取所有任务

```cpp
const std::vector<std::shared_ptr<Task>>& getTasks() const
```

**示例：**
```cpp
auto tasks = TaskManager::instance().getTasks();
for (const auto& task : tasks) {
    LOG_INFO("Task: {}", task->getName());
}
```

---

### 7. 获取正在运行的任务

```cpp
std::vector<std::shared_ptr<Task>> getRunningTasks() const
```

**示例：**
```cpp
auto running = TaskManager::instance().getRunningTasks();
LOG_INFO("Running tasks: {}", running.size());
```

---

### 8. 等待所有任务完成

```cpp
void waitForAll()
```

**示例：**
```cpp
// 应用关闭前等待所有任务完成
TaskManager::instance().waitForAll();
```

---

### 9. 取消所有任务

```cpp
void cancelAll()
```

**示例：**
```cpp
// 应用关闭时取消所有任务
TaskManager::instance().cancelAll();
```

---

### 10. 获取运行中的任务数量

```cpp
size_t getRunningTaskCount() const
```

**示例：**
```cpp
size_t count = TaskManager::instance().getRunningTaskCount();
LOG_INFO("Running tasks: {}", count);
```

---

### 11. 更新任务状态（清理）

```cpp
void update()
```

**说明：** 在主循环中调用，清理已完成的任务

**示例：**
```cpp
void MyApp::on_update(double delta_time) override {
    // 清理已完成的任务
    TaskManager::instance().update();
}
```

---

## Task API

### 1. 获取任务信息

```cpp
const std::string& getName() const          // 任务名称
TaskType getType() const                    // 任务类型
TaskStatus getStatus() const                // 任务状态
float getProgress() const                   // 进度值（0 - max_progress）
float getMaxProgress() const                // 最大进度值
float getProgressPercent() const            // 进度百分比（0.0 - 1.0）
```

**示例：**
```cpp
LOG_INFO("Task: {}", task->getName());
LOG_INFO("Progress: {:.1f}%", task->getProgressPercent() * 100);
```

---

### 2. 更新进度

```cpp
void setProgress(float progress)            // 设置进度
void addProgress(float delta)               // 增加进度
```

**示例：**
```cpp
// 在任务函数中更新进度
for (int i = 0; i < 100; i++) {
    // ... 处理 ...

    // 更新进度
    float progress = (i + 1) * 100.0f / 100;
    task->setProgress(progress);

    // 或者
    task->addProgress(1.0f);
}
```

---

### 3. 检查状态

```cpp
bool shouldCancel() const                   // 是否应该取消
bool isRunning() const                      // 是否正在运行
bool isFinished() const                     // 是否已完成
```

**示例：**
```cpp
if (task->shouldCancel()) {
    cleanup();
    return;
}
```

---

### 4. 取消任务

```cpp
void cancel()
```

**示例：**
```cpp
task->cancel();
```

---

### 5. 设置完成回调

```cpp
void onCompleted(std::function<void()> callback)
```

**示例：**
```cpp
auto task = TaskManager::instance().launch("导出数据", ...);

task->onCompleted([]() {
    LOG_INFO("导出完成！");
    // 显示通知
    ImGui::OpenPopup("导出完成");
});
```

---

## 实际应用示例

### 示例 1：文件加载

```cpp
class FileLoader {
public:
    std::shared_ptr<Task> load_file(const std::string& path) {
        return TaskManager::instance().launch(
            std::format("加载: {}", path),
            [this, path](const std::atomic<bool>& should_cancel) {
                auto& tm = TaskManager::instance();
                auto tasks = tm.getTasks();
                auto task = tasks.back();

                // 读取文件大小
                size_t file_size = get_file_size(path);

                // 打开文件
                std::ifstream file(path, std::ios::binary);
                if (!file) {
                    LOG_ERROR("无法打开文件: {}", path);
                    return;
                }

                // 分块读取
                const size_t chunk_size = 4096;
                std::vector<uint8_t> buffer(chunk_size);
                size_t total_read = 0;

                while (file && !should_cancel) {
                    file.read(reinterpret_cast<char*>(buffer.data()), chunk_size);
                    size_t read = file.gcount();

                    if (read > 0) {
                        // 处理数据
                        process_data(buffer.data(), read);

                        total_read += read;

                        // 更新进度
                        float progress = static_cast<float>(total_read) /
                                        static_cast<float>(file_size) * 100.0f;
                        task->setProgress(progress);
                    }
                }

                if (should_cancel) {
                    LOG_INFO("文件加载被取消");
                } else {
                    LOG_INFO("文件加载完成: {} bytes", total_read);
                }
            }
        );
    }

private:
    size_t get_file_size(const std::string& path) {
        std::filesystem::path p(path);
        return std::filesystem::file_size(p);
    }

    void process_data(uint8_t* data, size_t size) {
        // 处理数据...
    }
};
```

---

### 示例 2：ImGui 进度显示

```cpp
class TaskProgressView : public View {
public:
    void draw_content() override {
        ImGui::Text("任务进度");

        auto tasks = TaskManager::instance().getTasks();

        for (const auto& task : tasks) {
            if (task->isRunning() || task->getStatus() == TaskStatus::Pending) {
                // 任务名称
                ImGui::Text("%s", task->getName().c_str());

                // 进度条
                float progress = task->getProgressPercent();
                ImGui::ProgressBar(progress, ImVec2(200, 0));

                // 百分比
                ImGui::SameLine();
                ImGui::Text("%.1f%%", progress * 100);

                // 取消按钮
                if (ImGui::Button(std::format("取消##{}", task->getName()).c_str())) {
                    TaskManager::instance().cancel(task);
                }

                ImGui::Separator();
            }
        }

        // 统计
        auto running = TaskManager::instance().getRunningTasks();
        ImGui::Text("运行中的任务: %zu", running.size());
    }
};
```

---

### 示例 3：批量处理

```cpp
class BatchProcessor {
public:
    std::shared_ptr<Task> process_files(const std::vector<std::string>& files) {
        return TaskManager::instance().launch(
            "批量处理文件",
            [this, files](const std::atomic<bool>& should_cancel) {
                auto& tm = TaskManager::instance();
                auto tasks = tm.getTasks();
                auto task = tasks.back();

                size_t total = files.size();

                for (size_t i = 0; i < files.size(); i++) {
                    if (should_cancel) {
                        LOG_INFO("批量处理被取消");
                        break;
                    }

                    // 处理单个文件
                    process_file(files[i]);

                    // 更新进度
                    float progress = static_cast<float>(i + 1) /
                                    static_cast<float>(total) * 100.0f;
                    task->setProgress(progress);

                    LOG_INFO("[{}/{}] 处理完成: {}",
                        i + 1, total, files[i]);
                }

                LOG_INFO("批量处理完成");
            },
            TaskType::Background
        );
    }

private:
    void process_file(const std::string& path) {
        // 处理文件...
    }
};
```

---

### 示例 4：带完成回调的任务

```cpp
class DataExporter {
public:
    void export_data() {
        auto task = TaskManager::instance().launch(
            "导出数据",
            [this](const std::atomic<bool>& should_cancel) {
                // 导出逻辑
                std::vector<Data> data = collect_data();

                for (size_t i = 0; i < data.size(); i++) {
                    if (should_cancel) return;

                    write_data(data[i]);
                    m_task->addProgress(100.0f / data.size());
                }
            }
        );

        // 设置完成回调
        task->onCompleted([this]() {
            m_export_complete = true;

            // 在主线程中显示通知
            if (ImGui::GetCurrentContext()) {
                ImGui::OpenPopup("导出完成");
            }
        });

        m_task = task;
    }

private:
    std::shared_ptr<Task> m_task;
    bool m_export_complete = false;
};
```

---

### 示例 5：任务管理器视图

```cpp
class TaskManagerView : public View {
public:
    void draw_content() override {
        ImGui::Text("任务管理器");
        ImGui::Separator();

        auto tasks = TaskManager::instance().getTasks();

        if (tasks.empty()) {
            ImGui::TextDisabled("无活动任务");
            return;
        }

        // 任务列表
        for (const auto& task : tasks) {
            draw_task(task);
            ImGui::Separator();
        }

        // 操作按钮
        if (ImGui::Button("取消所有")) {
            TaskManager::instance().cancelAll();
        }

        ImGui::SameLine();
        if (ImGui::Button("等待完成")) {
            TaskManager::instance().waitForAll();
        }
    }

private:
    void draw_task(const std::shared_ptr<Task>& task) {
        // 状态颜色
        ImColor color;
        const char* status_text = "";

        switch (task->getStatus()) {
            case TaskStatus::Pending:
                color = ImColor(255, 255, 0);
                status_text = "等待中";
                break;
            case TaskStatus::Running:
                color = ImColor(0, 150, 255);
                status_text = "运行中";
                break;
            case TaskStatus::Completed:
                color = ImColor(0, 255, 0);
                status_text = "已完成";
                break;
            case TaskStatus::Cancelled:
                color = ImColor(255, 128, 0);
                status_text = "已取消";
                break;
            case TaskStatus::Failed:
                color = ImColor(255, 0, 0);
                status_text = "失败";
                break;
        }

        // 任务名称和状态
        ImGui::TextColored(color, "%s", task->getName().c_str());
        ImGui::SameLine();
        ImGui::Text("[%s]", status_text);

        // 进度条
        if (task->isRunning()) {
            float progress = task->getProgressPercent();
            ImGui::ProgressBar(progress, ImVec2(-1, 0));
            ImGui::Text("%.1f%%", progress * 100);
        }

        // 取消按钮
        if (task->isRunning()) {
            if (ImGui::Button(std::format("取消##{}", task->getName()).c_str())) {
                TaskManager::instance().cancel(task);
            }
        }
    }
};
```

---

## 最佳实践

### ✅ DO

1. **检查取消标志**
   ```cpp
   [](const std::atomic<bool>& should_cancel) {
       for (int i = 0; i < 1000000; i++) {
           if (should_cancel) {
               cleanup();
               return;
           }
           do_work(i);
       }
   }
   ```

2. **更新进度**
   ```cpp
   for (int i = 0; i < total; i++) {
       process(i);
       task->setProgress((i + 1) * 100.0f / total);
   }
   ```

3. **使用完成回调**
   ```cpp
   task->onCompleted([]() {
       LOG_INFO("任务完成！");
       show_notification();
   });
   ```

4. **在主循环中更新**
   ```cpp
   void on_update(double delta_time) override {
       TaskManager::instance().update();  // 清理已完成任务
   }
   ```

### ❌ DON'T

1. **不要忘记检查取消**
   ```cpp
   // ❌ 不好 - 无法取消
   [](const auto&) {
       for (int i = 0; i < 1000000; i++) {
           do_work(i);  // 无法中断！
       }
   }

   // ✅ 好
   [](const auto& should_cancel) {
       for (int i = 0; i < 1000000; i++) {
           if (should_cancel) return;
           do_work(i);
       }
   }
   ```

2. **不要在阻塞任务中执行耗时操作**
   ```cpp
   // ❌ 不好 - 阻塞 UI
   auto task = TaskManager::instance().launch("工作", []{
       std::this_thread::sleep_for(std::chrono::seconds(10));
   }, TaskType::Blocking);  // 会阻塞 UI！

   // ✅ 好 - 使用异步
   auto task = TaskManager::instance().launch("工作", []{
       std::this_thread::sleep_for(std::chrono::seconds(10));
   }, TaskType::Normal);
   ```

3. **不要忘记清理**
   ```cpp
   // ❌ 不好 - 任务会累积
   // 不调用 update()

   // ✅ 好 - 定期清理
   void on_update(double delta_time) override {
       TaskManager::instance().update();
   }
   ```

---

## 完整应用示例

```cpp
#include "core/tasks/task_manager.h"
#include "core/app/application.h"

class MyApp : public Application {
private:
    std::shared_ptr<Task> m_loading_task;
    std::shared_ptr<Task> m_export_task;

public:
    bool on_init() override {
        // 启动初始加载任务
        m_loading_task = load_resources();

        // 设置完成回调
        m_loading_task->onCompleted([]() {
            LOG_INFO("资源加载完成，应用就绪");
        });

        return true;
    }

    void on_update(double delta_time) override {
        // 清理已完成任务
        TaskManager::instance().update();
    }

    void on_render() override {
        // 显示任务进度
        draw_task_progress();

        // 导出按钮
        if (ImGui::Button("导出数据")) {
            m_export_task = export_data();
        }
    }

    void on_shutdown() override {
        // 等待所有任务完成
        LOG_INFO("等待任务完成...");
        TaskManager::instance().waitForAll();
        LOG_INFO("所有任务已完成");
    }

private:
    std::shared_ptr<Task> load_resources() {
        return TaskManager::instance().launch(
            "加载资源",
            [](const std::atomic<bool>& should_cancel) {
                auto& tm = TaskManager::instance();
                auto tasks = tm.getTasks();
                auto task = tasks.back();

                // 加载纹理
                load_textures(should_cancel, task);
                if (should_cancel) return;

                task->addProgress(30.0f);

                // 加载着色器
                load_shaders(should_cancel, task);
                if (should_cancel) return;

                task->addProgress(30.0f);

                // 加载模型
                load_models(should_cancel, task);
                if (should_cancel) return;

                task->addProgress(40.0f);

                LOG_INFO("资源加载完成");
            },
            TaskType::Critical  // 关键任务
        );
    }

    std::shared_ptr<Task> export_data() {
        return TaskManager::instance().launch(
            "导出数据",
            [](const std::atomic<bool>& should_cancel) {
                auto& tm = TaskManager::instance();
                auto tasks = tm.getTasks();
                auto task = tasks.back();

                // 收集数据
                std::vector<Data> data = collect_data();

                // 写入文件
                for (size_t i = 0; i < data.size(); i++) {
                    if (should_cancel) return;

                    write_data(data[i]);
                    task->addProgress(100.0f / data.size());
                }

                LOG_INFO("导出完成");
            }
        );
    }

    void draw_task_progress() {
        ImGui::Begin("任务进度");

        auto running = TaskManager::instance().getRunningTasks();
        for (const auto& task : running) {
            ImGui::Text("%s", task->getName().c_str());

            float progress = task->getProgressPercent();
            ImGui::ProgressBar(progress, ImVec2(200, 0));

            ImGui::SameLine();
            ImGui::Text("%.1f%%", progress * 100);

            if (ImGui::Button("取消")) {
                TaskManager::instance().cancel(task);
            }

            ImGui::Separator();
        }

        ImGui::End();
    }
};
```

---

## API 快速参考

### TaskManager 方法

| 方法 | 说明 |
|------|------|
| `instance()` | 获取单例 |
| `create(name, func, type)` | 创建任务 |
| `launch(name, func, type)` | 创建并启动任务 |
| `start(task)` | 启动任务 |
| `cancel(task)` | 取消任务 |
| `cancelAll()` | 取消所有任务 |
| `waitForAll()` | 等待所有完成 |
| `getTasks()` | 获取所有任务 |
| `getRunningTasks()` | 获取运行中的任务 |
| `getRunningTaskCount()` | 获取运行任务数量 |
| `update()` | 更新（清理已完成任务） |

### Task 方法

| 方法 | 说明 |
|------|------|
| `getName()` | 获取名称 |
| `getType()` | 获取类型 |
| `getStatus()` | 获取状态 |
| `getProgress()` | 获取进度值 |
| `getProgressPercent()` | 获取进度百分比 |
| `setProgress(progress)` | 设置进度 |
| `addProgress(delta)` | 增加进度 |
| `shouldCancel()` | 是否应该取消 |
| `isRunning()` | 是否运行中 |
| `isFinished()` | 是否已完成 |
| `cancel()` | 取消任务 |
| `onCompleted(callback)` | 设置完成回调 |

---

**文件**: `core/tasks/task_manager.h`
**源码**: `core/tasks/task_manager.cpp`
**相关**: Result 类型, 日志系统
