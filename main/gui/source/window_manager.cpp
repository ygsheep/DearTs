#include "../include/window_manager.h"
#include "../../../lib/libdearts/include/dearts/api/dearts_api.hpp"
#include "../../../lib/libdearts/include/dearts/api/event_manager.hpp"
#include <SDL.h>
#include <SDL_syswm.h>
#include <iostream>
#include <algorithm>
#include "logger.h"
#include "resource/font_resource.h"
#include "resource/vscode_icons.hpp"
#include <memory>

namespace DearTs::GUI {

/**
 * WindowManager构造函数
 * 初始化成员变量并注册事件监听器
 */
WindowManager::WindowManager(SDL_Window* window) 
    : m_sdlWindow(window), m_hwnd(nullptr), m_isDragging(false), m_isMaximized(false),
      m_dragOffsetX(0), m_dragOffsetY(0), m_titleBarHeight(30.0f),
      m_showSearchDialog(false), m_searchInputFocused(false),
      m_normalX(0), m_normalY(0), m_normalWidth(800), m_normalHeight(600) {
    
    strcpy_s(m_windowTitle, sizeof(m_windowTitle), "DearTs Application");
    strcpy_s(m_searchBuffer, sizeof(m_searchBuffer), "");
    
    // 注册窗口相关事件监听器
    registerEventHandlers();
}

/**
 * WindowManager析构函数
 * 取消事件监听器注册
 */
WindowManager::~WindowManager() {
    unregisterEventHandlers();
}

/**
 * 初始化窗口管理器
 * 获取Windows句柄并设置无边框样式
 */
bool WindowManager::initialize() {
    if (!m_sdlWindow) {
        std::cerr << "SDL Window is null" << std::endl;
        return false;
    }
    
    // 获取Windows窗口句柄
    m_hwnd = getHWND();
    if (!m_hwnd) {
        std::cerr << "Failed to get Windows handle" << std::endl;
        return false;
    }
    
    // 设置无边框样式
    setBorderlessStyle();
    
    // 保存初始窗口状态
    saveWindowState();
    
    // 发布窗口初始化完成事件
    WindowInitializedEvent::post(this);
    
    return true;
}

/**
 * 渲染自定义标题栏
 * 绘制标题栏UI元素，参考ImHex的现代化设计
 */
void WindowManager::renderTitleBar() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, m_titleBarHeight));
    
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                                   ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;
    
    // 设置标题栏样式 - 参考ImHex的现代化设计
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 6.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    
    // 设置标题栏背景色 - 深色主题
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    
    if (ImGui::Begin("##TitleBar", nullptr, window_flags)) {
        const float windowWidth = ImGui::GetWindowWidth();
        const float buttonHeight = m_titleBarHeight - 2.0f;
        const float buttonWidth = buttonHeight * 1.5f;
        
        // 窗口标题 - 左侧显示
        const char* title = m_windowTitle;
        ImVec2 titleSize = ImGui::CalcTextSize(title);
        
        ImGui::SetCursorPosX(12.0f);
        ImGui::SetCursorPosY((m_titleBarHeight - titleSize.y) * 0.5f);
        ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 1.0f), "%s", title);
        
        // 渲染搜索框
        renderSearchBox(windowWidth, buttonWidth);
        
        // 渲染窗口控制按钮
        renderWindowControls(windowWidth, buttonWidth, buttonHeight);
    }
    
    ImGui::End();
    
    // 恢复样式
    ImGui::PopStyleColor(1);
    ImGui::PopStyleVar(4);
    
    // 渲染搜索对话框
    if (m_showSearchDialog) {
        renderSearchDialog();
    }
}

/**
 * 处理窗口事件
 * 处理鼠标拖拽、按钮点击等事件
 */
void WindowManager::handleEvent(const SDL_Event& event) {
    switch (event.type) {
        case SDL_MOUSEBUTTONDOWN:
            if (event.button.button == SDL_BUTTON_LEFT) {
                int mouseX, mouseY;
                SDL_GetMouseState(&mouseX, &mouseY);
                if (isInTitleBarArea(mouseX, mouseY)) {
                    startDragging(mouseX, mouseY);
                }
            }
            break;
            
        case SDL_MOUSEBUTTONUP:
            if (event.button.button == SDL_BUTTON_LEFT) {
                stopDragging();
            }
            break;
            
        case SDL_MOUSEMOTION:
            if (m_isDragging) {
                updateDragging(event.motion.x, event.motion.y);
            }
            break;
            
        case SDL_KEYDOWN:
            handleKeyboardShortcuts(event.key);
            break;
    }
}

