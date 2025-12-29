/**
 * @file main.cpp
 * @brief SDL3 + ImGui 对象级交互示例
 * @details 展示如何在 ImGui 中嵌入 SDL3 渲染的内容，并实现对象级交互
 *
 * 功能演示：
 * 1. ImGui 可收起区域
 * 2. SDL3 离屏渲染到纹理
 * 3. 在 ImGui 中显示 SDL 纹理
 * 4. ✨ 鼠标悬停检测（高亮对象）
 * 5. ✨ 点击选择对象
 * 6. ✨ 对象信息提示
 * 7. 视图平移和缩放
 */

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <SDL3/SDL.h>
#include <iostream>
#include <cmath>
#include <algorithm>

#include "interactive_objects.hpp"

// ================ 全局变量 ================

SDL_Window* g_window = nullptr;
SDL_Renderer* g_renderer = nullptr;
bool g_running = true;

// SDL 纹理（渲染目标）
SDL_Texture* g_render_texture = nullptr;
constexpr int TEXTURE_WIDTH = 800;
constexpr int TEXTURE_HEIGHT = 600;

// 视图变换
struct ViewTransform {
    float offset_x = 0.0f;
    float offset_y = 0.0f;
    float scale = 1.0f;
} g_transform;

// 交互状态
bool g_is_dragging_view = false;
ImVec2 g_drag_start_pos;

// 动画时间
float g_animation_time = 0.0f;
bool g_animation_paused = false;

// 对象管理器
ObjectManager g_object_manager;

// 交互状态
InteractiveObject* g_hovered_object = nullptr;
InteractiveObject* g_selected_object = nullptr;

// ================ 函数声明 ================

bool init();
void shutdown();
void render_to_texture();
void draw_gui();
void handle_events();

/**
 * @brief 坐标转换：ImGui 屏幕坐标 → SDL 纹理坐标
 */
ImVec2 screen_to_texture(ImVec2 screen_pos, ImVec2 child_pos) {
    ImVec2 tex_pos;
    tex_pos.x = (screen_pos.x - child_pos.x - g_transform.offset_x) / g_transform.scale;
    tex_pos.y = (screen_pos.y - child_pos.y - g_transform.offset_y) / g_transform.scale;
    return tex_pos;
}

/**
 * @brief 坐标转换：SDL 纹理坐标 → ImGui 屏幕坐标
 */
ImVec2 texture_to_screen(ImVec2 tex_pos, ImVec2 child_pos) {
    ImVec2 screen_pos;
    screen_pos.x = child_pos.x + tex_pos.x * g_transform.scale + g_transform.offset_x;
    screen_pos.y = child_pos.y + tex_pos.y * g_transform.scale + g_transform.offset_y;
    return screen_pos;
}

// ================ 主函数 ================

