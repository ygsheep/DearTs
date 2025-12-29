#ifndef DEARTS_DEV_EXAMPLES_SDL_RENDER_VIEW_HPP
#define DEARTS_DEV_EXAMPLES_SDL_RENDER_VIEW_HPP

/**
 * @file sdl_render_view.hpp
 * @brief 在 ImGui 中嵌入 SDL3 渲染内容的示例 View
 *
 * 功能演示：
 * 1. ImGui 可收起/展开区域
 * 2. SDL3 直接渲染内容到纹理
 * 3. 支持平移（鼠标拖拽）和缩放（滚轮）
 * 4. 与 ImGui 集成
 */

#include "../../core/ui/view.h"
#include <SDL3/SDL.h>
#include <imgui.h>
#include <memory>

namespace DearTs {
namespace Examples {

/**
 * @class SDLRenderView
 * @brief 演示如何在 ImGui 中嵌入 SDL3 渲染内容
 *
 * 实现原理：
 * 1. 创建离屏 SDL 纹理作为渲染目标
 * 2. 将内容绘制到纹理
 * 3. 使用 ImGui::Image() 将纹理显示在 ImGui 界面中
 * 4. 处理鼠标事件实现平移和缩放
 */
class SDLRenderView : public Core::UI::View {
public:
    SDLRenderView();
    ~SDLRenderView() override;

    std::string getName() const override {
        return "SDL3 渲染示例";
    }

    void draw_content() override;

private:
    /**
     * @brief 初始化 SDL 纹理
     */
    bool init_texture(SDL_Renderer* renderer);

    /**
     * @brief 渲染内容到纹理
     */
    void render_to_texture();

    /**
     * @brief 处理鼠标交互
     */
    void handle_input();

    /**
     * @brief 绘制一些示例图形（使用 SDL3）
     */
    void draw_sample_graphics(SDL_Renderer* renderer, const SDL_FRect& rect);

private:
    // SDL 纹理（渲染目标）
    SDL_Texture* m_texture = nullptr;

    // 纹理尺寸
    static constexpr int TEXTURE_WIDTH = 800;
    static constexpr int TEXTURE_HEIGHT = 600;

    // 视图变换参数
    struct ViewTransform {
        float offset_x = 0.0f;    ///< X 轴偏移
        float offset_y = 0.0f;    ///< Y 轴偏移
        float scale = 1.0f;       ///< 缩放比例
    } m_transform;

    // 交互状态
    bool m_is_dragging = false;
    ImVec2 m_drag_start_pos;

    // ImGui 纹理 ID（SDL_Texture 转换而来）
    ImTextureID m_texture_id = nullptr;

    // 示例数据（用于绘制动态内容）
    float m_animation_time = 0.0f;
};

} // namespace Examples
} // namespace DearTs

#endif // DEARTS_DEV_EXAMPLES_SDL_RENDER_VIEW_HPP
