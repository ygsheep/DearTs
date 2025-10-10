#include "window_base.h"
#include "layouts/layout_manager.h"
#include "../utils/logger.h"
#include <iostream>

namespace DearTs {
namespace Core {
namespace Window {

/**
 * WindowBase构造函数
 * 初始化窗口配置
 */
WindowBase::WindowBase(const std::string& title)
    : title_(title), layoutManager_(), windowMode_(WindowMode::STANDARD) {
    // 设置默认窗口配置
    config_.title = title_;
    config_.size = WindowSize(1280, 720);
    config_.position = WindowPosition::centered();

    // 根据窗口模式设置不同的窗口标志
    config_.flags = WindowFlags::BORDERLESS;  // 默认使用无边框窗口

    // 设置布局管理器的父窗口
    layoutManager_.setParentWindow(this);
}

/**
 * 初始化窗口
 * 使用窗口管理器创建窗口
 */
bool WindowBase::initialize() {
    DEARTS_LOG_INFO("初始化窗口基类: " + title_);
    
    // 获取窗口管理器实例
    auto& windowManager = WindowManager::getInstance();
    
    // 使用窗口管理器创建窗口
    window_ = windowManager.createWindow(config_);
    if (!window_) {
        DEARTS_LOG_ERROR("创建窗口失败: " + title_);
        return false;
    }
    
    // 设置窗口的用户数据为this指针，便于在事件处理中获取WindowBase实例
    window_->setUserData(this);

#if defined(_WIN32)
    // 根据窗口模式决定是否启用 Aero Snap 处理器
    if (windowMode_ == WindowMode::AERO_SNAP) {
        // 创建 Aero Snap 处理器，需要传递SDL_Window*而不是WindowBase*
        SDL_Window* sdlWindow = window_ ? window_->getSDLWindow() : nullptr;
        if (sdlWindow) {
            aeroSnapHandler_ = std::make_shared<AeroSnapHandler>(sdlWindow);
            if (aeroSnapHandler_) {
                if (aeroSnapHandler_->initialize()) {
                    DEARTS_LOG_INFO("Aero Snap 模式已启用");
                } else {
                    DEARTS_LOG_ERROR("Aero Snap 处理器初始化失败");
                    aeroSnapHandler_.reset();
                }
            } else {
                DEARTS_LOG_WARN("Aero Snap 处理器创建失败");
            }
        } else {
            DEARTS_LOG_ERROR("无法获取SDL窗口句柄，Aero Snap处理器创建失败");
        }
    } else {
        // 标准模式禁用 Aero Snap 处理器
        aeroSnapHandler_.reset();
        DEARTS_LOG_INFO("标准无边框窗口模式，Aero Snap处理器已禁用");
    }
#endif

    DEARTS_LOG_INFO("窗口初始化成功: " + title_);
    return true;
}

/**
 * 渲染窗口内容
 */
void WindowBase::render() {
    // 通过布局管理器渲染所有布局
    layoutManager_.renderAll();
}

/**
 * 更新窗口逻辑
 */
void WindowBase::update() {
    // 通过布局管理器更新所有布局
    if (window_) {
        auto size = window_->getSize();
        layoutManager_.updateAll(static_cast<float>(size.width), static_cast<float>(size.height));
    }
}

/**
 * 处理窗口事件
 * @param event SDL事件
 */
void WindowBase::handleEvent(const SDL_Event& event) {
    // 添加调试日志
    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        DEARTS_LOG_INFO("WindowBase::handleEvent - 鼠标左键按下事件");
    }

#if defined(_WIN32)
    // 优先使用 Aero Snap 处理器处理事件（如果存在且窗口模式为AERO_SNAP）
    if (aeroSnapHandler_ && windowMode_ == WindowMode::AERO_SNAP) {
        if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
            DEARTS_LOG_INFO("WindowBase::handleEvent - 调用AeroSnapHandler处理事件");
        }
        if (aeroSnapHandler_->handleEvent(event)) {
            // 事件已被 Aero Snap 处理器处理，不需要继续传递
            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                DEARTS_LOG_INFO("WindowBase::handleEvent - AeroSnapHandler已处理事件，停止传递");
            }
            return;
        } else {
            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                DEARTS_LOG_INFO("WindowBase::handleEvent - AeroSnapHandler未处理事件，继续传递给布局");
            }
        }
    } else {
        if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
            DEARTS_LOG_INFO("WindowBase::handleEvent - 无AeroSnapHandler或窗口不是无边框，直接传递给布局");
        }
    }
#endif

    // 通过布局管理器将事件传递给所有布局
    layoutManager_.handleEvent(event);
}

/**
 * 检查窗口是否应该关闭
 * @return 是否应该关闭
 */
bool WindowBase::shouldClose() const {
    return window_ ? window_->shouldClose() : true;
}

/**
 * 设置窗口标题
 * @param title 窗口标题
 */
