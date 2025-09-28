/**
 * @file logger.h
 * @brief 现代C++20日志系统
 * @details 使用模板、constexpr和source_location实现高性能日志记录
 * @author DearTs Team
 * @date 2024
 */

#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <mutex>
#include <atomic>
#include <memory>
#include <fstream>
#include <thread>
#include <queue>
#include <condition_variable>
#include <filesystem>
#include <unordered_map>
#include <functional>

// 确保定义了 NOMINMAX 宏以避免 Windows.h 中的 min/max 宏冲突
#ifndef NOMINMAX
#define NOMINMAX
#endif

// 确保定义了 WIN32_LEAN_AND_MEAN 宏以减少 Windows.h 的包含内容
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

namespace DearTs {
    namespace Utils {
        
        /**
         * @brief 日志级别枚举
         */
        enum class LogLevel : int {
            LOG_TRACE = 0,
            LOG_DEBUG = 1,
            LOG_INFO = 2,
            LOG_WARN = 3,
            LOG_ERROR = 4,
            LOG_FATAL = 5
        };
        
        /**
         * @brief 现代C++20日志器类
         * @details 线程安全的单例日志器，使用原子操作和互斥锁保证并发安全
         */
        class Logger {
        public:
            /**
             * @brief 获取Logger单例实例
             * @return Logger引用
             */
            static Logger& getInstance() noexcept {
                static Logger instance;
                return instance;
            }
            
            /**
             * @brief 设置日志级别（线程安全）
             * @param level 日志级别
             */
            void setLevel(LogLevel level) noexcept {
                current_level_.store(static_cast<int>(level), std::memory_order_relaxed);
            }
            
            /**
             * @brief 获取当前日志级别（线程安全）
             * @return 当前日志级别
             */
            LogLevel getLevel() const noexcept {
                return static_cast<LogLevel>(current_level_.load(std::memory_order_relaxed));
            }
            
            /**
             * @brief 启用文件输出
             * @param filename 日志文件名
             * @param enable 是否启用文件输出
             */
            void enableFileOutput(const std::string& filename, bool enable = true) {
                std::lock_guard<std::mutex> lock(file_mutex_);
                
                if (enable && !file_output_enabled_.load(std::memory_order_relaxed)) {
                    // 关闭当前文件流（如果已打开）
                    if (file_stream_.is_open()) {
                        file_stream_.close();
                    }
                    
                    // 确保目录存在
                    std::filesystem::path logPath(filename);
                    if (logPath.has_parent_path()) {
                        std::filesystem::create_directories(logPath.parent_path());
                    }
                    
                    // 打开新的日志文件
                    file_stream_.open(filename, std::ios::app);
                    if (file_stream_.is_open()) {
                        log_filename_ = filename;
                        file_output_enabled_.store(true, std::memory_order_relaxed);
                        
                        // 启动文件写入线程（如果尚未运行）
                        if (!writer_running_.load(std::memory_order_relaxed)) {
                            writer_running_.store(true, std::memory_order_relaxed);
                            file_writer_thread_ = std::thread(&Logger::fileWriterThread, this);
                        }
                    }
                } else if (!enable && file_output_enabled_.load(std::memory_order_relaxed)) {
                    // 停止文件输出
                    file_output_enabled_.store(false, std::memory_order_relaxed);
                    
                    // 通知写入线程退出
                    {
                        std::lock_guard<std::mutex> queueLock(queue_mutex_);
                        queue_cv_.notify_all();
                    }
                    
                    // 等待写入线程结束
                    if (file_writer_thread_.joinable()) {
                        // 确保线程知道需要退出
                        writer_running_.store(false, std::memory_order_relaxed);
                        queue_cv_.notify_all(); // 再次通知以确保线程能退出等待
                        file_writer_thread_.join();
                    }
                    
                    // 关闭文件流
                    if (file_stream_.is_open()) {
                        file_stream_.close();
                    }
                    log_filename_.clear();
                }
            }
            
            /**
             * @brief 检查是否启用了文件输出
             * @return 是否启用了文件输出
             */
            bool isFileOutputEnabled() const noexcept {
                return file_output_enabled_.load(std::memory_order_relaxed);
            }
            
            /**
             * @brief 设置缓冲区大小
             * @param size 缓冲区大小（字节）
             */
            void setBufferSize(size_t size) noexcept {
                buffer_size_.store(size, std::memory_order_relaxed);
            }
            
            /**
             * @brief 获取缓冲区大小
             * @return 缓冲区大小（字节）
             */
            size_t getBufferSize() const noexcept {
                return buffer_size_.load(std::memory_order_relaxed);
            }
            
