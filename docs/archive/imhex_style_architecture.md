# ImHex风格架构文档

## 概述

本文档描述了从ImHex项目参考实现的三个核心系统：
1. **事件系统** (Event System)
2. **内容注册系统** (Content Registry)
3. **视图系统** (View System)

所有系统已集成到DearTs框架中，并使用Dear ImGui作为UI框架。

---

## 1. 事件系统 (Event System)

### 核心概念

事件系统基于发布-订阅模式，允许模块间解耦通信。

### 主要组件

#### 1.1 Event（事件基类）

```cpp
class Event {
public:
    virtual std::string get_name() const = 0;
    virtual size_t get_type_id() const = 0;
};
```

所有自定义事件都应该继承`Event`类并实现这两个方法。

#### 1.2 EventManager（事件管理器）

单例类，负责事件的订阅、发布和管理。

**主要方法：**

```cpp
// 订阅事件
size_t subscribe(size_t event_type_id, EventCallback callback);

// 取消订阅
void unsubscribe(size_t event_type_id, size_t subscription_id);

// 同步发布事件
void publish(const Event& event);

// 异步发布事件（添加到队列）
void publish_async(std::unique_ptr<Event> event);

// 处理所有异步事件
void process_async_events();
```

#### 1.3 EventSubscription（订阅管理）

RAII风格的订阅管理类，自动处理取消订阅。

```cpp
EventSubscription sub = subscribe_event<EventKeyPressed>([](const EventKeyPressed& e) {
    // 处理事件
});
// 超出作用域自动取消订阅
```

### 内置事件

#### 应用程序事件

- `EventApplicationInitialized` - 应用程序初始化完成
- `EventApplicationShutdown` - 应用程序关闭

#### 窗口事件

- `EventWindowClose` - 窗口关闭请求
- `EventWindowResize` - 窗口大小改变

#### 帧事件

- `EventFrameBegin` - 帧开始
- `EventFrameEnd` - 帧结束

#### 输入事件

- `EventKeyPressed` - 按键按下

#### 请求事件

- `EventRequestExit` - 请求退出

### 使用示例

```cpp
// 1. 定义自定义事件
class MyCustomEvent : public Event {
public:
    explicit MyCustomEvent(int data) : m_data(data) {}

    std::string get_name() const override {
        return "MyCustomEvent";
    }

    size_t get_type_id() const override {
        return static_cast<size_t>(EventType::MyCustom);
    }

    int get_data() const { return m_data; }

private:
    enum EventType { MyCustom = 200 };
    int m_data;
};

// 2. 订阅事件
auto subscription = subscribe_event<MyCustomEvent>([](const MyCustomEvent& e) {
    LOG_INFO("Received custom event with data: {}", e.get_data());
});

// 3. 发布事件（同步）
MyCustomEvent event(42);
EventManager::get_instance().publish(event);

// 4. 发布事件（异步）
EventManager::get_instance().publish_async(
    std::make_unique<MyCustomEvent>(42)
);

// 5. 处理异步事件队列（在主循环中）
EventManager::get_instance().process_async_events();
```

---

## 2. 内容注册系统 (Content Registry)

### 核心概念

Content Registry是ImHex的核心设计，提供统一的内容注册接口。

### 注册子系统

#### 2.1 Settings Registry（设置注册）

```cpp
// 添加设置项
auto& setting = Settings::add(
    UnlocalizedString("general"),
    UnlocalizedString("enable_feature"),
    "true"  // 默认值
);

// 配置验证回调
setting.validate_callback = [](const std::string& value) {
    return value == "true" || value == "false";
};

// 配置变更回调
setting.change_callback = [](const std::string& value) {
    LOG_INFO("Setting changed to: {}", value);
};

// 读取设置
std::string value = Settings::get(
    UnlocalizedString("general"),
    UnlocalizedString("enable_feature")
);

// 修改设置
Settings::set(
    UnlocalizedString("general"),
    UnlocalizedString("enable_feature"),
    "false"
);
```

