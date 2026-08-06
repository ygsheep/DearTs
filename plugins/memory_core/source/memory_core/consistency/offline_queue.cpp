/**
 * @file offline_queue.cpp
 * @brief 离线队列实现
 */

#include "memory_core/consistency/offline_queue.hpp"
#include "liblogger/logger.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <chrono>
#include <mutex>

// JSON 序列化辅助（简单实现，实际应使用 nlohmann/json）
namespace {

std::string escape_json(const std::string& str) {
    std::string result;
    for (char c : str) {
        switch (c) {
            case '"':  result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default:   result += c; break;
        }
    }
    return result;
}

std::string unescape_json(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '\\' && i + 1 < str.length()) {
            switch (str[i + 1]) {
                case '"':  result += '"';  ++i; break;
                case '\\': result += '\\'; ++i; break;
                case 'n':  result += '\n'; ++i; break;
                case 'r':  result += '\r'; ++i; break;
                case 't':  result += '\t'; ++i; break;
                default:   result += str[i]; break;
            }
        } else {
            result += str[i];
        }
    }
    return result;
}

} // anonymous namespace

namespace DearTs::Plugins::MemoryCore::Consistency {

// ============ 单例实现 ============

OfflineQueue& OfflineQueue::instance() {
    static OfflineQueue instance;
    return instance;
}

// ============ 初始化 ============

DearTs::Core::Result<void, std::string> OfflineQueue::initialize(
    const OfflineQueueConfig& config
) {
    if (m_initialized) {
        LOG_WARN("OfflineQueue already initialized");
        return DearTs::Core::Result<void, std::string>::ok();
    }

    m_config = config;
    m_operations.clear();
    m_next_operation_id = 1;
    m_initialized = true;

    // 从磁盘加载（如果启用持久化）
    if (config.persist_to_disk) {
        auto load_result = load_from_disk();
        if (load_result.isErr()) {
            LOG_WARN("Failed to load offline queue from disk: {}", load_result.error());
            // 不失败，继续使用空队列
        }
    }

    update_stats();
    LOG_INFO("OfflineQueue initialized: {} operations", m_operations.size());
    return DearTs::Core::Result<void, std::string>::ok();
}

void OfflineQueue::shutdown() {
    if (!m_initialized) {
        return;
    }

    // 保存到磁盘（如果启用持久化）
    if (m_config.persist_to_disk) {
        auto save_result = save_to_disk();
        if (save_result.isErr()) {
            LOG_ERROR("Failed to save offline queue: {}", save_result.error());
        }
    }

    LOG_INFO("OfflineQueue shutdown: {} operations", m_operations.size());
    log_stats();
    m_initialized = false;
}

// ============ 操作管理 ============

DearTs::Core::Result<int64_t, std::string> OfflineQueue::enqueue(
    const OfflineOperation& operation
) {
    if (!m_initialized) {
        return DearTs::Core::Result<int64_t, std::string>::err(
            "OfflineQueue not initialized"
        );
    }

    // 检查队列大小限制
    if (static_cast<int>(m_operations.size()) >= m_config.max_queue_size) {
        return DearTs::Core::Result<int64_t, std::string>::err(
            "Offline queue is full"
        );
    }

    // 创建操作副本并分配 ID
    OfflineOperation op = operation;
    op.id = generate_operation_id();
    op.status = OfflineOperationStatus::Pending;
    op.retry_count = 0;
    op.created_at = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    m_operations.push_back(op);
    update_stats();

    LOG_DEBUG("Enqueued operation: type={}, id={}, correlation_id={}",
              static_cast<int>(op.type), op.id, op.correlation_id);

    return DearTs::Core::Result<int64_t, std::string>::ok(op.id);
}

DearTs::Core::Result<int64_t, std::string> OfflineQueue::enqueue(
    OfflineOperationType type,
    const std::string& payload,
    const std::string& correlation_id
) {
    OfflineOperation operation;
    operation.type = type;
    operation.payload = payload;
    operation.correlation_id = correlation_id;
    operation.status = OfflineOperationStatus::Pending;

    return enqueue(operation);
}

std::optional<OfflineOperation> OfflineQueue::dequeue() {
    if (!m_initialized) {
        return std::nullopt;
    }

    // 查找第一个待处理操作
    auto it = std::find_if(m_operations.begin(), m_operations.end(),
        [](const OfflineOperation& op) {
            return op.status == OfflineOperationStatus::Pending;
        });

    if (it == m_operations.end()) {
        return std::nullopt;
    }

    // 标记为执行中
    it->status = OfflineOperationStatus::InProgress;
    update_stats();

    LOG_DEBUG("Dequeued operation: id={}, type={}",
              it->id, static_cast<int>(it->type));

    return *it;
}

DearTs::Core::Result<void, std::string> OfflineQueue::mark_completed(int64_t operation_id) {
    auto it = std::find_if(m_operations.begin(), m_operations.end(),
        [operation_id](const OfflineOperation& op) {
            return op.id == operation_id;
        });

    if (it == m_operations.end()) {
        return DearTs::Core::Result<void, std::string>::err(
            "Operation not found: " + std::to_string(operation_id)
        );
    }

    it->status = OfflineOperationStatus::Completed;
    it->completed_at = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    it->error_message = std::nullopt;

    update_stats();

    LOG_DEBUG("Marked operation as completed: id={}", operation_id);
    return DearTs::Core::Result<void, std::string>::ok();
}

DearTs::Core::Result<void, std::string> OfflineQueue::mark_failed(
    int64_t operation_id,
    const std::string& error_message
) {
    auto it = std::find_if(m_operations.begin(), m_operations.end(),
        [operation_id](const OfflineOperation& op) {
            return op.id == operation_id;
        });

    if (it == m_operations.end()) {
        return DearTs::Core::Result<void, std::string>::err(
            "Operation not found: " + std::to_string(operation_id)
        );
    }

    it->retry_count++;
    it->error_message = error_message;

    // 检查是否应该永久失败
    if (should_permanently_fail(*it)) {
        it->status = OfflineOperationStatus::PermanentlyFailed;
        LOG_WARN("Operation permanently failed: id={}, retries={}, error={}",
                 operation_id, it->retry_count, error_message);
    } else {
        it->status = OfflineOperationStatus::Pending;
        LOG_WARN("Operation failed, will retry: id={}, retries={}/{}",
                 operation_id, it->retry_count, m_config.max_retry_count);
    }

    update_stats();
    return DearTs::Core::Result<void, std::string>::ok();
}

DearTs::Core::Result<int, std::string> OfflineQueue::retry_failed() {
    int retry_count = 0;

    for (auto& op : m_operations) {
        if (op.status == OfflineOperationStatus::Failed) {
            op.status = OfflineOperationStatus::Pending;
            retry_count++;
        }
    }

    if (retry_count > 0) {
        update_stats();
        LOG_INFO("Retrying {} failed operations", retry_count);
    }

    return DearTs::Core::Result<int, std::string>::ok(retry_count);
}

// ============ 查询 ============

DearTs::Core::Result<OfflineOperation, std::string> OfflineQueue::get_operation(
    int64_t operation_id
) const {
    auto it = std::find_if(m_operations.begin(), m_operations.end(),
        [operation_id](const OfflineOperation& op) {
            return op.id == operation_id;
        });

    if (it == m_operations.end()) {
        return DearTs::Core::Result<OfflineOperation, std::string>::err(
            "Operation not found: " + std::to_string(operation_id)
        );
    }

    return DearTs::Core::Result<OfflineOperation, std::string>::ok(*it);
}

std::vector<OfflineOperation> OfflineQueue::get_pending_operations() const {
    std::vector<OfflineOperation> pending;
    for (const auto& op : m_operations) {
        if (op.status == OfflineOperationStatus::Pending) {
            pending.push_back(op);
        }
    }
    return pending;
}

std::vector<OfflineOperation> OfflineQueue::get_failed_operations() const {
    std::vector<OfflineOperation> failed;
    for (const auto& op : m_operations) {
        if (op.status == OfflineOperationStatus::Failed ||
            op.status == OfflineOperationStatus::PermanentlyFailed) {
            failed.push_back(op);
        }
    }
    return failed;
}

std::vector<OfflineOperation> OfflineQueue::get_operations_by_correlation(
    const std::string& correlation_id
) const {
    std::vector<OfflineOperation> result;
    for (const auto& op : m_operations) {
        if (op.correlation_id == correlation_id) {
            result.push_back(op);
        }
    }
    return result;
}

// ============ 维护 ============

DearTs::Core::Result<int, std::string> OfflineQueue::cleanup_completed(int64_t older_than_ms) {
    int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    int cleaned = 0;
    auto it = m_operations.begin();
    while (it != m_operations.end()) {
        if (it->status == OfflineOperationStatus::Completed &&
            it->completed_at.has_value() &&
            (now - it->completed_at.value()) > older_than_ms) {
            it = m_operations.erase(it);
            cleaned++;
        } else {
            ++it;
        }
    }

    if (cleaned > 0) {
        update_stats();
        LOG_INFO("Cleaned {} completed operations", cleaned);
    }

    return DearTs::Core::Result<int, std::string>::ok(cleaned);
}

DearTs::Core::Result<int, std::string> OfflineQueue::cleanup_permanently_failed() {
    int cleaned = 0;
    auto it = m_operations.begin();
    while (it != m_operations.end()) {
        if (it->status == OfflineOperationStatus::PermanentlyFailed) {
            it = m_operations.erase(it);
            cleaned++;
        } else {
            ++it;
        }
    }

    if (cleaned > 0) {
        update_stats();
        LOG_INFO("Cleaned {} permanently failed operations", cleaned);
    }

    return DearTs::Core::Result<int, std::string>::ok(cleaned);
}

OfflineQueueStats OfflineQueue::get_stats() const {
    return m_stats;
}

DearTs::Core::Result<void, std::string> OfflineQueue::clear() {
    int count = static_cast<int>(m_operations.size());
    m_operations.clear();
    m_next_operation_id = 1;
    update_stats();

    LOG_INFO("Cleared offline queue: {} operations removed", count);
    return DearTs::Core::Result<void, std::string>::ok();
}

// ============ 持久化 ============

DearTs::Core::Result<void, std::string> OfflineQueue::save_to_disk() {
    if (!m_config.persist_to_disk) {
        return DearTs::Core::Result<void, std::string>::ok();
    }

    std::ofstream file(m_config.storage_path);
    if (!file.is_open()) {
        return DearTs::Core::Result<void, std::string>::err(
            "Failed to open file for writing: " + m_config.storage_path
        );
    }

    // 写入每个操作的 JSON
    for (const auto& op : m_operations) {
        file << op.to_json() << "\n";
    }

    file.close();
    LOG_DEBUG("Saved {} operations to disk", m_operations.size());
    return DearTs::Core::Result<void, std::string>::ok();
}

DearTs::Core::Result<void, std::string> OfflineQueue::load_from_disk() {
    if (!m_config.persist_to_disk) {
        return DearTs::Core::Result<void, std::string>::ok();
    }

    std::ifstream file(m_config.storage_path);
    if (!file.is_open()) {
        LOG_WARN("No existing offline queue file found");
        return DearTs::Core::Result<void, std::string>::ok();
    }

    m_operations.clear();
    std::string line;
    int loaded = 0;

    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }

