#include "title_bar_layout.h"
#include "../window_base.h"
#include "../utils/logger.h"
#include "../resource/font_resource.h"
#include "../resource/material_symbols_icons.hpp"
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
    , iconTexture_(nullptr)
    , buttonClicked_(false)
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

    // 同步窗口状态
    updateWindowState();

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
                // 检查是否在标题栏区域（排除按钮区域）
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

    // 计算按钮区域
    const float buttonHeight = titleBarHeight_ - 2.0f;
    const float buttonWidth = buttonHeight * 1.5f;
    const float buttonsStartX = windowWidth - buttonWidth * 3; // 3个按钮的起始位置

    DEARTS_LOG_INFO("标题栏区域检测 - 鼠标坐标: (" + std::to_string(x) + "," + std::to_string(y) +
                   ") 窗口宽度: " + std::to_string(windowWidth) +
                   " 按钮区域: " + std::to_string(buttonsStartX) + "-" + std::to_string(windowWidth) +
                   " 标题栏高度: " + std::to_string(static_cast<int>(titleBarHeight_)));

    // 检查是否在标题栏高度范围内
    if (y < 0 || y > static_cast<int>(titleBarHeight_)) {
        DEARTS_LOG_INFO("鼠标超出标题栏高度范围");
        return false;
    }

    // 排除按钮区域（右侧3个按钮区域）
    if (x >= buttonsStartX && x <= windowWidth) {
        DEARTS_LOG_INFO("鼠标在按钮区域，不触发拖拽 - x: " + std::to_string(x) +
                       " 按钮区域起始: " + std::to_string(buttonsStartX));
        return false;
    }

    // 检查是否在窗口有效范围内
    bool inTitleArea = x >= 0 && x < buttonsStartX && y >= 0 && y <= static_cast<int>(titleBarHeight_);

    if (inTitleArea) {
        DEARTS_LOG_INFO("鼠标在标题栏区域，触发拖拽 - x: " + std::to_string(x) +
                       " y: " + std::to_string(y));
    } else {
        DEARTS_LOG_INFO("鼠标不在标题栏区域 - x: " + std::to_string(x) + " y: " + std::to_string(y));
    }

    return inTitleArea;
}

/**
 * 开始拖拽窗口
 */
void TitleBarLayout::startDragging(int mouseX, int mouseY) {
    DEARTS_LOG_INFO("!!! startDragging 被调用 - 参数: (" + std::to_string(mouseX) + "," + std::to_string(mouseY) + ") !!!");

    if (!parentWindow_ || !parentWindow_->getSDLWindow()) {
        return;
    }

// 使用原始SDL实现（无边框窗口模式）
    // 如果窗口处于最大化状态，先还原窗口
    bool actuallyMaximized = isActuallyMaximized();
    if (actuallyMaximized) {
        // 保存当前状态并还原窗口
        auto pos = parentWindow_->getPosition();
        auto size = parentWindow_->getSize();
        saveNormalState(pos.x, pos.y, size.width, size.height);

        // 还原窗口
        parentWindow_->restore();
        isMaximized_ = false;

        // 计算鼠标在新窗口位置中的相对位置
        // 让鼠标位于窗口标题栏的合理位置
        int windowWidth, windowHeight;
        SDL_GetWindowSize(parentWindow_->getSDLWindow(), &windowWidth, &windowHeight);

        // 估算鼠标在还原后窗口中的位置（基于鼠标在屏幕上的位置）
        int screenX, screenY;
        SDL_GetGlobalMouseState(&screenX, &screenY);

        // 计算窗口应该放置的位置，使鼠标位于标题栏的合理位置
        int newWindowX = screenX - std::min(mouseX, windowWidth - 100); // 确保鼠标不会太靠右
        int newWindowY = screenY - static_cast<int>(titleBarHeight_ / 2); // 鼠标位于标题栏中间

        // 设置窗口位置
        parentWindow_->setPosition(WindowPosition(newWindowX, newWindowY));

        // 更新拖拽偏移量（相对于新窗口位置）
        dragOffsetX_ = screenX - newWindowX;
        dragOffsetY_ = screenY - newWindowY;
    } else {
        // 正常的拖拽开始
        dragOffsetX_ = mouseX;
        dragOffsetY_ = mouseY;
    }

    isDragging_ = true;
    DEARTS_LOG_INFO("开始拖拽窗口，偏移量: (" + std::to_string(dragOffsetX_) + "," + std::to_string(dragOffsetY_) + ")");
}

