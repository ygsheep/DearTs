# DearTs Framework - Claude Code Guide

DearTs Framework is a modern C++20 application framework based on SDL3 + ImGui, featuring an ImHex-style plugin system, type-safe event bus, and comprehensive developer tools.

## Build Commands

### Prerequisites
- CMake 3.20+
- C++20 compatible compiler (MSVC 2022, GCC 11+, Clang 13+)
- SDL3
- ImGui 2.13.3+
- nlohmann/json
- fmtlib

### Building

```bash
# Configure build (Release mode)
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build all targets
cmake --build build --config Release

# Build specific target
cmake --build build --target DearTsApp --config Release

# Clean build
cmake --build build --target clean
```

### Running

```bash
# Run application
./build/bin/DearTsApp.exe        # Windows
./build/bin/DearTsApp            # Linux/macOS
```

### Development Build

```bash
# Configure with debug symbols
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Build with verbose output
cmake --build build --verbose

# Run tests (when available)
ctest --test-dir build --verbose
```

---

## Architecture Overview

### Core Design Principles

DearTs Framework follows these architectural patterns:

1. **Plugin-First Architecture** - Extensible through IPlugin interface with lifecycle management
2. **Event-Driven Design** - Type-safe EventBus for decoupled communication
3. **Content Registry** - ImHex-style centralized command/tool/settings registration
4. **Type Safety** - Result<T, E> for error handling, std::variant for type-safe values
5. **RAII** - Automatic resource management (EventBus::Token, ConfigScope, Task smart pointers)
6. **Modern C++20** - Concepts, ranges, coroutines, std::format

### System Components

```
DearTs Framework
├── Core Systems
│   ├── Plugin System (IPlugin, PluginManager)
│   ├── EventBus (Type-safe pub/sub)
│   ├── Content Registry (Commands, Views, Tools, Settings)
│   ├── ConfigManager (Hierarchical config with JSON)
│   ├── TaskManager (Async tasks with progress)
│   └── Logger (Thread-safe async logging)
│
├── UI Layer
│   ├── View System (Dockable ImGui windows)
│   ├── TitleBar (Custom toolbar buttons)
│   └── Layout Management
│
└── Plugin Ecosystem
    ├── Builtin Plugin (Default UI components)
    └── Custom Plugins (User-extensible)
```

### Plugin Lifecycle

```
Unloaded
    ↓ add_builtin() / load_from_file()
Loaded
    ↓ enable()
Enabled
    ↓ disable()
Loaded
    ↓ unload()
Unloaded
```

### Data Flow

```
User Action
    ↓
ContentRegistry::Commands::invoke()
    ↓
EventBus::publish(Event)
    ↓
Plugin Event Handlers (subscribed via EventBus::Token)
    ↓
UI Updates / Background Tasks
```

---

## Key Directories

### `/core/` - Framework Core Systems

```
core/
├── plugin/          # Plugin system (IPlugin, PluginManager)
├── events/          # EventBus implementation
├── content/         # Content Registry (commands, views, tools, settings)
├── ui/              # UI components (View base class, TitleBar, Layout)
├── tasks/           # TaskManager (async task execution)
├── config/          # ConfigManager (hierarchical config with JSON)
└── utils/           # Utility types (Result<T,E>, std::format helpers)
```

### `/plugins/` - Plugin Implementations

```
plugins/
├── builtin/         # Built-in plugin (compiled statically)
│   ├── include/
│   │   └── views/   # View implementations (DataInspector, HelloWorld)
│   └── source/      # BuiltinPlugin implementation
└── QUICSTART.md     # Plugin development guide
```

### `/dearts-dev/` - Developer Documentation

```
dearts-dev/
├── SKILL.md                    # Framework overview and quick reference
├── references/
│   ├── config_manager_api.md    # ConfigManager API manual (676 lines)
│   ├── logger_api.md            # Logger API manual (727 lines)
│   ├── task_manager_api.md      # TaskManager API manual (917 lines)
│   └── plugin_system_api.md     # Plugin System API manual (1038 lines)
└── *_UPDATE_2025.md            # Documentation update summaries
```

### `/docs/` - User Documentation

```
docs/
├── plugin_system_guide.md      # Plugin system user guide (706 lines)
└── (additional user guides)
```

---

## Important Files

### API Documentation (READ THESE FIRST!)

When implementing features, **consult the API manuals** before reading source code:

1. **`dearts-dev/references/plugin_system_api.md`** (1038 lines)
   - IPlugin interface (get_info, on_load, on_unload, on_enable, on_disable)
   - PluginManager API (12 methods)
   - 4 complete examples (Hello World, Data Processor, Toolbar, Plugin Manager)
   - Plugin development checklist and best practices

2. **`dearts-dev/references/config_manager_api.md`** (676 lines)
   - ConfigManager::get/set/remove methods
   - ConfigValue types (bool, int, double, string)
   - ConfigScope for RAII prefix management
   - JSON load/save support

3. **`dearts-dev/references/task_manager_api.md`** (917 lines)
   - TaskManager::launch() for async tasks
   - Task types (Normal, Background, Blocking, Critical)
   - Progress tracking and cancellation
   - ImGui progress bar integration

4. **`dearts-dev/references/logger_api.md`** (727 lines)
   - LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_FATAL
   - Thread-safe async file writing
   - Duplicate message filtering

### Plugin Development

- **`plugins/QUICKSTART.md`** - Quick start guide for creating plugins
- **`plugins/builtin/`** - Reference implementation of a built-in plugin
- **`docs/plugin_system_guide.md`** - Comprehensive plugin system guide

### Core Headers

- **`core/plugin/plugin.h`** - IPlugin interface, PluginManager
- **`core/events/event_bus.h`** - EventBus for type-safe events
- **`core/content/registry.h`** - Content Registry (commands, views, tools)
- **`core/config/config_manager.h`** - ConfigManager, ConfigScope
- **`core/tasks/task_manager.h`** - TaskManager, Task
- **`liblogger/logger.h`** - Logging macros

---

## Development Workflow

### 1. Using the API Documentation

**STOP - Don't read source code yet!**

The framework has comprehensive API documentation. Always check the API manuals first:

```cpp
// Need to use ConfigManager?
// Read: dearts-dev/references/config_manager_api.md

// Need to create a plugin?
// Read: dearts-dev/references/plugin_system_api.md

// Need async task execution?
// Read: dearts-dev/references/task_manager_api.md

// Need to add logging?
// Read: dearts-dev/references/logger_api.md
```

Each API manual includes:
- ✅ Quick start guide
- ✅ Complete API reference
- ✅ Practical code examples
- ✅ Best practices (DO's and DON'Ts)
- ✅ Integration patterns

### 2. Plugin Development Workflow

Creating a new plugin:

```cpp
// 1. Read plugin_system_api.md (section: Quick Start)
// 2. Create plugin class inheriting IPlugin
class MyPlugin : public IPlugin {
public:
    PluginInfo get_info() const override {
        return PluginInfo{
            .name = "MyPlugin",
            .author = "Your Name",
            .description = "Plugin description",
            .version = "1.0.0",
            .api_version = "1.0.0"
        };
    }

    Result<void, std::string> on_load() override {
        // Register commands, views, tools
        ContentRegistry::Commands::register_handler(...);
        ContentRegistry::Views::add<MyView>();

        return Result::ok();
    }

    void on_unload() override {
        // Cleanup (RAII handles most)
    }
};

// 3. Register plugin
PluginManager::instance().add_builtin(
    std::make_unique<MyPlugin>()
);
```

Reference:
- **API**: `dearts-dev/references/plugin_system_api.md`
- **Example**: `plugins/builtin/`
- **Guide**: `plugins/QUICKSTART.md`

### 3. Using Core Systems

**ConfigManager** - Configuration management:

```cpp
#include "core/config/config_manager.h"

// Use ConfigScope for automatic prefixing
ConfigScope config("myplugin");

// Get/Set values
bool enabled = config.get_or<bool>("enabled", true);
config.set("interval", 60);

// Load/Save JSON
ConfigManager::instance().load_from_file("config.json");
ConfigManager::instance().save_to_file("config.json");
```

**TaskManager** - Async task execution:

```cpp
#include "core/tasks/task_manager.h"

// Launch async task
auto task = TaskManager::instance().launch(
    "Loading File",
    [](const auto& cancel) {
        // Do work
        for (int i = 0; i < 100 && !cancel.is_cancelled(); i++) {
            task.update_progress(i, 100);
            // ... process ...
        }
        return Result::ok();
    }
);

// Check completion
if (task->is_done()) {
    auto result = task->get_result();
}
```

**EventBus** - Type-safe events:

```cpp
#include "core/events/event_bus.h"

// Define event
struct DataModifiedEvent {
    size_t offset;
    size_t size;
};

// Subscribe (RAII - auto-unsubscribe)
EventBus::Token token = EventBus::instance().subscribe<DataModifiedEvent>(
    [](const DataModifiedEvent& e) {
        LOG_INFO("Data modified: {} bytes at offset {}", e.size, e.offset);
    }
);

// Publish
EventBus::instance().publish(DataModifiedEvent{0, 1024});
```