#### 2.2 Tools Registry（工具注册）

```cpp
// 注册工具
Tools::add(
    UnlocalizedString("my_tool"),
    "This is my custom tool",
    []() {
        LOG_INFO("Tool executed!");
    }
);

// 获取所有工具
const auto& tools = Tools::get_all();
for (const auto& tool : tools) {
    tool.callback();  // 执行工具
}
```

#### 2.3 Commands Registry（命令注册）

```cpp
// 注册命令
auto& cmd = Commands::add(
    UnlocalizedString("my_command"),
    "Execute my command",
    []() {
        LOG_INFO("Command executed!");
    }
);

// 设置快捷键
cmd.shortcut = "CTRL+M";

// 设置启用条件
cmd.enabled_callback = []() {
    return true;  // 或根据条件返回
};

// 获取所有命令
const auto& commands = Commands::get_all();
```

#### 2.4 MainMenu Registry（主菜单注册）

```cpp
// 添加菜单项
auto& menu_item = MainMenu::add(
    {"File", "My Submenu"},  // 菜单路径
    UnlocalizedString("my_menu_item"),
    []() {
        LOG_INFO("Menu item clicked!");
    }
);

// 配置菜单项
menu_item.shortcut = "CTRL+SHIFT+M";
menu_item.checked_callback = []() {
    return some_condition;
};
menu_item.enabled_callback = []() {
    return true;
};

// 获取所有菜单项
const auto& menu_items = MainMenu::get_all();
```

#### 2.5 Callbacks Registry（回调注册）

```cpp
// 添加初始化回调
Callbacks::add_on_init([]() {
    LOG_INFO("Initialized!");
});

// 添加关闭回调
Callbacks::add_on_shutdown([]() {
    LOG_INFO("Shutting down!");
});

// 添加更新回调
Callbacks::add_on_update([](double delta_time) {
    // 每帧更新
});

// 添加渲染回调
Callbacks::add_on_render([]() {
    // 每帧渲染
});

// 执行回调（由框架自动调用）
Callbacks::run_init_callbacks();
Callbacks::run_update_callbacks(delta_time);
Callbacks::run_render_callbacks();
Callbacks::run_shutdown_callbacks();
```

---

## 3. 视图系统 (View System)

### 核心概念

View系统基于Dear ImGui，提供灵活的窗口管理。

### View类型

#### 3.1 View（基类）

所有视图的基类，定义了通用接口。

```cpp
class View {
public:
    // 绘制视图
    virtual void draw(ImGuiWindowFlags extra_flags) = 0;

    // 绘制内容（子类实现）
    virtual void draw_content() = 0;

    // 绘制帮助文本（可选）
    virtual void draw_help_text() {}

    // 生命周期回调
    virtual void on_open() {}
    virtual void on_close() {}

    // 配置方法
    virtual bool should_draw() const;
    virtual ImGuiWindowFlags get_window_flags() const;
    virtual ImVec2 get_min_size() const;
    virtual ImVec2 get_max_size() const;

    // 状态管理
    bool& get_window_open_state();
    bool is_focused() const;
};
```

#### 3.2 ViewWindow（标准窗口）

最常见的视图类型，支持停靠。

```cpp
class MyView : public ViewWindow {
public:
    MyView() : ViewWindow(UnlocalizedString("my_view"), ICON) {}

    void draw_content() override {
        ImGui::Text("Hello from MyView!");
    }
};
```

#### 3.3 ViewSpecial（特殊视图）

不处理窗口创建，只绘制内容。

```cpp
class MySpecialView : public ViewSpecial {
public:
    MySpecialView() : ViewSpecial(UnlocalizedString("special")) {}

    void draw_content() override {
        // 自定义绘制逻辑
    }
};
```

#### 3.4 ViewFloating（浮动窗口）

无法停靠的浮动窗口。

