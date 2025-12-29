/**
 * @file result.h
 * @brief Result 类型用于统一错误处理
 * @details 提供类型安全的错误处理机制，避免异常
 * @author DearTs Team
 * @date 2024
 * @version 1.0.0
 */

#pragma once

#include <variant>
#include <string>
#include <stdexcept>
#include <format>
#include <type_traits>

namespace DearTs::Core {

// 内部错误类型包装器（用于 Result<T, T> 的情况）
namespace detail {
    template<typename T>
    struct ErrorWrapper {
        T value;
    };
}

template<typename T, typename E>
class Result;

/**
 * @brief Result 类型用于表示可能失败的操作
 * @tparam T 成功时的返回值类型
 * @tparam E 失败时的错误类型，默认为 std::string
 *
 * @example
 * // 使用示例
 * Result<int, std::string> divide(int a, int b) {
 *     if (b == 0) {
 *         return Result::err("Division by zero");
 *     }
 *     return Result::ok(a / b);
 * }
 *
 * auto result = divide(10, 2);
 * if (result.isOk()) {
 *     LOG_INFO("Result: {}", result.unwrap());
 * } else {
 *     LOG_ERROR("Error: {}", result.error());
 * }
 */
template<typename T, typename E = std::string>
class Result {
    // 使用条件编译处理 T 和 E 相同的情况
    using VariantType = std::conditional_t<
        std::is_same_v<T, E>,
        std::variant<T, detail::ErrorWrapper<E>>,
        std::variant<T, E>
    >;
public:
    using value_type = T;
    using error_type = E;

    /**
     * @brief 构造成功结果
     */
    static Result ok(T value) {
        if constexpr (std::is_same_v<T, E>) {
            return Result(std::in_place_index<0>, std::move(value));
        } else {
            return Result(std::in_place_index<0>, std::move(value));
        }
    }

    /**
     * @brief 构造失败结果
     */
    static Result err(E error) {
        if constexpr (std::is_same_v<T, E>) {
            return Result(std::in_place_index<1>, detail::ErrorWrapper<E>{std::move(error)});
        } else {
            return Result(std::in_place_index<1>, std::move(error));
        }
    }

    // 拷贝构造
    Result(const Result&) = default;
    Result& operator=(const Result&) = default;

    // 移动构造
    Result(Result&&) noexcept = default;
    Result& operator=(Result&&) noexcept = default;

    /**
     * @brief 检查是否为成功结果
     */
    [[nodiscard]] bool isOk() const {
        return m_data.index() == 0;
    }

    /**
     * @brief 检查是否为失败结果
     */
    [[nodiscard]] bool isErr() const {
        return m_data.index() == 1;
    }

    /**
     * @brief 获取成功值
     * @throws std::runtime_error 如果结果为错误
     */
    [[nodiscard]] T& unwrap() {
        if (isErr()) {
            throw std::runtime_error("Called unwrap on error result");
        }
        if constexpr (std::is_same_v<T, E>) {
            return std::get<0>(m_data);
        } else {
            return std::get<0>(m_data);
        }
    }

    /**
     * @brief 获取成功值（const 版本）
     * @throws std::runtime_error 如果结果为错误
     */
    [[nodiscard]] const T& unwrap() const {
        if (isErr()) {
            throw std::runtime_error("Called unwrap on error result");
        }
        if constexpr (std::is_same_v<T, E>) {
            return std::get<0>(m_data);
        } else {
            return std::get<0>(m_data);
        }
    }

    /**
     * @brief 获取成功值，如果失败则返回默认值
     */
    [[nodiscard]] T unwrap_or(T default_value) const {
        if (isOk()) {
            return std::get<0>(m_data);
        }
        return default_value;
    }

    /**
     * @brief 获取错误值
     * @throws std::runtime_error 如果结果为成功
     */
    [[nodiscard]] E& error() {
        if (isOk()) {
            throw std::runtime_error("Called error on success result");
        }
        if constexpr (std::is_same_v<T, E>) {
            return std::get<1>(m_data).value;
        } else {
            return std::get<1>(m_data);
        }
    }

    /**
     * @brief 获取错误值（const 版本）
     * @throws std::runtime_error 如果结果为成功
     */
    [[nodiscard]] const E& error() const {
        if (isOk()) {
            throw std::runtime_error("Called error on success result");
        }
        if constexpr (std::is_same_v<T, E>) {
            return std::get<1>(m_data).value;
        } else {
            return std::get<1>(m_data);
        }
    }

