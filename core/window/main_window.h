#pragma once

#include "window_base.h"
#include "layouts/title_bar_layout.h"
#include "layouts/sidebar_layout.h"
#include <string>
#include <imgui.h>

namespace DearTs {
namespace Core {
namespace Window {

/**
 * @brief 主窗口类
 * 继承自WindowBase，作为应用程序的主窗口
 */
class MainWindow : public WindowBase {
public:
    /**
     * @brief 构造函数
     * @param title 窗口标题
     */
    explicit MainWindow(const std::string& title = "DearTs Application");
    
    /**
     * @brief 初始化窗口
     * @return 初始化是否成功
     */
    bool initialize() override;
    
    /**
     * @brief 渲染窗口内容
     */
    void render() override;
    
    /**
     * @brief 更新窗口逻辑
     * @param deltaTime 时间增量（秒）
     */
    void update(double deltaTime) override;
    
    /**
     * @brief 处理窗口事件
     * @param event SDL事件
     */
    void handleEvent(const SDL_Event& event) override;
    
    /**
     * @brief 获取侧边栏布局
     */
    SidebarLayout* getSidebarLayout() const;
    
    /**
     * @brief 获取内容区域的X坐标
     */
    float getContentAreaX() const;
    
    /**
     * @brief 获取内容区域的宽度
     */
    float getContentAreaWidth() const;

private:
    // 主窗口内容相关
    bool showDemoWindow_;  ///< 是否显示ImGui演示窗口
    bool showAnotherWindow_;  ///< 是否显示另一个窗口
    ImVec4 clearColor_;  ///< 清屏颜色
};

} // namespace Window
} // namespace Core
} // namespace DearTs