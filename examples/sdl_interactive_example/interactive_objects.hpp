/**
 * @file interactive_objects.hpp
 * @brief 可交互对象系统定义
 * @details 实现可在 SDL3 纹理中交互的对象
 */

#pragma once

#include <SDL3/SDL.h>
#include <imgui.h>
#include <string>
#include <vector>
#include <memory>
#include <cmath>

// ImGui 颜色宏辅助（如果未定义）
#ifndef IM_COL32_R
#define IM_COL32_R(col) ((col >> IM_COL32_R_SHIFT) & 0xFF)
#endif
#ifndef IM_COL32_G
#define IM_COL32_G(col) ((col >> IM_COL32_G_SHIFT) & 0xFF)
#endif
#ifndef IM_COL32_B
#define IM_COL32_B(col) ((col >> IM_COL32_B_SHIFT) & 0xFF)
#endif
#ifndef IM_COL32_A
#define IM_COL32_A(col) ((col >> IM_COL32_A_SHIFT) & 0xFF)
#endif

/**
 * @struct InteractiveObject
 * @brief 可交互对象基类
 */
struct InteractiveObject {
    int id = 0;                    ///< 唯一 ID
    std::string name;              ///< 对象名称
    std::string description;       ///< 描述信息

    // 位置和尺寸
    ImVec2 center{0, 0};           ///< 中心点
    float size = 50.0f;            ///< 大小（半径或半边长）
    float rotation = 0.0f;         ///< 旋转角度（弧度）

    // 颜色
    ImU32 color = IM_COL32(255, 255, 255, 255);        ///< 正常颜色
    ImU32 hover_color = IM_COL32(255, 255, 100, 255);  ///< 悬停颜色
    ImU32 selected_color = IM_COL32(100, 255, 255, 255); ///< 选中颜色

    // 状态
    bool is_hovered = false;       ///< 是否悬停
    bool is_selected = false;      ///< 是否被选中
    bool is_visible = true;        ///< 是否可见

    // 动画参数
    float anim_offset = 0.0f;      ///< 动画偏移（让每个对象动画不同步）

    /**
     * @brief 检测点是否在对象内
     * @param point 要检测的点（纹理坐标）
     * @return true 如果点在对象内
     */
    virtual bool contains(ImVec2 point) const = 0;

    /**
     * @brief 渲染对象到 SDL 纹理
     * @param renderer SDL 渲染器
     */
    virtual void render(SDL_Renderer* renderer) = 0;

    /**
     * @brief 获取对象的边界框
     * @return 边界矩形（纹理坐标）
     */
    virtual SDL_FRect get_bounds() const = 0;

    /**
     * @brief 获取对象的描述信息
     * @return 描述字符串
     */
    virtual std::string get_info() const {
        return name + " (ID: " + std::to_string(id) + ")";
    }

    virtual ~InteractiveObject() = default;
};

/**
 * @struct InteractiveRect
 * @brief 可交互矩形
 */
struct InteractiveRect : public InteractiveObject {
    bool contains(ImVec2 point) const override {
        // 简化检测：使用不旋转的边界框
        SDL_FRect bounds = get_bounds();
        return point.x >= bounds.x && point.x <= bounds.x + bounds.w &&
               point.y >= bounds.y && point.y <= bounds.y + bounds.h;
    }

