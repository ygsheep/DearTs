/**
 * @file main.cpp
 * @brief SDL3 + ImGui 混合渲染示例
 * @details 展示如何在 ImGui 中嵌入 SDL3 渲染的内容，并支持平移和缩放
 *
 * 功能演示：
 * 1. ImGui 可收起区域
 * 2. SDL3 离屏渲染到纹理
 * 3. 在 ImGui 中显示 SDL 纹理
 * 4. 鼠标拖拽平移
 * 5. 鼠标滚轮缩放
 * 6. 动画效果展示
 */

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <SDL3/SDL.h>
#include <iostream>
#include <cmath>
#include <algorithm>

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
bool g_is_dragging = false;
ImVec2 g_drag_start_pos;

// 动画时间
float g_animation_time = 0.0f;

// ================ 函数声明 ================

bool init();
void shutdown();
void render_to_texture();
void draw_sample_graphics(SDL_Renderer* renderer, const SDL_FRect& rect, float time);
void draw_gui();
void handle_events();

// ================ 主函数 ================

int main(int argc, char* argv[]) {
    std::cout << "=== SDL3 + ImGui 混合渲染示例 ===" << std::endl;
    std::cout << "操作说明：" << std::endl;
    std::cout << "  - 鼠标拖拽：平移视图" << std::endl;
    std::cout << "  - 鼠标滚轮：缩放视图" << std::endl;
    std::cout << "  - 点击标题栏：收起/展开区域" << std::endl;
    std::cout << "====================================" << std::endl;

    // 初始化
    if (!init()) {
        std::cerr << "初始化失败！" << std::endl;
        return 1;
    }

    // 主循环
    while (g_running) {
        // 处理事件
        handle_events();

        // 渲染到纹理（30 FPS 更新）
        static float last_render_time = 0.0f;
        g_animation_time += ImGui::GetIO().DeltaTime;
        if (g_animation_time - last_render_time > 0.033f) {
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
    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, "SDL3 + ImGui 混合渲染示例");
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
        "../../resources/fonts/OPPOSans-M.ttf", // 更深的调试目录
        "../../../resources/fonts/OPPOSans-M.ttf" // 更更深的调试目录
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
            std::cout << "成功加载中文字体: " << font_path << std::endl;
            font_loaded = true;

            // 添加更大的字体用于标题
            font_config.MergeMode = false; // 不合并，创建独立的字体
            io.Fonts->AddFontFromFileTTF(font_path, 24.0f, &font_config, io.Fonts->GetGlyphRangesChineseFull());
            break;
        }
    }

    if (!font_loaded) {
        std::cerr << "警告：未能加载中文字体，使用默认字体（可能无法显示中文）" << std::endl;
        // 使用默认字体作为后备
        io.Fonts->AddFontDefault();
    }

    // 初始化 ImGui SDL3 后端
    ImGui_ImplSDL3_InitForSDLRenderer(g_window, g_renderer);
    ImGui_ImplSDLRenderer3_Init(g_renderer);
    std::cout << "ImGui 初始化成功" << std::endl;

    // 设置 ImGui 样式
    ImGui::StyleColorsDark();

    // 创建离屏纹理（渲染目标）
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

    // 设置纹理混合模式
    SDL_SetTextureBlendMode(g_render_texture, SDL_BLENDMODE_BLEND);
    std::cout << "离屏纹理创建成功 (" << TEXTURE_WIDTH << "x" << TEXTURE_HEIGHT << ")" << std::endl;

    // 初始渲染一次
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

    // 定义渲染区域
    SDL_FRect render_rect = {
        0.0f, 0.0f,
        static_cast<float>(TEXTURE_WIDTH),
        static_cast<float>(TEXTURE_HEIGHT)
    };

    // 绘制示例图形
    draw_sample_graphics(g_renderer, render_rect, g_animation_time);

    // 恢复原渲染目标
    SDL_SetRenderTarget(g_renderer, old_target);
}

