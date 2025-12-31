# DearTs Framework 测试计划

## 📋 概述

本文档描述了 DearTs Framework 的完整测试策略，包括单元测试、集成测试和 UI 自动化测试。

**测试框架选择：**
- **Google Test (GTest)**：单元测试和集成测试
- **ImGui Test Engine**：UI 自动化测试

**测试目标：**
- 核心工具类：80%+ 覆盖率
- 事件系统：70%+ 覆盖率
- 插件系统：60%+ 覆盖率
- UI 组件：关键路径覆盖

---

## 🏗️ 测试架构

### 目录结构

```
tests/
├── CMakeLists.txt                    # 测试构建配置
├── README.md                         # 测试使用说明
├── unit/                             # 单元测试
│   ├── core/
│   │   ├── plugin/                  # 插件系统测试
│   │   │   ├── plugin_info_test.cpp
│   │   │   ├── plugin_wrapper_test.cpp
│   │   │   └── plugin_manager_test.cpp
│   │   ├── config/                  # 配置管理测试
│   │   │   ├── config_manager_test.cpp
│   │   │   ├── config_scope_test.cpp
│   │   │   └── config_persistence_test.cpp
│   │   ├── events/                  # 事件系统测试
│   │   │   ├── event_bus_test.cpp
│   │   │   └── event_token_test.cpp
│   │   ├── tasks/                   # 任务管理测试
│   │   │   ├── task_manager_test.cpp
│   │   │   └── task_test.cpp
│   │   └── content/                 # Content Registry 测试
│   │       ├── commands_test.cpp
│   │       ├── views_test.cpp
│   │       ├── tools_test.cpp
│   │       └── settings_test.cpp
│   └── utils/                       # 工具类测试
│       ├── result_test.cpp
│       └── variant_helpers_test.cpp
│
├── integration/                      # 集成测试
│   ├── plugin_lifecycle/            # 插件生命周期测试
│   │   └── full_lifecycle_test.cpp
│   ├── event_flow/                  # 事件流测试
│   │   └── pub_sub_chain_test.cpp
│   └── config_persistence/          # 配置持久化测试
│       └── json_io_test.cpp
│
├── ui/                               # UI 自动化测试
│   ├── test_runner.cpp              # ImGui Test Engine 运行器
│   ├── views/                       # View 组件测试
│   │   ├── title_bar_test.cpp
│   │   ├── data_inspector_test.cpp
│   │   └── command_palette_test.cpp
│   ├── commands/                    # 命令测试
│   │   └── command_invocation_test.cpp
│   └── interactions/                # UI 交互测试
│       ├── menu_navigation_test.cpp
│       └── shortcut_test.cpp
│
└── mocks/                            # Mock 和测试辅助
    ├── mock_plugin.hpp
    ├── mock_config.json
    └── test_helpers.hpp
```

---

## 🎯 Phase 1：基础设施搭建

### 1.1 CMake 配置

**目标**：集成 Google Test 和 ImGui Test Engine

**任务清单**：
- [ ] 添加 Google Test 子目录
- [ ] 配置 Google Test 发现和运行
- [ ] 创建测试可执行文件
- [ ] 配置 ImGui Test Engine
- [ ] 集成 CTest

**CMakeLists.txt 结构**：
```cmake
# tests/CMakeLists.txt

# == Google Test 集成 ==
add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/googletest)

# == 单元测试可执行文件 ==
add_executable(dearts_unit_tests
    # unit test sources...
)

target_link_libraries(dearts_unit_tests
    PRIVATE
    gtest
    gtest_main
    # 链接需要测试的库
)

# == UI 测试可执行文件 ==
add_executable(dearts_ui_tests
    ui/test_runner.cpp
    # UI test sources...
)

target_link_libraries(dearts_ui_tests
    PRIVATE
    imgui_test_engine
    # 链接 DearTs 库
)

# == CTest 集成 ==
include(GoogleTest)
gtest_discover_tests(dearts_unit_tests)
```

**预计时间**：1-2 天

---

## 🎯 Phase 2：核心单元测试

### 2.1 ConfigManager 测试

**文件**：`tests/unit/core/config/config_manager_test.cpp`

