/**
 * @file profiler.h
 * @brief 简化的性能分析器
 * @author DearTs Team
 * @date 2024
 */

#pragma once

#ifndef DEARTS_PROFILER_H
#define DEARTS_PROFILER_H

// 直接包含必要的标准库头文件以避免预编译头文件问题
#include <chrono>
#include <string>
#include <unordered_map>
#include <memory>
#include <fstream>

namespace DearTs {
namespace Core {
namespace Utils {

/**
 * @brief 简单的性能计时器
 */
class SimpleTimer {
public:
    /**
     * @brief 构造函数，开始计时
     */
    SimpleTimer() : start_time_(std::chrono::high_resolution_clock::now()) {}
    
    /**
     * @brief 获取经过的时间（毫秒）
     * @return 经过的时间
     */
    double getElapsedMs() const {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time_);
        return duration.count() / 1000.0;
    }
    
    /**
     * @brief 重置计时器
     */
    void reset() {
        start_time_ = std::chrono::high_resolution_clock::now();
    }
    
private:
    std::chrono::high_resolution_clock::time_point start_time_;
};

/**
 * @brief 简化的性能分析器
 */
class Profiler {
public:
    static Profiler& getInstance();

    void initialize();
    void shutdown();

    // 性能分析相关方法
    void beginSession(const std::string& name);
    void endSession();
    void writeProfile(const char* name);

private:
    Profiler() = default;
    ~Profiler() = default;
    Profiler(const Profiler&) = delete;
    Profiler& operator=(const Profiler&) = delete;

    std::ofstream m_outputStream;
    int m_profileCount = 0;
};

} // namespace Utils
} // namespace Core
} // namespace DearTs

// 便利宏定义
#ifdef DEARTS_DEBUG
    #define DEARTS_PROFILE_TIMER(name) DearTs::Core::Utils::SimpleTimer timer_##name
    #define DEARTS_PROFILE_START(name) DearTs::Core::Utils::Profiler::getInstance().beginSession(name)
    #define DEARTS_PROFILE_END(name) DearTs::Core::Utils::Profiler::getInstance().endSession()
    #define DEARTS_PROFILE_FRAME(time) DearTs::Core::Utils::Profiler::getInstance().writeProfile(#time)
#else
    #define DEARTS_PROFILE_TIMER(name)
    #define DEARTS_PROFILE_START(name)
    #define DEARTS_PROFILE_END(name)
    #define DEARTS_PROFILE_FRAME(time)
#endif

#endif // DEARTS_PROFILER_H