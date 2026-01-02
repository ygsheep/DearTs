# Live2D 示例程序

## 概述

这是一个独立的 Live2D 插件演示程序，使用 **OpenGL 3.3 Core** 作为渲染后端。

### 特点

- ✅ **独立程序**：不依赖 DearTs 主程序
- ✅ **OpenGL 后端**：使用 OpenGL 3.3 Core Profile
- ✅ **Live2D 集成**：完整的 Live2D Cubism SDK 集成
- ✅ **ImGui 界面**：现代化的 ImGui 用户界面
- ✅ **跨平台**：支持 Windows 和 Linux

## 技术栈

| 组件 | 库/框架 |
|------|---------|
| 窗口系统 | SDL3 |
| 渲染后端 | OpenGL 3.3 Core |
| UI 框架 | ImGui 1.90 + ImGui_ImplOpenGL3 |
| Live2D SDK | Cubism SDK 4.x |
| OpenGL 加载器 | GLAD |
| 日志系统 | DearTs Logger |

## 编译方法

### 1. 配置项目

```bash
cmake -B build -DBUILD_EXAMPLES=ON
```

### 2. 编译

```bash
cmake --build build --target live2d_example --config Release
```

### 3. 运行

```bash
./build/bin/live2d_example.exe        # Windows
./build/bin/live2d_example            # Linux
```

## 目录结构

```
examples/live2d_example/
├── main.cpp                    # 主入口
├── opengl_application.hpp       # OpenGL 应用程序头文件
├── opengl_application.cpp       # OpenGL 应用程序实现
├── CMakeLists.txt              # 构建配置
└── README.md                   # 本文件
```

## 依赖要求

### Windows
- Visual Studio 2022 (MSVC 19.3+)
- Windows 10/11
- OpenGL 3.3+ 驱动

### Linux
- GCC 11+ 或 Clang 13+
- Mesa 20.0+ (OpenGL 3.3 支持)
- Wayland 或 X11

## 功能说明

### 已实现功能

- ✅ SDL3 窗口创建和管理
- ✅ OpenGL 3.3 上下文初始化
- ✅ ImGui 集成（SDL3 + OpenGL3 后端）
- ✅ 基本渲染循环
- ✅ 时间和帧率管理

### 待实现功能

- ⏳ Live2D 插件集成
- ⏳ Live2D 模型加载和渲染
- ⏳ Live2D 设置界面
- ⏳ 模型切换功能
- ⏳ 动画控制

## 使用说明

1. **启动程序**：运行 `live2d_example`
2. **界面说明**：
   - 主窗口显示 Live2D 模型
   - 右侧面板显示设置选项
3. **快捷键**：
   - `ESC` 或 `F11`：退出程序

## 故障排除

### 问题：OpenGL 上下文创建失败

**解决方案**：
- 更新显卡驱动
- 检查 OpenGL 版本：`glxinfo` (Linux) 或 GPU-Z (Windows)

### 问题：Live2D 模型加载失败

**解决方案**：
- 确认模型文件存在于 `resources/live2d/` 目录
- 检查模型文件格式是否正确

## 许可证

本示例程序遵循 MIT 许可证。

## 联系方式

- 项目主页：[DearTs Framework](https://github.com/your-repo)
- 问题反馈：[Issues](https://github.com/your-repo/issues)