**测试用例**：
```cpp
// 基本操作
TEST(ConfigManager, SetGetInteger) { }
TEST(ConfigManager, SetGetString) { }
TEST(ConfigManager, SetGetDouble) { }
TEST(ConfigManager, SetGetBool) { }

// 类型转换
TEST(ConfigManager, TypeMismatchReturnsError) { }
TEST(ConfigManager, InvalidKeyReturnsError) { }

// ConfigScope
TEST(ConfigScope, PrefixesKeys) { }
TEST(ConfigScope, NestedScopes) { }

// 持久化
TEST(ConfigManager, SaveToJson) { }
TEST(ConfigManager, LoadFromJson) { }
TEST(ConfigManager, LoadInvalidJson) { }

// 元数据
TEST(ConfigManager, RegisterMeta) { }
TEST(ConfigManager, ValidateCallback) { }
TEST(ConfigManager, ChangeCallback) { }
```

**预计时间**：1 天

---

### 2.2 EventBus 测试

**文件**：`tests/unit/core/events/event_bus_test.cpp`

**测试用例**：
```cpp
// 订阅/发布
TEST(EventBus, SubscribeAndPublish) { }
TEST(EventBus, MultipleSubscribers) { }
TEST(EventBus, Unsubscribe) { }

// Token RAII
TEST(EventBus, TokenAutoUnsubscribe) { }
TEST(EventBus, TokenMove) { }
TEST(EventBus, TokenCopyDeleted) { }

// 事件数据
TEST(EventBus, EventWithData) { }
TEST(EventBus, EventWithComplexData) { }

// 线程安全（可选）
TEST(EventBus, ConcurrentPublish) { }
```

**预计时间**：1 天

---

### 2.3 Result<T,E> 测试

**文件**：`tests/unit/core/utils/result_test.cpp`

**测试用例**：
```cpp
// 基本操作
TEST(Result, OkValue) { }
TEST(Result, ErrValue) { }
TEST(Result, IsOkIsErr) { }

// unwrap/unwrap_or
TEST(Result, UnwrapOk) { }
TEST(Result, UnwrapErr) { }
TEST(Result, UnwrapOrDefault) { }

// map
TEST(Result, MapOk) { }
TEST(Result, MapErr) { }
TEST(Result, MapBoth) { }

// 链式调用
TEST(Result, ChainedMaps) { }
```

**预计时间**：0.5 天

---

### 2.4 PluginManager 测试

**文件**：`tests/unit/core/plugin/plugin_manager_test.cpp`

**Mock 插件**：
```cpp
class MockPlugin : public IPlugin {
    // 用于测试的简单插件实现
};
```

**测试用例**：
```cpp
// 插件注册
TEST(PluginManager, AddBuiltin) { }
TEST(PluginManager, AddDuplicateName) { }

// 插件状态
TEST(PluginManager, LoadSuccess) { }
TEST(PluginManager, LoadFailure) { }
TEST(PluginManager, Enable) { }
TEST(PluginManager, Disable) { }
TEST(PluginManager, Unload) { }

// API 版本
TEST(PluginManager, ApiVersionMismatch) { }
TEST(PluginManager, ApiVersionCompatible) { }

// 查询
TEST(PluginManager, GetByName) { }
TEST(PluginManager, ListAll) { }
```

**预计时间**：1.5 天

---

### 2.5 TaskManager 测试

**文件**：`tests/unit/core/tasks/task_manager_test.cpp`

**测试用例**：
```cpp
// 任务启动
TEST(TaskManager, LaunchNormalTask) { }
TEST(TaskManager, LaunchBackgroundTask) { }
TEST(TaskManager, LaunchBlockingTask) { }

// 进度更新
TEST(TaskManager, TaskProgress) { }
TEST(TaskManager, TaskCompletion) { }

// 取消
TEST(TaskManager, CancelTask) { }
TEST(TaskManager, CancelCompletedTask) { }

// 结果
TEST(TaskManager, TaskSuccessResult) { }
TEST(TaskManager, TaskErrorResult) { }
```

**预计时间**：1 天

---

## 🎯 Phase 3：集成测试

### 3.1 插件完整生命周期

**文件**：`tests/integration/plugin_lifecycle/full_lifecycle_test.cpp`

**测试场景**：
```cpp
TEST(PluginLifecycle, FullCycle) {
    // 1. 创建插件
    // 2. add_builtin()
    // 3. load()
    // 4. enable()
    // 5. disable()
    // 6. unload()
    // 验证每个状态转换
}
```

**预计时间**：1 天

---

### 3.2 事件流测试

**文件**：`tests/integration/event_flow/pub_sub_chain_test.cpp`

