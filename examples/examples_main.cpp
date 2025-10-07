/**
 * DearTs ApplicationManager 完整使用示例
 * 
 * 展示如何使用 ApplicationManager 创建和运行 GUI 应用程序
 * 
 * @author DearTs Team
 * @version 1.0.0
 * @date 2025
 */

#include "../main/gui/include/application_manager.h"
#include <iostream>
#include <memory>

// Windows控制台UTF-8支持
#ifdef _WIN32
#include <Windows.h>
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
#endif
}

/**
 * 主函数
 */
int main(int argc, char* argv[]) {
    // 设置控制台UTF-8支持
    setupConsoleUTF8();
    
    std::cout << "=== DearTs ApplicationManager 完整使用示例 ===" << std::endl;
    
    try {
        // 创建GUI应用程序管理器实例
        std::cout << "创建GUI应用程序管理器实例..." << std::endl;
        auto appManager = std::make_unique<DearTs::GUI::ApplicationManager>();
        std::cout << "✓ 应用程序管理器实例创建成功" << std::endl;
        
        // 初始化应用程序管理器
        std::cout << "初始化应用程序管理器..." << std::endl;
        if (!appManager->initialize()) {
            std::cerr << "❌ GUI应用程序管理器初始化失败" << std::endl;
            return -1;
        }
        std::cout << "✓ 应用程序管理器初始化成功" << std::endl;
        
        // 检查应用程序状态
        std::cout << "应用程序是否已初始化: " << (appManager->isInitialized() ? "是" : "否") << std::endl;
        std::cout << "应用程序是否正在运行: " << (appManager->isRunning() ? "是" : "否") << std::endl;
        
        // 获取窗口和渲染器
        SDL_Window* window = appManager->getWindow();
        SDL_Renderer* renderer = appManager->getRenderer();
        DearTs::GUI::WindowManager* windowManager = appManager->getWindowManager();
        
        std::cout << "窗口句柄: " << window << std::endl;
        std::cout << "渲染器句柄: " << renderer << std::endl;
        std::cout << "窗口管理器: " << windowManager << std::endl;
        
        // 请求退出应用程序
        std::cout << "请求退出应用程序..." << std::endl;
        appManager->requestExit();
        std::cout << "✓ 退出请求已发送" << std::endl;
        
        std::cout << "✓ 示例应用程序运行完成" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ 应用程序运行时发生异常: " << e.what() << std::endl;
        return -1;
    } catch (...) {
        std::cerr << "❌ 应用程序发生未知异常" << std::endl;
        return -1;
    }
}