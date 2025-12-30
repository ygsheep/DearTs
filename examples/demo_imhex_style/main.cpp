/**
 * @file demo_new_architecture.cpp
 * @brief 展示新架构特性的演示程序
 * @details 这个示例演示了所有新架构特性的使用方法
 * @author DearTs Team
 * @date 2025
 * @version 1.0.0
 */

#include "core/app/application.h"
#include "core/event/event_bus.h"
#include "core/content/settings.h"
#include "core/content/commands.h"
#include "core/content/tools.h"
#include "core/content/callbacks.h"
#include "core/content/project_manager.h"
#include "core/plugin/plugin.h"
#include "core/config/config_manager.h"
// CommandPalette 已移至 plugins/command_palette，示例中不再需要
// #include "core/ui/command_palette.h"
#include "core/ui/title_bar.h"
#include "core/ui/theme_manager.h"
#include "core/ui/shortcut_manager.h"
#include "core/ui/task_widget.h"
#include "core/ui/view_manager.h"
#include "core/ui/layout_manager.h"
#include "core/ui/icon_font.hpp"
#include "core/tasks/task_manager.h"
#include "logger.h"
#include "imgui.h"
#include "core/ui/imgui_layer.h"
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <optional>

using namespace DearTs::Core;
using namespace DearTs::Core::ContentRegistry;
using namespace DearTs::Core::Plugin;
using namespace DearTs::Core::Tasks;

// ================ 事件定义 ================

/**
 * @brief 应用程序启动完成事件
 */
struct ApplicationStartupEvent {
    double startup_time_ms;
};

/**
 * @brief 文件加载事件
 */
struct FileLoadEvent {
    std::string filepath;
    size_t file_size;
    bool success;
};

/**
 * @brief 配置变更事件
 */
struct ConfigChangeEvent {
    std::string key;
    std::string old_value;
    std::string new_value;
};

// ================ 示例插件 ================

/**
 * @brief 示例插件：Hello World
 */
class HelloWorldPlugin : public Plugin::IPlugin {
public:
    PluginInfo get_info() const override {
        return {
            .name = "Hello World Plugin",
            .author = "DearTs Team",
            .description = "A simple demonstration plugin",
            .version = "1.0.0",
            .api_version = "1.0.0"
        };
    }

    Result<void, std::string> on_load() override {
        LOG_INFO("HelloWorldPlugin loaded!");

        // 注册命令
        Commands::add("helloworld.say_hello", "Say Hello", [this]() {
            LOG_INFO("Hello from HelloWorldPlugin!");
            m_hello_count++;
        }).shortcut = "Ctrl+H";

        // 注册工具
        Tools::add("helloworld.counter", "Hello Counter", [this]() {
            ImGui::Text("Hello count: %zu", m_hello_count);
        });

        // 订阅事件
        m_event_guard.emplace(Event::make_event_guard<FileLoadEvent>([](const FileLoadEvent& e) {
            if (e.success) {
                LOG_INFO("HelloWorldPlugin: File loaded successfully: {} ({} bytes)",
                         e.filepath, e.file_size);
            }
        }));

        return Result<void, std::string>::ok();
    }

    void on_unload() override {
        LOG_INFO("HelloWorldPlugin unloaded!");
    }

    void on_enable() override {
        LOG_INFO("HelloWorldPlugin enabled!");
    }

    void on_disable() override {
        LOG_INFO("HelloWorldPlugin disabled!");
    }

private:
    size_t m_hello_count = 0;
    std::optional<Event::EventGuard<FileLoadEvent>> m_event_guard;
};

// ================ 示例应用程序 ================

class DemoApplication : public App::Application {
public:
    // ========== 标题栏配置访问器 ==========
    // 通过 ConfigManager 管理标题栏配置
    struct TitleBarConfig {
        // 布局配置
        float frame_padding_x;
        float frame_padding_y;
        float title_left_margin;

        // 按钮配置
        float button_width;
        float button_height;
        float button_spacing;
        float button_right_margin;
        int button_count;

        // 拖动区域配置
        float button_area_width;

        // 从 ConfigManager 加载配置
        void load_from_manager() {
            auto& config = Config::ConfigManager::instance();

            frame_padding_x = config.get_or<double>("ui.titlebar.frame_padding_x", 8.0);
            frame_padding_y = config.get_or<double>("ui.titlebar.frame_padding_y", 10.0);
            title_left_margin = config.get_or<double>("ui.titlebar.title_left_margin", 10.0);

            button_width = config.get_or<double>("ui.titlebar.button_width", 36.0);
            button_height = config.get_or<double>("ui.titlebar.button_height", 30.0);
            button_spacing = config.get_or<double>("ui.titlebar.button_spacing", 3.0);
            button_right_margin = config.get_or<double>("ui.titlebar.button_right_margin", 10.0);
            button_count = config.get_or<int>("ui.titlebar.button_count", 4);

            button_area_width = config.get_or<double>("ui.titlebar.button_area_width", 150.0);
        }

        // 计算属性
        float get_title_bar_height() const {
            return frame_padding_y * 2.0f + 16.0f;
        }

        float get_buttons_total_width() const {
            return button_width * button_count + button_spacing * (button_count - 1);
        }

        float get_buttons_start_x(float window_width) const {
            return window_width - get_buttons_total_width() - button_right_margin;
        }

        bool is_in_button_area(float x, float window_width) const {
            return x >= (window_width - button_area_width);
        }
    };

    DemoApplication() = default;

    ~DemoApplication() override {
        // 清理 ImGui
        ImGui_ImplSDLRenderer3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();

        // 保存配置
        Config::ConfigManager::instance().save_to_file("demo_config.json");
    }

