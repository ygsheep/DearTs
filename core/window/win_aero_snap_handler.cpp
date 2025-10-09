#include "win_aero_snap_handler.h"
#include "../utils/logger.h"
#include <SDL_syswm.h>
#include <iostream>
#include <memory>

#if defined(_WIN32)

namespace DearTs {
namespace Core {
namespace Window {

// 静态成员定义
const wchar_t* AeroSnapHandler::WINDOW_PROPERTY_NAME = L"AeroSnapHandlerInstance";

/**
 * AeroSnapHandler构造函数
 */
AeroSnapHandler::AeroSnapHandler(SDL_Window* sdl_window)
    : sdlWindow_(sdl_window)
    , hwnd_(nullptr)
    , originalWndProc_(nullptr)
    , initialized_(false)
    , aeroSnapEnabled_(true)
    , titleBarHeight_(30.0f)
    , isDragging_(false)
    , dragStartX_(0)
    , dragStartY_(0)
    , windowStartX_(0)
    , windowStartY_(0) {
}

/**
 * 析构函数
 */
AeroSnapHandler::~AeroSnapHandler() {
    shutdown();
}

/**
 * 初始化Aero Snap处理器
 */
bool AeroSnapHandler::initialize() {
    if (initialized_) {
        return true;
    }

    if (!sdlWindow_) {
        DEARTS_LOG_ERROR("SDL窗口句柄为空");
        return false;
    }

    // 获取Windows窗口句柄
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    if (!SDL_GetWindowWMInfo(sdlWindow_, &wmInfo)) {
        DEARTS_LOG_ERROR("无法获取SDL窗口系统信息: " + std::string(SDL_GetError()));
        return false;
    }

    if (wmInfo.subsystem != SDL_SYSWM_WINDOWS) {
        DEARTS_LOG_ERROR("当前系统不是Windows");
        return false;
    }

    hwnd_ = wmInfo.info.win.window;
    if (!hwnd_) {
        DEARTS_LOG_ERROR("无法获取Windows窗口句柄");
        return false;
    }

    // 安装自定义窗口过程
    if (!installWindowProc()) {
        DEARTS_LOG_ERROR("安装自定义窗口过程失败");
        return false;
    }

    // 扩展窗口框架到客户区
    extendFrameIntoClientArea(hwnd_);

    // 绑定处理器实例到窗口
    bindToWindow(hwnd_);

    // 强制刷新窗口样式，确保DWM效果立即生效
    refreshWindowStyle(hwnd_);
    DEARTS_LOG_INFO("Aero Snap处理器DWM初始化刷新完成");

    initialized_ = true;
    DEARTS_LOG_INFO("Aero Snap处理器初始化成功 (v2.0 with timing fixes)");
    return true;
}

/**
 * 关闭Aero Snap处理器
 */
void AeroSnapHandler::shutdown() {
    if (!initialized_) {
        return;
    }

    if (hwnd_) {
        // 卸载自定义窗口过程
        uninstallWindowProc();

        // 从窗口解绑处理器实例
        unbindFromWindow(hwnd_);

        hwnd_ = nullptr;
    }

    initialized_ = false;
    DEARTS_LOG_INFO("Aero Snap处理器已关闭");
}

/**
 * 处理Windows消息
 */
bool AeroSnapHandler::handleWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (!initialized_ || !aeroSnapEnabled_) {
        return false;
    }

    switch (msg) {
        case WM_NCACTIVATE:
            // 保持窗口激活状态，确保边框正确显示
            return TRUE;

        case WM_NCPAINT:
            // 处理非客户区绘制，确保Aero效果
            return TRUE;

        case WM_NCHITTEST:
            // 执行命中测试，支持窗口边缘调整大小
            {
                POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                LRESULT result = hitTestNCA(hwnd, pt.x, pt.y);
                if (result != HTCLIENT) {
                    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, result);
                    return TRUE;
                }
            }
            break;

        case WM_NCLBUTTONDOWN:
        case WM_NCLBUTTONDBLCLK:
            // 处理非客户区鼠标点击，支持标题栏操作
            return handleNcMouseMessage(hwnd, msg, wParam, lParam) != 0;

        case WM_ENTERSIZEMOVE:
        case WM_EXITSIZEMOVE:
        case WM_SIZING:
        case WM_MOVING:
            // 处理窗口大小和移动消息
            return handleSizeMoveMessage(hwnd, msg, wParam, lParam) != 0;

        case WM_SIZE:
            // 窗口大小改变时强制刷新DWM效果
            if (wParam == SIZE_MAXIMIZED || wParam == SIZE_RESTORED) {
                // 延迟刷新，确保Windows完成窗口状态切换
                SetTimer(hwnd, 2, 50, nullptr);
            }
            break;

        case WM_SHOWWINDOW:
            // 窗口显示时延迟刷新DWM效果
            if (wParam == TRUE) {
                DEARTS_LOG_INFO("窗口显示，设置200ms延迟DWM刷新定时器");
                SetTimer(hwnd, 1, 200, nullptr);
            }
            break;

