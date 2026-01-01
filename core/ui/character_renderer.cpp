/**
 * @file character_renderer.cpp
 * @brief 人物角色渲染器实现
 */

#include "character_renderer.h"
#include "liblogger/logger.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <filesystem>
#include <algorithm>

namespace DearTs::Core::UI {

CharacterRenderer::CharacterRenderer() {
    LOG_INFO("CharacterRenderer initialized");
}

CharacterRenderer::~CharacterRenderer() {
    clear();
}

bool CharacterRenderer::load_character_frames(const std::vector<std::string>& image_paths) {
    // 清空旧帧
    release_frames();

    // 加载新帧
    for (const auto& path : image_paths) {
        CharacterFrame frame;
        frame.image_path = path;
        frame.texture = load_texture(path);

        if (!frame.texture) {
            LOG_ERROR("Failed to load character frame: {}", path);
            release_frames();
            m_enabled = false;
            return false;
        }

        // 查询纹理大小
        if (SDL_QueryTexture(frame.texture, nullptr, nullptr, &frame.width, &frame.height) != 0) {
            LOG_ERROR("Failed to query texture size: {}", SDL_GetError());
            release_frames();
            m_enabled = false;
            return false;
        }

        m_frames.push_back(frame);
        LOG_INFO("Loaded character frame: {} ({}x{})", path, frame.width, frame.height);
    }

    if (m_frames.empty()) {
        LOG_ERROR("No character frames loaded");
        m_enabled = false;
        return false;
    }

    LOG_INFO("Character loaded: {} frames", m_frames.size());
    m_enabled = true;
    m_current_frame = 0;
    return true;
}

bool CharacterRenderer::load_character_from_directory(
    const std::string& directory,
    const std::string& pattern
) {
    std::vector<std::string> image_paths;

    // 构建完整目录路径
    std::string full_dir = directory;
    if (!std::filesystem::path(directory).is_absolute()) {
        full_dir = "resources/images/" + directory;
    }

    // 如果目录不存在，尝试从 resources/images 加载
    if (!std::filesystem::exists(full_dir)) {
        LOG_WARN("Directory not found: {}", full_dir);
        return false;
    }

    // 遍历目录，查找图片文件
    for (const auto& entry : std::filesystem::directory_iterator(full_dir)) {
        if (entry.is_regular_file()) {
            std::string path = entry.path().string();
            std::string ext = entry.path().extension().string();

            // 检查文件扩展名
            if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp") {
                // 如果指定了模式，进行匹配
                if (pattern.empty() || path.find(pattern) != std::string::npos) {
                    image_paths.push_back(path);
                }
            }
        }
    }

    // 按文件名排序（确保 0-菲比.png 在 1-菲比.png 前面）
    std::sort(image_paths.begin(), image_paths.end());

    if (image_paths.empty()) {
        LOG_ERROR("No character images found in: {}", full_dir);
        return false;
    }

    return load_character_frames(image_paths);
}

void CharacterRenderer::update(double delta_time) {
    if (!m_enabled || m_frames.empty()) {
        return;
    }

    update_animation(delta_time);
}

void CharacterRenderer::render() {
    if (!m_enabled || m_frames.empty()) {
        return;
    }

    if (m_current_frame >= m_frames.size()) {
        m_current_frame = 0;
    }

    const auto& frame = m_frames[m_current_frame];
    if (!frame.texture) {
        return;
    }

    // 获取当前视口大小
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 viewport_size = viewport->Size;

    // 计算渲染位置
    ImVec2 texture_size(
        static_cast<float>(frame.width) * m_scale,
        static_cast<float>(frame.height) * m_scale
    );
    ImVec2 pos = calculate_render_position(viewport_size, texture_size);

    // 获取 SDL renderer
    SDL_Window* window = SDL_GetRenderWindow(SDL_GetVideoDevice(), 0);
    if (!window) {
        LOG_WARN("No SDL window found for character rendering");
        return;
    }

    SDL_Renderer* renderer = SDL_GetRenderer(window);
    if (!renderer) {
        LOG_WARN("No SDL renderer found for character rendering");
        return;
    }

    // 设置透明度和混合颜色
    SDL_SetTextureAlphaMod(frame.texture, static_cast<uint8_t>(m_opacity * 255.0f));
    SDL_SetTextureColorMod(frame.texture, 255, 255, 255);

    // 渲染角色
    SDL_FRect dst_rect;
    dst_rect.x = pos.x;
    dst_rect.y = pos.y;
    dst_rect.w = texture_size.x;
    dst_rect.h = texture_size.y;

    SDL_RenderTexture(renderer, frame.texture, nullptr, &dst_rect);
}

