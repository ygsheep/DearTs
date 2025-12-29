# SDL3 + ImGui 对象级交互示例

## ✨ 概述

本示例展示了如何在 ImGui 中嵌入 SDL3 渲染的内容，并实现**完整的对象级交互**功能。

与之前的 `sdl_render_example` 不同，这个示例支持：
- ✅ **点击对象** - 选择图形并高亮显示
- ✅ **悬停检测** - 鼠标悬停时对象高亮
- ✅ **对象信息提示** - Tooltip 显示详细信息
- ✅ **属性面板** - 显示选中对象的属性
- ✅ **键盘快捷键** - ESC 取消选择，空格暂停动画

## 🎯 功能演示

### 可交互对象

示例包含 4 个可交互对象：

| 对象 | 类型 | 位置 | 效果 |
|------|------|------|------|
| 中心矩形 | 矩形 | (400, 300) | 旋转动画 |
| 左上矩形 | 矩形 | (200, 200) | 旋转动画（不同步） |
| 绿色圆圈 | 圆形 | (600, 400) | 脉冲动画 |
| 蓝色圆圈 | 圆形 | (200, 450) | 脉冲动画（不同步） |

### 交互效果

| 交互 | 效果 |
|------|------|
| 🖱️ 鼠标移动 | 悬停的对象变为**黄色**高亮 |
| 🖱️ 左键点击 | 选中的对象变为**青色**高亮 |
| 🖱️ 右键拖拽 | 平移整个视图 |
| 🖱️ 滚轮滚动 | 缩放视图（0.1x - 10x） |
| ⌨️ ESC 键 | 取消选择 |
| ⌨️ 空格键 | 暂停/恢复动画 |

### 信息显示

- **Tooltip** - 悬停时显示对象名称和描述
- **选中对象信息** - 面板显示详细属性（ID、位置、大小、状态）
- **状态栏** - 显示对象数量、选中对象、悬停对象

## 🚀 快速开始

### 编译

```bash
cmake --build build --target sdl_interactive_example --config Debug
```

### 运行

```bash
# Windows
.\build\bin\Debug\sdl_interactive_example.exe

# Linux/macOS
./build/bin/sdl_interactive_example
```

### 控制台输出

程序启动时会显示：
```
=== SDL3 + ImGui 对象级交互示例 ===
操作说明：
  - 鼠标移动：悬停高亮对象
  - 鼠标左键点击：选择对象
  - 鼠标右键拖拽：平移视图
  - 鼠标滚轮：缩放视图
  - ESC 键：取消选择
  - 空格键：暂停/恢复动画
======================================
SDL3 初始化成功
SDL 窗口创建成功
SDL 渲染器创建成功
成功加载中文字体: resources/fonts/OPPOSans-M.ttf
ImGui 初始化成功
离屏纹理创建成功 (800x600)
已创建 4 个可交互对象
```

## 💡 核心技术

### 1. 对象系统

```cpp
// 可交互对象基类
struct InteractiveObject {
    int id;
    std::string name;
    ImVec2 center;
    float size;
    ImU32 color, hover_color, selected_color;
    bool is_hovered, is_selected;

    virtual bool contains(ImVec2 point) const = 0;
    virtual void render(SDL_Renderer* renderer) = 0;
};

// 具体实现
struct InteractiveRect : public InteractiveObject { ... };
struct InteractiveCircle : public InteractiveObject { ... };
```

### 2. 对象管理器

```cpp
class ObjectManager {
    // 添加对象
    InteractiveRect* add_rect(...);
    InteractiveCircle* add_circle(...);

    // 渲染所有对象
    void render_all(SDL_Renderer* renderer);

    // 更新动画
    void update(float time);

    // 交互检测
    InteractiveObject* check_hover(ImVec2 tex_pos);
    InteractiveObject* check_click(ImVec2 tex_pos);

    // 状态管理
    void deselect_all();
    InteractiveObject* get_selected();
};
```

### 3. 坐标转换

```cpp
// ImGui 屏幕 → SDL 纹理
ImVec2 screen_to_texture(ImVec2 screen_pos, ImVec2 child_pos) {
    tex_pos.x = (screen_pos.x - child_pos.x - offset_x) / scale;
    tex_pos.y = (screen_pos.y - child_pos.y - offset_y) / scale;
    return tex_pos;
}

// SDL 纹理 → ImGui 屏幕
ImVec2 texture_to_screen(ImVec2 tex_pos, ImVec2 child_pos) {
    screen_pos.x = child_pos.x + tex_pos.x * scale + offset_x;
    screen_pos.y = child_pos.y + tex_pos.y * scale + offset_y;
    return screen_pos;
}
```

### 4. 交互检测流程

```
ImGui 检测鼠标事件
    ↓
获取鼠标屏幕坐标
    ↓
转换到纹理坐标
    ↓
ObjectManager.check_hover()
    ↓
遍历对象（从上到下）
    ↓
检测点是否在对象内
    ↓
设置对象状态（is_hovered, is_selected）
    ↓
触发重新渲染（needs_redraw = true）
```

## 📊 对比：视图级 vs 对象级

### 视图级交互（sdl_render_example）

```
✅ 平移整个视图
✅ 缩放整个视图
❌ 无法点击对象
❌ 无法悬停检测
```

### 对象级交互（sdl_interactive_example）

```
✅ 平移整个视图
✅ 缩放整个视图
✅ 点击对象（选择）
✅ 悬停检测（高亮）
✅ 对象信息提示
✅ 属性面板
✅ 键盘快捷键
```

## 🎨 使用示例

### 创建新对象