```cpp
class MyFloatingView : public ViewFloating {
public:
    MyFloatingView() : ViewFloating(UnlocalizedString("floating"), ICON) {}

    void draw_content() override {
        ImGui::Text("Floating window content");
    }
};
```

#### 3.5 ViewModal（模态窗口）

始终置顶且阻止其他窗口输入。

```cpp
class MyModalView : public ViewModal {
public:
    MyModalView() : ViewModal(UnlocalizedString("modal"), ICON) {}

    void draw_content() override {
        ImGui::Text("Modal content");

        if (ImGui::Button("Close")) {
            get_window_open_state() = false;
        }
    }

    bool has_close_button() const override {
        return false;  // 隐藏关闭按钮
    }
};
```

### 视图注册

```cpp
// 注册视图
Views::add<MyView>();
Views::add<MySpecialView>();
Views::add<MyFloatingView>();
Views::add<MyModalView>();

// 通过名称获取视图
View* view = Views::get_by_name(UnlocalizedString("my_view"));

// 获取当前聚焦的视图
View* focused = Views::get_focused();

// 获取所有视图
const auto& all_views = Views::get_all();
```

### 完整视图示例

```cpp
class HelloView : public ViewWindow {
public:
    HelloView() : ViewWindow(UnlocalizedString("hello"), "Hello") {
        m_counter = 0;
    }

    void draw_content() override {
        ImGui::Text("Hello from View System!");

        ImGui::Separator();

        ImGui::Text("Counter: %d", m_counter);

        if (ImGui::Button("Increment")) {
            m_counter++;
            on_counter_changed();
        }

        ImGui::SameLine();

        if (ImGui::Button("Reset")) {
            m_counter = 0;
            on_counter_changed();
        }

        draw_help_text();
    }

    void draw_help_text() override {
        ImGui::TextColored(
            ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
            "This is a sample view."
        );
    }

    void on_open() override {
        LOG_INFO("HelloView opened");
    }

    void on_close() override {
        LOG_INFO("HelloView closed");
    }

    ImVec2 get_min_size() const override {
        return ImVec2(300, 200);
    }

private:
    int m_counter;

    void on_counter_changed() {
        LOG_INFO("Counter changed to: {}", m_counter);
    }
};

// 注册视图
Views::add<HelloView>();
```

---

## 4. 集成使用

### 完整的应用程序流程

```cpp
class MyApp {
public:
    bool initialize() {
        // 1. 初始化SDL
        SDL_Init(SDL_INIT_VIDEO);

        // 2. 创建窗口和渲染器
        m_window = SDL_CreateWindow(...);
        m_renderer = SDL_CreateRenderer(...);

        // 3. 初始化ImGui层
        m_imgui_layer.initialize(m_window);

        // 4. 注册内容
        register_content();

        // 5. 订阅事件
        subscribe_events();

        // 6. 发布初始化事件
        EventApplicationInitialized init_event;
        EventManager::get_instance().publish(init_event);

        return true;
    }

    void run() {
        while (m_running) {
            // 1. 处理SDL事件
            while (SDL_PollEvent(&event)) {
                m_imgui_layer.process_event(event);
                handle_sdl_event(event);
            }

            // 2. 开始ImGui帧
            m_imgui_layer.begin_frame();

            // 3. 处理异步事件
            EventManager::get_instance().process_async_events();

            // 4. 发布帧开始事件
            EventFrameBegin frame_event(delta_time);
            EventManager::get_instance().publish(frame_event);

            // 5. 执行更新回调
            Callbacks::run_update_callbacks(delta_time);

            // 6. 创建主菜单
            create_main_menu_bar();

            // 7. 绘制所有视图
            draw_views();

            // 8. 执行渲染回调
            Callbacks::run_render_callbacks();

            // 9. 渲染ImGui
            m_imgui_layer.render();

            // 10. 呈现
            SDL_RenderPresent(m_renderer);

            // 11. 发布帧结束事件
            EventFrameEnd frame_end_event;
            EventManager::get_instance().publish(frame_end_event);
        }
    }

private:
    void register_content() {
        // 注册视图
        Views::add<MyView>();

        // 注册工具
        Tools::add(...);

        // 注册命令
        Commands::add(...);

        // 注册菜单项
        MainMenu::add(...);

        // 注册回调
        Callbacks::add_on_init(...);
        Callbacks::add_on_shutdown(...);
    }

    void subscribe_events() {
        subscribe_event<EventWindowClose>([](const EventWindowClose&) {
            // 处理窗口关闭
        });

        subscribe_event<EventKeyPressed>([](const EventKeyPressed& e) {
            // 处理按键
        });
    }

    void create_main_menu_bar() {
        if (ImGui::BeginMainMenuBar()) {
            // File菜单
            if (ImGui::BeginMenu("File")) {
                // 使用ContentRegistry::MainMenu::get_all()来添加菜单项
                ImGui::EndMenu();
            }

            // View菜单
            if (ImGui::BeginMenu("View")) {
                // 列出所有视图
                for (auto& [name, view] : Views::get_all()) {
                    bool open = view->get_window_open_state();
                    if (ImGui::MenuItem(view->get_name().c_str(), nullptr, &open)) {
                        view->get_window_open_state() = open;
                    }
                }
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }
    }

    void draw_views() {
        for (auto& [name, view] : Views::get_all()) {
            if (view->should_draw()) {
                view->draw();
            }
        }
    }
};
```

