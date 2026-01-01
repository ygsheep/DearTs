# 类型安全事件系统详解

DearTs Framework 提供了一个类型安全、编译时检查的事件系统，基于 EventBus 模式实现发布-订阅架构。

## 概述

**文件**: `core/event/event_bus.h`

事件系统是 DearTs 的核心通信机制，用于解耦模块间的依赖关系。

## 核心特性

- ✅ **类型安全** - 编译时类型检查，无需手动 ID
- ✅ **RAII 管理** - 自动订阅/取消订阅
- ✅ **线程安全** - 使用递归互斥锁保护
- ✅ **异步支持** - 支持异步事件队列
- ✅ **高性能** - 零拷贝传递复杂事件

## 基本用法

### 定义事件

事件可以是任意结构体或类：

```cpp
// 简单事件
struct WindowCloseEvent {
    int window_id;
    const char* title;
};

// 复杂事件
struct DataModifiedEvent {
    size_t offset;
    size_t size;
    std::vector<uint8_t> data;
    std::chrono::system_clock::time_point timestamp;
};

// 带默认值的事件
struct SettingsChangeEvent {
    std::string key;
    std::string old_value;
    std::string new_value;
};
```

### 订阅事件

```cpp
// 订阅 WindowCloseEvent
auto token = EventBus::instance().subscribe<WindowCloseEvent>(
    [](const WindowCloseEvent& e) {
        LOG_INFO("Window {} ({}) closed", e.window_id, e.title);
    }
);

// 订阅并捕获外部变量
std::string targetPath;
auto token2 = EventBus::instance().subscribe<FileModifiedEvent>(
    [targetPath](const FileModifiedEvent& e) {
        if (e.path == targetPath) {
            LOG_INFO("Target file modified!");
        }
    }
);
```

### 发布事件

```cpp
// 发布事件
EventBus::instance().publish(WindowCloseEvent{
    .window_id = 42,
    .title = "Main Window"
});

// 发布复杂事件
EventBus::instance().publish(DataModifiedEvent{
    .offset = 0x1000,
    .size = 256,
    .data = std::vector<uint8_t>(256, 0xFF),
    .timestamp = std::chrono::system_clock::now()
});
```

### RAII 自动管理

```cpp
{
    // 创建订阅作用域
    auto guard = make_event_guard<DataModifiedEvent>(
        [](const DataModifiedEvent& e) {
            LOG_INFO("Data modified: {} bytes at 0x{:X}",
                     e.size, e.offset);
        }
    );

    // 在此作用域内，订阅有效
    EventBus::instance().publish(DataModifiedEvent{...});

}  // guard 析构，自动取消订阅
```

## 高级用法

### 成员函数订阅

```cpp
class DataManager {
public:
    void onDataModified(const DataModifiedEvent& e) {
        LOG_INFO("Data modified in manager");
    }
};

// 订阅成员函数
DataManager manager;
auto token = EventBus::instance().subscribe<DataModifiedEvent>(
    &manager,
    &DataManager::onDataModified
);
```

### 取消订阅

```cpp
// 手动取消订阅
EventBus::instance().unsubscribe<DataModifiedEvent>(token);

// Token 析构时自动取消（RAII）
{
    auto token = EventBus::instance().subscribe<Event>(handler);
}  // 自动取消
```

### 条件订阅

```cpp
// 使用 lambda 过滤
auto token = EventBus::instance().subscribe<DataModifiedEvent>(
    [](const DataModifiedEvent& e) {
        // 只处理特定偏移的修改
        if (e.offset >= 0x1000 && e.offset < 0x2000) {
            LOG_INFO("Critical region modified!");
        }
    }
);
```

### 异步事件队列

```cpp
// 启用异步处理
EventBus::instance().setAsyncMode(true);

// 异步发布事件
EventBus::instance().publishAsync(DataModifiedEvent{...});

// 处理队列
EventBus::instance().processQueue();
```

## 实际应用示例

### 1. 文件系统监控