/**
 * 设置窗口标题
 */
void WindowManager::setWindowTitle(const char* title) {
    if (title) {
        strcpy_s(m_windowTitle, sizeof(m_windowTitle), title);
        SDL_SetWindowTitle(m_sdlWindow, title);
        
        // 发布标题更改事件
        WindowTitleChangedEvent::post(std::string(title));
    }
}

/**
 * 注册事件处理器
 */
void WindowManager::registerEventHandlers() {
    // 注册主题更改事件监听器
    ThemeChangedEvent::subscribe(this, [this](const std::string& themeName) {
        // 更新标题栏样式
        updateTitleBarStyle();
    });
    
    // 注册字体更改事件监听器
    FontChangedEvent::subscribe(this, [this](const std::string& fontName) {
        // 重新计算标题栏高度
        recalculateTitleBarHeight();
    });
}

/**
 * 取消事件处理器注册
 */
void WindowManager::unregisterEventHandlers() {
    ThemeChangedEvent::unsubscribe(this);
    FontChangedEvent::unsubscribe(this);
}

// 私有方法实现...

/**
 * 渲染搜索框
 */
void WindowManager::renderSearchBox(float windowWidth, float buttonsWidth) {
    const float searchBoxWidth = 200.0f;
    const float searchBoxHeight = m_titleBarHeight - 8.0f;
    const float searchBoxPosX = (windowWidth - searchBoxWidth) * 0.5f;
    
    ImVec2 titleSize = ImGui::CalcTextSize(m_windowTitle);
    
    // 确保搜索框不与标题和按钮重叠
    if (searchBoxPosX > titleSize.x + 30.0f && searchBoxPosX + searchBoxWidth < windowWidth - buttonsWidth - 20.0f) {
        ImGui::SetCursorPos(ImVec2(searchBoxPosX, (m_titleBarHeight - searchBoxHeight) * 0.5f));
        
        // 搜索框样式
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.12f, 0.12f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.25f, 0.25f, 0.9f));
        
        if (ImGui::Button("Search...", ImVec2(searchBoxWidth, searchBoxHeight))) {
            m_showSearchDialog = true;
            m_searchInputFocused = true;
        }
        
        ImGui::PopStyleColor(2);
    }
}




/**
 * 渲染窗口控制按钮
 */
void WindowManager::renderWindowControls(float windowWidth, float buttonWidth, float buttonHeight) {
    // 使用字体管理器的图标字体（如果可用）
    const auto fontManager = DearTs::Core::Resource::FontManager::getInstance();
    std::shared_ptr<DearTs::Core::Resource::FontResource> iconFont = nullptr;
    if (fontManager) {
        iconFont = fontManager->getFont("icons");
        if (iconFont) {
            DEARTS_LOG_INFO("Icon font found, pushing font");
            iconFont->pushFont();
        } else {
            DEARTS_LOG_INFO("Icon font not found, falling back to default font");
            // 如果图标字体不可用，回退到默认字体
            auto defaultFont = fontManager->getDefaultFont();
            if (defaultFont) {
                DEARTS_LOG_INFO("Default font found, pushing font");
                defaultFont->pushFont();
            } else {
                DEARTS_LOG_INFO("Default font not found");
            }
        }
    } else {
        DEARTS_LOG_INFO("Font manager not available");
    }

    // 标题栏按钮样式
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f)); // 透明背景
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
    
    // 最小化按钮
    ImGui::SetCursorPos(ImVec2(windowWidth - buttonWidth * 3, 1.0f));
    if (ImGui::Button(DearTs::Core::Resource::ICON_VS_CHROME_MINIMIZE.c_str(), ImVec2(buttonWidth, buttonHeight))) {
        minimizeWindow();
    }
    
    // 最大化/还原按钮
    ImGui::SetCursorPos(ImVec2(windowWidth - buttonWidth * 2, 1.0f));
    const char* maxButtonIcon = m_isMaximized ? DearTs::Core::Resource::ICON_VS_CHROME_RESTORE.c_str() : DearTs::Core::Resource::ICON_VS_CHROME_MAXIMIZE.c_str();  // 还原图标 / 最大化图标

    if (ImGui::Button(maxButtonIcon, ImVec2(buttonWidth, buttonHeight))) {
        toggleMaximize();
    }
    
    // 关闭按钮 - 特殊样式
    ImGui::PopStyleColor(3); // 弹出之前设置的3个颜色样式
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.9f, 0.1f, 0.1f, 1.0f));

    ImGui::SetCursorPos(ImVec2(windowWidth - buttonWidth, 0));
    if (ImGui::Button(DearTs::Core::Resource::ICON_VS_CHROME_CLOSE.c_str(), ImVec2(buttonWidth, buttonHeight))) {
        closeWindow();
    }
    ImGui::PopStyleColor(3); // 弹出关闭按钮的3个颜色样式
    
    // 恢复之前的字体
    if (iconFont) {
        DEARTS_LOG_INFO("Popping icon font");
        iconFont->popFont();
    } else if (fontManager) {
        auto defaultFont = fontManager->getDefaultFont();
        if (defaultFont) {
            DEARTS_LOG_INFO("Popping default font");
            defaultFont->popFont();
        }
    }
}

