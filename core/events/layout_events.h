/**
 * @file layout_events.h
 * @brief 布局相关事件定义（C++17特性）
 * @author DearTs Team
 * @date 2025
 */

#pragma once

#include <string>
#include <variant>
#include <optional>
#include <string_view>
#include "../events/event_system.h"

namespace DearTs {
namespace Core {
namespace Events {

/**
 * @brief 布局事件类型枚举
 */
enum class LayoutEventType : uint32_t {
    // 布局可见性事件
    LAYOUT_SHOW_REQUEST = 1000,      ///< 请求显示布局
    LAYOUT_HIDE_REQUEST = 1001,      ///< 请求隐藏布局
    LAYOUT_VISIBILITY_CHANGED = 1002, ///< 布局可见性已改变

    // 布局切换事件
    LAYOUT_SWITCH_REQUEST = 1010,    ///< 请求切换布局
    LAYOUT_SWITCH_COMPLETED = 1011,  ///< 布局切换完成

    // 布局生命周期事件
    LAYOUT_CREATED = 1020,           ///< 布局已创建
    LAYOUT_DESTROYED = 1021,         ///< 布局已销毁
    LAYOUT_UPDATED = 1022,           ///< 布局已更新

    // 布局状态事件
    LAYOUT_FOCUS_CHANGED = 1030,     ///< 布局焦点改变
    LAYOUT_RESIZED = 1031,           ///< 布局大小改变
    LAYOUT_MOVED = 1032,             ///< 布局位置改变
};

/**
 * @brief LayoutEventType 的自定义哈希函数
 */
struct LayoutEventTypeHash {
    size_t operator()(LayoutEventType type) const noexcept {
        using UT = std::underlying_type_t<LayoutEventType>;
        return std::hash<UT>()(static_cast<UT>(type));
    }
};

/**
 * @brief 布局切换数据结构
 */
struct LayoutSwitchData {
    std::string fromLayout;     ///< 源布局名称
    std::string toLayout;       ///< 目标布局名称
    std::string reason;         ///< 切换原因
    bool animated = true;       ///< 是否使用动画

    LayoutSwitchData(std::string_view from, std::string_view to, std::string_view why = "", bool anim = true)
        : fromLayout(from), toLayout(to), reason(why), animated(anim) {}
};

/**
 * @brief 布局可见性数据结构
 */
struct LayoutVisibilityData {
    std::string layoutName;     ///< 布局名称
    bool visible;               ///< 是否可见
    std::optional<std::string> reason; ///< 可见性改变原因

    LayoutVisibilityData(std::string_view name, bool isVisible, std::optional<std::string_view> why = std::nullopt)
        : layoutName(name), visible(isVisible),
          reason(why.has_value() ? std::make_optional(std::string(*why)) : std::nullopt) {}
};

/**
 * @brief 布局大小数据结构
 */
struct LayoutSizeData {
    std::string layoutName;     ///< 布局名称
    float width;                ///< 宽度
    float height;               ///< 高度

    LayoutSizeData(std::string_view name, float w, float h)
        : layoutName(name), width(w), height(h) {}
};

/**
 * @brief 布局位置数据结构
 */
struct LayoutPositionData {
    std::string layoutName;     ///< 布局名称
    float x;                    ///< X坐标
    float y;                    ///< Y坐标

    LayoutPositionData(std::string_view name, float xCoord, float yCoord)
        : layoutName(name), x(xCoord), y(yCoord) {}
};

/**
 * @brief 布局事件数据变体（C++17 std::variant）
 */
using LayoutEventData = std::variant<
    std::string,                ///< 布局名称（简单情况）
    LayoutSwitchData,           ///< 布局切换数据
    LayoutVisibilityData,       ///< 布局可见性数据
    LayoutSizeData,             ///< 布局大小数据
    LayoutPositionData          ///< 布局位置数据
>;

/**
 * @brief 布局事件基类
 */
class LayoutEvent : public Event {
public:
    /**
     * @brief 构造函数
     * @param type 事件类型
     * @param data 事件数据
     */
    LayoutEvent(LayoutEventType type, LayoutEventData data)
        : Event(static_cast<EventType>(type)), eventData_(std::move(data)) {}

    /**
     * @brief 获取布局事件类型
     */
    LayoutEventType getLayoutEventType() const {
        return static_cast<LayoutEventType>(getType());
    }

    /**
     * @brief 获取事件数据
     */
    const LayoutEventData& getEventData() const {
        return eventData_;
    }

