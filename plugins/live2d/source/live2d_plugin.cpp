/**
 * @file live2d_plugin.cpp
 * @brief Live2D 插件实现
 */

#include "live2d_plugin.hpp"
#include "views/live2d_settings_view.hpp"
#include "core/content/registry_base.h"
#include <SDL3/SDL_opengl.h>
#include <algorithm>

// 取消 Windows 宏定义，避免与 logger 冲突
#ifdef ERROR
#undef ERROR
#endif

#include "core/content/commands.h"
#include "core/content/settings.h"
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

    // 注意：不在此处初始化渲染器，因为 OpenGL 上下文还未创建
    // 渲染器将在首次调用 update() 或 render() 时延迟初始化

    // 注册命令
    register_commands();

    // 注册设置
    register_settings();

    // 注册视图
    register_views();

    // 扫描模型目录
    scan_models_directory();

    LOG_INFO("Live2DPlugin: Plugin loaded successfully (renderer will be initialized on first use)");
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

    // 先添加已加载的模型
    names.reserve(m_models.size() + m_discovered_models.size());
    for (const auto& [name, model] : m_models) {
        names.push_back(name);
    }

    // 再添加已扫描但未加载的模型（如果渲染器还未初始化）
    for (const auto& name : m_discovered_models) {
        // 避免重复添加
        if (std::find(names.begin(), names.end(), name) == names.end()) {
            names.push_back(name);
        }
    }

    return names;
}

void Live2DPlugin::set_active_model(const std::string& model_name) {
    // 检查模型是否已加载
    if (m_models.find(model_name) != m_models.end()) {
        m_active_model_name = model_name;
        LOG_INFO("Live2DPlugin: Active model set to '{}'", model_name);
        return;
    }

    // 模型未加载，尝试加载它
    LOG_INFO("Live2DPlugin: Model '{}' not loaded yet, attempting to load", model_name);

    // 确保渲染器已初始化
    ensure_renderer_initialized();

    // 检查渲染器是否初始化成功
    if (!m_renderer_initialized || !m_renderer) {
        LOG_ERROR("Live2DPlugin: Cannot load model '{}', renderer not initialized", model_name);
        LOG_ERROR("Live2DPlugin: Make sure OpenGL context is available");
        return;
    }

    // 尝试从 Live2DModelManager 加载模型
    auto& manager = Live2DModelManager::instance();
    auto load_result = manager.load_model(model_name);
    if (load_result.isErr()) {
        LOG_ERROR("Live2DPlugin: Failed to load model '{}': {}", model_name, load_result.error());
        return;
    }

    // 模型加载成功，设置为活动模型
    m_active_model_name = model_name;
    LOG_INFO("Live2DPlugin: Model '{}' loaded and set as active", model_name);
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

    // 确保渲染器已初始化（延迟初始化，等待 OpenGL 上下文创建）
    ensure_renderer_initialized();

    // 更新活动模型
    auto* active_model = get_active_model();
    if (active_model) {
        active_model->update(delta_time);
    }
}

