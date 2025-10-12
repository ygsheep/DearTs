/**
 * @file layout_events.cpp
 * @brief 布局相关事件实现
 * @author DearTs Team
 * @date 2025
 */

#include "layout_events.h"
#include "../utils/logger.h"
#include <algorithm>

namespace DearTs {
namespace Core {
namespace Events {

// LayoutEventDispatcher 实现

void LayoutEventDispatcher::subscribe(LayoutEventType type, LayoutEventHandler handler) {
    handlers_[type].push_back(std::move(handler));
    DEARTS_LOG_DEBUG("订阅布局事件: " + std::to_string(static_cast<uint32_t>(type)) +
                    ", 处理器数量: " + std::to_string(handlers_[type].size()));
}

void LayoutEventDispatcher::unsubscribe(LayoutEventType type) {
    auto it = handlers_.find(type);
    if (it != handlers_.end()) {
        handlers_.erase(it);
        DEARTS_LOG_DEBUG("取消订阅布局事件: " + std::to_string(static_cast<uint32_t>(type)));
    }
}

bool LayoutEventDispatcher::dispatch(const LayoutEvent& event) {
    LayoutEventType type = event.getLayoutEventType();
    auto it = handlers_.find(type);

    if (it == handlers_.end() || it->second.empty()) {
        DEARTS_LOG_DEBUG("未找到布局事件处理器: " + std::to_string(static_cast<uint32_t>(type)));
        return false;
    }

    bool handled = false;
    DEARTS_LOG_DEBUG("分发布局事件: " + event.getName() +
                    " 到 " + std::to_string(it->second.size()) + " 个处理器");

    // 复制处理器列表，避免在处理过程中修改原列表
    auto handlers = it->second;
    for (const auto& handler : handlers) {
        try {
            if (handler(event)) {
                handled = true;
            }
        } catch (const std::exception& e) {
            DEARTS_LOG_ERROR("布局事件处理器异常: " + std::string(e.what()));
        }
    }

    return handled;
}

void LayoutEventDispatcher::clear() {
    size_t totalHandlers = 0;
    for (const auto& pair : handlers_) {
        totalHandlers += pair.second.size();
    }

    handlers_.clear();
    DEARTS_LOG_DEBUG("清除所有布局事件订阅，共清除 " + std::to_string(totalHandlers) + " 个处理器");
}

// LayoutEventUtils 实现

namespace LayoutEventUtils {

void requestShowLayout(std::string_view layoutName, std::optional<std::string_view> reason) {
    LayoutShowRequestEvent event(layoutName, reason);

    auto* eventSystem = EventSystem::getInstance();
    if (eventSystem) {
        bool dispatched = eventSystem->dispatchEvent(event);
        DEARTS_LOG_INFO("发送布局显示请求: " + std::string(layoutName) +
                        (reason ? " 原因: " + std::string(*reason) : "") +
                        (dispatched ? " [已分发]" : " [未分发]"));
    } else {
        DEARTS_LOG_ERROR("事件系统未初始化，无法发送布局显示请求");
    }
}

void requestHideLayout(std::string_view layoutName, std::optional<std::string_view> reason) {
    LayoutHideRequestEvent event(layoutName, reason);

    auto* eventSystem = EventSystem::getInstance();
    if (eventSystem) {
        bool dispatched = eventSystem->dispatchEvent(event);
        DEARTS_LOG_INFO("发送布局隐藏请求: " + std::string(layoutName) +
                        (reason ? " 原因: " + std::string(*reason) : "") +
                        (dispatched ? " [已分发]" : " [未分发]"));
    } else {
        DEARTS_LOG_ERROR("事件系统未初始化，无法发送布局隐藏请求");
    }
}

void requestSwitchLayout(std::string_view fromLayout, std::string_view toLayout,
                        std::string_view reason, bool animated) {
    LayoutSwitchRequestEvent event(fromLayout, toLayout, reason, animated);

    auto* eventSystem = EventSystem::getInstance();
    if (eventSystem) {
        bool dispatched = eventSystem->dispatchEvent(event);
        DEARTS_LOG_INFO("发送布局切换请求: " + std::string(fromLayout) + " -> " + std::string(toLayout) +
                        (reason.empty() ? "" : " 原因: " + std::string(reason)) +
                        (animated ? " [动画]" : " [无动画]") +
                        (dispatched ? " [已分发]" : " [未分发]"));
    } else {
        DEARTS_LOG_ERROR("事件系统未初始化，无法发送布局切换请求");
    }
}

} // namespace LayoutEventUtils

} // namespace Events
} // namespace Core
} // namespace DearTs