    /**
     * @brief 映射成功值
     */
    template<typename U, typename F>
    [[nodiscard]] auto map(F&& f) -> Result<U, E> {
        if (isOk()) {
            return Result<U, E>::ok(f(std::get<0>(m_data)));
        }
        return Result<U, E>::err(std::get<1>(m_data));
    }

    /**
     * @brief 映射错误值
     */
    template<typename F, typename G>
    [[nodiscard]] auto map_err(F&& f) -> Result<T, G> {
        if (isErr()) {
            if constexpr (std::is_same_v<T, E>) {
                return Result<T, G>::err(f(std::get<1>(m_data).value));
            } else {
                return Result<T, G>::err(f(std::get<1>(m_data)));
            }
        }
        if constexpr (std::is_same_v<T, E>) {
            return Result<T, G>::ok(std::get<0>(m_data));
        } else {
            return Result<T, G>::ok(std::get<0>(m_data));
        }
    }

    /**
     * @brief 链式调用
     */
    template<typename U, typename F>
    [[nodiscard]] auto and_then(F&& f) -> Result<U, E> {
        if (isOk()) {
            return f(std::get<0>(m_data));
        }
        if constexpr (std::is_same_v<T, E>) {
            return Result<U, E>::err(std::get<1>(m_data).value);
        } else {
            return Result<U, E>::err(std::get<1>(m_data));
        }
    }

private:
    VariantType m_data;

    Result(std::in_place_index_t<0>, T value)
        : m_data(std::move(value)) {}

    Result(std::in_place_index_t<1>, E error)
        : m_data(std::move(error)) {}

    Result(std::in_place_index_t<1>, detail::ErrorWrapper<E> error)
        : m_data(std::move(error)) {}
};

/**
 * @brief void 特化版本（用于没有返回值的操作）
 * @tparam E 错误类型
 */
template<typename E>
class Result<void, E> {
public:
    using value_type = void;
    using error_type = E;

    /**
     * @brief 构造成功结果
     */
    static Result ok() {
        return Result(true);
    }

    /**
     * @brief 构造失败结果
     */
    static Result err(E error) {
        return Result(std::move(error));
    }

    // 默认构造函数（删除）
    Result() = delete;

    // 拷贝构造
    Result(const Result&) = default;
    Result& operator=(const Result&) = default;

    // 移动构造
    Result(Result&&) noexcept = default;
    Result& operator=(Result&&) noexcept = default;

    /**
     * @brief 检查是否为成功结果
     */
    [[nodiscard]] bool isOk() const {
        return m_is_ok;
    }

    /**
     * @brief 检查是否为失败结果
     */
    [[nodiscard]] bool isErr() const {
        return !m_is_ok;
    }

    /**
     * @brief 获取错误值
     * @throws std::runtime_error 如果结果为成功
     */
    [[nodiscard]] E& error() {
        if (isOk()) {
            throw std::runtime_error("Called error on success result");
        }
        return m_error;
    }

    /**
     * @brief 获取错误值（const 版本）
     * @throws std::runtime_error 如果结果为成功
     */
    [[nodiscard]] const E& error() const {
        if (isOk()) {
            throw std::runtime_error("Called error on success result");
        }
        return m_error;
    }

    /**
     * @brief 映射错误值
     */
    template<typename F, typename G>
    [[nodiscard]] auto map_err(F&& f) -> Result<void, G> {
        if (isErr()) {
            return Result<void, G>::err(f(m_error));
        }
        return Result<void, G>::ok();
    }

private:
    bool m_is_ok;
    E m_error;

    explicit Result(bool is_ok)
        : m_is_ok(is_ok), m_error() {}

    explicit Result(E error)
        : m_is_ok(false), m_error(std::move(error)) {}
};

/**
 * @brief 便捷宏：用于快速返回错误
 */
#define TRY_RESULT(expr) \
    do { \
        auto _result = (expr); \
        if (_result.isErr()) { \
            return Result::err(_result.error()); \
        } \
    } while(0)

/**
 * @brief 便捷宏：用于解包 Result 值
 */
#define UNWRAP_RESULT(var, expr) \
    auto _result_##var = (expr); \
    if (_result_##var.isErr()) { \
        return Result::err(_result_##var.error()); \
    } \
    auto var = _result_##var.unwrap()

} // namespace DearTs::Core
