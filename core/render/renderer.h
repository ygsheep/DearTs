/**
 * DearTs Renderer System Header
 * 
 * 渲染系统头文件 - 提供跨平台渲染功能接口
 * 
 * @author DearTs Team
 * @version 1.0.0
 * @date 2025
 */

#pragma once

// Logger removed - using simple output instead
#include "../events/event_system.h"
#include "../window/window_manager.h"
#include <SDL.h>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <chrono>

// ImGui includes
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>

// 前向声明
struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;
struct SDL_Surface;

namespace DearTs {
namespace Core {
namespace Render {

// ============================================================================
// 枚举和常量
// ============================================================================

/**
 * @brief 渲染器类型
 */
enum class RendererType {
    SOFTWARE,           ///< 软件渲染
    HARDWARE,           ///< 硬件加速
    HARDWARE_VSYNC,     ///< 硬件加速 + 垂直同步
    AUTO                ///< 自动选择
};

/**
 * @brief 纹理格式
 */
enum class TextureFormat {
    UNKNOWN,
    RGB24,              ///< 24位RGB
    RGBA32,             ///< 32位RGBA
    ARGB32,             ///< 32位ARGB
    BGR24,              ///< 24位BGR
    BGRA32,             ///< 32位BGRA
    ABGR32,             ///< 32位ABGR
    YUV420P,            ///< YUV 4:2:0 平面格式
    YUV422,             ///< YUV 4:2:2
    UYVY,               ///< UYVY格式
    YVYU                ///< YVYU格式
};

/**
 * @brief 纹理访问模式
 */
enum class TextureAccess {
    STATIC,             ///< 静态纹理，很少更新
    STREAMING,          ///< 流式纹理，经常更新
    TARGET              ///< 渲染目标纹理
};

/**
 * @brief 混合模式
 */
enum class BlendMode {
    NONE,               ///< 无混合
    ALPHA,              ///< Alpha混合
    ADDITIVE,           ///< 加法混合
    MODULATE,           ///< 调制混合
    MULTIPLY,           ///< 乘法混合
    CUSTOM              ///< 自定义混合
};

/**
 * @brief 缩放质量
 */
enum class ScaleQuality {
    NEAREST,            ///< 最近邻
    LINEAR,             ///< 线性插值
    BEST                ///< 最佳质量
};

/**
 * @brief 渲染翻转模式
 */
enum class FlipMode {
    NONE,               ///< 不翻转
    HORIZONTAL,         ///< 水平翻转
    VERTICAL,           ///< 垂直翻转
    BOTH                ///< 水平和垂直翻转
};

// ============================================================================
// 结构体
// ============================================================================

/**
 * @brief 颜色结构
 */
struct Color {
    uint8_t r, g, b, a;
    
    Color() : r(0), g(0), b(0), a(255) {}
    Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255)
        : r(red), g(green), b(blue), a(alpha) {}
    
    // 预定义颜色
    static const Color C_WHITE;
    static const Color C_BLACK;
    static const Color C_RED;
    static const Color C_GREEN;
    static const Color C_BLUE;
    static const Color C_YELLOW;
    static const Color C_CYAN;
    static const Color C_MAGENTA;
    static const Color C_TRANSPARENT;
};

/**
 * @brief 点结构
 */
struct Point {
    int x, y;
    
    Point() : x(0), y(0) {}
    Point(int x_val, int y_val) : x(x_val), y(y_val) {}
};

/**
 * @brief 浮点点结构
 */
struct PointF {
    float x, y;
    
    PointF() : x(0.0f), y(0.0f) {}
    PointF(float x_val, float y_val) : x(x_val), y(y_val) {}
};

/**
 * @brief 矩形结构
 */
struct Rect {
    int x, y, w, h;
    
    Rect() : x(0), y(0), w(0), h(0) {}
    Rect(int x_val, int y_val, int width, int height)
        : x(x_val), y(y_val), w(width), h(height) {}
    
    bool contains(int px, int py) const {
        return px >= x && px < x + w && py >= y && py < y + h;
    }
    
    bool contains(const Point& point) const {
        return contains(point.x, point.y);
    }
    
    bool intersects(const Rect& other) const {
        return !(x >= other.x + other.w || other.x >= x + w ||
                 y >= other.y + other.h || other.y >= y + h);
    }
};

/**
 * @brief 浮点矩形结构
 */
struct RectF {
    float x, y, w, h;
    
