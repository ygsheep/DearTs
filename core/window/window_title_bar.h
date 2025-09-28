#pragma once
#include <SDL.h>
#include <imgui.h>
#include <Windows.h>
#include <memory>
#include <string>

#include "../utils/logger.h"
#include "../resource/font_resource.h"
#include "../resource/vscode_icons.hpp"

// Forward declare the Window class from the DearTs::Core::Window namespace
namespace DearTs {
namespace Core {
namespace Window {
class Window;
}
}
}

/** 
 * @brief 窗口标题栏类
 * 提供无边框窗口的标题栏拖拽、最小化、最大化、关闭等功能
 */
class WindowTitleBar {
public:
    /** 
     * @brief 构造函数
     * @param window 关联的窗口
     */
    explicit WindowTitleBar(std::shared_ptr<DearTs::Core::Window::Window> window);
    
    /**
     * @brief 析构函数
     */
    ~WindowTitleBar();
    
    /**
     * @brief 初始化窗口标题栏
     * 设置窗口样式为无边框，配置自定义标题栏
     * @return 初始化是否成功
     */
    bool initialize();
    
    /**
     * @brief 渲染自定义标题栏
     * 在ImGui渲染循环中调用，绘制标题栏UI
     */
    void render();
    
    /**
     * @brief 渲染搜索对话框
     * 显示搜索输入框和相关功能
     */
    void renderSearchDialog();
    
    /**
     * @brief 处理键盘快捷键
     * 处理Ctrl+F等快捷键操作
     */
    void handleKeyboardShortcuts();
    
    /**
     * @brief 处理窗口事件
     * 处理鼠标拖拽、按钮点击等事件
     * @param event SDL事件
     */
    void handleEvent(const SDL_Event& event);
    
    /**
     * @brief 检查是否在标题栏区域
     * @param x 鼠标X坐标
     * @param y 鼠标Y坐标
     * @return 是否在标题栏区域
     */
    bool isInTitleBarArea(int x, int y) const;
    
    /**
     * @brief 开始拖拽窗口
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     */
    void startDragging(int mouseX, int mouseY);
    
    /**
     * @brief 更新拖拽状态
     * @param mouseX 当前鼠标X坐标
     * @param mouseY 当前鼠标Y坐标
     */
    void updateDragging(int mouseX, int mouseY);
    
    /**
     * @brief 停止拖拽
     */
    void stopDragging();
    
    /**
     * @brief 最小化窗口
     */
    void minimizeWindow();
    
    /**
     * @brief 最大化/还原窗口
     */
    void toggleMaximize();
    
    /**
     * @brief 关闭窗口
     */
    void closeWindow();
    
    /**
     * @brief 获取标题栏高度
     * @return 标题栏高度（像素）
     */
    float getTitleBarHeight() const { return titleBarHeight_; }
    
    /**
     * @brief 设置窗口标题
     * @param title 窗口标题
     */
    void setWindowTitle(const std::string& title);
    
    /**
     * @brief 获取窗口标题
     * @return 窗口标题
     */
    const std::string& getWindowTitle() const { return windowTitle_; }
    
private:
    std::shared_ptr<DearTs::Core::Window::Window> window_;    ///< 关联的窗口
    SDL_Window* sdlWindow_;             ///< SDL窗口句柄
    HWND hwnd_;                         ///< Windows窗口句柄
    bool isDragging_;                   ///< 是否正在拖拽
    bool isMaximized_;                  ///< 是否已最大化
    int dragOffsetX_, dragOffsetY_;     ///< 拖拽偏移量
    float titleBarHeight_;              ///< 标题栏高度
    std::string windowTitle_;           ///< 窗口标题
    
    // 搜索功能相关
    bool showSearchDialog_;             ///< 是否显示搜索对话框
    char searchBuffer_[256];            ///< 搜索输入缓冲区
    bool searchInputFocused_;           ///< 搜索输入框是否获得焦点
    
    // 窗口状态保存
    int normalX_, normalY_;             ///< 正常状态位置
    int normalWidth_, normalHeight_;    ///< 正常状态大小
    
    /**
     * @brief 获取Windows窗口句柄
     * @return Windows窗口句柄
     */
    HWND getHWND();
    
    /**
     * @brief 设置窗口样式为无边框
     */
    void setBorderlessStyle();
    
    /**
     * @brief 保存当前窗口状态
     */
    void saveWindowState();
    
    /**
     * @brief 还原窗口状态
     */
    void restoreWindowState();
    
    /**
     * @brief 渲染标题文本
     */
    void renderTitle();
    
    /**
     * @brief 渲染搜索框
     */
    void renderSearchBox();
    
    /**
     * @brief 渲染窗口控制按钮
     */
    void renderControlButtons();
    
    /**
     * @brief 渲染fallback标题栏（当ImGui未初始化时使用）
     */
    void renderFallbackTitleBar();
};