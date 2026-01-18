/**
 * @file consistency_manager.cpp
 * @brief 一致性管理器实现
 */

#include "memory_core/consistency/consistency_manager.hpp"
#include "liblogger/logger.h"
#include <chrono>
#include <thread>

namespace DearTs::Plugins::MemoryCore::Consistency {

// ============ 单例实现 ============

ConsistencyManager& ConsistencyManager::instance() {
    static ConsistencyManager instance;
    return instance;
}

// ============ 初始化 ============

DearTs::Core::Result<void, std::string> ConsistencyManager::initialize(
    const ConsistencyManagerConfig& config
) {
    if (m_initialized) {
        LOG_WARN("ConsistencyManager already initialized");
        return DearTs::Core::Result<void, std::string>::ok();
    }

    m_config = config;
    m_connection_status = ConnectionStatus::Offline;
    m_stats = ConsistencyManagerStats::empty();
    m_running = false;
    m_initialized = true;

    // 初始化离线队列
    if (config.enable_offline_queue) {
        OfflineQueueConfig queue_config = OfflineQueueConfig::default_config();
        if (!config.node_id.empty()) {
            queue_config.storage_path = "offline_queue_" + config.node_id + ".db";
        }

        auto queue_result = OfflineQueue::instance().initialize(queue_config);
        if (queue_result.isErr()) {
            return DearTs::Core::Result<void, std::string>::err(
                "Failed to initialize offline queue: " + queue_result.error()
            );
        }
    }

    // 初始化重试策略
    if (config.enable_auto_retry) {
        RetryPolicy::instance().initialize(RetryPolicyConfig::default_config());
    }

    // 初始化同步协议
    SyncConfig sync_config;
    sync_config.node_id = config.node_id.empty()
        ? "client_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count())
        : config.node_id;
    sync_config.auto_sync = config.enable_auto_sync;
    sync_config.sync_interval_seconds = config.sync_interval_seconds;

    auto sync_result = SyncProtocol::instance().initialize(sync_config);
    if (sync_result.isErr()) {
        return DearTs::Core::Result<void, std::string>::err(
            "Failed to initialize sync protocol: " + sync_result.error()
        );
    }

    // 启动后台工作线程
    if (config.enable_auto_sync || config.enable_auto_retry) {
        m_running = true;
        m_worker_thread = std::thread(&ConsistencyManager::worker_thread, this);
    }

    update_stats();
    LOG_INFO("ConsistencyManager initialized: offline_queue={}, auto_sync={}, auto_retry={}",
             config.enable_offline_queue, config.enable_auto_sync, config.enable_auto_retry);
    return DearTs::Core::Result<void, std::string>::ok();
}

void ConsistencyManager::shutdown() {
    if (!m_initialized) {
        return;
    }

    // 停止后台线程
    m_running = false;
    m_cv.notify_all();

    if (m_worker_thread.joinable()) {
        m_worker_thread.join();
    }

    // 关闭组件
    OfflineQueue::instance().shutdown();
    // m_sync_protocol.shutdown(); // 无 shutdown 方法

    log_stats();
    LOG_INFO("ConsistencyManager shutdown");
    m_initialized = false;
}

// ============ 连接状态管理 ============

void ConsistencyManager::set_online() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_connection_status == ConnectionStatus::Online) {
        return;
    }

    ConnectionStatus old_status = m_connection_status;
    m_connection_status = ConnectionStatus::Online;

    LOG_INFO("Connection status: Offline -> Online");

    // 处理离线队列
    if (m_config.enable_offline_queue) {
        LOG_INFO("Processing offline queue after coming online...");
        auto process_result = process_offline_queue();
        if (process_result.isErr()) {
            LOG_ERROR("Failed to process offline queue: {}", process_result.error());
        } else {
            LOG_INFO("Processed {} offline operations", process_result.unwrap());
        }
    }

    // 触发同步
    if (m_config.enable_auto_sync) {
        LOG_INFO("Triggering auto sync after coming online...");
        perform_sync();
    }

    update_stats();
    notify_connection_status_change(old_status);
}

void ConsistencyManager::set_offline() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_connection_status == ConnectionStatus::Offline) {
        return;
    }

    ConnectionStatus old_status = m_connection_status;
    m_connection_status = ConnectionStatus::Offline;

    LOG_INFO("Connection status: Online -> Offline");
    update_stats();
    notify_connection_status_change(old_status);
}

ConnectionStatus ConsistencyManager::get_connection_status() const {
    return m_connection_status;
}

bool ConsistencyManager::is_online() const {
    return m_connection_status == ConnectionStatus::Online ||
           m_connection_status == ConnectionStatus::Syncing;
}

void ConsistencyManager::set_connection_status_callback(ConnectionStatusCallback callback) {
    m_connection_status_callback = std::move(callback);
}

// ============ 操作执行 ============

DearTs::Core::Result<void, std::string> ConsistencyManager::execute_operation(
    std::function<DearTs::Core::Result<void, std::string>()> operation,
    OfflineOperationType operation_type,
    const std::string& payload
) {
    // 如果在线，直接执行
    if (is_online()) {
        if (m_config.enable_auto_retry) {
            // 使用重试策略执行
            return RetryPolicy::instance().execute_with_retry(operation);
        } else {
            // 直接执行
            return operation();
        }
    }

    // 如果离线，添加到离线队列
    if (m_config.enable_offline_queue) {
        LOG_INFO("Operation queued (offline): type={}", static_cast<int>(operation_type));

        auto enqueue_result = OfflineQueue::instance().enqueue(operation_type, payload);
        if (enqueue_result.isErr()) {
            return DearTs::Core::Result<void, std::string>::err(
                "Failed to enqueue operation: " + enqueue_result.error()
            );
        }

        return DearTs::Core::Result<void, std::string>::ok();
    }

    // 既不在线也不启用离线队列
    return DearTs::Core::Result<void, std::string>::err(
        "Operation failed: offline and offline queue disabled"
    );
}

