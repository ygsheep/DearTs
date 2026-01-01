/**
 * @file toolbox_application.cpp
 * @brief 工具箱应用程序实现
 */

#include "dearts_application.hpp"
#include "toast_manager.hpp"
#include "toast_plugin.hpp"
#include "builtin_plugin.hpp"
#include "command_palette_plugin.hpp"
#include "core/content/callbacks.h"
#include "core/content/commands.h"
#include "liblogger/logger.h"
#include "logger_viewer_plugin.hpp"
#include "navigation_plugin.hpp"
#include "settings_plugin.hpp"
#include <SDL3/SDL.h>
#include <chrono>
#include <format>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <implot.h>
#include <thread>

namespace DearTs::Main::GUI {

bool DearTsApplication::on_init() {
    LOG_INFO("DearTsApplication initializing...");

    // 1. 设置配置管理器（必须在 setup_imgui 之前，以便加载字体配置）
    setup_config();

    // 2. 设置 ImGui（会从 ConfigManager 读取字体大小）
    if (!setup_imgui()) {
        LOG_ERROR("Failed to setup ImGui");
        return false;
    }

    // 3. 设置事件监听器
    setup_events();

    // 4. 设置命令和工具
    setup_commands_and_tools();

    // 5. 加载插件
    setup_plugins();

    // 6. 设置快捷键
    setup_shortcuts();

    // 7. 设置视图
    setup_views();

    // 8. 设置自定义标题栏
    m_title_bar.set_borderless(true);
    Core::UI::WindowControls::set_current_window(m_window);

    // 添加标题栏按钮
    m_title_bar.add_button(ICON_SETTINGS, "设置", [&]() {
        LOG_INFO("Settings button clicked");
        // 切换设置窗口
        auto* settings_view = Core::ContentRegistry::Views::get_by_name(
            Core::ContentRegistry::UnlocalizedString("Settings")
        );
        if (settings_view != nullptr) {
            bool& is_open = settings_view->get_window_open_state();
            is_open = !is_open;
            LOG_INFO("Settings window {}", is_open ? "opened" : "closed");
        } else {
            LOG_WARN("Settings view not found!");
        }
    }, ImVec4(0.3f, 0.6f, 0.9f, 1.0f));

    m_title_bar.add_button(ICON_INFO, "关于", [&]() {
        LOG_INFO("About button clicked");
        m_show_about_window = !m_show_about_window;
    }, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));

    // 9. 设置背景和角色渲染器
    LOG_INFO("Loading background and character renderers...");

    // 设置 renderer 到渲染器单例
    Core::UI::BackgroundRenderer::instance().set_renderer(m_renderer);
    Core::UI::CharacterRenderer::instance().set_renderer(m_renderer);

    // 从 ConfigManager 加载角色配置
    load_character_config();

    LOG_INFO("DearTsApplication initialized successfully");
    return true;
}

