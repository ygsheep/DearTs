/**
 * @file live2d_renderer_gl.cpp
 * @brief Live2D OpenGL 渲染器实现
 */

#include "live2d_renderer_gl.hpp"
// GLEW 必须在任何其他 OpenGL 头之前包含
#include <GL/glew.h>
#include <SDL3/SDL_opengl.h>

// 取消 Windows 宏定义，避免与 logger 冲突
#ifdef ERROR
#undef ERROR
#endif

// Live2D SDK headers
#include <Live2DCubismCoreWrapper.hpp>
#include <CubismFramework.hpp>
#include <ICubismAllocator.hpp>
#include <CubismModelSettingJson.hpp>
#include <Model/CubismUserModel.hpp>
#include <Rendering/CubismRenderer.hpp>

#include "liblogger/logger.h"
#include <nlohmann/json.hpp>
#include <chrono>

// 使用 Live2D 命名空间简写
using namespace Live2D::Cubism::Framework;

namespace DearTs::Plugins::Live2D {

// ============================================================================
// Live2D Allocator 实现
// ============================================================================

/**
 * @brief Live2D SDK 内存分配器
 */
class Live2DAllocator : public ICubismAllocator {
public:
    void* Allocate(const csmSizeType size) override {
        return malloc(size);
    }

    void Deallocate(void* memory) override {
        free(memory);
    }

    void* AllocateAligned(const csmSizeType size, const csmUint32 alignment) override {
        // Windows 使用 _aligned_malloc，其他平台使用 aligned_alloc
#ifdef _WIN32
        return _aligned_malloc(size, alignment);
#else
        return aligned_alloc(alignment, size);
#endif
    }

    void DeallocateAligned(void* alignedMemory) override {
#ifdef _WIN32
        _aligned_free(alignedMemory);
#else
        free(alignedMemory);
#endif
    }
};

// ============================================================================
// Live2D 日志回调
// ============================================================================

/**
 * @brief Live2D SDK 日志回调函数（适配器，匹配 csmLogFunction 签名）
 */
static void Live2DLogPrintAdapter(const char* message) {
    // 默认使用 Info 级别
    LOG_INFO("Live2D SDK: {}", message);
}

/**
 * @brief Live2D SDK 日志回调函数（带级别）
 */
static void Live2DLogPrint(const int level, const char* message) {
    switch (level) {
        case CubismFramework::Option::LogLevel_Verbose:
        case CubismFramework::Option::LogLevel_Debug:
            LOG_DEBUG("Live2D SDK: {}", message);
            break;
        case CubismFramework::Option::LogLevel_Info:
            LOG_INFO("Live2D SDK: {}", message);
            break;
        case CubismFramework::Option::LogLevel_Warning:
            LOG_WARN("Live2D SDK: {}", message);
            break;
        case CubismFramework::Option::LogLevel_Error:
            LOG_ERROR("Live2D SDK: {}", message);
            break;
        default:
            LOG_INFO("Live2D SDK: {}", message);
            break;
    }
}

// 全局分配器实例
static Live2DAllocator s_allocator;

// ============================================================================
// 构造函数和析构函数
// ============================================================================

Live2DRendererGL::Live2DRendererGL() = default;

Live2DRendererGL::~Live2DRendererGL() {
    shutdown();
}

// ============================================================================
// 初始化和清理
// ============================================================================

Result<void, std::string> Live2DRendererGL::initialize(const RendererConfig& config) {
    if (m_state != RendererState::Uninitialized) {
        return Result<void, std::string>::err("Renderer already initialized");
    }

    m_config = config;
    m_viewport_width = config.viewport_width;
    m_viewport_height = config.viewport_height;

    LOG_INFO("Live2DRendererGL: Initializing renderer ({}x{})",
             m_viewport_width, m_viewport_height);

    // SDL3 的 OpenGL 上下文应该在主应用程序中创建
    // 我们只需要检查是否已经有活动的 OpenGL 上下文

    // 初始化 Live2D Framework
    auto result = initialize_cubism_framework();
    if (result.isErr()) {
        return result;
    }

    // 初始化 OpenGL
    result = initialize_opengl();
    if (result.isErr()) {
        cleanup_cubism_framework();
        return result;
    }

    // 如果需要 FBO
    if (m_config.use_fbo) {
        result = create_fbo();
        if (result.isErr()) {
            LOG_WARN("Live2DRendererGL: Failed to create FBO, continuing without it");
            m_config.use_fbo = false;
        }
    }

    m_state = RendererState::Initialized;
    m_last_frame_time = SDL_GetTicks();

    LOG_INFO("Live2DRendererGL: Renderer initialized successfully");
    return Result<void, std::string>::ok();
}

void Live2DRendererGL::shutdown() {
    if (m_state == RendererState::Uninitialized) {
        return;
    }

    LOG_INFO("Live2DRendererGL: Shutting down renderer");

    // 销毁 FBO
    if (m_config.use_fbo) {
        destroy_fbo();
    }

    // 清理 Live2D Framework
    cleanup_cubism_framework();

    m_state = RendererState::Uninitialized;
    LOG_INFO("Live2DRendererGL: Renderer shutdown complete");
}

// ============================================================================
// 渲染控制
// ============================================================================

void Live2DRendererGL::begin_frame() {
    if (m_state != RendererState::Initialized && m_state != RendererState::Rendering) {
        return;
    }

    m_state = RendererState::Rendering;

    // 绑定 FBO（如果使用）
    if (m_config.use_fbo && m_fbo != 0) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    }

