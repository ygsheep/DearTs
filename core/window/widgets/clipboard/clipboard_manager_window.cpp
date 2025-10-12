#include "clipboard_manager_window.h"
#include "clipboard_history_layout.h"
#include "../../utils/logger.h"
#include "../../resource/font_resource.h"
#include <imgui.h>
#include <SDL_syswm.h>
#include <iostream>

namespace DearTs::Core::Window::Widgets::Clipboard {

/**
 * ClipboardManagerWindow构造函数
 */
ClipboardManagerWindow::ClipboardManagerWindow(const std::string& title)
    : WindowBase(title)
    , clipboard_layout_(nullptr)
    , is_visible_(false)
    , initialized_(false) {
    // 设置为标准无边框窗口
    setWindowMode(WindowMode::STANDARD);
    DEARTS_LOG_INFO("ClipboardManagerWindow构造函数: 设置为无边框窗口");
}

/**
 * ClipboardManagerWindow析构函数
 */
ClipboardManagerWindow::~ClipboardManagerWindow() {
    DEARTS_LOG_INFO("ClipboardManagerWindow析构函数");
    
    // 清理布局资源
    clipboard_layout_.reset();
}

/**
 * 初始化窗口
 */
bool ClipboardManagerWindow::initialize() {
    DEARTS_LOG_INFO("初始化剪切板管理器窗口: " + title_);

    // 调用基类初始化
    if (!WindowBase::initialize()) {
        DEARTS_LOG_ERROR("基类窗口初始化失败: " + title_);
        return false;
    }

    // 创建剪切板历史布局
    try {
        clipboard_layout_ = std::make_unique<ClipboardHistoryLayout>();
        if (!clipboard_layout_) {
            DEARTS_LOG_ERROR("创建剪切板历史布局失败");
            return false;
        }
        DEARTS_LOG_INFO("剪切板历史布局创建成功");
    } catch (const std::exception& e) {
        DEARTS_LOG_ERROR("创建剪切板历史布局异常: " + std::string(e.what()));
        return false;
    }

    // 设置窗口初始位置和大小
    auto initialPos = getPosition();
    auto initialSize = getSize();
    setPosition(initialPos);
    setSize(initialSize);

    // 默认隐藏窗口
    hideWindow();

    initialized_ = true;
    DEARTS_LOG_INFO("剪切板管理器窗口初始化成功: " + title_);
    return true;
}

/**
 * 渲染窗口内容
 */
void ClipboardManagerWindow::render() {
    if (!initialized_ || !is_visible_) {
        return;
    }

    // 使用字体推送机制来获得更好的渲染质量
    auto fontManager = DearTs::Core::Resource::FontManager::getInstance();
    std::shared_ptr<DearTs::Core::Resource::FontResource> defaultFont = nullptr;
    if (fontManager) {
        defaultFont = fontManager->getDefaultFont();
        if (defaultFont) {
            defaultFont->pushFont();
        }
    }

    // 设置ImGui窗口参数
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar |
                                 ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoCollapse |
                                 ImGuiWindowFlags_NoScrollbar |
                                 ImGuiWindowFlags_NoScrollWithMouse;

    // 设置深色主题背景
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f, 0.12f, 0.12f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.15f, 0.15f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));

    // 开始主窗口渲染
    ImGui::Begin("ClipboardManager", nullptr, windowFlags);
    
    // 渲染自定义标题栏
    renderCustomTitleBar();
    
    // 渲染剪切板历史布局
    if (clipboard_layout_) {
        // 为内容区域留出标题栏空间
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 40.0f);
        
        // 创建内容区域
        ImVec2 contentSize = ImVec2(
            ImGui::GetContentRegionAvail().x,
            ImGui::GetContentRegionAvail().y
        );
        
        ImGui::BeginChild("ClipboardContent", contentSize, false, 
                         ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        
        clipboard_layout_->render();
        
        ImGui::EndChild();
    }

    ImGui::End();
    
    // 恢复样式
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);

    // 恢复字体
    if (defaultFont) {
        defaultFont->popFont();
    }
}

/**
 * 更新窗口逻辑
 */
void ClipboardManagerWindow::update() {
    if (!initialized_ || !is_visible_) {
        return;
    }

    // 调用基类更新
    WindowBase::update();

    // 更新剪切板布局
    if (clipboard_layout_) {
        // 获取窗口大小并传递给布局
        auto windowSize = getSize();
        clipboard_layout_->updateLayout(windowSize.width, windowSize.height);

        // 启动剪切板监听（如果还未启动）
        SDL_Window* sdl_window = getSDLWindow();
        if (sdl_window) {
            clipboard_layout_->startClipboardMonitoring(sdl_window);
        }
    }
}

/**
 * 处理窗口事件
 */
void ClipboardManagerWindow::handleEvent(const SDL_Event& event) {
    if (!initialized_ || !is_visible_) {
        return;
    }

    // 调用基类事件处理
    WindowBase::handleEvent(event);

    // 处理鼠标事件
    handleMouseEvents(event);

    // 处理键盘事件
    handleKeyboardEvents(event);

    // 处理窗口事件
    handleWindowEvents(event);
}

/**
 * 显示窗口
 */
void ClipboardManagerWindow::showWindow() {
    if (!initialized_) {
        DEARTS_LOG_ERROR("窗口未初始化，无法显示");
        return;
    }

    is_visible_ = true;
    show();
    
    if (clipboard_layout_) {
        clipboard_layout_->setVisible(true);
        clipboard_layout_->refreshHistory();
    }
    
    DEARTS_LOG_INFO("剪切板管理器窗口已显示");
}