        case WM_TIMER:
            // 处理定时器消息
            if (wParam == 1 || wParam == 2) {
                KillTimer(hwnd, wParam);
                DEARTS_LOG_INFO("定时器触发，执行DWM刷新 (timer_id: " + std::to_string(wParam) + ")");
                refreshWindowStyle(hwnd);
            }
            break;

        case WM_ACTIVATE:
            // 窗口激活时刷新DWM效果
            if (wParam != WA_INACTIVE) {
                DEARTS_LOG_INFO("窗口激活，立即刷新DWM效果");
                refreshWindowStyle(hwnd);
            }
            break;
    }

    return false;
}

/**
 * 开始拖拽操作
 */
void AeroSnapHandler::startDragging(int mouseX, int mouseY) {
    if (!initialized_ || !hwnd_) {
        return;
    }

    // 保存初始状态
    isDragging_ = true;
    dragStartX_ = mouseX;
    dragStartY_ = mouseY;

    RECT windowRect;
    GetWindowRect(hwnd_, &windowRect);
    windowStartX_ = windowRect.left;
    windowStartY_ = windowRect.top;

    // 发送系统拖拽消息，触发Aero Snap
    PostMessage(hwnd_, WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(mouseX, mouseY));
}

/**
 * 检查是否在标题栏区域
 */
bool AeroSnapHandler::isInTitleBarArea(int x, int y, float titleBarHeight) const {
    if (!initialized_) {
        return false;
    }

    // 检查是否在标题栏高度范围内
    if (y >= 0 && y <= static_cast<int>(titleBarHeight)) {
        // 排除控制按钮区域（右侧约150像素）
        RECT windowRect;
        GetWindowRect(hwnd_, &windowRect);
        int windowWidth = windowRect.right - windowRect.left;

        if (x < windowWidth - 150) {
            return true;
        }
    }

    return false;
}

/**
 * 检查窗口是否最大化
 */
bool AeroSnapHandler::isMaximized() const {
    if (!hwnd_) {
        return false;
    }

    return (GetWindowLongPtr(hwnd_, GWL_STYLE) & WS_MAXIMIZE) != 0;
}

/**
 * 最大化/还原窗口
 */
void AeroSnapHandler::toggleMaximize() {
    if (!hwnd_) {
        return;
    }

    if (isMaximized()) {
        ShowWindow(hwnd_, SW_RESTORE);
    } else {
        ShowWindow(hwnd_, SW_MAXIMIZE);
    }
}

/**
 * 最小化窗口
 */
void AeroSnapHandler::minimize() {
    if (!hwnd_) {
        return;
    }

    ShowWindow(hwnd_, SW_MINIMIZE);
}

/**
 * 关闭窗口
 */
void AeroSnapHandler::close() {
    if (!hwnd_) {
        return;
    }

    PostMessage(hwnd_, WM_CLOSE, 0, 0);
}

/**
 * 安装自定义窗口过程
 */
bool AeroSnapHandler::installWindowProc() {
    if (!hwnd_) {
        return false;
    }

    // 保存原始窗口过程
    originalWndProc_ = reinterpret_cast<WNDPROC>(
        GetWindowLongPtr(hwnd_, GWLP_WNDPROC)
    );

    if (!originalWndProc_) {
        DEARTS_LOG_ERROR("无法获取原始窗口过程");
        return false;
    }

    // 设置新的窗口过程
    SetWindowLongPtr(hwnd_, GWLP_WNDPROC,
        reinterpret_cast<LONG_PTR>(customWindowProc));

    return true;
}

/**
 * 卸载自定义窗口过程
 */
void AeroSnapHandler::uninstallWindowProc() {
    if (hwnd_ && originalWndProc_) {
        // 恢复原始窗口过程
        SetWindowLongPtr(hwnd_, GWLP_WNDPROC,
            reinterpret_cast<LONG_PTR>(originalWndProc_));
        originalWndProc_ = nullptr;
    }
}

/**
 * 自定义窗口过程
 */
LRESULT CALLBACK AeroSnapHandler::customWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // 获取绑定的处理器实例
    AeroSnapHandler* handler = getInstanceFromHWND(hwnd);
    if (!handler) {
        // 如果没有找到处理器实例，调用默认处理
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    // 处理Aero Snap相关消息
    if (handler->handleWindowMessage(hwnd, msg, wParam, lParam)) {
        return 0;
    }

    // 调用原始窗口过程
    if (handler->originalWndProc_) {
        return CallWindowProc(handler->originalWndProc_, hwnd, msg, wParam, lParam);
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

/**
 * 处理非客户区鼠标消息
 */
LRESULT AeroSnapHandler::handleNcMouseMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_NCLBUTTONDOWN:
            if (wParam == HTCAPTION) {
                // 开始拖拽标题栏
                isDragging_ = true;
                dragStartX_ = GET_X_LPARAM(lParam);
                dragStartY_ = GET_Y_LPARAM(lParam);

                RECT windowRect;
                GetWindowRect(hwnd, &windowRect);
                windowStartX_ = windowRect.left;
                windowStartY_ = windowRect.top;
            }
            break;

        case WM_NCLBUTTONDBLCLK:
            if (wParam == HTCAPTION) {
                // 双击标题栏切换最大化状态
                toggleMaximize();
                return 0;
            }
            break;
    }

    // 调用默认处理
    if (originalWndProc_) {
        return CallWindowProc(originalWndProc_, hwnd, msg, wParam, lParam);
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

/**
 * 处理窗口大小和移动消息
 */
LRESULT AeroSnapHandler::handleSizeMoveMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_ENTERSIZEMOVE:
            // 开始大小或移动操作
            break;

        case WM_EXITSIZEMOVE:
            // 结束大小或移动操作
            isDragging_ = false;
            break;

        case WM_SIZING:
            // 正在调整大小
            return TRUE;

        case WM_MOVING:
            // 正在移动，这里可以实现自定义的吸附逻辑
            if (isDragging_) {
                RECT* rect = reinterpret_cast<RECT*>(lParam);

                // 可以在这里添加边缘吸附逻辑
                // 例如：检测屏幕边缘并调整位置
            }
            return TRUE;
    }

    // 调用默认处理
    if (originalWndProc_) {
        return CallWindowProc(originalWndProc_, hwnd, msg, wParam, lParam);
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

/**
 * 执行窗口边缘命中测试
 */
LRESULT AeroSnapHandler::hitTestNCA(HWND hwnd, int x, int y) {
    if (!initialized_) {
        return HTCLIENT;
    }

    RECT windowRect;
    GetWindowRect(hwnd, &windowRect);

    // 转换为窗口相对坐标
    int relativeX = x - windowRect.left;
    int relativeY = y - windowRect.top;
    int width = windowRect.right - windowRect.left;
    int height = windowRect.bottom - windowRect.top;

    // 检查是否在标题栏区域
    if (relativeY <= static_cast<int>(titleBarHeight_)) {
        // 检查控制按钮区域
        if (relativeX >= width - 150) {
            return HTCLIENT; // 让ImGui处理控制按钮
        }
        return HTCAPTION; // 标题栏区域
    }

    // 检查边框区域（用于调整大小）
    bool onTop = relativeY < BORDER_WIDTH;
    bool onBottom = relativeY > height - BORDER_WIDTH;
    bool onLeft = relativeX < BORDER_WIDTH;
    bool onRight = relativeX > width - BORDER_WIDTH;

    if (onTop && onLeft) return HTTOPLEFT;
    if (onTop && onRight) return HTTOPRIGHT;
    if (onBottom && onLeft) return HTBOTTOMLEFT;
    if (onBottom && onRight) return HTBOTTOMRIGHT;
    if (onTop) return HTTOP;
    if (onBottom) return HTBOTTOM;
    if (onLeft) return HTLEFT;
    if (onRight) return HTRIGHT;

    return HTCLIENT;
}

/**
 * 扩展窗口框架到客户区
 */
void AeroSnapHandler::extendFrameIntoClientArea(HWND hwnd) {
    if (!hwnd) {
        return;
    }

    // 使用DWM扩展窗口框架
    MARGINS margins = { -1 }; // -1表示扩展到整个客户区
    DwmExtendFrameIntoClientArea(hwnd, &margins);
}

/**
 * 刷新窗口样式，确保DWM效果立即生效
 */
void AeroSnapHandler::refreshWindowStyle(HWND hwnd) {
    if (!hwnd) {
        return;
    }

    DEARTS_LOG_INFO("执行DWM窗口样式刷新");

    // 强制重绘窗口
    InvalidateRect(hwnd, nullptr, TRUE);
    UpdateWindow(hwnd);

    // 获取当前窗口样式
    LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
    LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);

    // 重新设置样式，强制Windows重新评估窗口外观
    SetWindowLongPtr(hwnd, GWL_STYLE, style);
    SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle);

    // 强制窗口重新计算非客户区
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                 SWP_FRAMECHANGED | SWP_DRAWFRAME);

    // 再次发送DWM扩展，确保效果立即生效
    extendFrameIntoClientArea(hwnd);
}

/**
 * 获取AeroSnap处理器实例（通过窗口属性）
 */
AeroSnapHandler* AeroSnapHandler::getInstanceFromHWND(HWND hwnd) {
    return reinterpret_cast<AeroSnapHandler*>(
        GetPropW(hwnd, WINDOW_PROPERTY_NAME)
    );
}

/**
 * 将处理器实例绑定到窗口
 */
void AeroSnapHandler::bindToWindow(HWND hwnd) {
    if (hwnd) {
        SetPropW(hwnd, WINDOW_PROPERTY_NAME, this);
    }
}

/**
 * 从窗口解绑处理器实例
 */
void AeroSnapHandler::unbindFromWindow(HWND hwnd) {
    if (hwnd) {
        RemovePropW(hwnd, WINDOW_PROPERTY_NAME);
    }
}

} // namespace Window
} // namespace Core
} // namespace DearTs

#endif // _WIN32