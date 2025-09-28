#include "renderer.h"
#include "../utils/logger.h"
#include <SDL.h>
#include <SDL_image.h>
#include <algorithm>
#include <chrono>

// ImGui includes
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>

namespace DearTs {
namespace Core {
namespace Render {

// 静态成员定义
const Color Color::C_WHITE(255, 255, 255, 255);
const Color Color::C_BLACK(0, 0, 0, 255);
const Color Color::C_RED(255, 0, 0, 255);
const Color Color::C_GREEN(0, 255, 0, 255);
const Color Color::C_BLUE(0, 0, 255, 255);
const Color Color::C_YELLOW(255, 255, 0, 255);
const Color Color::C_CYAN(0, 255, 255, 255);
const Color Color::C_MAGENTA(255, 0, 255, 255);
const Color Color::C_TRANSPARENT(0, 0, 0, 0);

// SDLTexture 实现
SDLTexture::SDLTexture(SDL_Texture* texture, const TextureInfo& info)
    : texture_(texture)
    , info_(info) {
}

SDLTexture::~SDLTexture() {
    if (texture_) {
        SDL_DestroyTexture(texture_);
        texture_ = nullptr;
    }
}

uint32_t SDLTexture::getId() const {
    return info_.id;
}

int SDLTexture::getWidth() const {
    return info_.width;
}

int SDLTexture::getHeight() const {
    return info_.height;
}

TextureFormat SDLTexture::getFormat() const {
    return info_.format;
}

TextureAccess SDLTexture::getAccess() const {
    return info_.access;
}

bool SDLTexture::updateData(const void* data, const Rect* rect) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!texture_ || !data) {
        return false;
    }
    
    SDL_Rect sdlRect;
    SDL_Rect* rectPtr = nullptr;
    
    if (rect) {
        sdlRect.x = rect->x;
        sdlRect.y = rect->y;
        sdlRect.w = rect->w;
        sdlRect.h = rect->h;
        rectPtr = &sdlRect;
    }
    
