#include "main_window_optimized.h"
#include "layouts/title_bar_layout.h"
#include "layouts/sidebar_layout.h"
#include "layouts/pomodoro_layout.h"
#include "layouts/exchange_record_layout.h"
#include "widgets/clipboard/clipboard_history_layout.h"
#include "../utils/logger.h"
#include "../events/layout_events.h"
#include <SDL_syswm.h>
#include <iostream>

namespace DearTs {
namespace Core {
namespace Window {

// RAII字体管理器实现
MainWindow::FontManager::FontManager() {
    auto fontManager = DearTs::Core::Resource::FontManager::getInstance();
    if (fontManager) {
        font_ = fontManager->getDefaultFont();
        if (font_) {
            font_->pushFont();
        }
    }
}

MainWindow::FontManager::~FontManager() {
    if (font_) {
        font_->popFont();
    }
}

// 构造函数 - 最小化操作，避免在构造函数中调用复杂方法
MainWindow::MainWindow(const std::string& title)
    : WindowBase(title)
    , clearColor_(ImVec4(0.082f, 0.082f, 0.082f, 1.0f))  // ImGui Dark背景色
    , sidebarLayout_(nullptr)
    , clipboard_monitoring_started_(false) {
    DEARTS_LOG_INFO("MainWindow构造函数完成");
    std::cout << "[DEBUG] MainWindow构造函数被调用" << std::endl;
}

// 析构函数 - 简化
MainWindow::~MainWindow() {
    DEARTS_LOG_INFO("MainWindow析构函数");
}

// 初始化 - 确保正确的初始化顺序
bool MainWindow::initialize() {
    DEARTS_LOG_INFO("初始化主窗口: " + title_);

    // 首先设置窗口模式（在WindowBase初始化之前）
    setWindowMode(WindowMode::STANDARD);
    DEARTS_LOG_INFO("MainWindow: 设置为无边框窗口");

    // 然后调用基类初始化
    if (!WindowBase::initialize()) {
        DEARTS_LOG_ERROR("基类窗口初始化失败: " + title_);
        return false;
    }

    // 现在可以安全地调用布局相关方法
    DEARTS_LOG_INFO("MainWindow: 开始注册布局");
    registerLayouts();
    setupSidebarEventHandlers();

    // 设置标题栏窗口标题
    if (auto* titleBar = static_cast<TitleBarLayout*>(getLayoutManager().getLayout("TitleBar", getWindowId()))) {
        titleBar->setWindowTitle(title_);
    }

    DEARTS_LOG_INFO("主窗口初始化成功: " + title_);
    return true;
}

// 渲染 - 采用和原始版本相同的渲染逻辑
void MainWindow::render() {
    // RAII字体管理
    FontManager fontManager;

    // 记录渲染窗口ID用于调试
    std::string renderWindowId = getWindowId();
    DEARTS_LOG_INFO("!!! MainWindow::render 开始 - 使用窗口ID: " + renderWindowId);

    // 使用字体推送机制来获得更好的渲染质量
    auto fontManagerInstance = DearTs::Core::Resource::FontManager::getInstance();
    std::shared_ptr<DearTs::Core::Resource::FontResource> defaultFont = nullptr;
    if (fontManagerInstance) {
        defaultFont = fontManagerInstance->getDefaultFont();
        if (defaultFont) {
            defaultFont->pushFont();
        }
    }

    // 通过LayoutManager渲染当前窗口的所有布局，传递正确的窗口ID
    getLayoutManager().renderAll(getWindowId());

    // 计算内容区域参数
    ContentArea content = getContentArea();

    // 渲染当前内容布局（如果有的话）
    std::string currentLayout = getLayoutManager().getCurrentContentLayout();
    DEARTS_LOG_INFO("🎯 MainWindow渲染 - 当前布局: " + (currentLayout.empty() ? "无" : currentLayout) + " (窗口ID: " + getWindowId() + ")");

    if (!currentLayout.empty()) {
        LayoutBase* layout = getLayoutManager().getLayout(currentLayout, getWindowId());
        if (layout) {
            DEARTS_LOG_INFO("📋 布局存在: " + currentLayout + " 可见性: " + std::string(layout->isVisible() ? "✅可见" : "❌隐藏"));

            if (layout->isVisible()) {
                DEARTS_LOG_INFO("🎨 开始渲染固定内容区域 - 布局: " + currentLayout);
                // 创建固定的内容区域窗口
                ImGui::SetNextWindowPos(ImVec2(content.x, content.y));
                ImGui::SetNextWindowSize(ImVec2(content.width, content.height));

                ImGuiWindowFlags contentFlags = ImGuiWindowFlags_NoTitleBar |
                                               ImGuiWindowFlags_NoResize |
                                               ImGuiWindowFlags_NoMove |
                                               ImGuiWindowFlags_NoCollapse |
                                               ImGuiWindowFlags_NoBringToFrontOnFocus;

                // 设置内容区域背景色
                ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.082f, 0.082f, 0.082f, 1.0f));

                if (ImGui::Begin("##ContentArea", nullptr, contentFlags)) {
                    DEARTS_LOG_DEBUG("调用renderInFixedArea - 布局: " + currentLayout);
                    // 调用布局的固定区域渲染方法
                    layout->renderInFixedArea(content.x, content.y, content.width, content.height);
                    DEARTS_LOG_DEBUG("renderInFixedArea完成 - 布局: " + currentLayout);
                }
                ImGui::End();

                ImGui::PopStyleColor();
            } else {
                DEARTS_LOG_WARN("布局存在但不可见: " + currentLayout);
            }
        } else {
            DEARTS_LOG_ERROR("布局不存在: " + currentLayout);
        }
    } else {
        // 渲染默认内容
        DEARTS_LOG_INFO("🔄 渲染默认内容 (没有可见的内容布局)");
        renderDefaultContent();
    }