void CharacterRenderer::clear() {
    release_frames();
    m_enabled = false;
    m_current_frame = 0;
    LOG_INFO("Character cleared");
}

SDL_Texture* CharacterRenderer::load_texture(const std::string& image_path) {
    // 构建完整路径
    std::string full_path;
    if (std::filesystem::path(image_path).is_absolute()) {
        full_path = image_path;
    } else {
        full_path = "resources/images/" + image_path;
    }

    // 获取当前窗口
    SDL_Window* window = SDL_GetRenderWindow(SDL_GetVideoDevice(), 0);
    if (!window) {
        LOG_ERROR("No SDL window found");
        return nullptr;
    }

    // 获取 renderer（SDL3 使用 SDL_GetRenderer）
    SDL_Renderer* renderer = SDL_GetRenderer(window);
    if (!renderer) {
        LOG_ERROR("No SDL renderer found");
        return nullptr;
    }

    // 使用 SDL_image 加载图片
    SDL_Texture* texture = IMG_LoadTexture(renderer, full_path.c_str());
    if (!texture) {
        const char* error = IMG_GetError();
        LOG_ERROR("IMG_LoadTexture failed: {}", error ? error : "Unknown error");
        return nullptr;
    }

    return texture;
}

void CharacterRenderer::release_frames() {
    for (auto& frame : m_frames) {
        if (frame.texture) {
            SDL_DestroyTexture(frame.texture);
            frame.texture = nullptr;
        }
    }
    m_frames.clear();
}

ImVec2 CharacterRenderer::calculate_render_position(
    const ImVec2& viewport_size,
    const ImVec2& texture_size
) const {
    ImVec2 pos;
    pos.x = m_margin;
    pos.y = m_margin;

    switch (m_position) {
        case CharacterPosition::BottomRight:
            pos.x = viewport_size.x - texture_size.x - m_margin;
            pos.y = viewport_size.y - texture_size.y - m_margin;
            break;

        case CharacterPosition::BottomLeft:
            pos.x = m_margin;
            pos.y = viewport_size.y - texture_size.y - m_margin;
            break;

        case CharacterPosition::TopRight:
            pos.x = viewport_size.x - texture_size.x - m_margin;
            pos.y = m_margin;
            break;

        case CharacterPosition::TopLeft:
            pos.x = m_margin;
            pos.y = m_margin;
            break;

        case CharacterPosition::Center:
            pos.x = (viewport_size.x - texture_size.x) / 2.0f;
            pos.y = (viewport_size.y - texture_size.y) / 2.0f;
            break;
    }

    return pos;
}

void CharacterRenderer::update_animation(double delta_time) {
    if (m_frames.size() <= 1) {
        return;
    }

    m_frame_timer += static_cast<float>(delta_time);

    if (m_frame_timer >= m_frame_interval) {
        m_frame_timer = 0.0f;

        switch (m_animation_mode) {
            case AnimationMode::None:
                // 不切换帧
                break;

            case AnimationMode::FrameLoop:
                // 循环播放
                m_current_frame = (m_current_frame + 1) % m_frames.size();
                break;

            case AnimationMode::PingPong:
                // 往复播放
                m_current_frame += m_ping_pong_direction;
                if (m_current_frame >= m_frames.size() - 1) {
                    m_ping_pong_direction = -1;
                } else if (m_current_frame <= 0) {
                    m_ping_pong_direction = 1;
                }
                break;

            case AnimationMode::Random:
                // 随机切换（但不重复当前帧）
                size_t new_frame = m_current_frame;
                while (new_frame == m_current_frame && m_frames.size() > 1) {
                    new_frame = rand() % m_frames.size();
                }
                m_current_frame = new_frame;
                break;
        }
    }
}

} // namespace DearTs::Core::UI
