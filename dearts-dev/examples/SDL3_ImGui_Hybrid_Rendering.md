# SDL3 + ImGui 混合渲染完整指南

## 概述

本文档展示如何在 ImGui 的可收起区域内嵌入 SDL3 直接渲染的内容，并支持平移和缩放操作。

## 核心原理

### 渲染流程

```
1. 创建 SDL 纹理（渲染目标）
        ↓
2. 使用 SDL3 API 绘制内容到纹理
        ↓
3. 将纹理转换为 ImTextureID
        ↓
4. 在 ImGui 中使用 ImGui::Image() 显示
        ↓
5. 处理鼠标事件实现平移/缩放
```

### 关键技术点

#### 1. 创建离屏纹理（渲染目标）

```cpp
m_texture = SDL_CreateTexture(
    renderer,
    SDL_PIXELFORMAT_RGBA8888,
    SDL_TEXTUREACCESS_TARGET,  // 关键：允许作为渲染目标
    width, height
);
```

**`SDL_TEXTUREACCESS_TARGET` 的作用**：
- 允许纹理作为 `SDL_SetRenderTarget()` 的目标
- 可以使用 SDL3 渲染 API 绘制到纹理上
- 而不是从静态图片加载

#### 2. 绘制到纹理

```cpp
// 保存当前渲染目标
SDL_Texture* old_target = SDL_GetRenderTarget(renderer);

// 设置新的渲染目标
SDL_SetRenderTarget(renderer, m_texture);

// 清空纹理
SDL_RenderClear(renderer);

// 绘制内容
SDL_RenderLine(renderer, x1, y1, x2, y2);
SDL_RenderFillRect(renderer, &rect);
// ... 更多绘制命令 ...

// 恢复原渲染目标
SDL_SetRenderTarget(renderer, old_target);
```

#### 3. 在 ImGui 中显示

```cpp
// SDL3 中，ImTextureID 可以直接转换自 SDL_Texture*
ImTextureID texture_id = (ImTextureID)m_texture;

// 在 ImGui 中显示
ImGui::Image(texture_id, ImVec2(width, height));
```

#### 4. 实现平移和缩放

**平移**：
```cpp
if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
    ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
    offset_x += delta.x;
    offset_y += delta.y;
}
```

**缩放**：
```cpp
if (ImGui::IsItemHovered()) {
    float wheel = ImGui::GetIO().MouseWheel;
    if (wheel > 0) scale *= 1.1f;
    else if (wheel < 0) scale /= 1.1f;
}
```

## 使用示例

### 集成到 DearTsApplication

```cpp
// 在 dearts_application.cpp 中
void DearTsApplication::setup_views() {
    // 注册 SDL 渲染示例视图
    ContentRegistry::Views::add<DearTs::Examples::SDLRenderView>();
}
```

### 操作说明

- **收起/展开**：点击 "SDL3 渲染区域" 标题栏
- **平移**：鼠标左键拖拽
- **缩放**：鼠标滚轮
- **重置视图**：点击 "重置视图" 按钮

## 关键实现细节

### 1. 坐标系统转换

**ImGui 坐标系**：
- 原点在窗口左上角
- 单位：屏幕像素

**SDL 坐标系**：
- 原点在渲染目标左上角
- 单位：纹理像素

**变换应用**：
```cpp
ImVec2 p0(
    child_cursor.x + (display_size.x - scaled_width) * 0.5f + offset_x,
    child_cursor.y + (display_size.y - scaled_height) * 0.5f + offset_y
);
```

### 2. 性能优化

**问题**：每帧都重新渲染到纹理会很慢

**解决方案**：使用缓存
```cpp
// 只在内容变化时重新渲染
bool content_changed = check_content_changed();
if (content_changed) {
    render_to_texture();
}

// 或者使用定时器
static float last_render_time = 0.0f;
if (current_time - last_render_time > 0.033f) { // 30 FPS
    render_to_texture();
    last_render_time = current_time;
}
```

### 3. 纹理生命周期管理

```cpp
// 构造时创建
SDLRenderView::SDLRenderView() {
    SDL_Renderer* renderer = get_renderer();
    m_texture = SDL_CreateTexture(...);
}

// 析构时销毁
SDLRenderView::~SDLRenderView() {
    if (m_texture) {
        SDL_DestroyTexture(m_texture);
        m_texture = nullptr;
    }
}
```

**重要**：确保在 SDL_Renderer 销毁前销毁纹理

### 4. 与 ImGui 集成

