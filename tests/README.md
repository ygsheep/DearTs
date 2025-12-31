# DearTs Framework 测试使用指南

本指南帮助你快速开始使用 DearTs Framework 的测试套件。

---

## 📋 目录结构

```
tests/
├── unit/              # 单元测试（Google Test）
├── integration/       # 集成测试（Google Test）
├── ui/               # UI 自动化测试（ImGui Test Engine）
└── mocks/            # Mock 和测试辅助
```

---

## 🚀 快速开始

### 1. 构建测试

```bash
# 配置构建（包含测试）
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# 构建所有测试
cmake --build build

# 仅构建测试
cmake --build build --target dearts_unit_tests
cmake --build build --target dearts_ui_tests
```

### 2. 运行测试

#### 运行所有单元测试

```bash
# 使用 CTest
cd build
ctest --verbose

# 或直接运行可执行文件
./bin/dearts_unit_tests
```

#### 运行特定测试

```bash
# 运行特定测试套件
./bin/dearts_unit_tests --gtest_filter=ConfigManager.*

# 运行特定测试用例
./bin/dearts_unit_tests --gtest_filter=ConfigManager.SetGetInteger

# 运行多个测试模式
./bin/dearts_unit_tests --gtest_filter=ConfigManager.*:EventBus.*
```

#### 运行 UI 测试

```bash
# 带窗口运行
./bin/dearts_ui_tests

# 无头模式（CI 服务器）
./bin/dearts_ui_tests --headless

# 详细输出
./bin/dearts_ui_tests --verbose
```

---

## 🧪 编写单元测试

### 基本模板

```cpp
#include <gtest/gtest.h>
#include "core/config/config_manager.h"

using namespace DearTs::Core::Config;

class ConfigManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 每个测试前执行
        mgr = &ConfigManager::instance();
    }

    void TearDown() override {
        // 每个测试后执行
    }

    ConfigManager* mgr;
};

TEST_F(ConfigManagerTest, SetGetInteger) {
    // Arrange（准备）
    const std::string key = "test.value";
    const int expected = 42;

    // Act（执行）
    auto result = mgr->set(key, expected);
    auto value = mgr->get<int>(key);

    // Assert（断言）
    ASSERT_TRUE(result.isOk());
    ASSERT_TRUE(value.isOk());
    EXPECT_EQ(value.unwrap(), expected);
}
```

### 常用断言

```cpp
// 布尔值
EXPECT_TRUE(condition);
ASSERT_FALSE(condition);

// 相等性
EXPECT_EQ(expected, actual);
ASSERT_NE(val1, val2);

// 比较
EXPECT_LT(val1, val2);  // Less than
EXPECT_LE(val1, val2);  // Less or equal
EXPECT_GT(val1, val2);  // Greater than
EXPECT_GE(val1, val2);  // Greater or equal

// 指针
EXPECT_PTR_EQ(ptr1, ptr2);
EXPECT_NULL(ptr);

// 浮点数
EXPECT_FLOAT_EQ(expected, actual);
EXPECT_NEAR(val1, val2, abs_error);

// 异常（如果使用异常）
EXPECT_THROW(statement, exception_type);
ASSERT_ANY_THROW(statement);
```

---

## 🎨 编写 UI 测试

### 基本模板

```cpp
#include "imgui_test_engine/imgui_te_engine.h"

void TestTitleBarButton(ImGuiTestContext* ctx) {
    // 设置参考路径
    ctx->SetRef("DearTsWindow");

    // 点击按钮
    ctx->ItemClick("TitleBar/SettingsButton");

    // 验证窗口可见
    ctx->ItemIsVisible("SettingsWindow");

    // 关闭窗口
    ctx->ItemClick("SettingsWindow/CloseButton");
}

// 注册测试
IM_REGISTER_TEST(ctx, "ui", "title_bar_settings_button")
    ->TestFunc = TestTitleBarButton;
```

### 常用操作

```cpp
// 点击
ctx->ItemClick("MyButton");
ctx->ItemDoubleClick("MyItem");

// 输入
ctx->ItemInputValue("TextInput", "Hello World");
ctx->ItemInputValue("Slider", 123);

// 菜单
ctx->MenuCheck("//File/Save");
ctx->MenuUncheck("//View/Logs");

// 快捷键
ctx->KeyChord(ImGuiMod_Ctrl | ImGuiKey_S);
ctx->KeyDown(ImGuiKey_A);
ctx->KeyUp(ImGuiKey_A);

// 查询
ctx->ItemIsVisible("MyItem");
ctx->ItemExists("MyItem");
ctx->ItemIsChecked("MyCheckbox");

// 等待
ctx->WaitNoEvent(100);
```

---

## 📊 代码覆盖率

### 启用覆盖率

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON
cmake --build build
./build/bin/dearts_unit_tests
```

### 生成覆盖率报告

```bash
# 使用 gcov
gcov build/tests/unit/CMakeFiles/dearts_unit_tests.dir/core/config/*.gcda

# 使用 lcov（推荐）
lcov --capture --directory build --output-file coverage.info
genhtml coverage.info --output-directory coverage_report

# 查看报告
open coverage_report/index.html  # macOS
xdg-open coverage_report/index.html  # Linux
```

---

## 🐛 调试测试

### 详细输出

```bash
# Google Test 详细输出
./bin/dearts_unit_tests --gtest_print_time=1

# 打印所有测试名称
./bin/dearts_unit_tests --gtest_list_tests

# 重复失败的测试
./bin/dearts_unit_tests --gtest_repeat=3 --gtest_break_on_failure
```

### 调试特定测试

```bash
# 仅运行一个测试
./bin/dearts_unit_tests --gtest_filter=ConfigManager.SetGetInteger

# 使用调试器
gdb ./bin/dearts_unit_tests
(gdb) break ConfigManagerTest::SetGetInteger
(gdb) run --gtest_filter=ConfigManager.SetGetInteger
```

---

## 📦 CI/CD 集成

### GitHub Actions 示例

```yaml
name: Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3

      - name: Configure CMake
        run: cmake -B build -DCMAKE_BUILD_TYPE=Debug

      - name: Build
        run: cmake --build build

      - name: Run Tests
        run: ctest --test-dir build --verbose

      - name: Upload Coverage
        if: success()
        uses: codecov/codecov-action@v3
        with:
          files: ./coverage.info
```

---

## 📚 更多资源

- [Google Test Primer](https://google.github.io/googletest/primer.html)
- [Google Test Advanced Guide](https://google.github.io/googletest/advanced.html)
- [ImGui Test Engine Wiki](https://github.com/ocornut/imgui_test_engine/wiki)
- [CMake Testing Guide](https://cmake.org/cmake/help/latest/manual/ctest.1.html)

---

## 🤝 贡献测试

当你添加新功能时，请同时添加测试：

1. **单元测试**：测试核心逻辑
2. **集成测试**：测试组件交互
3. **UI 测试**：测试 UI 交互（如适用）

### 测试命名约定

- 文件：`<component>_test.cpp`
- 测试套件：`<ComponentName>`
- 测试用例：`<MethodName>_<Scenario>`

示例：
```cpp
TEST_F(ConfigManagerTest, SetGetInteger_ReturnsCorrectValue) { }
```

---

**Happy Testing! 🚀**
