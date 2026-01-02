# Live2D Cubism SDK 集成 - 需求文档

## 📖 项目概述

在 DearTs Framework 中集成 Live2D Cubism SDK，实现动态角色渲染系统，作为现有静态/动画角色系统的扩展。

**项目目标**:
- 提供完整的 Live2D 模型加载和管理能力
- 支持流畅的动画、表情和物理效果
- 用户友好的设置界面和开发者友好的调试工具
- 跨平台兼容性（Windows + Linux）
- 保持与现有角色系统的兼容性

**开发周期**: 3-4 个月（18-21 周）

---

## ⚠️ 系统约束

!> **强制要求**:
- **必须使用 Live2D Cubism SDK for OpenGL**（跨平台一致性）
- **必须通过 FBO 离屏渲染** 解决 SDL3_Renderer 与 Live2D 渲染冲突
- **必须作为插件实现**（不破坏现有系统架构）
- **必须保留现有角色系统**（CharacterType::Single, CharacterType::Animated）

!> **平台支持**:
- **Windows 10/11**: OpenGL 3.2+
- **Linux (Ubuntu/Debian)**: OpenGL 3.2+ (Wayland/X11)

!> **技术栈限制**:
- **C++20**: 必须符合框架现有标准
- **SDL3**: 必须使用 SDL3_Renderer 后端
- **ImGui**: 必须使用 ImGui_ImplSDLRenderer3
- **EventBus**: 必须通过 EventBus 进行事件通信
- **ConfigManager**: 必须使用 ConfigScope 进行配置管理

!> **架构约束**:
- **插件化架构**: 必须实现为 IPlugin 接口
- **Content Registry**: 必须通过 ContentRegistry 注册视图和命令
- **Result<T, E>**: 必须使用框架错误处理机制

!> **性能要求**:
- **渲染开销**: < 5ms/帧（1920x1080）
- **总帧率**: > 60 FPS
- **模型加载**: < 2 秒

!> **许可证**:
- Live2D Cubism SDK: **Live2D Proprietary License**（免费用于个人/商业用途，但不得重新分发）
- 项目代码: 遵循 DearTs Framework 现有许可证

---

## 🚫 禁止事项

!> **严格禁止**:
- ❌ 不得修改现有 CharacterRenderer 实现（仅扩展 CharacterType 枚举）
- ❌ 不得破坏 SDL3_Renderer 的状态管理
- ❌ 不得使用 Direct3D/Vulkan 后端（仅限 OpenGL）
- ❌ 不得在插件中直接访问全局状态（必须通过 ConfigManager/EventBus）
- ❌ 不得绕过 EventBus 进行组件间通信
- ❌ 不得使用硬编码路径（必须通过配置管理）

!> **临时禁止**（跨阶段）:
- ⛔ 阶段 1-2 禁止实现动画系统
- ⛔ 阶段 1-3 禁止实现物理引擎
- ⛔ 阶段 1-4 禁止实现鼠标交互
- ⛔ 阶段 1-5 禁止进行性能优化

---

## 📋 功能需求

### Phase 1: SDK 集成和基础渲染（4-6 周）

**目标**: 在屏幕上显示静态 Live2D 模型

#### 功能清单

##### 1.1 Live2D SDK 下载和配置
- [ ] 下载 Live2D Cubism SDK for OpenGL
  - 版本: Cubism SDK for Native 4.x (最新稳定版)
  - 下载地址: https://www.live2d.com/download/cubism-sdk/
  - 位置: `third_party/Live2D/`

- [ ] 配置 CMake 构建系统
  - 添加 `option(ENABLE_LIVE2D "Enable Live2D plugin" OFF)`
  - 链接 Live2D Core Framework 库
  - 配置 OpenGL 依赖（Windows: opengl32, Linux: OpenGL::GL）

##### 1.2 抽象渲染器接口
- [ ] 创建 `core/ui/live2d_renderer.h`
  - 定义 `ILive2DRenderer` 抽象接口
  - 定义 `Live2DRendererFactory` 工厂类

