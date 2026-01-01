// DearTs 视图模板

#pragma once

#include "core/ui/view.h"
#include <imgui.h>
#include <string>

class MyView : public dearts::View {
public:
    // 构造函数
    MyView() = default;
    ~MyView() override = default;

    // 视图名称（必须实现）
    std::string getName() const override {
        return "My View";
    }

    // 渲染内容（必须实现）
    void drawContent() override {
        if (ImGui::Begin("My View")) {
            renderHeader();
            renderBody();
            renderFooter();
        }
        ImGui::End();
    }

    // 可选：自定义窗口标志
    ImGuiWindowFlags getFlags() const override {
        return ImGuiWindowFlags_MenuBar;
    }

    // 可选：是否显示窗口
    bool hasWindow() const override {
        return true;  // 如果返回 false，drawContent 直接渲染
    }

    // 可选：处理事件
    void onEvent(const SDL_Event& event) {
        // 处理 SDL 事件
    }

private:
    void renderHeader() {
        // 菜单栏
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New")) {}
                if (ImGui::MenuItem("Open")) {}
                if (ImGui::MenuItem("Save")) {}
                ImGui::Separator();
                if (ImGui::MenuItem("Exit")) {}
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Edit")) {
                if (ImGui::MenuItem("Undo")) {}
                if (ImGui::MenuItem("Redo")) {}
                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }
    }

    void renderBody() {
        ImGui::Text("View Content");

        // 示例控件
        static int counter = 0;
        if (ImGui::Button("Increment")) {
            counter++;
        }
        ImGui::SameLine();
        ImGui::Text("Counter: %d", counter);

        // 输入框
        static char buf[256] = "";
        ImGui::InputText("Input", buf, sizeof(buf));

        // 复选框
        static bool enabled = true;
        ImGui::Checkbox("Enabled", &enabled);

        // 滑块
        static float value = 0.5f;
        ImGui::SliderFloat("Value", &value, 0.0f, 1.0f);
    }

    void renderFooter() {
        ImGui::Separator();
        ImGui::Text("Status: Ready");
    }
};

// 使用示例：
// 在应用初始化时注册视图：
// ViewManager::instance().addView<MyView>();