/**
 * 更新拖拽状态
 */
void TitleBarLayout::updateDragging(int mouseX, int mouseY) {
    if (!isDragging_ || !parentWindow_ || !parentWindow_->getSDLWindow()) {
        return;
    }

    // 获取当前鼠标的全局位置
    int globalMouseX, globalMouseY;
    SDL_GetGlobalMouseState(&globalMouseX, &globalMouseY);

    // 计算窗口应该移动到的新位置
    int targetX = globalMouseX - dragOffsetX_;
    int targetY = globalMouseY - dragOffsetY_;

    // 获取窗口当前位置
    int windowX, windowY;
    SDL_GetWindowPosition(parentWindow_->getSDLWindow(), &windowX, &windowY);

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
    if (!parentWindow_) {
        return;
    }

// 使用原始SDL实现（无边框窗口模式）
    parentWindow_->minimize();
}

/**
 * 获取窗口的实际状态
 */
bool TitleBarLayout::isActuallyMaximized() const {
    if (!parentWindow_ || !parentWindow_->getSDLWindow()) {
        return false;
    }

    // 对于BORDERLESS窗口，SDL的MAXIMIZED标志可能不准确
    // 我们通过比较窗口大小和屏幕可用区域来判断是否最大化
    uint32_t flags = SDL_GetWindowFlags(parentWindow_->getSDLWindow());
    if (flags & SDL_WINDOW_MAXIMIZED) {
        return true;
    }

    // 手动检测：比较当前窗口大小与屏幕可用区域
    int displayIndex = SDL_GetWindowDisplayIndex(parentWindow_->getSDLWindow());
    SDL_Rect usableBounds;
    if (SDL_GetDisplayUsableBounds(displayIndex, &usableBounds) == 0) {
        auto currentPos = parentWindow_->getPosition();
        auto currentSize = parentWindow_->getSize();

        // 主要检测：窗口大小是否匹配屏幕可用区域
        bool sizeMatches = (currentSize.width == usableBounds.w && currentSize.height == usableBounds.h);

        // 位置检测：允许一定的误差（因为拖拽可能会稍微改变位置）
        int positionTolerance = 50; // 允许50像素的误差
        bool positionMatches = (abs(currentPos.x - usableBounds.x) <= positionTolerance &&
                              abs(currentPos.y - usableBounds.y) <= positionTolerance);
 
        // 如果大小匹配且位置接近屏幕可用区域，认为窗口处于最大化状态
        if (sizeMatches && positionMatches) {
            return true;
        }

        // 另外，如果窗口大小匹配屏幕可用区域，也认为是最大化状态（即使位置有较大偏差）
        if (sizeMatches) {
            return true;
        }
    }

    return false;
}

/**
 * 更新窗口状态（与SDL同步）
 */
void TitleBarLayout::updateWindowState() {
    if (!parentWindow_ || !parentWindow_->getSDLWindow()) {
        return;
    }

    // 同步窗口状态
    bool actuallyMaximized = isActuallyMaximized();
    if (isMaximized_ != actuallyMaximized) {
        isMaximized_ = actuallyMaximized;

        // 如果窗口刚被最大化，显示屏幕信息
        if (isMaximized_) {
            int displayIndex = SDL_GetWindowDisplayIndex(parentWindow_->getSDLWindow());
            SDL_Rect usableBounds;
            if (SDL_GetDisplayUsableBounds(displayIndex, &usableBounds) == 0) {
                DEARTS_LOG_INFO("最大化窗口屏幕区域: " + std::to_string(usableBounds.w) + "x" + std::to_string(usableBounds.h) +
                               " 位置: (" + std::to_string(usableBounds.x) + "," + std::to_string(usableBounds.y) + ")");
            }
        }
    }

    // 如果窗口被最大化了但之前没有保存正常状态，则保存当前状态
    if (isMaximized_ && (normalWidth_ == 0 || normalHeight_ == 0)) {
        auto pos = parentWindow_->getPosition();
        auto size = parentWindow_->getSize();
        saveNormalState(pos.x, pos.y, size.width, size.height);}
}

/**
 * 最大化/还原窗口
 */
