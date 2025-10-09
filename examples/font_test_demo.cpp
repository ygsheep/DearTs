/**
 * @file font_test_demo.cpp
 * @brief 独立的SDL+ImGui字体渲染测试demo
 * @author DearTs Team
 * @date 2025
 */

#include <SDL.h>
#include <SDL_syswm.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <misc/freetype/imgui_freetype.h>
#include <iostream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif

// 全局变量
SDL_Window* g_window = nullptr;
SDL_Renderer* g_renderer = nullptr;
bool g_show_demo_window = true;
bool g_show_font_test = true;
ImFont* g_chinese_font = nullptr;
ImFont* g_default_font = nullptr;
ImFont* g_font_12px = nullptr;
ImFont* g_font_14px = nullptr;
ImFont* g_font_16px = nullptr;
ImFont* g_font_18px = nullptr;

/**
 * 初始化SDL和ImGui
 */
bool initialize() {
    // 初始化SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        std::cerr << "SDL初始化失败: " << SDL_GetError() << std::endl;
        return false;
    }

    // 创建窗口
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(
        SDL_WINDOW_OPENGL |
        SDL_WINDOW_RESIZABLE |
        SDL_WINDOW_ALLOW_HIGHDPI
    );

    g_window = SDL_CreateWindow(
        "DearTs 字体渲染测试 Demo",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1280,
        720,
        window_flags
    );

    if (!g_window) {
        std::cerr << "窗口创建失败: " << SDL_GetError() << std::endl;
        return false;
    }

    // 创建渲染器
    g_renderer = SDL_CreateRenderer(
        g_window,
        -1,
        SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED
    );

    if (!g_renderer) {
        std::cerr << "渲染器创建失败: " << SDL_GetError() << std::endl;
        return false;
    }

    // 设置ImGui上下文
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    // 注释掉docking功能，当前版本可能不支持
    // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // 设置字体缩放让字体更宽更清晰
    io.FontGlobalScale = 1.2f;

    // 设置ImGui样式
    ImGui::StyleColorsDark();

    // 初始化ImGui平台/渲染器后端
    ImGui_ImplSDL2_InitForSDLRenderer(g_window, g_renderer);
    ImGui_ImplSDLRenderer2_Init(g_renderer);

    return true;
}

/**
 * 加载字体
 */