int main(int argc, char* argv[]) {
    std::cout << "=== SDL3 + ImGui 对象级交互示例 ===" << std::endl;
    std::cout << "操作说明：" << std::endl;
    std::cout << "  - 鼠标移动：悬停高亮对象" << std::endl;
    std::cout << "  - 鼠标左键点击：选择对象" << std::endl;
    std::cout << "  - 鼠标右键拖拽：平移视图" << std::endl;
    std::cout << "  - 鼠标滚轮：缩放视图" << std::endl;
    std::cout << "  - ESC 键：取消选择" << std::endl;
    std::cout << "  - 空格键：暂停/恢复动画" << std::endl;
    std::cout << "======================================" << std::endl;

    // 初始化
    if (!init()) {
        std::cerr << "初始化失败！" << std::endl;
        return 1;
    }

    // 创建示例对象
    // 矩形 1 - 中心旋转
    auto* rect1 = g_object_manager.add_rect(
        ImVec2(TEXTURE_WIDTH / 2.0f, TEXTURE_HEIGHT / 2.0f),
        100.0f,
        IM_COL32(255, 100, 100, 255),
        "中心矩形"
    );
    rect1->description = "在中心旋转的红色矩形";

    // 矩形 2 - 左上角
    auto* rect2 = g_object_manager.add_rect(
        ImVec2(200.0f, 200.0f),
        60.0f,
        IM_COL32(255, 150, 50, 255),
        "左上矩形"
    );
    rect2->anim_offset = 1.0f; // 不同步的动画

    // 圆形 1 - 右下角
    auto* circle1 = g_object_manager.add_circle(
        ImVec2(600.0f, 400.0f),
        50.0f,
        IM_COL32(100, 255, 100, 255),
        "绿色圆圈"
    );
    circle1->description = "脉冲效果的绿色圆形";
    circle1->anim_offset = 2.0f;

    // 圆形 2 - 左下角
    auto* circle2 = g_object_manager.add_circle(
        ImVec2(200.0f, 450.0f),
        40.0f,
        IM_COL32(100, 200, 255, 255),
        "蓝色圆圈"
    );
    circle2->anim_offset = 3.0f;

    std::cout << "已创建 " << g_object_manager.get_count() << " 个可交互对象" << std::endl;

    // 主循环
    while (g_running) {
        // 处理事件
        handle_events();

        // 更新动画
        if (!g_animation_paused) {
            g_object_manager.update(g_animation_time);
        }

        // 渲染到纹理（按需）
        static float last_render_time = 0.0f;
        g_animation_time += ImGui::GetIO().DeltaTime;
        if (g_object_manager.needs_redraw() || g_animation_time - last_render_time > 0.033f) {
            render_to_texture();
            last_render_time = g_animation_time;
        }

        // 开始 ImGui 帧
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // 绘制 GUI
        draw_gui();

        // 渲染
        ImGui::Render();
        SDL_SetRenderDrawColor(g_renderer, 30, 30, 30, 255);
        SDL_RenderClear(g_renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), g_renderer);
        SDL_RenderPresent(g_renderer);
    }

    // 清理
    shutdown();

    std::cout << "程序正常退出" << std::endl;
    return 0;
}

// ================ 初始化和清理 ================

bool init() {
    // 初始化 SDL
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return false;
    }
    std::cout << "SDL3 初始化成功" << std::endl;

    // 创建窗口
    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, "SDL3 + ImGui 对象级交互示例");
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, 1280);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, 720);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);

    g_window = SDL_CreateWindowWithProperties(props);
    SDL_DestroyProperties(props);

    if (!g_window) {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        return false;
    }
    std::cout << "SDL 窗口创建成功" << std::endl;

    // 创建渲染器
    g_renderer = SDL_CreateRenderer(g_window, nullptr);
    if (!g_renderer) {
        std::cerr << "Failed to create renderer: " << SDL_GetError() << std::endl;
        return false;
    }
    std::cout << "SDL 渲染器创建成功" << std::endl;

    // 初始化 ImGui
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // 加载中文字体
    io.Fonts->Clear();

    ImFontConfig font_config;
    font_config.OversampleH = 2;
    font_config.OversampleV = 2;
    font_config.PixelSnapH = true;

    static const char* font_paths[] = {
        "resources/fonts/OPPOSans-M.ttf",
        "resources/fonts/Noto nerd.ttf",
        "../resources/fonts/OPPOSans-M.ttf",
        "../../resources/fonts/OPPOSans-M.ttf",
        "../../../resources/fonts/OPPOSans-M.ttf"
    };

    bool font_loaded = false;
    for (const char* font_path : font_paths) {
        ImFont* font = io.Fonts->AddFontFromFileTTF(
            font_path,
            16.0f,
            &font_config,
            io.Fonts->GetGlyphRangesChineseFull()
        );

        if (font != nullptr) {
            io.FontDefault = font;
            std::cout << "成功加载中文字体: " << font_path << std::endl;
            font_loaded = true;

            font_config.MergeMode = false;
            io.Fonts->AddFontFromFileTTF(font_path, 24.0f, &font_config, io.Fonts->GetGlyphRangesChineseFull());
            break;
        }
    }

    if (!font_loaded) {
        std::cerr << "警告：未能加载中文字体，使用默认字体" << std::endl;
        io.Fonts->AddFontDefault();
    }

    ImGui_ImplSDL3_InitForSDLRenderer(g_window, g_renderer);
    ImGui_ImplSDLRenderer3_Init(g_renderer);
    std::cout << "ImGui 初始化成功" << std::endl;

    ImGui::StyleColorsDark();

    // 创建离屏纹理
    g_render_texture = SDL_CreateTexture(
        g_renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        TEXTURE_WIDTH,
        TEXTURE_HEIGHT
    );

    if (!g_render_texture) {
        std::cerr << "Failed to create render texture: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_SetTextureBlendMode(g_render_texture, SDL_BLENDMODE_BLEND);
    std::cout << "离屏纹理创建成功 (" << TEXTURE_WIDTH << "x" << TEXTURE_HEIGHT << ")" << std::endl;

    // 初始渲染
    render_to_texture();

    return true;
}

