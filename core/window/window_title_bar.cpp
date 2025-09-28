#include "window_title_bar.h"
#include "window_manager.h"
#include "../utils/string_utils.h"
#include "../resource/font_resource.h"
#include <SDL_syswm.h>
#include <iostream>
#include <algorithm>

// ImGui includes
#include <imgui.h>

/**
 * WindowTitleBar构造函数
 * 初始化成员变量
 */
WindowTitleBar::WindowTitleBar(std::shared_ptr<DearTs::Core::Window::Window> window) 
    : window_(window)
    , sdlWindow_(nullptr)
    , hwnd_(nullptr)
    , isDragging_(false)
    , isMaximized_(false)
    , dragOffsetX_(0)
    , dragOffsetY_(0)
    , titleBarHeight_(30.0f)
    , showSearchDialog_(false)
    , searchInputFocused_(false)
    , normalX_(0)
    , normalY_(0)
    , normalWidth_(800)
    , normalHeight_(600) {
    
    if (window_) {
        sdlWindow_ = window_->getSDLWindow();
        windowTitle_ = window_->getTitle();
    }
    
    memset(searchBuffer_, 0, sizeof(searchBuffer_));
}

/**
 * WindowTitleBar析构函数
 */
WindowTitleBar::~WindowTitleBar() {
    // 清理资源
}

/**
 * 初始化窗口标题栏
 * 获取Windows句柄并设置无边框样式
 */
bool WindowTitleBar::initialize() {
    DEARTS_LOG_INFO("调用WindowTitleBar::initialize()");
    if (!sdlWindow_) {
        DEARTS_LOG_ERROR("SDL窗口为空");
        return false;
    }
    
    // 获取Windows窗口句柄
    hwnd_ = getHWND();
    if (!hwnd_) {
        DEARTS_LOG_ERROR("获取Windows句柄失败");
        return false;
    }
    
    // 设置无边框样式
    setBorderlessStyle();
    
    // 保存初始窗口状态
    saveWindowState();
    
    DEARTS_LOG_INFO("WindowTitleBar::initialize()成功完成");
    return true;
}

/**
 * 渲染自定义标题栏
 * 绘制标题栏UI元素，参考ImHex的现代化设计
 */
void WindowTitleBar::render() {
    DEARTS_LOG_INFO("调用WindowTitleBar::render()");
    // 检查ImGui是否已初始化
    if (!ImGui::GetCurrentContext()) {
        DEARTS_LOG_INFO("ImGui上下文为空，使用备用标题栏");
        // ImGui未初始化，使用SDL直接绘制简单标题栏
        // renderFallbackTitleBar();
        return;
    }
    
    // 检查是否在ImGui帧作用域内
    // 注意：这个检查可能会导致断言失败，所以我们只记录日志
    // if (!ImGui::GetCurrentContext()->WithinFrameScope) {
    //     DEARTS_LOG_WARN("ImGui不在帧作用域内");
    // }
    
    // 获取主视口
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    if (!viewport) {
        DEARTS_LOG_INFO("ImGui视口为空，使用备用标题栏");
        renderFallbackTitleBar();
        return;
    }
    
    DEARTS_LOG_INFO("渲染ImGui标题栏");
    // 设置标题栏窗口位置和大小
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, titleBarHeight_));
    
    // 标题栏窗口标志
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | 
                                   ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | 
                                   ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | 
                                   ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus;
    
    // 开始标题栏窗口
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 6.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f, 0.12f, 0.12f, 1.0f)); // 深色背景
    
    DEARTS_LOG_INFO("为标题栏调用ImGui::Begin");
    if (ImGui::Begin("##MainWindowTitleBar", nullptr, window_flags)) {
        // 渲染标题栏内容
        renderTitle();
        renderSearchBox();
        renderControlButtons();
    }
    DEARTS_LOG_INFO("标题栏的ImGui::Begin完成");

    
    // 处理键盘快捷键
    handleKeyboardShortcuts();
    
    // 渲染搜索对话框（如果需要）
    renderSearchDialog();
    
    ImGui::End();
    ImGui::PopStyleColor(); // WindowBg
    ImGui::PopStyleVar(4);
    DEARTS_LOG_INFO("WindowTitleBar::render()完成");
}

/**
 * 处理窗口事件
 * 处理鼠标拖拽等事件
 */