**测试场景**：
```cpp
TEST(EventFlow, PublisherToMultipleSubscribers) {
    // 插件 A 发布事件
    // 插件 B 和 C 订阅并响应
    // 验证两个插件都收到事件
}

TEST(EventFlow, EventBusUnsubscribeOnPluginUnload) {
    // 验证插件卸载时自动取消订阅
}
```

**预计时间**：0.5 天

---

### 3.3 配置持久化测试

**文件**：`tests/integration/config_persistence/json_io_test.cpp`

**测试场景**：
```cpp
TEST(ConfigPersistence, SaveAndReload) {
    // 1. 设置配置
    // 2. 保存到 JSON
    // 3. 清空配置
    // 4. 从 JSON 加载
    // 5. 验证配置恢复
}
```

**预计时间**：0.5 天

---

## 🎯 Phase 4：UI 自动化测试

### 4.1 ImGui Test Engine 集成

**文件**：`tests/ui/test_runner.cpp`

**基本结构**：
```cpp
#include "imgui_test_engine/imgui_te_engine.h"

void RegisterTests(ImGuiTestEngine* engine) {
    // 注册所有 UI 测试
}

int main(int argc, char** argv) {
    // 初始化 ImGui Test Engine
    // 运行测试
}
```

**预计时间**：1 天

---

### 4.2 View 组件测试

**文件**：`tests/ui/views/title_bar_test.cpp`

**测试用例**：
```cpp
void TestTitleBarSettingsButton(ImGuiTestContext* ctx) {
    ctx->SetRef("DearTsWindow");
    ctx->ItemClick("TitleBar/SettingsButton");
    ctx->ItemIsVisible("SettingsWindow");
}

void TestTitleBarShortcuts(ImGuiTestContext* ctx) {
    // 测试 Ctrl+, 打开设置
    ctx->KeyChord(ImGuiMod_Ctrl | ImGuiKey_Comma);
    ctx->ItemIsVisible("SettingsWindow");
}
```

**预计时间**：1-2 天

---

### 4.3 命令面板测试

**文件**：`tests/ui/views/command_palette_test.cpp`

**测试用例**：
```cpp
void TestCommandPaletteOpen(ImGuiTestContext* ctx) {
    ctx->KeyChord(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_P);
    ctx->ItemIsVisible("CommandPalette");
    ctx->ItemIsVisible("CommandPalette/Input");
}

void TestCommandExecution(ImGuiTestContext* ctx) {
    ctx->SetRef("CommandPalette");
    ctx->ItemInputValue("Input", "hello world");
    ctx->ItemClick("Hello World Command");
    // 验证命令执行结果
}
```

**预计时间**：1 天

---

## 🎯 Phase 5：CI/CD 集成

### 5.1 GitHub Actions 配置

**文件**：`.github/workflows/tests.yml`

```yaml
name: Tests

on:
  push:
    branches: [develop, main]
  pull_request:
    branches: [develop, main]

jobs:
  unit-tests:
    runs-on: ${{ matrix.os }}
    strategy:
      matrix:
        os: [ubuntu-latest, windows-latest, macos-latest]
        build_type: [Debug, Release]

    steps:
      - uses: actions/checkout@v3

      - name: Configure CMake
        run: cmake -B build -DCMAKE_BUILD_TYPE=${{ matrix.build_type }}

      - name: Build
        run: cmake --build build --config ${{ matrix.build_type }}

      - name: Run Unit Tests
        run: ctest --test-dir build -C ${{ matrix.build_type }} --output-on-failure

      - name: Upload Test Results
        if: always()
        uses: actions/upload-artifact@v3
        with:
          name: test-results-${{ matrix.os }}-${{ matrix.build_type }}
          path: build/Testing/

  ui-tests:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3

      - name: Configure CMake
        run: cmake -B build -DCMAKE_BUILD_TYPE=Release

      - name: Build
        run: cmake --build build --config Release

      - name: Run UI Tests (Headless)
        run: ./build/bin/dearts_ui_tests --headless --verbose

      - name: Upload Screenshots
        if: failure()
        uses: actions/upload-artifact@v3
        with:
          name: ui-test-screenshots
          path: tests/ui/screenshots/
```

**预计时间**：1 天

---

## 📊 测试覆盖率

### 目标覆盖率