    void render(SDL_Renderer* renderer) override {
        if (!is_visible) return;

        // 计算矩形的四个角（考虑旋转）
        std::vector<SDL_FPoint> corners(4);
        for (int i = 0; i < 4; i++) {
            float theta = rotation + i * 3.14159f / 2.0f;
            corners[i].x = center.x + size * std::cos(theta);
            corners[i].y = center.y + size * std::sin(theta);
        }

        // 选择颜色（优先级：选中 > 悬停 > 正常）
        ImU32 render_color = color;
        if (is_selected) {
            render_color = selected_color;
        } else if (is_hovered) {
            render_color = hover_color;
        }

        SDL_SetRenderDrawColor(renderer,
                               IM_COL32_R(render_color),
                               IM_COL32_G(render_color),
                               IM_COL32_B(render_color),
                               IM_COL32_A(render_color));

        // 绘制矩形边框
        for (int i = 0; i < 4; i++) {
            SDL_RenderLine(renderer,
                           corners[i].x, corners[i].y,
                           corners[(i + 1) % 4].x, corners[(i + 1) % 4].y);
        }

        // 如果被选中，绘制填充效果（用更粗的线条）
        if (is_selected) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 128);
            for (int i = 0; i < 4; i++) {
                // 绘制内框
                float t = 0.2f; // 内缩比例
                SDL_FPoint p1 = {
                    corners[i].x + (center.x - corners[i].x) * t,
                    corners[i].y + (center.y - corners[i].y) * t
                };
                SDL_FPoint p2 = {
                    corners[(i + 1) % 4].x + (center.x - corners[(i + 1) % 4].x) * t,
                    corners[(i + 1) % 4].y + (center.y - corners[(i + 1) % 4].y) * t
                };
                SDL_RenderLine(renderer, p1.x, p1.y, p2.x, p2.y);
            }
        }

        // 绘制中心点（辅助视觉）
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_FPoint center_point = {center.x, center.y};
        SDL_RenderPoint(renderer, center_point.x, center_point.y);
    }

    SDL_FRect get_bounds() const override {
        // 简化：返回不旋转的边界框
        return SDL_FRect{
            center.x - size,
            center.y - size,
            size * 2.0f,
            size * 2.0f
        };
    }

    std::string get_info() const override {
        return "矩形 " + name + " (ID: " + std::to_string(id) + ")";
    }
};

/**
 * @struct InteractiveCircle
 * @brief 可交互圆形
 */
struct InteractiveCircle : public InteractiveObject {
    bool contains(ImVec2 point) const override {
        float dx = point.x - center.x;
        float dy = point.y - center.y;
        return (dx * dx + dy * dy) <= (size * size);
    }

    void render(SDL_Renderer* renderer) override {
        if (!is_visible) return;

        int num_segments = 32;

        // 选择颜色
        ImU32 render_color = color;
        if (is_selected) {
            render_color = selected_color;
        } else if (is_hovered) {
            render_color = hover_color;
        }

        SDL_SetRenderDrawColor(renderer,
                               IM_COL32_R(render_color),
                               IM_COL32_G(render_color),
                               IM_COL32_B(render_color),
                               IM_COL32_A(render_color));

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

        // 如果悬停或选中，绘制高亮外圈
        if (is_hovered || is_selected) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

            float outer_size = size + 8.0f;
            for (int i = 0; i < num_segments; i++) {
                float theta1 = 2.0f * 3.14159f * i / num_segments;
                float theta2 = 2.0f * 3.14159f * (i + 1) / num_segments;

                SDL_FPoint p1 = {
                    center.x + outer_size * std::cos(theta1),
                    center.y + outer_size * std::sin(theta1)
                };
                SDL_FPoint p2 = {
                    center.x + outer_size * std::cos(theta2),
                    center.y + outer_size * std::sin(theta2)
                };

                SDL_RenderLine(renderer, p1.x, p1.y, p2.x, p2.y);
            }
        }

        // 如果被选中，绘制中心十字
        if (is_selected) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 200);
            float cross_size = size * 0.3f;
            SDL_RenderLine(renderer,
                           center.x - cross_size, center.y,
                           center.x + cross_size, center.y);
            SDL_RenderLine(renderer,
                           center.x, center.y - cross_size,
                           center.x, center.y + cross_size);
        }
    }

    SDL_FRect get_bounds() const override {
        return SDL_FRect{
            center.x - size,
            center.y - size,
            size * 2.0f,
            size * 2.0f
        };
    }

    std::string get_info() const override {
        return "圆形 " + name + " (ID: " + std::to_string(id) + ")";
    }
};

/**
 * @class ObjectManager
 * @brief 对象管理器
 */
class ObjectManager {
private:
    std::vector<std::unique_ptr<InteractiveObject>> m_objects;
    int m_next_id = 1;
    InteractiveObject* m_selected_object = nullptr;
    bool m_needs_redraw = true;

public:
    /**
     * @brief 添加矩形
     */
    InteractiveRect* add_rect(ImVec2 center, float size, ImU32 color,
                              const std::string& name = "") {
        auto rect = std::make_unique<InteractiveRect>();
        rect->id = m_next_id++;
        rect->name = name.empty() ? "矩形" : name;
        rect->description = "旋转的红色矩形";
        rect->center = center;
        rect->size = size;
        rect->color = color;
        rect->hover_color = IM_COL32(255, 255, 100, 255);  // 黄色悬停
        rect->selected_color = IM_COL32(100, 255, 255, 255); // 青色选中

        m_objects.push_back(std::move(rect));
        m_needs_redraw = true;

        return static_cast<InteractiveRect*>(m_objects.back().get());
    }

