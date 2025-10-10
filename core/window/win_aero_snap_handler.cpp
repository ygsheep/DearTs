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

    // 获取当前窗口样式并隐藏原生标题栏
    LONG_PTR style = GetWindowLongPtr(hwnd_, GWL_STYLE);
    LONG_PTR exStyle = GetWindowLongPtr(hwnd_, GWL_EXSTYLE);

    // ImHex风格的窗口样式修改 - 更精确的边框控制
    style &= ~WS_CAPTION;  // 移除标题栏
    style &= ~WS_THICKFRAME; // ImHex也移除了粗边框，然后重新添加需要的部分
    // 保留 Aero Snap 需要的基本功能
    style |= WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX; // 重新启用这些功能

    // 移除扩展样式的边框效果
    exStyle &= ~(WS_EX_CLIENTEDGE | WS_EX_WINDOWEDGE | WS_EX_DLGMODALFRAME);

    // ImHex风格 - 添加合成窗口属性
    exStyle |= WS_EX_COMPOSITED; // 启用窗口合成以减少闪烁

    // 应用新的窗口样式
    SetWindowLongPtr(hwnd_, GWL_STYLE, style);
    SetWindowLongPtr(hwnd_, GWL_EXSTYLE, exStyle);

    // 使用ImHex风格的DWM扩展设置
    MARGINS margins = {1, 1, 1, 1}; // ImHex使用1,1,1,1而不是-1
    DwmExtendFrameIntoClientArea(hwnd_, &margins);

    // 根据ImHex实现，添加必要的DWM设置以支持Aero Snap
    {
        constexpr BOOL value = TRUE;
        DwmSetWindowAttribute(hwnd_, DWMWA_NCRENDERING_ENABLED, &value, sizeof(value));
    }
    {
        constexpr DWMNCRENDERINGPOLICY value = DWMNCRP_ENABLED;
        DwmSetWindowAttribute(hwnd_, DWMWA_NCRENDERING_POLICY, &value, sizeof(value));
    }

    // ImHex风格的增强DWM设置
    {
        // 启用MMCSS (Multimedia Class Scheduler Service)
        DwmEnableMMCSS(TRUE);
    }
    {
        // 启用沉浸式深色模式
        constexpr BOOL value = TRUE;
        DwmSetWindowAttribute(hwnd_, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
    }

    // 设置标题栏颜色，与自定义标题栏背景色匹配
    {
        // 设置标题栏背景色为深灰色 (与ImGui的WindowBg颜色匹配)
        DWORD color = RGB(30, 30, 30); // RGB(0.12*255, 0.12*255, 0.12*255) ≈ (30, 30, 30)
        DwmSetWindowAttribute(hwnd_, DWMWA_CAPTION_COLOR, &color, sizeof(color));
    }
    {
        // 设置标题栏文本颜色为浅灰色
        DWORD color = RGB(230, 230, 230); // RGB(0.9*255, 0.9*255, 0.9*255) ≈ (230, 230, 230)
        DwmSetWindowAttribute(hwnd_, DWMWA_TEXT_COLOR, &color, sizeof(color));
    }

    DEARTS_LOG_INFO("Aero Snap处理器初始化 - 保守的边框隐藏方法");

    // 绑定处理器实例到窗口
    bindToWindow(hwnd_);

    // ImHex风格的精确时序控制 - 使用高精度定时器
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    LARGE_INTEGER startTime, endTime;
    QueryPerformanceCounter(&startTime);

    // 强制刷新窗口样式，确保DWM效果立即生效
    refreshWindowStyle(hwnd_);

    // 等待DWM完成处理 (ImHex风格的时序同步)
    constexpr int maxWaitTime = 100; // 最大等待100ms
    constexpr int checkInterval = 1;  // 每1ms检查一次
    int waitedTime = 0;

    do {
        Sleep(checkInterval);
        waitedTime += checkInterval;
        QueryPerformanceCounter(&endTime);

        // 检查DWM是否已经完成处理
        if (static_cast<double>((endTime.QuadPart - startTime.QuadPart) * 1000) / frequency.QuadPart > 10) {
            break; // 如果已经超过10ms，认为DWM处理完成
        }
    } while (waitedTime < maxWaitTime);

    DEARTS_LOG_INFO("Aero Snap处理器DWM初始化刷新完成 (精确时序控制: " +
                   std::to_string(static_cast<double>((endTime.QuadPart - startTime.QuadPart) * 1000) / frequency.QuadPart) + "ms)");

    initialized_ = true;
    DEARTS_LOG_INFO("Aero Snap处理器初始化成功 (v3.0 with ImHex-style timing)");
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
            // 让 Windows 处理 Aero Snap
            return DefWindowProc(hwnd, msg, wParam, lParam);

        case WM_NCPAINT:
            // 让 Windows 处理 Aero Snap
            return DefWindowProc(hwnd, msg, wParam, lParam);

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

    // 计算相对于窗口客户区的坐标
    // WM_NCLBUTTONDOWN的lParam需要相对于窗口左上角的坐标
    int clientX = mouseX - windowRect.left;
    int clientY = mouseY - windowRect.top;

    DEARTS_LOG_INFO("AeroSnapHandler: 发送WM_NCLBUTTONDOWN，屏幕坐标:(" +
                   std::to_string(mouseX) + "," + std::to_string(mouseY) +
                   ") 窗口坐标:(" + std::to_string(clientX) + "," + std::to_string(clientY) + ")");

    // 发送系统拖拽消息，触发Aero Snap
    // 使用相对于窗口的坐标，而不是屏幕坐标
    PostMessage(hwnd_, WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(clientX, clientY));
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
 * 处理SDL事件
 */
bool AeroSnapHandler::handleEvent(const SDL_Event& event) {
    if (!initialized_ || !aeroSnapEnabled_ || !hwnd_) {
        return false;
    }

    switch (event.type) {
        case SDL_MOUSEBUTTONDOWN:
            if (event.button.button == SDL_BUTTON_LEFT) {
                DEARTS_LOG_INFO("AeroSnapHandler: 处理鼠标按下事件");

                // 获取鼠标相对于SDL窗口的坐标
                int mouseX, mouseY;
                int windowX, windowY;
                SDL_GetWindowPosition(sdlWindow_, &windowX, &windowY);
                SDL_GetGlobalMouseState(&mouseX, &mouseY);

                // 转换为窗口相对坐标
                int relativeX = mouseX - windowX;
                int relativeY = mouseY - windowY;

                DEARTS_LOG_INFO("AeroSnapHandler: 鼠标位置(" + std::to_string(relativeX) + "," + std::to_string(relativeY) + ") 标题栏高度: " + std::to_string(static_cast<int>(titleBarHeight_)));

                if (isInTitleBarArea(relativeX, relativeY, titleBarHeight_)) {
                    DEARTS_LOG_INFO("AeroSnapHandler: 在标题栏区域，开始拖拽");
                    // 开始拖拽操作，使用屏幕坐标
                    startDragging(mouseX, mouseY);
                    return true;
                } else {
                    DEARTS_LOG_INFO("AeroSnapHandler: 不在标题栏区域");
                }
            }
            break;

        case SDL_MOUSEBUTTONUP:
            if (event.button.button == SDL_BUTTON_LEFT && isDragging_) {
                // 结束拖拽
                isDragging_ = false;
                return true;
            }
            break;

        case SDL_MOUSEMOTION:
            // 鼠标移动事件已在Windows消息处理中处理
            // 这里可以添加额外的逻辑，如果需要的话
            break;

        case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_MAXIMIZED ||
                event.window.event == SDL_WINDOWEVENT_RESTORED ||
                event.window.event == SDL_WINDOWEVENT_MOVED ||
                event.window.event == SDL_WINDOWEVENT_RESIZED) {
                // 窗口状态改变时刷新DWM效果
                refreshWindowStyle(hwnd_);
                return true;
            }
            break;
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

                DEARTS_LOG_INFO("AeroSnapHandler: WM_NCLBUTTONDOWN处理 - 让Windows处理系统拖拽");
                // 对于HTCAPTION，直接让Windows处理，这样可以触发Aero Snap
                return DefWindowProc(hwnd, msg, wParam, lParam);
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

    // 对于其他情况，调用默认处理
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
        // 在Aero Snap模式下，标题栏区域也返回HTCLIENT，让SDL处理事件
        // 这样TitleBarLayout就能收到鼠标事件，然后由AeroSnapHandler的SDL事件处理来触发拖拽
        return HTCLIENT; // 让SDL处理标题栏区域的鼠标事件
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

    // 获取当前窗口样式并确保隐藏原生标题栏
    LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
    LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);

    // ImHex风格的窗口样式确保
    style &= ~WS_CAPTION;  // 移除标题栏
    style &= ~WS_THICKFRAME; // 移除粗边框
    style |= WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX; // 重新启用Aero Snap需要的功能

    // 移除所有扩展样式的边框效果
    exStyle &= ~(WS_EX_CLIENTEDGE | WS_EX_WINDOWEDGE | WS_EX_DLGMODALFRAME);
    exStyle |= WS_EX_COMPOSITED; // 确保合成属性启用

    // 重新应用样式
    SetWindowLongPtr(hwnd, GWL_STYLE, style);
    SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle);

    // 重新应用DWM扩展（ImHex风格）
    MARGINS margins = {1, 1, 1, 1}; // ImHex使用1,1,1,1而不是-1
    DwmExtendFrameIntoClientArea(hwnd, &margins);

    // 强制窗口重新计算非客户区
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                 SWP_FRAMECHANGED | SWP_DRAWFRAME);
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