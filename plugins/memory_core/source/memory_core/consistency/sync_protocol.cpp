/**
 * @file sync_protocol.cpp
 * @brief 同步协议实现
 */

#include "memory_core/consistency/sync_protocol.hpp"
#include "memory_core/memory/memory_manager.hpp"
#include "liblogger/logger.h"
#include <algorithm>
#include <sstream>
#include <chrono>
#include <set>
#include <cctype>

namespace DearTs::Plugins::MemoryCore::Consistency {

// ============ VersionVector 实现 ============

void VersionVector::increment(const std::string& node_id) {
    m_entries[node_id]++;
}

int64_t VersionVector::get(const std::string& node_id) const {
    auto it = m_entries.find(node_id);
    return it != m_entries.end() ? it->second : 0;
}

void VersionVector::set(const std::string& node_id, int64_t version) {
    m_entries[node_id] = version;
}

void VersionVector::merge(const VersionVector& other) {
    for (const auto& [node_id, version] : other.m_entries) {
        if (m_entries[node_id] < version) {
            m_entries[node_id] = version;
        }
    }
}

int VersionVector::compare(const VersionVector& other) const {
    // 收集所有节点
    std::set<std::string> all_nodes;
    for (const auto& [node_id, _] : m_entries) {
        all_nodes.insert(node_id);
    }
    for (const auto& [node_id, _] : other.m_entries) {
        all_nodes.insert(node_id);
    }

    bool this_less = false;
    bool this_greater = false;

    for (const auto& node_id : all_nodes) {
        int64_t v1 = get(node_id);
        int64_t v2 = other.get(node_id);

        if (v1 < v2) {
            this_less = true;
        } else if (v1 > v2) {
            this_greater = true;
        }
    }

    if (this_less && !this_greater) {
        return -1;  // this < other
    } else if (this_greater && !this_less) {
        return 1;   // this > other
    } else if (!this_less && !this_greater) {
        return 0;   // this == other
    } else {
        // 既有小于也有大于，说明是并发的
        return 0;
    }
}

bool VersionVector::is_concurrent_with(const VersionVector& other) const {
    // 收集所有节点
    std::set<std::string> all_nodes;
    for (const auto& [node_id, _] : m_entries) {
        all_nodes.insert(node_id);
    }
    for (const auto& [node_id, _] : other.m_entries) {
        all_nodes.insert(node_id);
    }

    bool this_less = false;
    bool this_greater = false;

    for (const auto& node_id : all_nodes) {
        int64_t v1 = get(node_id);
        int64_t v2 = other.get(node_id);

        if (v1 < v2) {
            this_less = true;
        } else if (v1 > v2) {
            this_greater = true;
        }
    }

    // 如果既有小于也有大于，说明是并发的
    return this_less && this_greater;
}

std::string VersionVector::to_string() const {
    std::ostringstream oss;
    oss << "{";
    bool first = true;
    for (const auto& [node_id, version] : m_entries) {
        if (!first) {
            oss << ",";
        }
        oss << "\"" << node_id << "\":" << version;
        first = false;
    }
    oss << "}";
    return oss.str();
}

VersionVector VersionVector::from_string(const std::string& str) {
    VersionVector vv;
    // 简化实现，实际需要完整的 JSON 解析
    // TODO: 实现完整的解析逻辑
    return vv;
}

std::string VersionVectorEntry::to_string() const {
    return node_id + ":" + std::to_string(version);
}

VersionVectorEntry VersionVectorEntry::from_string(const std::string& str) {
    VersionVectorEntry entry;
    size_t colon_pos = str.find(':');
    if (colon_pos != std::string::npos) {
        entry.node_id = str.substr(0, colon_pos);
        entry.version = std::stoll(str.substr(colon_pos + 1));
    }
    return entry;
}

// ============ SyncItem 实现 ============

bool SyncItem::needs_upload() const {
    // 本地版本更新或没有远程版本
    return !remote_version.has_value() || local_version > remote_version.value();
}

bool SyncItem::needs_download() const {
    // 远程版本更新
    return remote_version.has_value() && remote_version.value() > local_version;
}

// ============ 单例实现 ============

SyncProtocol& SyncProtocol::instance() {
    static SyncProtocol instance;
    return instance;
}

// ============ 初始化 ============

DearTs::Core::Result<void, std::string> SyncProtocol::initialize(
    const SyncConfig& config
) {
    if (m_initialized) {
        LOG_WARN("SyncProtocol already initialized");
        return DearTs::Core::Result<void, std::string>::ok();
    }

    m_config = config;
    m_status = SyncStatus::Idle;
    m_initialized = true;

    LOG_INFO("SyncProtocol initialized: node_id={}, server_url={}",
             config.node_id, config.server_url);
    return DearTs::Core::Result<void, std::string>::ok();
}

void SyncProtocol::shutdown() {
    if (!m_initialized) {
        return;
    }

    LOG_INFO("SyncProtocol shutdown");
    m_initialized = false;
}

// ============ 同步操作 ============

DearTs::Core::Result<SyncResult, std::string> SyncProtocol::sync(
    SyncProgressCallback progress_callback
) {
    if (!m_initialized) {
        return DearTs::Core::Result<SyncResult, std::string>::err(
            "SyncProtocol not initialized"
        );
    }

    if (m_config.server_url.empty()) {
        return DearTs::Core::Result<SyncResult, std::string>::err(
            "Server URL not configured"
        );
    }

    auto start_time = std::chrono::system_clock::now();
    m_status = SyncStatus::Syncing;

    report_progress(0, 100, "Starting sync...");

    SyncResult result;
    result.status = SyncStatus::Completed;
    result.uploaded_count = 0;
    result.downloaded_count = 0;
    result.conflict_count = 0;
    result.skipped_count = 0;

    try {
        // 1. 上传本地更改
        report_progress(10, 100, "Uploading local changes...");
        auto upload_result = upload_changes();
        if (upload_result.isOk()) {
            result.uploaded_count = upload_result.unwrap();
        } else {
            result.status = SyncStatus::Failed;
            result.error_messages.push_back("Upload failed: " + upload_result.error());
        }

        // 2. 下载远程更改
        report_progress(50, 100, "Downloading remote changes...");
        auto download_result = download_changes();
        if (download_result.isOk()) {
            result.downloaded_count = download_result.unwrap();
        } else {
            result.status = SyncStatus::Failed;
            result.error_messages.push_back("Download failed: " + download_result.error());
        }

        report_progress(100, 100, "Sync completed");

    } catch (const std::exception& e) {
        result.status = SyncStatus::Failed;
        result.error_messages.push_back(e.what());
    }

    auto end_time = std::chrono::system_clock::now();
    result.duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time
    ).count();

