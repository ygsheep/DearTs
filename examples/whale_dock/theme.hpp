#pragma once

#include <imgui.h>

#include <cmath>

namespace whale_dock {

// ============================================================================
// 主题常量：颜色、尺寸、动画
// ============================================================================

namespace theme {

// ---- 尺寸 ----
constexpr float kWidth       = 380.0f;  // 岛宽度
constexpr float kCollapsedH  = 44.0f;   // 胶囊高度
constexpr float kExpandedH   = 180.0f;  // 扩展卡片高度
constexpr float kAlertH      = 56.0f;   // 告警高度
constexpr float kCorner      = 16.0f;   // 圆角
constexpr float kPaddingX    = 16.0f;   // 水平内边距
constexpr float kPaddingY    = 10.0f;   // 垂直内边距
constexpr float kSpacing     = 8.0f;    // 元素间距

// ---- 动画 ----
constexpr float kAnimDuration    = 0.25f;  // 状态切换动画时长（秒）
constexpr float kIdleCollapseSec = 5.0f;   // 静默后自动收起（秒）
constexpr float kAlertBlinkSpeed = 4.0f;   // 告警闪烁频率（Hz）

// ---- 颜色 ----
// 背景半透明深色
constexpr ImU32 kBgColor       = IM_COL32(24, 24, 32, 235);
constexpr ImU32 kBgHover       = IM_COL32(34, 34, 44, 240);
constexpr ImU32 kBgAlert       = IM_COL32(40, 28, 20, 240);

// 文字
constexpr ImU32 kTextPrimary   = IM_COL32(240, 240, 248, 255);
constexpr ImU32 kTextSecondary = IM_COL32(160, 160, 175, 220);
constexpr ImU32 kTextDim       = IM_COL32(110, 110, 125, 200);

// 余额颜色（按阈值）
inline ImU32 balanceColor(double usd) {
    if (usd < 1.0)  return IM_COL32(235, 80, 75, 255);   // 红
    if (usd < 5.0)  return IM_COL32(245, 180, 60, 255);  // 黄
    return IM_COL32(80, 200, 120, 255);                   // 绿
}

// 上下文条四档水位颜色（复用 cache_aware_compactor 阈值）
// 0-50% 绿、50-80% 黄、80-90% 橙、90-100% 红
inline ImU32 contextBarColor(float ratio) {
    if (ratio >= 0.90f) return IM_COL32(235, 80, 75, 255);
    if (ratio >= 0.80f) return IM_COL32(245, 140, 50, 255);
    if (ratio >= 0.50f) return IM_COL32(245, 180, 60, 255);
    return IM_COL32(80, 200, 120, 255);
}

// 告警边框颜色
inline ImU32 alertBorderColor(float blink_t) {
    // sin 波调制 alpha 实现闪烁
    const float a = 0.5f + 0.5f * std::sin(blink_t * kAlertBlinkSpeed * 2.0f * 3.14159f);
    const int alpha = static_cast<int>(180 + 75 * a);
    return IM_COL32(245, 140, 50, alpha);
}

}  // namespace theme

// ============================================================================
// 动画曲线
// ============================================================================

namespace anim {

inline float clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }

/// ease-out cubic：进入主体
inline float easeOutCubic(float t) {
    t = 1.0f - t;
    return 1.0f - t * t * t;
}

/// ease-in cubic：离开加速
inline float easeInCubic(float t) { return t * t * t; }

/// ease-out back：进入末端轻微过冲（macOS 弹性感）
inline float easeOutBack(float t, float overshoot = 0.25f) {
    const float c1 = 1.70158f + overshoot;
    const float c3 = c1 + 1.0f;
    t -= 1.0f;
    return 1.0f + c3 * t * t * t + c1 * t * t;
}

/// 线性插值
inline float lerp(float a, float b, float t) { return a + (b - a) * t; }

}  // namespace anim

}  // namespace whale_dock
