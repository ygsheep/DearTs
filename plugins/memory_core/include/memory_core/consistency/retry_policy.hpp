/**
 * @file retry_policy.hpp
 * @brief 重试策略 - 定义操作重试行为
 *
 * 功能：
 * - 多种退避策略
 * - 可配置重试次数和间隔
 * - 抖动支持
 * - 条件重试
 */

#pragma once

#include "core/result.h"
#include "liblogger/logger.h"
#include <string>
#include <vector>
#include <functional>
#include <random>
#include <chrono>
#include <thread>
#include <optional>

namespace DearTs::Plugins::MemoryCore::Consistency {

/**
 * @brief 退避策略类型
 */
enum class BackoffStrategy {
    Fixed,                 ///< 固定间隔
    Linear,                ///< 线性增长
    Exponential,           ///< 指数增长
    ExponentialWithJitter  ///< 指数增长 + 抖动
};

/**
 * @brief 重试结果
 */
enum class RetryResult {
    Success,               ///< 成功
    Retry,                 ///< 需要重试
    GiveUp                 ///< 放弃重试
};

/**
 * @brief 重试策略配置
 */
struct RetryPolicyConfig {
    int max_attempts;                      ///< 最大尝试次数
    std::chrono::milliseconds initial_delay; ///< 初始延迟
    std::chrono::milliseconds max_delay;    ///< 最大延迟
    BackoffStrategy backoff_strategy;       ///< 退避策略
    double jitter_factor;                   ///< 抖动因子 [0-1]
    bool enable_retry_on_timeout;           ///< 超时时是否重试
    bool enable_retry_on_network_error;     ///< 网络错误时是否重试

    /**
     * @brief 默认配置
     */
    static RetryPolicyConfig default_config() {
        return RetryPolicyConfig{
            .max_attempts = 3,
            .initial_delay = std::chrono::milliseconds(1000),
            .max_delay = std::chrono::milliseconds(30000),
            .backoff_strategy = BackoffStrategy::ExponentialWithJitter,
            .jitter_factor = 0.1,
            .enable_retry_on_timeout = true,
            .enable_retry_on_network_error = true
        };
    }

    /**
     * @brief 激进配置（快速重试）
     */
    static RetryPolicyConfig aggressive_config() {
        return RetryPolicyConfig{
            .max_attempts = 5,
            .initial_delay = std::chrono::milliseconds(200),
            .max_delay = std::chrono::milliseconds(5000),
            .backoff_strategy = BackoffStrategy::Exponential,
            .jitter_factor = 0.05,
            .enable_retry_on_timeout = true,
            .enable_retry_on_network_error = true
        };
    }

    /**
     * @brief 保守配置（慢速重试）
     */
    static RetryPolicyConfig conservative_config() {
        return RetryPolicyConfig{
            .max_attempts = 2,
            .initial_delay = std::chrono::milliseconds(2000),
            .max_delay = std::chrono::milliseconds(60000),
            .backoff_strategy = BackoffStrategy::Fixed,
            .jitter_factor = 0.0,
            .enable_retry_on_timeout = false,
            .enable_retry_on_network_error = true
        };
    }
};

/**
 * @brief 重试统计
 */
struct RetryStats {
    int total_attempts;                     ///< 总尝试次数
    int successful_attempts;                ///< 成功次数
    int failed_attempts;                    ///< 失败次数
    std::chrono::milliseconds total_delay;  ///< 总延迟时间
    std::chrono::milliseconds last_delay;   ///< 最后一次延迟

    /**
     * @brief 创建空统计
     */
    static RetryStats empty() {
        return RetryStats{
            .total_attempts = 0,
            .successful_attempts = 0,
            .failed_attempts = 0,
            .total_delay = std::chrono::milliseconds(0),
            .last_delay = std::chrono::milliseconds(0)
        };
    }
};

/**
 * @brief 重试条件函数类型
 *
 * @param error_message 错误信息
 * @param attempt_number 当前尝试次数
 * @return true 表示应该重试，false 表示放弃
 */
using RetryCondition = std::function<bool(const std::string& error_message, int attempt_number)>;

/**
 * @brief 重试策略
 *
 * 管理操作重试行为，支持多种退避策略
 */
class RetryPolicy {
public:
    /**
     * @brief 获取单例实例
     */
    static RetryPolicy& instance();

    /**
     * @brief 删除拷贝构造和赋值
     */
    RetryPolicy(const RetryPolicy&) = delete;
    RetryPolicy& operator=(const RetryPolicy&) = delete;

    // ============ 重试控制 ============

    /**
     * @brief 计算下次重试延迟
     * @param attempt_number 当前尝试次数（从 1 开始）
     * @return 延迟时间
     */
    std::chrono::milliseconds calculate_delay(int attempt_number) const;

    /**
     * @brief 判断是否应该重试
     * @param error_message 错误信息
     * @param attempt_number 当前尝试次数
     * @return true 表示应该重试
     */
    bool should_retry(const std::string& error_message, int attempt_number) const;

    /**
     * @brief 设置自定义重试条件
     * @param condition 重试条件函数
     */
    void set_retry_condition(RetryCondition condition);

