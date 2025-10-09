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
    : title_(title), layoutManager_() {
    // 设置默认窗口配置为无边框窗口
    config_.title = title_;
    config_.size = WindowSize(1280, 720);
    config_.position = WindowPosition::centered();
    config_.flags = WindowFlags::BORDERLESS;  // 使用无边框窗口

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
    // 无边框窗口不需要Aero Snap处理器，禁用以避免冲突
    aeroSnapHandler_.reset();
    DEARTS_LOG_INFO("无边框窗口模式，Aero Snap处理器已禁用");
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

} // namespace Window
} // namespace Core
} // namespace DearTs