    /**
     * @brief 添加圆形
     */
    InteractiveCircle* add_circle(ImVec2 center, float size, ImU32 color,
                                  const std::string& name = "") {
        auto circle = std::make_unique<InteractiveCircle>();
        circle->id = m_next_id++;
        circle->name = name.empty() ? "圆形" : name;
        circle->description = "脉冲的绿色圆形";
        circle->center = center;
        circle->size = size;
        circle->color = color;
        circle->hover_color = IM_COL32(100, 255, 100, 255); // 绿色悬停
        circle->selected_color = IM_COL32(255, 100, 255, 255); // 紫色选中

        m_objects.push_back(std::move(circle));
        m_needs_redraw = true;

        return static_cast<InteractiveCircle*>(m_objects.back().get());
    }

    /**
     * @brief 渲染所有对象到纹理
     */
    void render_all(SDL_Renderer* renderer) {
        for (auto& obj : m_objects) {
            obj->render(renderer);
        }
        m_needs_redraw = false;
    }

    /**
     * @brief 更新动画
     */
    void update(float time) {
        for (auto& obj : m_objects) {
            // 矩形旋转
            if (auto* rect = dynamic_cast<InteractiveRect*>(obj.get())) {
                float old_rotation = rect->rotation;
                rect->rotation = time * 2.0f + rect->anim_offset;

                // 如果旋转了，标记需要重绘
                if (std::abs(rect->rotation - old_rotation) > 0.01f) {
                    m_needs_redraw = true;
                }
            }

            // 圆形脉冲
            if (auto* circle = dynamic_cast<InteractiveCircle*>(obj.get())) {
                float old_size = circle->size;
                float pulse = 0.5f + 0.5f * std::sin(time * 3.0f + circle->anim_offset);
                circle->size = 50.0f + 30.0f * pulse;

                // 如果大小变化了，标记需要重绘
                if (std::abs(circle->size - old_size) > 0.1f) {
                    m_needs_redraw = true;
                }
            }
        }
    }

    /**
     * @brief 检测悬停（返回最上层的对象）
     */
    InteractiveObject* check_hover(ImVec2 tex_pos) {
        // 重置所有悬停状态
        for (auto& obj : m_objects) {
            if (obj->is_hovered) {
                obj->is_hovered = false;
                m_needs_redraw = true;
            }
        }

        // 从后往前检测（后绘制的在上面）
        for (auto it = m_objects.rbegin(); it != m_objects.rend(); ++it) {
            if ((*it)->contains(tex_pos) && (*it)->is_visible) {
                (*it)->is_hovered = true;
                m_needs_redraw = true;
                return it->get();
            }
        }

        return nullptr;
    }

    /**
     * @brief 检测点击
     */
    InteractiveObject* check_click(ImVec2 tex_pos) {
        // 取消之前的选择
        if (m_selected_object) {
            m_selected_object->is_selected = false;
            m_needs_redraw = true;
        }

        // 从后往前检测
        for (auto it = m_objects.rbegin(); it != m_objects.rend(); ++it) {
            if ((*it)->contains(tex_pos) && (*it)->is_visible) {
                (*it)->is_selected = true;
                m_selected_object = it->get();
                m_needs_redraw = true;
                return m_selected_object;
            }
        }

        m_selected_object = nullptr;
        return nullptr;
    }

    /**
     * @brief 获取选中的对象
     */
    InteractiveObject* get_selected() const {
        return m_selected_object;
    }

    /**
     * @brief 取消选择
     */
    void deselect_all() {
        for (auto& obj : m_objects) {
            obj->is_selected = false;
        }
        m_selected_object = nullptr;
        m_needs_redraw = true;
    }

    /**
     * @brief 获取所有对象
     */
    const std::vector<std::unique_ptr<InteractiveObject>>& get_objects() const {
        return m_objects;
    }

    /**
     * @brief 是否需要重绘
     */
    bool needs_redraw() const {
        return m_needs_redraw;
    }

    /**
     * @brief 标记需要重绘
     */
    void mark_dirty() {
        m_needs_redraw = true;
    }

    /**
     * @brief 清空所有对象
     */
    void clear() {
        m_objects.clear();
        m_selected_object = nullptr;
        m_next_id = 1;
        m_needs_redraw = true;
    }

    /**
     * @brief 获取对象数量
     */
    size_t get_count() const {
        return m_objects.size();
    }
};
