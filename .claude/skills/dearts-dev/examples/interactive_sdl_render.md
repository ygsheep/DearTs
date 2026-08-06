# SDL3 + ImGui 对象级交互实现指南

## 问题分析

### 当前限制

```
SDL3 纹理（像素数据）
    ↓
ImGui::Image() 显示
    ↓
只能检测整个纹理的鼠标事件
    ❌ 无法知道鼠标在哪个图形上
```

### 目标

```
SDL3 渲染 + 对象信息
    ↓
ImGui::Image() 显示
    ↓
坐标转换 + 对象检测
    ✅ 知道鼠标在哪个图形上
    ✅ 触发对应对象的交互
```

---

## 解决方案架构

### 1. 对象系统

定义可交互的对象：

```cpp
struct RenderObject {
    enum Type {
        Rectangle,  // 矩形
        Circle,     // 圆圈
        Line,       // 线条
        Text        // 文本
    };

    Type type;
    SDL_FRect bounds;     // 边界框
    ImVec2 center;        // 中心点
    float rotation;       // 旋转角度
    ImU32 color;          // 颜色
    bool is_hovered;      // 是否悬停
    bool is_clicked;      // 是否被点击
    int id;               // 唯一 ID

    // 检测点是否在对象内
    bool contains(float x, float y) const;
};
```

### 2. 坐标转换系统

```cpp
// ImGui 屏幕坐标 → SDL 纹理坐标
ImVec2 screen_to_texture(ImVec2 screen_pos) {
    ImVec2 child_pos = ImGui::GetCursorScreenPos();

    // 应用逆变换
    float tex_x = (screen_pos.x - child_pos.x - offset_x) / scale;
    float tex_y = (screen_pos.y - child_pos.y - offset_y) / scale;

    return ImVec2(tex_x, tex_y);
}

// SDL 纹理坐标 → ImGui 屏幕坐标
ImVec2 texture_to_screen(ImVec2 tex_pos) {
    ImVec2 child_pos = ImGui::GetCursorScreenPos();

    float screen_x = child_pos.x + tex_pos.x * scale + offset_x;
    float screen_y = child_pos.y + tex_pos.y * scale + offset_y;

    return ImVec2(screen_x, screen_y);
}
```

### 3. 对象管理器

```cpp
class ObjectManager {
private:
    std::vector<std::unique_ptr<RenderObject>> m_objects;

public:
    // 添加对象
    void add_object(RenderObject* obj);

    // 渲染所有对象到纹理
    void render_all(SDL_Renderer* renderer);

    // 检测鼠标悬停
    RenderObject* check_hover(float tex_x, float tex_y);

    // 检测点击
    RenderObject* check_click(float tex_x, float tex_y);

    // 更新对象状态
    void update(float time);
};
```

---

## 完整实现代码

### 步骤 1：定义可交互对象

```cpp
struct InteractiveObject {
    int id = 0;
    std::string name;

    // 位置和尺寸
    ImVec2 center;
    float size = 50.0f;
    float rotation = 0.0f;

    // 颜色
    ImU32 color = IM_COL32(255, 255, 255, 255);
    ImU32 hover_color = IM_COL32(255, 255, 100, 255);

    // 状态
    bool is_hovered = false;
    bool is_clicked = false;
    bool is_dragging = false;

    // 动画
    float anim_offset = 0.0f;

    // 检测点是否在对象内
    virtual bool contains(ImVec2 point) const = 0;

    // 渲染到 SDL 纹理
    virtual void render(SDL_Renderer* renderer) = 0;

    // 获取边界框
    virtual SDL_FRect get_bounds() const = 0;
};
```

### 步骤 2：实现具体对象类型

