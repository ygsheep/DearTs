#include "main_window.h"
#include "layouts/title_bar_layout.h"
#include "layouts/sidebar_layout.h"
#include "layouts/pomodoro_layout.h"
#include "layouts/exchange_record_layout.h"
#include "../utils/logger.h"
#include "../resource/font_resource.h"
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
    , showDemoWindow_(false)
    , showAnotherWindow_(false)
    , clearColor_(0.45f, 0.55f, 0.60f, 1.00f)
    , currentView_(MainViewType::DEFAULT)
    , pomodoroLayout_(nullptr)
    , exchangeRecordLayout_(nullptr) {
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
    
    // 添加标题栏布局
    auto titleBarLayout = std::make_unique<TitleBarLayout>();
    titleBarLayout->setWindowTitle(title_);
    addLayout("TitleBar", std::move(titleBarLayout));
    
    // 添加侧边栏布局
    auto sidebarLayout = std::make_unique<SidebarLayout>();
    
    // 创建"高效工具"可展开菜单项
    SidebarItem productivityItem("productivity", "高效工具", false, "高效工具", "", true);
    
    // 添加子项目 (使用绝对路径确保图片能正确加载)
    SidebarItem pomodoroItem("pomodoro", "番茄时钟", false, "番茄时钟");
    SidebarItem dataAnalysisItem("data-analysis", "数据分析", false, "数据分析");
    
    productivityItem.children.push_back(pomodoroItem);
    productivityItem.children.push_back(dataAnalysisItem);
    
    // 添加"高效工具"菜单项到侧边栏
    sidebarLayout->addItem(productivityItem);

    // 创建"鸣潮"可展开菜单项
    SidebarItem wutheringWavesItem("wuthering-waves", "鸣潮", false, "鸣潮游戏工具", "", true);

    // 添加子项目
    SidebarItem exchangeRecordItem("exchange-record", "换取记录", false, "声骸换取记录");

    wutheringWavesItem.children.push_back(exchangeRecordItem);

    // 添加"鸣潮"菜单项到侧边栏
    sidebarLayout->addItem(wutheringWavesItem);
    
    // 设置侧边栏状态变化回调
    sidebarLayout->setStateCallback([this](bool isExpanded, float currentWidth) {
        // 当侧边栏状态变化时，可以在这里处理相关逻辑
        DEARTS_LOG_INFO("侧边栏状态变化 - 展开: " + std::string(isExpanded ? "是" : "否") + 
                       ", 宽度: " + std::to_string(currentWidth));
    });
    
    // 设置侧边栏项目点击回调
    sidebarLayout->setItemClickCallback([this](const std::string& itemId) {
        DEARTS_LOG_INFO("侧边栏项目点击: " + itemId);
        // 隐藏所有布局
        if (pomodoroLayout_) {
            pomodoroLayout_->setVisible(false);
        }
        if (exchangeRecordLayout_) {
            exchangeRecordLayout_->setVisible(false);
        }

        // 根据点击的项目切换视图
        if (itemId == "pomodoro") {
            currentView_ = MainViewType::POMODORO;
            if (pomodoroLayout_) {
                pomodoroLayout_->setVisible(true);
                DEARTS_LOG_INFO("番茄时钟布局设置为可见");
            }
        } else if (itemId == "exchange-record") {
            currentView_ = MainViewType::EXCHANGE_RECORD;
            if (exchangeRecordLayout_) {
                exchangeRecordLayout_->setVisible(true);

                // 检查是否有游戏路径配置
                if (exchangeRecordLayout_->hasGamePathConfiguration()) {
                    DEARTS_LOG_INFO("存在游戏路径配置，重新搜索最新URL");
                    exchangeRecordLayout_->refreshUrlFromSavedPath();
                } else {
                    DEARTS_LOG_INFO("无游戏路径配置，开始自动搜索");
                    exchangeRecordLayout_->startSearch();
                }

                DEARTS_LOG_INFO("换取记录布局设置为可见");
            }
        } else {
            currentView_ = MainViewType::DEFAULT;
            DEARTS_LOG_INFO("切换到默认视图");
        }
    });
    
    addLayout("Sidebar", std::move(sidebarLayout));
    
    // 获取标题栏布局并设置窗口状态
    TitleBarLayout* titleBar = static_cast<TitleBarLayout*>(getLayout("TitleBar"));
    if (titleBar) {
        auto pos = getPosition();
        auto size = getSize();
        titleBar->saveNormalState(pos.x, pos.y, size.width, size.height);
    }
    
    // 创建番茄时钟布局
    pomodoroLayout_ = new PomodoroLayout();
    pomodoroLayout_->setVisible(false); // 默认隐藏

    // 创建换取记录布局
    exchangeRecordLayout_ = new ExchangeRecordLayout();
    exchangeRecordLayout_->setVisible(false); // 默认隐藏

    DEARTS_LOG_INFO("主窗口初始化成功: " + title_);
    return true;
}