void WindowTitleBar::handleEvent(const SDL_Event& event) {
    DEARTS_LOG_INFO("调用WindowTitleBar::handleEvent()，事件类型: " + std::to_string(event.type));
    switch (event.type) {
        case SDL_MOUSEBUTTONDOWN:
            DEARTS_LOG_INFO("收到SDL_MOUSEBUTTONDOWN，按钮: " + std::to_string(event.button.button) + 
                           ", x: " + std::to_string(event.button.x) + ", y: " + std::to_string(event.button.y));
            if (event.button.button == SDL_BUTTON_LEFT) {
                if (isInTitleBarArea(event.button.x, event.button.y)) {
                    DEARTS_LOG_INFO("鼠标在标题栏区域内，开始拖拽");
                    // 使用窗口相对坐标进行拖拽
                    startDragging(event.button.x, event.button.y);
                } else {
                    DEARTS_LOG_INFO("鼠标不在标题栏区域内");
                }
            }
            break;
            
        case SDL_MOUSEBUTTONUP:
            DEARTS_LOG_INFO("收到SDL_MOUSEBUTTONUP，按钮: " + std::to_string(event.button.button));
            if (event.button.button == SDL_BUTTON_LEFT) {
                stopDragging();
            }
            break;
            
        case SDL_MOUSEMOTION:
            if (isDragging_) {
                DEARTS_LOG_INFO("拖拽过程中收到SDL_MOUSEMOTION，x: " + std::to_string(event.motion.x) + 
                               ", y: " + std::to_string(event.motion.y));
                // 使用窗口相对坐标进行拖拽更新
                updateDragging(event.motion.x, event.motion.y);
            }
            break;
    }
}

/**
 * 检查是否在标题栏区域
 */
bool WindowTitleBar::isInTitleBarArea(int x, int y) const {
    DEARTS_LOG_INFO("isInTitleBarArea() called with x: " + std::to_string(x) + ", y: " + std::to_string(y) + 
                   ", titleBarHeight_: " + std::to_string(titleBarHeight_));
    bool result = y >= 0 && y <= static_cast<int>(titleBarHeight_);
    DEARTS_LOG_INFO("isInTitleBarArea() result: " + std::to_string(result));
    return result;
}

/**
 * 开始拖拽窗口
 */
void WindowTitleBar::startDragging(int mouseX, int mouseY) {
    DEARTS_LOG_INFO("startDragging() called with mouseX: " + std::to_string(mouseX) + ", mouseY: " + std::to_string(mouseY));
    if (isMaximized_) {
        DEARTS_LOG_INFO("Window is maximized, not starting drag");
        return; // 最大化状态下不允许拖拽
    }
    
    isDragging_ = true;
    
    // 保存鼠标在窗口内的相对位置作为拖拽偏移量
    dragOffsetX_ = mouseX;
    dragOffsetY_ = mouseY;
    DEARTS_LOG_INFO("Drag started, dragOffsetX_: " + std::to_string(dragOffsetX_) + ", dragOffsetY_: " + std::to_string(dragOffsetY_));
}

/**
 * 更新拖拽状态
 */
void WindowTitleBar::updateDragging(int mouseX, int mouseY) {
    DEARTS_LOG_INFO("updateDragging() called with mouseX: " + std::to_string(mouseX) + ", mouseY: " + std::to_string(mouseY));
    if (!isDragging_ || isMaximized_ || !window_) {
        DEARTS_LOG_INFO("Not updating drag, isDragging_: " + std::to_string(isDragging_) + 
                       ", isMaximized_: " + std::to_string(isMaximized_) + ", window_: " + std::to_string(window_ ? 1 : 0));
        return;
    }
    
    // 获取当前窗口位置
    auto pos = window_->getPosition();
    int currentX = pos.x;
    int currentY = pos.y;
    DEARTS_LOG_INFO("Current window position: x: " + std::to_string(currentX) + ", y: " + std::to_string(currentY));
    
    // 计算鼠标移动的偏移量
    int deltaX = mouseX - dragOffsetX_;
    int deltaY = mouseY - dragOffsetY_;
    DEARTS_LOG_INFO("Delta: x: " + std::to_string(deltaX) + ", y: " + std::to_string(deltaY));
    
    // 计算新的窗口位置
    int newX = currentX + deltaX;
    int newY = currentY + deltaY;
    DEARTS_LOG_INFO("New window position: x: " + std::to_string(newX) + ", y: " + std::to_string(newY));
    
    // 获取屏幕尺寸进行边界检查
    SDL_DisplayMode displayMode;
    if (SDL_GetCurrentDisplayMode(0, &displayMode) == 0) {
        // 确保窗口不会完全移出屏幕
        auto size = window_->getSize();
        int windowWidth = size.width;
        int windowHeight = size.height;
        
        // 限制窗口位置，至少保留标题栏可见
        const int minVisibleArea = 50;
        newX = (std::max)(minVisibleArea - windowWidth, (std::min)(newX, displayMode.w - minVisibleArea));
        newY = (std::max)(0, (std::min)(newY, displayMode.h - minVisibleArea));
        DEARTS_LOG_INFO("Adjusted window position: x: " + std::to_string(newX) + ", y: " + std::to_string(newY));
    }
    
    // 移动窗口
    window_->setPosition(DearTs::Core::Window::WindowPosition(newX, newY));
    DEARTS_LOG_INFO("Window position set");
}

