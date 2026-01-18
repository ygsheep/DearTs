/**
 * @file measure_context.cpp
 * @brief 测量窗口上下文管理器实现
 */

#include "chat/ui/measure_context.hpp"
#include "chat/ui/markdown_renderer.hpp"
#include <functional>

namespace DearTs::Plugins::Chat::UI {

// ============================================================================
// MeasureStyle 实现
// ============================================================================

size_t MeasureStyle::hash() const {
    // 内联哈希组合辅助函数
    auto hash_combine = [](size_t& seed, auto value) {
        seed ^= std::hash<decltype(value)>{}(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    };

    size_t seed = 0;
    hash_combine(seed, padding_x);
    hash_combine(seed, padding_y);
    hash_combine(seed, enable_markdown);
    hash_combine(seed, text_color.x);
    hash_combine(seed, text_color.y);
    hash_combine(seed, text_color.z);
    hash_combine(seed, text_color.w);
    return seed;
}

// ============================================================================
// MeasureContext 实现
// ============================================================================

MeasureContext& MeasureContext::instance() {
    static MeasureContext instance;
    return instance;
}

float MeasureContext::measure_markdown_height(
    const std::string& content,
    float width,
    const MeasureStyle& style
) {
    // 创建哈希键
    ContentHash hash = create_hash_key(content, width, style, true);

    // 尝试从缓存获取
    if (auto cached_height = MeasureCache::instance().lookup(hash)) {
        return *cached_height;
    }

    // 缓存未命中，执行实际测量
    const float measured_height = perform_measurement(content, width, style, true);

    // 插入缓存
    MeasureCache::instance().insert(hash, measured_height);

    return measured_height;
}

float MeasureContext::measure_text_height(
    const std::string& content,
    float width,
    const MeasureStyle& style
) {
    // 创建哈希键
    ContentHash hash = create_hash_key(content, width, style, false);

    // 尝试从缓存获取
    if (auto cached_height = MeasureCache::instance().lookup(hash)) {
        return *cached_height;
    }

    // 缓存未命中，执行实际测量
    const float measured_height = perform_measurement(content, width, style, false);

    // 插入缓存
    MeasureCache::instance().insert(hash, measured_height);

    return measured_height;
}

void MeasureContext::clear_cache() {
    MeasureCache::instance().clear();
}

void MeasureContext::clear_expired_cache(uint32_t max_age_ms) {
    // 临时修改配置的过期时间
    const auto original_config = MeasureCache::instance().get_config();
    MeasureCacheConfig temp_config = original_config;
    temp_config.max_age = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::milliseconds(max_age_ms));
    MeasureCache::instance().set_config(temp_config);

    // 清除过期条目
    MeasureCache::instance().clear_expired();

    // 恢复原始配置
    MeasureCache::instance().set_config(original_config);
}

MeasureCacheStats MeasureContext::get_cache_stats() const {
    return MeasureCache::instance().get_stats();
}

const MeasureCacheConfig& MeasureContext::get_cache_config() const {
    return MeasureCache::instance().get_config();
}

void MeasureContext::set_cache_config(const MeasureCacheConfig& config) {
    MeasureCache::instance().set_config(config);
}

float MeasureContext::perform_measurement(
    const std::string& content,
    float width,
    const MeasureStyle& style,
    bool use_markdown
) {
    // 保存当前 ImGui 状态
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(style.padding_x, style.padding_y));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));  // 透明背景

    // 使用固定窗口 ID（避免泄漏）
    static const char* MEASURE_WINDOW_ID = "##dearts_measure_window";

    // 创建隐藏的测量窗口
    ImGui::BeginChild(
        MEASURE_WINDOW_ID,
        ImVec2(width, 10000.0f),  // 足够大的高度
        false,
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoDecoration  // 完全隐藏装饰
    );

    // 添加内边距
    ImGui::Indent(style.padding_x);
    ImGui::Dummy(ImVec2(0.0f, style.padding_y));

    const float content_avail_width = width - style.padding_x * 2;

    // 渲染内容（仅测量，不显示）
    if (use_markdown) {
        MarkdownRenderer::render(content);
    } else {
        ImGui::PushStyleColor(ImGuiCol_Text, style.text_color);
        ImGui::PushTextWrapPos(content_avail_width);
        ImGui::Text("%s", content.c_str());
        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();
    }

    ImGui::Dummy(ImVec2(0.0f, style.padding_y));
    ImGui::Unindent(style.padding_x);

    // 获取测量高度
    const float measured_height = ImGui::GetCursorPosY();

    ImGui::EndChild();
    ImGui::PopStyleColor();  // ChildBg
    ImGui::PopStyleVar(3);    // WindowPadding, ChildRounding, ChildBorderSize

    return measured_height;
}

ContentHash MeasureContext::create_hash_key(
    const std::string& content,
    float width,
    const MeasureStyle& style,
    bool use_markdown
) {
    ContentHash hash;

    // 内容哈希：使用 std::hash
    hash.content_hash = std::hash<std::string>{}(content);

    // 宽度哈希：转换为整数后哈希（避免浮点精度问题）
    // 乘以 10 精确到 0.1 像素
    hash.width_hash = std::hash<int>{}(static_cast<int>(width * 10));

    // 样式哈希：组合所有样式参数
    size_t style_seed = style.hash();
    hash_combine(style_seed, use_markdown);
    hash.style_hash = style_seed;

    return hash;
}

template<typename T>
void MeasureContext::hash_combine(size_t& seed, const T& value) {
    seed ^= std::hash<T>{}(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

} // namespace DearTs::Plugins::Chat::UI