| 组件 | 目标覆盖率 | 优先级 |
|------|----------|--------|
| ConfigManager | 85% | 🔴 高 |
| EventBus | 80% | 🔴 高 |
| Result<T,E> | 90% | 🔴 高 |
| PluginManager | 70% | 🟡 中 |
| TaskManager | 70% | 🟡 中 |
| ContentRegistry | 60% | 🟡 中 |
| UI 组件 | 关键路径 | 🟢 低 |

### 覆盖率工具

**使用 GCC/Clang gcov**：
```cmake
# 启用覆盖率
option(ENABLE_COVERAGE "Enable coverage reporting" ON)

if(ENABLE_COVERAGE)
    target_compile_options(dearts_unit_tests PRIVATE --coverage)
    target_link_options(dearts_unit_tests PRIVATE --coverage)
endif()
```

**生成报告**：
```bash
# 运行测试
./build/bin/dearts_unit_tests

# 生成覆盖率
gcov tests/unit/core/config/*.gcda
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_report
```

---

## 📝 测试编写指南

### 单元测试模板

```cpp
#include <gtest/gtest.h>
#include "core/config/config_manager.h"

using namespace DearTs::Core::Config;

class ConfigManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 每个测试前执行
        mgr = &ConfigManager::instance();
        mgr->clear(); // 清空配置
    }

    void TearDown() override {
        // 每个测试后执行
    }

    ConfigManager* mgr;
};

TEST_F(ConfigManagerTest, SetGetInteger) {
    // Arrange
    const std::string key = "test.value";
    const int expected = 42;

    // Act
    auto result = mgr->set(key, expected);
    auto value = mgr->get<int>(key);

    // Assert
    ASSERT_TRUE(result.isOk());
    ASSERT_TRUE(value.isOk());
    EXPECT_EQ(value.unwrap(), expected);
}

TEST_F(ConfigManagerTest, GetNonExistentKeyReturnsError) {
    // Act
    auto value = mgr->get<int>("non.existent.key");

    // Assert
    EXPECT_TRUE(value.isErr());
}
```

### UI 测试模板

```cpp
#include "imgui_test_engine/imgui_te_engine.h"

void TestMyViewInteraction(ImGuiTestContext* ctx) {
    // 设置引用路径
    ctx->SetRef("DearTsWindow/MyView");

    // 测试按钮点击
    ctx->ItemClick("MyButton");

    // 验证结果
    ctx->ItemIsVisible("ResultLabel");

    // 测试输入
    ctx->ItemInputValue("TextInput", "Hello World");

    // 测试菜单
    ctx->MenuCheck("//File/Save");
}
```

---

## ⏱️ 总时间估算

| Phase | 任务 | 预计时间 |
|-------|------|---------|
| Phase 1 | 基础设施搭建 | 1-2 天 |
| Phase 2 | 核心单元测试 | 5-6 天 |
| Phase 3 | 集成测试 | 2 天 |
| Phase 4 | UI 自动化测试 | 3-4 天 |
| Phase 5 | CI/CD 集成 | 1 天 |
| **总计** | | **12-15 天** |

---

## 🚀 快速开始

### 运行所有测试

```bash
# 构建
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# 运行单元测试
cd build
ctest --verbose

# 或直接运行
./bin/dearts_unit_tests

# 运行 UI 测试
./bin/dearts_ui_tests
```

### 运行特定测试

```bash
# 运行特定测试套件
./bin/dearts_unit_tests --gtest_filter=ConfigManager.*

# 运行特定测试用例
./bin/dearts_unit_tests --gtest_filter=ConfigManager.SetGetInteger
```

---

## 📚 参考资料

- [Google Test Documentation](https://google.github.io/googletest/)
- [ImGui Test Engine Wiki](https://github.com/ocornut/imgui_test_engine/wiki)
- [CMake Testing with CTest](https://cmake.org/cmake/help/latest/manual/ctest.1.html)

---

## 🔄 后续优化

### 短期（1-2 个月）
- [ ] 完成核心单元测试
- [ ] 集成 CI/CD
- [ ] 达到 70%+ 覆盖率

### 中期（3-6 个月）
- [ ] 添加性能测试
- [ ] 添加模糊测试
- [ ] 添加内存泄漏检测

### 长期（6-12 个月）
- [ ] 测试文档化
- [ ] 测试驱动开发（TDD）实践
- [ ] 持续改进测试覆盖率

---

**最后更新**：2025-12-30
**负责人**：DearTs Team