**接口要求**:
```cpp
class ILive2DRenderer {
public:
    virtual Result<void, std::string> initialize(
        SDL_Renderer* renderer, int width, int height
    ) = 0;
    virtual void render(Live2DModelInstance* model) = 0;
    virtual SDL_Texture* get_texture() const = 0;
    virtual void resize(int width, int height) = 0;
    virtual bool is_initialized() const = 0;
};
```

##### 1.3 OpenGL 渲染器实现
- [ ] 创建 `plugins/live2d/source/live2d_renderer_gl.cpp`
  - 实现 FBO（Frame Buffer Object）离屏渲染
  - 实现 SDL 纹理上传机制（FBO → SDL_Texture）
  - OpenGL 状态保存/恢复

**技术要求**:
- FBO 分辨率: 屏幕分辨率的 1/2
- 像素格式: GL_RGBA8
- 纹理上传: glReadPixels + SDL_UpdateTexture

##### 1.4 模型实例类
- [ ] 创建 `core/ui/live2d_model_instance.h/.cpp`
  - 定义 `ModelInfo` 结构体
  - 实现 `.model3.json` 文件解析
  - 实现 Cubism SDK 模型加载

**ModelInfo 结构**:
```cpp
struct ModelInfo {
    std::string id;
    std::string name;
    std::string directory;
    std::string model3Path;
};
```

##### 1.5 插件主类
- [ ] 创建 `plugins/live2d/include/live2d_plugin.hpp`
  - 实现 `IPlugin` 接口
  - 实现插件生命周期管理
  - 注册到 PluginManager

##### 1.6 主渲染循环集成
- [ ] 修改 `main/gui/source/dearts_application.cpp`
  - 在 `on_render()` 中添加 Live2D 渲染调用
  - 条件编译: `#ifdef ENABLE_LIVE2D`

**渲染顺序**:
1. ImGui 渲染
2. CharacterRenderer 渲染（现有）
3. Live2D 渲染（新增）

##### 1.7 测试和验证
- [ ] 加载 Live2D 官方示例模型（Haru）
- [ ] 验证静态模型显示
- [ ] 验证 FBO → SDL 纹理渲染管线
- [ ] 性能测试: 帧率 > 60 FPS

**验收标准**:
- ✅ Live2D 模型在屏幕上正确显示
- ✅ 无 OpenGL 状态冲突
- ✅ 帧率 > 60 FPS
- ✅ 内存无泄漏

---

### Phase 2: 模型管理和配置系统（3-4 周）

**目标**: 通过设置界面切换和管理多个模型

#### 功能清单

##### 2.1 模型管理器
- [ ] 创建 `core/ui/live2d_model_manager.h/.cpp`
  - 实现单例模式
  - 实现模型注册/卸载/切换
  - 实现模型目录扫描功能

**接口要求**:
```cpp
class Live2DModelManager {
public:
    static Live2DModelManager& instance();

    Result<void, std::string> register_model(const ModelInfo& info);
    Result<void, std::string> load_model(const std::string& id);
    void unload_model(const std::string& id);
    bool set_active_model(const std::string& id);
    Live2DModelInstance* get_active_model();

    Result<size_t, std::string> scan_models_from_directory(
        const std::string& directory
    );
};
```

##### 2.2 配置系统
- [ ] 定义 Live2D 配置 JSON 结构
  - 配置前缀: `live2d.*`
  - 支持模型列表、位置、缩放、透明度

**配置结构**:
```json
{
  "live2d": {
    "enabled": true,
    "active_model_id": "haru",
    "position": 0,
    "scale": 0.5,
    "opacity": 1.0,
    "margin": 20.0,
    "models": [
      {
        "id": "haru",
        "name": "Haru",
        "directory": "resources/live2d/Haru",
        "model3_path": "Haru.model3.json"
      }
    ]
  }
}
```

- [ ] 实现 ConfigScope 集成
  - 使用 `ConfigScope config("live2d");`
  - 实现配置加载/保存

##### 2.3 设置界面
- [ ] 创建 `plugins/live2d/include/widgets/live2d_settings_widget.hpp`
  - 模型选择下拉框
  - 位置/缩放/透明度控制
  - 与 ConfigManager 集成

