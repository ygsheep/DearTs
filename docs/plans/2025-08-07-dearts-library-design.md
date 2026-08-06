# DearTs 库化设计文档

**日期：** 2025-08-07
**状态：** 已实施（2026-08-07 全部步骤完成并验证通过）
**分支：** develop（实施基于 develop，领先 main 39 个提交）

## 目标

将 DearTs 框架从"源码直接编译进主程序"重构为**可复用的静态 C++ 库**，支持：

1. 其他项目通过 `find_package(dearts)` 或 `add_subdirectory` 链接使用
2. 完整封装框架核心（core）+ 内置插件
3. 保留当前环境的可构建性（当前系统无 SDL3/ImGui 系统包，依赖以 third_party 子目录编译）

## 用户决策记录

| 决策点 | 选择 |
|--------|------|
| 使用场景 | 静态链接库 |
| 库粒度 | 完整框架 + 内置插件 |
| 依赖策略 | 混合方案：third_party 子目录编译 + find_package 导出支持 |
| 关键约束 | ImGui 深度定制（IMGUI_USER_CONFIG + FreeType + lunaSVG + Test Engine），无法用 vcpkg 版本 |

## 当前架构（现状）

- `core/` 源码被 `main/gui/CMakeLists.txt` 直接以 `target_sources` 编译进 `DearTs` 可执行文件
- 内置插件（builtin/settings/logger_viewer/navigation/toast_notification/command_palette）也直接编译进主程序
- 主程序 `dearts_application.cpp` 手动 include 插件头文件并在 `setup_plugins()` 注册
- 依赖：SDL3、FreeType、lunaSVG、nlohmann/json（third_party 子目录）+ ImGui、ImPlot（手动 add_library）

## 目标架构

```
DearTs Framework
├── dearts 静态库（新目标）
│   ├── core/ 全部源码
│   ├── 内置插件源码
│   ├── 链接 imgui_full, implot, SDL3-static, deartsdl_logger, freetype, lunasvg
│   └── 导出 PUBLIC include 目录（core、plugins、lib、third_party）
├── DearTs 可执行文件（改为链接 dearts）
│   └── 仅保留 main.cpp + dearts_application.*
└── 安装/导出
    ├── deartsConfig.cmake + deartsTargets.cmake
    └── 头文件 + 库文件安装规则
```

## 实施步骤

### 1. 新建 `core/CMakeLists.txt` 或顶层 `dearts` 库目标
- 定义 `dearts` 静态库，收集 `core/` 全部 `.cpp`
- PUBLIC include 目录：core、core 子目录、lib/liblogger、plugins 各 include、third_party 相关
- 链接第三方库
- 处理 Windows/MSVC 编译选项

### 2. 内置插件并入库
- 将 6 个内置插件的 `.cpp` 加入 `dearts` 源文件
- 导出各插件的 include 目录（PUBLIC），使主程序仍能 include 插件头文件注册

### 3. 主程序改为链接库
- `main/gui/CMakeLists.txt` 移除 core + 插件的 `target_sources`
- `target_link_libraries(DearTs PRIVATE dearts)`
- 保留 main.cpp、dearts_application.*、resources

### 4. 提供安装/导出配置
- `install(TARGETS dearts EXPORT deartsTargets ...)`
- `install(DIRECTORY core/ plugins/ ... DESTINATION include)`
- `configure_package_config_file` 生成 `deartsConfig.cmake`
- `write_basic_package_version_file`

### 5. 验证
- 配置 + 构建 `DearTsApp` 成功
- 编写一个最小示例验证 `find_package(dearts)` 可用

## 实施结果（2026-08-07）

### 实际完成的变更

