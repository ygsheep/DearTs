// DearTs 框架最小示例：验证 find_package(dearts) 安装集成
// 使用框架核心 API：EventBus（类型安全事件） + ConfigManager（配置） + Logger

#include "core/event/event_bus.h"
#include "core/config/config_manager.h"
#include "core/plugin/plugin.h"
#include "logger.h"

#include <iostream>
#include <string>

using namespace DearTs;
using namespace DearTs::Core::Event;
using namespace DearTs::Core::Config;

// 示例事件
struct HelloEvent {
    std::string message;
};

int main() {
    // 1. Logger
    auto logger = DearTs::Logger::get_instance();
    LOG_INFO("DeartsMinimal: Logger initialized");

    // 2. EventBus 发布/订阅
    EventToken token = EventBus::instance().subscribe<HelloEvent>(
        [](const HelloEvent& e) {
            LOG_INFO("DeartsMinimal: Received event: {}", e.message);
        }
    );
    EventBus::instance().publish(HelloEvent{"hello from event bus"});

    // 3. ConfigManager 读写
    auto& config = ConfigManager::instance();
    config.set("minimal.counter", 42);
    int counter = config.get<int>("minimal.counter").unwrap_or(-1);

    std::cout << "DeartsMinimal: config counter = " << counter << std::endl;
    std::cout << "DeartsMinimal: OK - DearTs framework linked successfully!" << std::endl;
    return 0;
}