    // 获取窗口（用于 TitleBar 的窗口控制）
    SDL_Window* get_sdl_window() const { return m_window; }

protected:
    bool on_init() override {
        LOG_INFO("DemoApplication initializing...");

        // 1. 初始化 ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;  // 启用停靠功能 (1 << 7)
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // 启用多视口支持 (1 << 10)

        // 加载中文字体
        io.Fonts->Clear(); // 清除默认字体

        // 字体配置
        ImFontConfig font_config;
        font_config.OversampleH = 2;
        font_config.OversampleV = 2;
        font_config.PixelSnapH = true;

        // 尝试加载字体（按优先级）
        static const char* font_paths[] = {
            "resources/fonts/OPPOSans-M.ttf",      // 运行目录
            "resources/fonts/Noto nerd.ttf",       // 运行目录
            "../resources/fonts/OPPOSans-M.ttf",  // IDE 调试目录
            "../../resources/fonts/OPPOSans-M.ttf" // 更深的调试目录
        };

        bool font_loaded = false;
        for (const char* font_path : font_paths) {
            // 加载中文字体，包含完整的汉字字符集
            ImFont* font = io.Fonts->AddFontFromFileTTF(
                font_path,
                16.0f,
                &font_config,
                io.Fonts->GetGlyphRangesChineseFull()
            );

            if (font != nullptr) {
                io.FontDefault = font;
                LOG_INFO("成功加载中文字体: {}", font_path);
                font_loaded = true;

                // 添加更大的字体用于标题
                font_config.MergeMode = false; // 不合并，创建独立的字体
                io.Fonts->AddFontFromFileTTF(font_path, 24.0f, &font_config, io.Fonts->GetGlyphRangesChineseFull());
                break;
            } else {
                LOG_DEBUG("尝试加载字体失败: {}", font_path);
            }
        }

        if (!font_loaded) {
            LOG_WARN("未能加载中文字体，使用默认字体（可能无法显示中文）");
            // 使用默认字体作为后备
            io.Fonts->AddFontDefault();
        }

        // 加载图标字体（Material Symbols）
        LOG_INFO("正在加载图标字体...");
        if (UI::IconFont::loadMaterialSymbols(18.0f)) {
            LOG_INFO("成功加载 Material Symbols 图标字体");
        } else {
            LOG_WARN("图标字体加载失败，部分图标可能显示为 ???");
            LOG_INFO("提示：运行 download_icon_font.bat 下载图标字体");
        }

        // 注意：不要手动调用 io.Fonts->Build()
        // ImGui SDL3 后端会在初始化时自动构建字体纹理

        // 应用默认主题（暗色）
        UI::ThemeManager::instance().setTheme(UI::Theme::Dark);
        UI::ThemeManager::instance().applyImGuiStyle();

        // 初始化 ImGui SDL3 后端
        if (!ImGui_ImplSDL3_InitForSDLRenderer(m_window, m_renderer)) {
            LOG_ERROR("Failed to initialize ImGui SDL3 backend");
            return false;
        }

        // SDL3 不再默认启用文本输入，需要显式启动以接收键盘事件
        SDL_StartTextInput(m_window);
        LOG_INFO("SDL3 text input enabled for keyboard events");

        if (!ImGui_ImplSDLRenderer3_Init(m_renderer)) {
            LOG_ERROR("Failed to initialize ImGui SDL3 renderer");
            return false;
        }

        // 2. 设置配置管理器
        setup_config();

        // 3. 注册事件监听器
        setup_events();

        // 4. 注册命令和工具
        setup_commands_and_tools();

        // 5. 加载插件
        setup_plugins();

        // 6. 设置自定义标题栏
        m_title_bar.set_borderless(true);

        // 设置当前窗口指针（用于窗口控制）
        UI::WindowControls::set_current_window(m_window);

        m_title_bar.add_button("⚙", "设置", [this]() {
            LOG_INFO("Settings button clicked");
            m_show_demo_window = !m_show_demo_window;
        }, ImVec4(0.3f, 0.6f, 0.9f, 1.0f));

        m_title_bar.add_button("ℹ", "关于", [this]() {
            LOG_INFO("About button clicked");
            m_show_about_window = !m_show_about_window;
        }, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));

        // 8. 注册快捷键
        setup_shortcuts();

        // 9. 初始化视图系统
        setup_views();

        // 10. 注册生命周期回调
        Callbacks::add_on_update([this](double delta_time) {
            this->update(delta_time);
        });

        Callbacks::add_on_render([this]() {
            this->render_demo_window();
        });

        // 发布启动完成事件
        Event::EventBus::instance().publish(ApplicationStartupEvent{
            .startup_time_ms = 42.5
        });

