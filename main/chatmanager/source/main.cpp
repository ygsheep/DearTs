/**
 * @file main.cpp
 * @brief ChatManager 应用程序入口
 */

#include "chatmanager/application.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    // 创建应用实例
    ChatManager::Application app;

    // 初始化
    if (!app.initialize()) {
        std::cerr << "Failed to initialize ChatManager application" << std::endl;
        return -1;
    }

    // 运行主循环
    app.run();

    // 关闭应用
    app.shutdown();

    return 0;
}