/**
 * 隐藏窗口
 */
void ClipboardManagerWindow::hideWindow() {
    is_visible_ = false;
    hide();
    
    if (clipboard_layout_) {
        clipboard_layout_->setVisible(false);
    }
    
    DEARTS_LOG_INFO("剪切板管理器窗口已隐藏");
}

/**
 * 切换窗口显示状态
 */
void ClipboardManagerWindow::toggleWindow() {
    if (is_visible_) {
        hideWindow();
    } else {
        showWindow();
    }
}

/**
 * 刷新历史记录
 */
void ClipboardManagerWindow::refreshHistory() {
    if (clipboard_layout_) {
        clipboard_layout_->refreshHistory();
        DEARTS_LOG_INFO("剪切板历史记录已刷新");
    }
}

/**
 * 清空历史记录
 */
void ClipboardManagerWindow::clearHistory() {
    if (clipboard_layout_) {
        clipboard_layout_->clearHistory();
        DEARTS_LOG_INFO("剪切板历史记录已清空");
    }
}


/**
 * 计算布局
 */
void ClipboardManagerWindow::calculateLayout() {
    // 窗口布局计算逻辑
    auto screenSize = getSize();

    // 设置合适的窗口大小（主窗口的70%）
    WindowSize targetSize(
        static_cast<int>(screenSize.width * 0.7f),
        static_cast<int>(screenSize.height * 0.8f)
    );

    // 居中显示
    WindowPosition targetPos(
        (screenSize.width - targetSize.width) / 2,
        (screenSize.height - targetSize.height) / 2
    );

    setPosition(targetPos);
    setSize(targetSize);
}

/**
 * 获取窗口位置
 */
ImVec2 ClipboardManagerWindow::getWindowPosition() const {
    auto pos = getPosition();
    return ImVec2(static_cast<float>(pos.x), static_cast<float>(pos.y));
}

/**
 * 获取窗口大小
 */
ImVec2 ClipboardManagerWindow::getWindowSize() const {
    auto size = getSize();
    return ImVec2(static_cast<float>(size.width), static_cast<float>(size.height));
}

/**
 * 渲染自定义标题栏
 */
void ClipboardManagerWindow::renderCustomTitleBar() {
    ImVec2 windowSize = getWindowSize();
    
    // 标题栏背景
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.08f, 0.08f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    
    ImGui::BeginChild("TitleBar", ImVec2(windowSize.x, 40.0f), false, 
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    
    // 标题文本
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.0f);
    ImGui::Text("剪切板管理器");
    
    // 关闭按钮
    ImVec2 closeButtonSize = ImVec2(30.0f, 30.0f);
    ImGui::SameLine(windowSize.x - closeButtonSize.x - 5.0f);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5.0f);
    
    if (ImGui::Button("✕", closeButtonSize)) {
        hideWindow();
    }
    
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

/**
 * 处理鼠标事件
 */
void ClipboardManagerWindow::handleMouseEvents(const SDL_Event& event) {
    switch (event.type) {
        case SDL_MOUSEBUTTONDOWN:
            if (event.button.button == SDL_BUTTON_LEFT) {
                // 处理左键点击
                ImVec2 mousePos = ImVec2(event.button.x, event.button.y);
                // 检查是否点击了标题栏
                if (mousePos.y < 40.0f) {
                    // 开始拖拽窗口
                    DEARTS_LOG_DEBUG("开始拖拽剪切板管理器窗口");
                }
            }
            break;
            
        case SDL_MOUSEBUTTONUP:
            if (event.button.button == SDL_BUTTON_LEFT) {
                // 结束拖拽
            }
            break;
            
        case SDL_MOUSEMOTION:
            if (event.motion.state & SDL_BUTTON_LMASK) {
                // 拖拽中
                auto currentPos = getPosition();
                WindowPosition newPos(
                    currentPos.x + event.motion.xrel,
                    currentPos.y + event.motion.yrel
                );
                setPosition(newPos);
            }
            break;
    }
}

/**
 * 处理键盘事件
 */
void ClipboardManagerWindow::handleKeyboardEvents(const SDL_Event& event) {
    switch (event.type) {
        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:
                    hideWindow();
                    DEARTS_LOG_INFO("ESC键按下，隐藏剪切板管理器窗口");
                    break;
                    
                case SDLK_F5:
                    refreshHistory();
                    DEARTS_LOG_INFO("F5键按下，刷新剪切板历史");
                    break;
            }
            break;
    }
}

/**
 * 处理窗口事件
 */
void ClipboardManagerWindow::handleWindowEvents(const SDL_Event& event) {
    switch (event.type) {
        case SDL_WINDOWEVENT:
            switch (event.window.event) {
                case SDL_WINDOWEVENT_FOCUS_GAINED:
                    DEARTS_LOG_DEBUG("剪切板管理器窗口获得焦点");
                    break;
                    
                case SDL_WINDOWEVENT_FOCUS_LOST:
                    DEARTS_LOG_DEBUG("剪切板管理器窗口失去焦点");
                    break;
                    
                case SDL_WINDOWEVENT_CLOSE:
                    hideWindow();
                    DEARTS_LOG_INFO("窗口关闭事件，隐藏剪切板管理器窗口");
                    break;
            }
            break;
    }
}

} // namespace DearTs::Core::Window::Widgets::Clipboard