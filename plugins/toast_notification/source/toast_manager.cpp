/**
 * @file toast_manager.cpp
 * @brief Toast Manager 实现
 */

#include "toast_manager.hpp"
#include "core/ui/icon_font.hpp"
#include <algorithm>
#include <imgui.h>

namespace DearTs::Plugins::Toast {

// ================ ToastManager 实现 ================

int ToastManager::show(
    const std::string& title,
    const std::string& message,
    ToastType type,
    std::chrono::milliseconds duration
) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // 检查是否超过最大数量
    if (m_toasts.size() >= static_cast<size_t>(m_config.max_toasts)) {
        // 移除最旧的 Toast
        if (!m_toasts.empty()) {
            m_toasts.front().is_exiting = true;
        }
    }

    // 创建新 Toast
    ToastMessage toast(title, message, type, duration);
    toast.id = generate_id();

    // Save id before moving
    int toast_id = toast.id;
    m_toasts.push_back(std::move(toast));

    return toast_id;
}

int ToastManager::info(const std::string& title, const std::string& message) {
    return show(title, message, ToastType::Info);
}

int ToastManager::success(const std::string& title, const std::string& message) {
    return show(title, message, ToastType::Success);
}

int ToastManager::warning(const std::string& title, const std::string& message) {
    return show(title, message, ToastType::Warning);
}

int ToastManager::error(const std::string& title, const std::string& message) {
    return show(title, message, ToastType::Error);
}

void ToastManager::close(int id) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = std::find_if(m_toasts.begin(), m_toasts.end(),
        [id](const ToastMessage& t) { return t.id == id; });

    if (it != m_toasts.end()) {
        it->is_exiting = true;
    }
}

void ToastManager::close_all() {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& toast : m_toasts) {
        toast.is_exiting = true;
    }
}

void ToastManager::update(float delta_time) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // 安全检查：确保动画时长配置有效
    if (m_config.enter_duration <= 0.0) {
        printf("WARNING: enter_duration is %.6f, resetting to 0.5\n", m_config.enter_duration);
        m_config.enter_duration = 0.5;
    }
    if (m_config.exit_duration <= 0.0) {
        printf("WARNING: exit_duration is %.6f, resetting to 0.3\n", m_config.exit_duration);
        m_config.exit_duration = 0.3;
    }

    for (auto& toast : m_toasts) {
        // 更新动画
        if (toast.is_entering) {
            toast.animation_progress += delta_time / static_cast<float>(m_config.enter_duration);
            if (toast.animation_progress >= 1.0f) {
                toast.animation_progress = 1.0f;
                toast.is_entering = false;
            }
        } else if (toast.is_exiting) {
            toast.animation_progress -= delta_time / static_cast<float>(m_config.exit_duration);
            if (toast.animation_progress <= 0.0f) {
                toast.animation_progress = 0.0f;
            }
        }

        // 更新透明度
        toast.alpha = ease_out_cubic(toast.animation_progress);

        // 处理悬停暂停逻辑
        if (m_config.pause_on_hover) {
            // 检测悬停状态变化：从未悬停变为悬停
            if (toast.is_hovered && !toast.was_hovered) {
                // 刚开始悬停，保存剩余时间
                toast.hover_remaining_time = std::chrono::milliseconds(toast.get_remaining_time_ms());
            }
            // 悬停期间，每帧延长过期时间
            else if (toast.is_hovered && toast.was_hovered) {
                toast.expire_at = std::chrono::steady_clock::now() + toast.hover_remaining_time;
            }

            // 更新悬停状态追踪
            toast.was_hovered = toast.is_hovered;
        }
    }

    // 清理已过期或退出动画完成的 Toast
    cleanup_expired();
}

