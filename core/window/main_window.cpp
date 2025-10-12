#include "main_window.h"
#include "layouts/title_bar_layout.h"
#include "layouts/sidebar_layout.h"
#include "layouts/pomodoro_layout.h"
#include "layouts/exchange_record_layout.h"
#include "../utils/logger.h"
#include "../resource/font_resource.h"
#include "../resource/IconsMaterialSymbols.h"
#include "../events/layout_events.h"

// 包含剪切板管理器布局
#include "widgets/clipboard/clipboard_history_layout.h"
#include <imgui.h>
#include <SDL_syswm.h>
#include <iostream>

namespace DearTs {
namespace Core {
namespace Window {

/**
 * MainWindow构造函数
 */
MainWindow::MainWindow(const std::string& title)
    : WindowBase(title)
    , clearColor_(0.45f, 0.55f, 0.60f, 1.00f)
    , layoutManager_(LayoutManager::getInstance())
    , sidebarLayout_(nullptr)
    , clipboard_monitoring_started_(false)
    , registeredLayoutIds_() {
    // 设置为无边框窗口模式，关闭Aero Snap以确保文件夹选择功能正常工作
    setWindowMode(WindowMode::STANDARD);
    DEARTS_LOG_INFO("MainWindow构造函数: 窗口模式设置为无边框窗口（关闭Aero Snap）");
}

/**
 * MainWindow析构函数
 */
MainWindow::~MainWindow() {
    DEARTS_LOG_INFO("MainWindow析构函数");

    // 清理引用
    sidebarLayout_ = nullptr;
}

/**
 * 初始化窗口
 */
bool MainWindow::initialize() {
    DEARTS_LOG_INFO("初始化主窗口: " + title_);

    // 调用基类初始化
    if (!WindowBase::initialize()) {
        DEARTS_LOG_ERROR("基类窗口初始化失败: " + title_);
        return false;
    }

    // 初始化布局管理器的事件系统
    layoutManager_.initializeEventSystem();
    layoutManager_.setParentWindow(this);

    // 使用新的布局注册机制
    registerAllLayoutTypes();
    setupLayoutDependencies();
    setupLayoutPriorities();
    initializeRegisteredLayouts();

    // 设置侧边栏事件处理
    setupSidebarEventHandlers();

    // 获取标题栏布局并设置窗口状态
    TitleBarLayout* titleBar = static_cast<TitleBarLayout*>(layoutManager_.getLayout("TitleBar"));
    if (titleBar) {
        auto pos = getPosition();
        auto size = getSize();
        titleBar->saveNormalState(pos.x, pos.y, size.width, size.height);
    }

    // 注意：剪切板监听器将在update()方法中启动，因为此时窗口可能还未完全创建
    clipboard_monitoring_started_ = false;

    DEARTS_LOG_INFO("主窗口初始化成功: " + title_);
    return true;
}

/**
 * 渲染窗口内容
 */
void MainWindow::render() {
    // 使用字体推送机制来获得更好的渲染质量
    auto fontManager = DearTs::Core::Resource::FontManager::getInstance();
    std::shared_ptr<DearTs::Core::Resource::FontResource> defaultFont = nullptr;
    if (fontManager) {
        defaultFont = fontManager->getDefaultFont();
        if (defaultFont) {
            defaultFont->pushFont();
        }
    }

    // 通过LayoutManager渲染所有布局
    layoutManager_.renderAll();

    // 计算内容区域参数
    float sidebarWidth = 0.0f;
    if (sidebarLayout_) {
        sidebarWidth = sidebarLayout_->getCurrentWidth();
    }
    const float titleBarHeight = 30.0f;
    const float contentX = sidebarWidth;
    const float contentY = titleBarHeight;
    const float contentWidth = ImGui::GetIO().DisplaySize.x - sidebarWidth;
    const float contentHeight = ImGui::GetIO().DisplaySize.y - titleBarHeight;

    // 渲染当前内容布局（如果有的话）
    std::string currentLayout = layoutManager_.getCurrentContentLayout();
    DEARTS_LOG_DEBUG("主窗口渲染 - 当前布局: " + (currentLayout.empty() ? "无" : currentLayout));

    if (!currentLayout.empty()) {
        LayoutBase* layout = layoutManager_.getLayout(currentLayout);
        if (layout) {
            DEARTS_LOG_DEBUG("布局存在: " + currentLayout + " 可见性: " + std::string(layout->isVisible() ? "可见" : "隐藏"));

            if (layout->isVisible()) {
                DEARTS_LOG_DEBUG("开始渲染固定内容区域 - 布局: " + currentLayout);
                // 创建固定的内容区域窗口
                ImGui::SetNextWindowPos(ImVec2(contentX, contentY));
                ImGui::SetNextWindowSize(ImVec2(contentWidth, contentHeight));

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
                    layout->renderInFixedArea(contentX, contentY, contentWidth, contentHeight);
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
        DEARTS_LOG_DEBUG("渲染默认内容");
        renderDefaultContent();
    }

    // 恢复字体
    if (defaultFont) {
        defaultFont->popFont();
    }
}

/**
 * 更新窗口逻辑
 */
void MainWindow::update() {
    // update方法被频繁调用，移除冗余日志输出
    // 调用基类更新
    WindowBase::update();
    
    // 更新标题栏窗口标题
    TitleBarLayout* titleBar = static_cast<TitleBarLayout*>(getLayout("TitleBar"));
    if (titleBar) {
        titleBar->setWindowTitle(getTitle());
    }
    
    // 更新所有布局
    float sidebarWidth = 0.0f;
    if (sidebarLayout_) {
        sidebarWidth = sidebarLayout_->getCurrentWidth();
    }

    layoutManager_.updateAll(ImGui::GetIO().DisplaySize.x - sidebarWidth,
                              ImGui::GetIO().DisplaySize.y - 30);

    // 更新剪切板管理器布局的监听器
    if (!clipboard_monitoring_started_) {
        auto* clipboardLayout = static_cast<DearTs::Core::Window::Widgets::Clipboard::ClipboardHistoryLayout*>(
            layoutManager_.getLayout("ClipboardHelper"));
        if (clipboardLayout && clipboardLayout->isVisible()) {
            SDL_Window* sdl_window = getSDLWindow();
            if (sdl_window) {
                clipboardLayout->startClipboardMonitoring(sdl_window);
                clipboard_monitoring_started_ = true;
                DEARTS_LOG_INFO("剪切板监听器已启动");
            } else {
                DEARTS_LOG_DEBUG("SDL窗口句柄仍不可用，将在下次尝试");
            }
        }
    }

    // 在这里可以添加自定义更新逻辑
}

/**
 * 处理窗口事件
 */
void MainWindow::handleEvent(const SDL_Event& event) {
    // 调用基类事件处理（会将事件传递给所有布局）
    WindowBase::handleEvent(event);

    // 在这里可以添加自定义事件处理逻辑
}

/**
 * 获取侧边栏布局
 */
SidebarLayout* MainWindow::getSidebarLayout() const {
    return static_cast<SidebarLayout*>(getLayout("Sidebar"));
}

/**
 * 获取内容区域的X坐标
 */
float MainWindow::getContentAreaX() const {
    SidebarLayout* sidebar = getSidebarLayout();
    if (sidebar) {
        return sidebar->getCurrentWidth();
    }
    return 0.0f;
}

/**
 * 获取内容区域的宽度
 */
float MainWindow::getContentAreaWidth() const {
    SidebarLayout* sidebar = getSidebarLayout();
    if (sidebar) {
        return ImGui::GetIO().DisplaySize.x - sidebar->getCurrentWidth();
    }
    return ImGui::GetIO().DisplaySize.x;
}

/**
 * 初始化系统布局（标题栏、侧边栏）
 */
void MainWindow::initializeSystemLayouts() {
    DEARTS_LOG_INFO("初始化系统布局");

    // 添加标题栏布局
    auto titleBarLayout = std::make_unique<TitleBarLayout>();
    titleBarLayout->setWindowTitle(title_);
    layoutManager_.addLayout("TitleBar", std::move(titleBarLayout));

    // 添加侧边栏布局
    auto sidebarLayout = std::make_unique<SidebarLayout>();

    // 创建"高效工具"可展开菜单项
    SidebarItem productivityItem("productivity", "高效工具", false, "高效工具", "", true);
    SidebarItem pomodoroItem("pomodoro", "番茄时钟", false, "番茄时钟");
    SidebarItem dataAnalysisItem("data-analysis", "数据分析", false, "数据分析");
    productivityItem.children.push_back(pomodoroItem);
    productivityItem.children.push_back(dataAnalysisItem);
    sidebarLayout->addItem(productivityItem);

    // 创建"文本工具"可展开菜单项
    SidebarItem textToolsItem("text-tools", "文本工具", false, "文本处理工具", "", true);
    SidebarItem clipboardItem("clipboard-helper", "剪切板管理器", false, "剪切板历史记录与分词分析工具");
    textToolsItem.children.push_back(clipboardItem);
    sidebarLayout->addItem(textToolsItem);

    // 创建"鸣潮"可展开菜单项
    SidebarItem wutheringWavesItem("wuthering-waves", "鸣潮", false, "鸣潮游戏工具", "", true);
    SidebarItem exchangeRecordItem("exchange-record", "换取记录", false, "声骸换取记录");
    wutheringWavesItem.children.push_back(exchangeRecordItem);
    sidebarLayout->addItem(wutheringWavesItem);

    // 添加到LayoutManager并获取引用
    layoutManager_.addLayout("Sidebar", std::move(sidebarLayout));
    sidebarLayout_ = static_cast<SidebarLayout*>(layoutManager_.getLayout("Sidebar"));

    DEARTS_LOG_INFO("系统布局初始化完成");
}

/**
 * 初始化内容布局（番茄时钟、换取记录、剪切板管理等）
 */
void MainWindow::initializeContentLayouts() {
    DEARTS_LOG_INFO("初始化内容布局");

    // 创建番茄时钟布局
    auto pomodoroLayout = std::make_unique<PomodoroLayout>();
    pomodoroLayout->setVisible(false); // 默认隐藏
    layoutManager_.addLayout("Pomodoro", std::move(pomodoroLayout));

    // 创建换取记录布局
    auto exchangeRecordLayout = std::make_unique<ExchangeRecordLayout>();
    exchangeRecordLayout->setVisible(false); // 默认隐藏
    layoutManager_.addLayout("ExchangeRecord", std::move(exchangeRecordLayout));

    // 创建剪切板管理器布局
    auto clipboardLayout = std::make_unique<DearTs::Core::Window::Widgets::Clipboard::ClipboardHistoryLayout>();
    clipboardLayout->setVisible(false); // 默认隐藏
    layoutManager_.addLayout("ClipboardHelper", std::move(clipboardLayout));

    DEARTS_LOG_INFO("内容布局初始化完成");
}

/**
 * 设置侧边栏事件处理
 */
void MainWindow::setupSidebarEventHandlers() {
    if (!sidebarLayout_) {
        DEARTS_LOG_ERROR("侧边栏布局未初始化，无法设置事件处理");
        return;
    }

    // 初始化侧边栏事件系统
    sidebarLayout_->initializeEventSystem();

    // 设置侧边栏状态变化回调
    sidebarLayout_->setStateCallback([this](bool isExpanded, float currentWidth) {
        DEARTS_LOG_INFO("侧边栏状态变化 - 展开: " + std::string(isExpanded ? "是" : "否") +
                       ", 宽度: " + std::to_string(currentWidth));
    });

    // 订阅侧边栏事件
    sidebarLayout_->subscribeSidebarEvent(Events::EventType::EVT_LAYOUT_SWITCH_REQUEST,
        [this](const Events::Event& event) -> bool {
            // 处理来自侧边栏的布局切换请求
            try {
                // 尝试转换为LayoutEvent
                const auto* layoutEvent = dynamic_cast<const Events::LayoutEvent*>(&event);
                if (layoutEvent) {
                    // 获取布局切换数据
                    const auto& eventData = layoutEvent->getEventData();
                    if (auto* switchData = std::get_if<Events::LayoutSwitchData>(&eventData)) {
                        std::string targetLayout = switchData->toLayout;
                        DEARTS_LOG_INFO("处理布局切换请求: " + switchData->fromLayout + " -> " + targetLayout);

                        // 映射侧边栏项目ID到布局名称
                        std::string layoutName = mapSidebarItemToLayout(targetLayout);
                        if (!layoutName.empty()) {
                            // 切换到目标布局
                            bool success = layoutManager_.switchToLayout(layoutName, switchData->animated);
                            if (success) {
                                DEARTS_LOG_INFO("布局切换成功: " + layoutName);
                            } else {
                                DEARTS_LOG_ERROR("布局切换失败: " + layoutName);
                            }
                            return success;
                        } else {
                            DEARTS_LOG_WARN("未找到对应的布局: " + targetLayout);
                        }
                    }
                } else {
                    // 如果不是LayoutEvent，尝试简单的字符串处理
                    DEARTS_LOG_DEBUG("收到非LayoutEvent，尝试简化处理");
                }
                return false;
            } catch (const std::exception& e) {
                DEARTS_LOG_ERROR("处理布局切换事件异常: " + std::string(e.what()));
                return false;
            }
        });

    // 设置项目点击回调（保持向后兼容性，主要负责特定布局的初始化）
    sidebarLayout_->setItemClickCallback([this](const std::string& itemId) {
        DEARTS_LOG_DEBUG("侧边栏项目点击回调: " + itemId);

        // 直接进行布局切换（简化版本）
        std::string layoutName = mapSidebarItemToLayout(itemId);
        if (!layoutName.empty()) {
            bool success = layoutManager_.switchToLayout(layoutName, true);
            if (success) {
                DEARTS_LOG_INFO("通过回调切换布局成功: " + layoutName);
            } else {
                DEARTS_LOG_ERROR("通过回调切换布局失败: " + layoutName);
            }
        }

        // 执行布局特定的初始化逻辑
        if (itemId == "exchange-record") {
            auto* exchangeLayout = static_cast<ExchangeRecordLayout*>(layoutManager_.getLayout("ExchangeRecord"));
            if (exchangeLayout) {
                if (exchangeLayout->hasGamePathConfiguration()) {
                    DEARTS_LOG_INFO("存在游戏路径配置，重新搜索最新URL");
                    exchangeLayout->refreshUrlFromSavedPath();
                } else {
                    DEARTS_LOG_INFO("无游戏路径配置，开始自动搜索");
                    exchangeLayout->startSearch();
                }
            }
        } else if (itemId == "clipboard-helper") {
            auto* clipboardLayout = static_cast<DearTs::Core::Window::Widgets::Clipboard::ClipboardHistoryLayout*>(
                layoutManager_.getLayout("ClipboardHelper"));
            if (clipboardLayout) {
                clipboardLayout->refreshHistory();
            }
        }
    });

    DEARTS_LOG_INFO("侧边栏事件驱动处理设置完成");
}

/**
 * 渲染默认内容（当没有内容布局可见时）
 */
void MainWindow::renderDefaultContent() {
    // 获取侧边栏宽度以调整主内容区域
    float sidebarWidth = 0.0f;
    // 主窗口内容相关
    static bool showDemoWindow_ = false; ///< 是否显示ImGui演示窗口
    static bool showAnotherWindow_ = false; ///< 是否显示另一个窗口
    if (sidebarLayout_) {
        sidebarWidth = sidebarLayout_->getCurrentWidth();
    }

    // 设置窗口位置和大小，为侧边栏留出空间
    ImGui::SetNextWindowPos(ImVec2(sidebarWidth, 30)); // 30是标题栏高度
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x - sidebarWidth,
                                   ImGui::GetIO().DisplaySize.y - 30));

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
    ImGui::Text("侧边栏宽度: %.1f", sidebarWidth);
    ImGui::Text("侧边栏状态: %s", sidebarLayout_ && sidebarLayout_->isExpanded() ? "展开" : "收起");
    ImGui::Text("当前布局: %s", layoutManager_.getCurrentContentLayout().empty() ? "无" : layoutManager_.getCurrentContentLayout().c_str());
    ImGui::Separator();

    ImGui::Text("欢迎使用 DearTs!");
    ImGui::Text("请从左侧侧边栏选择功能模块。");

    ImGui::Separator();

    ImGui::Text("颜色选择:");
    ImGui::ColorEdit3("清屏颜色", (float*)&clearColor_);

    ImGui::Separator();

    ImGui::Checkbox("显示ImGui演示", &showDemoWindow_);
    ImGui::Checkbox("显示另一个窗口", &showAnotherWindow_);

    ImGui::Separator();

    if (ImGui::Button("关闭窗口")) {
        close();
    }

    ImGui::End();
    ImGui::PopStyleColor();

      // 显示演示窗口
    if (showDemoWindow_) {
      ImGui::ShowDemoWindow(&showDemoWindow_);
    }

    if (showAnotherWindow_) {
      ImGui::Begin("另一个窗口", &showAnotherWindow_);
      ImGui::Text("这是另一个窗口!");
      if (ImGui::Button("关闭我")) {
        showAnotherWindow_ = false;
      }
      ImGui::End();
    }
}

// === 布局注册和初始化方法实现 ===

void MainWindow::registerAllLayoutTypes() {
    DEARTS_LOG_INFO("注册所有布局类型");

    registerSystemLayoutTypes();
    registerContentLayoutTypes();

    DEARTS_LOG_INFO("所有布局类型注册完成");
}

void MainWindow::registerSystemLayoutTypes() {
    DEARTS_LOG_INFO("注册系统布局类型");

    // 注册标题栏布局
    LayoutRegistration titleBarReg("TitleBar", LayoutType::SYSTEM, LayoutPriority::HIGHEST);
    titleBarReg.factory = []() -> std::unique_ptr<LayoutBase> {
        return std::make_unique<TitleBarLayout>();
    };
    titleBarReg.autoCreate = true;
    titleBarReg.persistent = false;

    if (layoutManager_.registerLayout(titleBarReg)) {
        registeredLayoutIds_.push_back("TitleBar");
    }

    // 注册侧边栏布局
    LayoutRegistration sidebarReg("Sidebar", LayoutType::SYSTEM, LayoutPriority::HIGH);
    sidebarReg.factory = [this]() -> std::unique_ptr<LayoutBase> {
        auto sidebar = std::make_unique<SidebarLayout>();

        // 创建"高效工具"可展开菜单项
        SidebarItem productivityItem("productivity", "高效工具", false, "高效工具", "", true);
        SidebarItem pomodoroItem("pomodoro", "番茄时钟", false, "番茄时钟");
        SidebarItem dataAnalysisItem("data-analysis", "数据分析", false, "数据分析");
        productivityItem.children.push_back(pomodoroItem);
        productivityItem.children.push_back(dataAnalysisItem);
        sidebar->addItem(productivityItem);

        // 创建"文本工具"可展开菜单项
        SidebarItem textToolsItem("text-tools", "文本工具", false, "文本处理工具", "", true);
        SidebarItem clipboardItem("clipboard-helper", "剪切板管理器", false, "剪切板历史记录与分词分析工具");
        textToolsItem.children.push_back(clipboardItem);
        sidebar->addItem(textToolsItem);

        // 创建"鸣潮"可展开菜单项
        SidebarItem wutheringWavesItem("wuthering-waves", "鸣潮", false, "鸣潮游戏工具", "", true);
        SidebarItem exchangeRecordItem("exchange-record", "换取记录", false, "声骸换取记录");
        wutheringWavesItem.children.push_back(exchangeRecordItem);
        sidebar->addItem(wutheringWavesItem);

        return sidebar;
    };
    sidebarReg.autoCreate = true;
    sidebarReg.persistent = false;

    if (layoutManager_.registerLayout(sidebarReg)) {
        registeredLayoutIds_.push_back("Sidebar");
    }

    DEARTS_LOG_INFO("系统布局类型注册完成");
}

void MainWindow::registerContentLayoutTypes() {
    DEARTS_LOG_INFO("注册内容布局类型");

    // 注册番茄时钟布局
    LayoutRegistration pomodoroReg("Pomodoro", LayoutType::CONTENT, LayoutPriority::NORMAL);
    pomodoroReg.factory = []() -> std::unique_ptr<LayoutBase> {
        return std::make_unique<PomodoroLayout>();
    };
    pomodoroReg.autoCreate = true;
    pomodoroReg.persistent = true;

    if (layoutManager_.registerLayout(pomodoroReg)) {
        registeredLayoutIds_.push_back("Pomodoro");
    }

    // 注册换取记录布局
    LayoutRegistration exchangeReg("ExchangeRecord", LayoutType::CONTENT, LayoutPriority::NORMAL);
    exchangeReg.factory = []() -> std::unique_ptr<LayoutBase> {
        return std::make_unique<ExchangeRecordLayout>();
    };
    exchangeReg.autoCreate = true;
    exchangeReg.persistent = true;

    if (layoutManager_.registerLayout(exchangeReg)) {
        registeredLayoutIds_.push_back("ExchangeRecord");
    }

    // 注册剪切板管理器布局
    LayoutRegistration clipboardReg("ClipboardHelper", LayoutType::CONTENT, LayoutPriority::NORMAL);
    clipboardReg.factory = []() -> std::unique_ptr<LayoutBase> {
        return std::make_unique<DearTs::Core::Window::Widgets::Clipboard::ClipboardHistoryLayout>();
    };
    clipboardReg.autoCreate = true;
    clipboardReg.persistent = true;

    if (layoutManager_.registerLayout(clipboardReg)) {
        registeredLayoutIds_.push_back("ClipboardHelper");
    }

    DEARTS_LOG_INFO("内容布局类型注册完成");
}

void MainWindow::setupLayoutDependencies() {
    DEARTS_LOG_INFO("设置布局依赖关系");

    // 内容布局依赖系统布局
    layoutManager_.addLayoutDependency("Pomodoro", "Sidebar");
    layoutManager_.addLayoutDependency("Pomodoro", "TitleBar");

    layoutManager_.addLayoutDependency("ExchangeRecord", "Sidebar");
    layoutManager_.addLayoutDependency("ExchangeRecord", "TitleBar");

    layoutManager_.addLayoutDependency("ClipboardHelper", "Sidebar");
    layoutManager_.addLayoutDependency("ClipboardHelper", "TitleBar");

    DEARTS_LOG_INFO("布局依赖关系设置完成");
}

void MainWindow::setupLayoutPriorities() {
    DEARTS_LOG_INFO("设置布局优先级");

    // 系统布局已经有默认的高优先级
    // 内容布局优先级在注册时已设置

    DEARTS_LOG_INFO("布局优先级设置完成");
}

void MainWindow::initializeRegisteredLayouts() {
    DEARTS_LOG_INFO("初始化已注册的布局");

    // 系统布局会在注册时自动创建，这里只需要获取引用
    sidebarLayout_ = static_cast<SidebarLayout*>(layoutManager_.getLayout("Sidebar"));

    // 设置标题栏的窗口标题
    TitleBarLayout* titleBar = static_cast<TitleBarLayout*>(layoutManager_.getLayout("TitleBar"));
    if (titleBar) {
        titleBar->setWindowTitle(title_);
    }

    // 内容布局已自动创建，但默认隐藏
    layoutManager_.hideLayout("Pomodoro", "初始隐藏");
    layoutManager_.hideLayout("ExchangeRecord", "初始隐藏");
    layoutManager_.hideLayout("ClipboardHelper", "初始隐藏");

    // 初始化布局状态
    layoutManager_.setLayoutState("TitleBar", LayoutState::ACTIVE);
    layoutManager_.setLayoutState("Sidebar", LayoutState::ACTIVE);

    DEARTS_LOG_INFO("已注册布局初始化完成");
}

std::string MainWindow::mapSidebarItemToLayout(const std::string& itemId) {
    // 映射侧边栏项目ID到布局名称
    if (itemId == "pomodoro") {
        return "Pomodoro";
    } else if (itemId == "exchange-record") {
        return "ExchangeRecord";
    } else if (itemId == "clipboard-helper") {
        return "ClipboardHelper";
    } else if (itemId == "data-analysis") {
        // 数据分析功能可以创建新的布局或使用默认布局
        return ""; // 暂时返回空，可以后续添加
    } else {
        DEARTS_LOG_DEBUG("未映射的侧边栏项目: " + itemId);
        return "";
    }
}

} // namespace Window
} // namespace Core
} // namespace DearTs