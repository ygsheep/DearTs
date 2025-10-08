#pragma once

#include "window_manager.h"
#include "layouts/layout_base.h"
#include "layouts/layout_manager.h"
#include "../utils/logger.h"
#include <memory>
#include <string>

namespace DearTs {
namespace Core {
namespace Window {

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
    void show();
    
    /**
     * @brief 隐藏窗口
     */
    void hide();
    
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
    
protected:
    std::shared_ptr<Window> window_;  ///< 窗口对象
    std::string title_;               ///< 窗口标题
    WindowConfig config_;             ///< 窗口配置
    LayoutManager layoutManager_;     ///< 布局管理器
};

} // namespace Window
} // namespace Core
} // namespace DearTs