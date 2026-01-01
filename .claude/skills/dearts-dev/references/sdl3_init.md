# SDL3 初始化指南

SDL3 是最新版本的跨平台多媒体库。

## 快速开始

SDL3 在 DearTs Application 基类中自动初始化。

## 创建窗口

```cpp
SDL_Window* window = SDL_CreateWindow(
    "DearTs Application",
    1280,
    720,
    SDL_WINDOW_RESIZABLE
);
```

## 事件处理

```cpp
SDL_Event event;
while (SDL_PollEvent(&event)) {
    if (event.type == SDL_EVENT_QUIT) {
        // 处理退出
    }
}
```

更多内容请参考项目文档。