    int result = SDL_UpdateTexture(texture_, rectPtr, data, info_.width * 4); // 假设32位格式
    return result == 0;
}

void* SDLTexture::lock(const Rect* rect) {
    mutex_.lock();
    
    if (!texture_) {
        mutex_.unlock();
        return nullptr;
    }
    
    SDL_Rect sdlRect;
    SDL_Rect* rectPtr = nullptr;
    
    if (rect) {
        sdlRect.x = rect->x;
        sdlRect.y = rect->y;
        sdlRect.w = rect->w;
        sdlRect.h = rect->h;
        rectPtr = &sdlRect;
    }
    
    void* pixels;
    int pitch;
    int result = SDL_LockTexture(texture_, rectPtr, &pixels, &pitch);
    
    if (result != 0) {
        mutex_.unlock();
        return nullptr;
    }
    
    return pixels;
}

void SDLTexture::unlock() {
    if (texture_) {
        SDL_UnlockTexture(texture_);
    }
    mutex_.unlock();
}

void SDLTexture::setBlendMode(BlendMode mode) {
    if (texture_) {
        SDL_BlendMode sdlMode = SDL_BLENDMODE_NONE;
        switch (mode) {
            case BlendMode::ALPHA:
                sdlMode = SDL_BLENDMODE_BLEND;
                break;
            case BlendMode::ADDITIVE:
                sdlMode = SDL_BLENDMODE_ADD;
                break;
            case BlendMode::MODULATE:
                sdlMode = SDL_BLENDMODE_MOD;
                break;
            default:
                sdlMode = SDL_BLENDMODE_NONE;
                break;
        }
        SDL_SetTextureBlendMode(texture_, sdlMode);
    }
}

void SDLTexture::setAlphaMod(uint8_t alpha) {
    if (texture_) {
        SDL_SetTextureAlphaMod(texture_, alpha);
    }
}

void SDLTexture::setColorMod(uint8_t r, uint8_t g, uint8_t b) {
    if (texture_) {
        SDL_SetTextureColorMod(texture_, r, g, b);
    }
}

TextureInfo SDLTexture::getInfo() const {
    return info_;
}

// SDLRenderer 实现
SDLRenderer::SDLRenderer()
    : renderer_(nullptr)
    , window_(nullptr)
    , current_target_(nullptr)
    , imgui_initialized_(false)
    , next_texture_id_(1) {
}

SDLRenderer::~SDLRenderer() {
    shutdown();
}

bool SDLRenderer::initialize(SDL_Window* window, const RendererConfig& config) {
    if (!window) {
        return false;
    }
    
    window_ = window;
    config_ = config;
    
    // 创建SDL渲染器
    Uint32 flags = 0;
    if (config.type == RendererType::HARDWARE || config.type == RendererType::HARDWARE_VSYNC) {
        flags |= SDL_RENDERER_ACCELERATED;
    } else if (config.type == RendererType::SOFTWARE) {
        flags |= SDL_RENDERER_SOFTWARE;
    }
    
    if (config.type == RendererType::HARDWARE_VSYNC || config.enable_vsync) {
        flags |= SDL_RENDERER_PRESENTVSYNC;
    }
    
    renderer_ = SDL_CreateRenderer(window, -1, flags);
    if (!renderer_) {
        return false;
    }
    
    // 设置缩放质量
    switch (config.scale_quality) {
        case ScaleQuality::NEAREST:
            SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
            break;
        case ScaleQuality::LINEAR:
            SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
            break;
        case ScaleQuality::BEST:
            SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");
            break;
    }
    
    return true;
}

bool SDLRenderer::initialize(SDL_Window* window) {
    // 调用IRenderer的initialize方法，使用默认配置
    RendererConfig config;
    return initialize(window, config);
}

std::string SDLRenderer::getType() const {
    return "SDLRenderer";
}

bool SDLRenderer::isInitialized() const {
    return renderer_ != nullptr;
}

void SDLRenderer::shutdown() {
    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
    
    // 清理纹理
    textures_.clear();
    next_texture_id_ = 1;
}

void SDLRenderer::beginFrame() {
    DEARTS_LOG_DEBUG("SDLRenderer::beginFrame() called");
    frame_start_time_ = std::chrono::steady_clock::now();
    DEARTS_LOG_DEBUG("SDLRenderer::beginFrame() completed");
}

void SDLRenderer::endFrame() {
    DEARTS_LOG_DEBUG("SDLRenderer::endFrame() called");
    updateStats();
    DEARTS_LOG_DEBUG("SDLRenderer::endFrame() completed");
}

void SDLRenderer::present() {
    DEARTS_LOG_DEBUG("SDLRenderer::present() called");
    if (renderer_) {
        SDL_RenderPresent(renderer_);
        DEARTS_LOG_DEBUG("SDL_RenderPresent() called successfully");
    } else {
        DEARTS_LOG_ERROR("SDLRenderer::present() - renderer_ is null");
    }
    DEARTS_LOG_DEBUG("SDLRenderer::present() completed");
}

void SDLRenderer::clear(const Color& color) {
    DEARTS_LOG_DEBUG("SDLRenderer::clear() called with color (" + 
                     std::to_string(color.r) + ", " + 
                     std::to_string(color.g) + ", " + 
                     std::to_string(color.b) + ", " + 
                     std::to_string(color.a) + ")");
    if (renderer_) {
        SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
        SDL_RenderClear(renderer_);
        DEARTS_LOG_DEBUG("SDL_RenderClear() called successfully");
    } else {
        DEARTS_LOG_ERROR("SDLRenderer::clear() - renderer_ is null");
    }
    DEARTS_LOG_DEBUG("SDLRenderer::clear() completed");
}

void SDLRenderer::clear(float r, float g, float b, float a) {
    // 转换float参数为Color对象
    clear(Color(static_cast<uint8_t>(r * 255), static_cast<uint8_t>(g * 255), 
                static_cast<uint8_t>(b * 255), static_cast<uint8_t>(a * 255)));
}

void SDLRenderer::setViewport(const Rect& viewport) {
    if (renderer_) {
        SDL_Rect rect = { viewport.x, viewport.y, viewport.w, viewport.h };
        SDL_RenderSetViewport(renderer_, &rect);
    }
}

void SDLRenderer::setViewport(int x, int y, int width, int height) {
    if (renderer_) {
        SDL_Rect rect = { x, y, width, height };
        SDL_RenderSetViewport(renderer_, &rect);
    }
}

Rect SDLRenderer::getViewport() const {
    Rect rect = { 0, 0, 0, 0 };
    if (renderer_) {
        SDL_Rect sdlRect;
        SDL_RenderGetViewport(renderer_, &sdlRect);
        rect.x = sdlRect.x;
        rect.y = sdlRect.y;
        rect.w = sdlRect.w;
        rect.h = sdlRect.h;
    }
    return rect;
}

void SDLRenderer::setClipRect(const Rect& rect) {
    if (renderer_) {
        SDL_Rect sdlRect = { rect.x, rect.y, rect.w, rect.h };
        SDL_RenderSetClipRect(renderer_, &sdlRect);
    }
}

void SDLRenderer::clearClipRect() {
    if (renderer_) {
        SDL_RenderSetClipRect(renderer_, nullptr);
    }
}

void SDLRenderer::setDrawColor(const Color& color) {
    if (renderer_) {
        SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    }
}

Color SDLRenderer::getDrawColor() const {
    Color color;
    if (renderer_) {
        SDL_GetRenderDrawColor(renderer_, &color.r, &color.g, &color.b, &color.a);
    }
    return color;
}

void SDLRenderer::setBlendMode(BlendMode mode) {
    // 在SDL渲染器中，混合模式通常在纹理级别设置
    // 这里可以留空或者实现其他逻辑
}

BlendMode SDLRenderer::getBlendMode() const {
    // 在SDL渲染器中，混合模式通常在纹理级别设置
    return BlendMode::NONE;
}

void SDLRenderer::drawPoint(int x, int y) {
    if (renderer_) {
        SDL_RenderDrawPoint(renderer_, x, y);
    }
}

void SDLRenderer::drawPoints(const Point* points, int count) {
    if (renderer_ && points && count > 0) {
        std::vector<SDL_Point> sdlPoints(count);
        for (int i = 0; i < count; ++i) {
            sdlPoints[i].x = points[i].x;
            sdlPoints[i].y = points[i].y;
        }
        SDL_RenderDrawPoints(renderer_, sdlPoints.data(), count);
    }
}

void SDLRenderer::drawLine(int x1, int y1, int x2, int y2) {
    if (renderer_) {
        SDL_RenderDrawLine(renderer_, x1, y1, x2, y2);
    }
}

void SDLRenderer::drawLines(const Point* points, int count) {
    if (renderer_ && points && count > 1) {
        std::vector<SDL_Point> sdlPoints(count);
        for (int i = 0; i < count; ++i) {
            sdlPoints[i].x = points[i].x;
            sdlPoints[i].y = points[i].y;
        }
        SDL_RenderDrawLines(renderer_, sdlPoints.data(), count);
    }
}

void SDLRenderer::drawRect(const Rect& rect) {
    if (renderer_) {
        SDL_Rect sdlRect = { rect.x, rect.y, rect.w, rect.h };
        SDL_RenderDrawRect(renderer_, &sdlRect);
    }
}

void SDLRenderer::fillRect(const Rect& rect) {
    if (renderer_) {
        SDL_Rect sdlRect = { rect.x, rect.y, rect.w, rect.h };
        SDL_RenderFillRect(renderer_, &sdlRect);
    }
}

void SDLRenderer::drawRects(const Rect* rects, int count) {
    if (renderer_ && rects && count > 0) {
        std::vector<SDL_Rect> sdlRects(count);
        for (int i = 0; i < count; ++i) {
            sdlRects[i].x = rects[i].x;
            sdlRects[i].y = rects[i].y;
            sdlRects[i].w = rects[i].w;
            sdlRects[i].h = rects[i].h;
        }
        SDL_RenderDrawRects(renderer_, sdlRects.data(), count);
    }
}

void SDLRenderer::fillRects(const Rect* rects, int count) {
    if (renderer_ && rects && count > 0) {
        std::vector<SDL_Rect> sdlRects(count);
        for (int i = 0; i < count; ++i) {
            sdlRects[i].x = rects[i].x;
            sdlRects[i].y = rects[i].y;
            sdlRects[i].w = rects[i].w;
            sdlRects[i].h = rects[i].h;
        }
        SDL_RenderFillRects(renderer_, sdlRects.data(), count);
    }
}

void SDLRenderer::drawTexture(ITexture* texture, const Rect* src_rect, const Rect* dst_rect) {
    if (!renderer_ || !texture) {
        return;
    }
    
    SDLTexture* sdlTexture = dynamic_cast<SDLTexture*>(texture);
    if (!sdlTexture) {
        return;
    }
    
    SDL_Rect srcRect, dstRect;
    SDL_Rect* srcPtr = nullptr;
    SDL_Rect* dstPtr = nullptr;
    
    if (src_rect) {
        srcRect.x = src_rect->x;
        srcRect.y = src_rect->y;
        srcRect.w = src_rect->w;
        srcRect.h = src_rect->h;
        srcPtr = &srcRect;
    }
    
    if (dst_rect) {
        dstRect.x = dst_rect->x;
        dstRect.y = dst_rect->y;
        dstRect.w = dst_rect->w;
        dstRect.h = dst_rect->h;
        dstPtr = &dstRect;
    }
    
    SDL_RenderCopy(renderer_, sdlTexture->getSDLTexture(), srcPtr, dstPtr);
}

void SDLRenderer::drawTextureEx(ITexture* texture, const Rect* src_rect, const RectF* dst_rect, 
                               double angle, const PointF* center, FlipMode flip) {
    if (!renderer_ || !texture) {
        return;
    }
    
    SDLTexture* sdlTexture = dynamic_cast<SDLTexture*>(texture);
    if (!sdlTexture) {
        return;
    }
    
    SDL_Rect srcRect, dstRect;
    SDL_FRect dstRectF;
    SDL_FPoint centerPoint;
    SDL_Rect* srcPtr = nullptr;
    SDL_FRect* dstPtr = nullptr;
    SDL_FPoint* centerPtr = nullptr;
    
    if (src_rect) {
        srcRect.x = src_rect->x;
        srcRect.y = src_rect->y;
        srcRect.w = src_rect->w;
        srcRect.h = src_rect->h;
        srcPtr = &srcRect;
    }
    
    if (dst_rect) {
        dstRectF.x = dst_rect->x;
        dstRectF.y = dst_rect->y;
        dstRectF.w = dst_rect->w;
        dstRectF.h = dst_rect->h;
        dstPtr = &dstRectF;
    }
    
    // 简化实现，不使用center参数
    SDL_RendererFlip sdlFlip = SDL_FLIP_NONE;
    switch (flip) {
        case FlipMode::HORIZONTAL:
            sdlFlip = SDL_FLIP_HORIZONTAL;
            break;
        case FlipMode::VERTICAL:
            sdlFlip = SDL_FLIP_VERTICAL;
            break;
        case FlipMode::BOTH:
            sdlFlip = static_cast<SDL_RendererFlip>(SDL_FLIP_HORIZONTAL | SDL_FLIP_VERTICAL);
            break;
        default:
            sdlFlip = SDL_FLIP_NONE;
            break;
    }
    
    SDL_RenderCopyExF(renderer_, sdlTexture->getSDLTexture(), srcPtr, dstPtr, angle, nullptr, sdlFlip);
}

std::shared_ptr<ITexture> SDLRenderer::createTexture(int width, int height, TextureFormat format, TextureAccess access) {
    if (!renderer_) {
        return nullptr;
    }
    
    // 转换格式
    Uint32 sdlFormat = SDL_PIXELFORMAT_RGBA32; // 默认格式
    switch (format) {
        case TextureFormat::RGB24:
            sdlFormat = SDL_PIXELFORMAT_RGB24;
            break;
        case TextureFormat::RGBA32:
            sdlFormat = SDL_PIXELFORMAT_RGBA32;
            break;
        // 其他格式可以根据需要添加
    }
    
    // 转换访问模式
    int sdlAccess = SDL_TEXTUREACCESS_STATIC;
    switch (access) {
        case TextureAccess::STATIC:
            sdlAccess = SDL_TEXTUREACCESS_STATIC;
            break;
        case TextureAccess::STREAMING:
            sdlAccess = SDL_TEXTUREACCESS_STREAMING;
            break;
        case TextureAccess::TARGET:
            sdlAccess = SDL_TEXTUREACCESS_TARGET;
            break;
    }
    
    SDL_Texture* sdlTexture = SDL_CreateTexture(renderer_, sdlFormat, sdlAccess, width, height);
    if (!sdlTexture) {
        return nullptr;
    }
    
    // 创建纹理信息
    TextureInfo info;
    info.id = generateTextureId();
    info.width = width;
    info.height = height;
    info.format = format;
    info.access = access;
    info.created_time = std::chrono::steady_clock::now();
    info.last_used_time = info.created_time;
    
    auto texture = std::make_shared<SDLTexture>(sdlTexture, info);
    
    // 存储纹理引用
    {
        std::lock_guard<std::mutex> lock(textures_mutex_);
        textures_[info.id] = texture;
    }
    
    return texture;
}

std::shared_ptr<ITexture> SDLRenderer::createTextureFromSurface(SDL_Surface* surface) {
    if (!renderer_ || !surface) {
        return nullptr;
    }
    
    SDL_Texture* sdlTexture = SDL_CreateTextureFromSurface(renderer_, surface);
    if (!sdlTexture) {
        return nullptr;
    }
    
    // 创建纹理信息
    TextureInfo info;
    info.id = generateTextureId();
    info.width = surface->w;
    info.height = surface->h;
    info.format = TextureFormat::RGBA32; // 假设格式
    info.access = TextureAccess::STATIC;
    info.created_time = std::chrono::steady_clock::now();
    info.last_used_time = info.created_time;
    
    auto texture = std::make_shared<SDLTexture>(sdlTexture, info);
    
    // 存储纹理引用
    {
        std::lock_guard<std::mutex> lock(textures_mutex_);
        textures_[info.id] = texture;
    }
    
    return texture;
}

std::shared_ptr<ITexture> SDLRenderer::loadTexture(const std::string& file_path) {
    if (!renderer_ || file_path.empty()) {
        return nullptr;
    }
    
    // 加载图像表面
    SDL_Surface* surface = IMG_Load(file_path.c_str());
    if (!surface) {
        return nullptr;
    }
    
    // 创建纹理
    auto texture = createTextureFromSurface(surface);
    
    // 清理表面
    SDL_FreeSurface(surface);
    
    if (texture) {
        // 更新纹理信息
        TextureInfo info = texture->getInfo();
        info.file_path = file_path;
        // 注意：这里没有直接更新info_，因为SDLTexture中的info_是const
    }
    
    return texture;
}

void SDLRenderer::destroyTexture(std::shared_ptr<ITexture> texture) {
    if (!texture) {
        return;
    }
    
    uint32_t id = texture->getId();
    
    std::lock_guard<std::mutex> lock(textures_mutex_);
    textures_.erase(id);
}

bool SDLRenderer::setRenderTarget(ITexture* target) {
    if (!renderer_) {
        return false;
    }
    
    SDL_Texture* sdlTexture = nullptr;
    if (target) {
        SDLTexture* sdlTarget = dynamic_cast<SDLTexture*>(target);
        if (!sdlTarget) {
            return false;
        }
        sdlTexture = sdlTarget->getSDLTexture();
    }
    
    int result = SDL_SetRenderTarget(renderer_, sdlTexture);
    if (result == 0) {
        current_target_ = target;
        return true;
    }
    
    return false;
}

ITexture* SDLRenderer::getRenderTarget() const {
    return current_target_;
}

void SDLRenderer::resetRenderTarget() {
    if (renderer_) {
        SDL_SetRenderTarget(renderer_, nullptr);
        current_target_ = nullptr;
    }
}

RendererConfig SDLRenderer::getConfig() const {
    return config_;
}

RenderStats SDLRenderer::getStats() const {
    return stats_;
}

std::string SDLRenderer::getRendererInfo() const {
    if (!renderer_) {
        return "Renderer not initialized";
    }
    
    SDL_RendererInfo info;
    if (SDL_GetRendererInfo(renderer_, &info) == 0) {
        return std::string(info.name);
    }
    
    return "Unknown renderer";
}

SDL_Surface* SDLRenderer::captureScreen() {
    // 这个功能比较复杂，需要创建一个纹理作为目标，渲染到纹理，然后读取像素
    // 为了简化，这里返回nullptr
    return nullptr;
}

bool SDLRenderer::saveScreenshot(const std::string& file_path) {
    // 这个功能也比较复杂，需要先捕获屏幕，然后保存到文件
    // 为了简化，这里返回false
    return false;
}

SDL_BlendMode SDLRenderer::convertBlendMode(BlendMode mode) const {
    switch (mode) {
        case BlendMode::NONE:
            return SDL_BLENDMODE_NONE;
        case BlendMode::ALPHA:
            return SDL_BLENDMODE_BLEND;
        case BlendMode::ADDITIVE:
            return SDL_BLENDMODE_ADD;
        case BlendMode::MODULATE:
            return SDL_BLENDMODE_MOD;
        default:
            return SDL_BLENDMODE_NONE;
    }
}

BlendMode SDLRenderer::convertSDLBlendMode(SDL_BlendMode mode) const {
    switch (mode) {
        case SDL_BLENDMODE_NONE:
            return BlendMode::NONE;
        case SDL_BLENDMODE_BLEND:
            return BlendMode::ALPHA;
        case SDL_BLENDMODE_ADD:
            return BlendMode::ADDITIVE;
        case SDL_BLENDMODE_MOD:
            return BlendMode::MODULATE;
        default:
            return BlendMode::NONE;
    }
}

uint32_t SDLRenderer::convertTextureFormat(TextureFormat format) const {
    switch (format) {
        case TextureFormat::RGB24:
            return SDL_PIXELFORMAT_RGB24;
        case TextureFormat::RGBA32:
            return SDL_PIXELFORMAT_RGBA32;
        default:
            return SDL_PIXELFORMAT_RGBA32;
    }
}

TextureFormat SDLRenderer::convertSDLTextureFormat(uint32_t format) const {
    switch (format) {
        case SDL_PIXELFORMAT_RGB24:
            return TextureFormat::RGB24;
        case SDL_PIXELFORMAT_RGBA32:
            return TextureFormat::RGBA32;
        default:
            return TextureFormat::RGBA32;
    }
}

int SDLRenderer::convertTextureAccess(TextureAccess access) const {
    switch (access) {
        case TextureAccess::STATIC:
            return SDL_TEXTUREACCESS_STATIC;
        case TextureAccess::STREAMING:
            return SDL_TEXTUREACCESS_STREAMING;
        case TextureAccess::TARGET:
            return SDL_TEXTUREACCESS_TARGET;
        default:
            return SDL_TEXTUREACCESS_STATIC;
    }
}

TextureAccess SDLRenderer::convertSDLTextureAccess(int access) const {
    switch (access) {
        case SDL_TEXTUREACCESS_STATIC:
            return TextureAccess::STATIC;
        case SDL_TEXTUREACCESS_STREAMING:
            return TextureAccess::STREAMING;
        case SDL_TEXTUREACCESS_TARGET:
            return TextureAccess::TARGET;
        default:
            return TextureAccess::STATIC;
    }
}

SDL_RendererFlip SDLRenderer::convertFlipMode(FlipMode flip) const {
    switch (flip) {
        case FlipMode::NONE:
            return SDL_FLIP_NONE;
        case FlipMode::HORIZONTAL:
            return SDL_FLIP_HORIZONTAL;
        case FlipMode::VERTICAL:
            return SDL_FLIP_VERTICAL;
        case FlipMode::BOTH:
            return static_cast<SDL_RendererFlip>(SDL_FLIP_HORIZONTAL | SDL_FLIP_VERTICAL);
        default:
            return SDL_FLIP_NONE;
    }
}

void SDLRenderer::updateStats() {
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - frame_start_time_);
    stats_.frame_time = duration.count() / 1000.0; // 转换为毫秒
    stats_.frame_count++;
}

