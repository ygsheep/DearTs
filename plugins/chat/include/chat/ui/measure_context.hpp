/**
 * @file measure_context.hpp
 * @brief 测量窗口上下文管理器
 * @details 提供隐藏的测量窗口，用于测量内容高度而不显示
 */

#pragma once

#include "chat/ui/measure_cache.hpp"
#include <imgui.h>
#include <string>

namespace DearTs::Plugins::Chat::UI {

/**
 * @brief 前向声明
 */
class MarkdownRenderer;

/**
 * @brief 测量样式配置
 */
struct MeasureStyle {
    float padding_x = 16.0f;
    float padding_y = 12.0f;
    bool enable_markdown = true;
    ImVec4 text_color{0.95f, 0.95f, 0.95f, 1.0f};

    /**
     * @brief 生成样式哈希
     */
    size_t hash() const;
};

/**
 * @brief 测量窗口管理器（单例）
 * @details 提供隐藏的测量窗口和缓存机制
 */
class MeasureContext {
public:
    /**
     * @brief 获取单例实例
     */
    static MeasureContext& instance();

    /**
     * @brief 测量 Markdown 内容高度
     * @param content Markdown 文本
     * @param width 可用宽度
     * @param style 测量样式
     * @return 测量到的高度（从缓存或实际测量）
     */
    float measure_markdown_height(
        const std::string& content,
        float width,
        const MeasureStyle& style = {}
    );

    /**
     * @brief 测量纯文本高度
     * @param content 文本内容
     * @param width 可用宽度
     * @param style 测量样式
     * @return 测量到的高度
     */
    float measure_text_height(
        const std::string& content,
        float width,
        const MeasureStyle& style = {}
    );

    /**
     * @brief 清除所有缓存
     */
    void clear_cache();

    /**
     * @brief 清除过期缓存
     * @param max_age_ms 最大缓存时间（毫秒）
     */
    void clear_expired_cache(uint32_t max_age_ms = 300000);

    /**
     * @brief 获取缓存统计信息
     */
    MeasureCacheStats get_cache_stats() const;

    /**
     * @brief 获取缓存配置
     */
    const MeasureCacheConfig& get_cache_config() const;

    /**
     * @brief 设置缓存配置
     */
    void set_cache_config(const MeasureCacheConfig& config);

    // 禁止拷贝和移动
    MeasureContext(const MeasureContext&) = delete;
    MeasureContext& operator=(const MeasureContext&) = delete;
    MeasureContext(MeasureContext&&) = delete;
    MeasureContext& operator=(MeasureContext&&) = delete;

private:
    MeasureContext() = default;
    ~MeasureContext() = default;

    /**
     * @brief 实际执行测量（内部方法）
     * @param content 内容
     * @param width 宽度
     * @param style 样式
     * @param use_markdown 是否使用 Markdown 渲染
     * @return 测量到的高度
     */
    float perform_measurement(
        const std::string& content,
        float width,
        const MeasureStyle& style,
        bool use_markdown
    );

    /**
     * @brief 创建哈希键
     */
    ContentHash create_hash_key(
        const std::string& content,
        float width,
        const MeasureStyle& style,
        bool use_markdown
    );

    /**
     * @brief 哈希组合辅助函数
     */
    template<typename T>
    static void hash_combine(size_t& seed, const T& value);
};

} // namespace DearTs::Plugins::Chat::UI
