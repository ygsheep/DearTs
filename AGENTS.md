# AGENTS.md - DearTs Framework Developer Guide

Guidelines for AI agents working on the DearTs C++20 framework (SDL3 + ImGui + plugin system).

**Output Format**: Reply in Chinese (中文), end with "喵！"

## Build Commands

```bash
# Configure (always start with this)
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build all
cmake --build build --config Release

# Build specific target
cmake --build build --target ChatManager
cmake --build build --target DearTsApp

# Build with parallel jobs (Linux)
cmake --build build -j$(nproc)

# Clean build
cmake --build build --target clean
```

## Test Commands

```bash
# Build tests
cmake -B build -DBUILD_TESTING=ON
cmake --build build

# Run all tests
ctest --test-dir build --verbose

# Run specific test executable
./build/tests/dearts_unit_tests
./build/tests/dearts_integration_tests

# Run single test (Google Test syntax)
./build/tests/dearts_unit_tests --gtest_filter="ConfigManagerTest.SetGetInteger*"
./build/tests/dearts_unit_tests --gtest_filter="*EventBus*"

# Run tests with pattern
./build/tests/dearts_unit_tests --gtest_filter="*Plugin*"
```

## Code Style

### General
- **C++20 standard** (CMakeLists.txt enforces this)
- 4-space indentation (no tabs)
- Max line length: 100 characters
- Use `//` for single-line, `/* */` for multi-line comments
- All source files must use UTF-8 encoding

### Naming Conventions

```cpp
// Classes/Structs: PascalCase
class PluginManager { };
struct PluginInfo { };

// Functions/Methods: camelCase
void initializeWindow();
Result<void, std::string> loadPlugin();

// Member variables: m_ prefix + camelCase
std::string m_pluginName;
int m_bufferSize;

// Constants: UPPER_SNAKE_CASE or kCamelCase
constexpr int MAX_BUFFER_SIZE = 1024;
constexpr int kDefaultTimeout = 30;

// Namespaces: lowercase, nested with ::
namespace DearTs::Core::Plugin { }

// Macros: UPPER_SNAKE_CASE with prefix
#define DEARTS_PLUGIN_EXPORT
```

### Includes Order

```cpp
// 1. Header file's own include (for .cpp files)
#include "core/plugin/plugin.h"

// 2. Project headers (alphabetical)
#include "core/config/config_manager.h"
#include "core/event/event_bus.h"
#include "core/result.h"

// 3. Third-party headers
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <imgui.h>

// 4. Standard library headers (alphabetical)
#include <memory>
#include <string>
#include <vector>
```

### Error Handling

**ALWAYS use `Result<T, E>` for error handling, never exceptions:**

```cpp
#include "core/result.h"

Result<void, std::string> on_load() {
    if (!init()) {
        return Result<void, std::string>::err("Initialization failed");
    }
    return Result<void, std::string>::ok();
}

// Usage
auto result = parseConfig(data);
if (result.isOk()) {
    int value = result.unwrap();
} else {
    LOG_ERROR("Failed: {}", result.error());
}
```

### RAII Patterns

```cpp
// EventBus subscription (auto-unsubscribe on destruction)
EventBus::Token m_token = EventBus::instance().subscribe<Event>(handler);

// ConfigScope (auto-prefix management)
ConfigScope config("myplugin");
config.set("key", value);  // Stored as "myplugin.key"

// Smart pointers for ownership
std::unique_ptr<IPlugin> plugin;
std::shared_ptr<Task> task;
```

## Architecture Patterns

### Plugin Development

```cpp
class MyPlugin : public IPlugin {
public:
    PluginInfo get_info() const override {
        return PluginInfo{
            .name = "MyPlugin",
            .author = "Developer",
            .description = "Plugin description",
            .version = "1.0.0",
            .api_version = "1.0.0"
        };
    }

    Result<void, std::string> on_load() override {
        ContentRegistry::Commands::register_handler(...);
        ContentRegistry::Views::add<MyView>();
        return Result<void, std::string>::ok();
    }

    void on_unload() override {
        // Cleanup (RAII handles most)
    }

private:
    ConfigScope m_config{"myplugin"};
    EventBus::Token m_eventToken;
};
```

### Key Directories

- `/core/` - Framework core (plugin system, events, config, tasks)
- `/plugins/` - Plugin implementations
- `/main/` - Application entry points (gui, chatmanager)
- `/lib/` - Shared libraries (liblogger)
- `/tests/` - Unit, integration, and UI tests
- `/third_party/` - External dependencies

## Testing Guidelines

```cpp
#include <gtest/gtest.h>

class MyTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup before each test
    }
    void TearDown() override {
        // Cleanup after each test
    }
};

TEST_F(MyTest, TestName_DescribesBehavior) {
    // Arrange
    const int expected = 42;
    
    // Act
    auto result = functionUnderTest();
    
    // Assert
    ASSERT_TRUE(result.isOk());
    EXPECT_EQ(result.unwrap(), expected);
}
```

## Important Notes

1. **Never use exceptions for control flow** - Use `Result<T, E>`
2. **Always use prefix for commands/settings**: `"myplugin.command_name"`
3. **Check API docs first**: `dearts-dev/references/*.md` has comprehensive manuals
4. **Platform-specific code**: Use `#ifdef _WIN32` / `#else` for platform differences
5. **Logging**: Use `LOG_INFO()`, `LOG_ERROR()`, `LOG_DEBUG()` macros from liblogger
6. **Thread safety**: Most core systems are thread-safe, check individual class docs

## Nix Development

```bash
# Enter development shell
nix-shell

# Run application
./run.sh
# or
./build/bin/ChatManager
```

When fixing build errors on Linux, check platform-specific code uses proper conditionals (`#ifdef _WIN32`).
