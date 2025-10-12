#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <SDL.h>

// Forward declarations
namespace DearTs {
namespace Core {
namespace Window {
    class WindowBase;
}
}
}

namespace DearTs {
namespace Core {
namespace Window {

/**
 * @brief 布局基类
 * 提供UI布局的基础接口和功能
 */
class LayoutBase {
public:
    /**
     * @brief 构造函数
     * @param name 布局名称
     */
    explicit LayoutBase(const std::string& name);
    
    /**
     * @brief 虚析构函数
     */
    virtual ~LayoutBase() = default;
    
    /**
     * @brief 渲染布局
     */
    virtual void render() = 0;
    
    /**
     * @brief 更新布局
     * @param width 可用宽度
     * @param height 可用高度
     */
    virtual void updateLayout(float width, float height) = 0;

    /**
     * @brief 处理事件
     * @param event SDL事件
     */
    virtual void handleEvent(const SDL_Event& event) = 0;

    /**
     * @brief 渲染内容（在固定区域内）
     * 默认实现调用render()，子类可以重写以适应固定区域渲染
     * @param contentX 内容区域X坐标
     * @param contentY 内容区域Y坐标
     * @param contentWidth 内容区域宽度
     * @param contentHeight 内容区域高度
     */
    virtual void renderInFixedArea(float contentX, float contentY, float contentWidth, float contentHeight) {
        // 默认行为：调用原始render方法
        render();
    }
    
    /**
     * @brief 获取布局名称
     */
    const std::string& getName() const { return name_; }
    
    /**
     * @brief 设置父窗口
     */
    void setParentWindow(WindowBase* window) { parentWindow_ = window; }
    
    /**
     * @brief 获取父窗口
     */
    WindowBase* getParentWindow() const { return parentWindow_; }
    
    /**
     * @brief 设置是否可见
     */
    void setVisible(bool visible) { visible_ = visible; }
    
    /**
     * @brief 检查是否可见
     */
    bool isVisible() const { return visible_; }
    
    /**
     * @brief 设置布局位置
     */
    void setPosition(float x, float y) { x_ = x; y_ = y; }
    
    /**
     * @brief 设置布局大小
     */
    void setSize(float width, float height) { width_ = width; height_ = height; }
    
    /**
     * @brief 获取布局X坐标
     */
    float getX() const { return x_; }
    
    /**
     * @brief 获取布局Y坐标
     */
    float getY() const { return y_; }
    
    /**
     * @brief 获取布局宽度
     */
    float getWidth() const { return width_; }
    
    /**
     * @brief 获取布局高度
     */
    float getHeight() const { return height_; }

protected:
    std::string name_;              ///< 布局名称
    WindowBase* parentWindow_;      ///< 父窗口
    bool visible_;                  ///< 是否可见
    float x_;                       ///< X坐标
    float y_;                       ///< Y坐标
    float width_;                   ///< 宽度
    float height_;                  ///< 高度
};

} // namespace Window
} // namespace Core
} // namespace DearTs