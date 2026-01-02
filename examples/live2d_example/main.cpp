/**
 * @file main.cpp
 * @brief Live2D 示例程序主入口
 * @details 独立的 OpenGL 应用程序，用于演示 Live2D 插件功能
 */

#include "opengl_application.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    std::cout << "=== Live2D Demo Application ===\n";
    std::cout << "Backend: OpenGL 3.3 Core\n\n";

    Live2DExample::OpenGLApplication app;

    if (!app.initialize()) {
        std::cerr << "Failed to initialize application\n";
        return 1;
    }

    return app.run();
}
