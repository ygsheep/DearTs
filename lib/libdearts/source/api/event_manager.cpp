#include <dearts/api/event_manager.hpp>

#include <algorithm>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>
#include <functional>

namespace dearts {
    
    // 静态成员变量定义
    std::multimap<void *, EventManager::EventList::iterator>& EventManager::getTokenStore() {
        static std::multimap<void *, EventList::iterator> tokenStore;
        return tokenStore;
    }
    
    EventManager::EventList& EventManager::getEvents() {
        static EventList events;
        return events;
    }
    
    std::recursive_mutex& EventManager::getEventMutex() {
        static std::recursive_mutex eventMutex;
        return eventMutex;
    }
    
    bool EventManager::isAlreadyRegistered(void *token, impl::EventId id) {
        auto &tokenStore = getTokenStore();
        auto range = tokenStore.equal_range(token);
        
        for (auto it = range.first; it != range.second; ++it) {
            if (it->second->first == id) {
                return true;
            }
        }
        
        return false;
    }
    
    void EventManager::unsubscribe(void *token, impl::EventId id) {
        std::lock_guard<std::recursive_mutex> lock(getEventMutex());
        
        auto &tokenStore = getTokenStore();
        auto range = tokenStore.equal_range(token);
        
        for (auto it = range.first; it != range.second;) {
            if (it->second->first == id) {
                auto &events = getEvents();
                events.erase(it->second);
                it = tokenStore.erase(it);
            } else {
                ++it;
            }
        }
    }
    
}