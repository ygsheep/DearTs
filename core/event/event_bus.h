/**
 * @file event_bus.h
 * @brief 类型安全的事件总线系统
 * @details 使用现代 C++ 实现的编译时类型安全事件系统
 * @author DearTs Team
 * @date 2025
 * @version 1.1.0
 */

#pragma once

#include <algorithm>
#include <functional>
#include <vector>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <typeindex>
#include "i_event_bus.h"

namespace DearTs::Core::Event {

/**
 * @brief 类型安全的事件总线
 *
 * @example
 * // 定义事件
 * struct WindowCloseEvent {
 *     int window_id;
 * };
 *
 * // 订阅事件
 * auto token = EventBus::instance().subscribe<WindowCloseEvent>([](const WindowCloseEvent& e) {
 *     LOG_INFO("Window {} closed", e.window_id);
 * });
 *
 * // 发布事件
 * EventBus::instance().publish(WindowCloseEvent{ .window_id = 42 });
 *
 * // 自动取消订阅（RAII）
 * // token 析构时自动取消订阅
 */
class EventBus final : public IEventBus {  // 单例类，禁止继承
public:
    /**
     * @brief 获取单例实例（线程安全，Magic Statics）
     */
    static EventBus& instance() noexcept {
        static EventBus instance;
        return instance;
    }

    // 删除所有拷贝和移动操作
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;
    EventBus(EventBus&&) = delete;
    EventBus& operator=(EventBus&&) = delete;

    // === IEventBus 类型擦除接口实现 ===

    EventToken subscribe_raw(std::type_index type,
                             std::function<void(const void*)> callback) override {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);

        auto token = EventToken(m_next_token_id++);

        auto& callbacks = m_callbacks[type];
        callbacks.push_back(CallbackWrapper{
            .callback = std::move(callback),
            .token_id = token.get_id()
        });

        return token;
    }

    void unsubscribe_raw(std::type_index type, const EventToken& token) override {
        if (!token.is_valid()) {
            return;
        }

        std::lock_guard<std::recursive_mutex> lock(m_mutex);

        auto it = m_callbacks.find(type);
        if (it != m_callbacks.end()) {
            auto& callbacks = it->second;
            callbacks.erase(
                std::remove_if(callbacks.begin(), callbacks.end(),
                    [&token](const CallbackWrapper& wrapper) {
                        return wrapper.token_id == token.get_id();
                    }),
                callbacks.end()
            );
        }
    }

    void publish_raw(std::type_index type, const void* event) override {
        // 拷贝回调列表，避免持锁调用用户代码（防止死锁与重入问题）
        std::vector<CallbackWrapper> callbacks_copy;
        {
            std::lock_guard<std::recursive_mutex> lock(m_mutex);
            auto it = m_callbacks.find(type);
            if (it == m_callbacks.end()) {
                return;  // 没有订阅者，直接返回
            }
            callbacks_copy = it->second;
        }

        for (const auto& wrapper : callbacks_copy) {
            try {
                wrapper.callback(event);
            } catch (const std::exception&) {
                // 记录异常但不中断其他回调
            } catch (...) {
                // 兜底：吞掉所有异常，防止 terminate
            }
        }
    }

    void publish_async_raw(std::type_index type,
                           std::function<void()> emitter) override {
        std::lock_guard<std::mutex> lock(m_async_mutex);

        m_async_queue.push_back(std::move(emitter));
    }

    void process_async_events() override {
        std::vector<std::function<void()>> queue_copy;

        {
            std::lock_guard<std::mutex> lock(m_async_mutex);
            queue_copy = std::move(m_async_queue);
            m_async_queue.clear();
        }

        for (auto& callback : queue_copy) {
            callback();
        }
    }

    void clear() override {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        m_callbacks.clear();
    }

private:
    EventBus() = default;
    ~EventBus() override = default;

    struct CallbackWrapper {
        std::function<void(const void*)> callback;
        EventToken::ID token_id;
    };

    std::recursive_mutex m_mutex;
    EventToken::ID m_next_token_id = 1;
    std::unordered_map<std::type_index, std::vector<CallbackWrapper>> m_callbacks;

    std::mutex m_async_mutex;
    std::vector<std::function<void()>> m_async_queue;
};

/**
 * @brief RAII 事件订阅管理器
 * @tparam T 事件类型
 */
template<typename T>
class EventGuard {
public:
    using CallbackType = std::function<void(const T&)>;

    EventGuard(CallbackType callback)
        : m_token(EventBus::instance().subscribe<T>(std::move(callback)))
    {}

    ~EventGuard() {
        EventBus::instance().unsubscribe<T>(m_token);
    }

    // 删除拷贝
    EventGuard(const EventGuard&) = delete;
    EventGuard& operator=(const EventGuard&) = delete;

    // 支持移动
    EventGuard(EventGuard&& other) noexcept
        : m_token(std::move(other.m_token))
    {
        other.m_token.invalidate();
    }

    EventGuard& operator=(EventGuard&& other) noexcept {
        if (this != &other) {
            EventBus::instance().unsubscribe<T>(m_token);
            m_token = std::move(other.m_token);
            other.m_token.invalidate();
        }
        return *this;
    }

private:
    EventToken m_token;
};

/**
 * @brief 便捷函数：创建事件守卫
 * @tparam T 事件类型
 * @param callback 事件回调
 * @return EventGuard 对象
 */
template<typename T>
[[nodiscard]] EventGuard<T> make_event_guard(std::function<void(const T&)> callback) {
    return EventGuard<T>(std::move(callback));
}

} // namespace DearTs::Core::Event
