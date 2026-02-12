/**
 * @file scale_manager.h
 * @brief 高分辨率自动缩放管理器
 *
 * 提供基于 DPI 的自动缩放检测和计算功能，支持 1K/2K/4K 显示器
 */

#pragma once

#include <SDL3/SDL.h>
#include <functional>
#include <string>
#include <atomic>

#include "core/result.h"

namespace DearTs::Core::UI {

/**
 * @brief 缩放模式枚举
 */
enum class ScaleMode {
    Auto,   // 自动模式：根据分辨率自动计算推荐缩放
    Manual, // 手动模式：用户手动指定缩放比例
    System  // 系统模式：使用系统 DPI 设置
};

// 用于简化作用域访问
using ScaleMode_ = ScaleMode;

/**
 * @brief 显示器信息结构
 */
struct DisplayInfo {
    int width;          // 显示器宽度（像素）
    int height;         // 显示器高度（像素）
    float dpi;          // DPI 值
    float scale;        // 计算的推荐缩放比例
    bool is_high_dpi;  // 是否为高 DPI 显示器
};

/**
 * @brief 缩放变更事件
 */
struct ScaleChangedEvent {
    float old_scale;    // 旧缩放比例
    float new_scale;    // 新缩放比例
    ScaleMode mode;      // 触发缩放的变更的模式
};

/**
 * @brief 缩放管理器类
 *
 * 单例类，负责：
 * - 自动 DPI 检测
 * - 智能缩放计算
 * - 配置持久化
 * - 事件通知
 */
class ScaleManager {
public:
    /**
     * @brief 获取单例实例
     */
    static ScaleManager& instance();

    /**
     * @brief 初始化缩放管理器
     * @param window SDL 窗口指针，用于检测显示器信息
     * @return 成功返回 Result::ok()，失败返回错误信息
     */
    Result<void, std::string> initialize(SDL_Window* window);

    /**
     * @brief 获取当前缩放比例
     */
    [[nodiscard]] float get_scale() const;

    /**
     * @brief 设置缩放比例
     * @param scale 新的缩放比例（范围：0.5 - 2.0）
     * @param mode 缩放模式（默认 Manual）
     * @return 成功返回 Result::ok()，失败返回错误信息
     */
    Result<void, std::string> set_scale(float scale, ScaleMode mode = ScaleMode::Manual);

    /**
     * @brief 启用自动缩放模式
     */
    void enable_auto_scale();

    /**
     * @brief 获取当前缩放模式
     */
    [[nodiscard]] ScaleMode get_mode() const;

    /**
     * @brief 应用缩放到 ImGui
     *
     * 设置 ImGuiIO::FontGlobalScale 和 ImGuiStyle::ScaleAllSizes
     */
    void apply_to_imgui();

    /**
     * @brief 获取显示器信息
     */
    [[nodiscard]] const DisplayInfo& get_display_info() const;

    /**
     * @brief 计算推荐缩放比例（静态方法）
     * @param width 显示器宽度
     * @param height 显示器高度
     * @return 推荐的缩放比例
     *
     * 分辨率与推荐缩放对应关系：
     * - ≤1920x1200 (1K)  -> 1.0x (100%)
     * - ≤2560x1600 (2K)  -> 1.5x (150%)
     * - >2560x1600 (4K+) -> 2.0x (200%)
     */
    [[nodiscard]] static float calculate_recommended_scale(int width, int height);

    /**
     * @brief 判断是否已初始化
     */
    [[nodiscard]] bool is_initialized() const;

private:
    ScaleManager() = default;
    ~ScaleManager() = default;

    // 禁止拷贝和移动
    ScaleManager(const ScaleManager&) = delete;
    ScaleManager& operator=(const ScaleManager&) = delete;
    ScaleManager(ScaleManager&&) = delete;
    ScaleManager& operator=(ScaleManager&&) = delete;

    /**
     * @brief 检测显示器 DPI 信息
     */
    Result<void, std::string> detect_display_dpi();

    /**
     * @brief 应用缩放（内部方法）
     */
    void apply_scale(float scale, ScaleMode mode);

    /**
     * @brief 保存缩放设置到配置
     */
    void save_to_config();

    /**
     * @brief 从配置加载缩放设置
     */
    void load_from_config();

    /**
     * @brief 通知缩放变更事件
     */
    void notify_scale_changed(float old_scale, float new_scale);

    SDL_Window* m_window = nullptr;
    float m_scale = 1.0f;
    ScaleMode m_mode = ScaleMode::Auto;
    DisplayInfo m_display_info{};
    bool m_initialized = false;
};

} // namespace DearTs::Core::UI