    m_status = result.status == SyncStatus::Completed ? SyncStatus::Idle : SyncStatus::Failed;

    LOG_INFO("Sync completed: uploaded={}, downloaded={}, conflicts={}, errors={}, duration={} ms",
             result.uploaded_count, result.downloaded_count,
             result.conflict_count, result.error_messages.size(),
             result.duration_ms);

    return DearTs::Core::Result<SyncResult, std::string>::ok(result);
}

DearTs::Core::Result<int, std::string> SyncProtocol::upload_changes() {
    // TODO: 实现实际的上传逻辑
    // 当前：占位符实现

    if (m_config.direction == SyncDirection::Download) {
        LOG_DEBUG("Upload skipped (direction is Download only)");
        return DearTs::Core::Result<int, std::string>::ok(0);
    }

    // 获取本地记忆
    auto& manager = Memory::MemoryManager::instance();
    auto memories_result = manager.get_all_memories();

    if (memories_result.isErr()) {
        return DearTs::Core::Result<int, std::string>::err(
            "Failed to get memories: " + memories_result.error()
        );
    }

    auto memories = memories_result.unwrap();

    // 构建同步项
    std::vector<SyncItem> items;
    for (const auto& memory : memories) {
        SyncItem item;
        item.local_id = std::to_string(memory.id);
        item.type = memory.type;
        item.content = memory.content;
        item.local_version = memory.created_at;
        item.remote_version = std::nullopt;
        item.has_conflict = false;

        if (item.needs_upload()) {
            items.push_back(item);
        }
    }

    // 上传
    auto upload_result = upload_local_changes(items);
    if (upload_result.isErr()) {
        return DearTs::Core::Result<int, std::string>::err(upload_result.error());
    }

    LOG_INFO("Uploaded {} changes", items.size());
    return DearTs::Core::Result<int, std::string>::ok(static_cast<int>(items.size()));
}