            /**
             * @brief 设置重复日志过滤时间窗口（毫秒）
             * @param windowMs 时间窗口（毫秒）
             */
            void setDuplicateFilterWindow(int windowMs) noexcept {
                duplicate_filter_window_ms_.store(windowMs, std::memory_order_relaxed);
            }
            
            /**
             * @brief 记录日志消息
             * @param level 日志级别
             * @param message 日志消息
             * @param file 源文件名
             * @param line 行号
             */
            void log(LogLevel level, const std::string& message, 
                    const char* file = __FILE__, int line = __LINE__) {
                if (static_cast<int>(level) < current_level_.load(std::memory_order_relaxed)) {
                    return;
                }
                
                // 检查是否为重复日志
                if (isDuplicateMessage(message, file, line)) {
                    return;
                }
                
                // 格式化日志消息
                std::string formattedMessage = formatLogMessage(level, message, file, line);
                
                // 输出到控制台
                {
                    std::lock_guard<std::mutex> lock(output_mutex_);
                    auto& stream = (static_cast<int>(level) >= static_cast<int>(LogLevel::LOG_ERROR)) ? std::cerr : std::cout;
                    stream << formattedMessage << std::endl;
                }
                
                // 输出到文件（如果启用）
                if (file_output_enabled_.load(std::memory_order_relaxed)) {
                    writeToFile(formattedMessage);
                }
            }
            
            /**
             * @brief TRACE级别日志
             * @param message 日志消息
             */
            void trace(const std::string& message, const char* file = __FILE__, int line = __LINE__) {
                log(LogLevel::LOG_TRACE, message, file, line);
            }
            
            /**
             * @brief DEBUG级别日志
             * @param message 日志消息
             */
            void debug(const std::string& message, const char* file = __FILE__, int line = __LINE__) {
                log(LogLevel::LOG_DEBUG, message, file, line);
            }
            
            /**
             * @brief INFO级别日志
             * @param message 日志消息
             */
            void info(const std::string& message, const char* file = __FILE__, int line = __LINE__) {
                log(LogLevel::LOG_INFO, message, file, line);
            }
            
            /**
             * @brief WARN级别日志
             * @param message 日志消息
             */
            void warn(const std::string& message, const char* file = __FILE__, int line = __LINE__) {
                log(LogLevel::LOG_WARN, message, file, line);
            }
            
            /**
             * @brief ERROR级别日志
             * @param message 日志消息
             */
            void error(const std::string& message, const char* file = __FILE__, int line = __LINE__) {
                log(LogLevel::LOG_ERROR, message, file, line);
            }
            
            /**
             * @brief FATAL级别日志
             * @param message 日志消息
             */
            void fatal(const std::string& message, const char* file = __FILE__, int line = __LINE__) {
                log(LogLevel::LOG_FATAL, message, file, line);
            }
            
        private:
            Logger() : writer_running_(false), duplicate_filter_window_ms_(1000) {
                // 不在构造函数中启动线程，而是在enableFileOutput中启动
            }
            
            ~Logger() {
                // 停止文件输出
                enableFileOutput("", false);
                
                // 确保线程已终止
                if (file_writer_thread_.joinable()) {
                    writer_running_.store(false, std::memory_order_relaxed);
                    queue_cv_.notify_all();
                    file_writer_thread_.join();
                }
            }
            Logger(const Logger&) = delete;
            Logger& operator=(const Logger&) = delete;
            Logger(Logger&&) = delete;
            Logger& operator=(Logger&&) = delete;
            
            /**
             * @brief 检查是否为重复消息
             * @param message 日志消息
             * @param file 源文件名
             * @param line 行号
             * @return 是否为重复消息
             */
            bool isDuplicateMessage(const std::string& message, const char* file, int line) {
                // 构造唯一键
                std::string key = std::string(file) + ":" + std::to_string(line) + ":" + message;
                
                // 获取当前时间
                auto now = std::chrono::steady_clock::now();
                auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
                
                std::lock_guard<std::mutex> lock(duplicate_mutex_);
                
                // 查找是否已存在该消息
                auto it = last_message_times_.find(key);
                if (it != last_message_times_.end()) {
                    // 检查是否在过滤时间窗口内
                    if (now_ms - it->second < duplicate_filter_window_ms_.load(std::memory_order_relaxed)) {
                        // 更新最后记录时间
                        it->second = now_ms;
                        return true; // 过滤掉重复消息
                    }
                }
                
                // 更新最后记录时间
                last_message_times_[key] = now_ms;
                return false;
            }
            