void DearTsApplication::load_character_config() {
    auto& config = Core::Config::ConfigManager::instance();
    auto& character_renderer = Core::UI::CharacterRenderer::instance();
    auto& character_manager = Core::UI::CharacterManager::instance();

    // 加载默认角色列表
    character_manager.load_default_characters();
    const auto& characters = character_manager.get_characters();

    if (characters.empty()) {
        LOG_WARN("No characters loaded");
        return;
    }

    // 尝试从 config 加载活动角色 ID
    auto active_id_result = config.get<std::string>("character.active_id");
    bool character_loaded = false;

    if (active_id_result.isOk()) {
        std::string active_id = active_id_result.unwrap();
        if (!active_id.empty() && character_manager.set_active_character(active_id)) {
            const Core::UI::CharacterInfo* character = character_manager.get_active_character();
            if (character) {
                // 释放旧角色
                character_renderer.clear();

                // 加载保存的角色
                if (character->type == Core::UI::CharacterType::Single) {
                    std::vector<std::string> paths = {character->image_path};
                    if (character_renderer.load_character_frames(paths)) {
                        character_renderer.set_enabled(true);
                        LOG_INFO("Loaded character from config: {} (Single)", character->name);
                        character_loaded = true;
                    }
                } else if (character->type == Core::UI::CharacterType::Animated) {
                    if (character_renderer.load_character_frames(character->frame_paths)) {
                        character_renderer.set_enabled(true);
                        LOG_INFO("Loaded character from config: {} (Animated)", character->name);
                        character_loaded = true;
                    }
                }
            }
        }
    }

    // 如果没有加载到角色（首次运行或配置无效），加载第一个角色作为默认
    if (!character_loaded) {
        const auto& first_character = characters[0];
        character_manager.set_active_character(first_character.id);
        character_renderer.clear();

        if (first_character.type == Core::UI::CharacterType::Single) {
            std::vector<std::string> paths = {first_character.image_path};
            if (character_renderer.load_character_frames(paths)) {
                character_renderer.set_enabled(true);
                character_renderer.set_position(Core::UI::CharacterPosition::BottomRight);
                character_renderer.set_scale(first_character.scale);
                character_renderer.set_opacity(first_character.opacity);
                character_renderer.set_animation_mode(Core::UI::AnimationMode::None);
                LOG_INFO("Loaded default character: {} (Single)", first_character.name);
            }
        } else if (first_character.type == Core::UI::CharacterType::Animated) {
            if (character_renderer.load_character_frames(first_character.frame_paths)) {
                character_renderer.set_enabled(true);
                character_renderer.set_position(Core::UI::CharacterPosition::BottomRight);
                character_renderer.set_scale(first_character.scale);
                character_renderer.set_opacity(first_character.opacity);
                character_renderer.set_animation_mode(Core::UI::AnimationMode::FrameLoop);
                character_renderer.set_frame_interval(first_character.frame_interval);
                LOG_INFO("Loaded default character: {} (Animated)", first_character.name);
            }
        }
    }

    // 加载并应用角色配置（覆盖上面设置的默认值）
    int position_index = config.get_or<int>("character.position", 0);
    character_renderer.set_position(static_cast<Core::UI::CharacterPosition>(position_index));

    float scale = static_cast<float>(config.get_or<double>("character.scale", 0.5));
    character_renderer.set_scale(scale);

    float opacity = static_cast<float>(config.get_or<double>("character.opacity", 0.3));
    character_renderer.set_opacity(opacity);

    int animation_mode = config.get_or<int>("character.animation_mode", 1);
    character_renderer.set_animation_mode(static_cast<Core::UI::AnimationMode>(animation_mode));

    float frame_interval = static_cast<float>(config.get_or<double>("character.frame_interval", 0.5));
    character_renderer.set_frame_interval(frame_interval);

    LOG_INFO("Character settings loaded from config: scale={:.2f}, opacity={:.2f}, position={}",
             scale, opacity, position_index);
}

void DearTsApplication::on_update(double delta_time) {
    m_current_fps = 1.0 / delta_time;

    // 更新任务管理器
    Core::Tasks::TaskManager::instance().update();

    // 更新 ToastManager
    DearTs::Plugins::Toast::ToastManager::instance().update(static_cast<float>(delta_time));

    // 更新角色动画
    Core::UI::CharacterRenderer::instance().update(delta_time);

    // 更新逻辑
    static double total_time = 0.0;
    total_time += delta_time;

    // 每5秒打印一次统计信息
    if (static_cast<int>(total_time) % 5 == 0 &&
        static_cast<int>(total_time) > m_last_log_time) {
        LOG_INFO("Running for {:.1f} seconds, FPS: {:.2f}",
                 total_time, m_current_fps);
        m_last_log_time = static_cast<int>(total_time);
    }
}

void DearTsApplication::on_render() {
    // 1. 清空渲染器
    if (m_renderer) {
        SDL_SetRenderDrawColor(m_renderer, 30, 30, 30, 255);
        SDL_RenderClear(m_renderer);
    }

    // 2. 渲染背景（在 ImGui 之前）
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    Core::UI::BackgroundRenderer::instance().render(m_renderer, viewport->Size);

    // 3. ImGui NewFrame
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    // 4. 处理快捷键（CommandPalette快捷键由插件系统处理）
    Core::UI::ShortcutManager::instance().handleShortcuts();

    // 5. 渲染自定义标题栏（返回标题栏高度）
    float title_bar_height = render_title_bar();

    // 6. 创建和渲染 DockSpace
    render_dock_space(title_bar_height);

    // 7. 渲染所有视图
    render_views();

    // 8. 渲染其他组件
    // render_menu_bar(); // 菜单已在 render_title_bar() 中绘制
    render_main_window();
    render_tool_windows();

    // 9. 渲染 Toast 通知
    DearTs::Plugins::Toast::ToastManager::instance().render();

    // 10. 渲染 ImGui
    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), m_renderer);

    // 11. 渲染角色（在 ImGui 之后，确保在最上层）
    Core::UI::CharacterRenderer::instance().render();

    // 12. 多视口支持
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