##### 2.4 CharacterManager 扩展
- [ ] 修改 `core/ui/character_manager.h/.cpp`
  - 添加 `CharacterType::Live2D`
  - 统一的字符类型接口

##### 2.5 测试和验证
- [ ] 测试模型切换功能
- [ ] 测试配置持久化
- [ ] 测试多模型管理

**验收标准**:
- ✅ 能够通过设置界面切换模型
- ✅ 配置正确保存和加载
- ✅ 支持至少 3 个模型同时注册

---

### Phase 3: 动画和物理引擎（4-5 周）

**目标**: 模型能够播放动画、表情和物理效果

#### 功能清单

##### 3.1 动画管理器
- [ ] 创建 `plugins/live2d/source/live2d_motion_manager.cpp`
  - 实现 `.motion3.json` 文件加载
  - 实现动画播放控制（播放/暂停/停止）
  - 实现动画优先级和淡入淡出

**接口要求**:
```cpp
class Live2DMotionManager {
public:
    void start_motion(const std::string& group, int index, int priority);
    void pause_motion();
    void resume_motion();
    void stop_motion();
    void update(double deltaTime);
};
```

##### 3.2 表情系统
- [ ] 实现 `.exp3.json` 文件加载
  - 表情切换
  - 与动画叠加

##### 3.3 物理引擎
- [ ] 创建 `plugins/live2d/source/live2d_physics.cpp`
  - 集成 Live2D Physics SDK
  - 实现 `.physics3.json` 解析
  - 实现头发、衣服摆动模拟

**物理参数**:
- 风力强度
- 摆动阻尼
- 碰撞检测

##### 3.4 自动动画
- [ ] 实现 Idle 动画
- [ ] 实现呼吸效果
- [ ] 实现随机动作触发

##### 3.5 测试和验证
- [ ] 测试动画播放
- [ ] 测试表情切换
- [ ] 测试物理效果

**验收标准**:
- ✅ 动画流畅播放（无卡顿）
- ✅ 表情正确叠加
- ✅ 物理效果自然
- ✅ 性能: 渲染开销 < 5ms/帧

---

### Phase 4: 事件系统和交互（3-4 周）

**目标**: 用户可以点击模型并触发交互

#### 功能清单

##### 4.1 事件定义
- [ ] 创建 `core/ui/live2d_events.h`
  - `Live2DModelLoadedEvent`
  - `Live2DParameterChangedEvent`
  - `Live2DMotionStartedEvent`
  - `Live2DMouseInteractionEvent`

##### 4.2 Hit Area 检测
- [ ] 创建 `plugins/live2d/source/live2d_hit_area.cpp`
  - 实现 `.canvas3.json` 解析
  - 实现鼠标点击检测
  - 实现拖拽跟随功能

**接口要求**:
```cpp
bool hit_test(float x, float y);
```

##### 4.3 事件订阅
- [ ] 在 Live2DPlugin 中订阅 SDL 鼠标事件
- [ ] 发布 Live2D 交互事件

##### 4.4 参数控制
- [ ] 实现参数控制接口
  - 程序化设置参数（如跟随鼠标）
  - 参数动画插值

##### 4.5 命令注册
- [ ] 通过 ContentRegistry::Commands 注册
  - 表情切换命令
  - 动画播放命令

##### 4.6 测试和验证
- [ ] 测试鼠标交互
- [ ] 测试参数控制
- [ ] 测试命令调用

**验收标准**:
- ✅ 鼠标点击正确触发
- ✅ 拖拽跟随流畅
- ✅ 参数控制响应及时
- ✅ 命令系统正常工作

---

### Phase 5: 调试工具和优化（2-3 周）

**目标**: 完善的调试工具和良好的性能

#### 功能清单

##### 5.1 调试视图
- [ ] 创建 `plugins/live2d/include/views/live2d_debug_view.hpp`
  - 显示参数列表和实时滑块
  - 显示 Hit Area 可视化
  - 显示性能统计信息