void draw_sample_graphics(SDL_Renderer* renderer, const SDL_FRect& rect, float time) {
    if (!renderer) return;

    float w = rect.w;
    float h = rect.h;
    float cx = w / 2.0f;  // 中心 X
    float cy = h / 2.0f;  // 中心 Y

    // 1. 绘制渐变背景
    for (int y = 0; y < static_cast<int>(h); y += 4) {
        float t = y / h;
        SDL_SetRenderDrawColor(renderer,
                               static_cast<Uint8>(30 * t),
                               static_cast<Uint8>(60 * (1 - t) + 20),
                               static_cast<Uint8>(80),
                               255);
        SDL_FRect line_rect = { rect.x, rect.y + y, w, 4.0f };
        SDL_RenderFillRect(renderer, &line_rect);
    }

    // 2. 绘制网格线
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 30);
    for (float x = 0; x < w; x += 50.0f) {
        SDL_RenderLine(renderer, x, 0, x, h);
    }
    for (float y = 0; y < h; y += 50.0f) {
        SDL_RenderLine(renderer, 0, y, w, y);
    }

    // 3. 绘制旋转的矩形
    float rect_size = 100.0f;
    float angle = time * 2.0f;  // 旋转角度

    SDL_FPoint rect_corners[4];
    for (int i = 0; i < 4; i++) {
        float theta = angle + i * 3.14159f / 2.0f;
        rect_corners[i].x = cx + rect_size * std::cos(theta);
        rect_corners[i].y = cy + rect_size * std::sin(theta);
    }

    SDL_SetRenderDrawColor(renderer, 255, 100, 100, 255);
    for (int i = 0; i < 4; i++) {
        SDL_RenderLine(renderer,
                       rect_corners[i].x, rect_corners[i].y,
                       rect_corners[(i + 1) % 4].x, rect_corners[(i + 1) % 4].y);
    }

    // 填充矩形（半透明）
    for (int i = 0; i < 4; i++) {
        SDL_FPoint tri[3] = {
            { cx, cy },
            { rect_corners[i].x, rect_corners[i].y },
            { rect_corners[(i + 1) % 4].x, rect_corners[(i + 1) % 4].y }
        };
        // SDL3 不支持直接填充三角形，这里用线框表示
    }

    // 4. 绘制脉冲圆圈
    float pulse = 0.5f + 0.5f * std::sin(time * 3.0f);
    float circle_radius = 50.0f + 30.0f * pulse;
    int num_segments = 32;

    SDL_SetRenderDrawColor(renderer, 100, 255, 100, 200);
    for (int i = 0; i < num_segments; i++) {
        float theta1 = 2.0f * 3.14159f * i / num_segments;
        float theta2 = 2.0f * 3.14159f * (i + 1) / num_segments;

        SDL_FPoint p1 = {
            cx + circle_radius * std::cos(theta1),
            cy + circle_radius * std::sin(theta1)
        };
        SDL_FPoint p2 = {
            cx + circle_radius * std::cos(theta2),
            cy + circle_radius * std::sin(theta2)
        };

        SDL_RenderLine(renderer, p1.x, p1.y, p2.x, p2.y);
    }

    // 5. 绘制第二个圆圈（反相旋转）
    float circle2_radius = 80.0f - 20.0f * pulse;
    SDL_SetRenderDrawColor(renderer, 100, 200, 255, 180);
    for (int i = 0; i < num_segments; i++) {
        float theta1 = -2.0f * 3.14159f * i / num_segments + time;  // 反向旋转
        float theta2 = -2.0f * 3.14159f * (i + 1) / num_segments + time;

        SDL_FPoint p1 = {
            cx + circle2_radius * std::cos(theta1),
            cy + circle2_radius * std::sin(theta1)
        };
        SDL_FPoint p2 = {
            cx + circle2_radius * std::cos(theta2),
            cy + circle2_radius * std::sin(theta2)
        };

        SDL_RenderLine(renderer, p1.x, p1.y, p2.x, p2.y);
    }

    // 6. 绘制中心十字
    float cross_size = 20.0f;
    SDL_SetRenderDrawColor(renderer, 255, 255, 100, 255);
    SDL_RenderLine(renderer, cx - cross_size, cy, cx + cross_size, cy);
    SDL_RenderLine(renderer, cx, cy - cross_size, cx, cy + cross_size);

    // 7. 绘制文字信息（使用线条绘制简单的文字）
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    float text_y = 20.0f;
    SDL_FRect text_rect = { 20.0f, text_y, 100.0f, 20.0f };
    SDL_RenderFillRect(renderer, &text_rect);  // 代表文字

    text_rect.y = text_y + 30.0f;
    text_rect.w = 150.0f;
    SDL_RenderFillRect(renderer, &text_rect);
}

// ================ ImGui GUI ================

