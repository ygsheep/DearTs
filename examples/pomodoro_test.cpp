#include "../core/window/layouts/pomodoro_layout.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    std::cout << "测试番茄时钟倒计时功能..." << std::endl;
    
    DearTs::Core::Window::PomodoroLayout pomodoro;
    pomodoro.setVisible(true);
    
    // 模拟点击开始按钮
    std::cout << "点击开始按钮..." << std::endl;
    pomodoro.testStartTimer();
    
    std::cout << "初始剩余时间: " << pomodoro.getRemainingTime() << "秒" << std::endl;
    
    // 模拟几秒钟的更新
    for (int i = 0; i < 5; i++) {
        std::cout << "更新第" << (i+1) << "次..." << std::endl;
        pomodoro.updateLayout(0, 0);  // 调用更新方法
        std::cout << "剩余时间: " << pomodoro.getRemainingTime() << "秒" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));  // 等待1秒
    }
    
    std::cout << "测试完成" << std::endl;
    return 0;
}