##### 5.2 性能优化
- [ ] 实现脏标记系统
  - 只在参数变化时重新渲染
- [ ] 实现 FBO 降采样
  - 支持 1/2, 1/4 分辨率
- [ ] 添加 PBO 异步传输
  - 避免 GPU 同步等待

##### 5.3 日志和错误处理
- [ ] 添加加载失败日志
- [ ] 添加渲染错误捕获
- [ ] 添加性能分析工具

##### 5.4 文档
- [ ] 编写 API 文档
- [ ] 编写模型制作指南
- [ ] 编写示例代码

##### 5.5 测试和验证
- [ ] 性能基准测试
- [ ] 内存泄漏检测
- [ ] 压力测试

**验收标准**:
- ✅ 调试工具功能完整
- ✅ 渲染开销 < 5ms/帧
- ✅ 无内存泄漏
- ✅ 文档完整

---

### Phase 6: 跨平台测试和修复（2-3 周）

**目标**: 在 Windows 和 Linux 上稳定运行

#### 功能清单

##### 6.1 Windows 平台测试
- [ ] OpenGL 驱动兼容性测试
- [ ] 不同分辨率测试
- [ ] 多显示器支持测试

##### 6.2 Linux 平台测试
- [ ] Ubuntu/Debian 包依赖测试
- [ ] Wayland/X11 兼容性测试
- [ ] GPU 驱动测试（NVIDIA/AMD/Intel）

##### 6.3 平台特定问题修复
- [ ] 路径分隔符修复
- [ ] 动态库加载修复
- [ ] 线程安全性修复

##### 6.4 性能基准测试
- [ ] 帧率测试（目标 > 60 FPS）
- [ ] 内存使用分析
- [ ] 加载时间优化

##### 6.5 文档完善
- [ ] 编写故障排除指南
- [ ] 编写平台特定说明

**验收标准**:
- ✅ Windows 和 Linux 稳定运行
- ✅ 帧率 > 60 FPS
- ✅ 无已知严重 Bug
- ✅ 文档完整

---

## ✅ 验收标准

### 功能完整性
- ✅ 支持加载和渲染 Live2D 模型
- ✅ 支持模型切换和管理
- ✅ 支持动画、表情、物理效果
- ✅ 支持用户交互（点击、拖拽）
- ✅ 提供完整的调试工具

### 性能指标
- ✅ 渲染开销 < 5ms/帧（1920x1080）
- ✅ 总帧率 > 60 FPS
- ✅ 模型加载时间 < 2 秒
- ✅ 内存占用合理（无泄漏）

### 兼容性
- ✅ Windows 10/11 支持
- ✅ Linux (Ubuntu/Debian) 支持
- ✅ 与现有角色系统兼容
- ✅ 不破坏现有功能

### 可维护性
- ✅ 代码符合框架规范
- ✅ 完整的 API 文档
- ✅ 完整的用户指南
- ✅ 完整的故障排除指南

### 测试覆盖
- ✅ 所有核心功能有测试
- ✅ 所有平台测试通过
- ✅ 性能测试通过
- ✅ 内存泄漏测试通过

---

## 📊 项目统计

- **总阶段数**: 6
- **总预计时间**: 18-21 周（3-4 个月）
- **关键文件数**: 22
- **优先级分布**:
  - P0 (最高): Phase 1, 2
  - P1 (高): Phase 3, 4
  - P2 (中): Phase 5, 6

---

## 🔗 相关文档

- `README_AGENT_FRAMEWORK.md` - 双代理协作框架说明
- `CLAUDE.md` - DearTs Framework 架构文档
- `dearts-dev/SKILL.md` - Framework 快速参考
- `dearts-dev/references/plugin_system_api.md` - 插件系统 API
- `plan_nifty-scribbling-feigenbaum.md` - Live2D 集成实施计划

---

## 📝 变更历史

- **2025-01-01**: 初始版本创建
  - 定义 6 个阶段
  - 定义系统约束和禁止事项
  - 定义验收标准

---

**文档版本**: 1.0.0
**创建时间**: 2025-01-01
**状态**: ✅ 需求定义完成，等待功能拆解
