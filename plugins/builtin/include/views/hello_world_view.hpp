/**
 * @file hello_world_view.hpp
 * @brief 示例：Hello World 视图
 * @details 展示如何创建一个简单的 UI 插件视图
 */

#pragma once

#include "core/ui/view.h"
#include <string>

namespace DearTs::Plugins::Builtin {

/**
 * @brief Hello World 视图
 *
 * 这是一个最简单的视图示例，展示如何创建自定义 UI
 */
class HelloWorldView : public Core::UI::ViewWindow {
public:
    explicit HelloWorldView()
        : ViewWindow("你好世界") {
    }

    ~HelloWorldView() override = default;

    /**
     * @brief 绘制视图内容
     */
    void draw_content() override {
        ImGui::Text("Hello from plugin!");

        ImGui::Separator();

        ImGui::Text("这是一个插件视图示例。");
        ImGui::Text("视图通过插件系统加载。");

        ImGui::Spacing();

        // 简单的交互
        if (ImGui::Button("点击我")) {
            m_click_count++;
            m_show_message = true;
        }

        ImGui::SameLine();
        ImGui::Text("点击次数: %d", m_click_count);

        // 显示消息
        if (m_show_message) {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "✓ 按钮被点击了！");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("这个消息会自动消失");
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("插件信息:");
        ImGui::BulletText("名称: Hello World Plugin");
        ImGui::BulletText("类型: UI View");
        ImGui::BulletText("作者: DearTs Team");
        ImGui::BulletText("版本: 1.0.0");
    }

    /**
     * @brief 获取最小窗口大小
     */
    ImVec2 get_min_size() const override {
        return ImVec2(400, 300);
    }

private:
    int m_click_count = 0;
    bool m_show_message = false;
};

} // namespace DearTs::Plugins::Builtin