        auto result = OfflineOperation::from_json(line);
        if (result.isOk()) {
            m_operations.push_back(result.unwrap());
            loaded++;
        } else {
            LOG_WARN("Failed to parse offline operation: {}", result.error());
        }
    }

    file.close();

    // 更新下一个 ID
    if (!m_operations.empty()) {
        auto max_it = std::max_element(m_operations.begin(), m_operations.end(),
            [](const OfflineOperation& a, const OfflineOperation& b) {
                return a.id < b.id;
            });
        m_next_operation_id = max_it->id + 1;
    }

    update_stats();
    LOG_INFO("Loaded {} operations from disk", loaded);
    return DearTs::Core::Result<void, std::string>::ok();
}

// ============ 私有辅助方法 ============

int64_t OfflineQueue::generate_operation_id() {
    return m_next_operation_id++;
}

bool OfflineQueue::should_permanently_fail(const OfflineOperation& operation) const {
    return operation.retry_count >= m_config.max_retry_count;
}

void OfflineQueue::update_stats() {
    m_stats.total_operations = static_cast<int>(m_operations.size());
    m_stats.pending_count = 0;
    m_stats.in_progress_count = 0;
    m_stats.completed_count = 0;
    m_stats.failed_count = 0;
    m_stats.permanently_failed_count = 0;

    for (const auto& op : m_operations) {
        switch (op.status) {
            case OfflineOperationStatus::Pending:
                m_stats.pending_count++;
                break;
            case OfflineOperationStatus::InProgress:
                m_stats.in_progress_count++;
                break;
            case OfflineOperationStatus::Completed:
                m_stats.completed_count++;
                break;
            case OfflineOperationStatus::Failed:
                m_stats.failed_count++;
                break;
            case OfflineOperationStatus::PermanentlyFailed:
                m_stats.permanently_failed_count++;
                break;
        }
    }

    int total_finished = m_stats.completed_count + m_stats.failed_count +
                        m_stats.permanently_failed_count;
    m_stats.success_rate = total_finished > 0
        ? static_cast<double>(m_stats.completed_count) / total_finished
        : 0.0;
}

