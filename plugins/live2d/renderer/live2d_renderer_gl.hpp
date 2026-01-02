/**
 * @file live2d_renderer_gl.hpp
 * @brief Live2D OpenGL 渲染器实现
 * @details 基于 Live2D Cubism SDK for OpenGL 的渲染器实现
 * @author DearTs Team
 * @date 2025
 * @version 1.0.0
 */

#pragma once

#include "ilive2d_renderer.hpp"
#include <SDL3/SDL.h>
#include <memory>

// 前向声明 OpenGL 类型，避免包含 OpenGL 头文件
typedef unsigned int GLuint;

// Live2D SDK forward declarations
namespace Live2D { namespace Cubism { namespace Framework {
    class CubismFramework;
    class CubismModel;
}}}

namespace DearTs::Plugins::Live2D {

// 导入 Result 类型到当前命名空间
using DearTs::Core::Result;

/**
 * @brief 性能统计信息
 */
struct PerformanceStats {
    float frame_time_ms = 0.0f;       ///< 帧渲染时间（毫秒）
    float fps = 0.0f;                 ///< 帧率
    uint32_t draw_calls = 0;          ///< 绘制调用次数
    uint32_t vertex_count = 0;        ///< 顶点数量
    uint32_t triangle_count = 0;      ///< 三角形数量
    uint32_t texture_bindings = 0;    ///< 纹理绑定次数
    bool is_dirty = true;             ///< 是否需要重新计算统计信息
};

/**
 * @brief Live2D OpenGL 渲染器
 *
 * @details
 * 使用 Live2D Cubism SDK for OpenGL 进行渲染。
 * 关键特性：
 * - 管理 Live2D Framework 生命周期
 * - OpenGL 上下文管理
 * - 性能监控和分析
 * - 帧缓冲对象（FBO）支持
 *
 * @note
 * - 需要在有效的 OpenGL 上下文中初始化
 * - 线程不安全，必须在主线程调用
 * - 使用 RAII 管理资源
 */
class Live2DRendererGL : public ILive2DRenderer {
public:
    /**
     * @brief 构造函数
     */
    Live2DRendererGL();

    /**
     * @brief 析构函数
     */
    ~Live2DRendererGL() override;

    // 禁止拷贝和移动
    Live2DRendererGL(const Live2DRendererGL&) = delete;
    Live2DRendererGL& operator=(const Live2DRendererGL&) = delete;
    Live2DRendererGL(Live2DRendererGL&&) = delete;
    Live2DRendererGL& operator=(Live2DRendererGL&&) = delete;

    /**
     * @brief 初始化渲染器
     * @param config 渲染器配置
     * @return 成功返回 void，失败返回错误信息
     */
    Result<void, std::string> initialize(const RendererConfig& config) override;

    /**
     * @brief 关闭渲染器并释放资源
     */
    void shutdown() override;

    /**
     * @brief 获取渲染器状态
     */
    RendererState get_state() const override;

    /**
     * @brief 开始渲染帧
     */
    void begin_frame() override;

    /**
     * @brief 结束渲染帧
     */
    void end_frame() override;

    /**
     * @brief 设置视口大小
     */
    void set_viewport(int width, int height) override;

    /**
     * @brief 获取视口宽度
     */
    int get_viewport_width() const override;

    /**
     * @brief 获取视口高度
     */
    int get_viewport_height() const override;

    /**
     * @brief 设置清除颜色
     */
    void set_clear_color(float r, float g, float b, float a = 1.0f) override;

    /**
     * @brief 获取性能统计信息
     * @return JSON 格式的性能数据
     */
    std::string get_profiling_data() const override;

    /**
     * @brief 检查渲染器是否可用
     */
    bool is_valid() const override;

    /**
     * @brief 获取性能统计信息（内部使用）
     */
    const PerformanceStats& get_stats() const { return m_stats; }

    /**
     * @brief 更新性能统计信息
     */
    void update_stats(float frame_time_ms);

private:
    /**
     * @brief 初始化 Live2D Framework
     */
    Result<void, std::string> initialize_cubism_framework();

    /**
     * @brief 清理 Live2D Framework
     */
    void cleanup_cubism_framework();

    /**
     * @brief 初始化 OpenGL 状态
     */
    Result<void, std::string> initialize_opengl();

    /**
     * @brief 创建帧缓冲对象（可选）
     */
    Result<void, std::string> create_fbo();

    /**
     * @brief 销毁帧缓冲对象
     */
    void destroy_fbo();

private:
    RendererState m_state = RendererState::Uninitialized;
    RendererConfig m_config;

    // OpenGL 状态
    int m_viewport_width = 0;
    int m_viewport_height = 0;
    float m_clear_color[4] = {0.0f, 0.0f, 0.0f, 1.0f};

    // 帧缓冲对象（可选）
    GLuint m_fbo = 0;
    GLuint m_fbo_texture = 0;
    GLuint m_fbo_depth_buffer = 0;

    // 性能统计
    PerformanceStats m_stats;
    uint64_t m_last_frame_time = 0;

    // Live2D Framework 初始化标志
    bool m_cubism_initialized = false;
};

} // namespace DearTs::Plugins::Live2D
