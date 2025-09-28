/**
 * DearTs Application - Main Entry Point
 * 
 * 基于ImHex架构设计的现代化应用程序框架
 * 采用分层架构、事件驱动、插件化设计
 * 
 * @author DearTs Team
 * @version 2.0.0
 * @date 2025
 */

#include "../core/core.h"
#include "../core/resource/font_resource.h"
#include <iostream>
#include <exception>
#include <Windows.h>

// Windows控制台UTF-8支持
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

/**
 * 设置控制台UTF-8编码支持
 */
void setupConsoleUTF8() {
#ifdef _WIN32
    // 设置控制台代码页为UTF-8
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    
    // 启用ANSI转义序列支持
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }
    
    // 设置标准输出为UTF-8 (移除_O_U8TEXT以避免宽字符输出断言失败)
    // _setmode(_fileno(stdout), _O_U8TEXT);
    // _setmode(_fileno(stderr), _O_U8TEXT);
#endif
}

/**
 * 打印应用程序信息
 */
void printApplicationInfo() {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                        DearTs Application                    ║\n";
    std::cout << "║                     Version 2.0.0 - 2025                     ║\n";
    std::cout << "║                                                              ║\n";
    std::cout << "║  基于ImHex架构设计的现代化应用程序框架                             ║\n";
    std::cout << "║  • 分层架构设计                                                ║\n";
    std::cout << "║  • 事件驱动系统                                                ║\n";
    std::cout << "║  • 插件化扩展                                                  ║\n";
    std::cout << "║  • 现代化UI界面                                                ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
}

/**
 * 全局异常处理器
 */
void setupGlobalExceptionHandler() {
    std::set_terminate([]() {
        try {
            std::rethrow_exception(std::current_exception());
        } catch (const std::exception& e) {
            std::cerr << "未捕获的异常: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "未知异常" << std::endl;
        }
        
        std::cerr << "应用程序将退出..." << std::endl;
        std::abort();
    });
}

/**
 * 注册全局事件监听器
 */
void registerGlobalEventListeners() {
    auto eventSystem = DearTs::Core::Events::EventSystem::getInstance();
    
    // 注册应用程序生命周期事件
    eventSystem->getDispatcher().subscribe(DearTs::Core::Events::EventType::EVT_APP_LAUNCHED, [](const DearTs::Core::Events::Event& event) {
        std::wcout << L"✓ 应用程序初始化完成" << std::endl;
        return true;
    });
    
    eventSystem->getDispatcher().subscribe(DearTs::Core::Events::EventType::EVT_APP_TERMINATED, [](const DearTs::Core::Events::Event& event) {
        std::wcout << L"✓ 应用程序正在关闭" << std::endl;
        return true;
    });
    
    eventSystem->getDispatcher().subscribe(DearTs::Core::Events::EventType::EVT_WINDOW_CLOSE_REQUESTED, [](const DearTs::Core::Events::Event& event) {
        std::wcout << L"✓ 收到退出请求" << std::endl;
        return true;
    });
    
    // 注册窗口事件
    eventSystem->getDispatcher().subscribe(DearTs::Core::Events::EventType::EVT_WINDOW_CREATED, [](const DearTs::Core::Events::Event& event) {
        std::wcout << L"✓ 窗口创建完成" << std::endl;
        return true;
    });
    
    eventSystem->getDispatcher().subscribe(DearTs::Core::Events::EventType::EVT_WINDOW_DESTROYED, [](const DearTs::Core::Events::Event& event) {
        std::wcout << L"✓ 窗口已关闭" << std::endl;
        return true;
    });
    
    // 注册输入事件
    eventSystem->getDispatcher().subscribe(DearTs::Core::Events::EventType::EVT_KEY_PRESSED, [](const DearTs::Core::Events::Event& event) {
        // 可以在这里处理全局按键事件
        return true;
    });
}

/**
 * 主函数
 * 应用程序入口点
 */
