#pragma once

#include "layout_base.h"
#include "../../resource/vscode_icons.hpp"
#include <string>
#include <vector>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <functional>

namespace DearTs {
namespace Core {
namespace Window {

/**
 * @brief 侧边栏项目结构
 */
struct SidebarItem {
    std::string id;           ///< 项目ID
    std::string icon;         ///< 图标字符（如Codicons字体中的字符）
    std::string text;         ///< 显示文本
    bool isActive;            ///< 是否为激活状态
    std::string tooltip;      ///< 提示文本
    std::string iconPath;     ///< 图标文件路径（如果使用图片）
    bool isExpandable;        ///< 是否为可展开项
    bool isExpanded;          ///< 是否已展开（仅对可展开项有效）
    std::vector<SidebarItem> children; ///< 子项目（仅对可展开项有效）
    
    SidebarItem(const std::string& id, const std::string& icon, const std::string& text, 
                bool active = false, const std::string& tooltip = "", const std::string& iconPath = "",
                bool expandable = false)
        : id(id), icon(icon), text(text), isActive(active), tooltip(tooltip), iconPath(iconPath),
          isExpandable(expandable), isExpanded(false) {}
};

/**
 * @brief 侧边栏状态变化回调函数类型
 */
using SidebarStateCallback = std::function<void(bool isExpanded, float currentWidth)>;

/**
 * @brief 侧边栏布局类
 * 实现可收起展开的侧边栏，有流畅的动画效果
 */
class SidebarLayout : public LayoutBase {
public:
    /**
     * @brief 构造函数
     */
    explicit SidebarLayout();
    
    /**
     * @brief 渲染侧边栏布局
     */
    void render() override;
    
    /**
     * @brief 更新侧边栏布局
     * @param width 可用宽度
     * @param height 可用高度
     */
    void updateLayout(float width, float height) override;
    
    /**
     * @brief 处理侧边栏事件
     * @param event SDL事件
     */
    void handleEvent(const SDL_Event& event) override;
    
    /**
     * @brief 设置侧边栏展开状态
     */
    void setExpanded(bool expanded);
    
    /**
     * @brief 检查侧边栏是否展开
     */
    bool isExpanded() const { return isExpanded_; }
    
    /**
     * @brief 切换侧边栏展开状态
     */
    void toggleExpanded();
    
    /**
     * @brief 添加侧边栏项目
     */
    void addItem(const SidebarItem& item);
    
    /**
     * @brief 移除侧边栏项目
     */
    void removeItem(const std::string& id);
    
    /**
     * @brief 获取侧边栏项目
     */
    SidebarItem* getItem(const std::string& id);
    
    /**
     * @brief 清除所有项目
     */
    void clearItems();
    
    /**
     * @brief 设置当前激活项目
     */
    void setActiveItem(const std::string& id);
    
    /**
     * @brief 获取当前激活项目ID
     */
    const std::string& getActiveItemId() const { return activeItemId_; }
    
    /**
     * @brief 设置侧边栏宽度
     */
    void setSidebarWidth(float width) { sidebarWidth_ = width; }
    
    /**
     * @brief 获取侧边栏宽度
     */
    float getSidebarWidth() const { return sidebarWidth_; }
    
    /**
     * @brief 获取当前宽度
     */
    float getCurrentWidth() const { return currentWidth_; }
    
    /**
     * @brief 设置动画持续时间（毫秒）
     */
    void setAnimationDuration(float duration) { animationDuration_ = duration; }
    
    /**
     * @brief 获取动画持续时间
     */
    float getAnimationDuration() const { return animationDuration_; }
    
    /**
     * @brief 设置状态变化回调函数
     */
    void setStateCallback(const SidebarStateCallback& callback) { stateCallback_ = callback; }

private:
    // 侧边栏状态
    bool isExpanded_;                    ///< 是否展开
    bool isAnimating_;                   ///< 是否正在动画
    float currentWidth_;                 ///< 当前宽度
    float targetWidth_;                  ///< 目标宽度
    float sidebarWidth_;                 ///< 展开时的侧边栏宽度
    float collapsedWidth_;               ///< 收起时的侧边栏宽度
    float animationDuration_;            ///< 动画持续时间（毫秒）
    float animationStartTime_;           ///< 动画开始时间
    std::string activeItemId_;           ///< 当前激活的项目ID
    
    // 侧边栏项目
    std::vector<SidebarItem> items_;     ///< 侧边栏项目列表
    
    // 样式设置
    ImVec4 backgroundColor_;             ///< 背景颜色
    ImVec4 itemNormalColor_;             ///< 项目正常颜色
    ImVec4 itemHoverColor_;              ///< 项目悬停颜色
    ImVec4 itemActiveColor_;             ///< 项目激活颜色
    ImVec4 itemTextColor_;               ///< 项目文本颜色
    ImVec4 itemTextHoverColor_;          ///< 项目文本悬停颜色
    
    // 回调函数
    SidebarStateCallback stateCallback_; ///< 状态变化回调函数
    
    /**
     * @brief 更新动画状态
     * @param deltaTime 时间增量（秒）
     */
    void updateAnimation(double deltaTime);
    
    /**
     * @brief 渲染展开状态的侧边栏
     */
    void renderExpanded();
    
    /**
     * @brief 渲染收起状态的侧边栏
     */
    void renderCollapsed();
    
    /**
     * @brief 渲染侧边栏项目
     * @param item 侧边栏项目
     * @param isExpanded 是否为展开状态
     */
    void renderItem(const SidebarItem& item, bool isExpanded);
    
    /**
     * @brief 计算动画插值
     * @param t 时间因子 (0.0 - 1.0)
     * @return 插值结果
     */
    float easeOutCubic(float t) const;
    
    /**
     * @brief 处理项目点击事件
     * @param itemId 项目ID
     */
    void handleItemClick(const std::string& itemId);
    
    /**
     * @brief 触发状态变化回调
     */
    void triggerStateCallback();
};

} // namespace Window
} // namespace Core
} // namespace DearTs