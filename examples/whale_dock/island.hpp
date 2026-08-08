#pragma once

#include "theme.hpp"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace whale_dock {

// ============================================================================
// 数据模型
// ============================================================================

struct ToolCallEntry {
    std::string tool_name;      // "Bash" / "Read" / "Edit"
    std::string arguments;      // 参数预览
    bool        is_error   = false;
    bool        is_running = false;
    float       duration_ms = 0.0f;
    std::string result_preview;  // 结果预览（1 行）
};

enum class ActivityKind {
    Idle,         // 💤 空闲
    Thinking,     // ● 思考中
    ToolRunning,  // ⚡ 工具运行中
    Done,         // ✓ 完成
};

struct IslandData {
    // 会话信息
    std::string project_root = "D:\\develop\\Workspace\\workx";
    std::string model        = "DeepSeek-V4-Flash";

    // 余额
    double balance_usd = 10.478;
    double task_cost   = 0.034;   // 本任务花费
    bool   cost_estimated = false;

    // 上下文
    int context_used  = 57234;
    int context_total = 1000000;
    int cache_hit_rate = 73;      // 百分比

    // 当前活动
    ActivityKind activity = ActivityKind::Idle;
    float activity_elapsed_sec = 0.0f;  // 当前活动已用时

    // 任务耗时
    float task_elapsed_sec = 12.0f;

    // 工具历史（最近 10 条）
    std::vector<ToolCallEntry> tool_history;

    // 告警
    std::string alert_message;
};

// ============================================================================
// 状态机
// ============================================================================

enum class Phase {
    Hidden,
    Collapsed,
    Expanded,
    Alert,
};

class IslandController {
public:
    void transitionTo(Phase next) {
        if (next == m_phase) return;
        m_prevPhase = m_phase;
        m_phase    = next;
        m_animT    = 0.0f;
        m_idleTimer = 0.0f;
    }

    void update(float dt, bool mouseHover) {
        // 动画推进
        if (m_animT < 1.0f) {
            m_animT = std::min(1.0f, m_animT + dt / theme::kAnimDuration);
        }

        // 静默计时
        m_idleTimer += dt;
        if (m_phase == Phase::Expanded && m_idleTimer > theme::kIdleCollapseSec && !mouseHover && !m_locked) {
            transitionTo(Phase::Collapsed);
        }

        // 告警闪烁计时
        m_blinkT += dt;

        // 数据活动计时
        m_data.activity_elapsed_sec += dt;
        m_data.task_elapsed_sec += dt;
    }

    // ---- 事件驱动 ----
    void onToolCall(const std::string& tool, const std::string& args) {
        m_data.activity = ActivityKind::ToolRunning;
        m_data.activity_elapsed_sec = 0.0f;
        m_data.tool_history.push_back({
            .tool_name = tool,
            .arguments = args,
            .is_running = true,
        });
        if (m_data.tool_history.size() > 10) m_data.tool_history.erase(m_data.tool_history.begin());
        transitionTo(Phase::Expanded);
    }

    void onToolResult(int callIdx, bool isError, float durationMs, const std::string& result) {
        m_data.activity = ActivityKind::Idle;
        if (callIdx >= 0 && callIdx < (int)m_data.tool_history.size()) {
            auto& tc = m_data.tool_history[callIdx];
            tc.is_running = false;
            tc.is_error = isError;
            tc.duration_ms = durationMs;
            tc.result_preview = result;
        }
        if (isError) {
            m_data.alert_message = "工具执行失败: " + m_data.tool_history[callIdx].tool_name;
            transitionTo(Phase::Alert);
        } else {
            transitionTo(Phase::Expanded);
        }
    }

    void onThinkingStart() {
        m_data.activity = ActivityKind::Thinking;
        m_data.activity_elapsed_sec = 0.0f;
        transitionTo(Phase::Expanded);
    }

    void onThinkingDone(float durationSec) {
        m_data.activity = ActivityKind::Idle;
        (void)durationSec;
        transitionTo(Phase::Collapsed);
    }

    void onAlert(const std::string& msg) {
        m_data.alert_message = msg;
        transitionTo(Phase::Alert);
    }

    void onBalanceUpdate(double balance) {
        m_data.balance_usd = balance;
        if (balance < 1.0 && m_phase != Phase::Alert) {
            onAlert("余额不足 $1.00，请及时充值");
        }
    }

    void onCostUpdate(double taskCost) {
        m_data.task_cost = taskCost;
    }

