/**
 * @file event_system.h
 * @brief 简化的事件系统头文件（使用自定义哈希函数，不依赖 std::hash 特化）
 * @author DearTs
 * @date 2025
 */

#pragma once

#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include <cstdint>
#include <type_traits> // for std::underlying_type_t

namespace DearTs {
namespace Core {
namespace Events {

/**
 * @brief 事件类型枚举
 */
    enum class EventType : uint32_t {
        NONE = 0,
        EVT_WINDOW_CLOSE,
        EVT_APP_LAUNCHED,
        EVT_APP_TERMINATED,
        EVT_WINDOW_RESIZE,
        EVT_WINDOW_CREATED,
        EVT_WINDOW_DESTROYED,
        EVT_WINDOW_MINIMIZED,
        EVT_WINDOW_MAXIMIZED,
        EVT_WINDOW_RESTORED,
        EVT_WINDOW_CLOSE_REQUESTED,
        EVT_WINDOW_MOVED,
        EVT_WINDOW_RESIZED,
        EVT_WINDOW_FOCUS_GAINED,
        EVT_WINDOW_FOCUS_LOST,

        // 应用程序事件
        EVT_APPLICATION_QUIT,
        EVT_APPLICATION_PAUSE,
        EVT_APPLICATION_RESUME,
        EVT_KEY_PRESSED,
        EVT_KEY_RELEASED,
        EVT_MOUSE_BUTTON_PRESSED,
        EVT_MOUSE_BUTTON_RELEASED,
        EVT_MOUSE_MOVED,
        EVT_MOUSE_SCROLLED,
        EVT_CUSTOM
    };

/**
* @brief EventType 的自定义哈希函数
*/
struct EventTypeHash {
    size_t operator()(EventType type) const noexcept {
        using UT = std::underlying_type_t<EventType>;
        return std::hash<UT>()(static_cast<UT>(type));
    }
};

/**
 * @brief 基础事件类
 */
class Event {
public:
    explicit Event(const EventType type) : type_(type) {}
    virtual ~Event() = default;

    EventType getType() const { return type_; }
    virtual std::string getName() const = 0;

private:
    EventType type_;
};

/**
 * @brief 事件处理器类型定义
 */
using EventHandler = std::function<bool(const Event&)>;

/**
 * @brief 简化的事件调度器
 */
class EventDispatcher {
public:
    EventDispatcher() = default;
    ~EventDispatcher() = default;

    void subscribe(EventType type, const EventHandler& handler);
    void unsubscribe(EventType type);
    bool dispatch(const Event& event);
    void clear();

private:
    std::unordered_map<EventType, std::vector<EventHandler>, EventTypeHash> handlers_;
};

/**
 * @brief 事件系统管理器
 */
class EventSystem {
public:
    static EventSystem* getInstance();

    void initialize();
    void shutdown();

    EventDispatcher& getDispatcher() { return dispatcher_; }
    bool dispatchEvent(const Event& event);

private:
    EventSystem() = default;
    ~EventSystem() = default;

    static EventSystem* instance_;
    EventDispatcher dispatcher_;
};

// 便利宏定义
#define EVENT_SYSTEM() DearTs::Core::Events::EventSystem::getInstance()

} // namespace Events
} // namespace Core
} // namespace DearTs