void TitleBarLayout::toggleMaximize() {
    if (!parentWindow_ || !parentWindow_->getSDLWindow()) return;

    // 使用原始SDL实现（无边框窗口模式）
    DEARTS_LOG_INFO("=== 最大化按钮被点击 ===");

    // 检查实际窗口状态，而不是依赖手动维护的状态
    bool actuallyMaximized = isActuallyMaximized();

    if (actuallyMaximized) {
        // 还原窗口
        DEARTS_LOG_INFO("执行窗口还原 - 位置: (" + std::to_string(normalX_) + "," + std::to_string(normalY_) +
                       ") 大小: " + std::to_string(normalWidth_) + "x" + std::to_string(normalHeight_));

        // 获取还原前的窗口状态
        auto beforePos = parentWindow_->getPosition();
        auto beforeSize = parentWindow_->getSize();

        parentWindow_->setPosition(WindowPosition(normalX_, normalY_));

        parentWindow_->setSize(WindowSize(normalWidth_, normalHeight_));

        parentWindow_->restore();

        // 强制更新状态，而不是手动设置
        // isMaximized_ = false; // 移除手动设置，让 updateWindowState() 处理

        // 获取还原后的窗口状态
        auto afterPos = parentWindow_->getPosition();
        auto afterSize = parentWindow_->getSize();

    } else {
        // 保存当前状态并最大化
        auto pos = parentWindow_->getPosition();
        auto size = parentWindow_->getSize();
        saveNormalState(pos.x, pos.y, size.width, size.height);
        DEARTS_LOG_INFO("保存当前窗口状态 - 位置: (" + std::to_string(pos.x) + "," + std::to_string(pos.y) +
                       ") 大小: " + std::to_string(size.width) + "x" + std::to_string(size.height));

        // 对于BORDERLESS窗口，手动实现最大化
        // 获取显示器可用区域大小（实际的屏幕工作区域）
        int displayIndex = SDL_GetWindowDisplayIndex(parentWindow_->getSDLWindow());
        SDL_Rect usableBounds;
        if (SDL_GetDisplayUsableBounds(displayIndex, &usableBounds) == 0) {
            DEARTS_LOG_INFO("手动设置窗口最大化到屏幕可用区域: (" + std::to_string(usableBounds.x) + "," + std::to_string(usableBounds.y) +
                           ") 大小: " + std::to_string(usableBounds.w) + "x" + std::to_string(usableBounds.h));

            // 手动设置窗口位置和大小来模拟最大化
            parentWindow_->setPosition(WindowPosition(usableBounds.x, usableBounds.y));
            parentWindow_->setSize(WindowSize(usableBounds.w, usableBounds.h));

            DEARTS_LOG_INFO("窗口已手动最大化，保存的原始位置: (" + std::to_string(pos.x) + "," + std::to_string(pos.y) +
                           ") 原始大小: " + std::to_string(size.width) + "x" + std::to_string(size.height) +
                           " 屏幕可用区域: " + std::to_string(usableBounds.w) + "x" + std::to_string(usableBounds.h));
        } else {
            DEARTS_LOG_INFO("无法获取屏幕可用区域，使用SDL最大化");
            // 如果无法获取屏幕区域，回退到SDL最大化
            parentWindow_->maximize();
        }

    }

    // 强制同步窗口状态
    updateWindowState();
}

/**
 * 关闭窗口
 */
void TitleBarLayout::closeWindow() {
    if (!parentWindow_) {
        return;
    }

// 使用原始SDL实现（无边框窗口模式）
    parentWindow_->close();
}

/**
 * 渲染标题文本
 */