    // ---- 鼠标交互 ----
    void onClick() {
        if (m_phase == Phase::Collapsed) {
            transitionTo(Phase::Expanded);
            m_locked = true;
        } else if (m_phase == Phase::Expanded) {
            if (m_locked) {
                m_locked = false;  // 解锁，允许自动收起
            } else {
                transitionTo(Phase::Collapsed);
            }
        } else if (m_phase == Phase::Alert) {
            transitionTo(Phase::Collapsed);  // 确认告警
        }
    }

    // ---- 访问器 ----
    Phase phase() const { return m_phase; }
    float animT() const { return m_animT; }
    float blinkT() const { return m_blinkT; }
    bool  locked() const { return m_locked; }
    const IslandData& data() const { return m_data; }
    IslandData& data() { return m_data; }

    /// 当前动画插值后的高度
    float currentHeight() const {
        const float t = anim::easeOutCubic(m_animT);
        const float h0 = heightOf(m_prevPhase);
        const float h1 = heightOf(m_phase);
        return anim::lerp(h0, h1, t);
    }

    /// 当前 alpha
    float currentAlpha() const {
        const float t = anim::easeOutCubic(m_animT);
        if (m_phase == Phase::Hidden) return anim::lerp(1.0f, 0.0f, t);
        if (m_prevPhase == Phase::Hidden) return anim::lerp(0.0f, 1.0f, t);
        return 1.0f;
    }

private:
    static float heightOf(Phase p) {
        switch (p) {
            case Phase::Hidden:    return 0.0f;
            case Phase::Collapsed: return theme::kCollapsedH;
            case Phase::Expanded:  return theme::kExpandedH;
            case Phase::Alert:     return theme::kAlertH;
        }
        return 0.0f;
    }

    Phase m_phase    = Phase::Hidden;
    Phase m_prevPhase = Phase::Hidden;
    float m_animT    = 0.0f;
    float m_idleTimer = 0.0f;
    float m_blinkT   = 0.0f;
    bool  m_locked   = false;  // 展开锁定（点击后不自动收起）
    IslandData m_data;
};

// ============================================================================
// 渲染器
// ============================================================================

class IslandRenderer {
public:
    /// 绘制岛（x, y 为左上角）
    void draw(ImDrawList* dl, float x, float y, const IslandController& ctrl) {
        const float alpha = ctrl.currentAlpha();
        if (alpha <= 0.01f) return;

        const float w = theme::kWidth;
        const float h = ctrl.currentHeight();
        if (h < 1.0f) return;

        const ImVec2 pmin(x, y);
        const ImVec2 pmax(x + w, y + h);

        // 鼠标检测
        const bool hovered = ImGui::IsMouseHoveringRect(pmin, pmax);
        const bool clicked = hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left);

        // ---- 背景圆角矩形 ----
        ImU32 bg = theme::kBgColor;
        if (ctrl.phase() == Phase::Alert) {
            bg = theme::kBgAlert;
        } else if (hovered) {
            bg = theme::kBgHover;
        }
        // 应用 alpha
        bg = applyAlpha(bg, alpha);

        // 阴影（多层叠加模拟）
        const ImU32 shadow = IM_COL32(0, 0, 0, (int)(80 * alpha));
        dl->AddRectFilled(ImVec2(pmin.x - 6, pmin.y - 6),
                          ImVec2(pmax.x + 6, pmax.y + 6), shadow, theme::kCorner + 6);
        dl->AddRectFilled(ImVec2(pmin.x - 2, pmin.y - 2),
                          ImVec2(pmax.x + 2, pmax.y + 2), shadow, theme::kCorner + 2);

        dl->AddRectFilled(pmin, pmax, bg, theme::kCorner);

        // 告警边框闪烁
        if (ctrl.phase() == Phase::Alert) {
            const ImU32 border = theme::alertBorderColor(ctrl.blinkT());
            dl->AddRect(pmin, pmax, applyAlpha(border, alpha), theme::kCorner, 0, 3.0f);
        }

        // ---- 按状态绘制内容 ----
        dl->PushClipRect(pmin, pmax, true);

        switch (ctrl.phase()) {
            case Phase::Collapsed:
                drawCollapsed(dl, pmin, pmax, ctrl.data(), alpha);
                break;
            case Phase::Expanded:
                drawExpanded(dl, pmin, pmax, ctrl.data(), alpha);
                break;
            case Phase::Alert:
                drawAlert(dl, pmin, pmax, ctrl.data(), ctrl.blinkT(), alpha);
                break;
            case Phase::Hidden:
                break;
        }

        dl->PopClipRect();