/**
 * 停止拖拽
 */
void WindowTitleBar::stopDragging() {
    DEARTS_LOG_INFO("stopDragging() called, isDragging_: " + std::to_string(isDragging_));
    isDragging_ = false;
}

/**
 * 最小化窗口
 */
void WindowTitleBar::minimizeWindow() {
    if (window_) {
        window_->minimize();
    }
}

/**
 * 最大化/还原窗口
 */
void WindowTitleBar::toggleMaximize() {
    if (!window_) return;
    
    if (isMaximized_) {
        // 还原窗口
        restoreWindowState();
        window_->restore();
        isMaximized_ = false;
    } else {
        // 保存当前状态并最大化
        saveWindowState();
        window_->maximize();
        isMaximized_ = true;
    }
}

/**
 * 关闭窗口
 */
void WindowTitleBar::closeWindow() {
    DEARTS_LOG_INFO("WindowTitleBar::closeWindow() called");
    if (window_) {
        DEARTS_LOG_INFO("Calling window_->close()");
        window_->close();
    } else {
        DEARTS_LOG_WARN("WindowTitleBar::closeWindow() - window_ is null");
    }
}

/**
 * 设置窗口标题
 */
void WindowTitleBar::setWindowTitle(const std::string& title) {
    windowTitle_ = title;
    if (window_) {
        window_->setTitle(title);
    }
}

/**
 * 获取Windows窗口句柄
 */
HWND WindowTitleBar::getHWND() {
    if (!sdlWindow_) return nullptr;
    
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    
    if (SDL_GetWindowWMInfo(sdlWindow_, &wmInfo)) {
        return wmInfo.info.win.window;
    }
    
    return nullptr;
}

/**
 * 设置窗口样式为无边框
 */
void WindowTitleBar::setBorderlessStyle() {
    if (!hwnd_) return;
    
    // 获取当前样式
    LONG style = GetWindowLong(hwnd_, GWL_STYLE);
    
    // 移除标题栏和边框
    style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
    
    // 应用新样式
    SetWindowLong(hwnd_, GWL_STYLE, style);
    
    // 刷新窗口
    SetWindowPos(hwnd_, nullptr, 0, 0, 0, 0, 
                 SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);
}

/**
 * 保存当前窗口状态
 */
void WindowTitleBar::saveWindowState() {
    if (window_ && !isMaximized_) {
        auto pos = window_->getPosition();
        auto size = window_->getSize();
        normalX_ = pos.x;
        normalY_ = pos.y;
        normalWidth_ = size.width;
        normalHeight_ = size.height;
    }
}

/**
 * 还原窗口状态
 */
void WindowTitleBar::restoreWindowState() {
    if (window_) {
        window_->setPosition(DearTs::Core::Window::WindowPosition(normalX_, normalY_));
        window_->setSize(DearTs::Core::Window::WindowSize(normalWidth_, normalHeight_));
    }
}

/**
 * 渲染搜索对话框
 * 实现搜索功能的UI界面，包括输入框、按钮和结果显示
 */