```cpp
// 可交互矩形
struct InteractiveRect : public InteractiveObject {
    bool contains(ImVec2 point) const override {
        // 简化：假设不旋转的矩形检测
        SDL_FRect bounds = get_bounds();
        return point.x >= bounds.x && point.x <= bounds.x + bounds.w &&
               point.y >= bounds.y && point.y <= bounds.y + bounds.h;
    }

    void render(SDL_Renderer* renderer) override {
        // 计算矩形的四个角（考虑旋转）
        std::vector<SDL_FPoint> corners(4);
        for (int i = 0; i < 4; i++) {
            float theta = rotation + i * 3.14159f / 2.0f;
            corners[i].x = center.x + size * std::cos(theta);
            corners[i].y = center.y + size * std::sin(theta);
        }

        // 设置颜色（悬停时高亮）
        if (is_hovered || is_clicked) {
            SDL_SetRenderDrawColor(renderer,
                                   IM_COL32_R(hover_color),
                                   IM_COL32_G(hover_color),
                                   IM_COL32_B(hover_color),
                                   IM_COL32_A(hover_color));
        } else {
            SDL_SetRenderDrawColor(renderer,
                                   IM_COL32_R(color),
                                   IM_COL32_G(color),
                                   IM_COL32_B(color),
                                   IM_COL32_A(color));
        }

        // 绘制矩形边框
        for (int i = 0; i < 4; i++) {
            SDL_RenderLine(renderer,
                           corners[i].x, corners[i].y,
                           corners[(i + 1) % 4].x, corners[(i + 1) % 4].y);
        }

        // 如果被点击，填充半透明
        if (is_clicked) {
            // 填充矩形（SDL3 需要自己实现填充逻辑）
        }
    }

    SDL_FRect get_bounds() const override {
        return SDL_FRect{
            center.x - size,
            center.y - size,
            size * 2,
            size * 2
        };
    }
};

// 可交互圆形
struct InteractiveCircle : public InteractiveObject {
    bool contains(ImVec2 point) const override {
        float dx = point.x - center.x;
        float dy = point.y - center.y;
        return (dx * dx + dy * dy) <= (size * size);
    }

    void render(SDL_Renderer* renderer) override {
        int num_segments = 32;

        // 设置颜色
        if (is_hovered || is_clicked) {
            SDL_SetRenderDrawColor(renderer,
                                   IM_COL32_R(hover_color),
                                   IM_COL32_G(hover_color),
                                   IM_COL32_B(hover_color),
                                   IM_COL32_A(hover_color));
        } else {
            SDL_SetRenderDrawColor(renderer,
                                   IM_COL32_R(color),
                                   IM_COL32_G(color),
                                   IM_COL32_B(color),
                                   IM_COL32_A(color));
        }

        // 绘制圆形
        for (int i = 0; i < num_segments; i++) {
            float theta1 = 2.0f * 3.14159f * i / num_segments;
            float theta2 = 2.0f * 3.14159f * (i + 1) / num_segments;

            SDL_FPoint p1 = {
                center.x + size * std::cos(theta1),
                center.y + size * std::sin(theta1)
            };
            SDL_FPoint p2 = {
                center.x + size * std::cos(theta2),
                center.y + size * std::sin(theta2)
            };

            SDL_RenderLine(renderer, p1.x, p1.y, p2.x, p2.y);
        }

        // 如果悬停，绘制高亮边框
        if (is_hovered) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            for (int i = 0; i < num_segments; i++) {
                float theta1 = 2.0f * 3.14159f * i / num_segments;
                float theta2 = 2.0f * 3.14159f * (i + 1) / num_segments;

                SDL_FPoint p1 = {
                    center.x + (size + 5) * std::cos(theta1),
                    center.y + (size + 5) * std::sin(theta1)
                };
                SDL_FPoint p2 = {
                    center.x + (size + 5) * std::cos(theta2),
                    center.y + (size + 5) * std::sin(theta2)
                };

                SDL_RenderLine(renderer, p1.x, p1.y, p2.x, p2.y);
            }
        }
    }

    SDL_FRect get_bounds() const override {
        return SDL_FRect{
            center.x - size,
            center.y - size,
            size * 2,
            size * 2
        };
    }
};
```

### 步骤 3：对象管理器

