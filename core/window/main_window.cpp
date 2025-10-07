#include "main_window.h"
#include "layouts/title_bar_layout.h"
#include "layouts/sidebar_layout.h"
#include "../utils/logger.h"
#include "../resource/font_resource.h"
#include "../resource/vscode_icons.hpp"
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
    , clearColor_(0.45f, 0.55f, 0.60f, 1.00f) {
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
    SidebarItem productivityItem("productivity", "⚡", "高效工具", false, "高效工具", "", true);
    
    // 添加子项目 (使用绝对路径确保图片能正确加载)
    SidebarItem pomodoroItem("pomodoro", "⏱️", "番茄时钟", false, "番茄时钟");
    SidebarItem dataAnalysisItem("data-analysis", "📊", "数据分析", false, "数据分析");
    
    productivityItem.children.push_back(pomodoroItem);
    productivityItem.children.push_back(dataAnalysisItem);
    
    // 添加"高效工具"菜单项到侧边栏
    sidebarLayout->addItem(productivityItem);
    
    // 设置侧边栏状态变化回调
    sidebarLayout->setStateCallback([this](bool isExpanded, float currentWidth) {
        // 当侧边栏状态变化时，可以在这里处理相关逻辑
        DEARTS_LOG_INFO("侧边栏状态变化 - 展开: " + std::string(isExpanded ? "是" : "否") + 
                       ", 宽度: " + std::to_string(currentWidth));
    });
    
    addLayout("Sidebar", std::move(sidebarLayout));
    
    // 获取标题栏布局并设置窗口状态
    TitleBarLayout* titleBar = static_cast<TitleBarLayout*>(getLayout("TitleBar"));
    if (titleBar) {
        auto pos = getPosition();
        auto size = getSize();
        titleBar->saveNormalState(pos.x, pos.y, size.width, size.height);
    }
    
    DEARTS_LOG_INFO("主窗口初始化成功: " + title_);
    return true;
}

/**
 * 渲染窗口内容
 */
void MainWindow::render() {
    // 调用基类渲染（会渲染所有布局，包括标题栏和侧边栏）
    WindowBase::render();
    
    // 使用默认字体
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
        
        ImGui::Begin("Hello, DearTs!", nullptr, mainContentFlags);
        
        ImGui::Text("DearTs 主窗口");
        ImGui::Text("应用程序平均 %.3f ms/帧 (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::Text("侧边栏宽度: %.1f", sidebarWidth);
        ImGui::Text("侧边栏状态: %s", sidebar && sidebar->isExpanded() ? "展开" : "收起");
        
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
void MainWindow::update(double deltaTime) {
    // 调用基类更新
    WindowBase::update(deltaTime);
    
    // 更新标题栏窗口标题
    TitleBarLayout* titleBar = static_cast<TitleBarLayout*>(getLayout("TitleBar"));
    if (titleBar) {
        titleBar->setWindowTitle(getTitle());
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