/**
 * DearTs GUI Application - Main Entry Point
 * 
 * 基于ImHex架构设计的现代化GUI应用程序框架
 * 采用分层架构、事件驱动、插件化设计
 * 
 * @author DearTs Team
 * @version 2.0.0
 * @date 2025
 */

#include "../main/gui/include/application_manager.h"
#include <iostream>
#include <exception>

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
 * 主函数
 * GUI应用程序入口点
 */
int main(int argc, char* argv[]) {
    // 设置控制台UTF-8支持
    setupConsoleUTF8();
    
    // 设置全局异常处理器
    setupGlobalExceptionHandler();
    
    try {
        // 创建应用程序管理器
        auto appManager = std::make_unique<DearTs::GUI::ApplicationManager>();
        
        // 初始化应用程序
        if (!appManager->initialize()) {
            std::cerr << "❌ 应用程序初始化失败" << std::endl;
            return -1;
        }

        // 运行主循环
        appManager->run();
        
        // 关闭应用程序
        appManager->shutdown();
        
        std::cout << "✓ 应用程序运行完成" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ 应用程序运行时发生异常: " << e.what() << std::endl;
        
        // 显示错误对话框
        std::string errorMsg = "应用程序发生异常:\n";
        errorMsg += e.what();
        errorMsg += "\n\n应用程序将退出。";
        
        MessageBoxA(nullptr, errorMsg.c_str(), "DearTs GUI Application - 错误", 
                   MB_OK | MB_ICONERROR | MB_TOPMOST);
        
        return -1;
        
    } catch (...) {
        std::cerr << "❌ 应用程序发生未知异常" << std::endl;
        
        MessageBoxA(nullptr, 
                   "应用程序发生未知异常。\n\n应用程序将退出。", 
                   "DearTs GUI Application - 错误", 
                   MB_OK | MB_ICONERROR | MB_TOPMOST);
        return -1;
    }
}
