#include "title_bar_layout.h"
#include "../window_base.h"
#include "../utils/logger.h"
#include "../resource/font_resource.h"
#include "../resource/vscode_icons.hpp"
#include <imgui.h>
#include <SDL_syswm.h>
#include <iostream>
#include <algorithm>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace DearTs {
namespace Core {
namespace Window {

/**
 * TitleBarLayout构造函数
 */
TitleBarLayout::TitleBarLayout()
    : LayoutBase("TitleBar")
    , windowTitle_("DearTs Application")
    , isDragging_(false)
    , isMaximized_(false)
    , dragOffsetX_(0)
    , dragOffsetY_(0)
    , titleBarHeight_(30.0f)
    , showSearchDialog_(false)
    , searchInputFocused_(false)
    , normalX_(0)
    , normalY_(0)
    , normalWidth_(1280)
    , normalHeight_(720)
#if defined(_WIN32)
    , hwnd_(nullptr)
#endif
{
    memset(searchBuffer_, 0, sizeof(searchBuffer_));
}

/**
 * 渲染标题栏布局
 */
void TitleBarLayout::render() {
    if (!parentWindow_ || !parentWindow_->getSDLWindow()) {
        return;
    }
    
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, titleBarHeight_));
    
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | 
                                   ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | 
                                   ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | 
                                   ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus;
    
    // 设置标题栏样式
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 6.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f, 0.12f, 0.12f, 1.0f)); // 深色背景
    
    if (ImGui::Begin("##MainWindowTitleBar", nullptr, window_flags)) {
        // 渲染标题栏内容
        renderTitle();
        renderSearchBox();
        renderControlButtons();
        
        // 处理键盘快捷键
        handleKeyboardShortcuts();
    }
    
    ImGui::End();
    ImGui::PopStyleColor(); // WindowBg
    ImGui::PopStyleVar(4);
    
    // 渲染搜索对话框
    renderSearchDialog();
}

/**
 * 更新标题栏布局
 */
void TitleBarLayout::updateLayout(float width, float height) {
    // 标题栏宽度与窗口宽度一致，高度固定
    width_ = width;
    height_ = titleBarHeight_;
}

/**
 * 处理标题栏事件
 */
void TitleBarLayout::handleEvent(const SDL_Event& event) {
    switch (event.type) {
        case SDL_MOUSEBUTTONDOWN:
            if (event.button.button == SDL_BUTTON_LEFT) {
                if (isInTitleBarArea(event.button.x, event.button.y)) {
                    startDragging(event.button.x, event.button.y);
                }
            }
            break;
            
        case SDL_MOUSEBUTTONUP:
            if (event.button.button == SDL_BUTTON_LEFT) {
                stopDragging();
            }
            break;
            
        case SDL_MOUSEMOTION:
            if (isDragging_) {
                updateDragging(event.motion.x, event.motion.y);
            }
            break;
    }
}

/**
 * 设置窗口标题
 */
void TitleBarLayout::setWindowTitle(const std::string& title) {
    windowTitle_ = title;
}

/**
 * 保存窗口正常状态位置和大小
 */
void TitleBarLayout::saveNormalState(int x, int y, int width, int height) {
    normalX_ = x;
    normalY_ = y;
    normalWidth_ = width;
    normalHeight_ = height;
}

/**
 * 检查是否在标题栏区域
 */
bool TitleBarLayout::isInTitleBarArea(int x, int y) const {
    if (!parentWindow_ || !parentWindow_->getSDLWindow()) {
        return false;
    }
    
    int windowWidth, windowHeight;
    SDL_GetWindowSize(parentWindow_->getSDLWindow(), &windowWidth, &windowHeight);
    
    return x >= 0 && x <= windowWidth && y >= 0 && y <= static_cast<int>(titleBarHeight_);
}

/**
 * 开始拖拽窗口
 */
void TitleBarLayout::startDragging(int mouseX, int mouseY) {
    if (isMaximized_ || !parentWindow_ || !parentWindow_->getSDLWindow()) {
        return;
    }
    
    isDragging_ = true;
    
    // 计算鼠标相对于窗口左上角的偏移量
    dragOffsetX_ = mouseX;
    dragOffsetY_ = mouseY;
}

/**
 * 更新拖拽状态
 */
