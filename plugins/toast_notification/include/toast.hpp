/**
 * @file toast.hpp
 * @brief Toast Notification 数据结构和类型定义
 * @details 提供气泡消息的核心数据结构和类型枚举
 */

#pragma once

#include <string>
#include <chrono>
#include <imgui.h>
#include "core/ui/icon_font.hpp"

namespace DearTs::Plugins::Toast {

/**
 * @brief Toast 消息类型
 */
enum class ToastType {
    Info,       ///< 信息提示（蓝色）
    Success,    ///< 成功提示（绿色）
    Warning,    ///< 警告提示（橙色）
    Error,      ///< 错误提示（红色）
    None        ///< 无类型（默认样式）
};

/**
 * @brief Toast 位置
 */
enum class ToastPosition {
    TopLeft,        ///< 左上角
    TopCenter,      ///< 上中
    TopRight,       ///< 右上角
    BottomLeft,     ///< 左下角
    BottomCenter,   ///< 下中
    BottomRight,    ///< 右下角
};

/**
 * @brief Toast 消息结构
 */
struct ToastMessage {
    std::string title;                          ///< 标题
    std::string message;                        ///< 消息内容
    ToastType type = ToastType::Info;           ///< 消息类型
    std::chrono::milliseconds duration = std::chrono::milliseconds(3000);  ///< 显示时长
    int id = 0;                                 ///< 唯一 ID

    // 动画状态
    float animation_progress = 0.0f;            ///< 动画进度（0.0 - 1.0）
    bool is_entering = true;                    ///< 是否正在进入
    bool is_exiting = false;                    ///< 是否正在退出

    // 时间跟踪
    std::chrono::steady_clock::time_point created_at;  ///< 创建时间
    std::chrono::steady_clock::time_point expire_at;   ///< 过期时间
    std::chrono::milliseconds hover_remaining_time;    ///< 悬停开始时的剩余时间
    bool was_hovered = false;                  ///< 上一帧是否悬停（用于检测悬停状态变化）

    // UI 状态
    bool is_hovered = false;                    ///< 是否被鼠标悬停
    ImVec2 size = ImVec2(0, 0);                 ///< 当前大小
    float alpha = 1.0f;                         ///< 透明度

    /**
     * @brief 构造函数
     */
    ToastMessage(
        std::string title,
        std::string message,
        ToastType type = ToastType::Info,
        std::chrono::milliseconds duration = std::chrono::milliseconds(3000)
    ) : title(std::move(title)),
        message(std::move(message)),
        type(type),
        duration(duration),
        animation_progress(0.0f),
        is_entering(true),
        is_exiting(false),
        created_at(std::chrono::steady_clock::now()),
        expire_at(created_at + duration),
        is_hovered(false),
        alpha(0.0f) {}  // 初始透明度为 0，用于淡入动画

    /**
     * @brief 检查是否已过期
     */
    bool is_expired() const {
        return std::chrono::steady_clock::now() >= expire_at;
    }

    /**
     * @brief 获取剩余时间（毫秒）
     */
    int64_t get_remaining_time_ms() const {
        auto now = std::chrono::steady_clock::now();
        if (now >= expire_at) return 0;
        return std::chrono::duration_cast<std::chrono::milliseconds>(expire_at - now).count();
    }

    /**
     * @brief 获取剩余时间比例（0.0 - 1.0）
     */
    float get_remaining_progress() const {
        auto total = duration.count();
        auto remaining = get_remaining_time_ms();
        return static_cast<float>(remaining) / static_cast<float>(total);
    }

    /**
     * @brief 获取类型的颜色
     */
    ImVec4 get_type_color() const {
        switch (type) {
            case ToastType::Info:
                return ImVec4(0.33f, 0.65f, 1.0f, 1.0f);    // 蓝色
            case ToastType::Success:
                return ImVec4(0.33f, 0.82f, 0.33f, 1.0f);   // 绿色
            case ToastType::Warning:
                return ImVec4(1.0f, 0.82f, 0.33f, 1.0f);    // 橙色
            case ToastType::Error:
                return ImVec4(1.0f, 0.33f, 0.33f, 1.0f);    // 红色
            default:
                return ImVec4(0.5f, 0.5f, 0.5f, 1.0f);      // 灰色
        }
    }

    /**
     * @brief 获取类型的图标（使用 Material Icons）
     */
    const char* get_type_icon() const {
        switch (type) {
            case ToastType::Info:
                return ICON_INFO;        // ""
            case ToastType::Success:
                return ICON_CHECK;       // ""
            case ToastType::Warning:
                return ICON_WARNING;     // "⚠"
            case ToastType::Error:
                return ICON_ERROR;       // ""
            default:
                return ICON_NOTIFICATION; // ""
        }
    }
};

/**
 * @brief Toast 配置
 */
struct ToastConfig {
    double animation_speed = 3.0;              ///< 动画速度
    double enter_duration = 0.5;               ///< 进入动画时长（秒）- 增加到 0.5s 让动画更明显
    double exit_duration = 0.3;                ///< 退出动画时长（秒）
    double max_width = 400.0;                  ///< 最大宽度
    double min_width = 300.0;                  ///< 最小宽度
    double padding_x = 20.0;                   ///< 水平内边距（增加到 20px）
    double padding_y = 16.0;                   ///< 垂直内边距（增加到 16px）
    double spacing = 8.0;                      ///< Toast 之间的间距
    int max_toasts = 5;                        ///< 最大同时显示数量
    int position = static_cast<int>(ToastPosition::TopRight);  ///< Toast 位置（0-5）
    bool show_progress_bar = true;             ///< 是否显示进度条
    bool show_close_button = true;             ///< 是否显示关闭按钮
    bool show_copy_button = true;              ///< 是否显示复制按钮
    bool pause_on_hover = true;                ///< 悬停时是否暂停计时
    bool click_to_close = false;               ///< 点击是否关闭
};

} // namespace DearTs::Plugins::Toast