DearTs::Core::Result<int, std::string> SyncProtocol::download_changes() {
    // TODO: 实现实际的下载逻辑
    // 当前：占位符实现

    if (m_config.direction == SyncDirection::Upload) {
        LOG_DEBUG("Download skipped (direction is Upload only)");
        return DearTs::Core::Result<int, std::string>::ok(0);
    }

    // 占位符：模拟下载
    std::vector<SyncItem> remote_items;

    auto apply_result = apply_remote_changes(remote_items);
    if (apply_result.isErr()) {
        return DearTs::Core::Result<int, std::string>::err(apply_result.error());
    }

    LOG_INFO("Downloaded {} changes", remote_items.size());
    return DearTs::Core::Result<int, std::string>::ok(static_cast<int>(remote_items.size()));
}

DearTs::Core::Result<int, std::string> SyncProtocol::resolve_conflicts(
    const std::vector<SyncItem>& conflict_items,
    ConflictResolution resolution
) {
    int resolved = 0;

    for (const auto& item : conflict_items) {
        switch (resolution) {
            case ConflictResolution::ServerWins:
                // 保留服务器版本，跳过本地版本
                break;

            case ConflictResolution::ClientWins:
                // 强制上传本地版本
                break;

            case ConflictResolution::NewestWins:
                // 比较时间戳
                break;

            case ConflictResolution::Manual:
                // 需要用户手动解决
                LOG_WARN("Manual conflict resolution required for item: {}", item.local_id);
                break;
        }
        resolved++;
    }

    LOG_INFO("Resolved {} conflicts using strategy: {}", resolved,
             static_cast<int>(resolution));
    return DearTs::Core::Result<int, std::string>::ok(resolved);
}

// ============ 配置 ============

void SyncProtocol::set_config(const SyncConfig& config) {
    m_config = config;
    LOG_INFO("SyncProtocol config updated: server_url={}", config.server_url);
}

// ============ 私有辅助方法 ============

std::vector<SyncItem> SyncProtocol::detect_conflicts(
    const std::vector<SyncItem>& items
) {
    std::vector<SyncItem> conflicts;

    for (const auto& item : items) {
        if (item.has_concurrent_version()) {
            // 检查版本向量是否并发
            // TODO: 实现完整的冲突检测逻辑
        }
    }

    return conflicts;
}

DearTs::Core::Result<int, std::string> SyncProtocol::apply_remote_changes(
    const std::vector<SyncItem>& items
) {
    auto& manager = Memory::MemoryManager::instance();
    int applied = 0;

    for (const auto& item : items) {
        if (!item.needs_download()) {
            continue;
        }

        // 检查是否已存在
        if (!item.local_id.empty()) {
            int64_t local_id = std::stoll(item.local_id);
            auto existing_result = manager.get_memory(local_id);

            if (existing_result.isOk()) {
                // 更新现有记忆
                auto& existing = existing_result.unwrap();
                existing.content = item.content;
                auto update_result = manager.update_memory(local_id, existing);
                if (update_result.isOk()) {
                    applied++;
                }
            } else {
                // 创建新记忆
                Memory::Memory new_memory;
                new_memory.type = item.type;
                new_memory.content = item.content;
                new_memory.importance = 0.5;

                auto add_result = manager.add_memory(new_memory);
                if (add_result.isOk()) {
                    applied++;
                }
            }
        }
    }

    return DearTs::Core::Result<int, std::string>::ok(applied);
}

DearTs::Core::Result<int, std::string> SyncProtocol::upload_local_changes(
    const std::vector<SyncItem>& items
) {
    // TODO: 实现实际的上传逻辑
    // 当前：占位符实现

    LOG_DEBUG("Uploading {} local changes (placeholder)", items.size());

    // 更新版本向量
    for (const auto& item : items) {
        m_version_vector.increment(m_config.node_id);
    }

    return DearTs::Core::Result<int, std::string>::ok(static_cast<int>(items.size()));
}

void SyncProtocol::report_progress(int current, int total, const std::string& message) {
    LOG_DEBUG("Sync progress: {}/{} - {}", current, total, message);

    if (m_progress_callback) {
        m_progress_callback(current, total, message);
    }
}

} // namespace DearTs::Plugins::MemoryCore::Consistency
