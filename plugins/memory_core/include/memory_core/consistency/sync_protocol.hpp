/**
 * @file sync_protocol.hpp
 * @brief 同步协议 - 处理与远程服务器的数据同步
 *
 * 功能：
 * - 双向同步协议
 * - 冲突检测和解决
 * - 增量同步
 * - 版本向量
 */

#pragma once

#include "core/result.h"
#include "memory_core/consistency/offline_queue.hpp"
#include "memory_core/memory/memory_manager.hpp"
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <optional>
#include <cstdint>
#include <map>
#include <chrono>

namespace DearTs::Plugins::MemoryCore::Consistency {

/**
 * @brief 同步方向
 */
enum class SyncDirection {
    Upload,                ///< 仅上传
    Download,              ///< 仅下载
    Bidirectional          ///< 双向同步
};

/**
 * @brief 同步状态
 */
enum class SyncStatus {
    Idle,                  ///< 空闲
    Connecting,            ///< 连接中
    Syncing,               ///< 同步中
    ConflictDetected,      ///< 检测到冲突
    Completed,             ///< 完成
    Failed                 ///< 失败
};

/**
 * @brief 冲突解决策略
 */
enum class ConflictResolution {
    ServerWins,            ///< 服务器优先
    ClientWins,            ///< 客户端优先
    NewestWins,            /// 最新修改优先
    Manual                 ///< 手动解决
};

/**
 * @brief 版本向量条目
 */
struct VersionVectorEntry {
    std::string node_id;   ///< 节点 ID
    int64_t version;       ///< 版本号

    /**
     * @brief 序列化为字符串
     */
    std::string to_string() const;

    /**
     * @brief 从字符串解析
     */
    static VersionVectorEntry from_string(const std::string& str);
};

/**
 * @brief 版本向量（用于检测冲突）
 */
class VersionVector {
public:
    /**
     * @brief 默认构造
     */
    VersionVector() = default;

    /**
     * @brief 增加版本
     * @param node_id 节点 ID
     */
    void increment(const std::string& node_id);

    /**
     * @brief 获取版本
     * @param node_id 节点 ID
     * @return 版本号，如果不存在返回 0
     */
    int64_t get(const std::string& node_id) const;

    /**
     * @brief 设置版本
     */
    void set(const std::string& node_id, int64_t version);

    /**
     * @brief 合并版本向量
     * @param other 另一个版本向量
     */
    void merge(const VersionVector& other);

    /**
     * @brief 比较版本向量
     * @param other 另一个版本向量
     * @return -1 表示 this < other，0 表示并发，1 表示 this > other
     */
    int compare(const VersionVector& other) const;

    /**
     * @brief 是否并发（有冲突）
     */
    bool is_concurrent_with(const VersionVector& other) const;

    /**
     * @brief 序列化为字符串
     */
    std::string to_string() const;

    /**
     * @brief 从字符串解析
     */
    static VersionVector from_string(const std::string& str);

    /**
     * @brief 获取所有条目
     */
    const std::map<std::string, int64_t>& get_entries() const {
        return m_entries;
    }

private:
    std::map<std::string, int64_t> m_entries;  ///< 节点 ID -> 版本号
};

/**
 * @brief 同步项
 */
struct SyncItem {
    std::string local_id;                     ///< 本地 ID
    std::optional<std::string> remote_id;      ///< 远程 ID
    Memory::MemoryType type;                  ///< 记忆类型
    std::string content;                      ///< 内容
    int64_t local_version;                    ///< 本地版本
    std::optional<int64_t> remote_version;    ///< 远程版本
    VersionVector version_vector;             ///< 版本向量
    bool has_conflict;                        ///< 是否有冲突

    /**
     * @brief 是否需要上传
     */
    bool needs_upload() const;

    /**
     * @brief 是否需要下载
     */
    bool needs_download() const;

    /**
     * @brief 是否有并发版本（可能有冲突）
     */
    bool has_concurrent_version() const {
        return has_conflict;
    }
};

/**
 * @brief 同步结果
 */
struct SyncResult {
    SyncStatus status;                        ///< 同步状态
    int uploaded_count;                       ///< 上传数量
    int downloaded_count;                     ///< 下载数量
    int conflict_count;                       ///< 冲突数量
    int skipped_count;                        ///< 跳过数量
    std::vector<std::string> error_messages;  ///< 错误信息
    int64_t duration_ms;                      ///< 耗时（毫秒）

    /**
     * @brief 创建成功结果
     */
    static SyncResult success(int uploaded, int downloaded) {
        return SyncResult{
            .status = SyncStatus::Completed,
            .uploaded_count = uploaded,
            .downloaded_count = downloaded,
            .conflict_count = 0,
            .skipped_count = 0,
            .error_messages = {},
            .duration_ms = 0
        };
    }