void ToastManager::render() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_toasts.empty()) {
        return;
    }

    // 获取 ImGui 上下文
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    if (!viewport) return;

    // Toast 窗口宽度和边距
    float toast_width = static_cast<float>(m_config.max_width);
    float edge_margin = 20.0f;
    float work_width = viewport->WorkSize.x;
    float work_height = viewport->WorkSize.y;

    // 根据位置配置计算起始坐标和堆叠方向
    ToastPosition pos_enum = static_cast<ToastPosition>(m_config.position);
    float x, y;
    float stack_direction_y = 1.0f;  // 1.0 = 从上往下，-1.0 = 从下往上

    switch (pos_enum) {
        case ToastPosition::TopLeft:
            x = viewport->WorkPos.x + edge_margin;
            y = viewport->WorkPos.y + edge_margin;
            stack_direction_y = 1.0f;
            break;
        case ToastPosition::TopCenter:
            x = viewport->WorkPos.x + (work_width - toast_width) / 2.0f;
            y = viewport->WorkPos.y + edge_margin;
            stack_direction_y = 1.0f;
            break;
        case ToastPosition::TopRight:
            x = viewport->WorkPos.x + work_width - toast_width - edge_margin;
            y = viewport->WorkPos.y + edge_margin;
            stack_direction_y = 1.0f;
            break;
        case ToastPosition::BottomLeft:
            x = viewport->WorkPos.x + edge_margin;
            y = viewport->WorkPos.y + work_height - edge_margin;
            stack_direction_y = -1.0f;
            break;
        case ToastPosition::BottomCenter:
            x = viewport->WorkPos.x + (work_width - toast_width) / 2.0f;
            y = viewport->WorkPos.y + work_height - edge_margin;
            stack_direction_y = -1.0f;
            break;
        case ToastPosition::BottomRight:
            x = viewport->WorkPos.x + work_width - toast_width - edge_margin;
            y = viewport->WorkPos.y + work_height - edge_margin;
            stack_direction_y = -1.0f;
            break;
        default:
            x = viewport->WorkPos.x + work_width - toast_width - edge_margin;
            y = viewport->WorkPos.y + edge_margin;
            stack_direction_y = 1.0f;
            break;
    }

    // 渲染每个 Toast
    for (auto& toast : m_toasts) {
        if (toast.animation_progress > 0.0f) {
            ImVec2 pos(x, y);
            draw_animated_toast(toast, pos);

            // 根据堆叠方向更新 y 坐标
            // 对于从下往上的布局，需要先减去当前 Toast 的高度
            if (stack_direction_y < 0.0f) {
                y -= (toast.size.y + static_cast<float>(m_config.spacing));
            } else {
                y += toast.size.y + static_cast<float>(m_config.spacing);
            }
        }
    }
}

void ToastManager::cleanup_expired() {
    auto it = std::remove_if(m_toasts.begin(), m_toasts.end(),
        [](const ToastMessage& t) {
            return t.is_exiting && t.animation_progress <= 0.0f;
        });

    m_toasts.erase(it, m_toasts.end());

    // 同时清理已过期且未悬停的 Toast
    it = std::remove_if(m_toasts.begin(), m_toasts.end(),
        [](const ToastMessage& t) {
            return t.is_expired() && !t.is_exiting && !t.is_hovered;
        });

    // 标记这些 Toast 为退出状态
    for (auto i = m_toasts.begin(); i != it; ++i) {
        if (i->is_expired() && !i->is_exiting) {
            i->is_exiting = true;
        }
    }
}

void ToastManager::remove(int id) {
    auto it = std::remove_if(m_toasts.begin(), m_toasts.end(),
        [id](const ToastMessage& t) { return t.id == id; });

    m_toasts.erase(it, m_toasts.end());
}

int ToastManager::generate_id() {
    return m_next_id++;
}