bool loadFonts() {
    ImGuiIO& io = ImGui::GetIO();

    // 清除默认字体
    io.Fonts->Clear();

    // 关键：设置FreeType加载器
    #ifdef IMGUI_ENABLE_FREETYPE
    std::cout << "设置FreeType加载器..." << std::endl;
    const ImFontLoader* freetypeLoader = ImGuiFreeType::GetFontLoader();
    if (freetypeLoader != nullptr) {
        io.Fonts->SetFontLoader(freetypeLoader);
        std::cout << "✓ FreeType字体加载器设置成功" << std::endl;
    } else {
        std::cerr << "✗ 无法获取FreeType加载器" << std::endl;
    }
    #else
    std::cout << "编译时FreeType未启用，使用默认加载器" << std::endl;
    #endif

    // 获取可执行文件目录
    std::string fontPath = "resources/fonts/OPPOSans-M.ttf";

    // 检查字体文件是否存在
    FILE* fontFile = fopen(fontPath.c_str(), "rb");
    if (fontFile) {
        fclose(fontFile);
        std::cout << "找到字体文件: " << fontPath << std::endl;
    } else {
        std::cerr << "字体文件不存在: " << fontPath << std::endl;
        return false;
    }

    // 配置字体 - 测试不同设置
    ImFontConfig config;
    config.SizePixels = 16.0f;  // 稍大的字体便于观察

    // 测试1: 禁用FreeType优化
    config.FontLoaderFlags = 0;
    config.OversampleH = 1;
    config.OversampleV = 1;
    config.PixelSnapH = true;
    strcpy_s(config.Name, sizeof(config.Name), "default_no_freetype");

    g_default_font = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 16.0f, &config);

    // 测试2: 启用FreeType + NoHinting
    ImFontConfig config2;
    config2.SizePixels = 16.0f;
    config2.FontLoaderFlags = ImGuiFreeTypeLoaderFlags_NoHinting;
    config2.OversampleH = 3;
    config2.OversampleV = 1;
    config2.PixelSnapH = true;
    strcpy_s(config2.Name, sizeof(config2.Name), "freetype_no_hinting");

    static const ImWchar chinese_ranges[] = {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x2000, 0x206F, // General Punctuation
        0x3000, 0x30FF, // CJK Symbols and Punctuations, Hiragana, Katakana
        0x31F0, 0x31FF, // Katakana Phonetic Extensions
        0xFF00, 0xFFEF, // Half-width characters
        0x4e00, 0x9FAF, // CJK Ideograms
        0,
    };

    g_chinese_font = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 16.0f, &config2, chinese_ranges);

    if (!g_chinese_font) {
        std::cerr << "中文字体加载失败" << std::endl;
        return false;
    }

    // 测试3: 启用FreeType + LightHinting
    ImFontConfig config3;
    config3.SizePixels = 16.0f;
    config3.FontLoaderFlags = ImGuiFreeTypeLoaderFlags_LightHinting;
    config3.OversampleH = 2;
    config3.OversampleV = 1;
    config3.PixelSnapH = true;
    config3.MergeMode = true;  // 合并字体
    strcpy_s(config3.Name, sizeof(config3.Name), "freetype_light_hinting");

    io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 14.0f, &config3, chinese_ranges);

    // 加载不同大小的字体用于测试
    static const ImWchar chinese_ranges_test[] = {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x2000, 0x206F, // General Punctuation
        0x3000, 0x30FF, // CJK Symbols and Punctuations, Hiragana, Katakana
        0x31F0, 0x31FF, // Katakana Phonetic Extensions
        0xFF00, 0xFFEF, // Half-width characters
        0x4e00, 0x9FAF, // CJK Ideograms
        0,
    };

    // 12px 字体
    ImFontConfig config12;
    config12.SizePixels = 12.0f;
    config12.FontLoaderFlags = ImGuiFreeTypeLoaderFlags_NoHinting;
    config12.OversampleH = 2;
    config12.OversampleV = 1;
    config12.PixelSnapH = true;
    strcpy_s(config12.Name, sizeof(config12.Name), "font_12px");
    g_font_12px = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 12.0f, &config12, chinese_ranges_test);

    // 14px 字体
    ImFontConfig config14;
    config14.SizePixels = 14.0f;
    config14.FontLoaderFlags = ImGuiFreeTypeLoaderFlags_NoHinting;
    config14.OversampleH = 2;
    config14.OversampleV = 1;
    config14.PixelSnapH = true;
    strcpy_s(config14.Name, sizeof(config14.Name), "font_14px");
    g_font_14px = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 14.0f, &config14, chinese_ranges_test);

    // 16px 字体
    ImFontConfig config16;
    config16.SizePixels = 16.0f;
    config16.FontLoaderFlags = ImGuiFreeTypeLoaderFlags_NoHinting;
    config16.OversampleH = 2;
    config16.OversampleV = 1;
    config16.PixelSnapH = true;
    strcpy_s(config16.Name, sizeof(config16.Name), "font_16px");
    g_font_16px = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 16.0f, &config16, chinese_ranges_test);

    // 18px 字体
    ImFontConfig config18;
    config18.SizePixels = 18.0f;
    config18.FontLoaderFlags = ImGuiFreeTypeLoaderFlags_NoHinting;
    config18.OversampleH = 2;
    config18.OversampleV = 1;
    config18.PixelSnapH = true;
    strcpy_s(config18.Name, sizeof(config18.Name), "font_18px");
    g_font_18px = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 18.0f, &config18, chinese_ranges_test);

    // 构建字体图集
    bool atlasBuilt = io.Fonts->Build();
    if (!atlasBuilt) {
        std::cerr << "字体图集构建失败" << std::endl;
        return false;
    }

    std::cout << "字体加载完成，包括 12px, 14px, 16px, 18px 大小" << std::endl;
    return true;
}

/**
 * 渲染UI
 */
