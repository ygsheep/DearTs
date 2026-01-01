# DearTs Framework - FFmpeg 集成需求文档

## 项目概述
为 DearTs Framework 添加 FFmpeg 支持，实现视频/音频转码、播放和元数据获取功能。

## 系统约束

### 技术栈约束
!> **强制要求**：必须使用 vcpkg 包管理器集成 FFmpeg，禁止使用预编译的 ffmpeg.exe
!> **平台支持**：必须同时支持 Windows 11 和 Linux
!> **编译器**：C++20 (MSVC 2022 / GCC 11+ / Clang 13+)
!> **SDL3 集成**：播放器必须基于 SDL3，项目已使用 SDL3，无需额外安装

### 架构约束
!> **静态链接**：所有依赖库必须静态链接（包括 FFmpeg）
!> **错误处理**：必须使用 `Result<T, E>` 模式，禁止使用异常
!> **日志系统**：使用项目现有的 `liblogger`，禁止引入其他日志库
!> **命名空间**：所有代码必须在 `dearts` 命名空间下

### 许可证约束
!> **FFmpeg 许可证**：FFmpeg 使用 LGPL 2.1+，静态链接时需要在应用中包含许可证声明
!> **GPL 编解码器**：禁止使用 GPL 编解码器（如 x264），避免整个应用被 GPL 污染

## 功能需求

### Phase 1: 基础设施（必做）
**优先级**：P0（最高）
**预计时间**：1-2 小时

#### 1.1 安装和配置 vcpkg
- [ ] 在项目根目录的上一级克隆 vcpkg 仓库
- [ ] 运行 bootstrap 脚本初始化 vcpkg
- [ ] 配置环境变量 `VCPKG_ROOT`

#### 1.2 安装 FFmpeg 包
- [ ] Windows: `vcpkg install ffmpeg:x64-windows`
- [ ] Linux: `vcpkg install ffmpeg:x64-linux`

#### 1.3 配置 CMake
- [ ] 创建 `CMakePresets.json` 配置 vcpkg toolchain
- [ ] 修改 `CMakeLists.txt` 添加 FFmpeg 查找逻辑
- [ ] 添加 `ENABLE_FFMPEG` 选项
- [ ] 创建 `ffmpeg_interface` 接口库
- [ ] 定义 `DEARTS_FFMPEG_SUPPORT` 宏

### Phase 2: 元数据获取功能（P1）
**优先级**：P1（高）
**预计时间**：1-2 小时

#### 2.1 创建 FFmpeg 封装类
文件：`core/utils/ffmpeg_wrapper.hpp` 和 `core/utils/ffmpeg_wrapper.cpp`

- [ ] 定义 `FFmpegWrapper` 类
- [ ] 实现 `initialize()` 方法
- [ ] 实现 `get_media_info()` 方法
- [ ] 实现 `is_available()` 方法

#### 2.2 元数据结构
```cpp
struct MediaInfo {
    double duration;      // 时长（秒）
    int width;            // 视频宽度
    int height;           // 视频高度
    double fps;           // 帧率
    std::string codec;    // 编解码器名称
    bool has_video;       // 是否包含视频
    bool has_audio;       // 是否包含音频
};
```

#### 2.3 测试用例
- [ ] 测试 MP4 文件元数据获取
- [ ] 测试音频文件元数据获取
- [ ] 测试不存在的文件（应返回错误）
- [ ] 测试损坏的文件（应返回错误）

### Phase 3: 媒体播放器功能（P1）
**优先级**：P1（高）
**预计时间**：4-6 小时

#### 3.1 创建播放器类
文件：`core/ui/media_player.hpp` 和 `core/ui/media_player.cpp`

- [ ] 定义 `MediaPlayer` 类
- [ ] 实现 `load()` 方法
- [ ] 实现 `play()` 方法
- [ ] 实现 `pause()` 方法
- [ ] 实现 `stop()` 方法
- [ ] 实现 `seek()` 方法（可选）
- [ ] 实现 `render()` 方法
- [ ] 实现 `set_volume()` / `get_volume()` 方法

#### 3.2 播放器状态
```cpp
enum class PlaybackState {
    Stopped,
    Playing,
    Paused,
    Error
};
```