---

## 5. 最佳实践

### 5.1 事件系统

- ✅ 使用`publish_async`避免在事件回调中阻塞
- ✅ 使用`EventSubscription` RAII类自动管理订阅
- ✅ 避免在事件回调中发布同步事件（可能导致死锁）
- ❌ 不要在事件回调中做耗时操作

### 5.2 Content Registry

- ✅ 在初始化阶段注册所有内容
- ✅ 使用`UnlocalizedString`作为标识符
- ✅ 为设置项提供验证和变更回调
- ❌ 不要在运行时动态注册内容

### 5.3 View系统

- ✅ 继承合适的View类型（Window/Special/Floating/Modal）
- ✅ 实现`draw_content()`绘制内容
- ✅ 使用`on_open()`和`on_close()`管理资源
- ❌ 不要在`draw_content()`中做耗时计算

---

## 6. 与ImHex的对比

### 相似之处

1. **Content Registry** - 几乎完全相同的设计
2. **View系统** - 基于ImGui的窗口管理
3. **事件系统** - 发布-订阅模式

### 主要差异

1. **UI框架** - DearTs使用SDL Renderer + ImGui，ImHex使用GLFW + OpenGL
2. **插件系统** - ImHex有完整的动态插件加载，DearTs暂未实现
3. **模式语言** - ImHex有自定义的模式语言，DearTs没有

---

## 7. 文件结构

```
core/
├── event/
│   ├── event.h              # 事件系统核心
│   ├── event.cpp
│   └── events.h             # 内置事件定义
├── content/
│   ├── content_registry.h   # Content Registry
│   └── content_registry.cpp
└── ui/
    ├── imgui_layer.h        # ImGui集成层
    ├── imgui_layer.cpp
    ├── view.h               # View系统
    └── view.cpp

examples/
└── demo_imhex_style/        # 完整示例
    ├── main.cpp
    └── CMakeLists.txt
```

---

## 8. 编译和运行

```bash
# 配置项目
cmake -B build

# 编译示例
cmake --build build --target demo_imhex_style

# 运行
./build/bin/demo_imhex_style
```

---

## 9. 总结

通过参考ImHex的架构，DearTs现在拥有：

1. ✅ **解耦的事件系统** - 模块间松耦合通信
2. ✅ **统一的内容注册** - 类似ImHex的ContentRegistry
3. ✅ **灵活的视图系统** - 基于ImGui的窗口管理
4. ✅ **完整的ImGui集成** - 包括停靠、多视口等特性

这些系统为构建复杂的工具类应用程序提供了坚实的基础。
