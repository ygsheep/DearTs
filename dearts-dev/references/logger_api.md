# Logger API 完全手册

## 概述

Logger 是 DearTs Framework 的日志系统，提供高性能、线程安全的日志记录功能，支持控制台输出和异步文件写入。

**核心特性：**
- ✅ **线程安全** - 使用原子操作和互斥锁
- ✅ **异步写入** - 独立线程处理文件 I/O，不阻塞主线程
- ✅ **重复过滤** - 智能过滤短时间内的重复日志
- ✅ **格式化支持** - 使用 std::format 进行高效格式化
- ✅ **日志级别** - TRACE, DEBUG, INFO, WARN, ERROR, FATAL
- ✅ **文件输出** - 可选的异步文件日志
- ✅ **单例模式** - 全局唯一实例

---

## 快速开始

### 基本用法

```cpp
#include "liblogger/logger.h"

using namespace DearTs;

// 设置日志级别
Logger::get_instance().set_level(LogLevel::DEBUG);

// 启用文件输出
Logger::get_instance().enable_file_output("logs/app.log");

// 记录日志
LOG_INFO("Application started");
LOG_ERROR("Failed to load file: {}", filename);
LOG_WARN("Memory usage: {} MB", usage);
```

---

## 日志级别

```cpp
enum class LogLevel : int {
    TRACE = 0,    // 详细跟踪信息
    DEBUG = 1,    // 调试信息
    INFO = 2,     // 一般信息（默认）
    WARN = 3,     // 警告信息
    ERROR = 4,    // 错误信息
    FATAL = 5     // 致命错误信息
};
```

**使用场景：**
- `TRACE` - 非常详细的执行流程，通常只在开发时使用
- `DEBUG` - 调试信息，帮助定位问题
- `INFO` - 重要的应用状态信息（默认级别）
- `WARN` - 警告，不影响运行但需要注意
- `ERROR` - 错误，某些功能失败但应用仍可运行
- `FATAL` - 致命错误，导致应用无法继续运行

---

## Logger API

### 1. 获取单例

```cpp
static Logger& Logger::get_instance() noexcept
```

**示例：**
```cpp
auto& logger = Logger::get_instance();
```

---

### 2. 设置日志级别

```cpp
void Logger::set_level(LogLevel level) noexcept
```

**示例：**
```cpp
// 开发时使用 DEBUG 级别
Logger::get_instance().set_level(LogLevel::DEBUG);

// 发布时使用 INFO 级别
Logger::get_instance().set_level(LogLevel::INFO);

// 只显示错误
Logger::get_instance().set_level(LogLevel::ERROR);
```

---

### 3. 获取日志级别

```cpp
LogLevel Logger::get_level() const noexcept
```

**示例：**
```cpp
auto level = Logger::get_instance().get_level();
LOG_INFO("Current log level: {}", static_cast<int>(level));
```

---

### 4. 启用文件输出

```cpp
void Logger::enable_file_output(const std::string& filename, bool enable = true)
```

**参数：**
- `filename` - 日志文件路径
- `enable` - 是否启用（默认 true）

**示例：**
```cpp
// 启用文件输出
Logger::get_instance().enable_file_output("logs/app.log");

// 禁用文件输出
Logger::get_instance().enable_file_output("logs/app.log", false);
```

---

### 5. 检查文件输出状态

```cpp
bool Logger::is_file_output_enabled() const noexcept
```

**示例：**
```cpp
if (Logger::get_instance().is_file_output_enabled()) {
    LOG_INFO("File logging is enabled");
}
```

---

### 6. 设置缓冲区大小

```cpp
void Logger::set_buffer_size(size_t size) noexcept
```

**参数：**
- `size` - 缓冲区大小（字节），默认 4096

**示例：**
```cpp
// 设置更大的缓冲区以减少 I/O
Logger::get_instance().set_buffer_size(8192);
```

---

## 日志宏

### 简单字符串日志（无需格式化）

```cpp
#define LOG_TRACE_STR(msg)  ::DearTs::get_logger().trace(msg)
#define LOG_DEBUG_STR(msg)  ::DearTs::get_logger().debug(msg)
#define LOG_INFO_STR(msg)   ::DearTs::get_logger().info(msg)
#define LOG_WARN_STR(msg)   ::DearTs::get_logger().warn(msg)
#define LOG_ERROR_STR(msg)  ::DearTs::get_logger().error(msg)
#define LOG_FATAL_STR(msg)  ::DearTs::get_logger().fatal(msg)
```

**示例：**
```cpp
LOG_INFO_STR("Application started");
LOG_ERROR_STR("Failed to initialize");
```

---

### 格式化日志（使用 std::format）

