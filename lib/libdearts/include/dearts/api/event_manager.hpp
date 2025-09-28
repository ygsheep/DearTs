#pragma once

#include <dearts/dearts.hpp>
#include <functional>
#include <list>
#include <mutex>
#include <map>
#include <string_view>
#include <algorithm>

#define EVENT_DEF_IMPL(event_name, event_name_string, should_log, ...)                                                                                      \
    struct event_name final : public dearts::impl::Event<__VA_ARGS__> {                                                                                        \
        constexpr static auto Id = [] { return dearts::impl::EventId(event_name_string); }();                                                                  \
        constexpr static auto ShouldLog = (should_log);                                                                                                     \
        explicit event_name(Callback func) noexcept : Event(std::move(func)) { }                                                                            \
                                                                                                                                                            \
        static EventManager::EventList::iterator subscribe(Event::Callback function) { return EventManager::subscribe<event_name>(std::move(function)); }   \
        static void subscribe(void *token, Event::Callback function) { EventManager::subscribe<event_name>(token, std::move(function)); }                   \
        static void unsubscribe(const EventManager::EventList::iterator &token) noexcept { EventManager::unsubscribe(token); }                              \
        static void unsubscribe(void *token) noexcept { EventManager::unsubscribe<event_name>(token); }                                                     \
        static void post(auto &&...args) { EventManager::post<event_name>(std::forward<decltype(args)>(args)...); }                                         \
    }

#define EVENT_DEF(event_name, ...)          EVENT_DEF_IMPL(event_name, #event_name, true, __VA_ARGS__)
#define EVENT_DEF_NO_LOG(event_name, ...)   EVENT_DEF_IMPL(event_name, #event_name, false, __VA_ARGS__)

namespace dearts {
    
    namespace impl {
        
        /**
         * @brief 事件ID类，用于唯一标识事件类型
         */
        class EventId {
        public:
            explicit constexpr EventId(const char *eventName) {
                m_hash = 0x811C9DC5;
                for (const char c : std::string_view(eventName)) {
                    m_hash = (m_hash >> 5) | (m_hash << 27);
                    m_hash ^= c;
                }
            }
            
            constexpr bool operator==(const EventId &other) const {
                return m_hash == other.m_hash;
            }
            
            constexpr auto operator<=>(const EventId &other) const {
                return m_hash <=> other.m_hash;
            }
            
        private:
            u32 m_hash;
        };
        
        /**
         * @brief 事件基类
         */
        struct EventBase {
            EventBase() noexcept = default;
            virtual ~EventBase() = default;
        };
        
        /**
         * @brief 事件模板类，支持任意参数类型
         */
        template<typename... Params>
        struct Event : EventBase {
            using Callback = std::function<void(Params...)>;
            
            explicit Event(Callback func) noexcept : m_func(std::move(func)) { }
            
            template<typename E>
            void call(Params... params) const {
                try {
                    m_func(params...);
                } catch (const std::exception &) {
                    // Ignore unhandled exceptions
                    throw;
                }
            }
            
        private:
            Callback m_func;
        };
        
        template<typename T>
        concept EventType = std::derived_from<T, EventBase>;
        
    }
    
    /**
     * @brief 事件管理器，负责事件的订阅、发布和管理
     * 参考ImHex的事件系统设计，支持类型安全的事件处理
     */
    class EventManager {
    public:
        using EventList = std::multimap<impl::EventId, std::unique_ptr<impl::EventBase>>;
        
        /**
         * @brief 订阅事件
         * @tparam E 事件类型
         * @param function 事件处理函数
         * @return 事件迭代器，用于取消订阅
         */
        template<impl::EventType E>
        static EventList::iterator subscribe(typename E::Callback function) {
            std::scoped_lock lock(getEventMutex());
            return getEvents().emplace(E::Id, std::make_unique<E>(std::move(function)));
        }
        
        /**
         * @brief 使用令牌订阅事件
         * @tparam E 事件类型
         * @param token 令牌指针
         * @param function 事件处理函数
         */
        template<impl::EventType E>
        static void subscribe(void *token, typename E::Callback function) {
            std::scoped_lock lock(getEventMutex());
            if (!isAlreadyRegistered(token, E::Id)) {
                auto iter = getEvents().emplace(E::Id, std::make_unique<E>(std::move(function)));
                getTokenStore().emplace(token, iter);
            }
        }
        
        /**
         * @brief 取消订阅事件
         * @param token 事件迭代器
         */
        static void unsubscribe(const EventList::iterator &token) noexcept {
            std::scoped_lock lock(getEventMutex());
            getEvents().erase(token);
        }
        
        /**
         * @brief 使用令牌取消订阅事件
         * @tparam E 事件类型
         * @param token 令牌指针
         */
        template<impl::EventType E>
        static void unsubscribe(void *token) noexcept {
            unsubscribe(token, E::Id);
        }
        
        /**
         * @brief 发布事件
         * @tparam E 事件类型
         * @param args 事件参数
         */
        template<impl::EventType E>
        static void post(auto && ...args) {
            std::scoped_lock lock(getEventMutex());
            
            auto &events = getEvents();
            auto range = events.equal_range(E::Id);
            
            for (auto it = range.first; it != range.second; ++it) {
                auto event = static_cast<E*>(it->second.get());
                event->template call<E>(std::forward<decltype(args)>(args)...);
            }
        }
        
        /**
         * @brief 清除所有事件订阅
         */
        static void clear() noexcept {
            std::scoped_lock lock(getEventMutex());
            getEvents().clear();
            getTokenStore().clear();
        }
        
    private:
        static std::multimap<void *, EventList::iterator>& getTokenStore();
        static EventList& getEvents();
        static std::recursive_mutex& getEventMutex();
        
        static bool isAlreadyRegistered(void *token, impl::EventId id);
        static void unsubscribe(void *token, impl::EventId id);
    };
    
}