```cpp
struct FileCreatedEvent {
    std::string path;
    size_t size;
};

struct FileDeletedEvent {
    std::string path;
};

class FileManager {
public:
    FileManager() {
        // 订阅文件事件
        m_createdToken = EventBus::instance().subscribe<FileCreatedEvent>(
            std::bind(&FileManager::onFileCreated, this, std::placeholders::_1)
        );
        m_deletedToken = EventBus::instance().subscribe<FileDeletedEvent>(
            std::bind(&FileManager::onFileDeleted, this, std::placeholders::_1)
        );
    }

    void onFileCreated(const FileCreatedEvent& e) {
        m_files[e.path] = e.size;
        LOG_INFO("File created: {} ({} bytes)", e.path, e.size);
    }

    void onFileDeleted(const FileDeletedEvent& e) {
        m_files.erase(e.path);
        LOG_INFO("File deleted: {}", e.path);
    }

private:
    EventBus::Token m_createdToken;
    EventBus::Token m_deletedToken;
    std::unordered_map<std::string, size_t> m_files;
};
```

### 2. UI 事件处理

```cpp
struct MenuItemClickedEvent {
    std::string menu_id;
    std::string item_id;
};

struct WindowResizedEvent {
    int width;
    int height;
};

class UIManager {
public:
    void registerHandlers() {
        EventBus::instance().subscribe<MenuItemClickedEvent>(
            [this](const MenuItemClickedEvent& e) {
                onMenuItemClicked(e.item_id);
            }
        );

        EventBus::instance().subscribe<WindowResizedEvent>(
            [this](const WindowResizedEvent& e) {
                onWindowResized(e.width, e.height);
            }
        );
    }

private:
    void onMenuItemClicked(const std::string& itemId) {
        LOG_INFO("Menu item clicked: {}", itemId);
    }

    void onWindowResized(int width, int height) {
        m_width = width;
        m_height = height;
    }

    int m_width, m_height;
};
```

### 3. 数据变更通知

```cpp
struct DataChangedEvent {
    size_t offset;
    size_t size;
};

class HexEditor {
public:
    void modifyData(size_t offset, uint8_t value) {
        m_data[offset] = value;

        // 通知其他组件
        EventBus::instance().publish(DataChangedEvent{
            .offset = offset,
            .size = 1
        });
    }

    void insertData(size_t offset, const std::vector<uint8_t>& data) {
        m_data.insert(m_data.begin() + offset, data.begin(), data.end());

        EventBus::instance().publish(DataChangedEvent{
            .offset = offset,
            .size = data.size()
        });
    }
};
```

### 4. 插件事件通信

```cpp
// 插件定义自定义事件
struct PluginCustomEvent {
    std::string plugin_id;
    nlohmann::json data;
};

// 主程序订阅插件事件
EventBus::instance().subscribe<PluginCustomEvent>(
    [](const PluginCustomEvent& e) {
        LOG_INFO("Plugin {} sent event: {}", e.plugin_id, e.data.dump());
    }
);
```

## 事件系统架构

### 线程安全

```cpp
class EventBus {
private:
    std::recursive_mutex m_mutex;  // 递归互斥锁
    std::unordered_map<std::type_index, std::vector<EventHandler>> m_handlers;

public:
    template<typename Event>
    Token subscribe(Handler<Event> handler) {
        std::lock_guard lock(m_mutex);
        // ...
    }

    template<typename Event>
    void publish(const Event& event) {
        std::lock_guard lock(m_mutex);
        // ...
    }
};
```

### 性能优化

```cpp
// 批量发布
EventBus::instance().publishBatch(
    DataChangedEvent{...},
    FileModifiedEvent{...},
    SelectionChangedEvent{...}
);

// 延迟发布
EventBus::instance().publishDelayed(
    DataChangedEvent{...},
    std::chrono::milliseconds(100)
);
```

## 最佳实践

### 1. 事件命名规范

