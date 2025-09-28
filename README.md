# DearTs

DearTs 是一个基于 SDL2 和 ImGui 的现代化 C++ 应用程序框架，采用分层架构、事件驱动和插件化设计。该项目旨在提供一个灵活、可扩展的应用程序开发平台，特别适用于需要复杂 UI 和高性能图形渲染的桌面应用程序。

## 项目概述

DearTs 提供了一个完整的应用程序框架，集成了以下核心组件：

- **窗口管理**：基于 SDL2 的跨平台窗口系统
- **UI 渲染**：使用 ImGui 构建现代化用户界面
- **事件系统**：全局事件分发机制
- **资源管理**：字体、图像等资源的统一管理
- **插件系统**：动态加载和卸载功能模块
- **音频处理**：基于 SDL2_mixer 的音频支持

## 技术栈

- **语言**: C++20
- **构建系统**: CMake (3.20+)
- **图形库**: SDL2
- **UI框架**: ImGui
- **字体支持**: SDL2_ttf
- **图像支持**: SDL2_image
- **音频支持**: SDL2_mixer

## 项目结构

```
DearTs/
├── build/              # 构建目录
├── core/               # 核心库
│   ├── app/            # 应用管理
│   ├── audio/          # 音频处理
│   ├── events/         # 事件系统
│   ├── input/          # 输入处理
│   ├── patterns/       # 设计模式
│   ├── render/         # 渲染系统
│   ├── resource/       # 资源管理
│   ├── utils/          # 工具类
│   └── window/         # 窗口管理
├── examples/           # 示例代码
├── lib/                # 第三方库和核心库
│   ├── libdearts/      # DearTs 核心库
│   └── third_party/    # 第三方依赖
├── main/               # 主程序入口
├── plugins/            # 插件系统
├── resources/          # 资源文件
└── CMakeLists.txt      # 构建配置
```

## 快速开始

### 环境要求

- C++20 兼容编译器 (MSVC, GCC, Clang)
- CMake 3.20 或更高版本
- Visual Studio 2022 (Windows) 或相应开发环境

### 构建步骤

```bash
# 克隆项目
git clone <repository-url>
cd DearTs

# 创建构建目录
mkdir build && cd build

# 配置项目 (Windows with Visual Studio)
cmake -G "Visual Studio 17 2022" -A x64 ..

# 构建项目
cmake --build . --config Debug
```

### 运行应用

构建完成后，可执行文件位于：
- Windows: `build/Debug/DearTs.exe`

## 架构设计

### 分层架构

```
Application Layer (main/)
├── Core Library (core/)
│   ├── Application Management
│   ├── Event System
│   ├── Input Handling
│   ├── Resource Management
│   └── Window System
├── Plugin System (plugins/)
└── Third-Party Integration (lib/third_party/)
    ├── SDL2 (graphics/input)
    ├── ImGui (UI)
    └── Font Libraries
```

### 核心设计模式

1. **单例模式**：核心管理器采用单例模式确保全局唯一实例
2. **观察者模式**：通过事件系统实现组件间解耦
3. **适配器模式**：将不同接口适配到统一的抽象层

## 配置选项

CMake 构建时可使用以下选项：

- `-DDEARTS_BUILD_TESTS=ON`：构建测试 (默认: OFF)
- `-DDEARTS_BUILD_DOCS=ON`：构建文档 (默认: OFF)
- `-DDEARTS_BUILD_EXAMPLES=ON`：构建示例 (默认: OFF)
- `-DDEARTS_ENABLE_LOGGING=OFF`：禁用日志 (默认: ON)
- `-DDEARTS_ENABLE_PROFILING=ON`：启用性能分析 (默认: OFF)

## 插件系统

DearTs 支持动态插件加载：

1. 插件以动态库形式存在 (.dll/.so)
2. 插件需实现 `IPlugin` 接口
3. 支持插件依赖管理和自动加载
4. 提供插件生命周期管理

## 开发指南

### 代码规范

- 遵循 C++20 标准
- 使用 RAII 资源管理
- 命名规范：使用命名空间和驼峰命名法
- 内存管理：智能指针优先

### 扩展功能

1. **添加新窗口**：继承 `Window` 类并实现相应接口
2. **自定义渲染**：通过 `IRenderer` 接口扩展渲染功能
3. **事件处理**：定义新事件类型并注册事件处理器
4. **资源管理**：扩展 `ResourceManager` 支持新资源类型

## 许可证

该项目采用 [MIT 许可证](LICENSE)。

## 贡献

欢迎提交 Issue 和 Pull Request 来改进项目。

1. Fork 项目
2. 创建功能分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 开启 Pull Request