void WindowBase::setTitle(const std::string& title) {
    title_ = title;
    if (window_) {
        window_->setTitle(title_);
    }
}

/**
 * 获取窗口标题
 * @return 窗口标题
 */
std::string WindowBase::getTitle() const {
    return window_ ? window_->getTitle() : title_;
}

/**
 * 显示窗口
 */
void WindowBase::show() {
    if (window_) {
        window_->show();
    }
}

/**
 * 隐藏窗口
 */
void WindowBase::hide() {
    if (window_) {
        window_->hide();
    }
}

/**
 * 最小化窗口
 */
void WindowBase::minimize() {
    if (window_) {
        window_->minimize();
    }
}

/**
 * 最大化窗口
 */
void WindowBase::maximize() {
    if (window_) {
        window_->maximize();
    }
}

/**
 * 还原窗口
 */
void WindowBase::restore() {
    if (window_) {
        window_->restore();
    }
}

/**
 * 关闭窗口
 */
void WindowBase::close() {
    if (window_) {
        window_->close();
    }
}

/**
 * 获取窗口位置
 */
WindowPosition WindowBase::getPosition() const {
    return window_ ? window_->getPosition() : WindowPosition();
}

/**
 * 设置窗口位置
 */
void WindowBase::setPosition(const WindowPosition& position) {
    if (window_) {
        window_->setPosition(position);
    }
}

/**
 * 获取窗口大小
 */
WindowSize WindowBase::getSize() const {
    return window_ ? window_->getSize() : WindowSize();
}

/**
 * 设置窗口大小
 */
void WindowBase::setSize(const WindowSize& size) {
    if (window_) {
        window_->setSize(size);
    }
}

/**
 * 设置拖拽状态
 */
void WindowBase::setDragging(bool dragging) {
    if (window_) {
        window_->setDragging(dragging);
    }
}

/**
 * 检查是否正在拖拽
 */
bool WindowBase::isDragging() const {
    return window_ ? window_->isDragging() : false;
}

/**
 * 添加布局
 */
void WindowBase::addLayout(const std::string& name, std::unique_ptr<LayoutBase> layout) {
    // 通过布局管理器添加布局
    layoutManager_.addLayout(name, std::move(layout));
}

/**
 * 获取布局
 */
LayoutBase* WindowBase::getLayout(const std::string& name) const {
    // 通过布局管理器获取布局
    return layoutManager_.getLayout(name);
}

/**
 * 移除布局
 */
void WindowBase::removeLayout(const std::string& name) {
    // 通过布局管理器移除布局
    layoutManager_.removeLayout(name);
}

/**
 * 设置窗口模式
 * @param mode 窗口模式
 */
void WindowBase::setWindowMode(WindowMode mode) {
    if (windowMode_ != mode) {
        windowMode_ = mode;

        // 记录窗口模式变化
        if (windowMode_ == WindowMode::AERO_SNAP) {
            DEARTS_LOG_INFO("窗口模式设置为 Aero Snap 模式");
            // Aero Snap 模式需要使用有边框、可调整大小的窗口
            config_.flags = WindowFlags::RESIZABLE;  // 使用有边框、可调整大小的窗口以支持 Aero Snap
        } else {
            DEARTS_LOG_INFO("窗口模式设置为标准模式");
            // 标准模式使用无边框窗口
            config_.flags = WindowFlags::BORDERLESS;
        }

#if defined(_WIN32)
        // 如果窗口已经创建，需要重新配置 Aero Snap 处理器
        if (window_) {
            if (windowMode_ == WindowMode::AERO_SNAP) {
                // 创建 Aero Snap 处理器，需要传递SDL_Window*而不是WindowBase*
                SDL_Window* sdlWindow = window_ ? window_->getSDLWindow() : nullptr;
                if (sdlWindow) {
                    aeroSnapHandler_ = std::make_shared<AeroSnapHandler>(sdlWindow);
                    if (aeroSnapHandler_) {
                        if (aeroSnapHandler_->initialize()) {
                            DEARTS_LOG_INFO("Aero Snap 处理器已启用");
                        } else {
                            DEARTS_LOG_ERROR("Aero Snap 处理器初始化失败");
                            aeroSnapHandler_.reset();
                        }
                    } else {
                        DEARTS_LOG_WARN("Aero Snap 处理器创建失败");
                    }
                } else {
                    DEARTS_LOG_ERROR("无法获取SDL窗口句柄，Aero Snap处理器创建失败");
                }
            } else {
                // 标准模式禁用 Aero Snap 处理器
                aeroSnapHandler_.reset();
                DEARTS_LOG_INFO("标准无边框窗口模式，Aero Snap处理器已禁用");
            }
        } else {
            // 窗口还未创建，Aero Snap 处理器将在 initialize() 中创建
            DEARTS_LOG_INFO("窗口尚未创建，Aero Snap 处理器将在初始化时创建");
        }
#endif
    }
}

} // namespace Window
} // namespace Core
} // namespace DearTs