#### 3.3 FFmpeg + SDL3 集成
- [ ] 使用 FFmpeg 解码视频/音频
- [ ] 使用 SDL3 渲染视频帧
- [ ] 使用 SDL3 音频流播放音频
- [ ] 实现音视频同步（使用 PTS）

#### 3.4 测试用例
- [ ] 测试 MP4 视频播放
- [ ] 测试音频文件播放
- [ ] 测试播放/暂停/停止
- [ ] 测试音量控制
- [ ] 测试跳转功能（如果实现）
- [ ] 测试不支持的格式（应返回错误）

### Phase 4: 转码功能（P2）
**优先级**：P2（中）
**预计时间**：2-3 小时

#### 4.1 创建转码接口
在 `FFmpegWrapper` 类中添加：

- [ ] 实现 `transcode()` 方法
- [ ] 实现 `ProgressCallback` 回调机制

#### 4.2 转码配置
```cpp
struct TranscodeConfig {
    std::string input_file;
    std::string output_file;
    std::string output_format;
    int video_bitrate;
    int audio_bitrate;
    int width;
    int height;
    double fps;
};
```

#### 4.3 测试用例
- [ ] 测试视频转码（MP4 → WebM）
- [ ] 测试音频转码（MP3 → AAC）
- [ ] 测试分辨率调整
- [ ] 测试比特率调整
- [ ] 测试进度回调
- [ ] 测试取消转码（可选）

### Phase 5: 应用集成（P1）
**优先级**：P1（高）
**预计时间**：1 小时

#### 5.1 应用初始化
- [ ] 在 `DearTsApplication::initialize()` 中初始化 FFmpeg
- [ ] 记录 FFmpeg 可用状态到日志

#### 5.2 创建示例 View
- [ ] 创建 `MediaPlayerView` 类
- [ ] 实现文件选择对话框
- [ ] 实现播放控制按钮
- [ ] 实现进度条显示
- [ ] 实现音量滑块

#### 5.3 测试用例
- [ ] 测试应用启动时 FFmpeg 初始化
- [ ] 测试 View 中播放器集成
- [ ] 测试 ImGui UI 交互

## 验收标准

### 功能验收
- [ ] 所有测试用例通过
- [ ] 支持 Windows 11 和 Linux
- [ ] 无内存泄漏（使用 Valgrind / Visual Studio Diagnostic Tools 验证）
- [ ] 无线程安全问题
- [ ] 错误处理完善（所有错误路径都返回 `Result::err()`）

### 性能验收
- [ ] 4K 视频播放流畅（30 FPS+）
- [ ] 音频播放无卡顿
- [ ] 转码速度合理（实时速度的 0.5x+）

### 代码质量验收
- [ ] 符合项目 C++ 代码规范
- [ ] 所有公共 API 有文档注释
- [ ] 无编译警告（-Wall -Wextra）
- [ ] 通过静态分析工具（clang-tidy / cpplint）

## 禁止事项

!> **禁止使用**：
- ffmpeg.exe 命令行工具
- 其他 FFmpeg 封装库（如 OpenCV, GStreamer）
- C++ 异常处理
- 全局变量（除单例模式外）
- 硬编码路径

!> **禁止引入**：
- Boost 库
- Qt 库
- 其他大型依赖

## 参考资料

### 内部文档
- `CLAUDE.md` - DearTs Framework 架构文档
- `dearts-dev/SKILL.md` - 框架快速参考
- `dearts-dev/references/*.md` - API 手册

### 外部文档
- [FFmpeg API 文档](https://ffmpeg.org/doxygen/trunk/)
- [SDL3 Wiki](https://wiki.libsdl.org/SDL3/)
- [vcpkg 使用指南](https://learn.microsoft.com/en-us/vcpkg/)

### 示例代码
- [FFmpeg 官方示例](https://github.com/FFmpeg/FFmpeg/tree/master/doc/examples)
- [雷霄骅的 FFmpeg 教程](https://github.com/leixiaohua1020/ffmpegstudies)

## 版本历史

- **v1.0** (2025-01-01) - 初始版本，定义完整需求