    /**
     * @brief 创建失败结果
     */
    static SyncResult failed(const std::string& error) {
        return SyncResult{
            .status = SyncStatus::Failed,
            .uploaded_count = 0,
            .downloaded_count = 0,
            .conflict_count = 0,
            .skipped_count = 0,
            .error_messages = {error},
            .duration_ms = 0
        };
    }
};

/**
 * @brief 同步配置
 */
struct SyncConfig {
    std::string node_id;                      ///< 节点 ID（唯一标识）
    std::string server_url;                   ///< 服务器 URL
    SyncDirection direction;                  ///< 同步方向
    ConflictResolution conflict_resolution;   ///< 冲突解决策略
    bool auto_sync;                           ///< 是否自动同步
    int sync_interval_seconds;                ///< 同步间隔（秒）
    bool compress_data;                       ///< 是否压缩数据
    int batch_size;                           ///< 批量大小

    /**
     * @brief 默认配置
     */
    static SyncConfig default_config() {
        return SyncConfig{
            .node_id = "client_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()),
            .server_url = "",
            .direction = SyncDirection::Bidirectional,
            .conflict_resolution = ConflictResolution::NewestWins,
            .auto_sync = false,
            .sync_interval_seconds = 300,  // 5 分钟
            .compress_data = true,
            .batch_size = 100
        };
    }
};

/**
 * @brief 同步进度回调
 *
 * @param current 当前数量
 * @param total 总数量
 * @param message 进度消息
 */
using SyncProgressCallback = std::function<void(int current, int total, const std::string& message)>;

/**
 * @brief 同步协议
 *
 * 负责与远程服务器进行数据同步
 */
class SyncProtocol {
public:
    /**
     * @brief 获取单例实例
     */
    static SyncProtocol& instance();

    /**
     * @brief 删除拷贝构造和赋值
     */
    SyncProtocol(const SyncProtocol&) = delete;
    SyncProtocol& operator=(const SyncProtocol&) = delete;

    // ============ 初始化 ============

    /**
     * @brief 初始化同步协议
     * @param config 同步配置
     * @return 成功或错误信息
     */
    DearTs::Core::Result<void, std::string> initialize(
        const SyncConfig& config = SyncConfig::default_config()
    );

    /**
     * @brief 关闭同步协议
     */
    void shutdown();

    // ============ 同步操作 ============

    /**
     * @brief 执行同步
     * @param progress_callback 进度回调（可选）
     * @return 同步结果
     */
    DearTs::Core::Result<SyncResult, std::string> sync(
        SyncProgressCallback progress_callback = nullptr
    );

    /**
     * @brief 上传本地更改
     * @return 上传数量或错误信息
     */
    DearTs::Core::Result<int, std::string> upload_changes();

    /**
     * @brief 下载远程更改
     * @return 下载数量或错误信息
     */
    DearTs::Core::Result<int, std::string> download_changes();

    /**
     * @brief 解决冲突
     * @param conflict_items 冲突项列表
     * @param resolution 解决策略
     * @return 解决的数量或错误信息
     */
    DearTs::Core::Result<int, std::string> resolve_conflicts(
        const std::vector<SyncItem>& conflict_items,
        ConflictResolution resolution
    );

    // ============ 状态查询 ============

    /**
     * @brief 获取同步状态
     * @return 当前状态
     */
    SyncStatus get_status() const {
        return m_status;
    }

    /**
     * @brief 获取配置
     * @return 当前配置
     */
    const SyncConfig& get_config() const {
        return m_config;
    }

    /**
     * @brief 是否在线
     * @return true 表示已连接到服务器
     */
    bool is_online() const {
        return m_status == SyncStatus::Idle ||
               m_status == SyncStatus::Syncing ||
               m_status == SyncStatus::Completed;
    }

    /**
     * @brief 获取版本向量
     * @return 当前版本向量
     */
    const VersionVector& get_version_vector() const {
        return m_version_vector;
    }

    // ============ 配置 ============

    /**
     * @brief 更新配置
     * @param config 新配置
     */
    void set_config(const SyncConfig& config);

    /**
     * @brief 设置进度回调
     * @param callback 回调函数
     */
    void set_progress_callback(SyncProgressCallback callback) {
        m_progress_callback = std::move(callback);
    }

private:
    /**
     * @brief 私有构造函数（单例模式）
     */
    SyncProtocol() = default;

    /**
     * @brief 析构函数
     */
    ~SyncProtocol() = default;

    /**
     * @brief 检测冲突
     * @param items 同步项列表
     * @return 有冲突的项
     */
    std::vector<SyncItem> detect_conflicts(const std::vector<SyncItem>& items);

    /**
     * @brief 应用远程更改
     * @param items 远程项列表
     * @return 应用数量或错误信息
     */
    DearTs::Core::Result<int, std::string> apply_remote_changes(
        const std::vector<SyncItem>& items
    );

    /**
     * @brief 上传本地更改
     * @param items 本地项列表
     * @return 上传数量或错误信息
     */
    DearTs::Core::Result<int, std::string> upload_local_changes(
        const std::vector<SyncItem>& items
    );

    /**
     * @brief 报告进度
     */
    void report_progress(int current, int total, const std::string& message);

    // ============ 成员变量 ============

    SyncConfig m_config;                              ///< 配置
    SyncStatus m_status;                              ///< 状态
    VersionVector m_version_vector;                   ///< 版本向量
    SyncProgressCallback m_progress_callback;         ///< 进度回调
    bool m_initialized;                               ///< 是否已初始化
};

} // namespace DearTs::Plugins::MemoryCore::Consistency