```cpp
class ObjectManager {
private:
    std::vector<std::unique_ptr<InteractiveObject>> m_objects;
    int m_next_id = 1;

public:
    // 添加矩形
    InteractiveRect* add_rect(ImVec2 center, float size, ImU32 color) {
        auto rect = std::make_unique<InteractiveRect>();
        rect->id = m_next_id++;
        rect->name = "Rect_" + std::to_string(rect->id);
        rect->center = center;
        rect->size = size;
        rect->color = color;
        rect->hover_color = IM_COL32(255, 255, 100, 255);

        m_objects.push_back(std::move(rect));
        return static_cast<InteractiveRect*>(m_objects.back().get());
    }

    // 添加圆形
    InteractiveCircle* add_circle(ImVec2 center, float size, ImU32 color) {
        auto circle = std::make_unique<InteractiveCircle>();
        circle->id = m_next_id++;
        circle->name = "Circle_" + std::to_string(circle->id);
        circle->center = center;
        circle->size = size;
        circle->color = color;
        circle->hover_color = IM_COL32(100, 255, 100, 255);

        m_objects.push_back(std::move(circle));
        return static_cast<InteractiveCircle*>(m_objects.back().get());
    }

    // 渲染所有对象
    void render_all(SDL_Renderer* renderer) {
        for (auto& obj : m_objects) {
            obj->render(renderer);
        }
    }

    // 更新动画
    void update(float time) {
        for (auto& obj : m_objects) {
            // 示例：旋转矩形
            if (auto* rect = dynamic_cast<InteractiveRect*>(obj.get())) {
                rect->rotation = time * 2.0f + rect->anim_offset;
            }

            // 示例：脉冲圆形
            if (auto* circle = dynamic_cast<InteractiveCircle*>(obj.get())) {
                float pulse = 0.5f + 0.5f * std::sin(time * 3.0f + circle->anim_offset);
                circle->size = 50.0f + 30.0f * pulse;
            }
        }
    }

    // 检测悬停（返回最上层的对象）
    InteractiveObject* check_hover(ImVec2 tex_pos) {
        // 重置所有悬停状态
        for (auto& obj : m_objects) {
            obj->is_hovered = false;
        }

        // 从后往前检测（后绘制的在上面）
        for (auto it = m_objects.rbegin(); it != m_objects.rend(); ++it) {
            if ((*it)->contains(tex_pos)) {
                (*it)->is_hovered = true;
                return it->get();
            }
        }

        return nullptr;
    }

    // 检测点击
    InteractiveObject* check_click(ImVec2 tex_pos) {
        // 从后往前检测
        for (auto it = m_objects.rbegin(); it != m_objects.rend(); ++it) {
            if ((*it)->contains(tex_pos)) {
                (*it)->is_clicked = !(*it)->is_clicked; // 切换状态
                return it->get();
            }
        }

        return nullptr;
    }

    // 获取所有对象
    const std::vector<std::unique_ptr<InteractiveObject>>& get_objects() const {
        return m_objects;
    }

    // 清空所有对象
    void clear() {
        m_objects.clear();
    }
};
```

### 步骤 4：集成到主程序

```cpp
// 全局对象管理器
ObjectManager g_object_manager;

void init() {
    // ... 原有初始化代码 ...

    // 创建可交互对象
    // 旋转矩形
    g_object_manager.add_rect(
        ImVec2(TEXTURE_WIDTH / 2, TEXTURE_HEIGHT / 2),
        100.0f,
        IM_COL32(255, 100, 100, 255)
    );

    // 脉冲圆圈
    g_object_manager.add_circle(
        ImVec2(200.0f, 200.0f),
        50.0f,
        IM_COL32(100, 255, 100, 255)
    );

    g_object_manager.add_circle(
        ImVec2(600.0f, 400.0f),
        60.0f,
        IM_COL32(100, 200, 255, 255)
    );
}

void render_to_texture() {
    // ... 原有代码 ...

    // 渲染所有对象
    g_object_manager.render_all(renderer);

    // ... 恢复渲染目标 ...
}

void draw_gui() {
    // ... 原有 ImGui 代码 ...

    // 创建不可见按钮
    ImGui::InvisibleButton("canvas", display_size);
    bool is_hovered = ImGui::IsItemHovered();

    if (is_hovered) {
        // 获取鼠标在 ImGui 中的位置
        ImVec2 mouse_pos = ImGui::GetMousePos();
        ImVec2 child_pos = ImGui::GetCursorScreenPos();

        // 转换到纹理坐标
        ImVec2 tex_pos;
        tex_pos.x = (mouse_pos.x - child_pos.x - g_transform.offset_x) / g_transform.scale;
        tex_pos.y = (mouse_pos.y - child_pos.y - g_transform.offset_y) / g_transform.scale;

        // 检测悬停
        InteractiveObject* hovered_obj = g_object_manager.check_hover(tex_pos);

        // 检测点击
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            InteractiveObject* clicked_obj = g_object_manager.check_click(tex_pos);

            if (clicked_obj) {
                std::cout << "点击了对象: " << clicked_obj->name << std::endl;

                // 可以触发对象特定的事件
                on_object_clicked(clicked_obj);
            }
        }

        // 检测拖拽
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            // 如果拖拽的是某个对象...
            // 可以实现对象拖拽逻辑
        }

        // 显示悬停信息
        if (hovered_obj) {
            ImGui::SetTooltip("%s (ID: %d)", hovered_obj->name.c_str(), hovered_obj->id);
        }
    }

    // ... 原有视图变换代码 ...
}

// 对象点击回调
void on_object_clicked(InteractiveObject* obj) {
    std::cout << "对象被点击: " << obj->name << std::endl;

    // 可以在这里添加特定逻辑
    // - 播放音效
    // - 显示属性面板
    // - 触发动画
    // - 发送事件
}
```