    RectF() : x(0.0f), y(0.0f), w(0.0f), h(0.0f) {}
    RectF(float x_val, float y_val, float width, float height)
        : x(x_val), y(y_val), w(width), h(height) {}
    
    bool contains(float px, float py) const {
        return px >= x && px < x + w && py >= y && py < y + h;
    }
    
    bool contains(const PointF& point) const {
        return contains(point.x, point.y);
    }
    
    bool intersects(const RectF& other) const {
        return !(x >= other.x + other.w || other.x >= x + w ||
                 y >= other.y + other.h || other.y >= y + h);
    }
};

/**
 * @brief 渲染器配置
 */
struct RendererConfig {
    RendererType type;
    bool enable_vsync;
    ScaleQuality scale_quality;
    Color clear_color;
    bool enable_batching;
    size_t max_batch_size;
    bool enable_culling;
    bool enable_depth_test;
    std::string shader_path;
    
    RendererConfig()
        : type(RendererType::AUTO)
        , enable_vsync(true)
        , scale_quality(ScaleQuality::LINEAR)
        , clear_color(Color(0, 0, 0, 255))  // BLACK
        , enable_batching(true)
        , max_batch_size(1000)
        , enable_culling(true)
        , enable_depth_test(false) {
    }
};

/**
 * @brief 纹理信息
 */
struct TextureInfo {
    uint32_t id;
    int width;
    int height;
    TextureFormat format;
    TextureAccess access;
    std::string file_path;
    size_t memory_size;
    std::chrono::steady_clock::time_point created_time;
    std::chrono::steady_clock::time_point last_used_time;
    
    TextureInfo()
        : id(0)
        , width(0)
        , height(0)
        , format(TextureFormat::UNKNOWN)
        , access(TextureAccess::STATIC)
        , memory_size(0) {
    }
};

/**
 * @brief 渲染统计信息
 */
struct RenderStats {
    uint64_t frame_count;
    uint64_t draw_calls;
    uint64_t vertices_rendered;
    uint64_t triangles_rendered;
    uint64_t textures_bound;
    uint64_t state_changes;
    double frame_time;
    double cpu_time;
    double gpu_time;
    size_t texture_memory;
    size_t vertex_buffer_memory;
    size_t total_memory;
    
    RenderStats()
        : frame_count(0)
        , draw_calls(0)
        , vertices_rendered(0)
        , triangles_rendered(0)
        , textures_bound(0)
        , state_changes(0)
        , frame_time(0.0)
        , cpu_time(0.0)
        , gpu_time(0.0)
        , texture_memory(0)
        , vertex_buffer_memory(0)
        , total_memory(0) {
    }
    
    void reset() {
        draw_calls = 0;
        vertices_rendered = 0;
        triangles_rendered = 0;
        textures_bound = 0;
        state_changes = 0;
        frame_time = 0.0;
        cpu_time = 0.0;
        gpu_time = 0.0;
    }
};

// ============================================================================
// 前向声明
// ============================================================================

class ITexture;
class IRenderer;
class RenderContext;
class RenderManager;

// ============================================================================
// 纹理接口
// ============================================================================

/**
 * @brief 纹理接口
 */
class ITexture {
public:
    virtual ~ITexture() = default;
    
    /**
     * @brief 获取纹理ID
     */
    virtual uint32_t getId() const = 0;
    
    /**
     * @brief 获取纹理宽度
     */
    virtual int getWidth() const = 0;
    
    /**
     * @brief 获取纹理高度
     */
    virtual int getHeight() const = 0;
    
    /**
     * @brief 获取纹理格式
     */
    virtual TextureFormat getFormat() const = 0;
    
    /**
     * @brief 获取纹理访问模式
     */
    virtual TextureAccess getAccess() const = 0;
    
    /**
     * @brief 更新纹理数据
     */
    virtual bool updateData(const void* data, const Rect* rect = nullptr) = 0;
    
    /**
     * @brief 锁定纹理进行直接访问
     */
    virtual void* lock(const Rect* rect = nullptr) = 0;
    
    /**
     * @brief 解锁纹理
     */
    virtual void unlock() = 0;
    
    /**
     * @brief 设置混合模式
     */
    virtual void setBlendMode(BlendMode mode) = 0;
    
    /**
     * @brief 设置Alpha调制
     */
    virtual void setAlphaMod(uint8_t alpha) = 0;
    
