# SDL3 + ImGui 混合渲染示例

## 简介

本示例展示如何在 ImGui 的可收起区域内嵌入 SDL3 直接渲染的内容，并支持平移和缩放操作。

## 功能特性

- ✅ ImGui 可收起/展开区域（CollapsingHeader）
- ✅ SDL3 离屏渲染到纹理
- ✅ 在 ImGui 中显示 SDL 纹理（ImGui::Image）
- ✅ 鼠标拖拽平移视图
- ✅ 鼠标滚轮缩放视图
- ✅ 实时动画效果
- ✅ 完整的交互体验

## 编译和运行

### 编译

```bash
cmake --build build --target sdl_render_example --config Debug
```

### 运行

```bash
# Windows
.\build\bin\Debug\sdl_render_example.exe

# Linux/macOS
./build/bin/sdl_render_example
```

## 操作说明

| 操作 | 功能 |
|------|------|
| 鼠标左键拖拽 | 平移视图 |
| 鼠标滚轮 | 缩放视图（0.1x - 10x） |
| 点击标题栏 | 收起/展开区域 |
| 重置视图按钮 | 恢复默认视图状态 |

## 技术原理

### 渲染流程

```
1. 创建 SDL 纹理（SDL_TEXTUREACCESS_TARGET）
        ↓
2. 使用 SDL3 API 绘制内容到纹理
        ↓
3. 将 SDL_Texture* 转换为 ImTextureID
        ↓
4. 在 ImGui 中使用 ImGui::Image() 显示
        ↓
5. 处理鼠标事件实现平移/缩放
```

### 关键代码

#### 1. 创建渲染目标纹理

```cpp
g_render_texture = SDL_CreateTexture(
    g_renderer,
    SDL_PIXELFORMAT_RGBA8888,
    SDL_TEXTUREACCESS_TARGET,  // 关键：允许作为渲染目标
    TEXTURE_WIDTH,
    TEXTURE_HEIGHT
);
```

#### 2. 渲染到纹理

```cpp
// 设置渲染目标
SDL_SetRenderTarget(g_renderer, g_render_texture);

// 清空纹理
SDL_RenderClear(g_renderer);

// 绘制内容
SDL_RenderLine(renderer, x1, y1, x2, y2);
SDL_RenderFillRect(renderer, &rect);
// ... 更多绘制命令 ...

// 恢复默认渲染目标
SDL_SetRenderTarget(g_renderer, nullptr);
```

#### 3. 在 ImGui 中显示

```cpp
// SDL3 中可以直接转换
ImTextureID texture_id = (ImTextureID)g_render_texture;

// 在 ImGui 中显示
ImGui::Image(texture_id, ImVec2(width, height));
```

#### 4. 平移和缩放

```cpp
// 平移：拖拽
if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
    ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
    offset_x += delta.x;
    offset_y += delta.y;
    ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
}

// 缩放：滚轮
float wheel = ImGui::GetIO().MouseWheel;
if (wheel > 0.0f) scale *= 1.1f;
else if (wheel < 0.0f) scale /= 1.1f;
```

## 示例内容

本示例绘制了以下动画图形：

1. **渐变背景** - 从深蓝到深紫的垂直渐变
2. **网格线** - 50x50 像素的网格
3. **旋转矩形** - 持续旋转的红色矩形
4. **脉冲圆圈** - 半径周期性变化的绿色圆圈
5. **反向旋转圆** - 反向旋转的蓝色圆圈
6. **中心十字** - 黄色十字标记
7. **文字占位** - 灰色矩形代表文字

## 应用场景

这种混合渲染技术适用于：

- **游戏小地图** - SDL 渲染游戏世界，ImGui 显示在小窗口
- **数据可视化** - 大量图形用 SDL 渲染，界面用 ImGui
- **图像编辑器** - 图像处理用 SDL，工具栏用 ImGui
- **调试视图** - 显示碰撞盒、路径等调试图形
- **性能监控** - 实时渲染性能图表

## 性能优化

当前实现：
- 每 33ms 更新一次纹理（30 FPS）
- 使用离屏纹理避免每帧重新渲染

优化建议：
1. 只在内容变化时重新渲染
2. 使用多线程渲染纹理
3. 降低更新频率（如 10-15 FPS）
4. 使用较小的纹理尺寸

## 扩展功能

可以轻松添加的功能：

1. **视图快照** - 保存和恢复视图状态
2. **多级缩放** - 预设的缩放级别
3. **选择工具** - 矩形选择、框选
4. **标注系统** - 在纹理上绘制标注
5. **导出功能** - 保存渲染内容为图片

## 常见问题

### Q: 纹理显示为黑色/白色方块？

**A**: 检查以下几点：
1. 纹理是否成功创建（`SDL_CreateTexture` 返回值）
2. 渲染目标是否正确设置（`SDL_SetRenderTarget`）
3. 是否正确渲染到纹理（检查绘制命令）
4. ImTextureID 转换是否正确

### Q: 缩放后内容模糊？

**A**: 提高纹理分辨率：
```cpp
constexpr int TEXTURE_WIDTH = 1920;   // 提高分辨率
constexpr int TEXTURE_HEIGHT = 1080;
```

### Q: 性能问题（FPS 下降）？

**A**: 降低更新频率：
```cpp
if (g_animation_time - last_render_time > 0.1f) { // 10 FPS
    render_to_texture();
    last_render_time = g_animation_time;
}
```

## 代码结构

```
sdl_render_example/
├── main.cpp          # 主程序（约 600 行）
├── CMakeLists.txt    # CMake 配置
└── README.md         # 本文件
```

## 依赖项

- SDL3
- ImGui
- imgui_impl_sdl3
- imgui_impl_sdlrenderer3
- C++20 编译器

## 参考资料

- [SDL3 Render Target](https://wiki.libsdl.org/SDL3/CategoryRender)
- [ImGui Image](https://github.com/ocornut/imgui/wiki/Image)
- [DearTs Framework](../../dearts-dev/SKILL.md)

## 许可证

本项目遵循 DearTs Framework 的许可证。

## 作者

DearTs Framework 开发团队

---

**注意**：这是一个教学示例，展示了 SDL3 和 ImGui 混合渲染的基本技术。在实际项目中，你可能需要根据具体需求进行调整和优化。
