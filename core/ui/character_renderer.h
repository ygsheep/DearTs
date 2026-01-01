/**
 * @file character_renderer.h
 * @brief 人物角色渲染器
 * @details 支持人物角色显示、动画帧切换、位置和大小调整
 * @author DearTs Team
 * @date 2025
 * @version 1.0.0
 */

#pragma once

#include <imgui.h>
#include <string>
#include <vector>
#include <memory>
#include <chrono>

namespace DearTs::Core::UI {

/**
 * @brief 角色位置
 */
enum class CharacterPosition {
    BottomRight,    ///< 右下角
    BottomLeft,     ///< 左下角
    TopRight,       ///< 右上角
    TopLeft,        ///< 左上角
    Center          ///< 居中
};

/**
 * @brief 动画模式
 */
enum class AnimationMode {
    None,           ///< 无动画
    FrameLoop,      ///< 帧循环
    PingPong,       ///< 往复播放
    Random          ///< 随机切换
};

/**
 * @brief 角色帧
 */
struct CharacterFrame {
    std::string image_path;    ///< 图片路径
    SDL_Texture* texture = nullptr;  ///< 纹理
    int width = 0;              ///< 宽度
    int height = 0;             ///< 高度
    float duration = 1.0f;      ///< 显示时长（秒）
};

/**
 * @brief 人物角色渲染器
 *
 * 提供人物角色渲染功能：
 * - 支持多帧动画
 * - 支持多种动画模式（循环、往复、随机）
 * - 可调整角色位置和大小
 * - 支持透明度和混合模式
 */
class CharacterRenderer {
public:
    /**
     * @brief 获取单例实例
     */
    static CharacterRenderer& instance() {
        static CharacterRenderer inst;
        return inst;
    }

    /**
     * @brief 加载角色帧序列
     * @param image_paths 图片路径列表（按顺序）
     * @return 成功返回 true
     */
    bool load_character_frames(const std::vector<std::string>& image_paths);

    /**
     * @brief 从目录加载所有角色帧
     * @param directory 目录路径
     * @param pattern 文件名模式（如 "菲比.png" 或 "0-*.png"）
     * @return 成功返回 true
     */
    bool load_character_from_directory(
        const std::string& directory,
        const std::string& pattern = ""
    );

    /**
     * @brief 更新动画
     * @param delta_time 时间增量（秒）
     */
    void update(double delta_time);

    /**
     * @brief 渲染角色
     */
    void render();

    /**
     * @brief 设置角色位置
     * @param position 位置
     */
    void set_position(CharacterPosition position) { m_position = position; }

    /**
     * @brief 获取角色位置
     */
    [[nodiscard]] CharacterPosition get_position() const { return m_position; }

    /**
     * @brief 设置缩放比例
     * @param scale 缩放比例（1.0 = 原始大小）
     */
    void set_scale(float scale) { m_scale = scale; }

    /**
     * @brief 获取缩放比例
     */
    [[nodiscard]] float get_scale() const { return m_scale; }

    /**
     * @brief 设置透明度 (0.0 - 1.0)
     * @param opacity 透明度
     */
    void set_opacity(float opacity) { m_opacity = opacity; }

    /**
     * @brief 获取透明度
     */
    [[nodiscard]] float get_opacity() const { return m_opacity; }

    /**
     * @brief 设置边距
     * @param margin 边距（像素）
     */
    void set_margin(float margin) { m_margin = margin; }

    /**
     * @brief 获取边距
     */
    [[nodiscard]] float get_margin() const { return m_margin; }

    /**
     * @brief 设置动画模式
     * @param mode 动画模式
     */
    void set_animation_mode(AnimationMode mode) { m_animation_mode = mode; }

    /**
     * @brief 获取动画模式
     */
    [[nodiscard]] AnimationMode get_animation_mode() const { return m_animation_mode; }

    /**
     * @brief 设置帧切换间隔（秒）
     * @param interval 间隔时间
     */
    void set_frame_interval(float interval) { m_frame_interval = interval; }

    /**
     * @brief 获取帧切换间隔
     */
    [[nodiscard]] float get_frame_interval() const { return m_frame_interval; }

    /**
     * @brief 启用/禁用角色渲染
     * @param enabled 是否启用
     */
    void set_enabled(bool enabled) { m_enabled = enabled; }

    /**
     * @brief 获取是否启用
     */
    [[nodiscard]] bool is_enabled() const { return m_enabled; }

    /**
     * @brief 清空角色
     */
    void clear();

    /**
     * @brief 获取当前帧索引
     */
    [[nodiscard]] size_t get_current_frame() const { return m_current_frame; }

    /**
     * @brief 获取帧总数
     */
    [[nodiscard]] size_t get_frame_count() const { return m_frames.size(); }

private:
    CharacterRenderer();
    ~CharacterRenderer();

    // 禁止拷贝
    CharacterRenderer(const CharacterRenderer&) = delete;
    CharacterRenderer& operator=(const CharacterRenderer&) = delete;

    /**
     * @brief 从文件加载纹理
     * @param image_path 图片路径
     * @return 纹理指针，失败返回 nullptr
     */
    SDL_Texture* load_texture(const std::string& image_path);

    /**
     * @brief 释放所有帧的纹理资源
     */
    void release_frames();

    /**
     * @brief 计算角色渲染位置
     * @param viewport_size 视口大小
     * @param texture_size 纹理大小
     * @return 渲染位置
     */
    ImVec2 calculate_render_position(
        const ImVec2& viewport_size,
        const ImVec2& texture_size
    ) const;

    /**
     * @brief 更新动画帧
     */
    void update_animation(double delta_time);

private:
    bool m_enabled = false;                       ///< 是否启用角色
    CharacterPosition m_position = CharacterPosition::BottomRight;  ///< 角色位置
    float m_scale = 0.5f;                         ///< 缩放比例
    float m_opacity = 1.0f;                       ///< 透明度
    float m_margin = 20.0f;                       ///< 边距

    AnimationMode m_animation_mode = AnimationMode::FrameLoop;  ///< 动画模式
    float m_frame_interval = 1.0f;               ///< 帧切换间隔（秒）
    float m_frame_timer = 0.0f;                  ///< 帧计时器
    size_t m_current_frame = 0;                  ///< 当前帧索引
    bool m_ping_pong_direction = 1;              ///< 往复播放方向（1=正向，-1=反向）

    std::vector<CharacterFrame> m_frames;        ///< 角色帧列表
};

} // namespace DearTs::Core::UI