---

## 高级交互功能

### 1. 对象拖拽

```cpp
struct InteractiveObject {
    bool is_dragging = false;
    ImVec2 drag_offset;
};

// 在 draw_gui() 中
if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
    InteractiveObject* clicked = g_object_manager.check_click(tex_pos);
    if (clicked) {
        clicked->is_dragging = true;
        clicked->drag_offset = ImVec2(
            clicked->center.x - tex_pos.x,
            clicked->center.y - tex_pos.y
        );
    }
}

if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
    for (auto& obj : g_object_manager.get_objects()) {
        if (obj->is_dragging) {
            ImVec2 tex_pos = screen_to_texture(ImGui::GetMousePos());
            obj->center = ImVec2(
                tex_pos.x + obj->drag_offset.x,
                tex_pos.y + obj->drag_offset.y
            );
        }
    }
}

if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
    for (auto& obj : g_object_manager.get_objects()) {
        obj->is_dragging = false;
    }
}
```

### 2. 右键菜单

```cpp
// 在 draw_gui() 中
if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
    InteractiveObject* clicked = g_object_manager.check_click(tex_pos);
    if (clicked) {
        ImGui::OpenPopup("ObjectContextMenu");
    }
}

if (ImGui::BeginPopup("ObjectContextMenu")) {
    if (ImGui::MenuItem("删除对象")) {
        // 删除对象
    }
    if (ImGui::MenuItem("复制对象")) {
        // 复制对象
    }
    if (ImGui::MenuItem("属性...")) {
        // 显示属性面板
    }
    ImGui::EndPopup();
}
```

### 3. 键盘事件

```cpp
// 在 handle_events() 中
void handle_events() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        // ... 原有事件处理 ...

        // 键盘事件
        if (event.type == SDL_EVENT_KEY_DOWN) {
            switch (event.key.key) {
                case SDLK_SPACE:
                    // 暂停/恢复动画
                    g_animation_paused = !g_animation_paused;
                    break;

                case SDLK_DELETE:
                    // 删除选中的对象
                    if (g_selected_object) {
                        g_object_manager.remove(g_selected_object);
                        g_selected_object = nullptr;
                    }
                    break;

                case SDLK_ESCAPE:
                    // 取消选择
                    g_selected_object = nullptr;
                    break;
            }
        }
    }
}
```

---

## 性能优化

### 1. 空间分区

```cpp
class SpatialHash {
    // 使用网格或四叉树加速碰撞检测
    std::unordered_map<int, std::vector<InteractiveObject*>> m_cells;

    void insert(InteractiveObject* obj);
    std::vector<InteractiveObject*> query(SDL_FRect area);
};
```

### 2. 渲染缓存

```cpp
// 只在对象变化时重新渲染
if (g_object_manager.needs_redraw()) {
    render_to_texture();
    g_object_manager.clear_dirty_flag();
}
```

---

## 总结

### 实现的交互功能

✅ **对象级交互**：
- 点击检测
- 悬停高亮
- 对象拖拽
- 右键菜单
- 键盘快捷键

✅ **视图级交互**（已有）：
- 平移
- 缩放
- 区域收起

✅ **高级功能**：
- 对象选择
- 属性编辑
- 事件系统
- 动画控制

### 关键要点

1. **对象系统** - 每个图形都是对象，包含状态和数据
2. **坐标转换** - ImGui 坐标 ↔ 纹理坐标
3. **碰撞检测** - 检测鼠标在哪个对象上
4. **状态管理** - 跟踪悬停、点击、拖拽等状态
5. **事件分发** - 将鼠标事件分发到对应对象

这样就能实现和原生 SDL3 程序一样的交互效果了！
