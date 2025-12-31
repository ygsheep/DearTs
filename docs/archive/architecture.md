# DearTs Framework 架构文档

> DearTs Framework 完整架构说明

## 目录结构图

```mermaid
graph TD
    A[DearTs] --> B[core/]
    A --> C[main/]
    A --> D[lib/]
    A --> E[third_party/]
    A --> F[resources/]
    A --> G[docs/]
    A --> H[build/]

    B --> B1[app/]
    B1 --> B1A[application.h]
    B1 --> B1B[application.cpp]

    C --> C1[gui/]
    C1 --> C1A[main.cpp]
    C1 --> C1B[CMakeLists.txt]

    D --> D1[liblogger/]

    E --> E1[SDL/]
    E --> E2[imgui/]
    E --> E3[freetype/]

    F --> F1[fonts/]
    F --> F2[icon/]
```

## 应用程序生命周期

```mermaid
stateDiagram-v2
    [*] --> UNINITIALIZED
    UNINITIALIZED --> INITIALIZING: run()
    INITIALIZING --> RUNNING: onInitialize() success
    INITIALIZING --> STOPPED: onInitialize() failure
    RUNNING --> PAUSED: pause()
    PAUSED --> RUNNING: resume()
    RUNNING --> STOPPING: requestShutdown()
    STOPPING --> STOPPED: onShutdown()
    STOPPED --> [*]
```

## 核心类关系图

```mermaid
classDiagram
    class Application {
        +run(title, width, height) int
        +getState() State
        +getFPS() float
        +requestShutdown() void
        +pause() void
        +resume() void
        #pollEvents() void
        #handleEvent(event) void
        +onInitialize() bool
        +onEvent(event) void
        +onUpdate(deltaTime) void
        +onRender() void
        +onShutdown() void
        -m_window SDL_Window*
        -m_renderer SDL_Renderer*
        -m_state State
    }

    class SDL_Window {
        +SDL_CreateWindow()
        +SDL_DestroyWindow()
        +SDL_SetWindowTitle()
        +SDL_SetWindowSize()
    }

    class SDL_Renderer {
        +SDL_CreateRenderer()
        +SDL_DestroyRenderer()
        +SDL_RenderClear()
        +SDL_RenderPresent()
    }

    class ImGuiContext {
        +CreateContext()
        +DestroyContext()
        +NewFrame()
        +Render()
    }

    class Logger {
        +init(path, level)
        +log(level, message)
        +flush()
        +shutdown()
    }

    Application --> SDL_Window : manages
    Application --> SDL_Renderer : manages
    Application --> ImGuiContext : uses
    Application --> Logger : uses
```

## 主循环流程

```mermaid
flowchart TD
    Start([开始]) --> Init[SDL_Init]
    Init --> CreateWindow[创建窗口]
    CreateWindow --> CreateRenderer[创建渲染器]
    CreateRenderer --> InitImGui[初始化 ImGui]
    InitImGui --> OnInit[调用 onInitialize]
    OnInit --> InitCheck{初始化成功?}
    InitCheck -->|否| Cleanup[清理资源]
    InitCheck -->|是| MainLoop[主循环]

    MainLoop --> CalcDelta[计算 Delta Time]
    CalcDelta --> PollEvents[轮询 SDL 事件]
    PollEvents --> OnEventCall[调用 onEvent]
    OnEventCall --> UpdateCall[调用 onUpdate]
    UpdateCall --> BeginFrame[ImGui NewFrame]
    BeginFrame --> OnRenderCall[调用 onRender]
    OnRenderCall --> RenderImGui[渲染 ImGui]
    RenderImGui --> Present[呈现渲染结果]
    Present --> CheckQuit{请求退出?}
    CheckQuit -->|否| CalcDelta
    CheckQuit -->|是| OnShutdown[调用 onShutdown]
    OnShutdown --> Cleanup
    Cleanup --> End([结束])
```

## 模块依赖关系

```mermaid
graph LR
    subgraph "应用层"
        A[main.cpp]
    end

    subgraph "核心层"
        B[Application]
    end

    subgraph "第三方库"
        C[SDL3]
        D[ImGui]
        E[FreeType]
    end

    subgraph "工具库"
        F[liblogger]
    end

    A --> B
    B --> C
    B --> D
    B --> E
    B --> F
    D --> C
```

## 事件处理流程

```mermaid
sequenceDiagram
    participant SDL as SDL3
    participant App as Application
    participant ImGui as ImGui
    participant User as 用户代码

    SDL->>App: SDL_Event (QUIT)
    App->>App: pollEvents()
    App->>App: handleEvent()
    App->>User: onEvent(event)
    User->>App: requestShutdown()

    SDL->>App: SDL_Event (KEY_DOWN)
    App->>App: handleEvent()
    App->>ImGui: ImGui_ImplSDL3_ProcessEvent()
    App->>User: onEvent(event)
    Note over User: 处理输入逻辑

    SDL->>App: SDL_Event (MOUSE_MOTION)
    App->>ImGui: ImGui_ImplSDL3_ProcessEvent()
    ImGui->>ImGui: 更新鼠标状态
```

## 渲染管线

