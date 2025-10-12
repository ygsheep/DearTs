/**
 * 分词助手窗口测试程序
 *
 * 专门用于测试 TextSegmentationWindow 的独立程序
 * 不依赖主窗口，只创建和渲染分词助手窗口
 *
 * @author DearTs Team
 * @version 1.0.0
 * @date 2025
 */

#include <iostream>
#include <memory>
#include <string>
#include <typeinfo>

// Windows控制台UTF-8支持
#ifdef _WIN32
#include <Windows.h>
#include <io.h>
#include <fcntl.h>
#endif

// DearTs 核心头文件
#include "../../core/window/window_manager.h"
#include "../../core/window/layouts/layout_manager.h"
#include "../../core/window/widgets/clipboard/text_segmenter.h"
#include "../../core/render/renderer.h"
#include "../../core/utils/logger.h"
#include "segmentation_test_layout.h"

// SDL 和 ImGui 头文件
#include <SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>

/**
 * 简单的分词窗口测试应用程序
 */
class SegmentationTestApp {
public:
    SegmentationTestApp() : m_running(false) {}

    ~SegmentationTestApp() {
        cleanup();
    }

    bool initialize() {
        std::cout << "初始化分词助手测试程序..." << std::endl;

        // 1. 初始化SDL
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::cerr << "SDL初始化失败: " << SDL_GetError() << std::endl;
            return false;
        }

        // 2. 初始化WindowManager
        if (!DearTs::Core::Window::WindowManager::getInstance().initialize()) {
            std::cerr << "WindowManager初始化失败" << std::endl;
            return false;
        }
        std::cout << "WindowManager初始化成功" << std::endl;

        // 3. 设置测试文本
        m_testText = "这是一个测试文本，用于验证分词助手窗口的渲染功能。";

        // 4. 创建主窗口（使用DearTs窗口系统）
        auto& windowManager = DearTs::Core::Window::WindowManager::getInstance();

        // 配置窗口
        DearTs::Core::Window::WindowConfig windowConfig;
        windowConfig.title = "分词助手测试";
        windowConfig.flags = DearTs::Core::Window::WindowFlags::BORDERLESS;

        auto mainWindow = windowManager.createWindow(windowConfig);

        if (!mainWindow) {
            std::cerr << "创建主窗口失败" << std::endl;
            return false;
        }

        // 设置窗口大小（屏幕的60%宽，70%高）
        SDL_DisplayMode displayMode;
        if (SDL_GetCurrentDisplayMode(0, &displayMode) == 0) {
            int windowWidth = static_cast<int>(displayMode.w * 0.6f);
            int windowHeight = static_cast<int>(displayMode.h * 0.7f);
            int windowX = (displayMode.w - windowWidth) / 2;
            int windowY = (displayMode.h - windowHeight) / 2;

            // 更新窗口配置
            windowConfig.size = DearTs::Core::Window::WindowSize(windowWidth, windowHeight);
            windowConfig.position = DearTs::Core::Window::WindowPosition(windowX, windowY);
            mainWindow->setSize(windowConfig.size);
            mainWindow->setPosition(windowConfig.position);

            std::cout << "窗口尺寸设置: " << windowWidth << "x" << windowHeight << std::endl;
        }

        // 创建窗口
        if (!mainWindow->create()) {
            std::cerr << "创建SDL窗口失败" << std::endl;
            return false;
        }

        // 创建SDL渲染器（支持ImGui）并初始化
        auto sdlRenderer = std::make_unique<DearTs::Core::Render::SDLRenderer>();

        std::cout << "准备初始化SDL渲染器..." << std::endl;

        // 直接初始化渲染器，避免setRenderer的重复初始化问题
        if (!sdlRenderer->initialize(mainWindow->getSDLWindow())) {
            std::cerr << "SDL渲染器初始化失败" << std::endl;
            return false;
        }

        std::cout << "SDL渲染器初始化成功" << std::endl;

        // 手动初始化ImGui
        std::cout << "准备初始化ImGui..." << std::endl;
        if (!sdlRenderer->initializeImGui(mainWindow->getSDLWindow(), sdlRenderer->getSDLRenderer())) {
            std::cerr << "ImGui初始化失败" << std::endl;
            return false;
        }

        std::cout << "ImGui初始化成功" << std::endl;

        // 注意：由于setRenderer会重新初始化渲染器，破坏我们的ImGui初始化，
        // 所以我们不使用setRenderer，而是直接使用渲染器

        // 验证渲染器
        if (!sdlRenderer->isInitialized()) {
            std::cerr << "渲染器初始化状态无效" << std::endl;
            return false;
        }

        std::cout << "渲染器准备就绪" << std::endl;

        // 保存渲染器引用供主循环使用
        m_sdlRenderer = std::move(sdlRenderer);

