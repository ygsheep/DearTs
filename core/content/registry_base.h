/**
 * @file registry_base.h
 * @brief Content Registry 基础类和通用定义
 * @author DearTs Team
 * @date 2025
 * @version 1.0.0
 */

#pragma once

#include <string>
#include <functional>

namespace DearTs::Core::ContentRegistry {

/**
 * @brief 非本地化字符串（用于标识）
 *
 * 所有需要本地化的字符串都应该使用此类
 */
class UnlocalizedString {
public:
    // 默认构造函数
    UnlocalizedString() = default;

    explicit UnlocalizedString(std::string string)
        : m_string(std::move(string)) {}

    // 方便的构造函数，接受const char*
    UnlocalizedString(const char* string)
        : m_string(string) {}

    [[nodiscard]] const std::string& get() const {
        return m_string;
    }

    [[nodiscard]] operator std::string() const {
        return m_string;
    }

    [[nodiscard]] bool operator==(const UnlocalizedString& other) const {
        return m_string == other.m_string;
    }

    [[nodiscard]] bool operator<(const UnlocalizedString& other) const {
        return m_string < other.m_string;
    }

private:
    std::string m_string;
};

/**
 * @brief 通用回调函数类型
 */
using Callback = std::function<void()>;

/**
 * @brief 带参数的回调函数类型
 */
template<typename... Args>
using ParamCallback = std::function<void(Args...)>;

} // namespace DearTs::Core::ContentRegistry