void shutdown() {
    if (g_render_texture) {
        SDL_DestroyTexture(g_render_texture);
        g_render_texture = nullptr;
    }

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    if (g_renderer) {
        SDL_DestroyRenderer(g_renderer);
        g_renderer = nullptr;
    }

    if (g_window) {
        SDL_DestroyWindow(g_window);
        g_window = nullptr;
    }

    SDL_Quit();
    std::cout << "资源已清理" << std::endl;
}

// ================ SDL3 渲染到纹理 ================

void render_to_texture() {
    if (!g_renderer || !g_render_texture) return;

    // 保存当前渲染目标
    SDL_Texture* old_target = SDL_GetRenderTarget(g_renderer);

    // 设置渲染目标到纹理
    if (!SDL_SetRenderTarget(g_renderer, g_render_texture)) {
        std::cerr << "Failed to set render target: " << SDL_GetError() << std::endl;
        return;
    }

    // 清空纹理（深色背景）
    SDL_SetRenderDrawColor(g_renderer, 20, 20, 40, 255);
    SDL_RenderClear(g_renderer);

    // 绘制网格背景
    SDL_SetRenderDrawColor(g_renderer, 255, 255, 255, 30);
    for (float x = 0; x < TEXTURE_WIDTH; x += 50.0f) {
        SDL_RenderLine(g_renderer, x, 0, x, TEXTURE_HEIGHT);
    }
    for (float y = 0; y < TEXTURE_HEIGHT; y += 50.0f) {
        SDL_RenderLine(g_renderer, 0, y, TEXTURE_WIDTH, y);
    }

    // 渲染所有对象
    g_object_manager.render_all(g_renderer);

    // 恢复原渲染目标
    SDL_SetRenderTarget(g_renderer, old_target);
}

// ================ ImGui GUI ================