void OfflineQueue::log_stats() const {
    LOG_INFO("OfflineQueue Stats: total={}, pending={}, in_progress={}, "
             "completed={}, failed={}, permanently_failed={}, success_rate={:.2f}%",
             m_stats.total_operations, m_stats.pending_count,
             m_stats.in_progress_count, m_stats.completed_count,
             m_stats.failed_count, m_stats.permanently_failed_count,
             m_stats.success_rate * 100);
}

// ============ OfflineOperation 序列化 ============

std::string OfflineOperation::to_json() const {
    std::ostringstream oss;
    oss << "{"
        << "\"id\":" << id << ","
        << "\"type\":" << static_cast<int>(type) << ","
        << "\"status\":" << static_cast<int>(status) << ","
        << "\"payload\":\"" << escape_json(payload) << "\","
        << "\"correlation_id\":\"" << escape_json(correlation_id) << "\","
        << "\"retry_count\":" << retry_count << ","
        << "\"created_at\":" << created_at << ",";

    if (completed_at.has_value()) {
        oss << "\"completed_at\":" << completed_at.value() << ",";
    } else {
        oss << "\"completed_at\":null,";
    }

    if (error_message.has_value()) {
        oss << "\"error_message\":\"" << escape_json(error_message.value()) << "\"";
    } else {
        oss << "\"error_message\":null";
    }

    oss << "}";
    return oss.str();
}

