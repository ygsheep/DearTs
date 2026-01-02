/**
 * @file sdl_texture_uploader.cpp
 * @brief SDL 纹理上传器实现
 */

#include "sdl_texture_uploader.hpp"
// GLEW 必须在任何其他 OpenGL 头之前包含
#include <GL/glew.h>
#include <SDL3/SDL_opengl.h>

// 取消 Windows 宏定义，避免与 logger 冲突
#ifdef ERROR
#undef ERROR
#endif

#include "liblogger/logger.h"
#include <chrono>

namespace DearTs::Plugins::Live2D {

// ============================================================================
// 构造函数和析构函数
// ============================================================================

SDLTextureUploader::SDLTextureUploader() = default;

SDLTextureUploader::~SDLTextureUploader() {
    shutdown();
}

// ============================================================================
// 初始化和清理
// ============================================================================

Result<void, std::string> SDLTextureUploader::initialize(const TextureUploadConfig& config) {
    if (m_initialized) {
        return Result<void, std::string>::err("Uploader already initialized");
    }

    m_config = config;
    LOG_INFO("SDLTextureUploader: Initializing (PBO: {})", config.use_pbo);

    // 如果启用 PBO，初始化 PBO
    if (config.use_pbo) {
        auto result = initialize_pbo();
        if (result.isErr()) {
            LOG_WARN("SDLTextureUploader: Failed to initialize PBO, falling back to synchronous upload");
            m_config.use_pbo = false;
        }
    }

    m_initialized = true;
    LOG_INFO("SDLTextureUploader: Initialized successfully");
    return Result<void, std::string>::ok();
}

void SDLTextureUploader::shutdown() {
    if (!m_initialized) {
        return;
    }

    LOG_INFO("SDLTextureUploader: Shutting down");

    // 清理 PBO
    if (m_config.use_pbo) {
        cleanup_pbo();
    }

    m_initialized = false;
    LOG_INFO("SDLTextureUploader: Shutdown complete");
}

// ============================================================================
// 纹理上传
// ============================================================================

SDL_Texture* SDLTextureUploader::upload_texture(
    GLuint gl_texture_id,
    int width,
    int height,
    SDL_Renderer* renderer
) {
    if (!m_initialized || !renderer) {
        LOG_ERROR("SDLTextureUploader: Invalid state or null renderer");
        return nullptr;
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    // 从 OpenGL 读取纹理数据
    std::vector<uint8_t> texture_data;
    auto result = read_gl_texture(gl_texture_id, width, height, texture_data);
    if (result.isErr()) {
        LOG_ERROR("SDLTextureUploader: Failed to read GL texture: {}", result.error());
        return nullptr;
    }

    // 创建 SDL 纹理
    SDL_Texture* sdl_texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_STATIC,
        width,
        height
    );

    if (!sdl_texture) {
        LOG_ERROR("SDLTextureUploader: Failed to create SDL texture: {}", SDL_GetError());
        return nullptr;
    }

    // 更新纹理数据
    if (!SDL_UpdateTexture(
        sdl_texture,
        nullptr,
        texture_data.data(),
        width * 4  // pitch = width * 4 bytes per pixel (RGBA)
    )) {
        LOG_ERROR("SDLTextureUploader: Failed to update SDL texture: {}", SDL_GetError());
        SDL_DestroyTexture(sdl_texture);
        return nullptr;
    }

    // 更新统计信息
    auto end_time = std::chrono::high_resolution_clock::now();
    float elapsed_ms = std::chrono::duration<float, std::milli>(end_time - start_time).count();

    m_stats.upload_count++;
    m_stats.total_bytes += texture_data.size();
    m_stats.total_time_ms += elapsed_ms;
    m_stats.avg_time_ms = m_stats.total_time_ms / m_stats.upload_count;

    return sdl_texture;
}

Result<void, std::string> SDLTextureUploader::update_texture(
    GLuint gl_texture_id,
    SDL_Texture* sdl_texture,
    int width,
    int height
) {
    if (!m_initialized || !sdl_texture) {
        return Result<void, std::string>::err("Invalid state or null texture");
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    // 从 OpenGL 读取纹理数据
    std::vector<uint8_t> texture_data;
    auto result = read_gl_texture(gl_texture_id, width, height, texture_data);
    if (result.isErr()) {
        return result;
    }

    // 更新纹理数据
    if (!SDL_UpdateTexture(
        sdl_texture,
        nullptr,
        texture_data.data(),
        width * 4
    )) {
        std::string error_msg = "Failed to update SDL texture: ";
        error_msg += SDL_GetError();
        return Result<void, std::string>::err(error_msg);
    }

    // 更新统计信息
    auto end_time = std::chrono::high_resolution_clock::now();
    float elapsed_ms = std::chrono::duration<float, std::milli>(end_time - start_time).count();

    m_stats.upload_count++;
    m_stats.total_bytes += texture_data.size();
    m_stats.total_time_ms += elapsed_ms;
    m_stats.avg_time_ms = m_stats.total_time_ms / m_stats.upload_count;

    return Result<void, std::string>::ok();
}

// ============================================================================
// OpenGL 纹理读取
// ============================================================================

Result<void, std::string> SDLTextureUploader::read_gl_texture(
    GLuint gl_texture_id,
    int width,
    int height,
    std::vector<uint8_t>& output_data
) {
    // 保存当前绑定状态
    GLint old_texture_binding;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &old_texture_binding);

    // 绑定目标纹理
    glBindTexture(GL_TEXTURE_2D, gl_texture_id);

    // 分配缓冲区
    output_data.resize(width * height * 4);  // RGBA = 4 bytes per pixel

    // 读取纹理数据
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, output_data.data());