    // 清除缓冲区
    glClearColor(m_clear_color[0], m_clear_color[1], m_clear_color[2], m_clear_color[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 设置视口
    glViewport(0, 0, m_viewport_width, m_viewport_height);

    // 重置统计信息
    m_stats.draw_calls = 0;
    m_stats.vertex_count = 0;
    m_stats.triangle_count = 0;
    m_stats.texture_bindings = 0;
}

void Live2DRendererGL::end_frame() {
    if (m_state != RendererState::Rendering) {
        return;
    }

    // 解绑 FBO（如果使用）
    if (m_config.use_fbo && m_fbo != 0) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // 计算帧时间
    uint64_t current_time = SDL_GetTicks();
    float frame_time = static_cast<float>(current_time - m_last_frame_time);
    m_last_frame_time = current_time;

    // 更新性能统计
    if (m_config.enable_profiling) {
        update_stats(frame_time);
    }

    m_state = RendererState::Initialized;
}

void Live2DRendererGL::set_viewport(int width, int height) {
    m_viewport_width = width;
    m_viewport_height = height;
    glViewport(0, 0, width, height);
}

int Live2DRendererGL::get_viewport_width() const {
    return m_viewport_width;
}

int Live2DRendererGL::get_viewport_height() const {
    return m_viewport_height;
}

void Live2DRendererGL::set_clear_color(float r, float g, float b, float a) {
    m_clear_color[0] = r;
    m_clear_color[1] = g;
    m_clear_color[2] = b;
    m_clear_color[3] = a;
}

// ============================================================================
// 性能监控
// ============================================================================

std::string Live2DRendererGL::get_profiling_data() const {
    nlohmann::json j;
    j["frame_time_ms"] = m_stats.frame_time_ms;
    j["fps"] = m_stats.fps;
    j["draw_calls"] = m_stats.draw_calls;
    j["vertex_count"] = m_stats.vertex_count;
    j["triangle_count"] = m_stats.triangle_count;
    j["texture_bindings"] = m_stats.texture_bindings;
    j["viewport_width"] = m_viewport_width;
    j["viewport_height"] = m_viewport_height;
    j["fbo_enabled"] = m_config.use_fbo;
    return j.dump(2);
}

void Live2DRendererGL::update_stats(float frame_time_ms) {
    m_stats.frame_time_ms = frame_time_ms;
    m_stats.fps = (frame_time_ms > 0.0f) ? (1000.0f / frame_time_ms) : 0.0f;
}

bool Live2DRendererGL::is_valid() const {
    return m_state == RendererState::Initialized || m_state == RendererState::Rendering;
}

RendererState Live2DRendererGL::get_state() const {
    return m_state;
}

// ============================================================================
// Live2D Framework 管理
// ============================================================================

Result<void, std::string> Live2DRendererGL::initialize_cubism_framework() {
    if (m_cubism_initialized) {
        return Result<void, std::string>::ok();
    }

    LOG_INFO("Live2DRendererGL: Initializing Cubism Framework");

    // 初始化 Live2D Cubism Framework
    CubismFramework::Option options;

    // 设置日志函数
    options.LogFunction = Live2DLogPrintAdapter;

    // 设置日志级别
    options.LoggingLevel = CubismFramework::Option::LogLevel_Info;

    // 启动 Live2D Framework
    if (!CubismFramework::StartUp(&s_allocator, &options)) {
        return Result<void, std::string>::err("Failed to start up Live2D Cubism Framework");
    }

    // 初始化 Framework
    CubismFramework::Initialize();

    m_cubism_initialized = true;
    LOG_INFO("Live2DRendererGL: Cubism Framework initialized successfully");

    return Result<void, std::string>::ok();
}

void Live2DRendererGL::cleanup_cubism_framework() {
    if (!m_cubism_initialized) {
        return;
    }

    LOG_INFO("Live2DRendererGL: Cleaning up Cubism Framework");

    // 清理 Live2D Cubism Framework
    Csm::CubismFramework::Dispose();

    m_cubism_initialized = false;
    LOG_INFO("Live2DRendererGL: Cubism Framework cleaned up");
}

// ============================================================================
// OpenGL 初始化
// ============================================================================

Result<void, std::string> Live2DRendererGL::initialize_opengl() {
    LOG_INFO("Live2DRendererGL: Initializing OpenGL state");

    // 初始化 GLEW
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::string error_msg = "Failed to initialize GLEW: ";
        error_msg += reinterpret_cast<const char*>(glewGetErrorString(err));
        LOG_ERROR("Live2DRendererGL: {}", error_msg);
        return Result<void, std::string>::err(error_msg);
    }

    LOG_INFO("Live2DRendererGL: GLEW {} initialized", reinterpret_cast<const char*>(glewGetString(GLEW_VERSION)));

    // 检查 OpenGL 版本
    LOG_INFO("Live2DRendererGL: OpenGL version: {}", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    LOG_INFO("Live2DRendererGL: GLSL version: {}", reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION)));