void WindowTitleBar::renderSearchDialog() {
    if (!showSearchDialog_) {
        return;
    }
    
    // 计算搜索框在标题栏中的位置
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float windowWidth = viewport->Size.x;
    const float searchBoxWidth = 200.0f;
    const float searchBoxPosX = (windowWidth - searchBoxWidth) * 0.5f;
    
    // 设置对话框位置
    ImVec2 searchBoxPos = ImVec2(viewport->Pos.x + searchBoxPosX, viewport->Pos.y + titleBarHeight_);
    ImGui::SetNextWindowPos(searchBoxPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(searchBoxWidth + 100, 150), ImGuiCond_Always);
    
    // 设置对话框样式
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.15f, 0.15f, 0.15f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.4f, 0.4f, 0.4f, 0.8f));
    
    // 开始搜索对话框窗口
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                            ImGuiWindowFlags_AlwaysAutoResize;
    
    if (ImGui::Begin("##SearchDialog", &showSearchDialog_, flags)) {
        // 搜索输入框 - 设置焦点
        if (searchInputFocused_) {
            ImGui::SetKeyboardFocusHere();
            searchInputFocused_ = false;
        }
        
        // 搜索输入框样式
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 4.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
        
        ImGui::SetNextItemWidth(-1); // 占满整个宽度
        bool enterPressed = ImGui::InputTextWithHint("##search_input", "输入搜索内容...", 
                                                    searchBuffer_, sizeof(searchBuffer_), 
                                                    ImGuiInputTextFlags_EnterReturnsTrue);
        
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(2);
        
        ImGui::Spacing();
        
        // 按钮区域
        float buttonWidth = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x * 2) / 3.0f;
        
        if (ImGui::Button("搜索", ImVec2(buttonWidth, 0)) || enterPressed) {
            // 执行搜索逻辑
            if (strlen(searchBuffer_) > 0) {
                // 这里可以添加实际的搜索功能
                std::cout << "搜索内容: " << searchBuffer_ << std::endl;
            }
        }
        
        ImGui::SameLine();
        if (ImGui::Button("清空", ImVec2(buttonWidth, 0))) {
            memset(searchBuffer_, 0, sizeof(searchBuffer_));
        }
        
        ImGui::SameLine();
        if (ImGui::Button("关闭", ImVec2(buttonWidth, 0))) {
            showSearchDialog_ = false;
        }
        
        // 搜索结果提示
        ImGui::Separator();
        if (strlen(searchBuffer_) > 0) {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "搜索: '%s'", searchBuffer_);
        } else {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "请输入搜索关键词");
        }
    }
    ImGui::End();
    
    // 检测点击搜索弹窗外部区域时隐藏搜索框
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        ImVec2 mousePos = ImGui::GetMousePos();
        ImVec2 windowPos = searchBoxPos;
        ImVec2 windowSize = ImVec2(searchBoxWidth + 100, 150);
        
        // 检查鼠标点击是否在搜索弹窗外部
        if (mousePos.x < windowPos.x || mousePos.x > windowPos.x + windowSize.x ||
            mousePos.y < windowPos.y || mousePos.y > windowPos.y + windowSize.y) {
            showSearchDialog_ = false;
        }
    }
    
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);
}

/**
 * 处理键盘快捷键
 * 实现Ctrl+F等快捷键功能
 */
void WindowTitleBar::handleKeyboardShortcuts() {
    // 检查Ctrl+F快捷键
    if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl)) {
        if (ImGui::IsKeyPressed(ImGuiKey_F)) {
            showSearchDialog_ = true;
            searchInputFocused_ = true;
        }
    }
    
    // 检查ESC键关闭搜索对话框
    if (showSearchDialog_ && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        showSearchDialog_ = false;
    }
}

/**
 * 渲染标题文本
 */
void WindowTitleBar::renderTitle() {
    const char* title = windowTitle_.c_str();
    
    // 使用字体管理器的默认字体（如果可用）
    auto fontManager = DearTs::Core::Resource::FontManager::getInstance();
    if (fontManager) {
        auto defaultFont = fontManager->getDefaultFont();
        if (defaultFont) {
            defaultFont->pushFont();
        }
    }
    
    ImVec2 titleSize = ImGui::CalcTextSize(title);
    
    ImGui::SetCursorPosX(12.0f);
    ImGui::SetCursorPosY((titleBarHeight_ - titleSize.y) * 0.5f);
    ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 1.0f), "%s", title);
    
    // 恢复之前的字体
    if (fontManager) {
        auto defaultFont = fontManager->getDefaultFont();
        if (defaultFont) {
            defaultFont->popFont();
        }
    }
}

/**
 * 渲染搜索框
 */
void WindowTitleBar::renderSearchBox() {
    const float windowWidth = ImGui::GetWindowWidth();
    const float searchBoxWidth = 200.0f;
    const float searchBoxHeight = titleBarHeight_ - 8.0f;
    const float buttonWidth = (titleBarHeight_ - 2.0f) * 1.5f;
    const float buttonsWidth = buttonWidth * 3; // 3个按钮
    const float searchBoxPosX = (windowWidth - searchBoxWidth) * 0.5f;
    
    ImVec2 titleSize = ImGui::CalcTextSize(windowTitle_.c_str());
    
    // 确保搜索框不与标题和按钮重叠
    if (searchBoxPosX > titleSize.x + 30.0f && searchBoxPosX + searchBoxWidth < windowWidth - buttonsWidth - 20.0f) {
        ImGui::SetCursorPos(ImVec2(searchBoxPosX, (titleBarHeight_ - searchBoxHeight) * 0.5f));
        
        // 使用字体管理器的默认字体（如果可用）
        auto fontManager = DearTs::Core::Resource::FontManager::getInstance();
        if (fontManager) {
            auto defaultFont = fontManager->getDefaultFont();
            if (defaultFont) {
                defaultFont->pushFont();
            }
        }
        
        // 搜索框样式
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.25f, 0.25f, 0.9f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 4.0f));
        
        // 搜索框按钮
        const char* searchDisplayText = strlen(searchBuffer_) > 0 ? searchBuffer_ : "搜索...";
        if (ImGui::Button(searchDisplayText, ImVec2(searchBoxWidth, searchBoxHeight))) {
            showSearchDialog_ = true;
            searchInputFocused_ = true;
        }
        
        // 搜索框工具提示
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("点击搜索或按 Ctrl+F");
            ImGui::EndTooltip();
        }
        
        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(3);
        
        // 恢复之前的字体
        if (fontManager) {
            auto defaultFont = fontManager->getDefaultFont();
            if (defaultFont) {
                defaultFont->popFont();
            }
        }
    }
}

