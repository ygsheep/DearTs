/**
 * @file offline_queue.hpp
 * @brief 离线队列 - 处理离线期间的操作
 *
 * 功能：
 * - 离线操作持久化
 * - 操作重放
 * - 队列持久化到数据库
 * - 支持多种操作类型
 */

#pragma once

#include "core/result.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <optional>
#include <cstdint>

namespace DearTs::Plugins::MemoryCore::Consistency {

/**
 * @brief 离线操作类型
 */
enum class OfflineOperationType {
    SaveMessage,           ///< 保存消息
    ExtractMemory,         ///< 提取记忆
    RAGQuery,              ///< RAG 查询
    SaveMemory,            ///< 保存记忆
    UpdateMemory,          ///< 更新记忆
    DeleteMemory,          ///< 删除记忆
    GenerateSummary        ///< 生成摘要
};

/**
 * @brief 离线操作状态
 */
enum class OfflineOperationStatus {
    Pending,               ///< 等待执行
    InProgress,            ///< 执行中
    Completed,             ///< 已完成
    Failed,                ///< 失败（将重试）
    PermanentlyFailed      /// 永久失败（不再重试）
};

/**
 * @brief 离线操作
 */
struct OfflineOperation {
    int64_t id;                                    ///< 操作 ID
    OfflineOperationType type;                     ///< 操作类型
    OfflineOperationStatus status;                 ///< 操作状态
    std::string payload;                           ///< 操作数据（JSON 序列化）
    std::string correlation_id;                    ///< 关联 ID（用于追踪）
    int retry_count;                               ///< 已重试次数
    int64_t created_at;                            ///< 创建时间
    std::optional<int64_t> completed_at;           ///< 完成时间
    std::optional<std::string> error_message;      ///< 错误信息

    /**
     * @brief 序列化为 JSON
     */
    std::string to_json() const;

    /**
     * @brief 从 JSON 反序列化
     */
    static DearTs::Core::Result<OfflineOperation, std::string> from_json(
        const std::string& json
    );
};

/**
 * @brief 离线队列配置
 */
struct OfflineQueueConfig {
    int max_queue_size;                ///< 最大队列大小
    int max_retry_count;               ///< 最大重试次数
    bool persist_to_disk;              ///< 是否持久化到磁盘
    std::string storage_path;          ///< 存储路径

    /**
     * @brief 默认配置
     */
    static OfflineQueueConfig default_config() {
        return OfflineQueueConfig{
            .max_queue_size = 1000,
            .max_retry_count = 3,
            .persist_to_disk = true,
            .storage_path = "offline_queue.db"
        };
    }
};

/**
 * @brief 离线队列统计
 */
struct OfflineQueueStats {
    int total_operations;              ///< 总操作数
    int pending_count;                 ///< 待处理数
    int in_progress_count;             ///< 执行中数
    int completed_count;               ///< 已完成数
    int failed_count;                  ///< 失败数
    int permanently_failed_count;      ///< 永久失败数
    double success_rate;               ///< 成功率

    /**
     * @brief 创建空统计
     */
    static OfflineQueueStats empty() {
        return OfflineQueueStats{
            .total_operations = 0,
            .pending_count = 0,
            .in_progress_count = 0,
            .completed_count = 0,
            .failed_count = 0,
            .permanently_failed_count = 0,
            .success_rate = 0.0
        };
    }
};

/**
 * @brief 离线队列 - 管理离线操作
 *
 * 负责在网络不可用时缓存操作，并在网络恢复后重放
 */
class OfflineQueue {
public:
    /**
     * @brief 获取单例实例
     */
    static OfflineQueue& instance();

    /**
     * @brief 删除拷贝构造和赋值
     */
    OfflineQueue(const OfflineQueue&) = delete;
    OfflineQueue& operator=(const OfflineQueue&) = delete;

    // ============ 初始化 ============

    /**
     * @brief 初始化离线队列
     * @param config 队列配置
     * @return 成功或错误信息
     */
    DearTs::Core::Result<void, std::string> initialize(
        const OfflineQueueConfig& config = OfflineQueueConfig::default_config()
    );

    /**
     * @brief 关闭队列
     */
    void shutdown();

    // ============ 操作管理 ============