            /**
             * @brief 格式化日志消息
             * @param level 日志级别
             * @param message 日志消息
             * @param file 源文件名
             * @param line 行号
             * @return 格式化后的日志消息
             */
            std::string formatLogMessage(LogLevel level, const std::string& message, 
                                       const char* file, int line) const noexcept {
                // 获取当前时间
                const auto now = std::chrono::system_clock::now();
                const auto time_t = std::chrono::system_clock::to_time_t(now);
                const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now.time_since_epoch()) % 1000;
                
                // 格式化时间
                std::stringstream ss;
                ss << "[" << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S")
                   << "." << std::setfill('0') << std::setw(3) << ms.count() << "] ";
                
                // 输出日志级别
                ss << "[" << getLevelString(level) << "] ";
                
                // 输出源码位置信息（仅显示文件名，不显示完整路径）
                const std::string filename = extractFilename(file);
                ss << "[" << filename << ":" << line << "] ";
                
                // 输出消息
                ss << message;
                
                return ss.str();
            }
            
            /**
             * @brief 写入日志到文件
             * @param message 格式化后的日志消息
             */
            void writeToFile(const std::string& message) {
                // 将消息添加到队列中
                {
                    std::lock_guard<std::mutex> lock(queue_mutex_);
                    log_queue_.push(message);
                }
                // 通知写入线程
                queue_cv_.notify_one();
            }
            
            /**
             * @brief 文件写入线程函数
             */
            void fileWriterThread() {
                // 线程运行标志
                while (writer_running_.load(std::memory_order_relaxed)) {
                    std::unique_lock<std::mutex> lock(queue_mutex_);
                    
                    // 等待条件变量或超时
                    queue_cv_.wait_for(lock, std::chrono::milliseconds(100), [this] {
                        return !log_queue_.empty() || !writer_running_.load(std::memory_order_relaxed);
                    });
                    
                    // 检查是否需要退出
                    if (!writer_running_.load(std::memory_order_relaxed)) {
                        break;
                    }
                    
                    // 处理队列中的所有消息
                    std::string buffer;
                    size_t bufferSize = 0;
                    const size_t maxBufferSize = buffer_size_.load(std::memory_order_relaxed);
                    
                    while (!log_queue_.empty() && bufferSize < maxBufferSize) {
                        const std::string& msg = log_queue_.front();
                        buffer += msg + "\n";
                        bufferSize += msg.length() + 1;
                        log_queue_.pop();
                    }
                    
                    // 释放锁以便其他线程可以继续添加消息
                    lock.unlock();
                    
                    // 如果有数据要写入
                    if (!buffer.empty()) {
                        std::lock_guard<std::mutex> fileLock(file_mutex_);
                        if (file_stream_.is_open()) {
                            file_stream_ << buffer;
                            file_stream_.flush();  // 实时保存
                        }
                    }
                }
            }
            
            /**
             * @brief 获取日志级别字符串
             * @param level 日志级别
             * @return 日志级别字符串
             */
            constexpr const char* getLevelString(LogLevel level) const noexcept {
                switch (level) {
                    case LogLevel::LOG_TRACE: return "TRACE";
                    case LogLevel::LOG_DEBUG: return "DEBUG";
                    case LogLevel::LOG_INFO:  return "INFO";
                    case LogLevel::LOG_WARN:  return "WARN";
                    case LogLevel::LOG_ERROR: return "ERROR";
                    case LogLevel::LOG_FATAL: return "FATAL";
                    default: return "UNKNOWN";
                }
            }
            
            /**
             * @brief 从完整路径中提取文件名
             * @param path 完整文件路径
             * @return 文件名
             */
            std::string extractFilename(const char* path) const noexcept {
                std::string pathStr(path);
                size_t pos = pathStr.find_last_of("/\\");
                return (pos != std::string::npos) ? pathStr.substr(pos + 1) : pathStr;
            }
            
            std::atomic<int> current_level_{2};  // LOG_INFO = 2
            mutable std::mutex output_mutex_;
            
            // 文件输出相关
            std::atomic<bool> file_output_enabled_{false};
            std::string log_filename_;
            mutable std::mutex file_mutex_;
            std::ofstream file_stream_;
            
            // 异步写入相关
            std::queue<std::string> log_queue_;
            mutable std::mutex queue_mutex_;
            std::condition_variable queue_cv_;
            std::thread file_writer_thread_;
            std::atomic<bool> writer_running_{false};
            std::atomic<size_t> buffer_size_{1024};  // 默认缓冲区大小1KB
            