    // 检查 OpenGL 错误
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        glBindTexture(GL_TEXTURE_2D, old_texture_binding);
        std::string error_msg = "OpenGL error after glGetTexImage: ";
        error_msg += std::to_string(error);
        return Result<void, std::string>::err(error_msg);
    }

    // 恢复绑定状态
    glBindTexture(GL_TEXTURE_2D, old_texture_binding);

    return Result<void, std::string>::ok();
}

// ============================================================================
// PBO 支持（Phase 5 优化）
// ============================================================================

Result<void, std::string> SDLTextureUploader::initialize_pbo() {
    LOG_INFO("SDLTextureUploader: Initializing PBO (buffer count: {})",
             m_config.pbo_buffer_count);

    // 生成 PBO
    glGenBuffers(m_config.pbo_buffer_count, m_pbo_ids);

    // 分配最大可能的纹理大小（4K 分辨率）
    const size_t buffer_size = 4096 * 4096 * 4;  // RGBA

    for (int i = 0; i < m_config.pbo_buffer_count; ++i) {
        glBindBuffer(GL_PIXEL_PACK_BUFFER, m_pbo_ids[i]);
        glBufferData(GL_PIXEL_PACK_BUFFER, buffer_size, nullptr, GL_DYNAMIC_READ);
    }

    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        cleanup_pbo();
        return Result<void, std::string>::err("Failed to initialize PBO buffers");
    }

    LOG_INFO("SDLTextureUploader: PBO initialized successfully");
    return Result<void, std::string>::ok();
}

void SDLTextureUploader::cleanup_pbo() {
    if (m_pbo_ids[0] != 0) {
        glDeleteBuffers(m_config.pbo_buffer_count, m_pbo_ids);
        for (int i = 0; i < m_config.pbo_buffer_count; ++i) {
            m_pbo_ids[i] = 0;
        }
        LOG_INFO("SDLTextureUploader: PBO cleaned up");
    }
}

Result<void, std::string> SDLTextureUploader::read_gl_texture_pbo(
    GLuint gl_texture_id,
    int width,
    int height,
    std::vector<uint8_t>& output_data
) {
    // TODO: Phase 5 实现 PBO 异步读取
    // 这是优化项，暂时使用同步读取
    return read_gl_texture(gl_texture_id, width, height, output_data);
}

// ============================================================================
// 统计信息
// ============================================================================

void SDLTextureUploader::reset_stats() {
    m_stats = TextureUploadStats{};
}

} // namespace DearTs::Plugins::Live2D