```mermaid
flowchart TD
    Start([帧开始]) --> Clear[清空渲染目标]
    Clear --> ImGuiNewFrame[ImGui NewFrame]
    ImGuiNewFrame --> ImGuiSDLEvent[处理 SDL 事件]
    ImGuiSDLEvent --> OnRender[调用 onRender]
    OnRender --> BuildUI[构建 ImGui UI]
    BuildUI --> ImGuiRender[ImGui Render]
    ImGuiRender --> DrawData[获取 DrawData]
    DrawData --> RenderDrawData[渲染 DrawData]
    RenderDrawData --> Present[呈现到屏幕]
    Present --> VSync{VSync?}
    VSync -->|是| WaitForVBlank[等待垂直同步]
    VSync -->|否| CapFPS{限制帧率?}
    CapFPS -->|是| Delay[延迟]
    CapFPS -->|否| End([帧结束])
    WaitForVBlank --> End
    Delay --> End
```

## 数据流向

```mermaid
flowchart LR
    subgraph "输入"
        A1[键盘]
        A2[鼠标]
        A3[手柄]
    end

    subgraph "SDL3"
        B1[事件队列]
    end

    subgraph "Application"
        C1[事件处理]
        C2[状态更新]
        C3[渲染]
    end

    subgraph "输出"
        D1[窗口]
        D2[音频]
    end

    A1 --> B1
    A2 --> B1
    A3 --> B1
    B1 --> C1
    C1 --> C2
    C2 --> C3
    C3 --> D1
    C3 --> D2
```

## 构建系统

```mermaid
graph TD
    A[CMake] --> B[配置阶段]
    B --> C[依赖检测]
    C --> D[SDL3]
    C --> E[FreeType]
    C --> F[OpenGL/Vulkan]

    D --> G[编译阶段]
    E --> G
    F --> G

    G --> H[core/app/application.cpp]
    G --> I[main/gui/main.cpp]

    H --> J[deartsdl_gui 可执行文件]
    I --> J

    J --> K[链接阶段]
    K --> L[静态库]
    K --> M[系统库]

    L --> N[最终可执行文件]
    M --> N
```

## 内存管理架构

```mermaid
classDiagram
    class ResourceManager {
        +loadTexture(path) Texture*
        +loadSound(path) Sound*
        +loadFont(path) Font*
        -m_textures map
        -m_sounds map
        -m_fonts map
    }

    class Texture {
        +Texture(SDL_Texture*)
        -m_texture SDL_Texture*
        -m_size Size
    }

    class Sound {
        +Sound(Mix_Chunk*)
        -m_chunk Mix_Chunk*
    }

    class Font {
        +Font(TTF_Font*)
        -m_font TTF_Font*
    }

    ResourceManager --> Texture : manages
    ResourceManager --> Sound : manages
    ResourceManager --> Font : manages
```

## 性能监控系统

```mermaid
flowchart LR
    A[性能数据采集] --> B[帧时间统计]
    A --> C[FPS 计算]
    A --> D[内存使用]
    A --> E[Draw Call 计数]

    B --> F[性能分析器]
    C --> F
    D --> F
    E --> F

    F --> G[性能报告]
    G --> H{性能警告?}
    H -->|是| I[优化建议]
    H -->|否| J[正常]
```

## 错误处理流程

```mermaid
flowchart TD
    A[错误发生] --> B{错误类型}
    B -->|SDL 错误| C[SDL_GetError]
    B -->|文件错误| D[文件路径检查]
    B -->|内存错误| E[内存分配失败]

    C --> F[记录错误日志]
    D --> F
    E --> F

    F --> G{致命错误?}
    G -->|是| H[清理资源]
    G -->|否| I[错误恢复]

    H --> J[应用退出]
    I --> K[继续运行]
```

## 扩展模块规划

```mermaid
graph TD
    A[DearTs Core] --> B[Input 模块]
    A --> C[Render 模块]
    A --> D[Resource 模块]
    A --> E[Audio 模块]
    A --> F[Physics 模块]
    A --> G[Network 模块]

    B --> B1[键盘管理]
    B --> B2[鼠标管理]
    B --> B3[手柄管理]

    C --> C1[批处理渲染]
    C --> C2[着色器管理]
    C --> C3[材质系统]

    D --> D1[资源加载]
    D --> D2[资源缓存]
    D --> D3[异步加载]

    E --> E1[音效播放]
    E --> E2[音乐播放]
    E --> E3[3D 音效]

    F --> F1[碰撞检测]
    F --> F2[物理模拟]

    G --> G1[TCP 网络]
    G --> G2[HTTP 客户端]
```

## 版本演进路线

```mermaid
timeline
    title DearTs Framework 发展路线
    section 2024 Q4
        v1.0 : 核心 Application 类<br>SDL3 集成
        : ImGui 集成<br>异步日志系统
    section 2025 Q1
        v1.5 : Input 模块<br>Resource 管理器
        : 多窗口支持
    section 2025 Q2
        v2.0 : Render 模块<br>Audio 系统
        : ECS 架构支持
    section 2025 Q3
        v2.5 : Physics 模块<br>Network 支持
        : 性能优化
    section 2025 Q4
        v3.0 : 完整游戏引擎<br>编辑器支持
        : 跨平台发布
```