    /**
     * @brief 添加操作到队列
     * @param operation 操作对象
     * @return 操作 ID 或错误信息
     */
    DearTs::Core::Result<int64_t, std::string> enqueue(
        const OfflineOperation& operation
    );

    /**
     * @brief 添加操作（便捷方法）
     * @param type 操作类型
     * @param payload 操作数据（JSON）
     * @param correlation_id 关联 ID
     * @return 操作 ID 或错误信息
     */
    DearTs::Core::Result<int64_t, std::string> enqueue(
        OfflineOperationType type,
        const std::string& payload,
        const std::string& correlation_id = ""
    );

    /**
     * @brief 获取下一个待处理操作
     * @return 操作对象，如果队列为空则返回 std::nullopt
     */
    std::optional<OfflineOperation> dequeue();

    /**
     * @brief 标记操作为完成
     * @param operation_id 操作 ID
     * @return 成功或错误信息
     */
    DearTs::Core::Result<void, std::string> mark_completed(int64_t operation_id);

    /**
     * @brief 标记操作为失败
     * @param operation_id 操作 ID
     * @param error_message 错误信息
     * @return 成功或错误信息
     */
    DearTs::Core::Result<void, std::string> mark_failed(
        int64_t operation_id,
        const std::string& error_message
    );

    /**
     * @brief 重试失败的操作
     * @return 重试的操作数量
     */
    DearTs::Core::Result<int, std::string> retry_failed();

    // ============ 查询 ============

    /**
     * @brief 获取操作
     * @param operation_id 操作 ID
     * @return 操作对象或错误信息
     */
    DearTs::Core::Result<OfflineOperation, std::string> get_operation(
        int64_t operation_id
    ) const;

    /**
     * @brief 获取所有待处理操作
     * @return 操作列表
     */
    std::vector<OfflineOperation> get_pending_operations() const;

    /**
     * @brief 获取失败操作
     * @return 操作列表
     */
    std::vector<OfflineOperation> get_failed_operations() const;

    /**
     * @brief 按关联 ID 获取操作
     * @param correlation_id 关联 ID
     * @return 操作列表
     */
    std::vector<OfflineOperation> get_operations_by_correlation(
        const std::string& correlation_id
    ) const;

    // ============ 维护 ============

    /**
     * @brief 清理已完成的操作
     * @param older_than_ms 清理早于此时间的操作（毫秒）
     * @return 清理的操作数量
     */
    DearTs::Core::Result<int, std::string> cleanup_completed(int64_t older_than_ms);

    /**
     * @brief 清理永久失败的操作
     * @return 清理的操作数量
     */
    DearTs::Core::Result<int, std::string> cleanup_permanently_failed();

    /**
     * @brief 获取队列统计
     * @return 统计信息
     */
    OfflineQueueStats get_stats() const;

    /**
     * @brief 打印统计信息到日志
     */
    void log_stats() const;

    /**
     * @brief 清空队列
     * @return 成功或错误信息
     */
    DearTs::Core::Result<void, std::string> clear();

    // ============ 持久化 ============

    /**
     * @brief 保存队列到磁盘
     * @return 成功或错误信息
     */
    DearTs::Core::Result<void, std::string> save_to_disk();

    /**
     * @brief 从磁盘加载队列
     * @return 成功或错误信息
     */
    DearTs::Core::Result<void, std::string> load_from_disk();

private:
    /**
     * @brief 私有构造函数（单例模式）
     */
    OfflineQueue() = default;

    /**
     * @brief 析构函数
     */
    ~OfflineQueue() = default;

    /**
     * @brief 生成唯一操作 ID
     */
    int64_t generate_operation_id();

    /**
     * @brief 检查是否应该永久失败
     */
    bool should_permanently_fail(const OfflineOperation& operation) const;

    /**
     * @brief 更新统计信息
     */
    void update_stats();

    // ============ 成员变量 ============

    OfflineQueueConfig m_config;                              ///< 配置
    std::vector<OfflineOperation> m_operations;                ///< 操作列表
    int64_t m_next_operation_id;                               ///< 下一个操作 ID
    bool m_initialized;                                        ///< 是否已初始化
    mutable OfflineQueueStats m_stats;                         ///< 统计信息
};

} // namespace DearTs::Plugins::MemoryCore::Consistency