        // 返回点击状态（供 main 调用 ctrl.onClick）
        if (clicked) m_clickConsumed = true;
    }

    bool consumeClick() {
        bool c = m_clickConsumed;
        m_clickConsumed = false;
        return c;
    }

private:
    bool m_clickConsumed = false;

    static ImU32 applyAlpha(ImU32 col, float a) {
        const int alpha = static_cast<int>(((col >> 24) & 0xFF) * a);
        return (col & 0x00FFFFFF) | (alpha << 24);
    }

    // ---- COLLAPSED 胶囊 ----
    void drawCollapsed(ImDrawList* dl, const ImVec2& pmin, const ImVec2& pmax,
                       const IslandData& d, float alpha) {
        ImFont* font = ImGui::GetIO().Fonts->Fonts.Size > 0
                           ? ImGui::GetIO().Fonts->Fonts[0]
                           : ImGui::GetFont();
        const float cy = pmin.y + (pmax.y - pmin.y) * 0.5f;
        float cx = pmin.x + theme::kPaddingX;

        // 🐋 logo（用文字代替，后续替换为图片）
        dl->AddText(font, 20.0f, ImVec2(cx, cy - 12),
                    applyAlpha(theme::kTextPrimary, alpha), "\xF0\x9F\x90\x8B");  // 🐋
        cx += 28.0f;

        // 分隔点
        drawDot(dl, cx, cy, applyAlpha(theme::kTextDim, alpha));
        cx += 12.0f;

        // 余额
        const ImU32 balCol = applyAlpha(theme::balanceColor(d.balance_usd), alpha);
        char buf[32];
        snprintf(buf, sizeof(buf), "$%.3f", d.balance_usd);
        dl->AddText(font, 15.0f, ImVec2(cx, cy - 8), balCol, buf);
        cx += ImGui::CalcTextSize(buf).x + 4.0f;
        // 字号差异，用固定宽度估算
        cx += 48.0f;

        drawDot(dl, cx, cy, applyAlpha(theme::kTextDim, alpha));
        cx += 12.0f;

        // 上下文条（5 格）
        const float ratio = (float)d.context_used / d.context_total;
        const float barW = 60.0f;
        const float barH = 6.0f;
        const ImU32 barCol = applyAlpha(theme::contextBarColor(ratio), alpha);
        const ImU32 barBg = applyAlpha(IM_COL32(60, 60, 70, 200), alpha);
        // 背景
        dl->AddRectFilled(ImVec2(cx, cy - barH / 2), ImVec2(cx + barW, cy + barH / 2),
                          barBg, barH / 2);
        // 填充
        dl->AddRectFilled(ImVec2(cx, cy - barH / 2), ImVec2(cx + barW * ratio, cy + barH / 2),
                          barCol, barH / 2);
        cx += barW + 6.0f;

        // 上下文数字
        snprintf(buf, sizeof(buf), "%dk/%dk", d.context_used / 1000, d.context_total / 1000);
        dl->AddText(font, 13.0f, ImVec2(cx, cy - 7),
                    applyAlpha(theme::kTextSecondary, alpha), buf);
        cx += 58.0f;

        drawDot(dl, cx, cy, applyAlpha(theme::kTextDim, alpha));
        cx += 12.0f;

        // 当前活动
        const char* icon = "";
        const char* label = "";
        ImU32 actCol = theme::kTextSecondary;
        switch (d.activity) {
            case ActivityKind::Thinking:
                icon = "\xE2\x97\x8F";  // ●
                snprintf(buf, sizeof(buf), "思考 %.1fs", d.activity_elapsed_sec);
                label = buf;
                actCol = IM_COL32(80, 200, 120, 255);
                break;
            case ActivityKind::ToolRunning:
                icon = "\xE2\x9A\xA1";  // ⚡
                snprintf(buf, sizeof(buf), "%s %.1fs",
                         d.tool_history.empty() ? "Tool" : d.tool_history.back().tool_name.c_str(),
                         d.activity_elapsed_sec);
                label = buf;
                actCol = IM_COL32(160, 140, 250, 255);
                break;
            case ActivityKind::Done:
                icon = "\xE2\x9C\x93";  // ✓
                label = "完成";
                actCol = IM_COL32(80, 200, 120, 255);
                break;
            case ActivityKind::Idle:
                icon = "\xF0\x9F\x92\xA4";  // 💤
                label = "空闲";
                actCol = theme::kTextDim;
                break;
        }
        dl->AddText(font, 14.0f, ImVec2(cx, cy - 8), applyAlpha(actCol, alpha), icon);
        cx += 18.0f;
        dl->AddText(font, 13.0f, ImVec2(cx, cy - 7), applyAlpha(actCol, alpha), label);

        // 右侧：任务耗时
        snprintf(buf, sizeof(buf), "\xE2\x8F\xB1 %ds", (int)d.task_elapsed_sec);  // ⏱
        const float textW = ImGui::CalcTextSize(buf).x;
        dl->AddText(font, 13.0f,
                    ImVec2(pmax.x - theme::kPaddingX - textW - 4, cy - 7),
                    applyAlpha(theme::kTextDim, alpha), buf);
    }

    // ---- EXPANDED 卡片 ----
    void drawExpanded(ImDrawList* dl, const ImVec2& pmin, const ImVec2& pmax,
                      const IslandData& d, float alpha) {
        ImFont* font = ImGui::GetIO().Fonts->Fonts.Size > 0
                           ? ImGui::GetIO().Fonts->Fonts[0]
                           : ImGui::GetFont();
        ImFont* smallFont = ImGui::GetIO().Fonts->Fonts.Size > 1
                                ? ImGui::GetIO().Fonts->Fonts[1]
                                : font;

        float y = pmin.y + theme::kPaddingY;
        const float x0 = pmin.x + theme::kPaddingX;
        const float x1 = pmax.x - theme::kPaddingX;
        char buf[128];

        // ---- 第 1 行：会话信息 + 余额 ----
        // 🐋 模型名
        dl->AddText(font, 15.0f, ImVec2(x0, y),
                    applyAlpha(theme::kTextPrimary, alpha), d.model.c_str());
        // 余额（右对齐）
        snprintf(buf, sizeof(buf), "$%.3f (\xE2\x96\xBC $%.3f \xE6\x9C\xAC\xE4\xBB\xBB\xE5\x8A\xA1)",
                 d.balance_usd, d.task_cost);  // ▼ 本任务
        const ImU32 balCol = applyAlpha(theme::balanceColor(d.balance_usd), alpha);
        const float balW = ImGui::CalcTextSize(buf).x;
        dl->AddText(font, 14.0f, ImVec2(x1 - balW, y + 1), balCol, buf);
        y += 22.0f;

        // ---- 第 2 行：上下文条 + 缓存命中 + 耗时 ----
        const float ratio = (float)d.context_used / d.context_total;
        const float barW = 80.0f;
        const float barH = 5.0f;
        const ImU32 barCol = applyAlpha(theme::contextBarColor(ratio), alpha);
        const ImU32 barBg = applyAlpha(IM_COL32(60, 60, 70, 200), alpha);
        dl->AddRectFilled(ImVec2(x0, y), ImVec2(x0 + barW, y + barH), barBg, barH / 2);
        dl->AddRectFilled(ImVec2(x0, y), ImVec2(x0 + barW * ratio, y + barH), barCol, barH / 2);

        snprintf(buf, sizeof(buf), "%d / %d  \xC2\xB7  \xE7\xBC\x93\xE5\xAD\x98 %d%%  \xC2\xB7  %.1fs",
                 d.context_used, d.context_total, d.cache_hit_rate, d.task_elapsed_sec);
        // " / " · "缓存" · "% · " · "s"
        dl->AddText(smallFont, 12.0f, ImVec2(x0 + barW + 8, y - 2),
                    applyAlpha(theme::kTextSecondary, alpha), buf);
        y += 20.0f;

        // 分隔线
        dl->AddLine(ImVec2(x0, y), ImVec2(x1, y), applyAlpha(IM_COL32(60, 60, 70, 180), alpha));
        y += 8.0f;

        // ---- 第 3 行：当前活动详情 ----
        if (d.activity == ActivityKind::ToolRunning && !d.tool_history.empty()) {
            const auto& tc = d.tool_history.back();
            // ⚡ Bash · cmake --build ...
            snprintf(buf, sizeof(buf), "\xE2\x9A\xA1 %s \xC2\xB7 %s",
                     tc.tool_name.c_str(), tc.arguments.c_str());  // ⚡ ·
            dl->AddText(font, 14.0f, ImVec2(x0, y),
                        applyAlpha(IM_COL32(160, 140, 250, 255), alpha), buf);
            y += 18.0f;
            // ⏳ 运行中...
            dl->AddText(smallFont, 12.0f, ImVec2(x0, y),
                        applyAlpha(theme::kTextDim, alpha),
                        "\xE2\x8F\xB3 \xE8\xBF\x90\xE8\xA1\x8C\xE4\xB8\xAD...");  // ⏳ 运行中...
        } else if (d.activity == ActivityKind::Thinking) {
            snprintf(buf, sizeof(buf), "\xE2\x97\x8F \xE6\x80\x9D\xE8\x80\x83\xE4\xB8\xAD %.1fs",
                     d.activity_elapsed_sec);  // ● 思考中
            dl->AddText(font, 14.0f, ImVec2(x0, y),
                        applyAlpha(IM_COL32(80, 200, 120, 255), alpha), buf);
        }
        y += 24.0f;

        // ---- 工具历史列表 ----
        if (!d.tool_history.empty()) {
            dl->AddText(smallFont, 11.0f, ImVec2(x0, y),
                        applyAlpha(theme::kTextDim, alpha),
                        "\xE5\xB7\xA5\xE5\x85\xB7\xE5\x8E\x86\xE5\x8F\xB2");  // 工具历史
            y += 16.0f;

            int idx = 1;
            for (const auto& tc : d.tool_history) {
                if (y > pmax.y - 20.0f) break;  // 超出裁剪

                const char* statusIcon;
                ImU32 statusCol;
                if (tc.is_running) {
                    statusIcon = "\xE2\x8F\xB3";  // ⏳
                    statusCol = IM_COL32(245, 180, 60, 255);
                } else if (tc.is_error) {
                    statusIcon = "\xE2\x9C\x97";  // ✗
                    statusCol = IM_COL32(235, 80, 75, 255);
                } else {
                    statusIcon = "\xE2\x9C\x93";  // ✓
                    statusCol = IM_COL32(80, 200, 120, 255);
                }

                snprintf(buf, sizeof(buf), "[%d] %s  %s  %s %.0fms",
                         idx, tc.tool_name.c_str(), tc.arguments.c_str(),
                         statusIcon, tc.duration_ms);
                // 截断过长文本
                float maxW = x1 - x0;
                // 简单截断：如果太长就截断
                std::string line = buf;
                if (ImGui::CalcTextSize(line.c_str()).x > maxW) {
                    while (ImGui::CalcTextSize(line.c_str()).x > maxW - 10 && line.size() > 3) {
                        line.pop_back();
                    }
                    line += "...";
                }

                dl->AddText(smallFont, 12.0f, ImVec2(x0, y),
                            applyAlpha(theme::kTextSecondary, alpha), line.c_str());
                y += 16.0f;
                idx++;
            }
        }

        // 底部提示
        const char* hint = "\xE2\x93\xB6 \xE7\x82\xB9\xE5\x87\xBB\xE6\x94\xB6\xE8\xB5\xB7";  // ⓶ 点击收起
        const float hintW = ImGui::CalcTextSize(hint).x;
        dl->AddText(smallFont, 11.0f,
                    ImVec2(x1 - hintW, pmax.y - theme::kPaddingY - 4),
                    applyAlpha(theme::kTextDim, alpha), hint);
    }

    // ---- ALERT 告警 ----
    void drawAlert(ImDrawList* dl, const ImVec2& pmin, const ImVec2& pmax,
                   const IslandData& d, float blinkT, float alpha) {
        ImFont* font = ImGui::GetIO().Fonts->Fonts.Size > 0
                           ? ImGui::GetIO().Fonts->Fonts[0]
                           : ImGui::GetFont();

        const float cy = pmin.y + (pmax.y - pmin.y) * 0.5f;
        const float x0 = pmin.x + theme::kPaddingX;

        // ⚠ 图标
        dl->AddText(font, 20.0f, ImVec2(x0, cy - 12),
                    applyAlpha(IM_COL32(245, 140, 50, 255), alpha),
                    "\xE2\x9A\xA0");  // ⚠
        const float iconW = 28.0f;

        // 告警消息
        dl->AddText(font, 14.0f, ImVec2(x0 + iconW, cy - 8),
                    applyAlpha(theme::kTextPrimary, alpha),
                    d.alert_message.c_str());

        // 右侧：点击确认
        const char* hint = "\xE7\x82\xB9\xE5\x87\xBB\xE7\xA1\xAE\xE8\xAE\xA4";  // 点击确认
        const float hintW = ImGui::CalcTextSize(hint).x;
        dl->AddText(font, 12.0f,
                    ImVec2(pmax.x - theme::kPaddingX - hintW, cy - 6),
                    applyAlpha(theme::kTextDim, alpha), hint);
    }

    // ---- 辅助：分隔点 ----
    void drawDot(ImDrawList* dl, float cx, float cy, ImU32 col) {
        dl->AddCircleFilled(ImVec2(cx, cy), 1.5f, col);
    }
};

}  // namespace whale_dock
