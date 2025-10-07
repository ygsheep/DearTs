#include "../../main/gui/include/gui_application.h"
#include <iostream>
#include <memory>

using namespace DearTs;

class TestApp : public GUIApplication {
public:
    TestApp() : GUIApplication() {}
    
    bool initialize(const Core::App::ApplicationConfig& config) override {
        if (!GUIApplication::initialize(config)) {
            return false;
        }
        
        std::cout << "测试应用程序初始化成功" << std::endl;
        return true;
    }
};

int main(int argc, char* argv[]) {
    std::cout << "=== 中文显示测试 ===" << std::endl;
    
    try {
        // 创建应用程序实例
        auto app = std::make_unique<TestApp>();
        
        // 配置应用程序
        Core::App::ApplicationConfig config;
        config.name = "Chinese Display Test";
        config.version = "1.0.0";
        
        // 初始化应用程序
        if (!app->initialize(config)) {
            std::cerr << "应用程序初始化失败" << std::endl;
            return -1;
        }
        
        // 运行应用程序
        int result = app->run();
        
        std::cout << "应用程序运行完成，退出代码: " << result << std::endl;
        return result;
        
    } catch (const std::exception& e) {
        std::cerr << "应用程序运行时发生异常: " << e.what() << std::endl;
        return -1;
    } catch (...) {
        std::cerr << "应用程序发生未知异常" << std::endl;
        return -1;
    }
}