- 顶层 `CMakeLists.txt`：新增 `dearts` 静态库目标（29 个 core .cpp + builtin/settings/logger_viewer/navigation/toast_notification/command_palette/clipboard_parser + 条件 ffmpeg 插件源），PUBLIC include 双模式（BUILD_INTERFACE 全路径 + INSTALL_INTERFACE），PUBLIC 链接 imgui_full/implot/SDL3::SDL3-shared/SDL3_image::SDL3_image/deartsdl_logger/freetype/lunasvg/nlohmann_json/boost_asio（条件 ffmpeg_interface），MSVC 传递 `/Zc:preprocessor`（`__VA_OPT__` 需求）
- `main/gui/CMakeLists.txt`：重写为仅 `main.cpp + dearts_application.*` + 资源复制，`target_link_libraries(DearTs PRIVATE dearts)`；移除了重复的 `option(DEARTS_FFMPEG_SUPPORT)`
- `main/chatmanager/CMakeLists.txt`：同样改为链接 dearts（core + builtin 源码移除，chat/memory_core 保留源码编译），补 `/utf-8`
- 安装/导出：
  - `cmake/deartsConfig.cmake.in`：先 find_package（QUIET + 回退）尝试 vcpkg 依赖，再 include deartsTargets.cmake，最后为缺失目标（SDL3-shared、SDL3_image-static、PNG::PNG、TIFF::TIFF、WebP::webp 系列、Boost::asio 等）定义 IMPORTED 回退目标
  - `install(EXPORT deartsTargets NAMESPACE DearTs::)`，同时导出第三方目标（imgui/imgui_sdl3/imgui_full/implot/freetype/deartsdl_logger/nlohmann_json/boost_asio/SDL3-shared/SDL3_Headers/SDL3_image-static/ffmpeg_interface）
  - imgui 系列 include 改 `$<BUILD_INTERFACE>/<INSTALL_INTERFACE>` 双模式；安装 nlohmann_json.natvis（INTERFACE_SOURCES 引用）
- `examples/dearts_minimal`：`find_package(dearts 1.0 REQUIRED)` + `DearTs::dearts` 的最小验证程序
- 顺带修复：5 个插件目标（toast_notification/clipboard_parser/navigation/test_plugin/test_dependency_plugin）补 MSVC `/utf-8`；chat/clipboard_parser/toast_notification 的悬空目标引用（dearts_core/logger_lib/dear_core）改为真实目标（dearts/deartsdl_logger）

### 验证结果

- 全量构建通过：`DearTs.exe` + `ChatManager.exe` + 全部插件目标
- `examples/dearts_minimal` 配置/编译/链接/运行全部通过（Logger + EventBus + ConfigManager 实测输出正确）
- 安装验证：`cmake --install build --prefix <prefix>` 成功，544 个文件

### 实施中发现的关键问题（已解决）

1. `install(EXPORT)` 要求所有被链接目标在导出集内或为 IMPORTED → 第三方目标一并导出；lunasvg/plutovg 走 config 回退（避免与 lunasvg 自带导出冲突）
2. 别名目标（SDL3::SDL3-shared 等）在导出文件中被解析为底层名（SDL3-shared 等），config 回退需用底层名
3. `IMGUI_USER_CONFIG=\"...\"` 嵌套引号在 config 文件中解析出错 → 改为分号列表单参数
4. `find_dependency` 失败会 FATAL → 全部改 `find_package(QUIET)` + 回退
5. `--target DearTs` 与 `dearts` 目标名大小写冲突（MSBuild 不区分大小写）→ 验证构建需用全量构建或 `--config` 形式

### 遗留事项（可选）

- `add_subdirectory` 方式已隐式验证（主项目自身即此方式）
- Test Engine（ENABLE_IMGUI_TEST_ENGINE）导出路径未验证（默认 OFF）
- Linux/Nix 平台安装导出未验证（config 中已有 `lib*.a` 分支）

## 风险与注意事项

1. **插件头文件被主程序 include**：插件 `.cpp` 编译进库，头文件仍导出，主程序可正常注册 —— 需验证符号链接
2. **ImGui 定制**：库必须携带 `IMGUI_USER_CONFIG` 编译定义，使用方若直接 include imgui.h 需确保 user config 一致
3. **静态库依赖传递**：`dearts` 需 PUBLIC 链接所有第三方库，使用方只需链接 `dearts`
4. **Windows 编译选项**：`/W4 /utf-8`、`_CRT_SECURE_NO_WARNINGS`、`_USE_MATH_DEFINES`、`NOMINMAX` 需在库上设置
