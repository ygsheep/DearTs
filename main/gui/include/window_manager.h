#pragma once

#include <SDL.h>
#include <imgui.h>
#include <Windows.h>
#include <functional>
#include <string>
#include "../../../lib/libdearts/include/dearts/api/event_manager.hpp"

namespace DearTs::GUI {

// 引入EventManager到当前命名空间
using dearts::EventManager;

// 定义窗口相关事件
EVENT_DEF(WindowInitializedEvent, void*);
EVENT_DEF(WindowTitleChangedEvent, const std::string&);
EVENT_DEF(ThemeChangedEvent, const std::string&);
EVENT_DEF(FontChangedEvent, const std::string&);

/**
 * WindowManager类 - 重构后的窗口管理器
 * 基于事件驱动架构，提供现代化的窗口管理功能
 * 参考ImHex的设计模式，支持插件化扩展
 */
class WindowManager {
public:
    /**
     * 构造函数
     * @param window SDL窗口句柄
     */
    explicit WindowManager(SDL_Window* window);
    
    /**
     * 析构函数
     */
    ~WindowManager();
    
    /**
     * 初始化窗口管理器
     * 设置窗口样式为无边框，配置自定义标题栏
     * @return 初始化是否成功
     */
    bool initialize();
    
    /**
     * 渲染自定义标题栏
     * 在ImGui渲染循环中调用，绘制标题栏UI
     */
    void renderTitleBar();
    
    /**
     * 渲染搜索对话框
     * 显示搜索输入框和相关功能
     */
    void renderSearchDialog();
    
    /**
     * 处理键盘快捷键
     * 处理Ctrl+F等快捷键操作
     * @param key 按键事件
     */
    void handleKeyboardShortcuts(const SDL_KeyboardEvent& key);
    
    /**
     * 处理窗口事件
     * 处理鼠标拖拽、按钮点击等事件
     * @param event SDL事件
     */
    void handleEvent(const SDL_Event& event);
    
    /**
     * 检查是否在标题栏区域
     * @param x 鼠标X坐标
     * @param y 鼠标Y坐标
     * @return 是否在标题栏区域
     */
    bool isInTitleBarArea(int x, int y) const;
    
    /**
     * 开始拖拽窗口
     * @param mouseX 鼠标X坐标
     * @param mouseY 鼠标Y坐标
     */
    void startDragging(int mouseX, int mouseY);
    
    /**
     * 更新拖拽状态
     * @param mouseX 当前鼠标X坐标
     * @param mouseY 当前鼠标Y坐标
     */
    void updateDragging(int mouseX, int mouseY);
    
    /**
     * 停止拖拽
     */
    void stopDragging();
    
    /**
     * 最小化窗口
     */
    void minimizeWindow();
    
    /**
     * 最大化/还原窗口
     */
    void toggleMaximize();
    
    /**
     * 关闭窗口
     */
    void closeWindow();
    
    /**
     * 设置窗口标题
     * @param title 新标题
     */
    void setWindowTitle(const char* title);
    
    /**
     * 获取标题栏高度
     * @return 标题栏高度（像素）
     */
    float getTitleBarHeight() const { return m_titleBarHeight; }
    
    /**
     * 获取SDL窗口句柄
     * @return SDL窗口句柄
     */
    SDL_Window* getSDLWindow() const { return m_sdlWindow; }
    
    /**
     * 获取Windows窗口句柄
     * @return Windows窗口句柄
     */
    HWND getWindowsHandle() const { return m_hwnd; }
    
    /**
     * 检查窗口是否最大化
     * @return 是否最大化
     */
    bool isMaximized() const { return m_isMaximized; }
    
    /**
     * 检查是否正在拖拽
     * @return 是否正在拖拽
     */
    bool isDragging() const { return m_isDragging; }

private:
    // 窗口相关成员变量
    SDL_Window* m_sdlWindow;          ///< SDL窗口句柄
    HWND m_hwnd;                      ///< Windows窗口句柄
    bool m_isDragging;                ///< 是否正在拖拽
    bool m_isMaximized;               ///< 是否已最大化
    int m_dragOffsetX, m_dragOffsetY; ///< 拖拽偏移量
    float m_titleBarHeight;           ///< 标题栏高度
    char m_windowTitle[256];          ///< 窗口标题
    
    // 搜索相关成员变量
    bool m_showSearchDialog;          ///< 是否显示搜索对话框
    char m_searchBuffer[256];         ///< 搜索输入缓冲区
    bool m_searchInputFocused;        ///< 搜索输入框是否获得焦点
    
    // 窗口状态保存
    int m_normalX, m_normalY;         ///< 正常状态位置
    int m_normalWidth, m_normalHeight; ///< 正常状态大小
    
    /**
     * 获取Windows窗口句柄
     * @return Windows窗口句柄
     */
    HWND getHWND();
    
    /**
     * 设置无边框样式
     */
    void setBorderlessStyle();
    
    /**
     * 保存窗口状态
     */
    void saveWindowState();
    
    /**
     * 恢复窗口状态
     */
    void restoreWindowState();
    
    /**
     * 渲染搜索框
     * @param windowWidth 窗口宽度
     * @param buttonsWidth 按钮区域宽度
     */
    void renderSearchBox(float windowWidth, float buttonsWidth);
    
    /**
     * 渲染窗口控制按钮
     * @param windowWidth 窗口宽度
     * @param buttonWidth 按钮宽度
     * @param buttonHeight 按钮高度
     */
    void renderWindowControls(float windowWidth, float buttonWidth, float buttonHeight);
    
    /**
     * 注册事件处理器
     */
    void registerEventHandlers();
    
    /**
     * 取消事件处理器注册
     */
    void unregisterEventHandlers();
    
    /**
     * 更新标题栏样式
     */
    void updateTitleBarStyle();
    
    /**
     * 重新计算标题栏高度
     */
    void recalculateTitleBarHeight();
};

} // namespace DearTs::GUI