uint32_t SDLRenderer::generateTextureId() {
    return next_texture_id_++;
}

// ImGui相关方法实现
bool SDLRenderer::initializeImGui(SDL_Window* window, SDL_Renderer* renderer) {
    if (imgui_initialized_) {
        return true;
    }
    
    // 检查ImGui版本
    IMGUI_CHECKVERSION();
    
    // 创建ImGui上下文
    ImGui::CreateContext();
    
    // 配置ImGui
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    // 初始化ImGui SDL2绑定
    if (!ImGui_ImplSDL2_InitForSDLRenderer(window, renderer)) {
        DEARTS_LOG_ERROR("Failed to initialize ImGui SDL2 binding");
        return false;
    }
    
    // 初始化ImGui SDL2渲染器绑定
    if (!ImGui_ImplSDLRenderer2_Init(renderer)) {
        DEARTS_LOG_ERROR("Failed to initialize ImGui SDL2 renderer binding");
        return false;
    }
    
    imgui_initialized_ = true;
    return true;
}

void SDLRenderer::shutdownImGui() {
    if (!imgui_initialized_) {
        return;
    }
    
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    
    imgui_initialized_ = false;
}

void SDLRenderer::newImGuiFrame() {
    DEARTS_LOG_DEBUG("SDLRenderer::newImGuiFrame() - 启动ImGui帧");
    
    if (!imgui_initialized_ || !window_ || !renderer_) {
        DEARTS_LOG_ERROR("SDLRenderer::newImGuiFrame() - ImGui未初始化或窗口/渲染器无效");
        return;
    }
    
    // 开始ImGui帧
    ImGui_ImplSDL2_NewFrame();
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui::NewFrame();
    
    DEARTS_LOG_DEBUG("SDLRenderer::newImGuiFrame() - ImGui帧已启动");
}

