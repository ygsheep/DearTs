#include "window_base.h"
#include "layouts/layout_manager.h"
#include "../utils/logger.h"
#include <iostream>

namespace DearTs {
namespace Core {
namespace Window {

// 静态成员变量定义
std::atomic<uint32_t> WindowBase::s_nextWindowId_{1};

/**
 * WindowBase构造函数
 * 初始化窗口配置
 */
WindowBase::WindowBase(const std::string& title)
    : title_(title), windowId_("Window_" + std::to_string(s_nextWindowId_++)), layoutManager_(LayoutManager::getInstance()), windowMode_(WindowMode::STANDARD), is_visible_(false) {
    // 设置默认窗口配置
    config_.title = title_;
    config_.size = WindowSize(1280, 720);
    config_.position = WindowPosition::centered();

    // 根据窗口模式设置不同的窗口标志
    config_.flags = WindowFlags::BORDERLESS;  // 默认使用无边框窗口

    // 设置布局管理器的父窗口
    layoutManager_.setParentWindow(this);

    // 初始化事件系统
    setupEventHandlers();
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

    // 分发窗口创建事件
    class WindowCreatedEvent : public Events::Event {
    public:
        WindowCreatedEvent() : Event(Events::EventType::EVT_WINDOW_CREATED) {}
        std::string getName() const override { return "WindowCreated"; }
    };
    WindowCreatedEvent windowCreatedEvent;
    dispatchWindowEvent(windowCreatedEvent);

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

    // 初始化布局系统
    initializeLayoutSystem();

    DEARTS_LOG_INFO("窗口初始化成功: " + title_);
    return true;
}

/**
 * 渲染窗口内容
 */
void WindowBase::render() {
    // 通过布局管理器渲染当前窗口的所有布局
    layoutManager_.renderAll(windowId_);

    // 渲染内容区域（子类可重写）
    renderContent();
}

/**
 * 更新窗口逻辑
 */
void WindowBase::update() {
    // 通过布局管理器更新当前窗口的所有布局
    if (window_) {
        auto size = window_->getSize();
        layoutManager_.updateAll(static_cast<float>(size.width), static_cast<float>(size.height), windowId_);
    }
}

/**
 * 处理窗口事件
 * @param event SDL事件
 */
void WindowBase::handleEvent(const SDL_Event& event) {
  
#if defined(_WIN32)
    // 优先使用 Aero Snap 处理器处理事件（如果存在且窗口模式为AERO_SNAP）
    if (aeroSnapHandler_ && windowMode_ == WindowMode::AERO_SNAP) {
        if (aeroSnapHandler_->handleEvent(event)) {
            // 事件已被 Aero Snap 处理器处理，不需要继续传递
            return;
        }
    }
#endif

    // 将SDL事件转换为系统事件并分发
    // 注意：这里仍然需要将SDL事件传递给布局管理器，因为布局组件需要处理SDL输入事件
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
        // is_visible_ 将在 onWindowShown() 中设置
        DEARTS_LOG_DEBUG("WindowBase::show() - 请求显示窗口: " + title_);
    }
}

/**
 * 隐藏窗口
 */
void WindowBase::hide() {
    if (window_) {
        window_->hide();
        // is_visible_ 将在 onWindowHidden() 中设置
        DEARTS_LOG_DEBUG("WindowBase::hide() - 请求隐藏窗口: " + title_);
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
    // 通过布局管理器添加布局到当前窗口
    layoutManager_.addLayout(name, std::move(layout), windowId_);
}

/**
 * 获取布局
 */
LayoutBase* WindowBase::getLayout(const std::string& name) const {
    // 通过布局管理器获取当前窗口的布局
    return layoutManager_.getWindowLayout(windowId_, name);
}

/**
 * 移除布局
 */
void WindowBase::removeLayout(const std::string& name) {
    // 通过布局管理器移除当前窗口的布局
    layoutManager_.removeLayout(name);
    // Note: removeLayout doesn't need windowId parameter as it removes globally
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

/**
 * @brief 设置事件处理器
 */
void WindowBase::setupEventHandlers() {
    // 订阅窗口相关事件
    subscribeEvent(Events::EventType::EVT_WINDOW_CREATED, [this](const Events::Event& event) {
        DEARTS_LOG_INFO("窗口创建事件: " + title_);
        // 窗口创建后初始化布局管理器事件系统
        layoutManager_.initializeEventSystem();
        return true;
    });

    subscribeEvent(Events::EventType::EVT_WINDOW_RESIZED, [this](const Events::Event& event) {
        DEARTS_LOG_DEBUG("窗口大小改变事件: " + title_);
        // 窗口大小改变时更新布局
        if (window_) {
            auto size = window_->getSize();
            layoutManager_.updateAll(static_cast<float>(size.width), static_cast<float>(size.height), windowId_);
        }
        return true;
    });

    subscribeEvent(Events::EventType::EVT_WINDOW_FOCUS_GAINED, [this](const Events::Event& event) {
        DEARTS_LOG_DEBUG("窗口获得焦点: " + title_);
        return true;
    });

    subscribeEvent(Events::EventType::EVT_WINDOW_FOCUS_LOST, [this](const Events::Event& event) {
        DEARTS_LOG_DEBUG("窗口失去焦点: " + title_);
        return true;
    });

    // 订阅布局事件
    subscribeEvent(Events::EventType::EVT_LAYOUT_SHOW_REQUEST, [this](const Events::Event& event) {
        DEARTS_LOG_DEBUG("布局显示请求事件: " + title_);
        return true;
    });

    subscribeEvent(Events::EventType::EVT_LAYOUT_HIDE_REQUEST, [this](const Events::Event& event) {
        DEARTS_LOG_DEBUG("布局隐藏请求事件: " + title_);
        return true;
    });

    subscribeEvent(Events::EventType::EVT_LAYOUT_SWITCH_REQUEST, [this](const Events::Event& event) {
        DEARTS_LOG_DEBUG("布局切换请求事件: " + title_);
        return true;
    });
}

/**
 * @brief 订阅窗口事件
 * @param eventType 事件类型
 * @param handler 事件处理器
 */
void WindowBase::subscribeEvent(Events::EventType eventType, std::function<bool(const Events::Event&)> handler) {
    eventDispatcher_.subscribe(eventType, handler);
    DEARTS_LOG_DEBUG("订阅事件: " + std::to_string(static_cast<uint32_t>(eventType)) + " for window: " + title_);
}

/**
 * @brief 取消订阅窗口事件
 * @param eventType 事件类型
 */
void WindowBase::unsubscribeEvent(Events::EventType eventType) {
    eventDispatcher_.unsubscribe(eventType);
    DEARTS_LOG_DEBUG("取消订阅事件: " + std::to_string(static_cast<uint32_t>(eventType)) + " for window: " + title_);
}

/**
 * @brief 分发窗口事件
 * @param event 事件对象
 * @return 是否被处理
 */
bool WindowBase::dispatchWindowEvent(const Events::Event& event) {
    bool handled = eventDispatcher_.dispatch(event);

    // 注意：系统事件不直接传递给布局管理器
    // 布局管理器只处理SDL事件，在handleEvent方法中处理

    return handled;
}

/**
 * @brief 初始化布局系统
 */
void WindowBase::initializeLayoutSystem() {
    DEARTS_LOG_INFO("初始化布局系统: " + title_);

    // 注册当前窗口上下文
    layoutManager_.registerWindowContext(windowId_, this);

    // 初始化布局管理器的事件系统
    layoutManager_.initializeEventSystem();

    // 注册默认布局
    registerDefaultLayouts();

    DEARTS_LOG_INFO("布局系统初始化完成: " + title_);
}

/**
 * @brief 注册默认布局（标题栏等）
 * 子类可以重写此方法来注册自己的默认布局
 */
void WindowBase::registerDefaultLayouts() {
    DEARTS_LOG_INFO("注册默认布局: " + title_);

    // 首先设置当前窗口为活跃窗口，确保后续创建的布局使用正确的窗口ID
    layoutManager_.setActiveWindow(windowId_);
    DEARTS_LOG_INFO("设置活跃窗口为: " + windowId_ + " (注册默认布局)");

    // 注册标题栏布局
    LayoutRegistration titleBarReg("TitleBar", LayoutType::SYSTEM, LayoutPriority::HIGHEST);
    titleBarReg.factory = [this]() -> std::unique_ptr<LayoutBase> {
        auto titleBar = std::make_unique<TitleBarLayout>();
        titleBar->setWindowTitle(title_);
        return titleBar;
    };
    titleBarReg.autoCreate = true;
    titleBarReg.persistent = false;

    if (layoutManager_.registerLayout(titleBarReg)) {
        DEARTS_LOG_INFO("标题栏布局注册成功: " + title_);
        // 不需要再次调用createRegisteredLayout，因为autoCreate=true会自动创建
        DEARTS_LOG_INFO("标题栏布局通过autoCreate自动创建完成");
    } else {
        DEARTS_LOG_ERROR("标题栏布局注册失败: " + title_);
    }
}

/**
 * @brief 渲染内容区域
 * 子类可以重写此方法来渲染特定的内容
 */
void WindowBase::renderContent() {
    // 默认实现：渲染内容区域
    const float titleBarHeight = 30.0f;
    const float contentX = 0.0f;
    const float contentY = titleBarHeight;
    const float contentWidth = ImGui::GetIO().DisplaySize.x;
    const float contentHeight = ImGui::GetIO().DisplaySize.y - titleBarHeight;

    // 获取当前内容布局
    std::string currentLayout = layoutManager_.getCurrentContentLayout();

    if (!currentLayout.empty()) {
        LayoutBase* layout = layoutManager_.getWindowLayout(windowId_, currentLayout);
        if (layout && layout->isVisible()) {
            // 创建固定的内容区域窗口
            ImGui::SetNextWindowPos(ImVec2(contentX, contentY));
            ImGui::SetNextWindowSize(ImVec2(contentWidth, contentHeight));

            ImGuiWindowFlags contentFlags = ImGuiWindowFlags_NoTitleBar |
                                           ImGuiWindowFlags_NoResize |
                                           ImGuiWindowFlags_NoMove |
                                           ImGuiWindowFlags_NoCollapse |
                                           ImGuiWindowFlags_NoBringToFrontOnFocus;

            // 设置内容区域背景色
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.082f, 0.082f, 0.082f, 1.0f));

            if (ImGui::Begin("##ContentArea", nullptr, contentFlags)) {
                // 调用布局的固定区域渲染方法
                layout->renderInFixedArea(contentX, contentY, contentWidth, contentHeight);
            }
            ImGui::End();

            ImGui::PopStyleColor();
        }
    } else {
        // 渲染默认内容
        ImGui::SetNextWindowPos(ImVec2(contentX, contentY));
        ImGui::SetNextWindowSize(ImVec2(contentWidth, contentHeight));

        ImGuiWindowFlags defaultFlags = ImGuiWindowFlags_NoTitleBar |
                                          ImGuiWindowFlags_NoResize |
                                          ImGuiWindowFlags_NoMove |
                                          ImGuiWindowFlags_NoCollapse |
                                          ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.082f, 0.082f, 0.082f, 1.0f));

        if (ImGui::Begin("##DefaultContent", nullptr, defaultFlags)) {
            ImGui::Text("默认内容区域");
            ImGui::Text("窗口: %s", title_.c_str());
        }

        ImGui::End();
        ImGui::PopStyleColor();
    }
}

} // namespace Window
} // namespace Core
} // namespace DearTs