**Logger** - Thread-safe logging:

```cpp
#include "liblogger/logger.h"

LOG_INFO("Loading file: {}", filename);
LOG_ERROR("Failed to load: {}", error_msg);
LOG_WARN("Deprecated feature used");

// With formatting
LOG_DEBUG("Processing item {} of {}", current, total);
```

**Content Registry** - Register commands, views, tools:

```cpp
#include "core/content/commands.h"
#include "core/content/registry.h"

// Register command
ContentRegistry::Commands::register_handler(
    "myplugin.action",
    "My Action",
    []() { /* do something */ },
    nullptr,        // enabled callback
    "Ctrl+Shift+M"  // shortcut
);

// Register view
ContentRegistry::Views::add<MyView>();

// Register tool
ContentRegistry::Tools::add("My Tool", []() { /* ... */ });

// Register settings
ContentRegistry::Settings::add("myplugin.key", "Name", default_value);
```

### 4. Creating Views

Views are ImGui windows that dock into the main interface:

```cpp
class MyView : public View {
public:
    std::string getName() const override {
        return "My View";
    }

    void draw_content() override {
        ImGui::Text("Hello from my view!");

        if (ImGui::Button("Click me")) {
            LOG_INFO("Button clicked");
        }
    }
};

// Register in plugin
ContentRegistry::Views::add<MyView>();
```

Reference: `plugins/builtin/include/views/data_inspector_view.hpp`

### 5. Common Patterns

**Pattern 1: Plugin with Configuration**

```cpp
class MyPlugin : public IPlugin {
private:
    ConfigScope m_config{"myplugin"};

public:
    Result<void, std::string> on_load() override {
        bool enabled = m_config.get_or<bool>("enabled", true);
        if (enabled) {
            // Enable features
        }
        return Result::ok();
    }
};
```

**Pattern 2: Plugin with Events**

```cpp
class MyPlugin : public IPlugin {
private:
    EventBus::Token m_eventToken;  // RAII auto-unsubscribe

public:
    Result<void, std::string> on_load() override {
        m_eventToken = EventBus::instance().subscribe<MyEvent>(
            [this](const MyEvent& e) {
                handleEvent(e);
            }
        );
        return Result::ok();
    }
};
```

**Pattern 3: Plugin with Background Tasks**

```cpp
class MyPlugin : public IPlugin {
private:
    std::shared_ptr<Task> m_task;

public:
    Result<void, std::string> on_load() override {
        m_task = TaskManager::instance().launch(
            "Background Task",
            [](const auto& cancel) {
                // Async work
            }
        );
        return Result::ok();
    }
};
```

---

## Best Practices

### ✅ DO

1. **Always use Result<T, E> for error handling**
   ```cpp
   Result<void, std::string> on_load() override {
       if (!init()) {
           return Result::err("Initialization failed");
       }
       return Result::ok();
   }
   ```

2. **Use RAII for resource management**
   - EventBus::Token (auto-unsubscribe)
   - ConfigScope (auto-prefix)
   - std::shared_ptr<Task> (auto-cleanup)

3. **Use ConfigScope for hierarchical config**
   ```cpp
   ConfigScope config("myplugin");  // Auto-prefixes with "myplugin."
   config.set("enabled", true);     // Stored as "myplugin.enabled"
   ```

4. **Log important events**
   ```cpp
   LOG_INFO("Plugin loading...");
   LOG_ERROR("Failed to load: {}", error);
   ```

5. **Use prefixes to avoid naming conflicts**
   - Commands: `"myplugin.action"`
   - Settings: `"myplugin.setting_name"`

6. **Check API documentation before source code**
   - 4 comprehensive API manuals in `dearts-dev/references/`
   - Each has quick start + examples + best practices

### ❌ DON'T

1. **Don't use exceptions for control flow**
   - Use Result<T, E> for expected errors
   - Exceptions only for truly exceptional conditions

2. **Don't forget API version**
   ```cpp
   PluginInfo{
       .api_version = "1.0.0"  // Must match framework
   }
   ```

3. **Don't use global state**
   - Store state in plugin class members
   - Use ConfigManager for persistent config

4. **Don't manually manage resources**
   - Let RAII handle cleanup
   - EventBus::Token auto-unsubscribes on destruction

5. **Don't read source code first**
   - Check API documentation: `dearts-dev/references/*.md`
   - Only read source when documentation is unclear

