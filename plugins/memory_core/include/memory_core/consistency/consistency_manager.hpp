/**
 * @file consistency_manager.hpp
 * @brief 一致性管理器 - 统一管理离线队列、重试策略和同步协议
 *
 * 功能：
 * - 离线/在线状态管理
 * - 自动重试失败操作
 * - 自动同步
 * - 冲突解决
 */

#pragma once

#include "core/result.h"
#include "memory_core/consistency/offline_queue.hpp"
#include "memory_core/consistency/retry_policy.hpp"
#include "memory_core/consistency/sync_protocol.hpp"
#include <string>
#include <functional>
#include <memory>
#include <optional>
#include <cstdint>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace DearTs::Plugins::MemoryCore::Consistency {

/**
 * @brief 连接状态
 */
enum class ConnectionStatus {
    Online,                ///< 在线
    Offline,               ///< 离线
    Connecting,            ///< 连接中
    Syncing                ///< 同步中
};

/**
 * @brief 一致性管理器配置
 */
struct ConsistencyManagerConfig {
    bool enable_offline_queue;                  ///< 启用离线队列
    bool enable_auto_sync;                      ///< 启用自动同步
    bool enable_auto_retry;                     ///< 启用自动重试
    int sync_interval_seconds;                  ///< 同步间隔（秒）
    int retry_interval_seconds;                 ///< 重试间隔（秒）
    std::string node_id;                        ///< 节点 ID

    /**
     * @brief 默认配置
     */
    static ConsistencyManagerConfig default_config() {
        return ConsistencyManagerConfig{
            .enable_offline_queue = true,
            .enable_auto_sync = false,
            .enable_auto_retry = true,
            .sync_interval_seconds = 300,  // 5 分钟
            .retry_interval_seconds = 30,  // 30 秒
            .node_id = ""
        };
    }
};

/**
 * @brief 一致性管理器统计
 */
struct ConsistencyManagerStats {
    ConnectionStatus connection_status;         ///< 连接状态
    int pending_operations;                     ///< 待处理操作数
    int failed_operations;                      ///< 失败操作数
    int total_syncs;                            ///< 总同步次数
    int successful_syncs;                       ///< 成功同步次数
    int failed_syncs;                           ///< 失败同步次数
    int64_t last_sync_time;                     ///< 最后同步时间
    double sync_success_rate;                   ///< 同步成功率

    /**
     * @brief 创建空统计
     */
    static ConsistencyManagerStats empty() {
        return ConsistencyManagerStats{
            .connection_status = ConnectionStatus::Offline,
            .pending_operations = 0,
            .failed_operations = 0,
            .total_syncs = 0,
            .successful_syncs = 0,
            .failed_syncs = 0,
            .last_sync_time = 0,
            .sync_success_rate = 0.0
        };
    }
};

/**
 * @brief 连接状态变化回调
 *
 * @param old_status 旧状态
 * @param new_status 新状态
 */
using ConnectionStatusCallback = std::function<void(
    ConnectionStatus old_status,
    ConnectionStatus new_status
)>;

/**
 * @brief 一致性管理器
 *
 * 统一管理离线队列、重试策略和同步协议
 */
class ConsistencyManager {
public:
    /**
     * @brief 获取单例实例
     */
    static ConsistencyManager& instance();

    /**
     * @brief 删除拷贝构造和赋值
     */
    ConsistencyManager(const ConsistencyManager&) = delete;
    ConsistencyManager& operator=(const ConsistencyManager&) = delete;

    // ============ 初始化 ============

    /**
     * @brief 初始化一致性管理器
     * @param config 管理器配置
     * @return 成功或错误信息
     */
    DearTs::Core::Result<void, std::string> initialize(
        const ConsistencyManagerConfig& config = ConsistencyManagerConfig::default_config()
    );

    /**
     * @brief 关闭一致性管理器
     */
    void shutdown();

    // ============ 连接状态管理 ============

