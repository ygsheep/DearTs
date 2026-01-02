/**
 * @file live2d_plugin.hpp
 * @brief Live2D 插件主类
 * @details Live2D Cubism SDK 集成插件
 * @author DearTs Team
 * @date 2025
 * @version 1.0.0
 */

#pragma once

#include "core/plugin/plugin.h"
#include "core/event/event_bus.h"
#include "core/config/config_manager.h"
#include "../renderer/live2d_renderer_gl.hpp"
#include "../renderer/sdl_texture_uploader.hpp"
#include "live2d_model_instance.hpp"
#include "live2d_model_manager.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <filesystem>

namespace DearTs::Plugins::Live2D {

// 导入类型到当前命名空间
using DearTs::Core::Result;
using DearTs::Core::Plugin::PluginInfo;
using DearTs::Core::Config::ConfigScope;

/**
 * @brief Live2D 模型位置
 */
enum class Live2DModelPosition {
    TopLeft,        ///< 左上角
    TopCenter,      ///< 顶部居中
    TopRight,       ///< 右上角
    CenterLeft,     ///< 左侧居中
    Center,         ///< 居中
    CenterRight,    ///< 右侧居中
    BottomLeft,     ///< 左下角
    BottomCenter,   ///< 底部居中
    BottomRight,    ///< 右下角
};

/**
 * @brief Live2D 插件配置
 */
struct Live2DPluginConfig {
    // === 模型设置 ===
    std::string models_directory = "resources/live2d";    ///< 模型目录
    std::string active_model = "";                       ///< 当前活动模型 ID
    float model_scale = 1.0f;                           ///< 模型缩放
    Live2DModelPosition model_position = Live2DModelPosition::BottomRight;  ///< 模型位置
    int model_x_offset = 0;                             ///< X 轴偏移
    int model_y_offset = 0;                             ///< Y 轴偏移
    float model_opacity = 1.0f;                         ///< 模型透明度 (0.0 - 1.0)

    // === 渲染设置 ===
    bool use_fbo = false;                               ///< 是否使用 FBO
    int fbo_downsample = 1;                             ///< FBO 降采样比例 (1, 2, 4)
    bool enable_profiling = false;                      ///< 是否启用性能分析

    // === 动画设置 ===
    bool auto_play_idle_motion = true;                  ///< 是否自动播放待机动作
    float motion_speed = 1.0f;                          ///< 动作播放速度
    bool enable_breath = true;                          ///< 是否启用呼吸效果
    bool enable_eye_blink = true;                       ///< 是否启用眨眼
    bool enable_physics = true;                         ///< 是否启用物理模拟

    // === 交互设置 ===
    bool enable_mouse_follow = false;                   ///< 是否启用鼠标跟随
    float mouse_follow_intensity = 0.5f;                ///< 鼠标跟随强度 (0.0 - 1.0)
    bool enable_drag = false;                           ///< 是否启用拖拽

    // === 调试设置 ===
    bool show_hit_areas = false;                        ///< 是否显示碰撞区域
    bool show_parameters = false;                       ///< 是否显示参数列表
};

/**
 * @brief Live2D 插件
 *
 * @details
 * Live2D Cubism SDK 集成插件，提供：
 * - Live2D 模型加载和渲染
 * - 模型参数控制
 * - 动作和物理模拟
 * - 事件系统（鼠标跟随、点击响应等）
 *
 * Phase 1 功能：
 * - SDK 集成和基础渲染
 * - 模型加载
 * - 基础渲染循环
 *
 * @note
 * - 插件使用 RAII 管理资源
 * - 线程不安全，必须在主线程调用
 */
class Live2DPlugin : public DearTs::Core::Plugin::IPlugin {
public:
    /**
     * @brief 构造函数
     */
    Live2DPlugin() = default;

    /**
     * @brief 析构函数
     */
    ~Live2DPlugin() override = default;

    /**
     * @brief 获取插件信息
     */
    PluginInfo get_info() const override;

    /**
     * @brief 插件加载时调用
     */
    Result<void, std::string> on_load() override;

    /**
     * @brief 插件卸载时调用
     */
    void on_unload() override;

    /**
     * @brief 插件启用时调用
     */
    void on_enable() override;

    /**
     * @brief 插件禁用时调用
     */
    void on_disable() override;

    // ========================================================================
    // 公共 API
    // ========================================================================

    /**
     * @brief 加载模型
     * @param model_name 模型名称
     * @param model_dir 模型目录
     * @return 成功返回 void，失败返回错误信息
     */
    Result<void, std::string> load_model(
        const std::string& model_name,
        const std::filesystem::path& model_dir
    );

    /**
     * @brief 卸载模型
     * @param model_name 模型名称
     */
    void unload_model(const std::string& model_name);

    /**
     * @brief 获取模型
     * @param model_name 模型名称
     * @return 模型指针，失败返回 nullptr
     */
    Live2DModelInstance* get_model(const std::string& model_name);

    /**
     * @brief 获取所有模型名称
     */
    std::vector<std::string> get_model_names() const;

    /**
     * @brief 设置活动模型
     * @param model_name 模型名称
     */
    void set_active_model(const std::string& model_name);

    /**
     * @brief 获取活动模型
     */
    Live2DModelInstance* get_active_model();

    /**
     * @brief 获取渲染器
     */
    Live2DRendererGL* get_renderer() { return m_renderer.get(); }

    /**
     * @brief 获取插件配置
     */
    Live2DPluginConfig& get_config() { return m_config; }
    const Live2DPluginConfig& get_config() const { return m_config; }

    /**
     * @brief 更新插件（每帧调用）
     * @param delta_time 时间增量（秒）
     */
    void update(float delta_time);

    /**
     * @brief 渲染插件（每帧调用）
     */
    void render();

private:
    /**
     * @brief 初始化渲染器
     */
    Result<void, std::string> initialize_renderer();

    /**
     * @brief 清理渲染器
     */
    void cleanup_renderer();

    /**
     * @brief 确保渲染器已初始化（延迟初始化）
     */
    void ensure_renderer_initialized();

    /**
     * @brief 加载扫描到的模型（在渲染器初始化后调用）
     */
    void load_discovered_models();

    /**
     * @brief 注册命令
     */
    void register_commands();

    /**
     * @brief 注册设置
     */
    void register_settings();

    /**
     * @brief 注册视图
     */
    void register_views();

    /**
     * @brief 扫描并加载模型目录
     */
    void scan_models_directory();

private:
    Live2DPluginConfig m_config;
    std::unique_ptr<Live2DRendererGL> m_renderer;
    std::unique_ptr<SDLTextureUploader> m_texture_uploader;
    bool m_renderer_initialized = false;

    // 模型管理
    std::unordered_map<std::string, std::unique_ptr<Live2DModelInstance>> m_models;
    std::string m_active_model_name;
    std::vector<std::string> m_discovered_models;  // 扫描到的模型（延迟加载）

    // 事件订阅（RAII 自动取消）
    std::vector<DearTs::Core::Event::EventToken> m_event_tokens;

    // 是否已启用
    bool m_enabled = false;
};

} // namespace DearTs::Plugins::Live2D