/**
 * 渲染窗口控制按钮
 */
void WindowTitleBar::renderControlButtons() {
    const float windowWidth = ImGui::GetWindowWidth();
    const float buttonHeight = titleBarHeight_ - 2.0f;
    const float buttonWidth = buttonHeight * 1.5f;
    
    // 使用字体管理器的图标字体（如果可用）
    auto fontManager = DearTs::Core::Resource::FontManager::getInstance();
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
    ImGui::SetCursorPos(ImVec2(windowWidth - buttonWidth * 3, 0));
    if (ImGui::Button(DearTs::Core::Resource::ICON_VS_CHROME_MINIMIZE.c_str(), ImVec2(buttonWidth, buttonHeight))) {  // 最小化图标
        minimizeWindow();
    }
    
    // 最大化/还原按钮
    ImGui::SetCursorPos(ImVec2(windowWidth - buttonWidth * 2, 0));
    const char* maxButtonIcon = isMaximized_ ? DearTs::Core::Resource::ICON_VS_CHROME_RESTORE.c_str() : DearTs::Core::Resource::ICON_VS_CHROME_MAXIMIZE.c_str();  // 还原图标 / 最大化图标
    if (ImGui::Button(maxButtonIcon, ImVec2(buttonWidth, buttonHeight))) {
        toggleMaximize();
    }
    
    // 关闭按钮 - 特殊样式
    ImGui::PopStyleColor(3);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.9f, 0.1f, 0.1f, 1.0f));
    
    ImGui::SetCursorPos(ImVec2(windowWidth - buttonWidth, 0));
    if (ImGui::Button(DearTs::Core::Resource::ICON_VS_CHROME_CLOSE.c_str(), ImVec2(buttonWidth, buttonHeight))) {  // 关闭图标
        closeWindow();
    }
    ImGui::PopStyleColor(3);
    
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

/**
 * 渲染fallback标题栏（当ImGui未初始化时使用）
 */
void WindowTitleBar::renderFallbackTitleBar() {
    DEARTS_LOG_INFO("WindowTitleBar::renderFallbackTitleBar() called");
    // 在ImGui未初始化时，使用SDL直接绘制简单标题栏
    if (!sdlWindow_) {
        DEARTS_LOG_INFO("SDL Window is null in renderFallbackTitleBar");
        return;
    }
    
    // 获取渲染器
    SDL_Renderer* renderer = SDL_GetRenderer(sdlWindow_);
    if (!renderer) {
        DEARTS_LOG_INFO("SDL Renderer is null in renderFallbackTitleBar");
        return;
    }
    
    // 获取窗口大小
    int windowWidth, windowHeight;
    SDL_GetWindowSize(sdlWindow_, &windowWidth, &windowHeight);
    
    DEARTS_LOG_INFO("Rendering fallback title bar with size: " + std::to_string(windowWidth) + "x" + std::to_string(windowHeight));
    
    // 绘制标题栏背景
    SDL_SetRenderDrawColor(renderer, 64, 64, 64, 255);
    SDL_Rect titleBarRect = { 0, 0, windowWidth, static_cast<int>(titleBarHeight_) };
    SDL_RenderFillRect(renderer, &titleBarRect);
    
    // 绘制标题栏边框
    SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
    SDL_RenderDrawRect(renderer, &titleBarRect);
    
    // 绘制窗口标题文本
    SDL_Color textColor = { 255, 255, 255, 255 }; // 白色文本
    // 注意：这里简化处理，实际项目中可能需要使用SDL_ttf来渲染文本
    
    DEARTS_LOG_INFO("WindowTitleBar::renderFallbackTitleBar() completed");
}