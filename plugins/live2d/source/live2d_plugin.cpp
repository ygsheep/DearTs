/**
 * @file live2d_plugin.cpp
 * @brief Live2D 插件实现
 */

#include "live2d_plugin.hpp"
#include <SDL3/SDL_opengl.h>

// 取消 Windows 宏定义，避免与 logger 冲突
#ifdef ERROR
#undef ERROR
#endif

#include "core/content/commands.h"
#include "core/event/event_bus.h"
#include "core/config/config_manager.h"
#include "liblogger/logger.h"

namespace DearTs::Plugins::Live2D {

// ============================================================================
// IPlugin 接口实现
// ============================================================================

PluginInfo Live2DPlugin::get_info() const {
    return PluginInfo{
        .name = "Live2D",
        .author = "DearTs Team",
        .description = "Live2D Cubism SDK integration for character rendering",
        .version = "1.0.0",
        .api_version = "1.0.0"
    };
}

Result<void, std::string> Live2DPlugin::on_load() {
    LOG_INFO("Live2DPlugin: Loading plugin");

    // 从配置文件加载插件配置
    ConfigScope config("live2d");
    m_config.models_directory = config.get_or<std::string>("models_directory", "models/live2d");
    m_config.enable_profiling = config.get_or<bool>("enable_profiling", false);
    m_config.use_fbo = config.get_or<bool>("use_fbo", false);
    m_config.fbo_downsample = config.get_or<int>("fbo_downsample", 1);
    m_config.auto_play_idle_motion = config.get_or<bool>("auto_play_idle_motion", true);
    m_config.model_scale = static_cast<float>(config.get_or<double>("model_scale", 1.0));

    // 初始化渲染器
    auto result = initialize_renderer();
    if (result.isErr()) {
        return result;
    }

    // 注册命令
    register_commands();

    // 注册设置
    register_settings();

    // 扫描模型目录
    scan_models_directory();

    LOG_INFO("Live2DPlugin: Plugin loaded successfully");
    return Result<void, std::string>::ok();
}

void Live2DPlugin::on_unload() {
    LOG_INFO("Live2DPlugin: Unloading plugin");

    // 卸载所有模型
    for (auto& [name, model] : m_models) {
        model->unload();
    }
    m_models.clear();
    m_active_model_name.clear();

    // 清理渲染器
    cleanup_renderer();

    // 事件订阅会自动取消（RAII）

    LOG_INFO("Live2DPlugin: Plugin unloaded");
}

void Live2DPlugin::on_enable() {
    LOG_INFO("Live2DPlugin: Enabling plugin");
    m_enabled = true;
}

void Live2DPlugin::on_disable() {
    LOG_INFO("Live2DPlugin: Disabling plugin");
    m_enabled = false;
}

// ============================================================================
// 模型管理
// ============================================================================

Result<void, std::string> Live2DPlugin::load_model(
    const std::string& model_name,
    const std::filesystem::path& model_dir
) {
    LOG_INFO("Live2DPlugin: Loading model '{}' from: {}", model_name, model_dir.string());

    // 检查模型是否已存在
    if (m_models.find(model_name) != m_models.end()) {
        std::string error_msg = "Model '";
        error_msg += model_name;
        error_msg += "' already loaded";
        return Result<void, std::string>::err(error_msg);
    }

    // 创建模型实例
    auto model = std::make_unique<Live2DModelInstance>();

    // 加载模型
    auto result = model->load_from_directory(model_dir);
    if (result.isErr()) {
        return result;
    }

    // 设置为活动模型（如果是第一个模型）
    if (m_active_model_name.empty()) {
        m_active_model_name = model_name;
    }

    // 添加到模型列表
    m_models[model_name] = std::move(model);

    LOG_INFO("Live2DPlugin: Model '{}' loaded successfully", model_name);
    return Result<void, std::string>::ok();
}

void Live2DPlugin::unload_model(const std::string& model_name) {
    auto it = m_models.find(model_name);
    if (it == m_models.end()) {
        LOG_WARN("Live2DPlugin: Model '{}' not found", model_name);
        return;
    }

    LOG_INFO("Live2DPlugin: Unloading model '{}'", model_name);
    it->second->unload();
    m_models.erase(it);

    // 如果卸载的是活动模型，切换到其他模型或清空
    if (m_active_model_name == model_name) {
        if (m_models.empty()) {
            m_active_model_name.clear();
        } else {
            m_active_model_name = m_models.begin()->first;
        }
    }
}

Live2DModelInstance* Live2DPlugin::get_model(const std::string& model_name) {
    auto it = m_models.find(model_name);
    if (it == m_models.end()) {
        return nullptr;
    }
    return it->second.get();
}

std::vector<std::string> Live2DPlugin::get_model_names() const {
    std::vector<std::string> names;
    names.reserve(m_models.size());
    for (const auto& [name, model] : m_models) {
        names.push_back(name);
    }
    return names;
}

void Live2DPlugin::set_active_model(const std::string& model_name) {
    if (m_models.find(model_name) == m_models.end()) {
        LOG_WARN("Live2DPlugin: Model '{}' not found", model_name);
        return;
    }
    m_active_model_name = model_name;
    LOG_INFO("Live2DPlugin: Active model set to '{}'", model_name);
}

Live2DModelInstance* Live2DPlugin::get_active_model() {
    if (m_active_model_name.empty()) {
        return nullptr;
    }
    return get_model(m_active_model_name);
}

// ============================================================================
// 更新和渲染
// ============================================================================

void Live2DPlugin::update(float delta_time) {
    if (!m_enabled) {
        return;
    }

    // 更新活动模型
    auto* active_model = get_active_model();
    if (active_model) {
        active_model->update(delta_time);
    }
}

void Live2DPlugin::render() {
    if (!m_enabled || !m_renderer) {
        return;
    }

    // 开始渲染帧
    m_renderer->begin_frame();

    // 渲染活动模型
    auto* active_model = get_active_model();
    if (active_model) {
        active_model->draw();
    }

    // 结束渲染帧
    m_renderer->end_frame();
}

// ============================================================================
// 私有方法
// ============================================================================

Result<void, std::string> Live2DPlugin::initialize_renderer() {
    LOG_INFO("Live2DPlugin: Initializing renderer");

    // 创建渲染器
    m_renderer = std::make_unique<Live2DRendererGL>();

    // 配置渲染器
    RendererConfig config;
    config.viewport_width = 800;   // TODO: 从主窗口获取
    config.viewport_height = 600;
    config.use_fbo = m_config.use_fbo;
    config.fbo_downsample = m_config.fbo_downsample;
    config.enable_profiling = m_config.enable_profiling;

    // 初始化渲染器
    auto result = m_renderer->initialize(config);
    if (result.isErr()) {
        LOG_ERROR("Live2DPlugin: Failed to initialize renderer: {}", result.error());
        return result;
    }

    // 创建纹理上传器
    m_texture_uploader = std::make_unique<SDLTextureUploader>();
    TextureUploadConfig upload_config;
    // TODO: 配置 PBO（Phase 5）

    result = m_texture_uploader->initialize(upload_config);
    if (result.isErr()) {
        LOG_WARN("Live2DPlugin: Failed to initialize texture uploader: {}", result.error());
        // 非致命错误，继续
    }

    LOG_INFO("Live2DPlugin: Renderer initialized successfully");
    return Result<void, std::string>::ok();
}

void Live2DPlugin::cleanup_renderer() {
    if (m_texture_uploader) {
        m_texture_uploader->shutdown();
        m_texture_uploader.reset();
    }

    if (m_renderer) {
        m_renderer->shutdown();
        m_renderer.reset();
    }

    LOG_INFO("Live2DPlugin: Renderer cleaned up");
}

void Live2DPlugin::register_commands() {
    // TODO: 注册 Live2D 相关命令
    // - live2d.load_model
    // - live2d.unload_model
    // - live2d.set_active_model
    // - live2d.set_parameter
    // - live2d.list_models
    LOG_DEBUG("Live2DPlugin: Commands registered");
}

void Live2DPlugin::register_settings() {
    // TODO: 注册 Live2D 设置到 Content Registry
    // - models_directory
    // - enable_profiling
    // - use_fbo
    // - model_scale
    LOG_DEBUG("Live2DPlugin: Settings registered");
}

void Live2DPlugin::scan_models_directory() {
    LOG_INFO("Live2DPlugin: Scanning models directory: {}",
             m_config.models_directory);

    // 使用 Live2DModelManager 扫描目录
    auto& manager = Live2DModelManager::instance();

    // 扫描模型目录（递归）
    auto result = manager.scan_directory(m_config.models_directory, true);
    if (result.isErr()) {
        LOG_ERROR("Live2DPlugin: Failed to scan models directory: {}", result.error());
        return;
    }

    LOG_INFO("Live2DPlugin: Found {} models", result.unwrap());

    // 获取所有已注册的模型
    auto registered_models = manager.get_registered_models();

    // 自动加载所有找到的模型
    for (const auto& model_id : registered_models) {
        LOG_INFO("Live2DPlugin: Loading model '{}'", model_id);

        auto load_result = manager.load_model(model_id);
        if (load_result.isErr()) {
            LOG_WARN("Live2DPlugin: Failed to load model '{}': {}",
                     model_id, load_result.error());
        } else {
            LOG_INFO("Live2DPlugin: Model '{}' loaded successfully", model_id);
        }
    }

    // 设置第一个模型为活动模型
    if (!registered_models.empty()) {
        auto set_result = manager.set_active_model(registered_models[0]);
        if (set_result.isOk()) {
            LOG_INFO("Live2DPlugin: Active model set to '{}'", registered_models[0]);
        }
    }

    LOG_DEBUG("Live2DPlugin: Model directory scan complete");
}

} // namespace DearTs::Plugins::Live2D