void draw_gui() {
    // 主窗口
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_FirstUseEver);

    ImGui::Begin("SDL3 混合渲染示例", nullptr, ImGuiWindowFlags_NoCollapse);

    // 说明文本
    ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f),
                       "在 ImGui 中嵌入 SDL3 渲染的内容");
    ImGui::Separator();

    // ================ 可收起的 SDL3 渲染区域 ================
    if (ImGui::CollapsingHeader("SDL3 渲染区域", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (!g_render_texture) {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "纹理未初始化");
            return;
        }

        // 计算显示尺寸（保持宽高比）
        float available_width = ImGui::GetContentRegionAvail().x;
        float aspect_ratio = static_cast<float>(TEXTURE_HEIGHT) / static_cast<float>(TEXTURE_WIDTH);
        float display_height = available_width * aspect_ratio;
        display_height = std::min(display_height, 400.0f);  // 限制最大高度

        ImVec2 display_size(available_width, display_height);

        // 创建子窗口（用于裁剪和接收事件）
        ImGui::BeginChild("SDLRenderCanvas", display_size, true,
                          ImGuiWindowFlags_NoScrollbar);

        // 获取子窗口位置
        ImVec2 child_pos = ImGui::GetCursorScreenPos();

        // 创建不可见按钮（接收鼠标事件）
        ImGui::InvisibleButton("canvas", display_size);

        bool is_hovered = ImGui::IsItemHovered();

        // 处理鼠标输入
        if (is_hovered) {
            // 鼠标拖拽（平移）
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                g_is_dragging = true;
                g_drag_start_pos = ImGui::GetMousePos();
            }

            if (g_is_dragging && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                ImVec2 mouse_delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
                g_transform.offset_x += mouse_delta.x;
                g_transform.offset_y += mouse_delta.y;
                ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
            }

            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                g_is_dragging = false;
            }

            // 鼠标滚轮（缩放）
            float wheel = ImGui::GetIO().MouseWheel;
            if (wheel != 0.0f) {
                float zoom_factor = 1.1f;
                if (wheel > 0.0f) {
                    g_transform.scale *= zoom_factor;
                } else {
                    g_transform.scale /= zoom_factor;
                }
                // 限制缩放范围
                g_transform.scale = std::clamp(g_transform.scale, 0.1f, 10.0f);
            }
        }

        // 绘制纹理（应用变换）
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        // 计算变换后的位置
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

        ImGui::EndChild();

        // 显示变换信息
        ImGui::Spacing();
        if (ImGui::Button("重置视图##reset")) {
            g_transform.offset_x = 0.0f;
            g_transform.offset_y = 0.0f;
            g_transform.scale = 1.0f;
        }

        ImGui::SameLine();
        ImGui::Text("偏移: (%.0f, %.0f) | 缩放: %.2fx",
                    g_transform.offset_x, g_transform.offset_y, g_transform.scale);

        // 显示 FPS
        ImGui::Text("动画时间: %.2f 秒", g_animation_time);
    }

    ImGui::Separator();

    // 操作说明
    if (ImGui::CollapsingHeader("操作说明")) {
        ImGui::BulletText("鼠标左键拖拽：平移视图");
        ImGui::BulletText("鼠标滚轮：缩放视图");
        ImGui::BulletText("点击标题栏：收起/展开区域");
        ImGui::BulletText("点击重置按钮：恢复默认视图");
    }

    // 技术说明
    if (ImGui::CollapsingHeader("技术说明")) {
        ImGui::Text("渲染流程：");
        ImGui::BulletText("SDL3 渲染到离屏纹理");
        ImGui::BulletText("ImGui::Image() 显示纹理");
        ImGui::BulletText("ImGui 处理鼠标事件");
        ImGui::BulletText("手动应用平移和缩放变换");

        ImGui::Spacing();
        ImGui::Text("纹理信息：");
        ImGui::BulletText("尺寸: %dx%d", TEXTURE_WIDTH, TEXTURE_HEIGHT);
        ImGui::BulletText("格式: RGBA8888");
        ImGui::BulletText("访问模式: RENDER_TARGET");
    }

    ImGui::End();
}

// ================ 事件处理 ================

void handle_events() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        // ImGui 事件处理
        ImGui_ImplSDL3_ProcessEvent(&event);

        // 窗口关闭事件
        if (event.type == SDL_EVENT_QUIT) {
            g_running = false;
        }
        if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            if (event.window.windowID == SDL_GetWindowID(g_window)) {
                g_running = false;
            }
        }
    }
}