    // 恢复字体
    if (defaultFont) {
        defaultFont->popFont();
    }
}

// 更新 - 简化逻辑
void MainWindow::update() {
    WindowBase::update();

    // 更新标题栏
    if (auto* titleBar = static_cast<TitleBarLayout*>(getLayoutManager().getLayout("TitleBar", getWindowId()))) {
        titleBar->setWindowTitle(getTitle());
    }

    // 更新剪切板监听器
    updateClipboardMonitoring();
}

// 事件处理 - 简化
void MainWindow::handleEvent(const SDL_Event& event) {
    WindowBase::handleEvent(event);
}

// 内容区域计算 - 统一方法
MainWindow::ContentArea MainWindow::getContentArea() const {
    float sidebarWidth = 0.0f;
    if (sidebarLayout_) {
        sidebarWidth = sidebarLayout_->getCurrentWidth();
    }

    const float titleBarHeight = 30.0f;
    const auto& io = ImGui::GetIO();

    return {
        .x = sidebarWidth,
        .y = titleBarHeight,
        .width = io.DisplaySize.x - sidebarWidth,
        .height = io.DisplaySize.y - titleBarHeight
    };
}

// 统一的布局注册 - 添加安全检查和调试信息
void MainWindow::registerLayouts() {
    DEARTS_LOG_INFO("MainWindow::registerLayouts - 开始注册布局");

    // 检查this指针有效性
    DEARTS_LOG_INFO("MainWindow::registerLayouts - this指针: " +
                    std::to_string(reinterpret_cast<uintptr_t>(this)));

    // 获取layoutManager引用并检查
    try {
        auto& layoutManager = getLayoutManager();
        DEARTS_LOG_INFO("MainWindow::registerLayouts - 成功获取layoutManager引用");

        // 记录当前窗口ID用于调试
        std::string currentWindowId = getWindowId();
        std::cout << "[LAYOUT] MainWindow::registerLayouts - 当前窗口ID: " << currentWindowId << std::endl;

        // 关键修复：确保当前窗口设置为活跃窗口
        std::string windowId = getWindowId();
        DEARTS_LOG_INFO("准备设置活跃窗口为: " + windowId + " (MainWindow布局注册)");
        layoutManager.setActiveWindow(windowId);
        DEARTS_LOG_INFO("设置活跃窗口为: " + windowId + " (MainWindow布局注册) - 完成");

        // 检查layoutManager是否有效
        if (reinterpret_cast<uintptr_t>(&layoutManager) == 0xFFFFFFFFFFFFFFFF) {
            DEARTS_LOG_ERROR("MainWindow::registerLayouts - layoutManager指针无效!");
            return;
        }

        // 注意：不重新注册 TitleBar，使用 WindowBase 的注册
        // TitleBar 已经在 WindowBase::registerDefaultLayouts() 中注册了

        // 侧边栏布局
        DEARTS_LOG_INFO("MainWindow::registerLayouts - 注册侧边栏布局");
        LayoutRegistration sidebarReg("Sidebar", LayoutType::SYSTEM, LayoutPriority::HIGH);
        sidebarReg.factory = [this]() -> std::unique_ptr<LayoutBase> {
            auto sidebar = std::make_unique<SidebarLayout>();
            setupSidebarItems(sidebar.get());
            return sidebar;
        };
        sidebarReg.autoCreate = true;

        if (layoutManager.registerLayout(sidebarReg)) {
            DEARTS_LOG_INFO("MainWindow::registerLayouts - 侧边栏布局注册成功");
        } else {
            DEARTS_LOG_ERROR("MainWindow::registerLayouts - 侧边栏布局注册失败");
        }

        // 内容布局
        struct ContentLayout {
            std::string name;
            std::function<std::unique_ptr<LayoutBase>()> factory;
        };

        std::vector<ContentLayout> contentLayouts = {
            {
                "Pomodoro",
                []() -> std::unique_ptr<LayoutBase> {
                    return std::make_unique<PomodoroLayout>();
                }
            },
            {
                "ExchangeRecord",
                []() -> std::unique_ptr<LayoutBase> {
                    return std::make_unique<ExchangeRecordLayout>();
                }
            },
            {
                "ClipboardHelper",
                []() -> std::unique_ptr<LayoutBase> {
                    return std::make_unique<DearTs::Core::Window::Widgets::Clipboard::ClipboardHistoryLayout>();
                }
            }
        };

        for (const auto& layout : contentLayouts) {
            LayoutRegistration reg(layout.name, LayoutType::CONTENT, LayoutPriority::NORMAL);
            reg.factory = layout.factory;
            reg.autoCreate = true;
            reg.persistent = true;

            if (layoutManager.registerLayout(reg)) {
                DEARTS_LOG_INFO("MainWindow::registerLayouts - 内容布局注册成功: " + layout.name);

                // 设置依赖关系
                layoutManager.addLayoutDependency(layout.name, "Sidebar");
                layoutManager.addLayoutDependency(layout.name, "TitleBar");

                // 初始隐藏
                layoutManager.hideLayout(layout.name, "初始隐藏");
            } else {
                DEARTS_LOG_ERROR("MainWindow::registerLayouts - 内容布局注册失败: " + layout.name);
            }
        }

        // 获取侧边栏引用
        sidebarLayout_ = static_cast<SidebarLayout*>(layoutManager.getLayout("Sidebar", getWindowId()));
        if (sidebarLayout_) {
            DEARTS_LOG_INFO("MainWindow::registerLayouts - 侧边栏引用获取成功");
        } else {
            DEARTS_LOG_ERROR("MainWindow::registerLayouts - 侧边栏引用获取失败");
        }

        DEARTS_LOG_INFO("MainWindow::registerLayouts - 布局注册完成");

    } catch (const std::exception& e) {
        DEARTS_LOG_ERROR("MainWindow::registerLayouts - 异常: " + std::string(e.what()));
    } catch (...) {
        DEARTS_LOG_ERROR("MainWindow::registerLayouts - 未知异常");
    }
}

// 侧边栏项目设置 - 统一方法
void MainWindow::setupSidebarItems(SidebarLayout* sidebar) {
    if (!sidebar) return;

    // 高效工具组
    SidebarItem productivityItem("productivity", "高效工具", false, "高效工具", "", true);
    productivityItem.children = {
        SidebarItem("pomodoro", "番茄时钟", false, "番茄时钟"),
        SidebarItem("data-analysis", "数据分析", false, "数据分析")
    };
    sidebar->addItem(productivityItem);

    // 文本工具组
    SidebarItem textToolsItem("text-tools", "文本工具", false, "文本处理工具", "", true);
    textToolsItem.children = {
        SidebarItem("clipboard-helper", "剪切板管理器", false, "剪切板历史记录与分词分析工具")
    };
    sidebar->addItem(textToolsItem);

    // 鸣潮工具组
    SidebarItem wutheringWavesItem("wuthering-waves", "鸣潮", false, "鸣潮游戏工具", "", true);
    wutheringWavesItem.children = {
        SidebarItem("exchange-record", "换取记录", false, "声骸换取记录")
    };
    sidebar->addItem(wutheringWavesItem);
}

// 侧边栏事件处理 - 简化
void MainWindow::setupSidebarEventHandlers() {
    if (!sidebarLayout_) return;

    sidebarLayout_->initializeEventSystem();

    // 简化的布局切换逻辑 - 每次调用时获取layoutManager引用
    sidebarLayout_->setItemClickCallback([this](const std::string& itemId) {
        std::string layoutName = mapSidebarItemToLayout(itemId);
        if (!layoutName.empty()) {
            auto& layoutManager = getLayoutManager();  // 每次调用时获取引用
            layoutManager.switchToLayout(layoutName, true);

            // 特定布局的初始化
            if (itemId == "exchange-record") {
                if (auto* exchangeLayout = static_cast<ExchangeRecordLayout*>(layoutManager.getLayout("ExchangeRecord", getWindowId()))) {
                    if (exchangeLayout->hasGamePathConfiguration()) {
                        exchangeLayout->refreshUrlFromSavedPath();
                    } else {
                        exchangeLayout->startSearch();
                    }
                }
            } else if (itemId == "clipboard-helper") {
                if (auto* clipboardLayout = static_cast<DearTs::Core::Window::Widgets::Clipboard::ClipboardHistoryLayout*>(
                    layoutManager.getLayout("ClipboardHelper", getWindowId()))) {
                    clipboardLayout->refreshHistory();
                }
            }
        }
    });

    DEARTS_LOG_INFO("侧边栏事件处理设置完成");
}

/**
 * 渲染默认内容（当没有内容布局可见时）
 */
void MainWindow::renderDefaultContent() {
    // 获取内容区域
    ContentArea content = getContentArea();

    // 设置窗口位置和大小
    ImGui::SetNextWindowPos(ImVec2(content.x, content.y));
    ImGui::SetNextWindowSize(ImVec2(content.width, content.height));

    ImGuiWindowFlags mainContentFlags = ImGuiWindowFlags_NoTitleBar |
                                       ImGuiWindowFlags_NoResize |
                                       ImGuiWindowFlags_NoMove |
                                       ImGuiWindowFlags_NoCollapse |
                                       ImGuiWindowFlags_NoBringToFrontOnFocus;

    // 设置ImGui Dark样式背景色
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.082f, 0.082f, 0.082f, 1.0f));
    ImGui::Begin("DefaultContent", nullptr, mainContentFlags);

    ImGui::Text("DearTs 主窗口");
    ImGui::Text("应用程序平均 %.3f ms/帧 (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::Text("侧边栏宽度: %.1f", content.x);
    ImGui::Text("当前布局: %s", getLayoutManager().getCurrentContentLayout().empty() ? "无" : getLayoutManager().getCurrentContentLayout().c_str());
    ImGui::Separator();

    ImGui::Text("欢迎使用 DearTs!");
    ImGui::Text("请从左侧侧边栏选择功能模块。");

    ImGui::Separator();

    ImGui::Text("颜色选择:");
    ImGui::ColorEdit3("清屏颜色", (float*)&clearColor_);

    ImGui::Separator();

    if (ImGui::Button("关闭窗口")) {
        close();
    }

    ImGui::End();
    ImGui::PopStyleColor();
}

// 剪切板监听器更新 - 每次调用时获取layoutManager引用
void MainWindow::updateClipboardMonitoring() {
    if (clipboard_monitoring_started_) return;

    auto* clipboardLayout = static_cast<DearTs::Core::Window::Widgets::Clipboard::ClipboardHistoryLayout*>(
        getLayoutManager().getLayout("ClipboardHelper", getWindowId()));

    if (clipboardLayout && clipboardLayout->isVisible()) {
        if (SDL_Window* sdl_window = getSDLWindow()) {
            clipboardLayout->startClipboardMonitoring(sdl_window);
            clipboard_monitoring_started_ = true;
            DEARTS_LOG_INFO("剪切板监听器已启动");
        }
    }
}

// 项目映射 - 简化
std::string MainWindow::mapSidebarItemToLayout(const std::string& itemId) {
    static const std::unordered_map<std::string, std::string> mappings = {
        {"pomodoro", "Pomodoro"},
        {"exchange-record", "ExchangeRecord"},
        {"clipboard-helper", "ClipboardHelper"}
    };

    auto it = mappings.find(itemId);
    return (it != mappings.end()) ? it->second : "";
}

} // namespace Window
} // namespace Core
} // namespace DearTs