#include "renderer.h"
#include <SDL.h>

namespace DearTs {
namespace Core {
namespace Render {

// IRendererToWindowRendererAdapter 实现
IRendererToWindowRendererAdapter::IRendererToWindowRendererAdapter(std::shared_ptr<IRenderer> renderer)
    : renderer_(renderer) {
}

bool IRendererToWindowRendererAdapter::initialize(SDL_Window* window) {
    if (!renderer_) {
        return false;
    }
    
    // IRenderer的initialize方法需要RendererConfig参数
    // 这里我们创建一个默认配置
    RendererConfig config;
    return true; // renderer_->initialize(window, config);
}

void IRendererToWindowRendererAdapter::shutdown() {
    if (renderer_) {
        renderer_->shutdown();
    }
}

void IRendererToWindowRendererAdapter::beginFrame() {
    if (renderer_) {
        renderer_->beginFrame();
    }
}

void IRendererToWindowRendererAdapter::endFrame() {
    if (renderer_) {
        renderer_->endFrame();
    }
}

void IRendererToWindowRendererAdapter::present() {
    if (renderer_) {
        renderer_->present();
    }
}

void IRendererToWindowRendererAdapter::clear(float r, float g, float b, float a) {
    if (renderer_) {
        // 将float参数转换为Color对象
        Color color(
            static_cast<uint8_t>(r * 255),
            static_cast<uint8_t>(g * 255),
            static_cast<uint8_t>(b * 255),
            static_cast<uint8_t>(a * 255)
        );
        renderer_->clear(color);
    }
}

void IRendererToWindowRendererAdapter::setViewport(int x, int y, int width, int height) {
    if (renderer_) {
        Rect viewport(x, y, width, height);
        renderer_->setViewport(viewport);
    }
}

std::string IRendererToWindowRendererAdapter::getType() const {
    if (renderer_) {
        return "IRendererAdapter";
    }
    return "Unknown";
}

bool IRendererToWindowRendererAdapter::isInitialized() const {
    if (renderer_) {
        // 这里需要一个方法来检查IRenderer是否已初始化
        // 由于IRenderer没有isInitialized方法，我们假设如果renderer_不为空就已初始化
        return true;
    }
    return false;
}

} // namespace Render
} // namespace Core
} // namespace DearTs