        LOG_INFO("DemoApplication initialized successfully!");
        return true;
    }

    void on_shutdown() override {
        LOG_INFO("DemoApplication shutting down...");

        // 清理资源
        // CommandPalette 现在由插件系统管理，无需手动清理

        // 卸载所有插件
        Plugin::PluginManager::instance().clear();

        LOG_INFO("DemoApplication shutdown complete!");
    }

    void on_event(const SDL_Event& event) override {
        // 调试：记录键盘事件
        if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
            LOG_INFO("SDL3 Event: type={}, key={}, scancode={}, mod={}",
                event.type == SDL_EVENT_KEY_DOWN ? "KEY_DOWN" : "KEY_UP",
                static_cast<int>(event.key.key), static_cast<int>(event.key.scancode),
                static_cast<unsigned int>(event.key.mod));
        }

        // 处理 SDL 事件
        ImGui_ImplSDL3_ProcessEvent(&const_cast<SDL_Event&>(event));

        // 调试：ImGui 事件处理后检查状态
        if (event.type == SDL_EVENT_KEY_DOWN) {
            ImGuiIO& io = ImGui::GetIO();
            LOG_INFO("After ImGui process: KeyCtrl={}, KeyShift={}, KeyAlt={}, KeySuper={}",
                io.KeyCtrl, io.KeyShift, io.KeyAlt, io.KeySuper);
        }

        // 处理窗口拖动
        if (m_title_bar.is_borderless()) {
            if (event.type == SDL_EVENT_MOUSE_MOTION) {
                if (m_is_dragging) {
                    // 获取全局鼠标位置（屏幕坐标）
                    float global_x, global_y;
                    SDL_GetGlobalMouseState(&global_x, &global_y);

                    // 计算新窗口位置：当前鼠标位置 - 初始鼠标偏移
                    int new_x = (int)(global_x - m_drag_start_pos.x);
                    int new_y = (int)(global_y - m_drag_start_pos.y);
                    SDL_SetWindowPosition(m_window, new_x, new_y);
                }
            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    // 检查是否在标题栏区域内（使用配置）
                    if (event.button.y < m_title_bar_config.get_title_bar_height()) {
                        // 排除右侧按钮区域（使用配置）
                        if (!m_title_bar_config.is_in_button_area(event.button.x, ImGui::GetMainViewport()->Size.x)) {
                            m_is_dragging = true;

                            // 获取全局鼠标位置和窗口位置
                            float global_mouse_x, global_mouse_y;
                            SDL_GetGlobalMouseState(&global_mouse_x, &global_mouse_y);

                            int window_x, window_y;
                            SDL_GetWindowPosition(m_window, &window_x, &window_y);

                            // 记录鼠标相对于窗口左上角的偏移
                            m_drag_start_pos = ImVec2(
                                global_mouse_x - (float)window_x,
                                global_mouse_y - (float)window_y
                            );
                            m_window_start_pos = ImVec2((float)window_x, (float)window_y);
                        }
                    }
                }
            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    m_is_dragging = false;
                }
            }
        }
    }

    void on_render() override {
        // 清空渲染器
        if (m_renderer) {
            SDL_SetRenderDrawColor(m_renderer, 30, 30, 30, 255);
            SDL_RenderClear(m_renderer);
        }

        // ImGui NewFrame
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // 调试：检查 ImGui IO 状态
        ImGuiIO& io = ImGui::GetIO();
        static int frame_count = 0;
        if (frame_count % 60 == 0) {  // 每 60 帧打印一次，避免日志过多
            LOG_INFO("ImGui IO state: KeyCtrl={}, KeyShift={}, KeyAlt={}, KeySuper={}",
                io.KeyCtrl, io.KeyShift, io.KeyAlt, io.KeySuper);
            LOG_INFO("ImGui IsKeyPressed(A)={}, IsKeyPressed(S)={}",
                ImGui::IsKeyPressed(ImGuiKey_A), ImGui::IsKeyPressed(ImGuiKey_S));
        }
        frame_count++;

        // 处理快捷键（必须在 ImGui::NewFrame() 之后调用，以确保 ImGui 键盘状态已更新）
        // CommandPalette 快捷键现在由插件系统处理
        // 处理其他快捷键
        LOG_DEBUG("About to call handleShortcuts()");
        UI::ShortcutManager::instance().handleShortcuts();
    }

