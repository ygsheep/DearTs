# Toast Notification 示例程序

## 概述

这是一个展示如何使用 Toast Notification 插件的独立示例程序。

## 功能特性

- ✅ **4种示例场景** - 基本消息、文件操作、表单验证、进度反馈
- ✅ **手动测试按钮** - 随时显示各种类型的 Toast
- ✅ **自动演示** - 每个场景都会自动播放一系列 Toast
- ✅ **统计信息** - 实时显示当前 Toast 数量
- ✅ **简洁代码** - 展示最简单的集成方式

## 示例场景

### 1. 基本消息
自动展示所有4种类型的 Toast（信息、成功、警告、错误）

### 2. 文件操作
模拟文件保存和加载场景，展示成功和失败状态

### 3. 表单验证
模拟表单验证流程，展示警告和成功状态

### 4. 进度反馈
模拟长时间操作的进度提示

## 编译和运行

### 编译

```bash
# 在 build 目录中
cmake --build . --config Debug --target toast_example
```

### 运行

```bash
# Windows
./build/examples/toast_example/Debug/toast_example.exe

# Linux/macOS
./build/examples/toast_example/toast_example
```

## 使用方法

1. **选择场景** - 点击顶部按钮选择要演示的场景
2. **手动测试** - 使用手动测试按钮随时显示不同类型的 Toast
3. **观察统计** - 查看当前 Toast 数量

## 代码示例

### 基本使用

```cpp
#include "toast_manager.hpp"

// 显示信息
ToastManager::instance().info("提示", "这是一条信息");

// 显示成功
ToastManager::instance().success("成功", "操作已完成");

// 显示警告
ToastManager::instance().warning("警告", "请注意");

// 显示错误
ToastManager::instance().error("错误", "操作失败");

// 关闭所有
ToastManager::instance().close_all();
```

### 在主循环中集成

```cpp
void main_loop() {
    while (running) {
        float delta_time = get_delta_time();

        // 更新 ToastManager
        ToastManager::instance().update(delta_time);

        // 渲染 ImGui
        ImGui::NewFrame();
        // ... 渲染其他 UI ...

        // 渲染 Toast（在 ImGui::Render 之后）
        ToastManager::instance().render();

        ImGui::Render();
        // ... SDL 渲染 ...
    }
}
```

## 学习要点

1. **头文件引入** - 只需包含 `toast_manager.hpp`
2. **更新和渲染** - 在主循环中调用 `update()` 和 `render()`
3. **命名空间** - 使用 `DearTs::Plugins::Toast` 或 `using namespace`
4. **简单 API** - 一行代码即可显示 Toast

## 扩展建议

尝试修改示例代码：
- 添加新的场景
- 自定义 Toast 配置（动画速度、显示时长等）
- 实现点击 Toast 的交互
- 添加更多类型的通知

## 相关文档

- [Toast Notification Plugin 使用指南](../../plugins/toast_notification/README.md)
- [Toast API 手册](../../plugins/toast_notification/README.md#api-参考)