int main(int argc, char* argv[]) {
//     // 设置控制台UTF-8支持
//     setupConsoleUTF8();
//
//     // 设置全局异常处理器
//     setupGlobalExceptionHandler();
//
//     // 打印应用程序信息
//     printApplicationInfo();
//
//     try {
//         std::cout << "正在启动应用程序..." << std::endl;
//
//         // 初始化DearTs核心系统
//         std::cout << "正在初始化核心系统..." << std::endl;
//         if (!DearTs::Core::init()) {
//             std::cerr << "❌ 核心系统初始化失败" << std::endl;
//             return -1;
//         }
//         std::cout << "✓ 核心系统初始化完成" << std::endl;
//         std::cout << "✓ 核心系统初始化成功" << std::endl;
//         std::cout << "版本信息: " << DearTs::Version::STRING << std::endl;
//
//         // 注册全局事件监听器
//         registerGlobalEventListeners();
//
//         // 创建渲染上下文
//         std::cout << "正在创建渲染上下文..." << std::endl;
//         auto& renderManager = DearTs::Core::Managers::render();
//         DearTs::Core::Window::WindowConfigSingleton& configSingleton = DearTs::Core::Window::WindowConfigSingleton::getInstance();
//         DearTs::Core::Window::WindowConfig config = configSingleton.getConfig();
//         config.title = "DearTs Application";
//         config.size = {1280, 720};
//         config.flags = DearTs::Core::Window::WindowFlags::BORDERLESS |
//                       DearTs::Core::Window::WindowFlags::RESIZABLE;
//         // 设置窗口图标
//         config.icon_path = "resources/icon.ico";
//         configSingleton.setConfig(config);
//
//         // 创建窗口但不立即创建SDL窗口
//         auto window = std::make_shared<DearTs::Core::Window::Window>(config);
//
//         // 现在创建SDL窗口
//         if (!window->create()) {
//             std::cerr << "❌ 主窗口创建失败" << std::endl;
//             DearTs::Core::shutdown();
//             return -1;
//         }
//
//         // 将窗口添加到窗口管理器
//         DearTs::Core::Managers::window().addWindow(window);
//
//         DearTs::Core::Render::RendererConfig rendererConfig;
//         auto renderContext = renderManager.createContext(window->getSDLWindow(), rendererConfig);
//         if (!renderContext) {
//             std::cerr << "❌ 渲染上下文创建失败" << std::endl;
//             DearTs::Core::shutdown();
//             return -1;
//         }
//         renderManager.setCurrentContext(renderContext);
//
//         // 将渲染器设置到窗口对象上
//         auto renderer = renderContext->getRenderer();
//         // 使用适配器将IRenderer转换为WindowRenderer
//         auto adapter = std::make_unique<DearTs::Core::Render::IRendererToWindowRendererAdapter>(renderer);
//         window->setRenderer(std::move(adapter));
//
//         // 初始化资源管理器
//         // 获取渲染上下文
//         auto contexts = renderManager.getAllContexts();
//
//         if (!contexts.empty()) {
//             auto context = contexts[0];
//             auto renderer = context->getRenderer();
//
//             // 检查是否是SDLRenderer并获取SDL_Renderer指针
//             auto sdlRenderer = dynamic_cast<DearTs::Core::Render::SDLRenderer*>(renderer.get());
//
//             if (sdlRenderer) {
//                 auto sdlRendererHandle = sdlRenderer->getSDLRenderer();
//
//                 auto resourceManager = DearTs::Core::Resource::ResourceManager::getInstance();
//
//                 if (!resourceManager->initialize(sdlRendererHandle)) {
//                     std::cerr << "❌ 资源管理器初始化失败" << std::endl;
//                     DearTs::Core::shutdown();
//                     return -1;
//                 }
//
//                 // 初始化字体管理器并加载默认字体
//                 auto fontManager = DearTs::Core::Resource::FontManager::getInstance();
//                 if (fontManager && fontManager->initialize()) {
//                     // 加载默认字体（与demo/old保持一致）
//                     std::cout << "正在加载默认字体..." << std::endl;
//                     if (!fontManager->loadDefaultFont(14.0f)) {
//                         std::cerr << "❌ 默认字体加载失败" << std::endl;
//                         // 继续运行，使用ImGui默认字体
//                     }
//                 } else {
//                     std::cerr << "❌ 字体管理器初始化失败" << std::endl;
//                     // 继续运行，使用ImGui默认字体
//                 }
//             } else {
//                 std::cerr << "❌ 无法获取SDL渲染器" << std::endl;
//                 DearTs::Core::shutdown();
//                 return -1;
//             }
//         } else {
//             std::cerr << "❌ 未找到渲染上下文" << std::endl;
//             DearTs::Core::shutdown();
//             return -1;
//         }
//
//         // 现在创建SDL窗口
//         if (!window->create()) {
//             DearTs::Core::shutdown();
//             return -1;
//         }
//
//         // 将窗口添加到窗口管理器
//         DearTs::Core::Managers::window().addWindow(window);
//
//         // 初始化ImGui
//         auto sdlRenderer = dynamic_cast<DearTs::Core::Render::SDLRenderer*>(renderer.get());
//         if (sdlRenderer) {
//             if (!sdlRenderer->initializeImGui(window->getSDLWindow(), sdlRenderer->getSDLRenderer())) {
//                 std::cerr << "❌ ImGui初始化失败" << std::endl;
//                 DearTs::Core::shutdown();
//                 return -1;
//             }
//             std::cout << "✓ ImGui初始化成功" << std::endl;
//         } else {
//             std::cerr << "❌ 无法获取SDL渲染器进行ImGui初始化" << std::endl;
//             DearTs::Core::shutdown();
//             return -1;
//         }
//
//         // 获取应用程序管理器并运行
//         auto& appManager = DEARTS_APP_MANAGER();
//
//         // 创建并运行应用程序
//         auto app = appManager.createApplication<DearTs::Core::App::Application>();
//         int result = appManager.runApplication(std::move(app));
//
//         std::cout << "✓ 应用程序运行完成，退出码: " << result << std::endl;
//         // 关闭核心系统
//         DearTs::Core::shutdown();
//         std::cout << "Returning from main function with exit code 0" << std::endl;
//         return 0;
//
//     } catch (const std::exception& e) {
//         std::cerr << "❌ 应用程序运行时发生异常: " << e.what() << std::endl;
//
//         // 显示错误对话框
//         std::string errorMsg = "应用程序发生异常:\n";
//         errorMsg += e.what();
//         errorMsg += "\n\n应用程序将退出。";
//
//         MessageBoxA(nullptr, errorMsg.c_str(), "DearTs Application - 错误",
//                    MB_OK | MB_ICONERROR | MB_TOPMOST);
//
//         return -1;
//
//     } catch (...) {
//         std::cerr << "❌ 应用程序发生未知异常" << std::endl;
//
//         MessageBoxA(nullptr,
//                    "应用程序发生未知异常。\n\n应用程序将退出。",
//                    "DearTs Application - 错误",
//                    MB_OK | MB_ICONERROR | MB_TOPMOST);
//
//         return -1;
//     }
}