```cpp
#define LOG_TRACE(fmt, ...) ::DearTs::get_logger().trace(std::format(fmt, __VA_ARGS__))
#define LOG_DEBUG(fmt, ...) ::DearTs::get_logger().debug(std::format(fmt, __VA_ARGS__))
#define LOG_INFO(fmt, ...)  ::DearTs::get_logger().info(std::format(fmt, __VA_ARGS__))
#define LOG_WARN(fmt, ...)  ::DearTs::get_logger().warn(std::format(fmt, __VA_ARGS__))
#define LOG_ERROR(fmt, ...) ::DearTs::get_logger().error(std::format(fmt, __VA_ARGS__))
#define LOG_FATAL(fmt, ...) ::DearTs::get_logger().fatal(std::format(fmt, __VA_ARGS__))
```

**示例：**
```cpp
LOG_INFO("User {} logged in from {}", username, ip_address);
LOG_ERROR("Failed to open file {}: {}", filename, error_msg);
LOG_WARN("Memory usage: {}/{} MB", used, total);
```

---

### 直接方法调用

```cpp
void trace(const std::string& msg)
void debug(const std::string& msg)
void info(const std::string& msg)
void warn(const std::string& msg)
void error(const std::string& msg)
void fatal(const std::string& msg)
```

**示例：**
```cpp
auto& logger = Logger::get_instance();
logger.info("Application started");
logger.error("Error: {}", error_code);
```

---

## 实际应用示例

### 示例 1：应用初始化

```cpp
class MyApp : public Application {
public:
    bool on_init() override {
        // 初始化日志
        auto& logger = Logger::get_instance();

        // 设置日志级别
        #ifdef DEBUG
            logger.set_level(LogLevel::DEBUG);
        #else
            logger.set_level(LogLevel::INFO);
        #endif

        // 启用文件输出
        logger.enable_file_output("logs/app.log");

        LOG_INFO("========================================");
        LOG_INFO("Application starting...");
        LOG_INFO("Version: {}", VERSION);
        LOG_INFO("Build: {}", BUILD_DATE);
        LOG_INFO("========================================");

        return true;
    }

    void on_shutdown() override {
        LOG_INFO("Application shutting down...");
        LOG_INFO("Goodbye!");
    }
};
```

---

### 示例 2：错误处理

```cpp
Result<Data, std::string> load_data(const std::string& path) {
    LOG_DEBUG("Loading data from: {}", path);

    std::ifstream file(path);
    if (!file) {
        LOG_ERROR("Failed to open file: {}", path);
        return Result<Data, std::string>::err("File not found");
    }

    try {
        Data data = parse_file(file);
        LOG_INFO("Successfully loaded {} bytes", data.size());
        return Result<Data, std::string>::ok(data);

    } catch (const std::exception& e) {
        LOG_ERROR("Exception while loading {}: {}", path, e.what());
        return Result<Data, std::string>::err(e.what());
    }
}
```

---

### 示例 3：性能追踪

```cpp
class Profiler {
private:
    std::string m_name;
    std::chrono::steady_clock::time_point m_start;

public:
    Profiler(std::string name)
        : m_name(std::move(name))
        , m_start(std::chrono::steady_clock::now()) {
        LOG_DEBUG("[{}] Started", m_name);
    }

    ~Profiler() {
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end - m_start
        ).count();

        LOG_DEBUG("[{}] Completed in {} ms", m_name, duration);
    }
};

// 使用
void process_data() {
    Profiler profiler("process_data");

    // 处理数据...

    // 函数结束时自动记录时间
}
```

---

### 示例 4：插件日志

```cpp
class MyPlugin : public IPlugin {
public:
    Result<void, std::string> on_load() override {
        LOG_INFO("MyPlugin: Loading...");

        try {
            initialize();
            LOG_INFO("MyPlugin: Loaded successfully");
            return Result::ok();

        } catch (const std::exception& e) {
            LOG_ERROR("MyPlugin: Failed to load - {}", e.what());
            return Result::err(e.what());
        }
    }

    void on_unload() override {
        LOG_INFO("MyPlugin: Unloading...");
        cleanup();
        LOG_INFO("MyPlugin: Unloaded");
    }

private:
    void initialize() {
        LOG_DEBUG("MyPlugin: Initializing components...");
        // 初始化逻辑...
        LOG_TRACE("MyPlugin: Component A initialized");
        LOG_TRACE("MyPlugin: Component B initialized");
    }
};
```

---

### 示例 5：任务日志