void TitleBarLayout::renderTitle() {
    // 加载图标纹理（如果尚未加载）
    if (!iconTexture_) {
        auto resourceManager = DearTs::Core::Resource::ResourceManager::getInstance();
        if (resourceManager) {
            // 尝试加载time-task.png图标
            iconTexture_ = resourceManager->getTexture("resources/icon.ico");
            // 如果time-task.png不存在，尝试加载time-data.png
            if (!iconTexture_) {
                iconTexture_ = resourceManager->getTexture("resources/icon/time-data.png");
            }
        }
    }
    
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
    
    // 渲染图标（如果已加载）
    float iconXPos = 8.0f;
    if (iconTexture_ && iconTexture_->getTexture()) {
        // 设置光标位置
        ImGui::SetCursorPosX(iconXPos);
        ImGui::SetCursorPosY((titleBarHeight_ - 16.0f) * 0.5f); // 假设图标高度为16px
        
        // 渲染图标（使用ImGui的Image函数）
        ImGui::Image((ImTextureID)iconTexture_->getTexture(), ImVec2(16.0f, 16.0f));
        
        // 更新标题文本的X位置，使其在图标右侧
        iconXPos += 20.0f; // 图标宽度(16) + 间距(4)
    }
    
    // 渲染标题文本
    ImGui::SetCursorPosX(iconXPos);
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
        // 首先尝试加载material_symbols字体
        iconFont = fontManager->getFont("material_symbols");
        if (!iconFont) {
            // 如果material_symbols不可用，尝试加载blendericons字体
            iconFont = fontManager->getFont("blendericons");
            if (!iconFont) {
                // 如果blendericons不可用，尝试加载icons字体
                iconFont = fontManager->getFont("icons");
            }
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
    if (ImGui::Button(DearTs::Core::Resource::ICON_MS_MINIMIZE.c_str(), ImVec2(buttonWidth, buttonHeight))) {
        DEARTS_LOG_INFO("最小化按钮被点击");
        buttonClicked_ = true;
        minimizeWindow();
    }
    
    // 最大化/还原按钮 - 使用实际窗口状态
    ImGui::SetCursorPos(ImVec2(windowWidth - buttonWidth * 2, 0));
    bool actuallyMaximized = isActuallyMaximized();
    const char* maxButtonIcon = actuallyMaximized ? DearTs::Core::Resource::ICON_MS_SELECT_WINDOW_2.c_str() : DearTs::Core::Resource::ICON_MS_CROP_SQUARE.c_str();

    // 使用PushID来确保按钮ID唯一，避免图标变化导致的ID冲突
    ImGui::PushID("maximize_button");
    // 添加更多调试信息
    DEARTS_LOG_INFO("渲染按钮 - 当前状态: " + std::string(actuallyMaximized ? "已最大化(显示还原图标)" : "正常(显示最大化图标)") +
                   " 图标字符: " + maxButtonIcon);

    // 使用固定前缀 + 状态标识符确保ID一致性
    std::string buttonId = actuallyMaximized ? "restore" : "maximize";
    std::string buttonLabel = std::string("##") + buttonId;

    if (ImGui::Button(buttonLabel.c_str(), ImVec2(buttonWidth, buttonHeight))) {
        DEARTS_LOG_INFO("最大化/还原按钮被按下！当前状态: " + std::string(actuallyMaximized ? "已最大化" : "正常"));
        buttonClicked_ = true;
        toggleMaximize();
    } else {
        // 检查鼠标是否悬停在按钮上
        if (ImGui::IsItemHovered()) {
            DEARTS_LOG_INFO("鼠标悬停在最大化/还原按钮上");
        }
    }

    // 在按钮区域中央绘制图标
    ImVec2 buttonPos = ImGui::GetItemRectMin();
    ImVec2 iconSize = ImGui::CalcTextSize(maxButtonIcon);
    ImVec2 iconPos = ImVec2(
        buttonPos.x + (buttonWidth - iconSize.x) * 0.5f,
        buttonPos.y + (buttonHeight - iconSize.y) * 0.5f
    );

    ImGui::SetCursorScreenPos(iconPos);
    ImGui::Text("%s", maxButtonIcon);

    ImGui::PopID();
    
    // 关闭按钮 - 特殊样式
    ImGui::PopStyleColor(3);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.9f, 0.1f, 0.1f, 1.0f));
    
    ImGui::SetCursorPos(ImVec2(windowWidth - buttonWidth, 0));
    if (ImGui::Button(DearTs::Core::Resource::ICON_MS_CLOSE.c_str(), ImVec2(buttonWidth, buttonHeight))) {
        DEARTS_LOG_INFO("关闭按钮被按下！");
        buttonClicked_ = true;
        closeWindow();
    } else {
        // 检查鼠标是否悬停在按钮上
        if (ImGui::IsItemHovered()) {
            DEARTS_LOG_INFO("鼠标悬停在关闭按钮上");
        }
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
            // 首先尝试加载material_symbols字体
            iconFont = fontManager->getFont("material_symbols");
            if (!iconFont) {
                // 如果material_symbols不可用，尝试加载icons字体
                iconFont = fontManager->getFont("icons");
            }
            if (iconFont) {
                iconFont->pushFont();
            }
        }
        
        // 搜索按钮样式
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 1.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 1.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.5f, 1.0f, 1.0f));
        
        // 搜索按钮 (使用搜索图标)
        if (ImGui::Button(DearTs::Core::Resource::ICON_MS_SEARCH.c_str(), ImVec2(24, 24)) || enterPressed) {
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