void ToastManager::render_toast(ToastMessage& toast, const ImVec2& position) {
    // 设置固定的窗口大小，确保有足够的宽度显示内容
    float window_width = static_cast<float>(m_config.max_width);

    // 设置窗口位置和大小
    ImGui::SetNextWindowPos(position, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(window_width, 0), ImGuiCond_Always);

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar |
                                   ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoCollapse |
                                   ImGuiWindowFlags_NoScrollbar |
                                   ImGuiWindowFlags_NoScrollWithMouse |
                                   ImGuiWindowFlags_NoNav |
                                   ImGuiWindowFlags_NoFocusOnAppearing;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(static_cast<float>(m_config.padding_x), static_cast<float>(m_config.padding_y)));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 8.0f);

    // 设置样式
    ImVec4 bg_color = ImVec4(0.13f, 0.13f, 0.13f, 0.95f * toast.alpha);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, bg_color);
    ImGui::PushStyleColor(ImGuiCol_Border, toast.get_type_color());

    std::string window_name = "Toast_" + std::to_string(toast.id);
    if (ImGui::Begin(window_name.c_str(), nullptr, window_flags)) {
        draw_toast_content(toast);
        toast.size = ImGui::GetWindowSize();
    }
    ImGui::End();

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);
}

void ToastManager::draw_animated_toast(ToastMessage& toast, const ImVec2& position) {
    // 从下往上的滑动动画 + 淡入效果
    // 增加滑动距离和动画时长，让效果更明显
    float slide_offset = (1.0f - toast.animation_progress) * 50.0f; // 从下方 50px 滑入（更明显）
    ImVec2 anim_pos(position.x, position.y + slide_offset);

    // 应用透明度和位置
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, toast.alpha);
    render_toast(toast, anim_pos);
    ImGui::PopStyleVar();
}

void ToastManager::draw_toast_content(ToastMessage& toast) {
    ImGui::PushID(toast.id);

    // 获取颜色
    ImVec4 type_color = toast.get_type_color();
    const char* icon = toast.get_type_icon();

    // 渲染图标 - 如果 IconFont 已加载，使用它；否则使用默认字体
    if (DearTs::Core::UI::IconFont::isLoaded()) {
        ImGui::PushFont(DearTs::Core::UI::IconFont::getFont());
        ImGui::PushStyleColor(ImGuiCol_Text, type_color);
        ImGui::Text("%s", icon);
        ImGui::PopStyleColor();
        ImGui::PopFont();
    } else {
        // IconFont 未加载，使用默认字体（可能会显示为方块）
        ImGui::PushStyleColor(ImGuiCol_Text, type_color);
        ImGui::Text("%s", icon);
        ImGui::PopStyleColor();
    }

    ImGui::SameLine(0, 8.0f);  // 图标和标题之间添加 8px 间距

    ImGui::TextColored(type_color, "%s", toast.title.c_str());

    // 关闭按钮
    if (m_config.show_close_button) {
        ImGui::SameLine(ImGui::GetWindowWidth() - 30);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.5f, 0.5f, 0.3f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.5f, 0.5f, 0.5f));

        if (ImGui::Button("✕")) {
            toast.is_exiting = true;
        }

        ImGui::PopStyleColor(3);

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("关闭");
        }
    }

    ImGui::Separator();

    // 消息内容 - 使用窗口宽度进行文本换行
    float content_width = ImGui::GetContentRegionAvail().x;
    ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + content_width);
    ImGui::TextWrapped("%s", toast.message.c_str());
    ImGui::PopTextWrapPos();

    // 进度条
    if (m_config.show_progress_bar && !toast.is_exiting) {
        ImGui::Spacing();
        float progress = toast.get_remaining_progress();
        ImGui::ProgressBar(progress, ImVec2(-1, 3), "");
    }

    // 检测悬停
    toast.is_hovered = ImGui::IsWindowHovered();

    // 点击关闭
    if (m_config.click_to_close && ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0)) {
        toast.is_exiting = true;
    }

    ImGui::PopID();
}

float ToastManager::ease_out_cubic(float x) {
    return 1.0f - std::pow(1.0f - x, 3.0f);
}

float ToastManager::ease_out_back(float x) {
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    return 1.0f + c3 * std::pow(x - 1.0f, 3.0f) + c1 * std::pow(x - 1.0f, 2.0f);
}

} // namespace DearTs::Plugins::Toast