            // 重复日志过滤相关
            std::atomic<int> duplicate_filter_window_ms_; // 重复日志过滤时间窗口（毫秒）
            std::unordered_map<std::string, long long> last_message_times_; // 记录每个消息最后出现的时间
            mutable std::mutex duplicate_mutex_;
        };

        
        /**
         * @brief 日志管理器类
         * @details 提供全局日志配置和管理功能
         */
        class LogManager {
        public:
            /**
             * @brief 获取LogManager单例实例
             * @return LogManager引用
             */
            static LogManager& getInstance() noexcept {
                static LogManager instance;
                return instance;
            }
            
            /**
             * @brief 获取全局Logger实例
             * @return Logger引用
             */
            Logger& getLogger() const noexcept {
                return Logger::getInstance();
            }
            
            /**
             * @brief 设置全局日志级别
             * @param level 日志级别
             */
            void setGlobalLevel(LogLevel level) noexcept {
                getLogger().setLevel(level);
            }
            
            /**
             * @brief 获取全局日志级别
             * @return 当前日志级别
             */
            LogLevel getGlobalLevel() const noexcept {
                return getLogger().getLevel();
            }
            
            /**
             * @brief 设置重复日志过滤时间窗口
             * @param windowMs 时间窗口（毫秒）
             */
            void setDuplicateFilterWindow(int windowMs) noexcept {
                getLogger().setDuplicateFilterWindow(windowMs);
            }
            
        private:
            LogManager() = default;
            ~LogManager() = default;
            LogManager(const LogManager&) = delete;
            LogManager& operator=(const LogManager&) = delete;
            LogManager(LogManager&&) = delete;
            LogManager& operator=(LogManager&&) = delete;
        };
        
        /**
         * @brief 获取全局Logger实例的便捷函数
         * @return Logger引用
         */
        inline Logger& getLogger() noexcept {
            return LogManager::getInstance().getLogger();
        }
        
    } // namespace Utils
} // namespace DearTs

// 现代C++日志便捷函数，替代宏定义
namespace DearTs {
    namespace Log {
        /**
         * @brief TRACE级别日志函数
         * @param message 日志消息
         */
        inline void trace(const std::string& message) {
            Utils::getLogger().trace(message);
        }
        
        /**
         * @brief DEBUG级别日志函数
         * @param message 日志消息
         */
        inline void debug(const std::string& message) {
            Utils::getLogger().debug(message);
        }
        
        /**
         * @brief INFO级别日志函数
         * @param message 日志消息
         */
        inline void info(const std::string& message) {
            Utils::getLogger().info(message);
        }
        
        /**
         * @brief WARN级别日志函数
         * @param message 日志消息
         */
        inline void warn(const std::string& message) {
            Utils::getLogger().warn(message);
        }
        
        /**
         * @brief ERROR级别日志函数
         * @param message 日志消息
         */
        inline void error(const std::string& message) {
            Utils::getLogger().error(message);
        }
        
        /**
         * @brief FATAL级别日志函数
         * @param message 日志消息
         */
        inline void fatal(const std::string& message) {
            Utils::getLogger().fatal(message);
        }
        
        /**
         * @brief 设置日志级别的便捷函数
         * @param level 日志级别
         */
        inline void setLevel(Utils::LogLevel level) noexcept {
            Utils::LogManager::getInstance().setGlobalLevel(level);
        }
        
        /**
         * @brief 获取当前日志级别的便捷函数
         * @return 当前日志级别
         */
        inline Utils::LogLevel getLevel() noexcept {
            return Utils::LogManager::getInstance().getGlobalLevel();
        }
        
        /**
         * @brief 设置重复日志过滤时间窗口
         * @param windowMs 时间窗口（毫秒）
         */
        inline void setDuplicateFilterWindow(int windowMs) noexcept {
            Utils::LogManager::getInstance().setDuplicateFilterWindow(windowMs);
        }
    }
} // namespace DearTs::Log

// 为了向后兼容，保留少量必要的宏定义
#define DEARTS_LOGGER() ::DearTs::Utils::getLogger()
#define DEARTS_LOG_TRACE(msg) ::DearTs::Utils::getLogger().trace(msg, __FILE__, __LINE__)
#define DEARTS_LOG_DEBUG(msg) ::DearTs::Utils::getLogger().debug(msg, __FILE__, __LINE__)
#define DEARTS_LOG_INFO(msg)  ::DearTs::Utils::getLogger().info(msg, __FILE__, __LINE__)
#define DEARTS_LOG_WARN(msg)  ::DearTs::Utils::getLogger().warn(msg, __FILE__, __LINE__)
#define DEARTS_LOG_ERROR(msg) ::DearTs::Utils::getLogger().error(msg, __FILE__, __LINE__)
#define DEARTS_LOG_FATAL(msg) ::DearTs::Utils::getLogger().fatal(msg, __FILE__, __LINE__)