/**
 * @file background_renderer.cpp
 * @brief 背景图渲染器实现
 */

#include "background_renderer.h"
#include "liblogger/logger.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <filesystem>

namespace DearTs::Core::UI {

BackgroundRenderer::BackgroundRenderer() {
    LOG_INFO("BackgroundRenderer initialized");
}

BackgroundRenderer::~BackgroundRenderer() {
    clear();
}

bool BackgroundRenderer::load_background(const std::string& image_path) {
    // 释放旧纹理
    release_texture();

    // 构建完整路径
    std::string full_path;
    if (std::filesystem::path(image_path).is_absolute()) {
        full_path = image_path;
    } else {
        // 默认从 resources/images 目录加载
        full_path = "resources/images/" + image_path;
    }

    // 加载新纹理（需要 m_renderer 已设置）
    if (!m_renderer) {
        LOG_ERROR("Renderer not set. Call set_renderer() first.");
        m_enabled = false;
        return false;
    }

    m_texture = load_texture(m_renderer, full_path);
    if (!m_texture) {
        LOG_ERROR("Failed to load background image: {}", full_path);
        m_enabled = false;
        return false;
    }

    // 查询纹理大小（SDL3 API）
    if (!SDL_GetTextureSize(m_texture, &m_texture_width, &m_texture_height)) {
        LOG_ERROR("Failed to query texture size: {}", SDL_GetError());
        release_texture();
        m_enabled = false;
        return false;
    }

    LOG_INFO("Background loaded: {} ({}x{})", full_path, m_texture_width, m_texture_height);
    m_enabled = true;
    return true;
}

void BackgroundRenderer::render(SDL_Renderer* renderer, const ImVec2& viewport_size) {
    if (!m_enabled || !m_texture || !renderer) {
        return;
    }

    // 计算渲染区域
    ImVec4 render_rect = calculate_render_rect(
        viewport_size,
        ImVec2(m_texture_width, m_texture_height)
    );

    SDL_FRect dst_rect;
    dst_rect.x = render_rect.x;
    dst_rect.y = render_rect.y;
    dst_rect.w = render_rect.z;
    dst_rect.h = render_rect.w;

    // 设置透明度（通过设置纹理 alpha 模式）
    SDL_SetTextureAlphaMod(m_texture, static_cast<uint8_t>(m_opacity * 255.0f));

    // 设置混合颜色
    SDL_SetTextureColorMod(
        m_texture,
        static_cast<uint8_t>(m_tint_color.x * 255.0f),
        static_cast<uint8_t>(m_tint_color.y * 255.0f),
        static_cast<uint8_t>(m_tint_color.z * 255.0f)
    );

    // 渲染背景
    if (m_mode == BackgroundMode::Tile) {
        // 平铺模式：需要渲染多个纹理
        for (float y = 0; y < viewport_size.y; y += m_texture_height) {
            for (float x = 0; x < viewport_size.x; x += m_texture_width) {
                SDL_FRect tile_rect;
                tile_rect.x = x;
                tile_rect.y = y;
                tile_rect.w = m_texture_width;
                tile_rect.h = m_texture_height;
                if (!SDL_RenderTexture(renderer, m_texture, nullptr, &tile_rect)) {
                    // 渲染失败，记录错误
                    const char* error = SDL_GetError();
                    if (error && *error) {
                        LOG_ERROR("SDL_RenderTexture (tile) failed: {}", error);
                    }
                }
            }
        }
    } else {
        // 其他模式：单次渲染
        if (!SDL_RenderTexture(renderer, m_texture, nullptr, &dst_rect)) {
            // 渲染失败，记录错误
            const char* error = SDL_GetError();
            if (error && *error) {
                LOG_ERROR("SDL_RenderTexture failed: {}", error);
            }
        }
    }
}

void BackgroundRenderer::clear() {
    release_texture();
    m_enabled = false;
    LOG_INFO("Background cleared");
}

SDL_Texture* BackgroundRenderer::load_texture(SDL_Renderer* renderer, const std::string& image_path) {
    // 使用 SDL_image 加载图片
    SDL_Texture* texture = IMG_LoadTexture(renderer, image_path.c_str());
    if (!texture) {
        // SDL3_image uses SDL's error system
        const char* error = SDL_GetError();
        LOG_ERROR("IMG_LoadTexture failed: {}", error ? error : "Unknown error");
        return nullptr;
    }

    return texture;
}

void BackgroundRenderer::release_texture() {
    if (m_texture) {
        SDL_DestroyTexture(m_texture);
        m_texture = nullptr;
        m_texture_width = 0;
        m_texture_height = 0;
    }
}

ImVec4 BackgroundRenderer::calculate_render_rect(
    const ImVec2& viewport_size,
    const ImVec2& texture_size
) const {
    ImVec4 result;
    result.x = 0;
    result.y = 0;
    result.z = viewport_size.x;
    result.w = viewport_size.y;

    switch (m_mode) {
        case BackgroundMode::Stretch:
            // 拉伸：填充整个视口
            result.x = 0;
            result.y = 0;
            result.z = viewport_size.x;
            result.w = viewport_size.y;
            break;

        case BackgroundMode::Center:
            // 居中：原始大小居中显示
            result.x = (viewport_size.x - texture_size.x) / 2.0f;
            result.y = (viewport_size.y - texture_size.y) / 2.0f;
            result.z = texture_size.x;
            result.w = texture_size.y;
            break;

        case BackgroundMode::Cover: {
            // 覆盖：保持比例，覆盖整个视口（可能裁剪）
            float viewport_aspect = viewport_size.x / viewport_size.y;
            float texture_aspect = texture_size.x / texture_size.y;

            if (viewport_aspect > texture_aspect) {
                // 视口更宽：以宽度为准
                result.z = viewport_size.x;
                result.w = viewport_size.x / texture_aspect;
                result.x = 0;
                result.y = (viewport_size.y - result.w) / 2.0f;
            } else {
                // 视口更高：以高度为准
                result.w = viewport_size.y;
                result.z = viewport_size.y * texture_aspect;
                result.x = (viewport_size.x - result.z) / 2.0f;
                result.y = 0;
            }
            break;
        }

        case BackgroundMode::Contain: {
            // 包含：保持比例，完整显示在视口内
            float viewport_aspect = viewport_size.x / viewport_size.y;
            float texture_aspect = texture_size.x / texture_size.y;

            if (viewport_aspect > texture_aspect) {
                // 视口更宽：以高度为准
                result.w = viewport_size.y;
                result.z = viewport_size.y * texture_aspect;
                result.x = (viewport_size.x - result.z) / 2.0f;
                result.y = 0;
            } else {
                // 视口更高：以宽度为准
                result.z = viewport_size.x;
                result.w = viewport_size.x / texture_aspect;
                result.x = 0;
                result.y = (viewport_size.y - result.w) / 2.0f;
            }
            break;
        }

        case BackgroundMode::Tile:
            // 平铺模式在 render() 中特殊处理
            break;
    }

    return result;
}

} // namespace DearTs::Core::UI
