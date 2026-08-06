/**
 * @file background_renderer.h
 * @brief 背景图渲染器
 * @details 支持图片背景渲染，包括平铺、拉伸、居中等模式
 * @author DearTs Team
 * @date 2025
 * @version 1.0.0
 */

#pragma once

#include <imgui.h>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <string>
#include <vector>
#include <memory>

namespace DearTs::Core::UI {

/**
 * @brief 背景渲染模式
 */
enum class BackgroundMode {
    Stretch,      ///< 拉伸填充整个窗口
    Tile,         ///< 平铺模式
    Center,       ///< 居中显示
    Cover,        ///< 覆盖（保持比例，裁剪多余部分）
    Contain       ///< 包含（保持比例，完整显示）
};

/**
 * @brief 背景渲染器
 *
 * 提供背景图渲染功能：
 * - 支持多种渲染模式（拉伸、平铺、居中、覆盖、包含）
 * - 支持透明度和颜色混合
 * - 自动适配窗口大小变化
 * - 支持 PNG/JPG 等常见图片格式
 */
class BackgroundRenderer final {  // 单例类，禁止继承
public:
    /**
     * @brief 获取单例实例（线程安全，Magic Statics）
     */
    static BackgroundRenderer& instance() noexcept {
        static BackgroundRenderer inst;
        return inst;
    }

    // 删除所有拷贝和移动操作
    BackgroundRenderer(const BackgroundRenderer&) = delete;
    BackgroundRenderer& operator=(const BackgroundRenderer&) = delete;
    BackgroundRenderer(BackgroundRenderer&&) = delete;
    BackgroundRenderer& operator=(BackgroundRenderer&&) = delete;

    /**
     * @brief 设置 SDL 渲染器
     * @param renderer SDL 渲染器指针
     */
    void set_renderer(SDL_Renderer* renderer) { m_renderer = renderer; }

    /**
     * @brief 加载背景图片
     * @param image_path 图片路径（相对于 resources/images/ 或绝对路径）
     * @return 成功返回 true
     */
    bool load_background(const std::string& image_path);

    /**
     * @brief 渲染背景
     * @param renderer SDL 渲染器
     * @param viewport 当前视口大小
     */
    void render(SDL_Renderer* renderer, const ImVec2& viewport_size);

    /**
     * @brief 设置背景模式
     * @param mode 背景模式
     */
    void set_mode(BackgroundMode mode) { m_mode = mode; }

    /**
     * @brief 获取当前背景模式
     */
    [[nodiscard]] BackgroundMode get_mode() const { return m_mode; }

    /**
     * @brief 设置背景透明度 (0.0 - 1.0)
     * @param opacity 透明度值
     */
    void set_opacity(float opacity) { m_opacity = opacity; }

    /**
     * @brief 获取背景透明度
     */
    [[nodiscard]] float get_opacity() const { return m_opacity; }

    /**
     * @brief 启用/禁用背景渲染
     * @param enabled 是否启用
     */
    void set_enabled(bool enabled) { m_enabled = enabled; }

    /**
     * @brief 获取是否启用
     */
    [[nodiscard]] bool is_enabled() const { return m_enabled; }

    /**
     * @brief 设置背景混合颜色
     * @param color 混合颜色
     */
    void set_tint_color(const ImVec4& color) { m_tint_color = color; }

    /**
     * @brief 获取背景混合颜色
     */
    [[nodiscard]] ImVec4 get_tint_color() const { return m_tint_color; }

    /**
     * @brief 清空背景
     */
    void clear();

private:
    BackgroundRenderer();
    ~BackgroundRenderer();

    /**
     * @brief 从文件加载纹理
     * @param renderer SDL 渲染器
     * @param image_path 图片路径
     * @return 纹理指针，失败返回 nullptr
     */
    SDL_Texture* load_texture(SDL_Renderer* renderer, const std::string& image_path);

    /**
     * @brief 释放纹理资源
     */
    void release_texture();

    /**
     * @brief 计算不同模式下的渲染矩形
     * @param viewport_size 视口大小
     * @param texture_size 纹理大小
     * @return 渲染矩形
     */
    ImVec4 calculate_render_rect(
        const ImVec2& viewport_size,
        const ImVec2& texture_size
    ) const;

private:
    bool m_enabled = false;                    ///< 是否启用背景
    BackgroundMode m_mode = BackgroundMode::Cover;  ///< 背景模式
    float m_opacity = 0.3f;                    ///< 透明度
    ImVec4 m_tint_color = ImVec4(1, 1, 1, 1); ///< 混合颜色

    SDL_Texture* m_texture = nullptr;          ///< 背景纹理
    float m_texture_width = 0.0f;              ///< 纹理宽度（SDL3 使用 float）
    float m_texture_height = 0.0f;             ///< 纹理高度（SDL3 使用 float）

    SDL_Renderer* m_renderer = nullptr;        ///< SDL 渲染器（需要在渲染时设置）
};

} // namespace DearTs::Core::UI