    /**
     * @brief 设置为在线状态
     */
    void set_online();

    /**
     * @brief 设置为离线状态
     */
    void set_offline();

    /**
     * @brief 获取连接状态
     * @return 当前状态
     */
    ConnectionStatus get_connection_status() const;

    /**
     * @brief 是否在线
     * @return true 表示在线
     */
    bool is_online() const;

    /**
     * @brief 设置连接状态回调
     * @param callback 回调函数
     */
    void set_connection_status_callback(ConnectionStatusCallback callback);

    // ============ 操作执行 ============

    /**
     * @brief 执行操作（带离线队列和重试）
     * @param operation 操作函数
     * @param operation_type 操作类型
     * @param payload 操作数据（JSON）
     * @return 操作结果或错误信息
     */
    DearTs::Core::Result<void, std::string> execute_operation(
        std::function<DearTs::Core::Result<void, std::string>()> operation,
        OfflineOperationType operation_type,
        const std::string& payload = ""
    );

    /**
     * @brief 处理离线队列（在恢复在线时调用）
     * @return 处理的操作数量或错误信息
     */
    DearTs::Core::Result<int, std::string> process_offline_queue();

    // ============ 同步 ============

    /**
     * @brief 手动触发同步
     * @return 同步结果或错误信息
     */
    DearTs::Core::Result<SyncResult, std::string> sync_now();

    /**
     * @brief 获取最后同步结果
     * @return 最后的同步结果
     */
    std::optional<SyncResult> get_last_sync_result() const;

    // ============ 统计 ============

    /**
     * @brief 获取统计信息
     * @return 统计数据
     */
    ConsistencyManagerStats get_stats() const;

    /**
     * @brief 打印统计信息
     */
    void log_stats() const;

    // ============ 配置访问 ============

    /**
     * @brief 获取离线队列
     * @return 离线队列引用
     */
    OfflineQueue& get_offline_queue() {
        return OfflineQueue::instance();
    }

    /**
     * @brief 获取重试策略
     * @return 重试策略引用
     */
    RetryPolicy& get_retry_policy() {
        return RetryPolicy::instance();
    }

    /**
     * @brief 获取同步协议
     * @return 同步协议引用
     */
    SyncProtocol& get_sync_protocol() {
        return SyncProtocol::instance();
    }

    /**
     * @brief 获取配置
     * @return 当前配置
     */
    const ConsistencyManagerConfig& get_config() const {
        return m_config;
    }

private:
    /**
     * @brief 私有构造函数（单例模式）
     */
    ConsistencyManager() = default;

    /**
     * @brief 析构函数
     */
    ~ConsistencyManager() = default;

    /**
     * @brief 后台工作线程
     */
    void worker_thread();

    /**
     * @brief 执行同步
     */
    void perform_sync();

    /**
     * @brief 执行重试
     */
    void perform_retry();

    /**
     * @brief 更新统计信息
     */
    void update_stats();

    /**
     * @brief 通知连接状态变化
     */
    void notify_connection_status_change(ConnectionStatus new_status);

    // ============ 成员变量 ============

    ConsistencyManagerConfig m_config;                        ///< 配置
    ConnectionStatus m_connection_status;                    ///< 连接状态
    ConnectionStatusCallback m_connection_status_callback;   ///< 连接状态回调

    std::optional<SyncResult> m_last_sync_result;            ///< 最后同步结果

    // 后台线程
    std::atomic<bool> m_running;                             ///< 运行标志
    std::thread m_worker_thread;                             ///< 工作线程
    std::mutex m_mutex;                                      ///< 互斥锁
    std::condition_variable m_cv;                            ///< 条件变量

    mutable std::mutex m_stats_mutex;                        ///< 统计互斥锁
    ConsistencyManagerStats m_stats;                         ///< 统计信息

    bool m_initialized;                                      ///< 是否已初始化
};

} // namespace DearTs::Plugins::MemoryCore::Consistency