void Live2DPlugin::render() {
    if (!m_enabled) {
        return;
    }

    // 确保渲染器已初始化
    ensure_renderer_initialized();

    if (!m_renderer) {
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

    m_renderer_initialized = false;

    LOG_INFO("Live2DPlugin: Renderer cleaned up");
}

void Live2DPlugin::ensure_renderer_initialized() {
    // 如果已经初始化，直接返回
    if (m_renderer_initialized) {
        return;
    }

    // 尝试初始化渲染器
    LOG_INFO("Live2DPlugin: Attempting to initialize renderer (lazy initialization)");

    auto result = initialize_renderer();
    if (result.isOk()) {
        m_renderer_initialized = true;
        LOG_INFO("Live2DPlugin: Renderer initialized successfully");

        // 渲染器初始化成功后，加载扫描到的模型
        load_discovered_models();
    } else {
        LOG_ERROR("Live2DPlugin: Failed to initialize renderer: {}", result.error());
        // 不抛出异常，标记为未初始化
        m_renderer_initialized = false;
    }
}

void Live2DPlugin::load_discovered_models() {
    // 检查渲染器是否已初始化
    if (!m_renderer_initialized || !m_renderer) {
        LOG_WARN("Live2DPlugin: Cannot load models, renderer not initialized");
        LOG_WARN("Live2DPlugin: Models will be loaded when OpenGL context is available");
        return;
    }

    if (m_discovered_models.empty()) {
        LOG_INFO("Live2DPlugin: No models to load");
        return;
    }

    LOG_INFO("Live2DPlugin: Loading {} discovered models", m_discovered_models.size());

    auto& manager = Live2DModelManager::instance();

    // 加载所有找到的模型
    for (const auto& model_id : m_discovered_models) {
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
    if (!m_discovered_models.empty()) {
        auto set_result = manager.set_active_model(m_discovered_models[0]);
        if (set_result.isOk()) {
            LOG_INFO("Live2DPlugin: Active model set to '{}'", m_discovered_models[0]);
        }
    }

    // 清空已发现的模型列表（已经加载）
    m_discovered_models.clear();
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
    using namespace DearTs::Core::ContentRegistry::Settings;

    // === 模型设置 ===
    {
        auto& item = add("live2d.models_directory", "Models Directory", m_config.models_directory);
        item.change_callback = [this](const std::string& value) {
            m_config.models_directory = value;
        };
    }

    {
        auto& item = add("live2d.active_model", "Active Model", m_config.active_model);
        item.change_callback = [this](const std::string& value) {
            m_config.active_model = value;
            set_active_model(value);
        };
    }

    {
        auto& item = add("live2d.model_scale", "Model Scale", std::to_string(m_config.model_scale));
        item.change_callback = [this](const std::string& value) {
            try {
                m_config.model_scale = std::stof(value);
            } catch (...) {
                LOG_WARN("Live2DPlugin: Invalid model_scale value: {}", value);
            }
        };
    }

    {
        auto& item = add("live2d.model_position", "Model Position", "BottomRight");
        item.change_callback = [this](const std::string& value) {
            if (value == "TopLeft") m_config.model_position = Live2DModelPosition::TopLeft;
            else if (value == "TopCenter") m_config.model_position = Live2DModelPosition::TopCenter;
            else if (value == "TopRight") m_config.model_position = Live2DModelPosition::TopRight;
            else if (value == "CenterLeft") m_config.model_position = Live2DModelPosition::CenterLeft;
            else if (value == "Center") m_config.model_position = Live2DModelPosition::Center;
            else if (value == "CenterRight") m_config.model_position = Live2DModelPosition::CenterRight;
            else if (value == "BottomLeft") m_config.model_position = Live2DModelPosition::BottomLeft;
            else if (value == "BottomCenter") m_config.model_position = Live2DModelPosition::BottomCenter;
            else m_config.model_position = Live2DModelPosition::BottomRight;
        };
    }

    {
        auto& item = add("live2d.model_x_offset", "Model X Offset", std::to_string(m_config.model_x_offset));
        item.change_callback = [this](const std::string& value) {
            try {
                m_config.model_x_offset = std::stoi(value);
            } catch (...) {
                LOG_WARN("Live2DPlugin: Invalid model_x_offset value: {}", value);
            }
        };
    }

    {
        auto& item = add("live2d.model_y_offset", "Model Y Offset", std::to_string(m_config.model_y_offset));
        item.change_callback = [this](const std::string& value) {
            try {
                m_config.model_y_offset = std::stoi(value);
            } catch (...) {
                LOG_WARN("Live2DPlugin: Invalid model_y_offset value: {}", value);
            }
        };
    }

    {
        auto& item = add("live2d.model_opacity", "Model Opacity", std::to_string(m_config.model_opacity));
        item.change_callback = [this](const std::string& value) {
            try {
                m_config.model_opacity = std::stof(value);
            } catch (...) {
                LOG_WARN("Live2DPlugin: Invalid model_opacity value: {}", value);
            }
        };
    }

    // === 渲染设置 ===
    {
        auto& item = add("live2d.use_fbo", "Use FBO", m_config.use_fbo ? "true" : "false");
        item.change_callback = [this](const std::string& value) {
            m_config.use_fbo = (value == "true");
        };
    }

    {
        auto& item = add("live2d.fbo_downsample", "FBO Downsample", std::to_string(m_config.fbo_downsample));
        item.change_callback = [this](const std::string& value) {
            try {
                int ds = std::stoi(value);
                if (ds == 1 || ds == 2 || ds == 4) {
                    m_config.fbo_downsample = ds;
                } else {
                    LOG_WARN("Live2DPlugin: Invalid fbo_downsample value: {}", value);
                }
            } catch (...) {
                LOG_WARN("Live2DPlugin: Invalid fbo_downsample value: {}", value);
            }
        };
    }

    {
        auto& item = add("live2d.enable_profiling", "Enable Profiling", m_config.enable_profiling ? "true" : "false");
        item.change_callback = [this](const std::string& value) {
            m_config.enable_profiling = (value == "true");
        };
    }

    // === 动画设置 ===
    {
        auto& item = add("live2d.auto_play_idle_motion", "Auto Play Idle Motion", m_config.auto_play_idle_motion ? "true" : "false");
        item.change_callback = [this](const std::string& value) {
            m_config.auto_play_idle_motion = (value == "true");
        };
    }

    {
        auto& item = add("live2d.motion_speed", "Motion Speed", std::to_string(m_config.motion_speed));
        item.change_callback = [this](const std::string& value) {
            try {
                m_config.motion_speed = std::stof(value);
            } catch (...) {
                LOG_WARN("Live2DPlugin: Invalid motion_speed value: {}", value);
            }
        };
    }

    {
        auto& item = add("live2d.enable_breath", "Enable Breath", m_config.enable_breath ? "true" : "false");
        item.change_callback = [this](const std::string& value) {
            m_config.enable_breath = (value == "true");
        };
    }

    {
        auto& item = add("live2d.enable_eye_blink", "Enable Eye Blink", m_config.enable_eye_blink ? "true" : "false");
        item.change_callback = [this](const std::string& value) {
            m_config.enable_eye_blink = (value == "true");
        };
    }

    {
        auto& item = add("live2d.enable_physics", "Enable Physics", m_config.enable_physics ? "true" : "false");
        item.change_callback = [this](const std::string& value) {
            m_config.enable_physics = (value == "true");
        };
    }

    // === 交互设置 ===
    {
        auto& item = add("live2d.enable_mouse_follow", "Enable Mouse Follow", m_config.enable_mouse_follow ? "true" : "false");
        item.change_callback = [this](const std::string& value) {
            m_config.enable_mouse_follow = (value == "true");
        };
    }

    {
        auto& item = add("live2d.mouse_follow_intensity", "Mouse Follow Intensity", std::to_string(m_config.mouse_follow_intensity));
        item.change_callback = [this](const std::string& value) {
            try {
                m_config.mouse_follow_intensity = std::stof(value);
            } catch (...) {
                LOG_WARN("Live2DPlugin: Invalid mouse_follow_intensity value: {}", value);
            }
        };
    }

    {
        auto& item = add("live2d.enable_drag", "Enable Drag", m_config.enable_drag ? "true" : "false");
        item.change_callback = [this](const std::string& value) {
            m_config.enable_drag = (value == "true");
        };
    }

    // === 调试设置 ===
    {
        auto& item = add("live2d.show_hit_areas", "Show Hit Areas", m_config.show_hit_areas ? "true" : "false");
        item.change_callback = [this](const std::string& value) {
            m_config.show_hit_areas = (value == "true");
        };
    }

    {
        auto& item = add("live2d.show_parameters", "Show Parameters", m_config.show_parameters ? "true" : "false");
        item.change_callback = [this](const std::string& value) {
            m_config.show_parameters = (value == "true");
        };
    }

    LOG_INFO("Live2DPlugin: Settings registered (19 settings)");
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

    // 注意：不在此处加载模型，因为 OpenGL 上下文还未创建
    // 模型将在渲染器初始化后延迟加载（在 ensure_renderer_initialized() 中）
    LOG_INFO("Live2DPlugin: Models registered (will be loaded after renderer initialization)");

    // 保存找到的模型列表，用于后续加载
    m_discovered_models = registered_models;

    LOG_DEBUG("Live2DPlugin: Model directory scan complete");
}

void Live2DPlugin::register_views() {
    LOG_INFO("Live2DPlugin: Registering views");

    // 注册 Live2D 设置视图
    using namespace DearTs::Core::ContentRegistry::Views;
    add<Live2DSettingsView>(this);

    LOG_INFO("Live2DPlugin: Views registered successfully");
}

} // namespace DearTs::Plugins::Live2D