        // 手动保存渲染器指针（不使用setRenderer避免重复初始化）
        // 注意：这种方法不完美，但可以用来测试
        // 实际使用时应该修复setRenderer方法

        // 添加到WindowManager
        if (!windowManager.addWindow("SegmentationTest", mainWindow)) {
            std::cerr << "添加窗口到WindowManager失败" << std::endl;
            return false;
        }

        // 创建分词测试布局并添加到布局管理器
        auto segmentationLayout = std::make_unique<DearTs::Examples::SegmentationTestLayout>();
        segmentationLayout->setTestText(m_testText);

        // 添加布局到布局管理器
        auto& layoutManager = DearTs::Core::Window::LayoutManager::getInstance();
        layoutManager.addLayout("SegmentationTest", std::move(segmentationLayout), "SegmentationTest");

        std::cout << "DearTs窗口系统初始化成功！" << std::endl;
        m_running = true;
        return true;
    }

    void run() {
        std::cout << "开始运行测试循环..." << std::endl;

        auto& windowManager = DearTs::Core::Window::WindowManager::getInstance();
        auto& layoutManager = DearTs::Core::Window::LayoutManager::getInstance();

        while (m_running && !windowManager.hasWindowsToClose()) {
            // 处理事件
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                windowManager.handleSDLEvent(event);
                layoutManager.handleEvent(event);

                switch (event.type) {
                    case SDL_QUIT:
                        m_running = false;
                        break;

                    case SDL_KEYDOWN:
                        if (event.key.keysym.sym == SDLK_ESCAPE) {
                            m_running = false;
                        }
                        break;
                }
            }

            // 更新所有窗口
            windowManager.updateAllWindows();

            // 获取主窗口
            auto mainWindow = windowManager.getWindowByName("SegmentationTest");
            if (mainWindow) {
                // 使用我们手动初始化的渲染器
                if (m_sdlRenderer) {
                    // 开始ImGui帧
                    m_sdlRenderer->newImGuiFrame();

                    // 渲染简单的测试UI
                    ImGui::Begin("分词助手测试", nullptr,
                                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

                    ImGui::Text("=== 分词助手窗口测试 ===");
                    ImGui::Text("这是一个测试窗口，用于验证ImGui渲染功能");
                    ImGui::Separator();

                    ImGui::Text("测试文本:");
                    ImGui::TextWrapped("%s", m_testText.c_str());
                    ImGui::Separator();

                    if (ImGui::Button("测试按钮")) {
                        std::cout << "按钮被点击了！" << std::endl;
                    }

                    ImGui::Text("按 ESC 键退出");
                    ImGui::End();

                    // 渲染ImGui
                    ImGui::Render();
                    ImDrawData* draw_data = ImGui::GetDrawData();
                    m_sdlRenderer->renderImGui(draw_data);
                } else {
                    std::cout << "无法获取m_sdlRenderer" << std::endl;
                }

                auto windowSize = mainWindow->getSize();
                layoutManager.updateAll(static_cast<float>(windowSize.width),
                                        static_cast<float>(windowSize.height), "SegmentationTest");
            } else {
                std::cout << "无法获取主窗口" << std::endl;
            }

            // 渲染所有窗口
            windowManager.renderAllWindows();

            // 控制帧率
            SDL_Delay(16); // ~60 FPS
        }

        std::cout << "测试循环结束" << std::endl;
    }

    void cleanup() {
        std::cout << "清理资源..." << std::endl;

        // 清理分词器
        m_textSegmenter.reset();

        // 清理DearTs窗口系统
        DearTs::Core::Window::WindowManager::getInstance().shutdown();

        // 清理SDL
        SDL_Quit();

        std::cout << "资源清理完成" << std::endl;
    }

private:
    std::unique_ptr<DearTs::Core::Window::Widgets::Clipboard::TextSegmenter> m_textSegmenter;
    std::unique_ptr<DearTs::Core::Render::SDLRenderer> m_sdlRenderer;  // 保存渲染器引用
    std::string m_testText;
    bool m_running;
};

/**
 * 主函数
 */
int main(int argc, char* argv[]) {
    // Windows控制台UTF-8支持
    #ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    setvbuf(stdout, nullptr, _IOFBF, 1024);
    #endif

    std::cout << "=== 分词助手窗口测试程序 ===" << std::endl;
    std::cout << "按 ESC 键或关闭窗口退出" << std::endl;
    std::cout << std::endl;

    try {
        SegmentationTestApp app;

        if (!app.initialize()) {
            std::cerr << "应用程序初始化失败" << std::endl;
            return -1;
        }

        app.run();

    } catch (const std::exception& e) {
        std::cerr << "异常: " << e.what() << std::endl;
        return -1;
    }

    std::cout << "程序正常退出" << std::endl;
    return 0;
}