void SDLRenderer::renderImGui(ImDrawData* draw_data) {
    DEARTS_LOG_DEBUG("SDLRenderer::renderImGui() - 渲染ImGui，draw_data: " + std::to_string(reinterpret_cast<uintptr_t>(draw_data)));
    
    if (!imgui_initialized_ || !renderer_) {
        DEARTS_LOG_ERROR("SDLRenderer::renderImGui() - ImGui未初始化或渲染器无效");
        return;
    }
    
    if (!draw_data) {
        DEARTS_LOG_WARN("SDLRenderer::renderImGui() - 无效的绘制数据");
        return;
    }
    
    // 渲染ImGui
    ImGui::Render();
    ImGui_ImplSDLRenderer2_RenderDrawData(draw_data, renderer_);
    
    DEARTS_LOG_DEBUG("SDLRenderer::renderImGui() - ImGui已渲染");
}

// RenderContext 实现
RenderContext::RenderContext() 
    : window_(nullptr)
    , initialized_(false) {
}

RenderContext::~RenderContext() {
    shutdown();
}

bool RenderContext::initialize(SDL_Window* window, const RendererConfig& config) {
    if (!window) {
        return false;
    }
    
    window_ = window;
    config_ = config;
    
    // 创建SDL渲染器
    auto sdlRenderer = std::make_shared<SDLRenderer>();
    if (!sdlRenderer->initialize(window, config)) {
        return false;
    }
    
    // 初始化ImGui
    sdlRenderer->initializeImGui(window, sdlRenderer->getSDLRenderer());
    
    renderer_ = sdlRenderer;
    initialized_ = true;
    return true;
}