```cpp
// ✅ 推荐 - 清晰的动词+名词
struct WindowClosedEvent {};
struct FileModifiedEvent {};
struct DataLoadedEvent {};

// ❌ 不推荐 - 模糊的名称
struct WindowEvent {};
struct FileEvent {};
```

### 2. 事件应该不可变

```cpp
// ✅ 推荐 - 使用 const 或值类型
struct DataModifiedEvent {
    const size_t offset;
    const std::vector<uint8_t> data;
};

// ❌ 不推荐 - 可变成员
struct DataModifiedEvent {
    size_t offset;  // 可能被修改
    std::vector<uint8_t>& data;  // 引用，生命周期风险
};
```

### 3. 避免循环订阅

```cpp
// ❌ 危险 - 可能导致无限循环
// Class A
EventBus::instance().subscribe<EventB>(
    [](const EventB&) {
        EventBus::instance().publish(EventA{});
    }
);

// Class B
EventBus::instance().subscribe<EventA>(
    [](const EventA&) {
        EventBus::instance().publish(EventB{});
    }
);
```

### 4. 使用 RAII 管理订阅

```cpp
// ✅ 推荐
class MyClass {
    MyClass() {
        m_token = EventBus::instance().subscribe<Event>(handler);
    }

private:
    EventBus::Token m_token;  // 自动管理
};

// ❌ 不推荐
class MyClass {
    ~MyClass() {
        // 容易忘记取消订阅
        EventBus::instance().unsubscribe<Event>(m_token);
    }
};
```

### 5. 复杂事件使用智能指针

```cpp
// ✅ 推荐 - 大对象使用智能指针
struct LargeDataEvent {
    std::shared_ptr<std::vector<uint8_t>> data;  // 避免拷贝
};

// 发布
EventBus::instance().publish(LargeDataEvent{
    .data = std::make_shared<std::vector<uint8_t>>(1024 * 1024)
});
```

## 常见问题

### Q: 如何处理事件异常？

```cpp
EventBus::instance().subscribe<Event>(
    [](const Event& e) {
        try {
            // 处理事件
        } catch (const std::exception& ex) {
            LOG_ERROR("Event handler exception: {}", ex.what());
        }
    }
);
```

### Q: 如何调试事件订阅？

```cpp
// 记录所有订阅
EventBus::instance().subscribe<DebugEvent>(
    [](const DebugEvent& e) {
        LOG_DEBUG("Event received: {}", e.name);
    }
);

// 统计订阅数量
size_t count = EventBus::instance().getSubscriberCount<Event>();
LOG_INFO("Event has {} subscribers", count);
```

### Q: 事件处理的顺序？

事件按照订阅的反序调用（后订阅的先处理）。

```cpp
// 先订阅
EventBus::instance().subscribe<Event>(handler1);
EventBus::instance().subscribe<Event>(handler2);

// 发布时：handler2 先执行，然后 handler1
EventBus::instance().publish(Event{});
```

## API 参考

```cpp
class EventBus {
public:
    static EventBus& instance();

    // 订阅
    template<typename Event>
    Token subscribe(Handler<Event> handler);

    template<typename Event, typename Class>
    Token subscribe(Class* instance, void (Class::*method)(const Event&));

    // 取消订阅
    template<typename Event>
    void unsubscribe(const Token& token);

    // 发布
    template<typename Event>
    void publish(const Event& event);

    template<typename Event>
    void publishAsync(const Event& event);

    // 批量发布
    template<typename... Events>
    void publishBatch(const Events&... events);

    // RAII 守卫
    template<typename Event>
    [[nodiscard]] auto make_guard(Handler<Event> handler);

    // 异步模式
    void setAsyncMode(bool enabled);
    void processQueue();

    // 查询
    template<typename Event>
    size_t getSubscriberCount() const;
};
```

## 参考资源

- 源码: `core/event/event_bus.h`
- 示例: `examples/demo_imhex_style/`
- 设计模式: Observer Pattern, Publish-Subscribe Pattern
