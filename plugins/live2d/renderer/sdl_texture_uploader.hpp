/**
 * @file sdl_texture_uploader.hpp
 * @brief SDL 纹理上传器
 * @details 将 OpenGL 纹理转换为 SDL3 纹理
 * @author DearTs Team
 * @date 2025
 * @version 1.0.0
 */

#pragma once

#include "core/result.h"
#include <SDL3/SDL.h>
#include <cstdint>
#include <memory>
#include <vector>

namespace DearTs::Plugins::Live2D {

/**
 * @brief 纹理上传配置
 */
struct TextureUploadConfig {
    bool use_pbo = false;           ///< 是否使用 PBO（像素缓冲对象）异步传输
    bool generate_mipmaps = false;  ///< 是否生成 mipmaps
    int pbo_buffer_count = 2;       ///< PBO 缓冲区数量（双缓冲或三缓冲）
};

/**
 * @brief 纹理上传统计
 */
struct TextureUploadStats {
    uint32_t upload_count = 0;      ///< 上传次数
    uint32_t total_bytes = 0;       ///< 总字节数
    float total_time_ms = 0.0f;     ///< 总时间（毫秒）
    float avg_time_ms = 0.0f;       ///< 平均时间（毫秒）
};

/**
 * @brief SDL 纹理上传器
 *
 * @details
 * 将 OpenGL 纹理转换为 SDL3 纹理，以便在 SDL3 渲染器中使用。
 *
 * 关键功能：
 * - 从 OpenGL 读取纹理数据
 * - 上传到 SDL 纹理
 * - 支持 PBO 异步传输（Phase 5 优化）
 * - 性能监控
 *
 * @note
 * - 性能关键路径，后续会添加 PBO 优化
 * - 纹理格式必须兼容（GL_RGBA -> SDL_PIXELFORMAT_RGBA32）
 * - 线程不安全，必须在主线程调用
 */
class SDLTextureUploader {
public:
    /**
     * @brief 构造函数
     */
    SDLTextureUploader();

    /**
     * @brief 析构函数
     */
    ~SDLTextureUploader();

    // 禁止拷贝和移动
    SDLTextureUploader(const SDLTextureUploader&) = delete;
    SDLTextureUploader& operator=(const SDLTextureUploader&) = delete;
    SDLTextureUploader(SDLTextureUploader&&) = delete;
    SDLTextureUploader& operator=(SDLTextureUploader&&) = delete;

    /**
     * @brief 初始化上传器
     * @param config 上传器配置
     * @return 成功返回 void，失败返回错误信息
     */
    Result<void, std::string> initialize(const TextureUploadConfig& config);

    /**
     * @brief 关闭上传器并释放资源
     */
    void shutdown();

    /**
     * @brief 从 OpenGL 纹理上传到 SDL 纹理
     * @param gl_texture_id OpenGL 纹理 ID
     * @param width 纹理宽度
     * @param height 纹理高度
     * @param renderer SDL 渲染器
     * @return SDL 纹理指针，失败返回 nullptr
     */
    [[nodiscard]] SDL_Texture* upload_texture(
        GLuint gl_texture_id,
        int width,
        int height,
        SDL_Renderer* renderer
    );

    /**
     * @brief 更新已有的 SDL 纹理
     * @param gl_texture_id OpenGL 纹理 ID
     * @param sdl_texture SDL 纹理
     * @param width 纹理宽度
     * @param height 纹理高度
     * @return 成功返回 void，失败返回错误信息
     */
    Result<void, std::string> update_texture(
        GLuint gl_texture_id,
        SDL_Texture* sdl_texture,
        int width,
        int height
    );

    /**
     * @brief 获取统计信息
     */
    [[nodiscard]] const TextureUploadStats& get_stats() const { return m_stats; }

    /**
     * @brief 重置统计信息
     */
    void reset_stats();

    /**
     * @brief 检查上传器是否可用
     */
    [[nodiscard]] bool is_valid() const { return m_initialized; }

private:
    /**
     * @brief 从 OpenGL 读取纹理数据
     * @param gl_texture_id OpenGL 纹理 ID
     * @param width 纹理宽度
     * @param height 纹理高度
     * @param output_data 输出数据缓冲区
     * @return 成功返回 void，失败返回错误信息
     */
    Result<void, std::string> read_gl_texture(
        GLuint gl_texture_id,
        int width,
        int height,
        std::vector<uint8_t>& output_data
    );

    /**
     * @brief 初始化 PBO（Phase 5 优化）
     */
    Result<void, std::string> initialize_pbo();

    /**
     * @brief 清理 PBO
     */
    void cleanup_pbo();

    /**
     * @brief 使用 PBO 异步读取纹理（Phase 5 优化）
     */
    Result<void, std::string> read_gl_texture_pbo(
        GLuint gl_texture_id,
        int width,
        int height,
        std::vector<uint8_t>& output_data
    );

private:
    bool m_initialized = false;
    TextureUploadConfig m_config;
    TextureUploadStats m_stats;

    // PBO 相关（Phase 5 优化）
    GLuint m_pbo_ids[3] = {0, 0, 0};  ///< 最多支持三缓冲
    uint32_t m_current_pbo_index = 0;
};

} // namespace DearTs::Plugins::Live2D
