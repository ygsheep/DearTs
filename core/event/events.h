#pragma once

#include "event.h"
#include <string>

namespace DearTs {
namespace Core {
namespace Event {

/**
 * @brief 应用程序生命周期事件
 */

class EventApplicationInitialized : public Event {
public:
    [[nodiscard]] std::string get_name() const override {
        return "EventApplicationInitialized";
    }

    [[nodiscard]] size_t get_type_id() const override {
        return static_cast<size_t>(EventType::ApplicationInitialized);
    }

    static constexpr size_t get_static_type_id() {
        return static_cast<size_t>(EventType::ApplicationInitialized);
    }

private:
    enum EventType {
        ApplicationInitialized = 1
    };
};

class EventApplicationShutdown : public Event {
public:
    explicit EventApplicationShutdown(int exit_code)
        : m_exit_code(exit_code) {}

    [[nodiscard]] std::string get_name() const override {
        return "EventApplicationShutdown";
    }

    [[nodiscard]] size_t get_type_id() const override {
        return static_cast<size_t>(EventType::ApplicationShutdown);
    }

    [[nodiscard]] int get_exit_code() const {
        return m_exit_code;
    }

    static constexpr size_t get_static_type_id() {
        return static_cast<size_t>(EventType::ApplicationShutdown);
    }

private:
    enum EventType {
        ApplicationShutdown = 2
    };

    int m_exit_code;
};

/**
 * @brief 窗口事件
 */

class EventWindowClose : public Event {
public:
    [[nodiscard]] std::string get_name() const override {
        return "EventWindowClose";
    }

    [[nodiscard]] size_t get_type_id() const override {
        return static_cast<size_t>(EventType::WindowClose);
    }

    static constexpr size_t get_static_type_id() {
        return static_cast<size_t>(EventType::WindowClose);
    }

private:
    enum EventType {
        WindowClose = 10
    };
};

class EventWindowResize : public Event {
public:
    explicit EventWindowResize(int width, int height)
        : m_width(width)
        , m_height(height) {}

    [[nodiscard]] std::string get_name() const override {
        return "EventWindowResize";
    }

    [[nodiscard]] size_t get_type_id() const override {
        return static_cast<size_t>(EventType::WindowResize);
    }

    [[nodiscard]] int get_width() const {
        return m_width;
    }

    [[nodiscard]] int get_height() const {
        return m_height;
    }

    static constexpr size_t get_static_type_id() {
        return static_cast<size_t>(EventType::WindowResize);
    }

private:
    enum EventType {
        WindowResize = 11
    };

    int m_width;
    int m_height;
};

/**
 * @brief 帧事件
 */

class EventFrameBegin : public Event {
public:
    explicit EventFrameBegin(double delta_time)
        : m_delta_time(delta_time) {}

    [[nodiscard]] std::string get_name() const override {
        return "EventFrameBegin";
    }

    [[nodiscard]] size_t get_type_id() const override {
        return static_cast<size_t>(EventType::FrameBegin);
    }

    [[nodiscard]] double get_delta_time() const {
        return m_delta_time;
    }

    static constexpr size_t get_static_type_id() {
        return static_cast<size_t>(EventType::FrameBegin);
    }

private:
    enum EventType {
        FrameBegin = 20
    };

    double m_delta_time;
};

class EventFrameEnd : public Event {
public:
    [[nodiscard]] std::string get_name() const override {
        return "EventFrameEnd";
    }

    [[nodiscard]] size_t get_type_id() const override {
        return static_cast<size_t>(EventType::FrameEnd);
    }

    static constexpr size_t get_static_type_id() {
        return static_cast<size_t>(EventType::FrameEnd);
    }

private:
    enum EventType {
        FrameEnd = 21
    };
};

/**
 * @brief 键盘事件
 */

class EventKeyPressed : public Event {
public:
    explicit EventKeyPressed(int key_code, bool repeat)
        : m_key_code(key_code)
        , m_repeat(repeat) {}

    [[nodiscard]] std::string get_name() const override {
        return "EventKeyPressed";
    }

    [[nodiscard]] size_t get_type_id() const override {
        return static_cast<size_t>(EventType::KeyPressed);
    }

    [[nodiscard]] int get_key_code() const {
        return m_key_code;
    }

    [[nodiscard]] bool is_repeat() const {
        return m_repeat;
    }

    static constexpr size_t get_static_type_id() {
        return static_cast<size_t>(EventType::KeyPressed);
    }

private:
    enum EventType {
        KeyPressed = 30
    };

    int m_key_code;
    bool m_repeat;
};

/**
 * @brief 请求事件（用于模块间通信）
 */

class EventRequestExit : public Event {
public:
    explicit EventRequestExit(int exit_code = 0)
        : m_exit_code(exit_code) {}

    [[nodiscard]] std::string get_name() const override {
        return "EventRequestExit";
    }

    [[nodiscard]] size_t get_type_id() const override {
        return static_cast<size_t>(EventType::RequestExit);
    }

    [[nodiscard]] int get_exit_code() const {
        return m_exit_code;
    }

    static constexpr size_t get_static_type_id() {
        return static_cast<size_t>(EventType::RequestExit);
    }

private:
    enum EventType {
        RequestExit = 100
    };

    int m_exit_code;
};

} // namespace Event
} // namespace Core
} // namespace DearTs
