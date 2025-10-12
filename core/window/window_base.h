#pragma once

#include "window_manager.h"
#include "layouts/layout_base.h"
#include "layouts/layout_manager.h"
#include "layouts/title_bar_layout.h"
#include "../utils/logger.h"
#include "../events/event_system.h"
#include <memory>
#include <string>
#include <functional>
#include <unordered_map>
#include <vector>

#if defined(_WIN32)
#include "win_aero_snap_handler.h"
#endif

namespace DearTs {
namespace Core {
namespace Window {

/**
 * @brief 窗口模式枚举
 */
enum class WindowMode {
    STANDARD,       ///< 标准无边框窗口（使用自定义拖拽和最大化逻辑）
    AERO_SNAP      ///< 特化无边框窗口（使用 Windows Aero Snap）
};

/**
 * @brief 窗口基类
 * 所有自定义窗口类的基类，提供通用的窗口功能
 */
class WindowBase {
public:
    /**
     * @brief 构造函数
     * @param title 窗口标题
     */
    explicit WindowBase(const std::string& title = "DearTs Window");
    
    /**
     * @brief 虚析构函数
     */
    virtual ~WindowBase() = default;
    
    /**
     * @brief 初始化窗口
     * @return 初始化是否成功
     */
    virtual bool initialize();
    
    /**
     * @brief 渲染窗口内容
     */
    virtual void render();
    
    /**
     * @brief 更新窗口逻辑
     */
    virtual void update();
    
    /**
     * @brief 处理窗口事件
     * @param event SDL事件
     */
    virtual void handleEvent(const SDL_Event& event);
    
    /**
     * @brief 获取窗口
     * @return 窗口指针
     */
    std::shared_ptr<Window> getWindow() const { return window_; }
    
    /**
     * @brief 获取SDL窗口
     * @return SDL窗口指针
     */
    SDL_Window* getSDLWindow() const { return window_ ? window_->getSDLWindow() : nullptr; }
    
    /**
     * @brief 检查窗口是否应该关闭
     * @return 是否应该关闭
     */
    bool shouldClose() const;

    /**
     * @brief 检查窗口是否可见
     * @return 是否可见
     */
    bool isVisible() const { return is_visible_; }

    /**
     * @brief 设置窗口标题
     * @param title 窗口标题
     */
    void setTitle(const std::string& title);
    
    /**
     * @brief 获取窗口标题
     * @return 窗口标题
     */
    std::string getTitle() const;
    
    /**
     * @brief 显示窗口
     */
    virtual void show();

    /**
     * @brief 隐藏窗口
     */
    virtual void hide();
    
    /**
     * @brief 最小化窗口
     */
    void minimize();
    
    /**
     * @brief 最大化窗口
     */
    void maximize();
    
    /**
     * @brief 还原窗口
     */
    void restore();
    
    /**
     * @brief 关闭窗口
     */
    void close();
    
    /**
     * @brief 获取窗口位置
     */
    WindowPosition getPosition() const;
    
    /**
     * @brief 设置窗口位置
     */
    void setPosition(const WindowPosition& position);
    
    /**
     * @brief 获取窗口大小
     */
    WindowSize getSize() const;
    
    /**
     * @brief 设置窗口大小
     */
    void setSize(const WindowSize& size);
    
    /**
     * @brief 设置拖拽状态
     */
    void setDragging(bool dragging);
    
    /**
     * @brief 检查是否正在拖拽
     */
    bool isDragging() const;
    
    /**
     * @brief 添加布局
     */
    void addLayout(const std::string& name, std::unique_ptr<LayoutBase> layout);
    
    /**
     * @brief 获取布局
     */
    LayoutBase* getLayout(const std::string& name) const;
    
    /**
     * @brief 移除布局
     */
    void removeLayout(const std::string& name);
    
    /**
     * @brief 获取布局管理器
     */
    LayoutManager& getLayoutManager() { return layoutManager_; }
    
    /**
     * @brief 获取布局管理器（const版本）
     */
    const LayoutManager& getLayoutManager() const { return layoutManager_; }

    /**
     * @brief 获取窗口ID
     * @return 窗口唯一标识符
     */
    const std::string& getWindowId() const { return windowId_; }

#if defined(_WIN32)
    /**
     * @brief 获取Aero Snap处理器
     */
    std::shared_ptr<AeroSnapHandler> getAeroSnapHandler() const { return aeroSnapHandler_; }
#endif

    /**
     * @brief 设置窗口模式
     * @param mode 窗口模式
     */
    void setWindowMode(WindowMode mode);

    /**
     * @brief 获取窗口模式
     * @return 当前窗口模式
     */
    WindowMode getWindowMode() const { return windowMode_; }

    /**
     * @brief 检查是否使用 Aero Snap 模式
     * @return 是否使用 Aero Snap
     */
    bool isAeroSnapMode() const { return windowMode_ == WindowMode::AERO_SNAP; }

    /**
     * @brief 订阅窗口事件
     * @param eventType 事件类型
     * @param handler 事件处理器
     */
    void subscribeEvent(Events::EventType eventType, std::function<bool(const Events::Event&)> handler);

    /**
     * @brief 取消订阅窗口事件
     * @param eventType 事件类型
     */
    void unsubscribeEvent(Events::EventType eventType);

    /**
     * @brief 分发窗口事件
     * @param event 事件对象
     * @return 是否被处理
     */
    bool dispatchWindowEvent(const Events::Event& event);

    /**
     * @brief 获取事件调度器
     * @return 事件调度器引用
     */
    Events::EventDispatcher& getEventDispatcher() { return eventDispatcher_; }

    /**
     * @brief 窗口显示事件通知（由Window类调用）
     */
    virtual void onWindowShown() {
        is_visible_ = true;
        DEARTS_LOG_DEBUG("WindowBase::onWindowShown - 窗口可见性设置为true: " + title_);
    }

    /**
     * @brief 窗口隐藏事件通知（由Window类调用）
     */
    virtual void onWindowHidden() {
        is_visible_ = false;
        DEARTS_LOG_DEBUG("WindowBase::onWindowHidden - 窗口可见性设置为false: " + title_);
    }

protected:
    /**
     * @brief 设置事件处理器
     */
    void setupEventHandlers();

    /**
     * @brief 注册默认布局（标题栏等）
     * 子类可以重写此方法来注册自己的默认布局
     */
    virtual void registerDefaultLayouts();

    /**
     * @brief 初始化布局系统
     */
    void initializeLayoutSystem();

    /**
     * @brief 渲染内容区域
     * 子类可以重写此方法来渲染特定的内容
     */
    virtual void renderContent();

protected:
    std::shared_ptr<Window> window_;  ///< 窗口对象
    std::string title_;               ///< 窗口标题
    std::string windowId_;            ///< 窗口唯一标识符
    WindowConfig config_;             ///< 窗口配置
    LayoutManager& layoutManager_;      ///< 布局管理器引用（单例）
    WindowMode windowMode_;           ///< 窗口模式
    Events::EventDispatcher eventDispatcher_; ///< 事件调度器
    bool is_visible_;                 ///< 窗口可见性状态（与SDL状态同步）
    std::vector<std::string> registeredLayoutIds_; ///< 已注册的布局ID列表

private:
    static std::atomic<uint32_t> s_nextWindowId_; ///< 下一个窗口ID计数器

#if defined(_WIN32)
    std::shared_ptr<AeroSnapHandler> aeroSnapHandler_;  ///< Aero Snap处理器
#endif
};

} // namespace Window
} // namespace Core
} // namespace DearTs