void DearTsApplication::on_event(const SDL_Event& event) {
    // 处理 SDL 事件
    ImGui_ImplSDL3_ProcessEvent(&const_cast<SDL_Event&>(event));

    // 处理窗口拖动
    if (m_title_bar.is_borderless()) {
        if (event.type == SDL_EVENT_MOUSE_MOTION) {
            if (m_is_dragging) {
                float global_x, global_y;
                SDL_GetGlobalMouseState(&global_x, &global_y);
                int new_x = (int)(global_x - m_drag_start_pos.x);
                int new_y = (int)(global_y - m_drag_start_pos.y);
                SDL_SetWindowPosition(m_window, new_x, new_y);
            }
        }
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            if (event.button.button == SDL_BUTTON_LEFT) {
                if (event.button.y < m_title_bar_config.get_title_bar_height()) {
                    if (!m_title_bar_config.is_in_button_area(event.button.x, ImGui::GetMainViewport()->Size.x)) {
                        m_is_dragging = true;

                        float global_mouse_x, global_mouse_y;
                        SDL_GetGlobalMouseState(&global_mouse_x, &global_mouse_y);

                        int window_x, window_y;
                        SDL_GetWindowPosition(m_window, &window_x, &window_y);

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

void DearTsApplication::on_shutdown() {
    LOG_INFO("DearTsApplication shutting down...");

    // 保存配置文件
    Core::Config::ConfigManager::instance().save_to_file("config.json");

    // 清理资源
    // CommandPalette 现在由插件系统管理，无需手动清理

    // 卸载所有插件
    Core::Plugin::PluginManager::instance().clear();

    // 关闭 ImGui
    if (m_imgui_context) {
        ImGui_ImplSDLRenderer3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImPlot::DestroyContext();
        ImGui::DestroyContext();
        m_imgui_context = nullptr;
    }

    LOG_INFO("Final statistics:");
    LOG_INFO("  Total frames: {}", m_frame_count);
    LOG_INFO("  Average FPS: {:.2f}", m_average_fps);
}

bool DearTsApplication::setup_imgui() {
    // 创建 ImGui 上下文
    IMGUI_CHECKVERSION();
    m_imgui_context = ImGui::CreateContext();
    if (!m_imgui_context) {
        LOG_ERROR("Failed to create ImGui context");
        return false;
    }
    ImGui::SetCurrentContext(m_imgui_context);

    // 创建 ImPlot 上下文
    ImPlot::CreateContext();
    LOG_INFO("ImPlot context created");

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    // 加载中文字体
    io.Fonts->Clear(); // 清除默认字体

    // 字体配置
    ImFontConfig font_config;
    font_config.OversampleH = 2;
    font_config.OversampleV = 2;
    font_config.PixelSnapH = true;

    // 从配置读取字体大小
    auto& config = Core::Config::ConfigManager::instance();
    float font_size = static_cast<float>(config.get_or<double>("dearts.font.size", 16.0));
    LOG_INFO("字体大小配置: {:.1f}px", font_size);

    // 尝试加载字体（按优先级）
    static const char* font_paths[] = {
        "resources/fonts/OPPOSans-M.ttf",       // 运行目录
        "resources/fonts/NotoSansSC-Regular.ttf",
        "resources/fonts/Noto nerd.ttf",
        "../resources/fonts/OPPOSans-M.ttf",  // IDE 调试目录
        "../resources/fonts/NotoSansSC-Regular.ttf",
        "../../resources/fonts/OPPOSans-M.ttf", // 更深的调试目录
        "../../resources/fonts/NotoSansSC-Regular.ttf"
    };

    bool font_loaded = false;
    for (const char* font_path : font_paths) {
        // 加载中文字体，包含完整的汉字字符集
        ImFont* font = io.Fonts->AddFontFromFileTTF(
            font_path,
            font_size,  // 使用配置的字体大小
            &font_config,
            io.Fonts->GetGlyphRangesChineseFull()
        );

        if (font != nullptr) {
            io.FontDefault = font;
            LOG_INFO("成功加载中文字体: {} (大小: {:.1f}px)", font_path, font_size);
            font_loaded = true;

            // 添加更大的字体用于标题（1.5倍大小）
            font_config.MergeMode = false; // 不合并，创建独立的字体
            io.Fonts->AddFontFromFileTTF(font_path, font_size * 1.5f, &font_config, io.Fonts->GetGlyphRangesChineseFull());
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
    if (Core::UI::IconFont::loadMaterialSymbols(18.0f)) {
        LOG_INFO("成功加载 Material Symbols 图标字体");
    } else {
        LOG_WARN("图标字体加载失败，部分图标可能显示为 ???");
        LOG_INFO("提示：运行 download_icon_font.bat 下载图标字体");
    }

    // 注意：不要手动调用 io.Fonts->Build()
    // ImGui SDL3 后端会在初始化时自动构建字体纹理

    // 应用默认主题（暗色）
    Core::UI::ThemeManager::instance().setTheme(Core::UI::Theme::Dark);
    Core::UI::ThemeManager::instance().applyImGuiStyle();

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

    return true;
}

void DearTsApplication::setup_config() {
    auto& config = Core::Config::ConfigManager::instance();

    // 尝试加载配置文件
    auto result = config.load_from_file("config.json");
    if (result.isErr()) {
        LOG_WARN("Failed to load config: {}", result.error());
    }

    // 注册配置元数据
    config.register_meta("dearts.titlebar.frame_padding_x", {
        .description = "Title bar horizontal padding",
        .default_value = 8.0,
        .is_required = false
    });

    config.register_meta("dearts.titlebar.frame_padding_y", {
        .description = "Title bar vertical padding (controls height)",
        .default_value = 10.0,
        .is_required = false
    });

    // 注册字体大小配置
    config.register_meta("dearts.font.size", {
        .description = "Font size in pixels (requires restart)",
        .default_value = 16.0,
        .is_required = false
    });

    // 注册窗口缩放配置
    config.register_meta("dearts.window.scale", {
        .description = "Window scale factor (requires restart)",
        .default_value = 1.0,
        .is_required = false
    });

    // 加载配置
    m_title_bar_config.frame_padding_x = static_cast<float>(config.get_or<double>("dearts.titlebar.frame_padding_x", 8.0));
    m_title_bar_config.frame_padding_y = static_cast<float>(config.get_or<double>("dearts.titlebar.frame_padding_y", 10.0));
}

void DearTsApplication::setup_events() {
    // 注册更新回调
    Core::ContentRegistry::Callbacks::add_on_update([this](double delta_time) {
        this->on_update(delta_time);
    });

    // 注册渲染回调
    Core::ContentRegistry::Callbacks::add_on_render([this]() {
        // 已在 on_render 中处理
    });
}

void DearTsApplication::setup_commands_and_tools() {
    using namespace Core::ContentRegistry;

    // 为所有视图注册切换命令
    // 格式: view.toggle.<view_name>

    // 侧边栏
    Commands::add("view.toggle.sidebar", "侧边栏", []() {
        auto* view = Views::get_by_name(UnlocalizedString("侧边栏"));
        if (view) {
            view->get_window_open_state() = !view->get_window_open_state();
        }
    });

    // 数据检查器
    Commands::add("view.toggle.data_inspector", "数据检查器", []() {
        auto* view = Views::get_by_name(UnlocalizedString("数据检查器"));
        if (view) {
            view->get_window_open_state() = !view->get_window_open_state();
        }
    });

    // 设置
    Commands::add("view.toggle.settings", "设置", []() {
        auto* view = Views::get_by_name(UnlocalizedString("设置"));
        if (view) {
            view->get_window_open_state() = !view->get_window_open_state();
        }
    });

    // 日志查看器
    Commands::add("view.toggle.logger_viewer", "日志查看器", []() {
        auto* view = Views::get_by_name(UnlocalizedString("日志查看器"));
        if (view) {
            view->get_window_open_state() = !view->get_window_open_state();
        }
    });

    // 通知测试器
    Commands::add("view.toggle.toast_tester", "通知测试器", []() {
        auto* view = Views::get_by_name(UnlocalizedString("通知测试器"));
        if (view) {
            view->get_window_open_state() = !view->get_window_open_state();
        }
    });

    LOG_INFO("视图切换命令已注册");
}

void DearTsApplication::setup_shortcuts() {
    using namespace Core::UI;

    auto& manager = ShortcutManager::instance();

    // 文件操作快捷键
    manager.addShortcut("file.open", Shortcut(ImGuiKey_O, true, false, false), []() {
        LOG_INFO("Shortcut: Open File (Ctrl+O)");
        Core::ContentRegistry::Commands::Registry::instance().execute("file.open");
    }, ShortcutType::Global);

    manager.addShortcut("file.save", Shortcut(ImGuiKey_S, true, false, false), []() {
        LOG_INFO("Shortcut: Save File (Ctrl+S)");
        Core::ContentRegistry::Commands::Registry::instance().execute("file.save");
    }, ShortcutType::Global);

    manager.addShortcut("file.exit", Shortcut(ImGuiKey_Q, true, false, false), [this]() {
        LOG_INFO("Shortcut: Exit (Ctrl+Q)");
        this->request_exit(0);
    }, ShortcutType::Global);

    // 视图快捷键
    manager.addShortcut("view.fullscreen", Shortcut(ImGuiKey_F11, false, false, false), []() {
        LOG_INFO("Shortcut: Toggle Fullscreen (F11)");
        Core::ContentRegistry::Commands::Registry::instance().execute("view.toggle_fullscreen");
    }, ShortcutType::Global);

    // 命令面板快捷键现在由 CommandPalettePlugin 管理
    // 这里不再需要注册

    // 主题快捷键
    manager.addShortcut("theme.dark", Shortcut(ImGuiKey_1, true, false, true), []() {
        LOG_INFO("Shortcut: Dark Theme (Alt+Ctrl+1)");
        ThemeManager::instance().setTheme(Theme::Dark);
    }, ShortcutType::Global);

    manager.addShortcut("theme.light", Shortcut(ImGuiKey_2, true, false, true), []() {
        LOG_INFO("Shortcut: Light Theme (Alt+Ctrl+2)");
        ThemeManager::instance().setTheme(Theme::Light);
    }, ShortcutType::Global);

    LOG_INFO("Shortcuts registered: {}", manager.getBindings().size());
}

void DearTsApplication::setup_views() {
    using namespace Core::UI;

    auto& view_manager = ViewManager::instance();
    (void)view_manager; // 暂时未使用，消除警告

    // 创建一些示例视图
    LOG_INFO("Views setup complete");
}

void DearTsApplication::setup_plugins() {
    auto& plugin_manager = Core::Plugin::PluginManager::instance();

    // 添加内置插件
    auto builtin_plugin = std::make_unique<DearTs::Plugins::Builtin::BuiltinPlugin>();
    auto result = plugin_manager.add_builtin(std::move(builtin_plugin));

    if (result.isErr()) {
        LOG_ERROR("Failed to load BuiltinPlugin: {}", result.error());
    } else {
        LOG_INFO("BuiltinPlugin loaded successfully");
    }

    // 添加设置插件
    auto settings_plugin = std::make_unique<DearTs::Plugins::Settings::SettingsPlugin>();
    result = plugin_manager.add_builtin(std::move(settings_plugin));

    if (result.isErr()) {
        LOG_ERROR("Failed to load SettingsPlugin: {}", result.error());
    } else {
        LOG_INFO("SettingsPlugin loaded successfully");
    }

    // 添加日志查看器插件
    auto logger_viewer_plugin = std::make_unique<DearTs::Plugins::LoggerViewer::LoggerViewerPlugin>();
    result = plugin_manager.add_builtin(std::move(logger_viewer_plugin));

    if (result.isErr()) {
        LOG_ERROR("Failed to load LoggerViewerPlugin: {}", result.error());
    } else {
        LOG_INFO("LoggerViewerPlugin loaded successfully");
    }

    // 添加导航插件
    auto navigation_plugin = std::make_unique<DearTs::Plugins::Navigation::NavigationPlugin>();
    result = plugin_manager.add_builtin(std::move(navigation_plugin));

    if (result.isErr()) {
        LOG_ERROR("Failed to load NavigationPlugin: {}", result.error());
    } else {
        LOG_INFO("NavigationPlugin loaded successfully");
    }

    // 添加 Toast 通知插件
    auto toast_plugin = std::make_unique<DearTs::Plugins::Toast::ToastPlugin>();
    result = plugin_manager.add_builtin(std::move(toast_plugin));

    if (result.isErr()) {
        LOG_ERROR("Failed to load ToastPlugin: {}", result.error());
    } else {
        LOG_INFO("ToastPlugin loaded successfully");
    }

    // 添加命令面板插件
    auto command_palette_plugin = std::make_unique<DearTs::Plugins::CommandPalette::CommandPalettePlugin>();
    result = plugin_manager.add_builtin(std::move(command_palette_plugin));

    if (result.isErr()) {
        LOG_ERROR("Failed to load CommandPalettePlugin: {}", result.error());
    } else {
        LOG_INFO("CommandPalettePlugin loaded successfully");
    }

    // 获取插件信息
    auto plugin_infos = plugin_manager.get_all_plugins_info();
    LOG_INFO("Plugin system initialized");
    LOG_INFO("Built-in plugins: {}", plugin_infos.size());

    // 可以从目录加载插件
    // plugin_manager.load_from_directory("plugins");
}

void DearTsApplication::render_menu_bar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("文件")) {
            if (ImGui::MenuItem("打开", "Ctrl+O")) {
                Core::ContentRegistry::Commands::Registry::instance().execute("file.open");
            }
            if (ImGui::MenuItem("保存", "Ctrl+S")) {
                Core::ContentRegistry::Commands::Registry::instance().execute("file.save");
            }
            ImGui::Separator();
            if (ImGui::MenuItem("退出", "Ctrl+Q")) {
                this->request_exit(0);
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("视图")) {
            if (ImGui::MenuItem("全屏", "F11")) {
                Core::ContentRegistry::Commands::Registry::instance().execute("view.toggle_fullscreen");
            }
            if (ImGui::MenuItem("重置布局")) {
                Core::ContentRegistry::Commands::Registry::instance().execute("view.reset_layout");
            }
            ImGui::Separator();
            if (ImGui::MenuItem("演示窗口")) {
                m_show_demo_window = !m_show_demo_window;
            }
            if (ImGui::MenuItem("指标窗口")) {
                m_show_metrics_window = !m_show_metrics_window;
            }
            if (ImGui::MenuItem("样式编辑器")) {
                m_show_style_editor = !m_show_style_editor;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("工具")) {
            if (ImGui::MenuItem("计算器")) {
                Core::ContentRegistry::Commands::Registry::instance().execute("tools.calculator");
            }
            if (ImGui::MenuItem("颜色选择器")) {
                Core::ContentRegistry::Commands::Registry::instance().execute("tools.color_picker");
            }
            if (ImGui::MenuItem("记事本")) {
                Core::ContentRegistry::Commands::Registry::instance().execute("tools.notepad");
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("系统")) {
            if (ImGui::MenuItem("任务和插件")) {
                m_show_task_plugin_window = !m_show_task_plugin_window;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("启动测试任务")) {
                Core::ContentRegistry::Commands::Registry::instance().execute("tasks.start_test");
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("主题")) {
            if (ImGui::MenuItem("暗色主题", "Alt+Ctrl+1")) {
                Core::UI::ThemeManager::instance().setTheme(Core::UI::Theme::Dark);
            }
            if (ImGui::MenuItem("亮色主题", "Alt+Ctrl+2")) {
                Core::UI::ThemeManager::instance().setTheme(Core::UI::Theme::Light);
            }
            if (ImGui::MenuItem("经典主题", "Alt+Ctrl+3")) {
                Core::UI::ThemeManager::instance().setTheme(Core::UI::Theme::Classic);
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("帮助")) {
            if (ImGui::MenuItem("关于")) {
                m_show_about_window = !m_show_about_window;
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void DearTsApplication::render_main_window() {
    // 主窗口由 DockSpace 管理，可以在这里添加固定的主视图
}

void DearTsApplication::render_tool_windows() {
    // 演示窗口
    if (m_show_demo_window) {
        ImGui::ShowDemoWindow(&m_show_demo_window);
    }

    // 指标窗口
    if (m_show_metrics_window) {
        ImGui::ShowMetricsWindow(&m_show_metrics_window);
    }

    // 样式编辑器
    if (m_show_style_editor) {
        ImGui::Begin("样式编辑器", &m_show_style_editor);
        ImGui::ShowStyleEditor();
        ImGui::End();
    }

    // 关于窗口
    if (m_show_about_window) {
        ImGui::Begin("关于", &m_show_about_window);
        ImGui::Text("DearTs 工具箱");
        ImGui::Separator();
        ImGui::Text("版本: 1.0.0");
        ImGui::Text("基于 DearTs 框架");
        ImGui::Text("ImGui 版本: {}", ImGui::GetVersion());
        ImGui::Separator();
        ImGui::Text("作者: DearTs Team");
        ImGui::End();
    }

    // 任务和插件管理窗口
    if (m_show_task_plugin_window) {
        ImGui::Begin("任务和插件", &m_show_task_plugin_window);

        if (ImGui::BeginTabBar("TaskPluginTabBar")) {
            // 任务系统标签
            if (ImGui::BeginTabItem("任务系统")) {
                size_t running_count = Core::Tasks::TaskManager::instance().getRunningTaskCount();
                ImGui::Text("运行中的任务: %zu 个", running_count);

                ImGui::Separator();

                if (ImGui::Button("启动快速任务")) {
                    Core::Tasks::TaskManager::instance().launch("快速任务", [](const std::atomic<bool>& should_cancel) {
                        for (int i = 0; i <= 10 && !should_cancel; ++i) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        }
                    });
                }

                ImGui::SameLine();
                if (ImGui::Button("启动长时间任务")) {
                    Core::Tasks::TaskManager::instance().launch("长时间任务", [](const std::atomic<bool>& should_cancel) {
                        for (int i = 0; i <= 100 && !should_cancel; ++i) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        }
                    });
                }

                ImGui::SameLine();
                if (ImGui::Button("取消所有任务")) {
                    Core::Tasks::TaskManager::instance().cancelAll();
                }

                // 显示运行中的任务
                auto running_tasks = Core::Tasks::TaskManager::instance().getRunningTasks();
                if (!running_tasks.empty()) {
                    ImGui::Separator();
                    ImGui::Text("活动任务:");
                    for (const auto& task : running_tasks) {
                        Core::UI::TaskWidget::renderTask(task);
                    }
                }

                ImGui::EndTabItem();
            }

            // 插件系统标签
            if (ImGui::BeginTabItem("插件系统")) {
                auto plugin_infos = Core::Plugin::PluginManager::instance().get_all_plugins_info();
                ImGui::Text("已加载插件: %zu 个", plugin_infos.size());

                ImGui::Separator();

                if (!plugin_infos.empty()) {
                    for (const auto& info : plugin_infos) {
                        ImGui::BulletText("%s v%s 作者: %s",
                                         info.name.c_str(),
                                         info.version.c_str(),
                                         info.author.c_str());
                    }
                } else {
                    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "暂无加载的插件");
                }

                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "提示: 插件功能正在开发中");

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::End();
    }
}

float DearTsApplication::render_title_bar() {
    float title_bar_height = 0.0f;

    if (!m_title_bar.is_borderless()) {
        return title_bar_height;
    }

    // 使用配置设置 MenuBar 的内边距（保持原有的 10px，让 MenuBar 高度为 36px）
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
        ImVec2(m_title_bar_config.frame_padding_x, m_title_bar_config.frame_padding_y));

    if (ImGui::BeginMainMenuBar()) {
        // 获取实际的 MenuBar 高度（在设置 FramePadding 之后）
        title_bar_height = ImGui::GetFrameHeight();

        // 绘制自定义标题栏内容
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 5));

        // 左侧：窗口标题（使用图标字体渲染图标部分）
        ImGui::SetCursorPosX(m_title_bar_config.title_left_margin);

        // 使用图标字体渲染图标
        if (Core::UI::IconFont::isLoaded()) {
            ImGui::PushFont(Core::UI::IconFont::getFont());
            ImGui::TextColored(ImVec4(0.3f, 0.6f, 0.9f, 1.0f), "%s", ICON_TOOLS "  ");
            ImGui::PopFont();
            // 继续使用中文字体渲染文本
            ImGui::SameLine(0, 0);
            ImGui::TextColored(ImVec4(0.3f, 0.6f, 0.9f, 1.0f), "DearTs 工具箱");
        } else {
            // 图标字体未加载，使用纯文本
            ImGui::TextColored(ImVec4(0.3f, 0.6f, 0.9f, 1.0f), "🧰 DearTs 工具箱");
        }

        // 右侧：控制按钮
        title_bar_height = m_title_bar_config.get_title_bar_height();
        float button_y = (title_bar_height - m_title_bar_config.button_height) / 2.0f;

        // 设置按钮样式：去除背景色，确保图标水平居中
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0)); // 透明背景
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.5f)); // 悬停时半透明灰色
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.4f, 0.7f)); // 按下时更深的灰色
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f)); // 水平和垂直居中

        // 使用图标字体（如果已加载）
        if (Core::UI::IconFont::isLoaded()) {
            ImGui::PushFont(Core::UI::IconFont::getFont());
        }

        // 计算按钮起始位置（4个按钮：关于、最小化、最大化/还原、关闭）
        float start_x = ImGui::GetMainViewport()->Size.x -
                       (4 * m_title_bar_config.button_width +
                        3 * m_title_bar_config.button_spacing +
                        m_title_bar_config.button_right_margin);

        // 关于按钮
        ImGui::SetCursorPos(ImVec2(start_x, button_y));
        if (ImGui::Button(ICON_INFO, ImVec2(m_title_bar_config.button_width, m_title_bar_config.button_height))) {
            m_show_about_window = !m_show_about_window;
        }
        if (ImGui::IsItemHovered()) {
            // ImGui::SetTooltip("关于");
        }

        // 最小化按钮
        ImGui::SetCursorPos(ImVec2(start_x + m_title_bar_config.button_width + m_title_bar_config.button_spacing, button_y));
        if (ImGui::Button(ICON_MINIMIZE, ImVec2(m_title_bar_config.button_width, m_title_bar_config.button_height))) {
            if (m_window) {
                SDL_MinimizeWindow(m_window);
            }
        }
        if (ImGui::IsItemHovered()) {
            // ImGui::SetTooltip("最小化");
        }

        // 最大化/还原按钮
        ImGui::SetCursorPos(ImVec2(start_x + (m_title_bar_config.button_width + m_title_bar_config.button_spacing) * 2, button_y));
        if (ImGui::Button(m_is_maximized ? ICON_RESTORE : ICON_MAXIMIZE,
                         ImVec2(m_title_bar_config.button_width, m_title_bar_config.button_height))) {
            if (m_window) {
                if (m_is_maximized) {
                    // 还原窗口
                    SDL_RestoreWindow(m_window);
                    m_is_maximized = false;
                } else {
                    // 最大化窗口
                    SDL_MaximizeWindow(m_window);
                    m_is_maximized = true;
                }
            }
        }
        if (ImGui::IsItemHovered()) {
            // ImGui::SetTooltip(m_is_maximized ? "恢復" : "最大化");
        }

        // 关闭按钮
        ImGui::SetCursorPos(ImVec2(start_x + (m_title_bar_config.button_width + m_title_bar_config.button_spacing) * 3, button_y));
        if (ImGui::Button(ICON_CLOSE, ImVec2(m_title_bar_config.button_width, m_title_bar_config.button_height))) {
            this->request_exit(0);
        }
        if (ImGui::IsItemHovered()) {
            // ImGui::SetTooltip("关闭");
        }

        // 恢复原来的字体
        if (Core::UI::IconFont::isLoaded()) {
            ImGui::PopFont();
        }

        ImGui::PopStyleColor(3); // Button, ButtonHovered, ButtonActive
        ImGui::PopStyleVar(2); // FramePadding + ButtonTextAlign
        ImGui::PopStyleVar(); // ItemSpacing

        ImGui::EndMainMenuBar();
    }
    ImGui::PopStyleVar(); // FramePadding

    return title_bar_height;
}

void DearTsApplication::render_dock_space(float title_bar_height) {
    // 创建 DockSpace
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    float menuBarHeight = m_title_bar.is_borderless() ? title_bar_height : 0.0f;

    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + menuBarHeight));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, viewport->Size.y - menuBarHeight));
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking |
                                   ImGuiWindowFlags_NoTitleBar |
                                   ImGuiWindowFlags_NoCollapse |
                                   ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoBackground |
                                   ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace", nullptr, window_flags);
    ImGui::PopStyleVar(3);

    ImGui::DockSpace(ImGui::GetID("MainDockSpace"));
    ImGui::End();
}

void DearTsApplication::render_views() {
    // 渲染所有视图（从 ContentRegistry::Views）
    for (auto& [name, view] : Core::ContentRegistry::Views::get_all()) {
        // 跟踪窗口状态
        view->track_window_state();

        if (view->should_draw()) {
            view->draw();
        }
    }
}

} // namespace DearTs::Main::GUI

