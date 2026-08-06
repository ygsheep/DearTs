# Toast Notification 示例程序

## 概述

这是一个展示如何使用 Toast Notification 插件和任务系统集成的独立示例程序。

## 功能特性

- ✅ **4种示例场景** - 基本消息、文件操作、网络请求、模型加载
- ✅ **手动测试按钮** - 随时显示各种类型的 Toast
- ✅ **自动演示** - 每个场景都会自动播放一系列 Toast
- ✅ **任务系统集成** - 展示任务系统与气泡插件的完美集成
- ✅ **统计信息** - 实时显示当前 Toast 数量和运行中的任务数
- ✅ **简洁代码** - 展示最简单的集成方式

## 示例场景

### 1. 基本消息
直接展示所有4种类型的 Toast（信息、成功、警告、错误）

### 2. 文件操作（使用任务系统）
模拟文件读取、保存、删除、复制等操作，展示：
- **文件读取** → Info 气泡（开始）→ Success 气泡（完成）
- **文件保存** → Info 气泡（开始）→ Success 气泡（完成）
- **文件删除** → Info 气泡（开始）→ Error 气泡（失败）
- **文件复制** → Info 气泡（开始）→ Warning 气泡（取消）

### 3. 网络请求（使用任务系统）
模拟网络 GET、POST、上传、下载等操作，展示：
- **GET 请求** → Info 气泡（开始）→ Success 气泡（完成）
- **POST 请求** → Info 气泡（开始）→ Success 气泡（完成）
- **上传文件** → Info 气泡（开始）→ Error 气泡（失败）
- **下载文件** → Info 气泡（开始）→ Success 气泡（完成）

### 4. 模型加载（使用任务系统）
模拟 Live2D 模型加载流程，展示：
- **加载模型** → Info 气泡（开始）→ Success 气泡（完成）
- **加载纹理** → Info 气泡（开始）→ Success 气泡（完成）
- **加载动画** → Info 气泡（开始）→ Error 气泡（失败）
- **加载物理** → Info 气泡（开始）→ Warning 气泡（取消）

## 任务系统集成

本示例展示了如何使用任务系统与气泡插件集成：

### 创建任务
```cpp
auto task = TaskManager::instance().launch(
    "任务名称",
    [](const std::atomic<bool>& should_cancel) {
        // 执行任务...
        // 检查取消标志
        if (should_cancel) return;
        
        // 任务完成
    },
    TaskType::Background
);
```

### 自动气泡显示
当任务状态改变时，ToastPlugin 会自动显示对应的气泡：
- 任务开始 → Info 气泡 "任务开始 - 正在执行: XXX"
- 任务完成 → Success 气泡 "任务完成 - XXX 已成功完成 (耗时: XXXms)"
- 任务失败 → Error 气泡 "任务失败 - XXX 失败: XXX (耗时: XXXms)"
- 任务取消 → Warning 气泡 "任务已取消 - XXX 已被取消 (耗时: XXXms)"

### 应用场景映射

| 场景 | 开始 | 成功 | 失败 | 取消 |
|--------|------|------|------|------|
| 文件读取 | Info | Success | Error | Warning |
| 文件保存 | Info | Success | Error | Warning |
| 文件删除 | Info | Success | Error | Warning |
| 文件复制 | Info | Success | Error | Warning |
| GET 请求 | Info | Success | Error | Warning |
| POST 请求 | Info | Success | Error | Warning |
| 上传文件 | Info | Success | Error | Warning |
| 下载文件 | Info | Success | Error | Warning |
| 加载模型 | Info | Success | Error | Warning |
| 加载纹理 | Info | Success | Error | Warning |
| 加载动画 | Info | Success | Error | Warning |
| 加载物理 | Info | Success | Error | Warning |

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
2. **观察效果** - 每个场景会自动执行一系列操作
3. **手动测试** - 使用手动测试按钮随时显示不同类型的 Toast
4. **查看统计** - 观察当前 Toast 数量和运行中的任务数

## 代码示例

### 基本使用
```cpp
// 显示信息
ToastManager::instance().info("提示", "这是一条信息提示");

// 显示成功
ToastManager::instance().success("成功", "操作已成功完成");

// 显示警告
ToastManager::instance().warning("警告", "请注意可能存在的问题");

// 显示错误
ToastManager::instance().error("错误", "操作失败，请重试");
```

### 使用任务系统
```cpp
// 创建文件读取任务
auto task = TaskManager::instance().launch(
    "读取文件",
    [](const std::atomic<bool>& should_cancel) {
        // 读取文件...
        // 检查取消标志
        if (should_cancel) return;
        
        // 完成读取
    },
    TaskType::Background
);

// 任务会自动触发气泡显示
// 开始时：Info 气泡
// 完成时：Success 气泡
// 失败时：Error 气泡
// 取消时：Warning 气泡
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

1. **头文件引入** - 需要包含 `toast_manager.hpp` 和 `core/tasks/task_manager.h`
2. **更新和渲染** - 在主循环中调用 `update()` 和 `render()`
3. **命名空间** - 使用 `DearTs::Plugins::Toast` 或 `using namespace`
4. **任务创建** - 使用 `TaskManager::instance().launch()` 创建任务
5. **自动气泡** - 任务状态变化会自动触发气泡显示

## 扩展建议

尝试修改示例代码：
- 添加新的场景
- 自定义 Toast 配置（动画速度、显示时长等）
- 实现点击 Toast 的交互
- 添加更多类型的通知
- 实现任务进度更新

## 相关文档

- [Toast Notification Plugin 使用指南](../../plugins/toast_notification/README.md)
- [任务系统文档](../../core/tasks/README.md)
- [事件系统文档](../../core/event/README.md)
