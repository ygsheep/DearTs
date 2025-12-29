/**
 * @file toast_view.hpp
 * @brief Toast Notification 视图组件
 * @details 提供一个测试 Toast 功能的 UI 视图
 */

#pragma once

#include "core/ui/view.h"
#include "toast_manager.hpp"
#include <string>

namespace DearTs::Plugins::Toast {

/**
 * @brief Toast 测试视图
 *
 * 提供一个 UI 界面来测试各种 Toast 消息
 */
class ToastView : public Core::UI::ViewWindow {
public:
    explicit ToastView()
        : ViewWindow("通知测试器") {
    }

    ~ToastView() override = default;

    /**
     * @brief 绘制视图内容
     */
    void draw_content() override {
        ImGui::Text("Toast Notification 测试面板");
        ImGui::Separator();

        // 标题输入
        ImGui::Text("标题:");
        ImGui::InputText("##Title", m_title_buffer, IM_ARRAYSIZE(m_title_buffer));

        // 消息输入
        ImGui::Text("消息:");
        ImGui::InputTextMultiline("##Message", m_message_buffer,
            IM_ARRAYSIZE(m_message_buffer), ImVec2(-1, ImGui::GetTextLineHeight() * 4));

        // 时长设置
        ImGui::Text("显示时长（毫秒）:");
        ImGui::SliderInt("##Duration", &m_duration, 1000, 10000);

        // 显示各种类型的 Toast
        ImGui::Spacing();
        ImGui::Text("显示 Toast:");

        if (ImGui::Button("信息")) {
            show_test_toast(ToastType::Info);
        }
        ImGui::SameLine();

        if (ImGui::Button("成功")) {
            show_test_toast(ToastType::Success);
        }
        ImGui::SameLine();

        if (ImGui::Button("警告")) {
            show_test_toast(ToastType::Warning);
        }
        ImGui::SameLine();

        if (ImGui::Button("错误")) {
            show_test_toast(ToastType::Error);
        }

        ImGui::Spacing();

        // 预设消息
        ImGui::Text("预设消息:");
        if (ImGui::Button("文件保存成功")) {
            ToastManager::instance().success(
                "保存成功",
                "文件已成功保存到磁盘"
            );
        }
        ImGui::SameLine();

        if (ImGui::Button("网络错误")) {
            ToastManager::instance().error(
                "网络错误",
                "无法连接到服务器，请检查网络连接"
            );
        }

        ImGui::Spacing();

        // 控制按钮
        if (ImGui::Button("关闭所有 Toast")) {
            ToastManager::instance().close_all();
        }

        // 配置选项
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("配置:");

        auto& config = ToastManager::instance().get_config();

        ImGui::Checkbox("显示进度条", &config.show_progress_bar);
        ImGui::Checkbox("显示关闭按钮", &config.show_close_button);
        ImGui::Checkbox("悬停暂停", &config.pause_on_hover);
        ImGui::Checkbox("点击关闭", &config.click_to_close);

        ImGui::Spacing();

        ImGui::SliderInt("最大数量", &config.max_toasts, 1, 10);
        double anim_speed_min = 1.0;
        double anim_speed_max = 10.0;
        ImGui::SliderScalar("动画速度", ImGuiDataType_Double, &config.animation_speed, &anim_speed_min, &anim_speed_max);

        // 统计信息
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("统计:");
        ImGui::BulletText("当前 Toast 数量: %zu", ToastManager::instance().get_count());
    }

    /**
     * @brief 获取最小窗口大小
     */
    ImVec2 get_min_size() const override {
        return ImVec2(400, 500);
    }

private:
    void show_test_toast(ToastType type) {
        std::string title = m_title_buffer[0] ? m_title_buffer : "测试标题";
        std::string message = m_message_buffer[0] ? m_message_buffer : "这是一条测试消息";

        ToastManager::instance().show(
            title,
            message,
            type,
            std::chrono::milliseconds(m_duration)
        );
    }

private:
    char m_title_buffer[256] = "测试标题";
    char m_message_buffer[512] = "这是一条测试消息内容";
    int m_duration = 3000;
};

} // namespace DearTs::Plugins::Toast