    /**
     * @brief 获取事件数据（可变）
     */
    LayoutEventData& getEventData() {
        return eventData_;
    }

    /**
     * @brief 访问事件数据（类型安全）
     * @tparam T 数据类型
     * @param visitor 访问器函数
     */
    template<typename T, typename Visitor>
    decltype(auto) visitData(Visitor&& visitor) {
        return std::visit(std::forward<Visitor>(visitor),
                         std::get<T>(eventData_));
    }

    /**
     * @brief 访问事件数据（类型安全，const版本）
     * @tparam T 数据类型
     * @param visitor 访问器函数
     */
    template<typename T, typename Visitor>
    decltype(auto) visitData(Visitor&& visitor) const {
        return std::visit(std::forward<Visitor>(visitor),
                         std::get<T>(eventData_));
    }

protected:
    LayoutEventData eventData_; ///< 事件数据
};

/**
 * @brief 布局显示请求事件
 */
class LayoutShowRequestEvent : public LayoutEvent {
public:
    explicit LayoutShowRequestEvent(std::string_view layoutName, std::optional<std::string_view> reason = std::nullopt)
        : LayoutEvent(LayoutEventType::LAYOUT_SHOW_REQUEST,
                    LayoutVisibilityData(layoutName, true, reason)) {}

    std::string getName() const override { return "LayoutShowRequest"; }
};

/**
 * @brief 布局隐藏请求事件
 */
class LayoutHideRequestEvent : public LayoutEvent {
public:
    explicit LayoutHideRequestEvent(std::string_view layoutName, std::optional<std::string_view> reason = std::nullopt)
        : LayoutEvent(LayoutEventType::LAYOUT_HIDE_REQUEST,
                    LayoutVisibilityData(layoutName, false, reason)) {}

    std::string getName() const override { return "LayoutHideRequest"; }
};

/**
 * @brief 布局切换请求事件
 */
class LayoutSwitchRequestEvent : public LayoutEvent {
public:
    LayoutSwitchRequestEvent(std::string_view fromLayout, std::string_view toLayout,
                           std::string_view reason = "", bool animated = true)
        : LayoutEvent(LayoutEventType::LAYOUT_SWITCH_REQUEST,
                    LayoutSwitchData(fromLayout, toLayout, reason, animated)) {}

    std::string getName() const override { return "LayoutSwitchRequest"; }
};

/**
 * @brief 布局事件处理器类型定义（C++17 std::function）
 */
using LayoutEventHandler = std::function<bool(const LayoutEvent&)>;

/**
 * @brief 布局事件调度器
 * 专门处理布局相关事件的调度器
 */
class LayoutEventDispatcher {
public:
    LayoutEventDispatcher() = default;
    ~LayoutEventDispatcher() = default;

    /**
     * @brief 订阅布局事件
     * @param type 事件类型
     * @param handler 事件处理器
     */
    void subscribe(LayoutEventType type, LayoutEventHandler handler);

    /**
     * @brief 取消订阅布局事件
     * @param type 事件类型
     */
    void unsubscribe(LayoutEventType type);

    /**
     * @brief 分发布局事件
     * @param event 布局事件
     */
    bool dispatch(const LayoutEvent& event);

    /**
     * @brief 清除所有订阅
     */
    void clear();

private:
    std::unordered_map<LayoutEventType, std::vector<LayoutEventHandler>, LayoutEventTypeHash> handlers_;
};

/**
 * @brief 布局事件系统便利函数
 */
namespace LayoutEventUtils {
    /**
     * @brief 发送布局显示请求
     * @param layoutName 布局名称
     * @param reason 原因（可选）
     */
    inline void requestShowLayout(std::string_view layoutName, std::optional<std::string_view> reason = std::nullopt);

    /**
     * @brief 发送布局隐藏请求
     * @param layoutName 布局名称
     * @param reason 原因（可选）
     */
    inline void requestHideLayout(std::string_view layoutName, std::optional<std::string_view> reason = std::nullopt);

    /**
     * @brief 发送布局切换请求
     * @param fromLayout 源布局名称
     * @param toLayout 目标布局名称
     * @param reason 原因（可选）
     * @param animated 是否使用动画
     */
    inline void requestSwitchLayout(std::string_view fromLayout, std::string_view toLayout,
                                 std::string_view reason = "", bool animated = true);
}

// 便利宏定义
#define LAYOUT_EVENT_DISPATCHER() DearTs::Core::Events::LayoutEventUtils

} // namespace Events
} // namespace Core
} // namespace DearTs