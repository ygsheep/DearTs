#pragma once

#if defined(_WIN32)

#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>
#include <SDL.h>
#include <memory>
#include <functional>

namespace DearTs {
namespace Core {
namespace Window {

/**
 * @brief Windows Aero Snap处理器
 *
 * 在SDL2框架下实现Windows Aero Snap功能支持
 * 通过处理Windows消息和DWM集成来实现系统级窗口管理功能
 */
class AeroSnapHandler {
public:
    /**
     * @brief 构造函数
     * @param sdl_window SDL窗口句柄
     */
    explicit AeroSnapHandler(SDL_Window* sdl_window);

    /**
     * @brief 析构函数
     */
    ~AeroSnapHandler();

    /**
     * @brief 初始化Aero Snap处理器
     * @return 初始化是否成功
     */
    bool initialize();

    /**
     * @brief 关闭Aero Snap处理器
     */
    void shutdown();

    /**
     * @brief 处理Windows消息
     * @param hwnd 窗口句柄
     * @param msg 消息类型
     * @param wParam 消息参数
     * @param lParam 消息参数
     * @return 是否已处理消息
     */
    bool handleWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    /**
     * @brief 开始拖拽操作
     * @param mouseX 鼠标X坐标（屏幕坐标）
     * @param mouseY 鼠标Y坐标（屏幕坐标）
     */
    void startDragging(int mouseX, int mouseY);

    /**
     * @brief 检查是否在标题栏区域
     * @param x 窗口相对X坐标
     * @param y 窗口相对Y坐标
     * @param titleBarHeight 标题栏高度
     * @return 是否在标题栏区域
     */
    bool isInTitleBarArea(int x, int y, float titleBarHeight) const;

    /**
     * @brief 启用/禁用Aero Snap功能
     * @param enabled 是否启用
     */
    void setAeroSnapEnabled(bool enabled) { aeroSnapEnabled_ = enabled; }

    /**
     * @brief 检查Aero Snap是否启用
     * @return 是否启用
     */
    bool isAeroSnapEnabled() const { return aeroSnapEnabled_; }

    /**
     * @brief 设置标题栏高度
     * @param height 标题栏高度
     */
    void setTitleBarHeight(float height) { titleBarHeight_ = height; }

    /**
     * @brief 获取窗口句柄
     * @return Windows窗口句柄
     */
    HWND getHWND() const { return hwnd_; }

    /**
     * @brief 检查窗口是否最大化
     * @return 是否最大化
     */
    bool isMaximized() const;

    /**
     * @brief 最大化/还原窗口
     */
    void toggleMaximize();

    /**
     * @brief 最小化窗口
     */
    void minimize();

    /**
     * @brief 关闭窗口
     */
    void close();

private:
    SDL_Window* sdlWindow_;                    ///< SDL窗口句柄
    HWND hwnd_;                               ///< Windows窗口句柄
    WNDPROC originalWndProc_;                 ///< 原始窗口过程
    bool initialized_;                        ///< 是否已初始化
    bool aeroSnapEnabled_;                    ///< Aero Snap是否启用
    float titleBarHeight_;                    ///< 标题栏高度

    // 拖拽相关状态
    bool isDragging_;                         ///< 是否正在拖拽
    int dragStartX_;                          ///< 拖拽开始X坐标
    int dragStartY_;                          ///< 拖拽开始Y坐标
    int windowStartX_;                        ///< 窗口初始X坐标
    int windowStartY_;                        ///< 窗口初始Y坐标

    /**
     * @brief 安装自定义窗口过程
     * @return 安装是否成功
     */
    bool installWindowProc();

    /**
     * @brief 卸载自定义窗口过程
     */
    void uninstallWindowProc();

    /**
     * @brief 自定义窗口过程
     * @param hwnd 窗口句柄
     * @param msg 消息类型
     * @param wParam 消息参数
     * @param lParam 消息参数
     * @return 消息处理结果
     */
    static LRESULT CALLBACK customWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    /**
     * @brief 处理非客户区鼠标消息
     * @param hwnd 窗口句柄
     * @param msg 消息类型
     * @param wParam 消息参数
     * @param lParam 消息参数
     * @return 消息处理结果
     */
    LRESULT handleNcMouseMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    /**
     * @brief 处理窗口大小和移动消息
     * @param hwnd 窗口句柄
     * @param msg 消息类型
     * @param wParam 消息参数
     * @param lParam 消息参数
     * @return 消息处理结果
     */
    LRESULT handleSizeMoveMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    /**
     * @brief 执行窗口边缘命中测试
     * @param hwnd 窗口句柄
     * @param x 屏幕X坐标
     * @param y 屏幕Y坐标
     * @return 命中测试结果
     */
    LRESULT hitTestNCA(HWND hwnd, int x, int y);

    /**
     * @brief 扩展窗口框架到客户区
     * @param hwnd 窗口句柄
     */
    void extendFrameIntoClientArea(HWND hwnd);

    /**
     * @brief 刷新窗口样式，确保DWM效果立即生效
     * @param hwnd 窗口句柄
     */
    void refreshWindowStyle(HWND hwnd);

    /**
     * @brief 获取AeroSnap处理器实例（通过窗口属性）
     * @param hwnd 窗口句柄
     * @return 处理器实例指针
     */
    static AeroSnapHandler* getInstanceFromHWND(HWND hwnd);

    /**
     * @brief 将处理器实例绑定到窗口
     * @param hwnd 窗口句柄
     */
    void bindToWindow(HWND hwnd);

    /**
     * @brief 从窗口解绑处理器实例
     * @param hwnd 窗口句柄
     */
    void unbindFromWindow(HWND hwnd);

    // 窗口属性名称
    static const wchar_t* WINDOW_PROPERTY_NAME;
    // 边框宽度常量
    static const int BORDER_WIDTH = 8;        ///< 边框宽度（像素）
    static const int CORNER_WIDTH = 16;       ///< 角落宽度（像素）
};

} // namespace Window
} // namespace Core
} // namespace DearTs

#endif // _WIN32