private:
    /**
     * @brief 设置配置管理器
     */
    void setup_config() {
        auto& config = Config::ConfigManager::instance();

        // 尝试加载配置文件
        auto result = config.load_from_file("demo_config.json");
        if (result.isErr()) {
            LOG_WARN("Failed to load config: {}", result.error());
        }

        // 注册配置元数据
        config.register_meta("demo.window.width", {
            .description = "Demo window width",
            .default_value = 1600,
            .is_required = false,
            .validate_callback = [](const Config::ConfigValue& value) {
                if (std::holds_alternative<int>(value)) {
                    int width = std::get<int>(value);
                    if (width < 640 || width > 7680) {
                        return Result<void, std::string>::err(
                            "Window width must be between 640 and 7680"
                        );
                    }
                }
                return Result<void, std::string>::ok();
            }
        });

        config.register_meta("demo.theme", {
            .description = "Application theme",
            .default_value = std::string("dark"),
            .is_required = false,
            .validate_callback = [](const Config::ConfigValue& value) {
                if (std::holds_alternative<std::string>(value)) {
                    std::string theme = std::get<std::string>(value);
                    if (theme != "dark" && theme != "light") {
                        return Result<void, std::string>::err(
                            "Theme must be 'dark' or 'light'"
                        );
                    }
                }
                return Result<void, std::string>::ok();
            },
            .change_callback = [](const Config::ConfigValue& value) {
                if (std::holds_alternative<std::string>(value)) {
                    std::string theme = std::get<std::string>(value);
                    LOG_INFO("Theme changed to: {}", theme);
                    // TODO: 应用主题
                }
            }
        });

        // ========== 标题栏配置 ==========
        config.register_meta("ui.titlebar.frame_padding_x", {
            .description = "Title bar horizontal padding",
            .default_value = 8.0,
            .is_required = false
        });

        config.register_meta("ui.titlebar.frame_padding_y", {
            .description = "Title bar vertical padding (controls height)",
            .default_value = 10.0,
            .is_required = false
        });

        config.register_meta("ui.titlebar.title_left_margin", {
            .description = "Title text left margin",
            .default_value = 10.0,
            .is_required = false
        });

        config.register_meta("ui.titlebar.button_width", {
            .description = "Title bar button width",
            .default_value = 36.0,
            .is_required = false
        });

        config.register_meta("ui.titlebar.button_height", {
            .description = "Title bar button height",
            .default_value = 30.0,
            .is_required = false
        });

        config.register_meta("ui.titlebar.button_spacing", {
            .description = "Title bar button spacing",
            .default_value = 3.0,
            .is_required = false
        });

        config.register_meta("ui.titlebar.button_right_margin", {
            .description = "Title bar buttons right margin",
            .default_value = 10.0,
            .is_required = false
        });

        config.register_meta("ui.titlebar.button_count", {
            .description = "Title bar button count",
            .default_value = 4,
            .is_required = false
        });

        config.register_meta("ui.titlebar.button_area_width", {
            .description = "Title bar button area width (for drag detection)",
            .default_value = 150.0,
            .is_required = false
        });

        // 加载标题栏配置
        m_title_bar_config.load_from_manager();

        // 使用配置作用域
        Config::ConfigScope scope("demo");
        auto width = scope.get_or<int>("window.width", 1600);
        auto height = scope.get_or<int>("window.height", 900);
        auto theme = scope.get_or("theme", std::string("dark"));

        LOG_INFO("Configuration: {}x{}, theme={}", width, height, theme);
    }

    /**
     * @brief 设置事件监听器
     */
    void setup_events() {
        // 监听应用程序启动事件
        m_startup_guard = Event::make_event_guard<ApplicationStartupEvent>(
            [](const ApplicationStartupEvent& e) {
                LOG_INFO("Application started in {:.2f} ms", e.startup_time_ms);
            }
        );

        // 监听文件加载事件
        m_file_load_guard = Event::make_event_guard<FileLoadEvent>(
            [](const FileLoadEvent& e) {
                if (e.success) {
                    LOG_INFO("File loaded: {} ({} bytes)", e.filepath, e.file_size);
                } else {
                    LOG_ERROR("Failed to load file: {}", e.filepath);
                }
            }
        );

        // 监听配置变更事件
        m_config_guard = Event::make_event_guard<ConfigChangeEvent>(
            [](const ConfigChangeEvent& e) {
                LOG_INFO("Config changed: {} = {} (was: {})",
                         e.key, e.new_value, e.old_value);
            }
        );
    }

    /**
     * @brief 设置命令和工具
     */
    void setup_commands_and_tools() {
        // 文件命令
        Commands::add("file.open", "Open File", []() {
            LOG_INFO("Opening file...");
            // TODO: 实现文件打开逻辑
        }).enabled_callback = []() {
            return true;  // 始终可用
        };

        Commands::add("file.save", "Save File", []() {
            LOG_INFO("Saving file...");
            // TODO: 实现文件保存逻辑
            Event::EventBus::instance().publish(FileLoadEvent{
                .filepath = "example.dat",
                .file_size = 1024,
                .success = true
            });
        }).enabled_callback = []() {
            return true;
        };

        Commands::add("file.exit", "Exit Application", [this]() {
            this->request_exit(0);
        });

        // 视图命令
        Commands::add("view.toggle_fullscreen", "Toggle Fullscreen", [this]() {
            // TODO: 实现全屏切换
            LOG_INFO("Toggling fullscreen...");
        });

        // 主题命令
        Commands::add("theme.dark", "Dark Theme", []() {
            UI::ThemeManager::instance().setTheme(UI::Theme::Dark);
        });

        Commands::add("theme.light", "Light Theme", []() {
            UI::ThemeManager::instance().setTheme(UI::Theme::Light);
        });

        Commands::add("theme.classic", "Classic Theme", []() {
            UI::ThemeManager::instance().setTheme(UI::Theme::Classic);
        });

        // 任务命令
        Commands::add("tasks.start_test", "Start Test Task", []() {
            TaskManager::instance().launch("Test Task", [](const std::atomic<bool>& should_cancel) {
                for (int i = 0; i <= 100 && !should_cancel; ++i) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    // 更新进度
                }
            });
        });

        // 项目命令
        Commands::add("project.new", "New Project", []() {
            LOG_INFO("Creating new project...");
            ContentRegistry::ProjectMetadata metadata;
            metadata.name = "New Project";
            metadata.author = "User";
            metadata.description = "A new project";

            auto project = ContentRegistry::ProjectManager::instance().createProject(metadata);
            LOG_INFO("Project created: {}", project->getMetadata().name);
        });

        Commands::add("project.save", "Save Project", []() {
            LOG_INFO("Saving project...");
            auto project = ContentRegistry::ProjectManager::instance().getCurrentProject();
            if (project) {
                ContentRegistry::ProjectManager::instance().saveProject(project, "test_project.json");
            } else {
                LOG_WARN("No project to save");
            }
        });

        Commands::add("project.close", "Close Project", []() {
            LOG_INFO("Closing project...");
            auto project = ContentRegistry::ProjectManager::instance().getCurrentProject();
            if (project) {
                ContentRegistry::ProjectManager::instance().closeProject(project);
            }
        });

        // 工具
        Tools::add("system_info", "System Information", []() {
            ImGui::Text("System Information");
            ImGui::Separator();
            ImGui::Text("Platform: SDL3");
            ImGui::Text("Renderer: SDL Renderer");
            ImGui::Text("DearTsd Version: 1.0.0");
        });

        Tools::add("performance_monitor", "Performance Monitor", [this]() {
            ImGui::Text("Performance");
            ImGui::Separator();
            ImGui::Text("FPS: %.1f", m_current_fps);
            ImGui::Text("Frame Time: %.3f ms", m_delta_time * 1000.0);
        });

        // 设置
        Settings::add("general", "theme", "dark");
        Settings::add("general", "language", "en");
        Settings::add("editor", "font_size", "14");
        Settings::add("editor", "show_line_numbers", "true");
    }

    /**
     * @brief 设置插件
     */
    void setup_plugins() {
        auto& plugin_manager = Plugin::PluginManager::instance();

        // 添加内置插件
        auto hello_world_plugin = std::make_unique<HelloWorldPlugin>();
        auto result = plugin_manager.add_builtin(std::move(hello_world_plugin));

        if (result.isErr()) {
            LOG_ERROR("Failed to load HelloWorldPlugin: {}", result.error());
        } else {
            LOG_INFO("HelloWorldPlugin loaded successfully");
        }

        // 可以从目录加载插件
        // plugin_manager.load_from_directory("plugins");
    }

    /**
     * @brief 设置快捷键
     */
    void setup_shortcuts() {
        using namespace UI;

        auto& manager = ShortcutManager::instance();

        // 文件操作快捷键
        manager.addShortcut("file.open", Shortcut(ImGuiKey_O, true, false, false), []() {
            LOG_INFO("Shortcut: Open File (Ctrl+O)");
            Commands::execute("file.open");
        }, ShortcutType::Global);

        manager.addShortcut("file.save", Shortcut(ImGuiKey_S, true, false, false), []() {
            LOG_INFO("Shortcut: Save File (Ctrl+S)");
            Commands::execute("file.save");
        }, ShortcutType::Global);

        manager.addShortcut("file.exit", Shortcut(ImGuiKey_Q, true, false, false), [this]() {
            LOG_INFO("Shortcut: Exit (Ctrl+Q)");
            this->request_exit(0);
        }, ShortcutType::Global);

        // 视图快捷键
        manager.addShortcut("view.fullscreen", Shortcut(ImGuiKey_F11, false, false, false), [this]() {
            LOG_INFO("Shortcut: Toggle Fullscreen (F11)");
            Commands::execute("view.toggle_fullscreen");
        }, ShortcutType::Global);

        // 命令面板快捷键（除了 Ctrl+P）
        manager.addShortcut("command_palette", Shortcut(ImGuiKey_P, true, false, false), []() {
            LOG_INFO("Shortcut: Command Palette (Ctrl+P)");
            // CommandPalette handles this itself
        }, ShortcutType::Global);

        // 主题快捷键
        manager.addShortcut("theme.dark", Shortcut(ImGuiKey_1, true, false, true), []() {
            LOG_INFO("Shortcut: Dark Theme (Alt+Ctrl+1)");
            ThemeManager::instance().setTheme(Theme::Dark);
        }, ShortcutType::Global);

        manager.addShortcut("theme.light", Shortcut(ImGuiKey_2, true, false, true), []() {
            LOG_INFO("Shortcut: Light Theme (Alt+Ctrl+2)");
            ThemeManager::instance().setTheme(Theme::Light);
        }, ShortcutType::Global);

        manager.addShortcut("theme.classic", Shortcut(ImGuiKey_3, true, false, true), []() {
            LOG_INFO("Shortcut: Classic Theme (Alt+Ctrl+3)");
            ThemeManager::instance().setTheme(Theme::Classic);
        }, ShortcutType::Global);

        // 演示快捷键冲突检测
        manager.addShortcut("test.conflict", Shortcut(ImGuiKey_S, true, false, false), []() {
            LOG_INFO("This shortcut conflicts with file.save");
        }, ShortcutType::Local);

        LOG_INFO("Shortcuts registered: %zu", manager.getBindings().size());
    }

    /**
     * @brief 设置视图系统
     */
    void setup_views() {
        using namespace UI;

        auto& view_manager = ViewManager::instance();

        // 创建几个示例视图来演示窗口布局系统

        // 性能监视视图
        class PerformanceView : public ViewWindow {
        public:
            explicit PerformanceView() : ViewWindow("performance_view", ICON_SPEED) {}

            void draw_content() override {
                // 使用图标字体渲染标题
                ImFont* current_font = ImGui::GetFont();
                if (UI::IconFont::isLoaded()) {
                    ImGui::PushFont(UI::IconFont::getFont());
                }

                ImGui::Text("%s 性能监视器", ICON_SPEED);

                if (UI::IconFont::isLoaded()) {
                    ImGui::PopFont();
                }

                ImGui::Separator();

                // 从主应用获取数据（这里简化处理）
                ImGui::Text("FPS: %.1f", 60.0f); // 示例数据
                ImGui::Text("帧时间: %.3f ms", 16.67f);
                ImGui::Text("内存使用: 128 MB");

                ImGui::Separator();
                // PlotLines 参数: label, values, values_count, values_offset, overlay_text, scale_min, scale_max, graph_size
                ImGui::PlotLines("帧率", m_fps_values, 100, 0, "FPS", 0.0f, 60.0f, ImVec2(0, 80));
            }

        private:
            float m_fps_values[100] = {0};
        };

        // 日志视图（简化版）
        class LogView : public ViewWindow {
        public:
            explicit LogView() : ViewWindow("log_view", ICON_LOGS) {}

            void draw_content() override {
                // 使用图标字体渲染标题
                ImFont* current_font = ImGui::GetFont();
                if (UI::IconFont::isLoaded()) {
                    ImGui::PushFont(UI::IconFont::getFont());
                }

                ImGui::Text("%s 日志查看器", ICON_LOGS);

                if (UI::IconFont::isLoaded()) {
                    ImGui::PopFont();
                }

                ImGui::Separator();

                ImGui::BeginChild("LogScroll", ImVec2(0, -30));
                ImGui::Text("[INFO] 应用程序启动");
                ImGui::Text("[INFO] ImGui 初始化完成");
                ImGui::Text("[WARN] 这是一个警告消息");
                ImGui::Text("[ERROR] 这是一个错误消息（演示）");
                ImGui::EndChild();

                if (ImGui::Button("清空日志")) {
                    // 清空日志逻辑
                }
                ImGui::SameLine();
                if (ImGui::Button("导出日志")) {
                    // 导出日志逻辑
                }
            }
        };

        // 属性视图
        class PropertiesView : public ViewWindow {
        public:
            explicit PropertiesView() : ViewWindow("properties_view", ICON_SETTINGS) {}

            void draw_content() override {
                // 使用图标字体渲染标题
                ImFont* current_font = ImGui::GetFont();
                if (UI::IconFont::isLoaded()) {
                    ImGui::PushFont(UI::IconFont::getFont());
                }

                ImGui::Text("%s 属性面板", ICON_SETTINGS);

                if (UI::IconFont::isLoaded()) {
                    ImGui::PopFont();
                }

                ImGui::Text("名称: DearTsd Demo");
                ImGui::Text("版本: 1.0.0");
                ImGui::Text("作者: DearTs 团队");

                ImGui::Separator();

                if (ImGui::CollapsingHeader("显示设置")) {
                    ImGui::Checkbox("垂直同步", &m_vsync);
                    ImGui::Checkbox("显示 FPS", &m_show_fps);
                    ImGui::SliderFloat("字体大小", &m_font_size, 12.0f, 24.0f);
                }

                if (ImGui::CollapsingHeader("主题设置")) {
                    if (ImGui::Button("暗色主题")) ThemeManager::instance().setTheme(Theme::Dark);
                    ImGui::SameLine();
                    if (ImGui::Button("亮色主题")) ThemeManager::instance().setTheme(Theme::Light);
                }
            }

        private:
            bool m_vsync = true;
            bool m_show_fps = true;
            float m_font_size = 16.0f;
        };

        // 添加视图到管理器
        // 注意：停靠功能会自动处理窗口位置，不需要手动设置初始位置
        auto perf_view = std::make_unique<PerformanceView>();
        perf_view->get_window_open_state() = true;  // 默认打开性能视图
        view_manager.addView(std::move(perf_view));

        auto log_view = std::make_unique<LogView>();
        log_view->get_window_open_state() = true;   // 默认打开日志视图
        view_manager.addView(std::move(log_view));

        auto prop_view = std::make_unique<PropertiesView>();
        prop_view->get_window_open_state() = true;  // 默认打开属性视图
        view_manager.addView(std::move(prop_view));

        LOG_INFO("视图系统初始化完成，已添加 %zu 个视图", view_manager.getAllViews().size());
    }

    /**
     * @brief 更新逻辑
     */
    void update(double delta_time) {
        m_current_fps = 1.0 / delta_time;

        // 更新任务管理器
        TaskManager::instance().update();

        // 模拟一些工作
        static double accumulator = 0.0;
        accumulator += delta_time;
        if (accumulator >= 5.0) {
            LOG_INFO("Application running smoothly...");
            accumulator = 0.0;
        }
    }

    /**
     * @brief 渲染演示窗口
     */
    void render_demo_window() {
        // === 方案：使用 ImHex 的方式 ===
        // 1. 使用 BeginMainMenuBar 绘制自定义标题栏（确保在最顶层）
        // 2. 创建主 DockSpace 窗口，位置从 MenuBar 下方开始
        // 3. 在 DockSpace 中渲染所有视图

        // 步骤1: 渲染自定义标题栏（使用 MenuBar 机制）
        float title_bar_height = 0.0f;  // 保存实际的标题栏高度

        if (m_title_bar.is_borderless()) {
            // 使用配置设置 MenuBar 的内边距
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                ImVec2(m_title_bar_config.frame_padding_x, m_title_bar_config.frame_padding_y));

            if (ImGui::BeginMainMenuBar()) {
                // 获取实际的 MenuBar 高度（在设置 FramePadding 之后）
                title_bar_height = ImGui::GetFrameHeight();

                // 绘制自定义标题栏内容
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 5));

                // 左侧：窗口标题（使用配置）
                ImGui::SetCursorPosX(m_title_bar_config.title_left_margin);
                ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 1.0f), "DearTsd 演示");

                // 右侧：控制按钮（使用配置）
                float start_x = m_title_bar_config.get_buttons_start_x(ImGui::GetWindowWidth());
                ImGui::SameLine(start_x);

                // 使用图标字体（如果已加载）
                if (UI::IconFont::isLoaded()) {
                    ImGui::PushFont(UI::IconFont::getFont());
                }

                // 设置按钮（使用配置）
                if (ImGui::Button(ICON_SETTINGS,
                    ImVec2(m_title_bar_config.button_width, m_title_bar_config.button_height))) {
                    // 设置菜单
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("设置");
                }
                ImGui::SameLine(0, m_title_bar_config.button_spacing);

                // 搜索按钮（使用配置）
                if (ImGui::Button(ICON_SEARCH,
                    ImVec2(m_title_bar_config.button_width, m_title_bar_config.button_height))) {
                    // 搜索
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("搜索");
                }
                ImGui::SameLine(0, m_title_bar_config.button_spacing);

                // 关于按钮（使用配置）
                if (ImGui::Button(ICON_INFO,
                    ImVec2(m_title_bar_config.button_width, m_title_bar_config.button_height))) {
                    // 关于对话框
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("关于");
                }
                ImGui::SameLine(0, m_title_bar_config.button_spacing);

                // 关闭按钮（使用配置）
                if (ImGui::Button(ICON_CLOSE,
                    ImVec2(m_title_bar_config.button_width, m_title_bar_config.button_height))) {
                    this->request_exit(0);
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("关闭");
                }

                // 恢复原来的字体
                if (UI::IconFont::isLoaded()) {
                    ImGui::PopFont();
                }

                ImGui::PopStyleVar(); // ItemSpacing

                ImGui::EndMainMenuBar();
            }

            ImGui::PopStyleVar(); // FramePadding
        }

        // 步骤2: 创建主 DockSpace 窗口（从 MenuBar 下方开始）
        ImGuiViewport* viewport = ImGui::GetMainViewport();

        // 使用保存的实际标题栏高度，而不是重新计算
        // 如果没有标题栏（非无边框模式），高度为 0
        float menuBarHeight = m_title_bar.is_borderless() ? title_bar_height : 0.0f;

        // 设置主 DockSpace 窗口的位置和大小（排除 MenuBar 区域）
        ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + menuBarHeight));
        ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, viewport->Size.y - menuBarHeight));
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar |
                                       ImGuiWindowFlags_NoCollapse |
                                       ImGuiWindowFlags_NoResize |
                                       ImGuiWindowFlags_NoMove |
                                       ImGuiWindowFlags_NoDocking |
                                       ImGuiWindowFlags_NoNavFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

        if (ImGui::Begin("##MainDockSpace", nullptr, windowFlags)) {
            // 创建停靠空间
            ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode);

            // 渲染所有视图
            UI::ViewManager::instance().renderAllViews();
        }

        ImGui::End();

        ImGui::PopStyleVar(3);

        // 渲染主演示窗口
        if (m_show_demo_window) {
            ImGui::SetNextWindowPos(ImVec2(20, 50), ImGuiCond_FirstUseEver);
            ImGui::Begin("新架构演示", &m_show_demo_window);

            ImGui::Text("DearTsd 新架构演示程序");
            ImGui::Separator();

            // 性能信息
            if (ImGui::CollapsingHeader("性能信息", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Text("FPS: %.1f", m_current_fps);
                ImGui::Text("帧时间: %.3f ms", m_delta_time * 1000.0);
            }

            // 命令面板演示
            if (ImGui::CollapsingHeader("命令面板", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Text("按 Ctrl+P 打开命令面板");
                ImGui::Text("可用命令: %zu 个", Commands::get_all().size());
            }

            // 主题系统演示
            if (ImGui::CollapsingHeader("主题", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Text("当前主题: %s", UI::ThemeManager::getThemeName(
                    UI::ThemeManager::instance().getCurrentTheme()));

                ImGui::Separator();

                if (ImGui::Button("暗色主题")) {
                    UI::ThemeManager::instance().setTheme(UI::Theme::Dark);
                }
                ImGui::SameLine();
                if (ImGui::Button("亮色主题")) {
                    UI::ThemeManager::instance().setTheme(UI::Theme::Light);
                }
                ImGui::SameLine();
                if (ImGui::Button("经典主题")) {
                    UI::ThemeManager::instance().setTheme(UI::Theme::Classic);
                }

                ImGui::Separator();
                ImGui::Text("主题颜色:");
                auto bg_color = UI::ThemeManager::instance().getColor("Background");
                auto text_color = UI::ThemeManager::instance().getColor("Text");
                ImGui::ColorButton("背景色", bg_color);
                ImGui::SameLine();
                ImGui::ColorButton("文本色", text_color);
            }

            // 配置演示
            if (ImGui::CollapsingHeader("配置", ImGuiTreeNodeFlags_DefaultOpen)) {
                Config::ConfigScope scope("demo");
                auto width = scope.get_or<int>("window.width", 1600);
                auto height = scope.get_or<int>("window.height", 900);
                auto theme = scope.get_or("theme", std::string("dark"));

                ImGui::Text("窗口大小: %dx%d", width, height);
                ImGui::Text("主题: %s", theme.c_str());

                if (ImGui::Button("切换主题")) {
                    static bool is_dark = true;
                    is_dark = !is_dark;
                    scope.set("theme", is_dark ? "dark" : "light");
                }
            }

            // 插件信息
            if (ImGui::CollapsingHeader("插件", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto plugin_infos = Plugin::PluginManager::instance().get_all_plugins_info();
                ImGui::Text("已加载插件: %zu 个", plugin_infos.size());

                for (const auto& info : plugin_infos) {
                    ImGui::BulletText("%s v%s 作者: %s",
                                     info.name.c_str(),
                                     info.version.c_str(),
                                     info.author.c_str());
                }
            }

            // 任务系统演示
            if (ImGui::CollapsingHeader("任务系统", ImGuiTreeNodeFlags_DefaultOpen)) {
                size_t running_count = TaskManager::instance().getRunningTaskCount();
                ImGui::Text("运行中的任务: %zu 个", running_count);

                ImGui::Separator();

                if (ImGui::Button("启动快速任务")) {
                    TaskManager::instance().launch("快速任务", [](const std::atomic<bool>& should_cancel) {
                        for (int i = 0; i <= 10 && !should_cancel; ++i) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        }
                    });
                }

                ImGui::SameLine();
                if (ImGui::Button("启动长时间任务")) {
                    TaskManager::instance().launch("长时间任务", [](const std::atomic<bool>& should_cancel) {
                        for (int i = 0; i <= 100 && !should_cancel; ++i) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        }
                    });
                }

                ImGui::SameLine();
                if (ImGui::Button("取消所有任务")) {
                    TaskManager::instance().cancelAll();
                }

                // 显示运行中的任务
                auto running_tasks = TaskManager::instance().getRunningTasks();
                if (!running_tasks.empty()) {
                    ImGui::Separator();
                    ImGui::Text("活动任务:");
                    for (const auto& task : running_tasks) {
                        UI::TaskWidget::renderTask(task);
                    }
                }
            }

            // 快捷键系统演示
            if (ImGui::CollapsingHeader("快捷键", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Text("已注册快捷键: %zu 个", UI::ShortcutManager::instance().getBindings().size());

                ImGui::Separator();
                ImGui::Text("常用快捷键:");
                ImGui::BulletText("Ctrl+O - 打开文件");
                ImGui::BulletText("Ctrl+S - 保存文件");
                ImGui::BulletText("Ctrl+Q - 退出");
                ImGui::BulletText("F11 - 全屏切换");
                ImGui::BulletText("Ctrl+P - 命令面板");
                ImGui::BulletText("Ctrl+Alt+1/2/3 - 切换主题");

                ImGui::Separator();
                ImGui::Text("试试按下这些按键来测试快捷键!");

                // 显示快捷键表格
                if (ImGui::CollapsingHeader("所有快捷键")) {
                    UI::ShortcutManager::renderSettings();
                }
            }

            // 项目管理演示
            if (ImGui::CollapsingHeader("项目管理", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto current_project = ContentRegistry::ProjectManager::instance().getCurrentProject();

                if (current_project) {
                    ImGui::Text("当前项目: %s", current_project->getMetadata().name.c_str());

                    const char* state_text = "未知";
                    switch (current_project->getState()) {
                        case ContentRegistry::ProjectState::Closed:   state_text = "已关闭"; break;
                        case ContentRegistry::ProjectState::Opened:   state_text = "已打开"; break;
                        case ContentRegistry::ProjectState::Modified: state_text = "已修改"; break;
                        case ContentRegistry::ProjectState::Saving:   state_text = "保存中"; break;
                        case ContentRegistry::ProjectState::Loading:  state_text = "加载中"; break;
                    }
                    ImGui::SameLine();
                    ImGui::Text("[%s]", state_text);

                    ImGui::Separator();
                    ImGui::Text("作者: %s", current_project->getMetadata().author.c_str());
                    ImGui::Text("描述: %s", current_project->getMetadata().description.c_str());
                    ImGui::Text("创建日期: %s", current_project->getMetadata().creation_date.c_str());
                    ImGui::Text("修改日期: %s", current_project->getMetadata().last_modified.c_str());
                    ImGui::Text("文件数: %zu 个", current_project->getFiles().size());

                    ImGui::Separator();
                    if (ImGui::Button("保存项目")) {
                        ContentRegistry::ProjectManager::instance().saveProject(current_project, "test_project.json");
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("关闭项目")) {
                        ContentRegistry::ProjectManager::instance().closeProject(current_project);
                    }

                    // 添加测试文件
                    static char test_file_path[256] = "test.dat";
                    ImGui::InputText("文件路径", test_file_path, sizeof(test_file_path));
                    ImGui::SameLine();
                    if (ImGui::Button("添加文件")) {
                        ContentRegistry::ProjectFile file;
                        file.path = test_file_path;
                        file.type = "binary";
                        file.size = 1024;
                        file.readonly = false;
                        current_project->addFile(file);
                    }
                } else {
                    ImGui::Text("未打开项目");
                    ImGui::Separator();

                    if (ImGui::Button("新建项目")) {
                        Commands::execute("project.new");
                    }

                    if (ImGui::Button("打开项目")) {
                        Commands::execute("file.open");
                    }
                }

                ImGui::Separator();
                ImGui::Text("最近项目: %zu 个", ContentRegistry::ProjectManager::instance().getRecentProjects().size());
                auto recent = ContentRegistry::ProjectManager::instance().getRecentProjects();
                for (const auto& path : recent) {
                    ImGui::BulletText("%s", path.c_str());
                }
            }

            // 事件测试
            if (ImGui::CollapsingHeader("事件", ImGuiTreeNodeFlags_DefaultOpen)) {
                if (ImGui::Button("触发测试事件")) {
                    Event::EventBus::instance().publish(FileLoadEvent{
                        .filepath = "test.dat",
                        .file_size = 2048,
                        .success = true
                    });
                }

                if (ImGui::Button("触发配置变更事件")) {
                    Event::EventBus::instance().publish(ConfigChangeEvent{
                        .key = "test.key",
                        .old_value = "old",
                        .new_value = "new"
                    });
                }
            }

            ImGui::End();
        }

        // 渲染关于窗口
        if (m_show_about_window) {
            ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
            ImGui::Begin("关于 DearTsd", &m_show_about_window);
            ImGui::Text("DearTsd 框架");
            ImGui::Separator();
            ImGui::Text("现代化的 C++20 应用程序框架");
            ImGui::Text("基于 SDL3 + ImGui");
            ImGui::Text("");
            ImGui::Text("版本: %s", m_config.version.c_str());
            ImGui::Text("作者: DearTs 团队");
            ImGui::End();
        }

        // CommandPalette 现在由插件系统自动渲染
        ImGui::Render();
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), m_renderer);
    }

private:
    // CommandPalette 已移至插件系统，不再需要成员变量
    UI::TitleBar m_title_bar;

    // 标题栏配置（从 ConfigManager 加载）
    TitleBarConfig m_title_bar_config;

    // 事件守卫（使用 optional 以支持默认构造）
    std::optional<Event::EventGuard<ApplicationStartupEvent>> m_startup_guard;
    std::optional<Event::EventGuard<FileLoadEvent>> m_file_load_guard;
    std::optional<Event::EventGuard<ConfigChangeEvent>> m_config_guard;

    // 状态
    bool m_show_demo_window = true;
    bool m_show_about_window = false;
    double m_current_fps = 0.0;

    // 窗口拖动状态
    bool m_is_dragging = false;
    ImVec2 m_drag_start_pos;
    ImVec2 m_window_start_pos;
};

// ================ 主函数 ================

int main(int argc, char* argv[]) {
    LOG_INFO("DearTsd 新架构演示程序启动中...");

    // 创建并运行应用程序
    DemoApplication app;

    App::ApplicationConfig config;
    config.name = "DearTsd 新架构演示";
    config.version = "1.0.0";
    config.window_width = 1600;
    config.window_height = 900;
    config.enable_vsync = true;
    config.enable_imgui = true;
    config.borderless = true;  // 启用无边框窗口（自定义标题栏）

    if (!app.initialize(config)) {
        LOG_ERROR("应用程序初始化失败!");
        return 1;
    }

    int exit_code = app.run();
    app.shutdown();

    LOG_INFO("DearTsd 新架构演示程序退出，退出码: {}", exit_code);
    return exit_code;
}
