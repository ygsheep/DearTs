/**
 * @file scale_manager.cpp
 * @brief 高分辨率自动缩放管理器实现
 */

#include "scale_manager.h"
#include "core/config/config_manager.h"
#include "liblogger/logger.h"
#include <imgui.h>
#include <algorithm>
#include <cmath>

namespace DearTs::Core::UI {

ScaleManager& ScaleManager::instance() {
    static ScaleManager instance;
    return instance;
}

Result<void, std::string> ScaleManager::initialize(SDL_Window* window) {
    if (m_initialized) {
        return Result<void, std::string>::err("ScaleManager already initialized");
    }

    if (!window) {
        return Result<void, std::string>::err("Invalid window pointer");
    }

    m_window = window;

    // 检测显示器分辨率
    SDL_DisplayID display = SDL_GetDisplayForWindow(window);
    if (display == 0) {
        LOG_WARN("Failed to get display for window, using defaults");
        m_display_info.width = 1920;
        m_display_info.height = 1080;
        m_display_info.dpi = 96.0f;
        m_display_info.scale = 1.0f;
        m_display_info.is_high_dpi = false;
    } else {
        // 获取显示器模式（分辨率）
        const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(display);
        if (mode) {
            m_display_info.width = mode->w;
            m_display_info.height = mode->h;
            LOG_INFO("Display resolution: {}x{}", mode->w, mode->h);

            // 计算推荐缩放比例（基于分辨率）
            m_display_info.scale = calculate_recommended_scale(mode->w, mode->h);

            // 估算 DPI（基于分辨率）
            // 1920x1080 @ ~96 DPI 是基准
            float resolution_ratio = std::sqrt((float)(mode->w * mode->h) / (1920.0f * 1080.0f));
            m_display_info.dpi = 96.0f * resolution_ratio;
        } else {
            LOG_WARN("Failed to get display mode, using defaults");
            m_display_info.width = 1920;
            m_display_info.height = 1080;
            m_display_info.scale = 1.0f;
            m_display_info.dpi = 96.0f;
        }

        // 判断是否为高 DPI 显示器（推荐缩放 > 1.0）
        m_display_info.is_high_dpi = (m_display_info.scale > 1.0f);

        LOG_INFO("Display info: {}x{} @ {:.1f} DPI (estimated), recommended scale: {:.2f}",
                 m_display_info.width, m_display_info.height, m_display_info.dpi, m_display_info.scale);
    }

    // 从配置加载用户保存的缩放设置
    load_from_config();

    // 如果配置中没有保存的值，使用推荐的缩放（自动模式）
    if (m_mode == ScaleMode::Auto && m_scale == 1.0f && m_display_info.scale > 1.0f) {
        m_scale = m_display_info.scale;
        LOG_INFO("Auto mode: using recommended scale {:.2f}", m_scale);
    }

    m_initialized = true;

    LOG_INFO("ScaleManager initialized: scale={:.2f}, mode={}",
              m_scale,
              m_mode == ScaleMode::Auto ? "Auto" : m_mode == ScaleMode::Manual ? "Manual" : "System");

    return Result<void, std::string>::ok();
}

float ScaleManager::get_scale() const {
    return m_scale;
}

Result<void, std::string> ScaleManager::set_scale(float scale, ScaleMode mode) {
    if (!m_initialized) {
        return Result<void, std::string>::err("ScaleManager not initialized");
    }

    // 限制缩放范围
    scale = std::clamp(scale, 0.5f, 2.0f);

    if (std::abs(scale - m_scale) < 0.01f) {
        // 变化太小，忽略
        return Result<void, std::string>::ok();
    }

    float old_scale = m_scale;
    apply_scale(scale, mode);

    LOG_INFO("Scale changed: {:.2f} -> {:.2f} (mode: {})",
              old_scale, m_scale,
              m_mode == ScaleMode::Auto ? "Auto" : m_mode == ScaleMode::Manual ? "Manual" : "System");

    // 保存到配置
    save_to_config();

    // 通知缩放变更
    notify_scale_changed(old_scale, m_scale);

    return Result<void, std::string>::ok();
}

void ScaleManager::enable_auto_scale() {
    if (!m_initialized) {
        LOG_WARN("ScaleManager not initialized, cannot enable auto scale");
        return;
    }

    if (m_mode == ScaleMode::Auto) {
        return; // 已经是自动模式
    }

    float old_scale = m_scale;
    m_mode = ScaleMode::Auto;
    m_scale = m_display_info.scale;

    LOG_INFO("Auto scale enabled: {:.2f} -> {:.2f}", old_scale, m_scale);

    save_to_config();
    notify_scale_changed(old_scale, m_scale);
}

ScaleMode ScaleManager::get_mode() const {
    return m_mode;
}

void ScaleManager::apply_to_imgui() {
    if (!m_initialized) {
        LOG_WARN("ScaleManager not initialized, cannot apply to ImGui");
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = m_scale;

    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(m_scale);

    LOG_DEBUG("Applied scale {:.2f} to ImGui (FontGlobalScale and Style)", m_scale);
}

const DisplayInfo& ScaleManager::get_display_info() const {
    return m_display_info;
}

float ScaleManager::calculate_recommended_scale(int width, int height) {
    if (width <= 0 || height <= 0) {
        return 1.0f;
    }

    // 精确匹配常见分辨率
    // 1K (1920x1080) 及以下
    if (width <= 1920 && height <= 1200) {
        return 1.0f;
    }
    // 2K (2560x1440) 范围
    if (width <= 2560 && height <= 1600) {
        return 1.5f;
    }
    // 4K (3840x2160) 及以上
    return 2.0f;
}

bool ScaleManager::is_initialized() const {
    return m_initialized;
}

void ScaleManager::apply_scale(float scale, ScaleMode mode) {
    m_scale = scale;
    m_mode = mode;
}

void ScaleManager::save_to_config() {
    auto& config = Core::Config::ConfigManager::instance();

    config.set("dearts.scale.value", static_cast<double>(m_scale));
    config.set("dearts.scale.mode", static_cast<int>(m_mode));

    LOG_DEBUG("Scale saved to config: value={:.2f}, mode={}", m_scale, static_cast<int>(m_mode));
}

void ScaleManager::load_from_config() {
    auto& config = Core::Config::ConfigManager::instance();

    // 读取缩放值
    auto scale_result = config.get<double>("dearts.scale.value");
    if (scale_result.isOk()) {
        float saved_scale = static_cast<float>(scale_result.unwrap());
        // 限制在有效范围内
        m_scale = std::clamp(saved_scale, 0.5f, 2.0f);
    }

    // 读取缩放模式
    auto mode_result = config.get<int>("dearts.scale.mode");
    if (mode_result.isOk()) {
        int saved_mode = mode_result.unwrap();
        if (saved_mode >= 0 && saved_mode <= 2) {
            m_mode = static_cast<ScaleMode>(saved_mode);
        }
    }

    LOG_DEBUG("Scale loaded from config: value={:.2f}, mode={}", m_scale, static_cast<int>(m_mode));
}

void ScaleManager::notify_scale_changed(float old_scale, float new_scale) {
    ScaleChangedEvent event{
        .old_scale = old_scale,
        .new_scale = new_scale,
        .mode = m_mode
    };

    // 通过 EventBus 发布事件（如果需要的话）
    // Core::Event::EventBus::instance().publish(event);
}

} // namespace DearTs::Core::UI
