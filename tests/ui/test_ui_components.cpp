#include "test_ui_components.hpp"
#include <imgui.h>
#include <cstring>
#include <algorithm>

namespace DearTs::TestUI {

// ===== TestTitleBar =====

void TestTitleBar::render() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New", "Ctrl+N")) {}
            if (ImGui::MenuItem("Open", "Ctrl+O")) {}
            if (ImGui::MenuItem("Save", "Ctrl+S")) {}
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem("Data Inspector")) {}
            if (ImGui::MenuItem("Logger")) {}
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Theme")) {
            if (ImGui::MenuItem("Dark")) {}
            if (ImGui::MenuItem("Light")) {}
            ImGui::EndMenu();
        }

        ImGui::Separator();

        if (ImGui::Button("Settings##TitleBar")) {
            s_settingsClicked = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Tasks##TitleBar")) {
            s_tasksClicked = true;
        }

        ImGui::EndMainMenuBar();
    }
}

bool TestTitleBar::isSettingsClicked() { return s_settingsClicked; }
bool TestTitleBar::isTasksClicked() { return s_tasksClicked; }
bool TestTitleBar::showFileMenu() { static bool s = false; return s; }
bool TestTitleBar::showViewMenu() { static bool s = false; return s; }
bool TestTitleBar::showThemeMenu() { static bool s = false; return s; }

// ===== TestCommandPalette =====

void TestCommandPalette::open() { s_open = true; s_filterBuffer[0] = '\0'; }
void TestCommandPalette::close() { s_open = false; }
bool TestCommandPalette::isOpen() { return s_open; }
bool TestCommandPalette::isFiltering() { return s_filterBuffer[0] != '\0'; }
void TestCommandPalette::setCommands(const std::vector<std::string>& cmds) { s_commands = cmds; }

void TestCommandPalette::render() {
    if (!s_open) return;

    ImGui::SetNextWindowPos(ImVec2(400, 200), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_Appearing);

    if (ImGui::Begin("Command Palette##Test", &s_open, ImGuiWindowFlags_NoCollapse)) {
        if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere(1);

        bool submitted = ImGui::InputText("##Input", s_filterBuffer, 256, ImGuiInputTextFlags_EnterReturnsTrue);

        ImGui::Separator();

        if (ImGui::BeginChild("##List")) {
            for (size_t i = 0; i < s_commands.size(); i++) {
                if (ImGui::Selectable(s_commands[i].c_str())) {
                    close();
                }
            }
        }
        ImGui::EndChild();

        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) close();
    }
    ImGui::End();
}

// ===== TestToastManager =====

void TestToastManager::show(const Toast& t) { s_toasts.push_back(t); }
void TestToastManager::showInfo(const char* t, const char* m) { show(Toast(t, m, 0)); }
void TestToastManager::showWarning(const char* t, const char* m) { show(Toast(t, m, 1)); }
void TestToastManager::showError(const char* t, const char* m) { show(Toast(t, m, 2)); }
void TestToastManager::showSuccess(const char* t, const char* m) { show(Toast(t, m, 3)); }

void TestToastManager::render() {
    if (s_toasts.empty()) return;

    float x = ImGui::GetIO().DisplaySize.x - 360;
    float y = ImGui::GetIO().DisplaySize.y - 100;

    for (size_t i = 0; i < s_toasts.size(); i++) {
        ImGui::SetNextWindowPos(ImVec2(x, y - i * 90));

        if (ImGui::Begin(s_toasts[i].title.c_str(), nullptr, ImGuiWindowFlags_NoResize)) {
            ImGui::Text("%s", s_toasts[i].message.c_str());
        }
        ImGui::End();
    }
}

// ===== TestView =====

void TestView::render(const char* name, bool& open) {
    if (!open) return;
    if (ImGui::Begin(name, &open)) {
        ImGui::Text("Test View: %s", name);
        if (ImGui::Button("Close")) open = false;
    }
    ImGui::End();
}

void TestView::renderAll() {
    render("Data Inspector", s_states[0].open);
    render("Logger", s_states[1].open);
    render("Theme Manager", s_states[2].open);
}

// ===== TestUIRenderer =====

void TestUIRenderer::renderAll() {
    TestTitleBar::render();

    if (s_testWindow) {
        ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("DearTs Test UI", &s_testWindow)) {
            ImGui::Text("=== Test Components ===");

            ImGui::Checkbox("ImGui Demo", &s_demoWindow);

            ImGui::Separator();
            ImGui::Text("Toasts:");
            if (ImGui::Button("Info Toast")) TestToastManager::showInfo("Info", "Test info message");
            ImGui::SameLine();
            if (ImGui::Button("Warning Toast")) TestToastManager::showWarning("Warning", "Test warning");
            ImGui::SameLine();
            if (ImGui::Button("Clear Toasts")) TestToastManager::clear();

            ImGui::Separator();
            ImGui::Text("Views:");
            static bool views[4] = {false, false, false, false};
            const char* names[] = {"Data Inspector", "Logger", "Theme Manager", "Test"};
            for (int i = 0; i < 4; i++) {
                if (ImGui::Checkbox(names[i], &views[i])) TestView::s_states[i].open = views[i];
            }
        }
        ImGui::End();
    }

    if (s_demoWindow) ImGui::ShowDemoWindow(&s_demoWindow);

    TestView::renderAll();
    TestCommandPalette::render();
    TestToastManager::render();
}

} // namespace DearTs::TestUI
