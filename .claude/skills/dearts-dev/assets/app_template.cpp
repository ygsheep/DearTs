// DearTs Framework 应用程序模板（2025 最新版）
// 使用方法：复制此文件并修改 MyApp 类

#include "core/app/application.h"
#include "core/ui/imgui_layer.h"
#include "core/ui/view.h"
#include "core/ui/view_manager.h"
#include "core/ui/command_palette.h"
#include "core/ui/title_bar.h"
#include "core/event/event_bus.h"
#include "core/content/settings.h"
#include <SDL3/SDL.h>
#include <imgui.h>
#include <memory>

// 自定义视图
class MyView : public dearts::View {
public:
    std::string getName() const override {
        return "My View";
    }

    void drawContent() override {
        ImGui::Text("Hello from My View!");
        ImGui::Separator();

        ImGui::Text("FPS: %.2f", ImGui::GetIO().Framerate);
        ImGui::Text("Frame time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);
    }
};

// 主应用程序
class MyApp : public dearts::Application {
public:
    bool onInitialize() override {
        LOG_INFO("MyApp: Initializing");

        // 1. 添加 ImGui Layer
        m_imguiLayer = std::make_shared<dearts::ImGuiLayer>();
        addLayer(m_imguiLayer);

        // 2. 注册视图
        dearts::ViewManager::instance().addView<MyView>();

        // 3. 注册命令
        registerCommands();

        // 4. 设置标题栏按钮
        setupTitleBar();

        // 5. 加载设置
        loadSettings();

        LOG_INFO("MyApp: Initialized successfully");
        return true;
    }

    void onEvent(SDL_Event& event) override {
        // ImGui 处理事件
        m_imguiLayer->onEvent(event);

        // 不要在 ImGui 捕获输入时处理
        if (ImGui::GetIO().WantCaptureMouse ||
            ImGui::GetIO().WantCaptureKeyboard) {
            return;
        }

        // 处理其他事件
        switch (event.type) {
            case SDL_EVENT_QUIT:
                requestShutdown();
                break;

            case SDL_EVENT_KEY_DOWN:
                if (event.key.key == SDLK_ESCAPE) {
                    requestShutdown();
                } else if (event.key.key == SDLK_P) {
                    if (ImGui::GetIO().KeyCtrl) {
                        // Ctrl+P - 打开命令调色板
                        dearts::CommandPalette::instance().open();
                    }
                }
                break;
        }
    }

    void onUpdate(float deltaTime) override {
        // 更新逻辑
    }

    void onRender() override {
        // 创建停靠空间
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

        // 渲染标题栏
        dearts::TitleBar::instance().render();

        // 渲染所有视图
        dearts::ViewManager::instance().render();

        // 渲染命令调色板
        dearts::CommandPalette::instance().render();

        // 演示窗口
        if (ImGui::Begin("My Application")) {
            ImGui::Text("Application State:");
            ImGui::Text("FPS: %.2f", getFPS());
            ImGui::Text("Average FPS: %.2f", getAverageFPS());
            ImGui::Separator();

            // 控件示例
            static bool showDemo = false;
            ImGui::Checkbox("Show ImGui Demo", &showDemo);
            if (showDemo) {
                ImGui::ShowDemoWindow(&showDemo);
            }
        }
        ImGui::End();
    }

    void onShutdown() override {
        LOG_INFO("MyApp: Shutting down");

        // 保存设置
        saveSettings();

        LOG_INFO("MyApp: Shutdown complete");
    }

private:
    void registerCommands() {
        // 注册命令到调色板
        dearts::CommandPalette::instance().addCommand({
            .id = "app.save",
            .name = "App: Save",
            .shortcut = "Ctrl+S",
            .callback = [this]() {
                LOG_INFO("Saving application state");
                saveSettings();
            }
        });

        dearts::CommandPalette::instance().addCommand({
            .id = "app.exit",
            .name = "App: Exit",
            .shortcut = "Alt+F4",
            .callback = [this]() {
                requestShutdown();
            }
        });
    }

    void setupTitleBar() {
        // 添加标题栏按钮
        dearts::TitleBar::instance().addButton({
            .icon = ICON_FA_SAVE,
            .tooltip = "Save (Ctrl+S)",
            .onClick = [this]() {
                saveSettings();
            }
        });

        dearts::TitleBar::instance().addButton({
            .icon = ICON_FA_COG,
            .tooltip = "Settings",
            .onClick = []() {
                dearts::ViewManager::instance().showView("Settings");
            }
        });
    }

    void loadSettings() {
        // 注册默认设置
        dearts::ContentRegistry::Settings::add(
            "app.auto_save",
            "Auto Save",
            true
        );

        dearts::ContentRegistry::Settings::add(
            "app.theme",
            "Theme",
            std::string("Dark")
        );

        // 从文件加载设置
        dearts::ContentRegistry::Settings::load("settings.json");
    }

    void saveSettings() {
        // 保存设置到文件
        dearts::ContentRegistry::Settings::save("settings.json");
    }

    std::shared_ptr<dearts::ImGuiLayer> m_imguiLayer;
};

int main(int argc, char* argv[]) {
    // 初始化日志系统
    Logger::init("logs/app.log", Logger::Level::Info);
    LOG_INFO("Application starting");

    // 创建并运行应用
    MyApp app;
    int result = app.run("My Application", 1280, 720);

    // 关闭日志系统
    Logger::shutdown();

    return result;
}