使用 `ImGui::InvisibleButton()` 接收鼠标事件：
```cpp
ImGui::InvisibleButton("Canvas", size);

if (ImGui::IsItemHovered()) {
    // 处理滚轮缩放
}

if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
    // 处理拖拽开始
}
```

## 应用场景

### 1. 游戏/模拟器中的小地图
- SDL 渲染游戏世界
- ImGui 显示在小窗口
- 支持平移/缩放查看

### 2. 数据可视化
- SDL 绘制大量图形（性能高）
- ImGui 提供界面框架
- 用户交互控制视图

### 3. 图像编辑器
- SDL 渲染图像和处理效果
- ImGui 提供工具栏和参数控制
- 平移/缩放查看图像细节

### 4. 调试视图
- SDL 渲染调试图形（碰撞盒、路径等）
- ImGui 可折叠的调试面板
- 不影响主渲染流程

## 扩展功能

### 1. 添加多级缩放

```cpp
// 支持不同缩放级别
float zoom_levels[] = { 0.25f, 0.5f, 1.0f, 2.0f, 4.0f };
int current_zoom = 2; // 默认 1.0x

// Ctrl+滚轮切换级别
if (ImGui::GetIO().KeyCtrl) {
    if (wheel > 0) current_zoom = std::min(current_zoom + 1, 4);
    else current_zoom = std::max(current_zoom - 1, 0);
    scale = zoom_levels[current_zoom];
}
```

### 2. 添加视图快照

```cpp
// 保存当前视图状态
struct ViewState {
    float offset_x, offset_y;
    float scale;
};

std::vector<ViewState> view_stack;

// 保存
view_stack.push_back(m_transform);

// 恢复
if (!view_stack.empty()) {
    m_transform = view_stack.back();
    view_stack.pop_back();
}
```

### 3. 添加选择和标注

```cpp
// 在纹理上绘制选择框
if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
    ImVec2 drag_start = ImGui::GetItemActiveMin();
    ImVec2 drag_end = ImGui::GetMousePos();

    // 绘制选择矩形
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRect(drag_start, drag_end, IM_COL32(255, 255, 0, 255));
}
```

## 常见问题

### Q1: 纹理显示为黑色/白色方块

**原因**：纹理转换失败或未正确渲染

**解决**：
```cpp
// 检查纹理是否有效
if (!m_texture) {
    LOG_ERROR("Texture is null");
    return;
}

// 验证渲染目标设置
if (!SDL_SetRenderTarget(renderer, m_texture)) {
    LOG_ERROR("Failed to set render target: {}", SDL_GetError());
    return;
}
```

### Q2: 缩放后内容模糊

**原因**：纹理分辨率不够高

**解决**：
```cpp
// 创建更高分辨率的纹理
constexpr int TEXTURE_WIDTH = 1920;  // 提高分辨率
constexpr int TEXTURE_HEIGHT = 1080;

// 显示时缩小
ImGui::Image(m_texture_id, ImVec2(800, 600));
```

### Q3: 性能问题（FPS 下降）

**原因**：每帧都重新渲染到纹理

**解决**：使用缓存，只在内容变化时渲染
```cpp
if (m_needs_redraw) {
    render_to_texture();
    m_needs_redraw = false;
}
```

### Q4: 鼠标位置不准确

**原因**：未正确处理坐标转换

**解决**：
```cpp
// ImGui 窗口坐标 → SDL 纹理坐标
ImVec2 mouse_screen = ImGui::GetMousePos();
ImVec2 mouse_local(
    (mouse_screen.x - canvas_pos.x - offset_x) / scale,
    (mouse_screen.y - canvas_pos.y - offset_y) / scale
);
```

## 参考资源

- [ImGui Image Documentation](https://github.com/ocornut/imgui/wiki/Image)
- [SDL3 Render Target](https://wiki.libsdl.org/SDL3/CategoryRender)
- [DearTs Framework View System](../../SKILL.md)

## 总结

SDL3 + ImGui 混合渲染的关键：

1. **离屏纹理作为渲染目标** - `SDL_TEXTUREACCESS_TARGET`
2. **ImGui::Image() 显示** - 自动裁剪，集成良好
3. **鼠标事件处理** - `ImGui::IsItemXXX()` 函数
4. **坐标变换** - ImGui 坐标 ↔ SDL 坐标
5. **性能优化** - 缓存渲染结果，按需更新

这种方案既利用了 SDL3 的高性能渲染能力，又享受了 ImGui 的界面布局优势！
