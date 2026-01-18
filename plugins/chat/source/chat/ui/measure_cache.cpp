/**
 * @file measure_cache.cpp
 * @brief 测量结果缓存系统实现
 */

#include "chat/ui/measure_cache.hpp"

namespace DearTs::Plugins::Chat::UI {

MeasureCache& MeasureCache::instance() {
    static MeasureCache instance;
    return instance;
}

std::optional<float> MeasureCache::lookup(const ContentHash& hash) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_cache.find(hash);
    if (it != m_cache.end()) {
        // 检查是否过期
        auto now = std::chrono::system_clock::now();
        auto age = std::chrono::duration_cast<std::chrono::seconds>(now - it->second.timestamp);

        if (age >= m_config.max_age) {
            // 过期，移除并返回未命中
            m_cache.erase(it);
            m_stats.misses++;
            m_stats.current_size = m_cache.size();
            return std::nullopt;
        }

        // 命中，更新访问计数和时间戳
        it->second.access_count++;
        it->second.timestamp = now;
        m_stats.hits++;
        return it->second.measured_height;
    }

    m_stats.misses++;
    return std::nullopt;
}

void MeasureCache::insert(const ContentHash& hash, float height) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // 如果达到最大条目数，执行 LRU 清除
    if (m_cache.size() >= m_config.max_entries) {
        evict_lru();
    }

    // 插入新条目
    m_cache.emplace(hash, CacheEntry(height));
    m_stats.current_size = m_cache.size();
}

void MeasureCache::clear_expired() {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto now = std::chrono::system_clock::now();
    auto it = m_cache.begin();

    while (it != m_cache.end()) {
        auto age = std::chrono::duration_cast<std::chrono::seconds>(now - it->second.timestamp);
        if (age >= m_config.max_age) {
            it = m_cache.erase(it);
            m_stats.evictions++;
        } else {
            ++it;
        }
    }

    m_stats.current_size = m_cache.size();
}

void MeasureCache::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cache.clear();
    m_stats.evictions += m_stats.current_size;
    m_stats.current_size = 0;
}

MeasureCacheStats MeasureCache::get_stats() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stats;
}

void MeasureCache::set_config(const MeasureCacheConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_config = config;

    // 如果新的最大条目数小于当前大小，执行清除
    if (m_config.max_entries < m_cache.size()) {
        while (m_cache.size() > m_config.max_entries) {
            evict_lru();
        }
    }
}

void MeasureCache::evict_lru() {
    if (m_cache.empty()) {
        return;
    }

    // 查找最久未访问的条目（基于时间戳和访问次数）
    auto oldest_it = std::min_element(
        m_cache.begin(),
        m_cache.end(),
        [](const auto& a, const auto& b) {
            // 先比较时间戳
            if (a.second.timestamp != b.second.timestamp) {
                return a.second.timestamp < b.second.timestamp;
            }
            // 时间戳相同，比较访问次数
            return a.second.access_count < b.second.access_count;
        }
    );

    if (oldest_it != m_cache.end()) {
        m_cache.erase(oldest_it);
        m_stats.evictions++;
    }
}

} // namespace DearTs::Plugins::Chat::UI