```cpp
auto task = TaskManager::instance().launch(
    "处理文件",
    [](const std::atomic<bool>& should_cancel) {
        LOG_INFO("任务开始: 处理文件");

        auto& tm = TaskManager::instance();
        auto tasks = tm.getTasks();
        auto task = tasks.back();

        try {
            for (int i = 0; i < 100; i++) {
                if (should_cancel) {
                    LOG_WARN("任务被取消");
                    return;
                }

                // 处理...
                LOG_TRACE("处理项目 {}/100", i + 1);

                // 更新进度
                task->setProgress((i + 1) * 100.0f / 100);
            }

            LOG_INFO("任务完成: 处理了 100 个项目");

        } catch (const std::exception& e) {
            LOG_ERROR("任务失败: {}", e.what());
        }
    }
);
```

---

### 示例 6：调试视图

```cpp
class LogViewer : public View {
private:
    bool m_scroll_to_bottom = false;

public:
    void draw_content() override {
        // 清除按钮
        if (ImGui::Button("清除")) {
            // TODO: 清除日志
        }

        ImGui::SameLine();
        ImGui::Checkbox("自动滚动", &m_auto_scroll);

        ImGui::Separator();

        // 日志过滤
        static bool show_trace = false;
        static bool show_debug = true;
        static bool show_info = true;
        static bool show_warn = true;
        static bool show_error = true;

        ImGui::Checkbox("TRACE", &show_trace);
        ImGui::SameLine();
        ImGui::Checkbox("DEBUG", &show_debug);
        ImGui::SameLine();
        ImGui::Checkbox("INFO", &show_info);
        ImGui::SameLine();
        ImGui::Checkbox("WARN", &show_warn);
        ImGui::SameLine();
        ImGui::Checkbox("ERROR", &show_error);

        ImGui::Separator();

        // TODO: 显示日志内容
        // 可以从日志文件读取或维护日志缓冲区

        if (m_scroll_to_bottom && m_auto_scroll) {
            ImGui::SetScrollHereY(1.0f);
            m_scroll_to_bottom = false;
        }
    }

private:
    bool m_auto_scroll = true;
};
```

---

## 最佳实践

### ✅ DO

1. **使用适当的日志级别**
   ```cpp
   // ✅ 好
   LOG_TRACE("Entering function, x={}", x);      // 详细调试
   LOG_DEBUG("Loaded {} items", count);          // 调试信息
   LOG_INFO("Application started");              // 重要信息
   LOG_WARN("Using default config");             // 警告
   LOG_ERROR("Failed to load: {}", path);        // 错误
   LOG_FATAL("Cannot continue");                 // 致命错误
   ```

2. **使用格式化日志**
   ```cpp
   // ✅ 好 - 类型安全，性能好
   LOG_INFO("User: {}, Action: {}, Time: {}", user, action, time);
   ```

3. **记录关键操作**
   ```cpp
   // ✅ 好
   LOG_INFO("Loading configuration from: {}", path);
   LOG_INFO("Configuration loaded successfully");
   LOG_INFO("Starting main loop");
   ```

4. **记录错误详情**
   ```cpp
   // ✅ 好
   catch (const std::exception& e) {
       LOG_ERROR("Exception in {}: {}", func_name, e.what());
   }
   ```

### ❌ DON'T

1. **不要过度使用 TRACE**
   ```cpp
   // ❌ 不好 - 太多 TRACE 日志影响性能
   void process_item(int i) {
       LOG_TRACE("Step 1");
       step1();
       LOG_TRACE("Step 2");
       step2();
       LOG_TRACE("Step 3");
       step3();
   }

   // ✅ 好 - 关键点才记录
   void process_item(int i) {
       LOG_DEBUG("Processing item {}", i);
       process(i);
   }
   ```

2. **不要在循环中记录过多日志**
   ```cpp
   // ❌ 不好 - 每次迭代都记录
   for (int i = 0; i < 1000000; i++) {
       LOG_DEBUG("Processing {}", i);  // 太多了！
       process(i);
   }

   // ✅ 好 - 定期记录进度
   for (int i = 0; i < 1000000; i++) {
       if (i % 10000 == 0) {
           LOG_DEBUG("Progress: {}/1000000", i);
       }
       process(i);
   }
   ```

3. **不要记录敏感信息**
   ```cpp
   // ❌ 不好 - 记录密码
   LOG_INFO("User login: {}, Password: {}", username, password);

   // ✅ 好
   LOG_INFO("User login: {}", username);
   ```

4. **不要忽略日志级别**
   ```cpp
   // ❌ 不好 - 所有都用 INFO
   LOG_INFO("Variable x = {}", x);  // 应该用 TRACE 或 DEBUG
   LOG_INFO("Temp value: {}", temp);  // 应该用 TRACE

   // ✅ 好 - 使用合适的级别
   LOG_TRACE("Variable x = {}", x);
   LOG_DEBUG("Temp value: {}", temp);
   ```

---

## 完整应用示例

