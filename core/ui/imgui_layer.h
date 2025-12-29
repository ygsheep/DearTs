#pragma once

#include <SDL3/SDL.h>
#include <memory>

struct ImGuiContext;

namespace DearTs {
namespace Core {
namespace UI {

/**
 * @brief ImGui 集成层
 *
 * 管理 ImGui 的初始化、渲染和关闭
 */
class ImGuiLayer {
public:
    ImGuiLayer();
    ~ImGuiLayer();

    /**
     * @brief 初始化 ImGui 层
     * @param window SDL 窗口
     * @return 是否成功
     */
    bool initialize(SDL_Window* window);

    /**
     * @brief 关闭 ImGui 层
     */
    void shutdown();

    /**
     * @brief 开始新帧
     */
    void begin_frame();

    /**
     * @brief 渲染 ImGui
     */
    void render();

    /**
     * @brief 处理 SDL 事件
     * @param event SDL 事件
     * @return 事件是否被处理
     */
    bool process_event(const SDL_Event& event);

    /**
     * @brief 获取 ImGui 上下文
     */
    [[nodiscard]] ImGuiContext* get_context() const;

    /**
     * @brief 设置字体缩放
     * @param scale 缩放因子
     */
    void set_font_scale(float scale);

    /**
     * @brief 获取字体缩放
     */
    [[nodiscard]] float get_font_scale() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace UI
} // namespace Core
} // namespace DearTs
