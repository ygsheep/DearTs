/**
 * @file sdl_render_view.cpp
 * @brief SDL3 + ImGui 混合渲染实现示例
 */

#include "sdl_render_view.hpp"
#include "../../core/app/application.h"
#include <imgui.h>
#include <cmath>
#include <algorithm>

namespace DearTs {
namespace Examples {

SDLRenderView::SDLRenderView() {
    LOG_INFO("SDLRenderView: Creating SDL3 render example view");

    // 获取 SDL_Renderer（从 Application）
    auto* app = Core::App::Application::instance();
    if (app) {
        SDL_Renderer* renderer = app->get_renderer();
        if (renderer && init_texture(renderer)) {
            LOG_INFO("SDLRenderView: Texture initialized successfully");
        } else {
            LOG_ERROR("SDLRenderView: Failed to initialize texture");
        }
    }
}

SDLRenderView::~SDLRenderView() {
    if (m_texture) {
        SDL_DestroyTexture(m_texture);
        m_texture = nullptr;
        LOG_INFO("SDLRenderView: Texture destroyed");
    }
}

bool SDLRenderView::init_texture(SDL_Renderer* renderer) {
    if (!renderer) {
        LOG_ERROR("SDLRenderView: Invalid renderer");
        return false;
    }

    // 创建离屏纹理作为渲染目标
    m_texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,  // 关键：设置为渲染目标
        TEXTURE_WIDTH,
        TEXTURE_HEIGHT
    );

    if (!m_texture) {
        LOG_ERROR("SDLRenderView: Failed to create texture: {}", SDL_GetError());
        return false;
    }

    // 设置纹理混合模式
    SDL_SetTextureBlendMode(m_texture, SDL_BLENDMODE_BLEND);

    // 转换为 ImTextureID（SDL3 可以直接转换）
    m_texture_id = (ImTextureID)m_texture;

    // 初始渲染一次
    render_to_texture();

    return true;
}

void SDLRenderView::draw_content() {
    // ImGui 可收起区域
    if (ImGui::CollapsingHeader("SDL3 渲染区域", ImGuiTreeNodeFlags_DefaultOpen)) {
        // 检查纹理是否有效
        if (!m_texture || !m_texture_id) {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "纹理未初始化");
            return;
        }

        // 获取 ImGui 窗口内容区域
        ImVec2 cursor_pos = ImGui::GetCursorScreenPos();

        // 计算显示尺寸（保持宽高比，适应窗口宽度）
        float available_width = ImGui::GetContentRegionAvail().x;
        float scale = available_width / static_cast<float>(TEXTURE_WIDTH);
        float display_height = static_cast<float>(TEXTURE_HEIGHT) * scale;
        display_height = std::min(display_height, 600.0f); // 限制最大高度

        ImVec2 display_size(available_width, display_height);

        // 显示 SDL 纹理
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));

        ImGui::BeginChild("SDLRenderContent", display_size, true,
                          ImGuiWindowFlags_NoScrollbar);

        // 处理鼠标输入（平移/缩放）
        handle_input();

        // 获取子窗口的绘制位置
        ImVec2 child_cursor = ImGui::GetCursorScreenPos();

        // 创建一个 ImGui Item 来接收鼠标事件
        ImGui::InvisibleButton("SDLRenderCanvas", display_size);

        // 检查鼠标是否在区域内
        bool is_hovered = ImGui::IsItemHovered();

        // 绘制纹理（应用平移和缩放）
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        // 计算纹理显示区域（应用变换）
        ImVec2 uv0(0, 0);
        ImVec2 uv1(1, 1);

        // 应用缩放（从中心缩放）
        float scaled_width = display_size.x * m_transform.scale;
        float scaled_height = display_size.y * m_transform.scale;

        // 应用平移
        ImVec2 p0(
            child_cursor.x + (display_size.x - scaled_width) * 0.5f + m_transform.offset_x,
            child_cursor.y + (display_size.y - scaled_height) * 0.5f + m_transform.offset_y
        );
        ImVec2 p1(p0.x + scaled_width, p0.y + scaled_height);

        // 绘制纹理
        draw_list->AddImage(m_texture_id, p0, p1, uv0, uv1, IM_COL32(255, 255, 255, 255));

        // 绘制边框
        draw_list->AddRect(p0, p1, IM_COL32(128, 128, 128, 255));

        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();

        // 显示控制信息
        ImGui::Spacing();
        if (ImGui::Button("重置视图")) {
            m_transform.offset_x = 0.0f;
            m_transform.offset_y = 0.0f;
            m_transform.scale = 1.0f;
        }
        ImGui::SameLine();
        ImGui::Text("偏移: (%.1f, %.1f) | 缩放: %.2fx",
                    m_transform.offset_x, m_transform.offset_y, m_transform.scale);

        // 更新动画时间
        m_animation_time += ImGui::GetIO().DeltaTime;

        // 定期重新渲染内容（演示动画效果）
        static float last_render_time = 0.0f;
        if (m_animation_time - last_render_time > 0.03f) { // 30 FPS
            render_to_texture();
            last_render_time = m_animation_time;
        }
    }
}