```cpp
#include "liblogger/logger.h"
#include "core/app/application.h"

class MyApp : public Application {
public:
    bool on_init() override {
        setup_logger();

        LOG_INFO("========================================");
        LOG_INFO("Application: {}", APP_NAME);
        LOG_INFO("Version: {}", APP_VERSION);
        LOG_INFO("Platform: {}", get_platform());
        LOG_INFO("========================================");

        // 加载配置
        LOG_INFO("Loading configuration...");
        if (!load_config()) {
            LOG_WARN("Using default configuration");
        }

        // 初始化系统
        LOG_DEBUG("Initializing subsystems...");
        init_renderer();
        init_audio();
        init_network();

        LOG_INFO("Application initialized successfully");
        return true;
    }

    void on_update(double delta_time) override {
        // 定期记录性能
        if (should_log_performance()) {
            LOG_DEBUG("FPS: {:.2f}, Delta: {:.3f}ms",
                get_fps(), delta_time * 1000);
        }
    }

    void on_shutdown() override {
        LOG_INFO("Application shutting down...");

        // 清理
        LOG_DEBUG("Cleaning up resources...");
        cleanup();

        LOG_INFO("Shutdown complete");
        LOG_INFO("========================================");
    }

private:
    void setup_logger() {
        auto& logger = Logger::get_instance();

        // 设置日志级别
        #ifdef _DEBUG
            logger.set_level(LogLevel::TRACE);
            LOG_DEBUG("Debug build - TRACE level enabled");
        #else
            logger.set_level(LogLevel::INFO);
        #endif

        // 启用文件输出
        std::string log_path = "logs/app.log";
        logger.enable_file_output(log_path);
        LOG_INFO("Logging to: {}", log_path);

        // 设置缓冲区
        logger.set_buffer_size(8192);
    }

    bool load_config() {
        try {
            LOG_DEBUG("Reading config file: {}", CONFIG_PATH);
            // 读取配置...
            LOG_INFO("Configuration loaded successfully");
            return true;

        } catch (const std::exception& e) {
            LOG_ERROR("Failed to load config: {}", e.what());
            return false;
        }
    }

    void init_renderer() {
        LOG_TRACE("Initializing renderer...");
        // 初始化...
        LOG_DEBUG("Renderer initialized: {}x{}", get_width(), get_height());
    }

    void init_audio() {
        LOG_TRACE("Initializing audio...");
        // 初始化...
        LOG_DEBUG("Audio system ready");
    }

    void init_network() {
        LOG_TRACE("Initializing network...");
        // 初始化...
        LOG_DEBUG("Network layer ready");
    }

    void cleanup() {
        LOG_DEBUG("Cleaning up...");
        // 清理...
    }

    bool should_log_performance() {
        static int frame_count = 0;
        return (++frame_count % 60) == 0;  // 每 60 帧记录一次
    }
};
```

---

## 日志格式

日志输出格式：

```
[YYYY-MM-DD HH:MM:SS.mmm] [LEVEL] [file:line] Message
```

**示例：**
```
[2025-12-28 14:30:25.123] [INFO] [main.cpp:42] Application started
[2025-12-28 14:30:25.456] [DEBUG] [config.cpp:78] Loading config from: config.json
[2025-12-28 14:30:25.789] [ERROR] [file.cpp:156] Failed to open file: data.bin
```

---

## API 快速参考

### Logger 方法

| 方法 | 说明 |
|------|------|
| `get_instance()` | 获取单例 |
| `set_level(level)` | 设置日志级别 |
| `get_level()` | 获取当前级别 |
| `enable_file_output(path, enable)` | 启用文件输出 |
| `is_file_output_enabled()` | 检查文件输出状态 |
| `set_buffer_size(size)` | 设置缓冲区大小 |
| `trace/debug/info/warn/error/fatal(msg)` | 记录日志 |

### 日志宏

| 宏 | 说明 |
|-----|------|
| `LOG_TRACE(fmt, ...)` | TRACE 级别格式化日志 |
| `LOG_DEBUG(fmt, ...)` | DEBUG 级别格式化日志 |
| `LOG_INFO(fmt, ...)` | INFO 级别格式化日志 |
| `LOG_WARN(fmt, ...)` | WARN 级别格式化日志 |
| `LOG_ERROR(fmt, ...)` | ERROR 级别格式化日志 |
| `LOG_FATAL(fmt, ...)` | FATAL 级别格式化日志 |

---

## 性能考虑

1. **异步写入** - 文件 I/O 在独立线程，不阻塞主线程
2. **重复过滤** - 自动过滤短时间内的重复消息
3. **条件编译** - 发布版本可以设置更高的日志级别
4. **缓冲区** - 可调整缓冲区大小优化性能

---

**文件**: `liblogger/logger.h`
**源码**: `liblogger/logger.cpp` (如果存在)
**相关**: TaskManager, ConfigManager