// 其他私有方法的实现...
// (为了保持文件长度合理，这里省略了其他方法的完整实现)

/**
 * 渲染搜索对话框
 * 显示全局搜索界面
 */
void WindowManager::renderSearchDialog() {
    if (!m_showSearchDialog) {
        return;
    }
    
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.3f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400, 100));
    
    if (ImGui::Begin("搜索", &m_showSearchDialog, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
        if (m_searchInputFocused) {
            ImGui::SetKeyboardFocusHere();
            m_searchInputFocused = false;
        }
        
        ImGui::InputText("##search", m_searchBuffer, sizeof(m_searchBuffer));
        
        if (ImGui::Button("搜索")) {
            // 执行搜索逻辑
            std::cout << "搜索: " << m_searchBuffer << std::endl;
            m_showSearchDialog = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("取消")) {
            m_showSearchDialog = false;
        }
    }
    ImGui::End();
}

/**
 * 处理键盘快捷键
 * @param key 键盘事件
 */
void WindowManager::handleKeyboardShortcuts(const SDL_KeyboardEvent& key) {
    if (key.type == SDL_KEYDOWN) {
        // Ctrl+F 打开搜索
        if ((key.keysym.mod & KMOD_CTRL) && key.keysym.sym == SDLK_f) {
            m_showSearchDialog = true;
            m_searchInputFocused = true;
        }
        // ESC 关闭搜索
        else if (key.keysym.sym == SDLK_ESCAPE) {
            m_showSearchDialog = false;
        }
    }
}

/**
 * 检查鼠标是否在标题栏区域
 * @param x 鼠标X坐标
 * @param y 鼠标Y坐标
 * @return 是否在标题栏区域
 */
bool WindowManager::isInTitleBarArea(int x, int y) const {
    return y >= 0 && y <= static_cast<int>(m_titleBarHeight);
}

/**
 * 开始拖拽窗口
 * @param mouseX 鼠标X坐标
 * @param mouseY 鼠标Y坐标
 */
void WindowManager::startDragging(int mouseX, int mouseY) {
    m_isDragging = true;
    m_dragOffsetX = mouseX;
    m_dragOffsetY = mouseY;
}

/**
 * 更新拖拽状态
 * @param mouseX 鼠标X坐标
 * @param mouseY 鼠标Y坐标
 */
void WindowManager::updateDragging(int mouseX, int mouseY) {
    if (m_isDragging && m_hwnd) {
        int windowX, windowY;
        SDL_GetWindowPosition(m_sdlWindow, &windowX, &windowY);
        
        int newX = windowX + (mouseX - m_dragOffsetX);
        int newY = windowY + (mouseY - m_dragOffsetY);
        
        SDL_SetWindowPosition(m_sdlWindow, newX, newY);
    }
}

/**
 * 停止拖拽窗口
 */