void draw_gui() {
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(450, 700), ImGuiCond_FirstUseEver);

    ImGui::Begin("SDL3 对象级交互示例", nullptr, ImGuiWindowFlags_NoCollapse);

    // 说明文本
    ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f),
                       "在 ImGui 中嵌入可交互的 SDL3 内容");
    ImGui::Separator();

    // ================ 可收起的 SDL3 渲染区域 ================
    if (ImGui::CollapsingHeader("SDL3 交互区域", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (!g_render_texture) {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "纹理未初始化");
            return;
        }

        // 计算显示尺寸
        float available_width = ImGui::GetContentRegionAvail().x;
        float aspect_ratio = static_cast<float>(TEXTURE_HEIGHT) / static_cast<float>(TEXTURE_WIDTH);
        float display_height = available_width * aspect_ratio;
        display_height = std::min(display_height, 400.0f);

        ImVec2 display_size(available_width, display_height);

        // 创建子窗口
        ImGui::BeginChild("SDLRenderCanvas", display_size, true,
                          ImGuiWindowFlags_NoScrollbar);

        // 获取子窗口位置
        ImVec2 child_pos = ImGui::GetCursorScreenPos();

        // 创建不可见按钮（接收鼠标事件）
        ImGui::InvisibleButton("canvas", display_size);

        bool is_hovered = ImGui::IsItemHovered();

        // ============== 处理鼠标交互 ==============
        if (is_hovered) {
            // 获取鼠标在 ImGui 中的位置
            ImVec2 mouse_pos = ImGui::GetMousePos();

            // 转换到纹理坐标
            ImVec2 tex_pos = screen_to_texture(mouse_pos, child_pos);

            // 检查是否在纹理范围内
            bool in_texture = (tex_pos.x >= 0 && tex_pos.x < TEXTURE_WIDTH &&
                              tex_pos.y >= 0 && tex_pos.y < TEXTURE_HEIGHT);

            if (in_texture) {
                // 悬停检测
                g_hovered_object = g_object_manager.check_hover(tex_pos);

                // 点击检测（左键）
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    g_selected_object = g_object_manager.check_click(tex_pos);

                    if (g_selected_object) {
                        std::cout << "选中对象: " << g_selected_object->get_info() << std::endl;
                    }
                }

                // 右键拖拽平移视图
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                    g_is_dragging_view = true;
                    g_drag_start_pos = mouse_pos;
                }

                if (g_is_dragging_view && ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
                    ImVec2 mouse_delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);
                    g_transform.offset_x += mouse_delta.x;
                    g_transform.offset_y += mouse_delta.y;
                    ImGui::ResetMouseDragDelta(ImGuiMouseButton_Right);
                }

                if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
                    g_is_dragging_view = false;
                }

                // 鼠标滚轮缩放
                float wheel = ImGui::GetIO().MouseWheel;
                if (wheel != 0.0f) {
                    float zoom_factor = 1.1f;
                    if (wheel > 0.0f) {
                        g_transform.scale *= zoom_factor;
                    } else {
                        g_transform.scale /= zoom_factor;
                    }
                    g_transform.scale = std::clamp(g_transform.scale, 0.1f, 10.0f);
                }
            } else {
                // 鼠标在纹理外，清除悬停状态
                if (g_hovered_object) {
                    g_hovered_object->is_hovered = false;
                    g_hovered_object = nullptr;
                }
            }
        }

        // 绘制纹理（应用变换）
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        float scaled_width = display_size.x * g_transform.scale;
        float scaled_height = display_size.y * g_transform.scale;

        ImVec2 p0(
            child_pos.x + (display_size.x - scaled_width) * 0.5f + g_transform.offset_x,
            child_pos.y + (display_size.y - scaled_height) * 0.5f + g_transform.offset_y
        );
        ImVec2 p1(p0.x + scaled_width, p0.y + scaled_height);

        // 绘制纹理
        ImTextureID texture_id = (ImTextureID)g_render_texture;
        draw_list->AddImage(texture_id, p0, p1, ImVec2(0, 0), ImVec2(1, 1),
                           IM_COL32(255, 255, 255, 255));

        // 绘制边框
        draw_list->AddRect(p0, p1, IM_COL32(128, 128, 128, 255));

        // ============== 显示悬停提示 ==============
        if (g_hovered_object && ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s\n%s",
                             g_hovered_object->get_info().c_str(),
                             g_hovered_object->description.c_str());
        }

        ImGui::EndChild();

        // 显示变换信息
        ImGui::Spacing();
        if (ImGui::Button("重置视图##reset")) {
            g_transform.offset_x = 0.0f;
            g_transform.offset_y = 0.0f;
            g_transform.scale = 1.0f;
        }

        ImGui::SameLine();
        ImGui::Checkbox("暂停动画", &g_animation_paused);

        ImGui::SameLine();
        if (ImGui::Button("取消选择")) {
            g_object_manager.deselect_all();
            g_selected_object = nullptr;
        }

        ImGui::Text("偏移: (%.0f, %.0f) | 缩放: %.2fx",
                    g_transform.offset_x, g_transform.offset_y, g_transform.scale);
        ImGui::Text("对象数量: %zu | 选中: %s | 悬停: %s",
                    g_object_manager.get_count(),
                    g_selected_object ? g_selected_object->name.c_str() : "无",
                    g_hovered_object ? g_hovered_object->name.c_str() : "无");
    }

    ImGui::Separator();

    // ================ 选中对象信息 ================
    if (ImGui::CollapsingHeader("选中对象信息")) {
        if (g_selected_object) {
            ImGui::Text("ID: %d", g_selected_object->id);
            ImGui::Text("名称: %s", g_selected_object->name.c_str());
            ImGui::Text("描述: %s", g_selected_object->description.c_str());
            ImGui::Text("位置: (%.1f, %.1f)", g_selected_object->center.x, g_selected_object->center.y);
            ImGui::Text("大小: %.1f", g_selected_object->size);

            if (g_selected_object->is_hovered) {
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "状态: 悬停");
            }
            if (g_selected_object->is_selected) {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "状态: 已选中");
            }
        } else {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "未选择任何对象");
            ImGui::Text("点击纹理中的对象进行选择");
        }
    }

    // ================ 操作说明 ================
    if (ImGui::CollapsingHeader("操作说明")) {
        ImGui::BulletText("🖱️ 鼠标移动：悬停高亮对象（黄色）");
        ImGui::BulletText("🖱️ 左键点击：选择对象（青色）");
        ImGui::BulletText("🖱️ 右键拖拽：平移视图");
        ImGui::BulletText("🖱️ 鼠标滚轮：缩放视图（0.1x - 10x）");
        ImGui::BulletText("⌨️ ESC 键：取消选择");
        ImGui::BulletText("⌨️ 空格键：暂停/恢复动画");
    }

    // ================ 技术说明 ================
    if (ImGui::CollapsingHeader("技术说明")) {
        ImGui::Text("对象系统：");
        ImGui::BulletText("InteractiveObject - 可交互对象基类");
        ImGui::BulletText("InteractiveRect - 可交互矩形");
        ImGui::BulletText("InteractiveCircle - 可交互圆形");
        ImGui::BulletText("ObjectManager - 对象管理器");

        ImGui::Spacing();
        ImGui::Text("坐标转换：");
        ImGui::BulletText("screen_to_texture() - ImGui → SDL纹理");
        ImGui::BulletText("texture_to_screen() - SDL纹理 → ImGui");

        ImGui::Spacing();
        ImGui::Text("交互检测：");
        ImGui::BulletText("check_hover() - 悬停检测");
        ImGui::BulletText("check_click() - 点击检测");
        ImGui::BulletText("contains() - 点是否在对象内");
    }

    ImGui::End();
}

// ================ 事件处理 ================

void handle_events() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);

        if (event.type == SDL_EVENT_QUIT) {
            g_running = false;
        }
        if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            if (event.window.windowID == SDL_GetWindowID(g_window)) {
                g_running = false;
            }
        }

        // 键盘事件
        if (event.type == SDL_EVENT_KEY_DOWN) {
            switch (event.key.key) {
                case SDLK_ESCAPE:
                    // 取消选择
                    g_object_manager.deselect_all();
                    g_selected_object = nullptr;
                    std::cout << "取消选择" << std::endl;
                    break;

                case SDLK_SPACE:
                    // 暂停/恢复动画
                    g_animation_paused = !g_animation_paused;
                    std::cout << "动画" << (g_animation_paused ? "暂停" : "恢复") << std::endl;
                    break;
            }
        }
    }
}
