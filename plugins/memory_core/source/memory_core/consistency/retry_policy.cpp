/**
 * @file retry_policy.cpp
 * @brief 重试策略实现
 */

#include "memory_core/consistency/retry_policy.hpp"
#include "liblogger/logger.h"
#include <thread>
#include <algorithm>
#include <sstream>
#include <cmath>

namespace DearTs::Plugins::MemoryCore::Consistency {

// ============ 单例 ============

RetryPolicy& RetryPolicy::instance() {
    static RetryPolicy s_instance;
    return s_instance;
}

// ============ 初始化 ============

DearTs::Core::Result<void, std::string> RetryPolicy::initialize(const RetryPolicyConfig& config) {
    m_config = config;
    m_stats = RetryStats::empty();
    m_random_engine.seed(std::random_device{}());
    return DearTs::Core::Result<void, std::string>::ok();
}

// ============ 重试控制 ============

std::chrono::milliseconds RetryPolicy::calculate_delay(int attempt_number) const {
    if (attempt_number < 1) {
        attempt_number = 1;
    }

    std::chrono::milliseconds delay;

    switch (m_config.backoff_strategy) {
        case BackoffStrategy::Fixed:
            delay = m_config.initial_delay;
            break;

        case BackoffStrategy::Linear:
            delay = m_config.initial_delay * attempt_number;
            break;

        case BackoffStrategy::Exponential:
        case BackoffStrategy::ExponentialWithJitter: {
            // 指数退避：delay = initial_delay * 2^(attempt_number - 1)
            int64_t delay_ms = m_config.initial_delay.count();
            for (int i = 1; i < attempt_number; ++i) {
                delay_ms *= 2;
            }
            delay = std::chrono::milliseconds(delay_ms);

            // 应用抖动
            if (m_config.backoff_strategy == BackoffStrategy::ExponentialWithJitter) {
                delay = apply_jitter(delay);
            }
            break;
        }

        default:
            delay = m_config.initial_delay;
            break;
    }

    // 限制最大延迟
    if (delay > m_config.max_delay) {
        delay = m_config.max_delay;
    }

    return delay;
}

bool RetryPolicy::should_retry(const std::string& error_message, int attempt_number) const {
    // 检查尝试次数
    if (attempt_number >= m_config.max_attempts) {
        return false;
    }

    // 使用自定义条件（如果设置）
    if (m_custom_condition) {
        return m_custom_condition(error_message, attempt_number);
    }

    // 默认重试条件
    bool should = false;

    // 网络错误
    if (m_config.enable_retry_on_network_error) {
        if (error_message.find("network") != std::string::npos ||
            error_message.find("connection") != std::string::npos ||
            error_message.find("timeout") != std::string::npos ||
            error_message.find("ECONNREFUSED") != std::string::npos ||
            error_message.find("ETIMEDOUT") != std::string::npos) {
            should = true;
        }
    }

    // 超时错误
    if (m_config.enable_retry_on_timeout) {
        if (error_message.find("timeout") != std::string::npos ||
            error_message.find("timed out") != std::string::npos) {
            should = true;
        }
    }

    // 服务器错误（5xx）
    if (error_message.find("500") != std::string::npos ||
        error_message.find("502") != std::string::npos ||
        error_message.find("503") != std::string::npos ||
        error_message.find("504") != std::string::npos) {
        should = true;
    }

    return should;
}

void RetryPolicy::set_retry_condition(RetryCondition condition) {
    m_custom_condition = std::move(condition);
}

DearTs::Core::Result<void, std::string> RetryPolicy::execute_with_retry(
    std::function<DearTs::Core::Result<void, std::string>()> operation
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
            return DearTs::Core::Result<void, std::string>::ok();
        }

        // 记录错误
        last_error = result.error();
        m_stats.failed_attempts++;

        // 检查是否应该重试
        if (!should_retry(last_error, attempt)) {
            LOG_WARN("Retry policy giving up after {} attempts: {}",
                     attempt, last_error);
            return DearTs::Core::Result<void, std::string>::err(last_error);
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
    return DearTs::Core::Result<void, std::string>::err(last_error);
}

// ============ 静态工具方法 ============

RetryPolicyConfig RetryPolicy::fixed_policy(
    std::chrono::milliseconds delay,
    int max_attempts
) {
    return RetryPolicyConfig{
        .max_attempts = max_attempts,
        .initial_delay = delay,
        .max_delay = delay,
        .backoff_strategy = BackoffStrategy::Fixed,
        .jitter_factor = 0.0,
        .enable_retry_on_timeout = true,
        .enable_retry_on_network_error = true
    };
}

RetryPolicyConfig RetryPolicy::exponential_policy(
    std::chrono::milliseconds initial_delay,
    std::chrono::milliseconds max_delay,
    int max_attempts
) {
    return RetryPolicyConfig{
        .max_attempts = max_attempts,
        .initial_delay = initial_delay,
        .max_delay = max_delay,
        .backoff_strategy = BackoffStrategy::Exponential,
        .jitter_factor = 0.0,
        .enable_retry_on_timeout = true,
        .enable_retry_on_network_error = true
    };
}

RetryPolicyConfig RetryPolicy::linear_policy(
    std::chrono::milliseconds initial_delay,
    std::chrono::milliseconds increment,
    std::chrono::milliseconds max_delay,
    int max_attempts
) {
    // 线性策略使用 increment 计算 max_delay
    return RetryPolicyConfig{
        .max_attempts = max_attempts,
        .initial_delay = initial_delay,
        .max_delay = max_delay,
        .backoff_strategy = BackoffStrategy::Linear,
        .jitter_factor = 0.0,
        .enable_retry_on_timeout = true,
        .enable_retry_on_network_error = true
    };
}

// ============ 私有辅助方法 ============

std::chrono::milliseconds RetryPolicy::apply_jitter(std::chrono::milliseconds delay) const {
    if (m_config.jitter_factor <= 0.0) {
        return delay;
    }

    // 计算抖动范围：[delay * (1 - jitter), delay * (1 + jitter)]
    std::uniform_real_distribution<double> dist(
        1.0 - m_config.jitter_factor,
        1.0 + m_config.jitter_factor
    );

    double jittered = delay.count() * dist(m_random_engine);
    return std::chrono::milliseconds(static_cast<int64_t>(jittered));
}

void RetryPolicy::update_stats(std::chrono::milliseconds delay, bool success) {
    m_stats.last_delay = delay;
    m_stats.total_delay += delay;

    if (success) {
        m_stats.successful_attempts++;
    } else {
        m_stats.failed_attempts++;
    }
    m_stats.total_attempts++;
}

void RetryPolicy::log_stats() const {
    LOG_INFO("RetryPolicy Stats: total_attempts={}, successful={}, failed={}, "
             "total_delay={} ms, last_delay={} ms",
             m_stats.total_attempts, m_stats.successful_attempts,
             m_stats.failed_attempts, m_stats.total_delay.count(),
             m_stats.last_delay.count());
}

} // namespace DearTs::Plugins::MemoryCore::Consistency