void WindowManager::stopDragging() {
    m_isDragging = false;
}

/**
 * 最小化窗口
 * 将窗口最小化到任务栏
 */
void WindowManager::minimizeWindow() {
    if (m_sdlWindow) {
        SDL_MinimizeWindow(m_sdlWindow);
    }
}

/**
 * 切换窗口最大化状态
 * 在最大化和正常状态之间切换
 */
void WindowManager::toggleMaximize() {
    if (!m_sdlWindow) return;
    
    if (m_isMaximized) {
        // 恢复到正常大小
        SDL_RestoreWindow(m_sdlWindow);
        SDL_SetWindowPosition(m_sdlWindow, m_normalX, m_normalY);
        SDL_SetWindowSize(m_sdlWindow, m_normalWidth, m_normalHeight);
        m_isMaximized = false;
    } else {
        // 保存当前窗口状态
        SDL_GetWindowPosition(m_sdlWindow, &m_normalX, &m_normalY);
        SDL_GetWindowSize(m_sdlWindow, &m_normalWidth, &m_normalHeight);
        
        // 最大化窗口
        SDL_MaximizeWindow(m_sdlWindow);
        m_isMaximized = true;
    }
}

/**
 * 关闭窗口
 * 发送窗口关闭事件
 */
void WindowManager::closeWindow() {
    if (m_sdlWindow) {
        SDL_Event closeEvent;
        closeEvent.type = SDL_QUIT;
        SDL_PushEvent(&closeEvent);
    }
}

/**
 * 获取Windows窗口句柄
 * @return Windows窗口句柄
 */
HWND WindowManager::getHWND() {
    if (!m_hwnd && m_sdlWindow) {
        SDL_SysWMinfo wmInfo;
        SDL_VERSION(&wmInfo.version);
        if (SDL_GetWindowWMInfo(m_sdlWindow, &wmInfo)) {
            m_hwnd = wmInfo.info.win.window;
        }
    }
    return m_hwnd;
}

/**
 * 设置无边框样式
 * 移除Windows默认的标题栏和边框
 */
void WindowManager::setBorderlessStyle() {
    HWND hwnd = getHWND();
    if (hwnd) {
        LONG style = GetWindowLong(hwnd, GWL_STYLE);
        style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
        SetWindowLong(hwnd, GWL_STYLE, style);
        
        LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
        exStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);
        SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);
        
        SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);
    }
}

/**
 * 保存窗口状态
 * 保存当前窗口的位置和大小信息
 */
void WindowManager::saveWindowState() {
    if (m_sdlWindow && !m_isMaximized) {
        SDL_GetWindowPosition(m_sdlWindow, &m_normalX, &m_normalY);
        SDL_GetWindowSize(m_sdlWindow, &m_normalWidth, &m_normalHeight);
    }
}

/**
 * 更新标题栏样式
 * 根据当前主题更新标题栏的外观
 */
void WindowManager::updateTitleBarStyle() {
    // 根据当前主题更新标题栏颜色和样式
    ImGuiStyle& style = ImGui::GetStyle();
    
    // 可以在这里根据主题调整标题栏的颜色
    // 例如：深色主题使用深色标题栏，浅色主题使用浅色标题栏
    if (style.Colors[ImGuiCol_WindowBg].x < 0.5f) {
        // 深色主题
        style.Colors[ImGuiCol_TitleBg] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
        style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
    } else {
        // 浅色主题
        style.Colors[ImGuiCol_TitleBg] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
        style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
    }
}

/**
 * 重新计算标题栏高度
 * 根据字体大小和DPI设置调整标题栏高度
 */
void WindowManager::recalculateTitleBarHeight() {
    ImGuiIO& io = ImGui::GetIO();
    float fontSize = io.FontDefault ? io.FontDefault->FontSize : 13.0f;
    float dpiScale = io.DisplayFramebufferScale.y;
    
    // 基础高度 + 字体大小 + DPI缩放 + 内边距
    m_titleBarHeight = (20.0f + fontSize * 1.5f) * dpiScale + 10.0f;
    
    // 确保最小高度
    if (m_titleBarHeight < 25.0f) {
        m_titleBarHeight = 25.0f;
    }
}

} // namespace DearTs::GUI