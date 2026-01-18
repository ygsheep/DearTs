/**
 * @file measure_cache.hpp
 * @brief 测量结果缓存系统
 * @details 用于缓存聊天消息的高度测量结果，避免重复渲染
 */

#pragma once

#include <chrono>
#include <functional>
#include <mutex>
#include <optional>
#include <unordered_map>

namespace DearTs::Plugins::Chat::UI {

/**
 * @brief 内容哈希键
 * @details 组合内容、宽度、样式三个维度的哈希值
 */
struct ContentHash {
    size_t content_hash = 0;      // 内容字符串哈希
    size_t width_hash = 0;        // 宽度参数哈希
    size_t style_hash = 0;        // 样式参数哈希

    /**
     * @brief 计算组合哈希值
     */
    size_t combined_hash() const {
        return content_hash ^ (width_hash << 1) ^ (style_hash << 2);
    }

    /**
     * @brief 相等比较
     */
    bool operator==(const ContentHash& other) const {
        return content_hash == other.content_hash &&
               width_hash == other.width_hash &&
               style_hash == other.style_hash;
    }
};

/**
 * @brief ContentHash 哈希函数（用于 unordered_map）
 */
struct ContentHasher {
    size_t operator()(const ContentHash& hash) const noexcept {
        return hash.combined_hash();
    }
};

/**
 * @brief 缓存条目
 */
struct CacheEntry {
    float measured_height;                           // 测量到的高度
    std::chrono::system_clock::time_point timestamp; // 缓存时间
    size_t access_count;                             // 访问次数（用于 LRU）

    explicit CacheEntry(float height)
        : measured_height(height)
        , timestamp(std::chrono::system_clock::now())
        , access_count(1) {}
};

/**
 * @brief 测量缓存配置
 */
struct MeasureCacheConfig {
    size_t max_entries = 100;                   // 最大缓存条目数
    std::chrono::seconds max_age{300};          // 最大缓存时间（5分钟）
    bool enable_lru = true;                     // 启用 LRU 清除
    bool enable_stats = true;                   // 启用统计信息
};

/**
 * @brief 测量缓存统计信息
 */
struct MeasureCacheStats {
    size_t hits = 0;         // 缓存命中次数
    size_t misses = 0;       // 缓存未命中次数
    size_t evictions = 0;    // 清除次数
    size_t current_size = 0; // 当前缓存大小

    /**
     * @brief 计算缓存命中率
     */
    double hit_rate() const {
        const double total = static_cast<double>(hits) + static_cast<double>(misses);
        return total > 0.0 ? static_cast<double>(hits) / total : 0.0;
    }
};

/**
 * @brief 测量缓存类
 * @details 单例模式，提供线程安全的测量结果缓存
 */
class MeasureCache {
public:
    /**
     * @brief 获取单例实例
     */
    static MeasureCache& instance();

    /**
     * @brief 查询缓存
     * @param hash 内容哈希键
     * @return 如果缓存命中，返回测量高度；否则返回 nullopt
     */
    std::optional<float> lookup(const ContentHash& hash);

    /**
     * @brief 插入缓存
     * @param hash 内容哈希键
     * @param height 测量到的高度
     */
    void insert(const ContentHash& hash, float height);

    /**
     * @brief 清除过期条目
     */
    void clear_expired();

    /**
     * @brief 清除所有缓存
     */
    void clear();

    /**
     * @brief 获取统计信息
     */
    MeasureCacheStats get_stats() const;

    /**
     * @brief 设置配置
     */
    void set_config(const MeasureCacheConfig& config);

    /**
     * @brief 获取配置
     */
    const MeasureCacheConfig& get_config() const { return m_config; }

    // 禁止拷贝和移动
    MeasureCache(const MeasureCache&) = delete;
    MeasureCache& operator=(const MeasureCache&) = delete;
    MeasureCache(MeasureCache&&) = delete;
    MeasureCache& operator=(MeasureCache&&) = delete;

private:
    MeasureCache() = default;
    ~MeasureCache() = default;

    /**
     * @brief 执行 LRU 清除
     */
    void evict_lru();

    using CacheMap = std::unordered_map<ContentHash, CacheEntry, ContentHasher>;

    CacheMap m_cache;
    MeasureCacheConfig m_config;
    MeasureCacheStats m_stats;
    mutable std::mutex m_mutex;  // 线程安全
};

} // namespace DearTs::Plugins::Chat::UI