    /**
     * @brief 设置颜色调制
     */
    virtual void setColorMod(uint8_t r, uint8_t g, uint8_t b) = 0;
    
    /**
     * @brief 获取纹理信息
     */
    virtual TextureInfo getInfo() const = 0;
};

// ============================================================================
// 渲染器接口
// ============================================================================

/**
 * @brief 渲染器接口
 */
class IRenderer {
public:
    virtual ~IRenderer() = default;
    
    // 初始化和清理
    virtual bool initialize(SDL_Window* window, const RendererConfig& config) = 0;
    virtual void shutdown() = 0;
    
    // 渲染控制
    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;
    virtual void present() = 0;
    virtual void clear(const Color& color = Color::C_BLACK) = 0;
    
    // 视口和裁剪
    virtual void setViewport(const Rect& viewport) = 0;
    virtual Rect getViewport() const = 0;
    virtual void setClipRect(const Rect& rect) = 0;
    virtual void clearClipRect() = 0;
    
    // 渲染状态
    virtual void setDrawColor(const Color& color) = 0;
    virtual Color getDrawColor() const = 0;
    virtual void setBlendMode(BlendMode mode) = 0;
    virtual BlendMode getBlendMode() const = 0;
    
    // 基本绘制
    virtual void drawPoint(int x, int y) = 0;
    virtual void drawPoints(const Point* points, int count) = 0;
    virtual void drawLine(int x1, int y1, int x2, int y2) = 0;
    virtual void drawLines(const Point* points, int count) = 0;
    virtual void drawRect(const Rect& rect) = 0;
    virtual void fillRect(const Rect& rect) = 0;
    virtual void drawRects(const Rect* rects, int count) = 0;
    virtual void fillRects(const Rect* rects, int count) = 0;
    
    // 纹理渲染
    virtual void drawTexture(ITexture* texture, const Rect* src_rect = nullptr, const Rect* dst_rect = nullptr) = 0;
    virtual void drawTextureEx(ITexture* texture, const Rect* src_rect, const RectF* dst_rect, 
                              double angle, const PointF* center, FlipMode flip) = 0;
    
    // 纹理管理
    virtual std::shared_ptr<ITexture> createTexture(int width, int height, TextureFormat format, TextureAccess access) = 0;
    virtual std::shared_ptr<ITexture> createTextureFromSurface(SDL_Surface* surface) = 0;
    virtual std::shared_ptr<ITexture> loadTexture(const std::string& file_path) = 0;
    virtual void destroyTexture(std::shared_ptr<ITexture> texture) = 0;
    
    // 渲染目标
    virtual bool setRenderTarget(ITexture* target) = 0;
    virtual ITexture* getRenderTarget() const = 0;
    virtual void resetRenderTarget() = 0;
    
    // 信息获取
    virtual RendererConfig getConfig() const = 0;
    virtual RenderStats getStats() const = 0;
    virtual std::string getRendererInfo() const = 0;
    
    // 截图
    virtual SDL_Surface* captureScreen() = 0;
    virtual bool saveScreenshot(const std::string& file_path) = 0;
};

// ============================================================================
// SDL渲染器实现
// ============================================================================

/**
 * @brief SDL纹理实现
 */
class SDLTexture : public ITexture {
public:
    SDLTexture(SDL_Texture* texture, const TextureInfo& info);
    virtual ~SDLTexture();
    
    // ITexture接口实现
    uint32_t getId() const override;
    int getWidth() const override;
    int getHeight() const override;
    TextureFormat getFormat() const override;
    TextureAccess getAccess() const override;
    bool updateData(const void* data, const Rect* rect = nullptr) override;
    void* lock(const Rect* rect = nullptr) override;
    void unlock() override;
    void setBlendMode(BlendMode mode) override;
    void setAlphaMod(uint8_t alpha) override;
    void setColorMod(uint8_t r, uint8_t g, uint8_t b) override;
    TextureInfo getInfo() const override;
    
    // SDL特定方法
    SDL_Texture* getSDLTexture() const { return texture_; }
    
private:
    SDL_Texture* texture_;
    TextureInfo info_;
    mutable std::mutex mutex_;
};

/**
 * @brief SDL渲染器实现
 */
class SDLRenderer : public IRenderer, public ::DearTs::Core::Window::WindowRenderer {
public:
    SDLRenderer();
    virtual ~SDLRenderer();
    
    // IRenderer接口实现
    bool initialize(SDL_Window* window, const RendererConfig& config) override;
    