void SDLRenderView::handle_input() {
    // 获取鼠标位置
    ImVec2 mouse_pos = ImGui::GetMousePos();

    // 鼠标拖拽（平移）
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        m_is_dragging = true;
        m_drag_start_pos = mouse_pos;
    }

    if (m_is_dragging && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        ImVec2 mouse_delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
        m_transform.offset_x += mouse_delta.x;
        m_transform.offset_y += mouse_delta.y;
        ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
    }

    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        m_is_dragging = false;
    }

    // 鼠标滚轮（缩放）
    if (ImGui::IsItemHovered()) {
        ImVector<float>& mouse_wheel = ImGui::GetIO().MouseWheel;
        if (mouse_wheel.size() > 0) {
            float wheel = mouse_wheel[0];
            if (wheel != 0.0f) {
                float zoom_factor = 1.1f;
                if (wheel > 0.0f) {
                    m_transform.scale *= zoom_factor;
                } else {
                    m_transform.scale /= zoom_factor;
                }
                // 限制缩放范围
                m_transform.scale = std::clamp(m_transform.scale, 0.1f, 10.0f);
            }
        }
    }
}

void SDLRenderView::render_to_texture() {
    // 获取 SDL_Renderer
    auto* app = Core::App::Application::instance();
    if (!app) return;

    SDL_Renderer* renderer = app->get_renderer();
    if (!renderer || !m_texture) return;

    // 保存当前渲染目标
    SDL_Texture* old_target = SDL_GetRenderTarget(renderer);

    // 设置渲染目标到我们的纹理
    if (!SDL_SetRenderTarget(renderer, m_texture)) {
        LOG_ERROR("SDLRenderView: Failed to set render target: {}", SDL_GetError());
        return;
    }

    // 清空纹理（透明背景）
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);

    // 绘制示例内容
    SDL_FRect render_rect = {
        0.0f, 0.0f,
        static_cast<float>(TEXTURE_WIDTH),
        static_cast<float>(TEXTURE_HEIGHT)
    };
    draw_sample_graphics(renderer, render_rect);

    // 恢复原来的渲染目标
    SDL_SetRenderTarget(renderer, old_target);
}

void SDLRenderView::draw_sample_graphics(SDL_Renderer* renderer, const SDL_FRect& rect) {
    if (!renderer) return;

    float w = rect.w;
    float h = rect.h;

    // 1. 绘制背景（渐变色）
    for (int y = 0; y < static_cast<int>(h); y += 4) {
        float t = y / h;
        SDL_SetRenderDrawColor(renderer,
                               static_cast<Uint8>(50 * t),
                               static_cast<Uint8>(100 * (1 - t)),
                               static_cast<Uint8>(150),
                               255);
        SDL_FRect line_rect = { rect.x, rect.y + y, w, 4.0f };
        SDL_RenderFillRect(renderer, &line_rect);
    }

    // 2. 绘制旋转的矩形（动画）
    float angle = m_animation_time * 2.0f; // 旋转角度
    float rect_size = 100.0f;
    float center_x = w / 2.0f;
    float center_y = h / 2.0f;

    // 计算矩形的四个角（旋转后）
    float cos_a = std::cos(angle);
    float sin_a = std::sin(angle);

    SDL_FPoint corners[4];
    for (int i = 0; i < 4; i++) {
        float theta = angle + i * 3.14159f / 2.0f;
        corners[i].x = center_x + rect_size * std::cos(theta);
        corners[i].y = center_y + rect_size * std::sin(theta);
    }

    // 绘制旋转的矩形
    SDL_SetRenderDrawColor(renderer, 255, 100, 100, 200);
    for (int i = 0; i < 4; i++) {
        SDL_RenderLine(renderer,
                       corners[i].x, corners[i].y,
                       corners[(i + 1) % 4].x, corners[(i + 1) % 4].y);
    }

    // 3. 绘制圆形（脉冲效果）
    float pulse = 0.5f + 0.5f * std::sin(m_animation_time * 3.0f);
    float circle_radius = 50.0f + 30.0f * pulse;
    int num_segments = 32;

    SDL_SetRenderDrawColor(renderer, 100, 255, 100, 150);
    for (int i = 0; i < num_segments; i++) {
        float theta1 = 2.0f * 3.14159f * i / num_segments;
        float theta2 = 2.0f * 3.14159f * (i + 1) / num_segments;

        SDL_FPoint p1 = {
            center_x + circle_radius * std::cos(theta1),
            center_y + circle_radius * std::sin(theta1)
        };
        SDL_FPoint p2 = {
            center_x + circle_radius * std::cos(theta2),
            center_y + circle_radius * std::sin(theta2)
        };

        SDL_RenderLine(renderer, p1.x, p1.y, p2.x, p2.y);
    }

    // 4. 绘制文本（SDL3 渲染文本）
    // 注意：SDL3 需要使用字体渲染器，这里简化处理
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    // 5. 绘制网格线
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 50);
    for (float x = 0; x < w; x += 50.0f) {
        SDL_RenderLine(renderer, x, 0, x, h);
    }
    for (float y = 0; y < h; y += 50.0f) {
        SDL_RenderLine(renderer, 0, y, w, y);
    }

    // 6. 绘制鼠标位置指示器
    // (如果需要，可以从 handle_input 传递鼠标位置)
}

} // namespace Examples
} // namespace DearTs