void renderUI() {
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // 简化的菜单栏（不使用docking）
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("选项")) {
            ImGui::MenuItem("字体测试", nullptr, &g_show_font_test);
            ImGui::MenuItem("ImGui演示", nullptr, &g_show_demo_window);
            ImGui::Separator();
            if (ImGui::MenuItem("退出")) {
                SDL_Event quit_event;
                quit_event.type = SDL_QUIT;
                SDL_PushEvent(&quit_event);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    // 字体测试窗口
    if (g_show_font_test) {
        ImGui::Begin("字体渲染测试", &g_show_font_test);

        ImGui::Text("字体渲染质量测试");
        ImGui::Separator();

        // 测试默认字体
        if (g_default_font) {
            ImGui::PushFont(g_default_font);
            ImGui::Text("默认字体 (No FreeType): Hello World! 你好世界！");
            ImGui::PopFont();
        }

        ImGui::Separator();

        // 测试FreeType字体
        if (g_chinese_font) {
            ImGui::PushFont(g_chinese_font);
            ImGui::Text("FreeType字体 (NoHinting): Hello World! 你好世界！");
            ImGui::PopFont();
        }

        ImGui::Separator();

        // 测试不同大小的文本
        ImGui::Text("不同大小的中文文本测试:");

        if (g_font_12px) {
            ImGui::PushFont(g_font_12px);
            ImGui::Text("12px: 这是一段测试文本，用来检查字体清晰度");
            ImGui::PopFont();
        }

        if (g_font_14px) {
            ImGui::PushFont(g_font_14px);
            ImGui::Text("14px: 这是一段测试文本，用来检查字体清晰度");
            ImGui::PopFont();
        }

        if (g_font_16px) {
            ImGui::PushFont(g_font_16px);
            ImGui::Text("16px: 这是一段测试文本，用来检查字体清晰度");
            ImGui::PopFont();
        }

        if (g_font_18px) {
            ImGui::PushFont(g_font_18px);
            ImGui::Text("18px: 这是一段测试文本，用来检查字体清晰度");
            ImGui::PopFont();
        }

        ImGui::Separator();

        // FreeType信息
        ImGui::Text("FreeType状态信息:");
        #ifdef IMGUI_ENABLE_FREETYPE
        ImGui::Text("FreeType支持: ✓ 已启用");
        #else
        ImGui::Text("FreeType支持: ✗ 未启用");
        #endif

        ImGuiIO& io = ImGui::GetIO();
        ImGui::Text("全局字体缩放: %.2f", io.FontGlobalScale);
        ImGui::Text("当前FPS: %.1f", ImGui::GetIO().Framerate);

        ImGui::Separator();

        // 字体对比
        ImGui::Text("字体对比测试:");
        ImGui::Text("English: The quick brown fox jumps over the lazy dog");
        ImGui::Text("中文: 春眠不觉晓，处处闻啼鸟。夜来风雨声，花落知多少。");
        ImGui::Text("数字: 0123456789 一二三四五六七八九零");
        ImGui::Text("符号: ！@#￥%……&*（）——+");

        ImGui::End();
    }

    // ImGui演示窗口
    if (g_show_demo_window) {
        ImGui::ShowDemoWindow(&g_show_demo_window);
    }

    // 渲染
    ImGui::Render();
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), g_renderer);
}

/**
 * 清理资源
 */
void cleanup() {
    // 清理ImGui
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    // 清理SDL
    if (g_renderer) {
        SDL_DestroyRenderer(g_renderer);
        g_renderer = nullptr;
    }

    if (g_window) {
        SDL_DestroyWindow(g_window);
        g_window = nullptr;
    }

    SDL_Quit();
}

/**
 * 主函数
 */
int main(int argc, char* argv[]) {
    std::cout << "=== DearTs 字体渲染测试 Demo ===" << std::endl;

    // 初始化
    if (!initialize()) {
        std::cerr << "初始化失败" << std::endl;
        return -1;
    }

    // 加载字体
    if (!loadFonts()) {
        std::cerr << "字体加载失败" << std::endl;
        cleanup();
        return -1;
    }

    std::cout << "初始化完成，开始运行..." << std::endl;

    // 主循环
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }

        // 清除屏幕
        SDL_SetRenderDrawColor(g_renderer, 45, 45, 48, 255);
        SDL_RenderClear(g_renderer);

        // 渲染UI
        renderUI();

        // 呈现
        SDL_RenderPresent(g_renderer);
    }

    std::cout << "程序正常退出" << std::endl;

    // 清理
    cleanup();
    return 0;
}