void RenderContext::shutdown() {
    if (renderer_) {
        renderer_->shutdown();
        renderer_.reset();
    }
    
    window_ = nullptr;
    initialized_ = false;
}

void RenderContext::setRenderer(std::shared_ptr<IRenderer> renderer) {
    renderer_ = renderer;
}

// RenderManager 实现
RenderManager& RenderManager::getInstance() {
    static RenderManager instance;
    return instance;
}

bool RenderManager::initialize() {
    if (initialized_) {
        return true;
    }
    
    initialized_ = true;
    return true;
}

void RenderManager::shutdown() {
    if (!initialized_) {
        return;
    }
    
    // 清理所有渲染上下文
    contexts_.clear();
    current_context_.reset();
    
    initialized_ = false;
}

std::shared_ptr<RenderContext> RenderManager::createContext(SDL_Window* window, const RendererConfig& config) {
    if (!window) {
        return nullptr;
    }
    
    auto context = std::make_shared<RenderContext>();
    if (!context->initialize(window, config)) {
        return nullptr;
    }
    
    {
        std::lock_guard<std::mutex> lock(contexts_mutex_);
        contexts_.push_back(context);
    }
    
    return context;
}

void RenderManager::destroyContext(std::shared_ptr<RenderContext> context) {
    if (!context) {
        return;
    }
    
    context->shutdown();
    
    std::lock_guard<std::mutex> lock(contexts_mutex_);
    auto it = std::find(contexts_.begin(), contexts_.end(), context);
    if (it != contexts_.end()) {
        contexts_.erase(it);
    }
}

std::vector<std::shared_ptr<RenderContext>> RenderManager::getAllContexts() const {
    std::lock_guard<std::mutex> lock(contexts_mutex_);
    return contexts_;
}

size_t RenderManager::getContextCount() const {
    std::lock_guard<std::mutex> lock(contexts_mutex_);
    return contexts_.size();
}

void RenderManager::setCurrentContext(std::shared_ptr<RenderContext> context) {
    current_context_ = context;
}

void RenderManager::setGlobalConfig(const RendererConfig& config) {
    global_config_ = config;
}

RenderStats RenderManager::getGlobalStats() const {
    RenderStats stats;
    return stats;
}

void RenderManager::resetGlobalStats() {
    // 重置所有上下文的统计信息
}

void RenderManager::setScaleQuality(ScaleQuality quality) {
    // 设置全局缩放质量
}

std::vector<std::string> RenderManager::getSupportedRenderers() const {
    std::vector<std::string> renderers;
    renderers.push_back("SDL_Renderer");
    return renderers;
}

} // namespace Render
} // namespace Core
} // namespace DearTs