---

## Troubleshooting

### Build Issues

**CMake can't find SDL3/ImGui:**
```bash
# Set CMAKE_PREFIX_PATH
cmake -B build -DCMAKE_PREFIX_PATH=/path/to/dependencies
```

**C++20 not supported:**
- Ensure compiler version: MSVC 2022, GCC 11+, Clang 13+
- Check CMakeLists.txt for `/std:c++20` or `-std=c++20`

### Plugin Not Loading

**Check:**
1. Plugin name is unique
2. `get_info()` returns valid PluginInfo
3. API version matches framework
4. `on_load()` returns Result::ok()

**Debug:**
```cpp
Result<void, std::string> on_load() override {
    LOG_INFO("MyPlugin: Loading...");  // Check logs
    // ...
    return Result::ok();
}
```

### Config Not Persisting

**Check:**
1. Called `save_to_file()` before exit
2. ConfigManager is not destroyed before save
3. JSON file path is writable

**Solution:**
```cpp
// Save on application exit
ConfigManager::instance().save_to_file("config.json");
```

### View Not Showing

**Check:**
1. View is registered: `ContentRegistry::Views::add<MyView>()`
2. View is opened: `view->get_window_open_state() = true`
3. `getName()` returns unique name

**Solution:**
```cpp
// After registration
auto* view = ContentRegistry::Views::get_by_name("My View");
if (view) {
    view->get_window_open_state() = true;
}
```

---

## Quick Reference

### Core Systems

| System | Header | API Manual |
|--------|--------|------------|
| Plugin System | `core/plugin/plugin.h` | `dearts-dev/references/plugin_system_api.md` |
| ConfigManager | `core/config/config_manager.h` | `dearts-dev/references/config_manager_api.md` |
| TaskManager | `core/tasks/task_manager.h` | `dearts-dev/references/task_manager_api.md` |
| Logger | `liblogger/logger.h` | `dearts-dev/references/logger_api.md` |
| EventBus | `core/events/event_bus.h` | `dearts-dev/SKILL.md` (section 5) |
| Content Registry | `core/content/registry.h` | `dearts-dev/SKILL.md` (section 7) |

### Common Operations

```cpp
// Plugin registration
PluginManager::instance().add_builtin(std::make_unique<MyPlugin>());

// Config operations
ConfigScope cfg("prefix");
cfg.set("key", value);
auto value = cfg.get_or<int>("key", default);

// Async task
auto task = TaskManager::instance().launch("Task Name", callback);

// Logging
LOG_INFO("Message: {}", arg);

// Event subscription
EventBus::Token token = EventBus::instance().subscribe<Event>(handler);

// Command registration
ContentRegistry::Commands::register_handler("id", "Name", callback);

// View registration
ContentRegistry::Views::add<MyView>();
```

---

## Additional Resources

### Documentation

- **`dearts-dev/SKILL.md`** - Framework overview and quick reference
- **`dearts-dev/references/*.md`** - API manuals (4 core systems)
- **`docs/plugin_system_guide.md`** - Plugin system user guide
- **`plugins/QUICKSTART.md`** - Plugin development quick start

### Examples

- **`plugins/builtin/`** - Built-in plugin implementation
- **`plugins/builtin/include/views/data_inspector_view.hpp`** - Practical view example
- **API manuals** - Each contains 4-6 complete examples

### Architecture

- **Event-driven design** - Decoupled communication via EventBus
- **Plugin-first** - All features extendable through plugins
- **ImHex-style** - Content Registry for centralized command/tool/settings
- **Modern C++20** - Type-safe, RAII, functional programming support

---

## Summary

**Key Points:**

1. **Read API documentation first** - `dearts-dev/references/*.md` (4 comprehensive manuals)
2. **Use Result<T, E> for errors** - Not exceptions
3. **RAII for resources** - EventBus::Token, ConfigScope, smart pointers
4. **Plugin-driven architecture** - Extend via IPlugin interface
5. **Event-driven communication** - EventBus for decoupled components
6. **Content Registry** - Centralized command/view/tool/setting registration

**Development Flow:**

1. Check API documentation in `dearts-dev/references/`
2. Reference examples in API manuals and `plugins/builtin/`
3. Only read source code when documentation is unclear
4. Use RAII patterns for automatic resource management
5. Log important operations with appropriate log levels

**Project Goals:**

- Type-safe C++20 framework
- ImHex-style extensibility
- Modern async task management
- Comprehensive API documentation (no source code reading required)