void TitleBarLayout::updateDragging(int mouseX, int mouseY) {
    if (!isDragging_ || isMaximized_ || !parentWindow_ || !parentWindow_->getSDLWindow()) {
        return;
    }
    
    // 获取窗口当前位置
    int windowX, windowY;
    SDL_GetWindowPosition(parentWindow_->getSDLWindow(), &windowX, &windowY);

    // 计算窗口应该移动到的新位置
    int targetX = windowX + (mouseX - dragOffsetX_);
    int targetY = windowY + (mouseY - dragOffsetY_);

    // 只有当位置确实发生变化时才移动窗口，避免不必要的窗口重绘导致抖动
    if (windowX != targetX || windowY != targetY) {
        SDL_SetWindowPosition(parentWindow_->getSDLWindow(), targetX, targetY);
    }
}

/**
 * 停止拖拽
 */
void TitleBarLayout::stopDragging() {
    isDragging_ = false;
}

/**
 * 最小化窗口
 */
void TitleBarLayout::minimizeWindow() {
    if (parentWindow_) {
        parentWindow_->minimize();
    }
}

/**
 * 最大化/还原窗口
 */
void TitleBarLayout::toggleMaximize() {
    if (!parentWindow_ || !parentWindow_->getSDLWindow()) return;
    
    if (isMaximized_) {
        // 还原窗口
        parentWindow_->setPosition(WindowPosition(normalX_, normalY_));
        parentWindow_->setSize(WindowSize(normalWidth_, normalHeight_));
        parentWindow_->restore();
        isMaximized_ = false;
    } else {
        // 保存当前状态并最大化
        auto pos = parentWindow_->getPosition();
        auto size = parentWindow_->getSize();
        saveNormalState(pos.x, pos.y, size.width, size.height);
        parentWindow_->maximize();
        isMaximized_ = true;
    }
}

/**
 * 关闭窗口
 */
void TitleBarLayout::closeWindow() {
    if (parentWindow_) {
        parentWindow_->close();
    }
}

/**
 * 渲染标题文本
 */
void TitleBarLayout::renderTitle() {
    const char* title = windowTitle_.c_str();
    
    // 使用字体管理器的默认字体（如果可用）
    auto fontManager = DearTs::Core::Resource::FontManager::getInstance();
    std::shared_ptr<DearTs::Core::Resource::FontResource> defaultFont = nullptr;
    if (fontManager) {
        defaultFont = fontManager->getDefaultFont();
        if (defaultFont) {
            defaultFont->pushFont();
        }
    }
    
    ImVec2 titleSize = ImGui::CalcTextSize(title);
    
    ImGui::SetCursorPosX(12.0f);
    ImGui::SetCursorPosY((titleBarHeight_ - titleSize.y) * 0.5f);
    ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 1.0f), "%s", title);
    
    // 恢复之前的字体
    if (defaultFont) {
        defaultFont->popFont();
    }
}

/**
 * 渲染搜索框
 */
void TitleBarLayout::renderSearchBox() {
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
        std::shared_ptr<DearTs::Core::Resource::FontResource> defaultFont = nullptr;
        if (fontManager) {
            defaultFont = fontManager->getDefaultFont();
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
        if (defaultFont) {
            defaultFont->popFont();
        }
    }
}

/**
 * 渲染窗口控制按钮
 */
void TitleBarLayout::renderControlButtons() {
    const float windowWidth = ImGui::GetWindowWidth();
    const float buttonHeight = titleBarHeight_ - 2.0f;
    const float buttonWidth = buttonHeight * 1.5f;
    
    // 使用字体管理器的图标字体（如果可用）
    auto fontManager = DearTs::Core::Resource::FontManager::getInstance();
    std::shared_ptr<DearTs::Core::Resource::FontResource> iconFont = nullptr;
    std::shared_ptr<DearTs::Core::Resource::FontResource> defaultFont = nullptr;
    if (fontManager) {
        // 首先尝试加载blendericons字体
        iconFont = fontManager->getFont("blendericons");
        if (!iconFont) {
            // 如果blendericons不可用，尝试加载icons字体
            iconFont = fontManager->getFont("icons");
        }
        if (iconFont) {
            iconFont->pushFont();
        } else {
            // 如果图标字体不可用，回退到默认字体
            defaultFont = fontManager->getDefaultFont();
            if (defaultFont) {
                defaultFont->pushFont();
            }
        }
    }
    
    // 标题栏按钮样式
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f)); // 透明背景
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
    
    // 最小化按钮
    ImGui::SetCursorPos(ImVec2(windowWidth - buttonWidth * 3, 0));
    if (ImGui::Button(DearTs::Core::Resource::ICON_VS_CHROME_MINIMIZE.c_str(), ImVec2(buttonWidth, buttonHeight))) {
        minimizeWindow();
    }
    
    // 最大化/还原按钮
    ImGui::SetCursorPos(ImVec2(windowWidth - buttonWidth * 2, 0));
    const char* maxButtonIcon = isMaximized_ ? DearTs::Core::Resource::ICON_VS_CHROME_RESTORE.c_str() : DearTs::Core::Resource::ICON_VS_CHROME_MAXIMIZE.c_str();
    if (ImGui::Button(maxButtonIcon, ImVec2(buttonWidth, buttonHeight))) {
        toggleMaximize();
    }
    
    // 关闭按钮 - 特殊样式
    ImGui::PopStyleColor(3);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.9f, 0.1f, 0.1f, 1.0f));
    
    ImGui::SetCursorPos(ImVec2(windowWidth - buttonWidth, 0));
    if (ImGui::Button(DearTs::Core::Resource::ICON_VS_CHROME_CLOSE.c_str(), ImVec2(buttonWidth, buttonHeight))) {
        closeWindow();
    }
    ImGui::PopStyleColor(3);
    
    // 恢复之前的字体
    if (iconFont) {
        iconFont->popFont();
    } else if (defaultFont) {
        defaultFont->popFont();
    }
}

