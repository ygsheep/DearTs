#include <iostream>
#include <SDL.h>

// 手动声明LayoutManager类的部分接口用于测试
namespace DearTs {
namespace Core {
namespace Window {
    class LayoutManager {
    public:
        LayoutManager();
        ~LayoutManager();
        size_t getLayoutCount() const;
    };
}
}
}

// 简单的测试函数
void testBasicFunctionality() {
    std::cout << "测试基本功能..." << std::endl;
    
    // 测试创建实例
    DearTs::Core::Window::LayoutManager layoutManager;
    std::cout << "LayoutManager实例创建成功" << std::endl;
    
    // 测试获取布局数量
    size_t count = layoutManager.getLayoutCount();
    std::cout << "布局数量: " << count << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "=== LayoutManager 基本功能测试 ===" << std::endl;
    
    // 初始化SDL
    if (SDL_Init(SDL_INIT_EVENTS) < 0) {
        std::cerr << "SDL初始化失败: " << SDL_GetError() << std::endl;
        return -1;
    }
    
    try {
        testBasicFunctionality();
    } catch (const std::exception& e) {
        std::cerr << "测试时发生异常: " << e.what() << std::endl;
        SDL_Quit();
        return -1;
    } catch (...) {
        std::cerr << "测试时发生未知异常" << std::endl;
        SDL_Quit();
        return -1;
    }
    
    // 关闭SDL
    SDL_Quit();
    
    std::cout << "基本功能测试完成!" << std::endl;
    return 0;
}