/**
 * 渲染窗口内容
 */
void MainWindow::render() {
    // 调用基类渲染（会渲染所有布局，包括标题栏和侧边栏）
    WindowBase::render();
    
    // 使用字体推送机制来获得更好的渲染质量
    auto fontManager = DearTs::Core::Resource::FontManager::getInstance();
    std::shared_ptr<DearTs::Core::Resource::FontResource> defaultFont = nullptr;
    if (fontManager) {
        defaultFont = fontManager->getDefaultFont();
        if (defaultFont) {
            defaultFont->pushFont();
        }
    }
    
    // 获取侧边栏宽度以调整主内容区域
    float sidebarWidth = 0.0f;
    SidebarLayout* sidebar = getSidebarLayout();
    if (sidebar) {
        sidebarWidth = sidebar->getCurrentWidth();
    }
    
    // 根据当前视图渲染内容
    if (currentView_ == MainViewType::POMODORO && pomodoroLayout_ && pomodoroLayout_->isVisible()) {
        // 渲染番茄时钟布局
        // 设置窗口位置和大小，为侧边栏留出空间
        ImGui::SetNextWindowPos(ImVec2(sidebarWidth, 30)); // 30是标题栏高度
        ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x - sidebarWidth,
                                       ImGui::GetIO().DisplaySize.y - 30));

        ImGuiWindowFlags mainContentFlags = ImGuiWindowFlags_NoTitleBar |
                                           ImGuiWindowFlags_NoResize |
                                           ImGuiWindowFlags_NoMove |
                                           ImGuiWindowFlags_NoCollapse |
                                           ImGuiWindowFlags_NoBringToFrontOnFocus;

        // 设置深色主题背景
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
        ImGui::Begin("MainContent", nullptr, mainContentFlags);
        pomodoroLayout_->render();
        ImGui::End();
        ImGui::PopStyleColor(); // 恢复背景色
    } else if (currentView_ == MainViewType::EXCHANGE_RECORD && exchangeRecordLayout_ && exchangeRecordLayout_->isVisible()) {
        // 渲染换取记录布局
        // 设置窗口位置和大小，为侧边栏留出空间
        ImGui::SetNextWindowPos(ImVec2(sidebarWidth, 30)); // 30是标题栏高度
        ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x - sidebarWidth,
                                       ImGui::GetIO().DisplaySize.y - 30));

        ImGuiWindowFlags mainContentFlags = ImGuiWindowFlags_NoTitleBar |
                                           ImGuiWindowFlags_NoResize |
                                           ImGuiWindowFlags_NoMove |
                                           ImGuiWindowFlags_NoCollapse |
                                           ImGuiWindowFlags_NoBringToFrontOnFocus;

        // 设置深色主题背景
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
        ImGui::Begin("MainContent", nullptr, mainContentFlags);
        exchangeRecordLayout_->render();
        ImGui::End();
        ImGui::PopStyleColor(); // 恢复背景色
    } else {
        // 渲染主窗口内容，留出侧边栏空间
        {
            static float f = 0.0f;
            static int counter = 0;

            // 设置窗口位置和大小，为侧边栏留出空间
            ImGui::SetNextWindowPos(ImVec2(sidebarWidth, 30)); // 30是标题栏高度
            ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x - sidebarWidth,
                                           ImGui::GetIO().DisplaySize.y - 30));

            ImGuiWindowFlags mainContentFlags = ImGuiWindowFlags_NoTitleBar |
                                               ImGuiWindowFlags_NoResize |
                                               ImGuiWindowFlags_NoMove |
                                               ImGuiWindowFlags_NoCollapse |
                                               ImGuiWindowFlags_NoBringToFrontOnFocus;

            // 设置深色主题背景
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
            ImGui::Begin("Hello, DearTs!", nullptr, mainContentFlags);
            
            ImGui::Text("DearTs 主窗口");
            ImGui::Text("应用程序平均 %.3f ms/帧 (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::Text("侧边栏宽度: %.1f", sidebarWidth);
            ImGui::Text("侧边栏状态: %s", sidebar && sidebar->isExpanded() ? "展开" : "收起");
            const char* viewName = "默认";
if (currentView_ == MainViewType::POMODORO) {
    viewName = "番茄时钟";
} else if (currentView_ == MainViewType::EXCHANGE_RECORD) {
    viewName = "换取记录";
}
ImGui::Text("当前视图: %s", viewName);

ImGui::Separator();

// FreeType彩色字符信息
ImGui::Text("FreeType字体渲染信息:");
ImGui::Text("- FreeType支持: %s", "已启用");

// 检查是否有Material Symbols字体支持彩色字符
auto materialSymbolsFont = DearTs::Core::Resource::FontManager::getInstance()->getFont("material_symbols");
if (materialSymbolsFont) {
    ImGui::Text("- Material Symbols字体: 已加载");
    ImGui::Text("- 可以尝试使用彩色图标字符");

    // 测试一些Material Symbols彩色字符
    ImGui::Text("Material Symbols图标测试:");
    ImGui::SameLine();

    // 使用Material Symbols字体显示一些图标
    {
        FONT_SCOPE("material_symbols");
        ImGui::Text("测试基本Material Symbols字符:");

        // 测试一些常见的Material Symbols字符 (使用正确的Unicode范围)
        ImGui::Text("\uEB8B \uEB8C \uEB8D \uEB8E \uEB8F");  // home, settings, etc.
        ImGui::Text("如果显示乱码，说明字体文件或字符映射有问题");

        ImGui::Separator();
        ImGui::Text("字体调试信息:");

        // 检查字体是否真的被加载
        auto font = DearTs::Core::Resource::FontManager::getInstance()->getFont("material_symbols");
        if (font && font->getFont()) {
            ImGui::Text("✓ Material Symbols字体对象有效");

            // 显示字体基本信息
            ImGui::Text("- 字体大小: %.1f", font->getConfig().size);
            ImGui::Text("- 字体路径: %s", font->getConfig().path.c_str());
            ImGui::Text("- 字体名称: %s", font->getConfig().name.c_str());

            // 检查FreeType标志
            auto flags = font->getConfig().glyphRanges != nullptr;
            ImGui::Text("- 有字符范围设置: %s", flags ? "是" : "否");

            ImGui::Text("LoadColor标志测试:");
            bool hasLoadColor = (font->getConfig().mergeMode); // 简化检查
            ImGui::Text("- LoadColor标志: %s", hasLoadColor ? "可能启用" : "未启用");

        } else {
            ImGui::Text("✗ Material Symbols字体对象无效");
        }
    }

    ImGui::Text("注意: 如果图标显示为方框，可能是字体不支持彩色字符或需要LoadColor标志");
} else {
    ImGui::Text("- Material Symbols字体: 未加载");
}

// 关于LoadColor标志的说明
ImGui::Separator();
ImGui::Text("FreeType LoadColor标志说明:");
ImGui::Text("- ImGuiFreeTypeBuilderFlags_LoadColor 可加载彩色字符");
ImGui::Text("- 适用于支持COLR/CPAL格式的OpenType字体");
ImGui::Text("- 可显示彩色图标和表情符号");
ImGui::Text("- 参见ImGui FONTS.md中的'使用彩色字符/表情符号'部分");

ImGui::Separator();
            
            ImGui::Text("计数器示例:");
            ImGui::Text("计数器值: %d", counter);
            
            if (ImGui::Button("增加计数器")) {
                counter++;
            }
            ImGui::SameLine();
            if (ImGui::Button("重置计数器")) {
                counter = 0;
            }
            
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
            ImGui::PopStyleColor(); // 恢复背景色
        }
    }
    
    // 显示另一个窗口
    if (showAnotherWindow_) {
        ImGui::Begin("另一个窗口", &showAnotherWindow_);
        ImGui::Text("这是另一个窗口!");
        if (ImGui::Button("关闭我")) {
            showAnotherWindow_ = false;
        }
        ImGui::End();
    }
    
    // 显示ImGui演示窗口
    if (showDemoWindow_) {
        ImGui::ShowDemoWindow(&showDemoWindow_);
    }
    
    // 恢复之前的字体
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
    
    // 更新番茄时钟布局（如果可见）
    if (pomodoroLayout_ && pomodoroLayout_->isVisible()) {
        pomodoroLayout_->updateLayout(0, 0);
    }

    // 更新换取记录布局
    if (exchangeRecordLayout_) {
        if (exchangeRecordLayout_->isVisible()) {
            exchangeRecordLayout_->updateLayout(0, 0);
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

} // namespace Window
} // namespace Core
} // namespace DearTs