    void shutdown() override;
    void beginFrame() override;
    void endFrame() override;
    void present() override;
    void clear(const Color& color = Color::C_BLACK) override;
    
    void setViewport(const Rect& viewport) override;
    Rect getViewport() const override;
    void setClipRect(const Rect& rect) override;
    void clearClipRect() override;
    
    void setDrawColor(const Color& color) override;
    Color getDrawColor() const override;
    void setBlendMode(BlendMode mode) override;
    BlendMode getBlendMode() const override;
    
    void drawPoint(int x, int y) override;
    void drawPoints(const Point* points, int count) override;
    void drawLine(int x1, int y1, int x2, int y2) override;
    void drawLines(const Point* points, int count) override;
    void drawRect(const Rect& rect) override;
    void fillRect(const Rect& rect) override;
    void drawRects(const Rect* rects, int count) override;
    void fillRects(const Rect* rects, int count) override;
    
    void drawTexture(ITexture* texture, const Rect* src_rect = nullptr, const Rect* dst_rect = nullptr) override;
    void drawTextureEx(ITexture* texture, const Rect* src_rect, const RectF* dst_rect, 
                      double angle, const PointF* center, FlipMode flip) override;
    
    std::shared_ptr<ITexture> createTexture(int width, int height, TextureFormat format, TextureAccess access) override;
    std::shared_ptr<ITexture> createTextureFromSurface(SDL_Surface* surface) override;
    std::shared_ptr<ITexture> loadTexture(const std::string& file_path) override;
    void destroyTexture(std::shared_ptr<ITexture> texture) override;
    
    bool setRenderTarget(ITexture* target) override;
    ITexture* getRenderTarget() const override;
    void resetRenderTarget() override;
    
    RendererConfig getConfig() const override;
    RenderStats getStats() const override;
    std::string getRendererInfo() const override;
    
    SDL_Surface* captureScreen() override;
    bool saveScreenshot(const std::string& file_path) override;
    
    // WindowRenderer接口实现
    bool initialize(SDL_Window* window) override;
    // 注意：shutdown、beginFrame、endFrame、present方法已经在IRenderer接口中实现
    void clear(float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 1.0f) override;
    void setViewport(int x, int y, int width, int height) override;
    std::string getType() const override;
    bool isInitialized() const override;
    
    // ImGui相关方法实现
    bool initializeImGui(SDL_Window* window, SDL_Renderer* renderer);
    void shutdownImGui();
    void newImGuiFrame();
    void renderImGui(ImDrawData* draw_data);
    
    // SDL特定方法
    SDL_Renderer* getSDLRenderer() const { return renderer_; }
    
private:
    SDL_Renderer* renderer_;
    SDL_Window* window_;
    RendererConfig config_;
    RenderStats stats_;
    ITexture* current_target_;
    
    // ImGui相关成员变量
    bool imgui_initialized_;
    
    std::unordered_map<uint32_t, std::shared_ptr<ITexture>> textures_;
    uint32_t next_texture_id_;
    mutable std::mutex textures_mutex_;
    
    std::chrono::steady_clock::time_point frame_start_time_;
    
    // 辅助方法
    SDL_BlendMode convertBlendMode(BlendMode mode) const;
    BlendMode convertSDLBlendMode(SDL_BlendMode mode) const;
    uint32_t convertTextureFormat(TextureFormat format) const;
    TextureFormat convertSDLTextureFormat(uint32_t format) const;
    int convertTextureAccess(TextureAccess access) const;
    TextureAccess convertSDLTextureAccess(int access) const;
    SDL_RendererFlip convertFlipMode(FlipMode flip) const;
    
    void updateStats();
    uint32_t generateTextureId();
    
    // ImGui相关方法
    bool initializeImGuiInternal();
    void shutdownImGuiInternal();
};

// ============================================================================
// IRenderer到WindowRenderer的适配器
// ============================================================================

/**
 * @brief IRenderer到WindowRenderer的适配器
 */
class IRendererToWindowRendererAdapter : public ::DearTs::Core::Window::WindowRenderer {
public:
    explicit IRendererToWindowRendererAdapter(std::shared_ptr<IRenderer> renderer);
    virtual ~IRendererToWindowRendererAdapter() = default;
    
    // WindowRenderer接口实现
    bool initialize(SDL_Window* window) override;
    void shutdown() override;
    void beginFrame() override;
    void endFrame() override;
    void present() override;
    void clear(float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 1.0f) override;
    void setViewport(int x, int y, int width, int height) override;
    std::string getType() const override;
    bool isInitialized() const override;
    
