/**
 * @file event_system.cpp
 * @brief 简化的事件系统实现
 * @author DearTs Team
 * @date 2024
 */

#include "event_system.h"
#include "../utils/logger.h"

namespace DearTs {
namespace Core {
namespace Events {

// EventDispatcher实现

/**
 * @brief 订阅事件
 * @param type 事件类型
 * @param handler 事件处理器
 */
void EventDispatcher::subscribe(EventType type, const EventHandler& handler) {
    handlers_[type].push_back(handler);
}

/**
 * @brief 取消订阅事件
 * @param type 事件类型
 */
void EventDispatcher::unsubscribe(EventType type) {
    handlers_.erase(type);
}

/**
 * @brief 分发事件
 * @param event 事件对象
 * @return 是否被处理
 */
bool EventDispatcher::dispatch(const Event& event) {
    auto it = handlers_.find(event.getType());
    if (it == handlers_.end()) {
        return false;
    }

    bool handled = false;
    for (const auto& handler : it->second) {
        if (handler(event)) {
            handled = true;
        }
    }

    return handled;
}

/**
 * @brief 清除所有订阅
 */
void EventDispatcher::clear() {
    handlers_.clear();
}

// EventSystem实现
EventSystem* EventSystem::instance_ = nullptr;

/**
 * @brief 获取单例实例
 * @return EventSystem实例指针
 */
EventSystem* EventSystem::getInstance() {
    if (!instance_) {
        instance_ = new EventSystem();
    }
    return instance_;
}

/**
 * @brief 初始化事件系统
 */
void EventSystem::initialize() {
    DEARTS_LOG_INFO("事件系统初始化完成");
}

/**
 * @brief 关闭事件系统
 */
void EventSystem::shutdown() {
    dispatcher_.clear();
    DEARTS_LOG_INFO("事件系统关闭");
}

/**
 * @brief 分发事件
 * @param event 事件对象
 * @return 是否被处理
 */
bool EventSystem::dispatchEvent(const Event& event) {
    return dispatcher_.dispatch(event);
}

} // namespace Events
} // namespace Core
} // namespace DearTs