    // 启用混合
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 禁用深度测试（2D 渲染不需要）
    glDisable(GL_DEPTH_TEST);

    // 禁用面剔除
    glDisable(GL_CULL_FACE);

    LOG_INFO("Live2DRendererGL: OpenGL state initialized");
    return Result<void, std::string>::ok();
}

// ============================================================================
// 帧缓冲对象（FBO）
// ============================================================================

Result<void, std::string> Live2DRendererGL::create_fbo() {
    LOG_INFO("Live2DRendererGL: Creating FBO with downsample factor: {}",
             m_config.fbo_downsample);

    int fbo_width = m_viewport_width / m_config.fbo_downsample;
    int fbo_height = m_viewport_height / m_config.fbo_downsample;

    // 创建纹理
    glGenTextures(1, &m_fbo_texture);
    glBindTexture(GL_TEXTURE_2D, m_fbo_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fbo_width, fbo_height,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    // 创建深度缓冲
    glGenRenderbuffers(1, &m_fbo_depth_buffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_fbo_depth_buffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16,
                         fbo_width, fbo_height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    // 创建 FBO
    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, m_fbo_texture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, m_fbo_depth_buffer);

    // 检查 FBO 状态
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        destroy_fbo();
        return Result<void, std::string>::err("FBO is not complete");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    LOG_INFO("Live2DRendererGL: FBO created successfully ({}x{})", fbo_width, fbo_height);
    return Result<void, std::string>::ok();
}

void Live2DRendererGL::destroy_fbo() {
    if (m_fbo != 0) {
        glDeleteFramebuffers(1, &m_fbo);
        m_fbo = 0;
    }
    if (m_fbo_texture != 0) {
        glDeleteTextures(1, &m_fbo_texture);
        m_fbo_texture = 0;
    }
    if (m_fbo_depth_buffer != 0) {
        glDeleteRenderbuffers(1, &m_fbo_depth_buffer);
        m_fbo_depth_buffer = 0;
    }
    LOG_INFO("Live2DRendererGL: FBO destroyed");
}

} // namespace DearTs::Plugins::Live2D
