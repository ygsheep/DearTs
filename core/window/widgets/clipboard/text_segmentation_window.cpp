#include "text_segmentation_window.h"
#include "text_segmentation_layout.h"
#include "../../layouts/title_bar_layout.h"
#include "../../utils/logger.h"
#include "../../resource/font_resource.h"
#include <imgui.h>
#include <SDL_syswm.h>
#include <iostream>

// 包含WindowPosition和WindowSize定义
#include "../../window_manager.h"

namespace DearTs::Core::Window::Widgets::Clipboard {

/**
 * TextSegmentationWindow构造函数
 */
TextSegmentationWindow::TextSegmentationWindow(const std::string& title, const std::string& content)
    : WindowBase(title)
    , content_(content)
    , initialized_(false) {
    // 设置为标准无边框窗口
    setWindowMode(WindowMode::STANDARD);
    DEARTS_LOG_INFO("TextSegmentationWindow构造函数: 设置为无边框窗口，内容长度: " + std::to_string(content.length()));
}

/**
 * TextSegmentationWindow析构函数
 */
TextSegmentationWindow::~TextSegmentationWindow() {
    DEARTS_LOG_INFO("TextSegmentationWindow析构函数");

    // 布局资源由LayoutManager自动管理，无需手动清理
}

/**
 * 初始化窗口
 */
bool TextSegmentationWindow::initialize() {
    DEARTS_LOG_INFO("初始化分词助手窗口: " + title_);

    // 调用基类初始化
    if (!WindowBase::initialize()) {
        DEARTS_LOG_ERROR("基类窗口初始化失败: " + title_);
        return false;
    }

    // 计算并设置窗口初始位置和大小
    calculateLayout();

    initialized_ = true;
    DEARTS_LOG_INFO("分词助手窗口初始化成功: " + title_);
    return true;
}

/**
 * 渲染窗口内容
 */
void TextSegmentationWindow::render() {
    if (!initialized_ || !isVisible()) {
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

    // 调用基类渲染（通过LayoutManager渲染系统布局）
    WindowBase::render();

    // 恢复字体
    if (defaultFont) {
        defaultFont->popFont();
    }
}

/**
 * 更新窗口逻辑
 */
void TextSegmentationWindow::update() {
    if (!initialized_ || !isVisible()) {
        return;
    }

    // 调用基类更新（通过LayoutManager更新所有布局）
    WindowBase::update();
}

/**
 * 处理窗口事件
 */
void TextSegmentationWindow::handleEvent(const SDL_Event& event) {
    if (!initialized_ || !isVisible()) {
        return;
    }

    // 调用基类事件处理（包括通过LayoutManager的事件处理）
    WindowBase::handleEvent(event);
}

/**
 * 重写WindowBase的显示方法
 */
void TextSegmentationWindow::show() {
    // 调用基类方法，确保状态同步
    WindowBase::show();
    // 设置自定义状态
    showWindow();
}

/**
 * 重写WindowBase的隐藏方法
 */
void TextSegmentationWindow::hide() {
    // 调用基类方法，确保状态同步
    WindowBase::hide();
    // 设置自定义状态
    hideWindow();
}

/**
 * 显示窗口
 */
void TextSegmentationWindow::showWindow() {
    if (!initialized_) {
        DEARTS_LOG_ERROR("窗口未初始化，无法显示");
        return;
    }

    // 通过窗口作用域的LayoutManager显示分词布局
    auto& layoutManager = LayoutManager::getInstance();
    layoutManager.setActiveWindow(getWindowId());  // 设置当前窗口为活跃窗口
    layoutManager.showLayout("Segmentation", "用户请求显示");
    DEARTS_LOG_INFO("分词助手窗口显示逻辑已执行");
}

/**
 * 隐藏窗口
 */
void TextSegmentationWindow::hideWindow() {
    // 通过窗口作用域的LayoutManager隐藏分词布局
    auto& layoutManager = LayoutManager::getInstance();
    layoutManager.setActiveWindow(getWindowId());  // 设置当前窗口为活跃窗口
    layoutManager.hideLayout("Segmentation", "用户请求隐藏");
    DEARTS_LOG_INFO("分词助手窗口隐藏逻辑已执行");
}

/**
 * 切换窗口显示状态
 */
void TextSegmentationWindow::toggleWindow() {
    if (isVisible()) {
        hide();
    } else {
        show();
    }
}

/**
 * 设置内容
 */
void TextSegmentationWindow::setContent(const std::string& content) {
    content_ = content;

    try {
        auto& layoutManager = LayoutManager::getInstance();
        auto* layoutPtr = layoutManager.getWindowLayout(getWindowId(), "Segmentation");

        if (!layoutPtr) {
            DEARTS_LOG_ERROR("getWindowLayout返回nullptr，窗口ID: " + getWindowId() + ", 布局名称: Segmentation");
            return;
        }

        TextSegmentationLayout* layout = static_cast<TextSegmentationLayout*>(layoutPtr);
        if (!layout) {
            DEARTS_LOG_ERROR("布局类型转换失败，可能不是TextSegmentationLayout类型");
            return;
        }

        layout->setText(content);
        DEARTS_LOG_INFO("已设置分词内容: " + std::to_string(content.length()) + " 字符");

    } catch (const std::exception& e) {
        DEARTS_LOG_ERROR("设置分词内容时发生异常: " + std::string(e.what()));
    } catch (...) {
        DEARTS_LOG_ERROR("设置分词内容时发生未知异常");
    }
}

/**
 * 获取内容
 */
std::string TextSegmentationWindow::getContent() const {
    try {
        auto& layoutManager = LayoutManager::getInstance();
        auto* layoutPtr = layoutManager.getWindowLayout(getWindowId(), "Segmentation");

        if (!layoutPtr) {
            DEARTS_LOG_WARN("getWindowLayout返回nullptr，返回缓存内容，窗口ID: " + getWindowId());
            return content_;
        }

        const TextSegmentationLayout* layout = static_cast<const TextSegmentationLayout*>(layoutPtr);
        if (!layout) {
            DEARTS_LOG_WARN("布局类型转换失败，返回缓存内容");
            return content_;
        }

        return layout->getText();

    } catch (const std::exception& e) {
        DEARTS_LOG_ERROR("获取分词内容时发生异常: " + std::string(e.what()));
        return content_;
    } catch (...) {
        DEARTS_LOG_ERROR("获取分词内容时发生未知异常");
        return content_;
    }
}

/**
 * 静态创建方法
 */
std::unique_ptr<TextSegmentationWindow> TextSegmentationWindow::create(const std::string& content) {
    auto window = std::make_unique<TextSegmentationWindow>("分词助手", content);
    if (window && window->initialize()) {
        return window;
    }
    DEARTS_LOG_ERROR("创建分词助手窗口失败");
    return nullptr;
}

/**
 * 计算布局
 */
void TextSegmentationWindow::calculateLayout() {
    // 获取屏幕的实际尺寸，而不是当前窗口尺寸
    SDL_DisplayMode displayMode;
    if (SDL_GetCurrentDisplayMode(0, &displayMode) == 0) {
        // 设置合适的窗口大小（屏幕的60%宽，70%高）
        WindowSize targetSize(
            static_cast<int>(displayMode.w * 0.6f),
            static_cast<int>(displayMode.h * 0.7f)
        );

        // 居中显示
        WindowPosition targetPos(
            (displayMode.w - targetSize.width) / 2,
            (displayMode.h - targetSize.height) / 2
        );

        WindowBase::setPosition(targetPos);
        WindowBase::setSize(targetSize);

        DEARTS_LOG_INFO("分词窗口布局计算完成: " + std::to_string(targetSize.width) + "x" + std::to_string(targetSize.height) +
                       " 位置: (" + std::to_string(targetPos.x) + "," + std::to_string(targetPos.y) + ")");
    } else {
        DEARTS_LOG_ERROR("无法获取屏幕显示模式，使用默认尺寸");
        // 如果无法获取屏幕尺寸，使用默认值
        WindowBase::setSize(WindowSize(800, 600));
        WindowBase::setPosition(WindowPosition(100, 100));
    }
}


/**
 * 处理鼠标事件
 */
void TextSegmentationWindow::handleMouseEvents(const SDL_Event& event) {
    switch (event.type) {
        case SDL_MOUSEBUTTONDOWN:
            if (event.button.button == SDL_BUTTON_LEFT) {
                // 处理左键点击
                ImVec2 mousePos = ImVec2(event.button.x, event.button.y);
                // 检查是否点击了标题栏
                if (mousePos.y < 40.0f) {
                    // 开始拖拽窗口
                    DEARTS_LOG_DEBUG("开始拖拽分词助手窗口");
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
void TextSegmentationWindow::handleKeyboardEvents(const SDL_Event& event) {
    switch (event.type) {
        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:
                    hide();
                    DEARTS_LOG_INFO("ESC键按下，隐藏分词助手窗口");
                    break;
            }
            break;
    }
}

/**
 * 处理窗口事件
 */
void TextSegmentationWindow::handleWindowEvents(const SDL_Event& event) {
    switch (event.type) {
        case SDL_WINDOWEVENT:
            switch (event.window.event) {
                case SDL_WINDOWEVENT_FOCUS_GAINED:
                    DEARTS_LOG_DEBUG("分词助手窗口获得焦点");
                    break;

                case SDL_WINDOWEVENT_FOCUS_LOST:
                    DEARTS_LOG_DEBUG("分词助手窗口失去焦点");
                    break;

                case SDL_WINDOWEVENT_CLOSE:
                    hide();
                    DEARTS_LOG_INFO("窗口关闭事件，隐藏分词助手窗口");
                    break;
            }
            break;
    }
}

// === 布局注册方法实现 ===

void TextSegmentationWindow::registerDefaultLayouts() {
    DEARTS_LOG_INFO("注册分词助手默认布局");

    auto& layoutManager = getLayoutManager();

    // 关键修复：确保当前窗口设置为活跃窗口
    layoutManager.setActiveWindow(getWindowId());
    DEARTS_LOG_INFO("设置活跃窗口为: " + getWindowId() + " (布局注册)");

    // 注册分词内容布局
    LayoutRegistration segmentationReg("Segmentation", LayoutType::CONTENT, LayoutPriority::NORMAL);
    segmentationReg.factory = [this]() -> std::unique_ptr<LayoutBase> {
        auto layout = std::make_unique<TextSegmentationLayout>();

        // 设置初始内容
        if (!content_.empty()) {
            layout->setContent(content_);
        }

        return layout;
    };
    segmentationReg.autoCreate = true;
    segmentationReg.persistent = true;

    if (layoutManager.registerLayout(segmentationReg)) {
        DEARTS_LOG_INFO("分词布局注册成功: Segmentation");

        // 自动创建分词布局（类似基类的标题栏布局创建方式）
        if (layoutManager.createRegisteredLayout("Segmentation")) {
            DEARTS_LOG_INFO("分词布局创建成功: Segmentation");
        } else {
            DEARTS_LOG_ERROR("分词布局创建失败: Segmentation");
        }
    } else {
        DEARTS_LOG_ERROR("分词布局注册失败: Segmentation");
    }
}

} // namespace DearTs::Core::Window::Widgets::Clipboard