    /**
     * @brief 执行带重试的操作
     * @param operation 要执行的操作
     * @return 操作结果或错误信息
     */
    template<typename T>
    DearTs::Core::Result<T, std::string> execute_with_retry(
        std::function<DearTs::Core::Result<T, std::string>()> operation
    );

    /**
     * @brief 执行带重试的操作（无返回值）
     * @param operation 要执行的操作
     * @return 操作结果或错误信息
     */
    DearTs::Core::Result<void, std::string> execute_with_retry(
        std::function<DearTs::Core::Result<void, std::string>()> operation
    );

    // ============ 配置 ============

    /**
     * @brief 获取配置
     * @return 当前配置
     */
    const RetryPolicyConfig& get_config() const {
        return m_config;
    }

    /**
     * @brief 更新配置
     * @param config 新配置
     */
    void set_config(const RetryPolicyConfig& config) {
        m_config = config;
    }

    // ============ 统计 ============

    /**
     * @brief 获取统计信息
     * @return 统计数据
     */
    const RetryStats& get_stats() const {
        return m_stats;
    }

    /**
     * @brief 重置统计信息
     */
    void reset_stats() {
        m_stats = RetryStats::empty();
    }

    /**
     * @brief 打印统计信息
     */
    void log_stats() const;

    // ============ 静态工具方法 ============

    /**
     * @brief 创建固定间隔策略
     * @param delay 延迟时间
     * @param max_attempts 最大尝试次数
     * @return 策略配置
     */
    static RetryPolicyConfig fixed_policy(
        std::chrono::milliseconds delay,
        int max_attempts = 3
    );

    /**
     * @brief 创建指数退避策略
     * @param initial_delay 初始延迟
     * @param max_delay 最大延迟
     * @param max_attempts 最大尝试次数
     * @return 策略配置
     */
    static RetryPolicyConfig exponential_policy(
        std::chrono::milliseconds initial_delay,
        std::chrono::milliseconds max_delay,
        int max_attempts = 3
    );

    /**
     * @brief 创建线性退避策略
     * @param initial_delay 初始延迟
     * @param increment 每次增量
     * @param max_delay 最大延迟
     * @param max_attempts 最大尝试次数
     * @return 策略配置
     */
    static RetryPolicyConfig linear_policy(
        std::chrono::milliseconds initial_delay,
        std::chrono::milliseconds increment,
        std::chrono::milliseconds max_delay,
        int max_attempts = 3
    );

    // ============ 初始化 ============

    /**
     * @brief 初始化重试策略
     * @param config 策略配置
     * @return 成功或错误信息
     */
    DearTs::Core::Result<void, std::string> initialize(
        const RetryPolicyConfig& config = RetryPolicyConfig::default_config()
    );

private:
    /**
     * @brief 私有构造函数（单例模式）
     */
    RetryPolicy() = default;

    /**
     * @brief 析构函数
     */
    ~RetryPolicy() = default;

    /**
     * @brief 应用抖动
     * @param delay 原始延迟
     * @return 带抖动的延迟
     */
    std::chrono::milliseconds apply_jitter(std::chrono::milliseconds delay) const;

    /**
     * @brief 更新统计
     * @param delay 延迟时间
     * @param success 是否成功
     */
    void update_stats(std::chrono::milliseconds delay, bool success);

    // ============ 成员变量 ============

    RetryPolicyConfig m_config;                     ///< 配置
    RetryStats m_stats;                             ///< 统计
    RetryCondition m_custom_condition;              ///< 自定义重试条件
    mutable std::mt19937 m_random_engine;           ///< 随机数生成器
};

// ============ 模板方法实现 ============

template<typename T>
DearTs::Core::Result<T, std::string> RetryPolicy::execute_with_retry(
    std::function<DearTs::Core::Result<T, std::string>()> operation
) {
    int attempt = 0;
    std::string last_error;

    while (attempt < m_config.max_attempts) {
        attempt++;
        m_stats.total_attempts++;

        // 执行操作
        auto result = operation();
        if (result.isOk()) {
            m_stats.successful_attempts++;
            return result;
        }

        // 记录错误
        last_error = result.error();
        m_stats.failed_attempts++;

        // 检查是否应该重试
        if (!should_retry(last_error, attempt)) {
            LOG_WARN("Retry policy giving up after {} attempts: {}",
                     attempt, last_error);
            return DearTs::Core::Result<T, std::string>::err(last_error);
        }

        // 如果还有重试机会，计算延迟并等待
        if (attempt < m_config.max_attempts) {
            auto delay = calculate_delay(attempt);
            m_stats.last_delay = delay;
            m_stats.total_delay += delay;

            LOG_INFO("Retrying in {} ms (attempt {}/{}): {}",
                     delay.count(), attempt, m_config.max_attempts, last_error);

            std::this_thread::sleep_for(delay);
        }
    }

    // 所有尝试都失败
    LOG_ERROR("All {} retry attempts failed: {}", m_config.max_attempts, last_error);
    return DearTs::Core::Result<T, std::string>::err(last_error);
}

} // namespace DearTs::Plugins::MemoryCore::Consistency
