#pragma once

#include "window_base.h"
#include "layouts/layout_manager.h"
#include "layouts/title_bar_layout.h"
#include "layouts/sidebar_layout.h"
#include <string>
#include <memory>
#include <imgui.h>

// 前向声明
namespace DearTs {
namespace Core {
namespace Window {
class PomodoroLayout; // 番茄时钟布局前向声明
class ExchangeRecordLayout; // 换取记录布局前向声明
namespace Widgets { namespace Clipboard { class ClipboardHistoryLayout; } } // 剪切板管理器布局前向声明
} // namespace Window
} // namespace Core
} // namespace DearTs

namespace DearTs {
namespace Core {
namespace Window {

/**
 * @brief 主窗口视图类型枚举
 */
enum class MainViewType {
    DEFAULT,           ///< 默认视图
    POMODORO,          ///< 番茄时钟视图
    EXCHANGE_RECORD,   ///< 换取记录视图
    CLIPBOARD_HELPER   ///< 剪切板管理器视图
};

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
     * @brief 析构函数
     */
    ~MainWindow() override;

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
     */
    void update() override;
    
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
    ImVec4 clearColor_;  ///< 清屏颜色

    // 布局管理
    LayoutManager& layoutManager_;  ///< 布局管理器引用

    // 系统布局引用（直接访问，不拥有所有权）
    SidebarLayout* sidebarLayout_;   ///< 侧边栏布局引用

    // 剪切板监听器状态
    bool clipboard_monitoring_started_;  ///< 剪切板监听器是否已启动

    // 布局注册表（使用新增强的LayoutManager功能）
    std::vector<std::string> registeredLayoutIds_;  ///< 已注册的布局ID列表

    /**
     * @brief 初始化系统布局（标题栏、侧边栏）
     */
    void initializeSystemLayouts();

    /**
     * @brief 初始化内容布局（番茄时钟、换取记录、剪切板管理等）
     */
    void initializeContentLayouts();

    /**
     * @brief 设置侧边栏事件处理
     */
    void setupSidebarEventHandlers();

    /**
     * @brief 渲染默认内容（当没有内容布局可见时）
     */
    void renderDefaultContent();

    // === 布局注册和初始化方法 ===

    /**
     * @brief 注册所有布局类型
     * 使用增强的LayoutManager注册机制
     */
    void registerAllLayoutTypes();

    /**
     * @brief 注册系统布局类型
     */
    void registerSystemLayoutTypes();

    /**
     * @brief 注册内容布局类型
     */
    void registerContentLayoutTypes();

    /**
     * @brief 创建布局依赖关系
     */
    void setupLayoutDependencies();

    /**
     * @brief 初始化已注册的布局
     */
    void initializeRegisteredLayouts();

    /**
     * @brief 设置布局优先级
     */
    void setupLayoutPriorities();

    /**
     * @brief 映射侧边栏项目ID到布局名称
     * @param itemId 侧边栏项目ID
     * @return 对应的布局名称，如果不存在则返回空字符串
     */
    std::string mapSidebarItemToLayout(const std::string& itemId);
};

} // namespace Window
} // namespace Core
} // namespace DearTs