DearTs::Core::Result<OfflineOperation, std::string>
OfflineOperation::from_json(const std::string& json) {
    // 简单 JSON 解析（实际应使用 nlohmann/json）
    OfflineOperation op;

    // 这里简化处理，实际需要完整的 JSON 解析器
    // 为了示例，我们只做基本解析

    size_t pos = 0;

    // 解析 id
    pos = json.find("\"id\":");
    if (pos == std::string::npos) {
        return DearTs::Core::Result<OfflineOperation, std::string>::err("Missing id field");
    }
    pos += 5;
    op.id = std::stoll(json.substr(pos));

    // 解析 type
    pos = json.find("\"type\":");
    if (pos == std::string::npos) {
        return DearTs::Core::Result<OfflineOperation, std::string>::err("Missing type field");
    }
    pos += 7;
    op.type = static_cast<OfflineOperationType>(std::stoi(json.substr(pos)));

    // 解析 status
    pos = json.find("\"status\":");
    if (pos == std::string::npos) {
        return DearTs::Core::Result<OfflineOperation, std::string>::err("Missing status field");
    }
    pos += 9;
    op.status = static_cast<OfflineOperationStatus>(std::stoi(json.substr(pos)));

    // 其他字段类似处理（省略详细实现）
    op.payload = "";
    op.correlation_id = "";
    op.retry_count = 0;
    op.created_at = 0;

    return DearTs::Core::Result<OfflineOperation, std::string>::ok(op);
}

} // namespace DearTs::Plugins::MemoryCore::Consistency