DearTs::Core::Result<int, std::string> ConsistencyManager::process_offline_queue() {
    int processed = 0;
    int failed = 0;

    while (true) {
        // 获取下一个待处理操作
        auto operation = OfflineQueue::instance().dequeue();
        if (!operation.has_value()) {
            break;  // 队列为空
        }

        auto& op = operation.value();

        // TODO: 根据 operation.type 执行相应的操作
        // 当前占位符实现：标记为已完成

        auto mark_result = OfflineQueue::instance().mark_completed(op.id);
        if (mark_result.isErr()) {
            LOG_WARN("Failed to mark operation as completed: {}", mark_result.error());
        } else {
            processed++;
        }
    }

    LOG_INFO("Processed {} offline operations ({} failed)", processed, failed);
    return DearTs::Core::Result<int, std::string>::ok(processed);
}

// ============ 同步 ============

DearTs::Core::Result<SyncResult, std::string> ConsistencyManager::sync_now() {
    if (!is_online()) {
        return DearTs::Core::Result<SyncResult, std::string>::err(
            "Cannot sync: offline"
        );
    }

    m_connection_status = ConnectionStatus::Syncing;

    auto sync_result = SyncProtocol::instance().sync(
        [this](int current, int total, const std::string& message) {
            LOG_DEBUG("Sync progress: {}/{} - {}", current, total, message);
        }
    );

    m_connection_status = ConnectionStatus::Online;

    if (sync_result.isOk()) {
        m_last_sync_result = sync_result.unwrap();

        // 更新统计
        std::lock_guard<std::mutex> lock(m_stats_mutex);
        m_stats.total_syncs++;
        if (m_last_sync_result->status == SyncStatus::Completed) {
            m_stats.successful_syncs++;
        } else {
            m_stats.failed_syncs++;
        }
        m_stats.last_sync_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();

        double total = m_stats.total_syncs;
        m_stats.sync_success_rate = total > 0
            ? static_cast<double>(m_stats.successful_syncs) / total
            : 0.0;
    }

    return sync_result;
}

std::optional<SyncResult> ConsistencyManager::get_last_sync_result() const {
    return m_last_sync_result;
}

// ============ 统计 ============

ConsistencyManagerStats ConsistencyManager::get_stats() const {
    std::lock_guard<std::mutex> lock(m_stats_mutex);
    return m_stats;
}

void ConsistencyManager::log_stats() const {
    std::lock_guard<std::mutex> lock(m_stats_mutex);

    const char* status_str;
    switch (m_stats.connection_status) {
        case ConnectionStatus::Online:  status_str = "Online"; break;
        case ConnectionStatus::Offline: status_str = "Offline"; break;
        case ConnectionStatus::Connecting: status_str = "Connecting"; break;
        case ConnectionStatus::Syncing:  status_str = "Syncing"; break;
        default: status_str = "Unknown"; break;
    }

    LOG_INFO("ConsistencyManager Stats: status={}, pending_ops={}, failed_ops={}, "
             "syncs={}/{}, sync_rate={:.2f}%",
             status_str, m_stats.pending_operations, m_stats.failed_operations,
             m_stats.successful_syncs, m_stats.total_syncs,
             m_stats.sync_success_rate * 100);
}

// ============ 私有辅助方法 ============

void ConsistencyManager::worker_thread() {
    LOG_INFO("ConsistencyManager worker thread started");

    while (m_running) {
        // 等待指定间隔
        std::unique_lock<std::mutex> lock(m_mutex);
        int wait_seconds = std::min(m_config.sync_interval_seconds,
                                    m_config.retry_interval_seconds);

        if (m_cv.wait_for(lock, std::chrono::seconds(wait_seconds),
                         [this] { return !m_running; })) {
            break;  // 收到停止信号
        }

        // 执行重试
        if (m_config.enable_auto_retry && is_online()) {
            perform_retry();
        }

        // 执行同步
        if (m_config.enable_auto_sync && is_online()) {
            perform_sync();
        }
    }

    LOG_INFO("ConsistencyManager worker thread stopped");
}

void ConsistencyManager::perform_sync() {
    LOG_DEBUG("Performing scheduled sync...");

    auto sync_result = sync_now();
    if (sync_result.isErr()) {
        LOG_ERROR("Scheduled sync failed: {}", sync_result.error());
    }
}

void ConsistencyManager::perform_retry() {
    LOG_DEBUG("Performing scheduled retry...");

    auto retry_result = OfflineQueue::instance().retry_failed();
    if (retry_result.isOk()) {
        int retried = retry_result.unwrap();
        if (retried > 0) {
            LOG_INFO("Retried {} failed operations", retried);
        }
    }
}

void ConsistencyManager::update_stats() {
    std::lock_guard<std::mutex> lock(m_stats_mutex);

    m_stats.connection_status = m_connection_status;
    auto queue_stats = OfflineQueue::instance().get_stats();
    m_stats.pending_operations = queue_stats.pending_count;
    m_stats.failed_operations = queue_stats.failed_count +
                                queue_stats.permanently_failed_count;
}

void ConsistencyManager::notify_connection_status_change(ConnectionStatus new_status) {
    if (m_connection_status_callback) {
        m_connection_status_callback(m_connection_status, new_status);
    }
}

} // namespace DearTs::Plugins::MemoryCore::Consistency