    // 获取底层渲染器
    std::shared_ptr<IRenderer> getRenderer() const { return renderer_; }
    
private:
    std::shared_ptr<IRenderer> renderer_;
};

// ============================================================================
// 渲染上下文
// ============================================================================

// ============================================================================
// 渲染上下文
// ============================================================================

/**
 * @brief 渲染上下文
 */
class RenderContext {
public:
    RenderContext();
    ~RenderContext();
    
    /**
     * @brief 初始化渲染上下文
     */
    bool initialize(SDL_Window* window, const RendererConfig& config = RendererConfig());
    
    /**
     * @brief 关闭渲染上下文
     */
    void shutdown();
    
    /**
     * @brief 获取渲染器
     */
    std::shared_ptr<IRenderer> getRenderer() const { return renderer_; }
    
    /**
     * @brief 设置渲染器
     */
    void setRenderer(std::shared_ptr<IRenderer> renderer);
    
    /**
     * @brief 获取窗口
     */
    SDL_Window* getWindow() const { return window_; }
    
    /**
     * @brief 获取配置
     */
    const RendererConfig& getConfig() const { return config_; }
    
    /**
     * @brief 更新配置
     */
    void updateConfig(const RendererConfig& config);
    
    /**
     * @brief 检查是否已初始化
     */
    bool isInitialized() const { return initialized_; }
    
private:
    std::shared_ptr<IRenderer> renderer_;
    SDL_Window* window_;
    RendererConfig config_;
    bool initialized_;
};

// ============================================================================
// 渲染管理器
// ============================================================================

/**
 * @brief 渲染管理器（单例）
 */
class RenderManager {
public:
    /**
     * @brief 获取单例实例
     */
    static RenderManager& getInstance();
    
    /**
     * @brief 初始化渲染管理器
     */
    bool initialize();
    
    /**
     * @brief 关闭渲染管理器
     */
    void shutdown();
    
    /**
     * @brief 创建渲染上下文
     */
    std::shared_ptr<RenderContext> createContext(SDL_Window* window, const RendererConfig& config = RendererConfig());
    
    /**
     * @brief 销毁渲染上下文
     */
    void destroyContext(std::shared_ptr<RenderContext> context);
    
    /**
     * @brief 获取当前渲染上下文
     */
    std::shared_ptr<RenderContext> getCurrentContext() const { return current_context_; }
    
    /**
     * @brief 设置当前渲染上下文
     */
    void setCurrentContext(std::shared_ptr<RenderContext> context);
    
    /**
     * @brief 获取所有渲染上下文
     */
    std::vector<std::shared_ptr<RenderContext>> getAllContexts() const;
    
    /**
     * @brief 获取渲染上下文数量
     */
    size_t getContextCount() const;
    
    /**
     * @brief 设置全局渲染配置
     */
    void setGlobalConfig(const RendererConfig& config);
    
    /**
     * @brief 获取全局渲染配置
     */
    const RendererConfig& getGlobalConfig() const { return global_config_; }
    
    /**
     * @brief 获取全局渲染统计
     */
    RenderStats getGlobalStats() const;
    
    /**
     * @brief 重置全局渲染统计
     */
    void resetGlobalStats();
    
    /**
     * @brief 设置缩放质量
     */
    void setScaleQuality(ScaleQuality quality);
    
    /**
     * @brief 获取支持的渲染器信息
     */
    std::vector<std::string> getSupportedRenderers() const;
    
    /**
     * @brief 检查是否已初始化
     */
    bool isInitialized() const { return initialized_; }
    
private:
    RenderManager() = default;
    ~RenderManager() = default;
    RenderManager(const RenderManager&) = delete;
    RenderManager& operator=(const RenderManager&) = delete;
    
    std::vector<std::shared_ptr<RenderContext>> contexts_;
    std::shared_ptr<RenderContext> current_context_;
    RendererConfig global_config_;
    mutable std::mutex contexts_mutex_;
    bool initialized_ = false;
};

// ============================================================================
// 便利宏
#define DEARTS_RENDER_MANAGER() DearTs::Core::Render::RenderManager::getInstance()
#define DEARTS_CURRENT_RENDERER() DEARTS_RENDER_MANAGER().getCurrentContext()->getRenderer()

} // namespace Render
} // namespace Core
} // namespace DearTs