/**
 * 渲染搜索对话框
 */
void TitleBarLayout::renderSearchDialog() {
    if (!showSearchDialog_) {
        return;
    }
    
    // 使用字体管理器的默认字体（如果可用）
    auto fontManager = DearTs::Core::Resource::FontManager::getInstance();
    std::shared_ptr<DearTs::Core::Resource::FontResource> defaultFont = nullptr;
    if (fontManager) {
        defaultFont = fontManager->getDefaultFont();
        if (defaultFont) {
            defaultFont->pushFont();
        }
    }
    
    // 获取主视口信息
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float dialogWidth = 300.0f;
    const float dialogHeight = 100.0f;
    
    // 将对话框定位在标题栏正下方并居中
    ImVec2 dialogPos = ImVec2(
        viewport->Pos.x + (viewport->Size.x - dialogWidth) * 0.5f,  // 水平居中
        viewport->Pos.y + titleBarHeight_  // 在标题栏下方
    );
    
    // 设置对话框位置和大小
    ImGui::SetNextWindowPos(dialogPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(dialogWidth, dialogHeight), ImGuiCond_Always);
    
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
        
        // 创建一个水平布局用于放置输入框和搜索按钮
        ImGui::BeginGroup();
        
        // 输入框占据大部分宽度
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 30.0f); // 留出按钮空间
        bool enterPressed = ImGui::InputTextWithHint("##search_input", "输入搜索内容...", 
                                                    searchBuffer_, sizeof(searchBuffer_), 
                                                    ImGuiInputTextFlags_EnterReturnsTrue);
        
        ImGui::SameLine();
        
        // 使用图标字体的搜索按钮
        // 获取图标字体
        std::shared_ptr<DearTs::Core::Resource::FontResource> iconFont = nullptr;
        if (fontManager) {
            iconFont = fontManager->getFont("icons");
            if (iconFont) {
                iconFont->pushFont();
            }
        }
        
        // 搜索按钮样式
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 1.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 1.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.5f, 1.0f, 1.0f));
        
        // 搜索按钮 (使用搜索图标)
        if (ImGui::Button(DearTs::Core::Resource::ICON_VS_SEARCH.c_str(), ImVec2(24, 24)) || enterPressed) {
            // 执行搜索逻辑
            if (strlen(searchBuffer_) > 0) {
                std::cout << "搜索内容: " << searchBuffer_ << std::endl;
            }
            showSearchDialog_ = false;
        }
        
        // 恢复之前的字体
        if (iconFont) {
            iconFont->popFont();
        }
        
        ImGui::PopStyleColor(3);
        ImGui::EndGroup();
        
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(2);
        
        // 搜索结果提示 (移除分隔线，使界面更简洁)
        if (strlen(searchBuffer_) > 0) {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "搜索: '%s'", searchBuffer_);
        }
    }
    ImGui::End();
    
    // 检测点击搜索弹窗外部区域时隐藏搜索框
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        ImVec2 mousePos = ImGui::GetMousePos();
        ImVec2 windowPos = dialogPos;
        ImVec2 windowSize = ImVec2(dialogWidth, dialogHeight);
        
        // 检查鼠标点击是否在搜索弹窗外部
        if (mousePos.x < windowPos.x || mousePos.x > windowPos.x + windowSize.x ||
            mousePos.y < windowPos.y || mousePos.y > windowPos.y + windowSize.y) {
            showSearchDialog_ = false;
        }
    }
    
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);
    
    // 恢复之前的字体
    if (defaultFont) {
        defaultFont->popFont();
    }
}

/**
 * 处理键盘快捷键
 */
void TitleBarLayout::handleKeyboardShortcuts() {
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

} // namespace Window
} // namespace Core
} // namespace DearTs