```cpp
// 添加矩形
auto* rect = g_object_manager.add_rect(
    ImVec2(300, 200),  // 位置
    80.0f,             // 大小
    IM_COL32(255, 128, 0, 255),  // 颜色
    "橙色矩形"         // 名称
);

// 添加圆形
auto* circle = g_object_manager.add_circle(
    ImVec2(500, 400),
    60.0f,
    IM_COL32(128, 0, 255, 255),
    "紫色圆形"
);
```

### 自定义交互行为

```cpp
// 在 draw_gui() 中
if (g_selected_object) {
    // 显示属性编辑器
    ImGui::SliderFloat2("位置", &g_selected_object->center.x, 0, TEXTURE_WIDTH);
    ImGui::SliderFloat("大小", &g_selected_object->size, 10, 200);

    // 颜色选择器
    ImColor color(g_selected_object->color);
    if (ImGui::ColorEdit4("颜色", &color.Value.x)) {
        g_selected_object->color = color;
        g_object_manager.mark_dirty();  // 标记需要重绘
    }
}
```

## 🔧 扩展功能

### 1. 添加对象拖拽

```cpp
// 在 draw_gui() 的鼠标事件处理中
if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
    g_selected_object = g_object_manager.check_click(tex_pos);
    if (g_selected_object) {
        g_dragging_object = g_selected_object;
        g_drag_offset = ImVec2(
            g_dragging_object->center.x - tex_pos.x,
            g_dragging_object->center.y - tex_pos.y
        );
    }
}

if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) && g_dragging_object) {
    ImVec2 tex_pos = screen_to_texture(ImGui::GetMousePos(), child_pos);
    g_dragging_object->center = ImVec2(
        tex_pos.x + g_drag_offset.x,
        tex_pos.y + g_drag_offset.y
    );
    g_object_manager.mark_dirty();
}
```

### 2. 添加右键菜单

```cpp
if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && g_selected_object) {
    ImGui::OpenPopup("ObjectContext");
}

if (ImGui::BeginPopup("ObjectContext")) {
    if (ImGui::MenuItem("删除")) {
        g_object_manager.remove(g_selected_object);
    }
    if (ImGui::MenuItem("复制")) {
        // 复制对象
    }
    if (ImGui::MenuItem("属性...")) {
        // 打开属性编辑器
    }
    ImGui::EndPopup();
}
```

### 3. 添加对象类型

```cpp
// 三角形
struct InteractiveTriangle : public InteractiveObject {
    bool contains(ImVec2 point) const override {
        // 重心坐标法检测
        // ...
    }

    void render(SDL_Renderer* renderer) override {
        // 绘制三角形
        SDL_RenderLine(renderer, ...);
    }
};
```

## 📈 性能考虑

### 优化渲染

```cpp
// 只在对象变化时重新渲染
if (g_object_manager.needs_redraw()) {
    render_to_texture();
}
```

### 优化碰撞检测

```cpp
// 使用四叉树或空间分区加速
class SpatialIndex {
    std::vector<InteractiveObject*> query(ImVec2 area);
};
```

### 减少动画更新

```cpp
// 降低动画帧率
if (g_animation_time - last_render_time > 0.1f) {  // 10 FPS
    g_object_manager.update(g_animation_time);
}
```

## 🐛 常见问题

### Q1: 点击对象没有反应

**检查**：
1. 是否在纹理区域内点击？
2. 缩放是否太大或太小？
3. 对象是否被其他对象遮挡？

**解决**：
- 查看控制台是否有选中对象的输出
- 检查 `g_selected_object` 是否为空

### Q2: 悬停没有高亮

**原因**：对象动画更新可能触发重绘

**解决**：已在代码中处理，`update()` 会标记 `needs_redraw`

### Q3: 坐标转换不正确

**检查**：
- `child_pos` 是否正确获取？
- `offset_x` 和 `scale` 是否正确应用？

**调试**：
```cpp
// 添加调试输出
std::cout << "Mouse: " << mouse_pos.x << ", " << mouse_pos.y << std::endl;
std::cout << "Texture: " << tex_pos.x << ", " << tex_pos.y << std::endl;
```

## 📁 文件结构

```
examples/sdl_interactive_example/
├── interactive_objects.hpp  ✅ 对象系统定义（400 行）
├── main.cpp                  ✅ 主程序（600 行）
├── CMakeLists.txt            ✅ CMake 配置
└── README.md                 ✅ 本文件
```

## 🎓 学习要点

1. **对象系统设计** - 如何设计可交互的对象
2. **坐标转换** - ImGui 和 SDL 坐标系之间的转换
3. **事件分发** - 如何将 ImGui 事件分发到 SDL 对象
4. **状态管理** - 如何跟踪和管理对象状态
5. **按需渲染** - 只在内容变化时重新渲染

## 🔗 相关资源

- [基础渲染示例](../sdl_render_example/)
- [完整交互指南](../../dearts-dev/examples/interactive_sdl_render.md)
- [ImGui Input Guide](https://github.com/ocornut/imgui/wiki/Input)
- [SDL3 Renderer](https://wiki.libsdl.org/SDL3/CategoryRender)

## ✨ 总结

这个示例展示了如何实现**完整的对象级交互**，包括：

- ✅ 对象系统（InteractiveObject）
- ✅ 坐标转换（screen ↔ texture）
- ✅ 碰撞检测（contains）
- ✅ 事件处理（点击、悬停）
- ✅ 状态管理（hover, selected）
- ✅ 信息显示（Tooltip, 面板）

**与纯 SDL3 程序的交互体验几乎相同！** 🎉

现在你可以在 ImGui 中创建完全可交互的 SDL3 内容了！
