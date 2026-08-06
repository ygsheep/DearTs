/**
 * @file main.cpp
 * @brief Markdown 渲染示例程序
 * @details 展示如何使用 imgui_markdown 库在 ImGui 中渲染 Markdown 内容
 */

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <SDL3/SDL.h>
#include <iostream>
#include <format>
#include <string>
#include <vector>
#include <chrono>
#include <thread>

// 包含 imgui_markdown 库
#include "imgui_markdown.h"

// Windows 特定头文件
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Shellapi.h>
#endif

// 全局变量
SDL_Window* g_window = nullptr;
SDL_Renderer* g_renderer = nullptr;
bool g_running = true;

// Markdown 字体
ImFont* g_font_H1 = nullptr;
ImFont* g_font_H2 = nullptr;
ImFont* g_font_H3 = nullptr;

// 示例 Markdown 内容索引
static int g_current_example = 0;

/**
 * @brief Markdown 链接回调 - 打开链接
 */
void LinkCallback(ImGui::MarkdownLinkCallbackData data_) {
    std::string url(data_.link, data_.linkLength);
    std::cout << "点击链接: " << url << std::endl;

#ifdef _WIN32
    // Windows 下打开浏览器
    ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
#elif __APPLE__
    // macOS 下打开浏览器
    std::string command = "open " + url;
    system(command.c_str());
#else
    // Linux 下打开浏览器
    std::string command = "xdg-open " + url;
    system(command.c_str());
#endif
}

/**
 * @brief Markdown 图片回调 - 加载图片
 */
inline ImGui::MarkdownImageData ImageCallback(ImGui::MarkdownLinkCallbackData data_) {
    // 简单示例：使用 ImGui 字体纹理作为占位符
    // 实际应用中应该根据 data_.link 加载真实图片

#ifdef IMGUI_HAS_TEXTURES
    ImTextureID image = ImGui::GetIO().Fonts->TexRef.GetTexID();
#else
    ImTextureID image = ImGui::GetIO().Fonts->TexID;
#endif

    ImGui::MarkdownImageData imageData;
    imageData.isValid = true;
    imageData.useLinkCallback = false;
    imageData.user_texture_id = image;
    imageData.size = ImVec2(100.0f, 100.0f);

    // 图片自适应宽度
    ImVec2 const contentSize = ImGui::GetContentRegionAvail();
    if (imageData.size.x > contentSize.x) {
        float const ratio = imageData.size.y / imageData.size.x;
        imageData.size.x = contentSize.x;
        imageData.size.y = contentSize.x * ratio;
    }

    return imageData;
}

/**
 * @brief Markdown 格式回调 - 自定义样式
 */
void MarkdownFormatCallback(const ImGui::MarkdownFormatInfo& markdownFormatInfo_, bool start_) {
    // 调用默认格式回调
    ImGui::defaultMarkdownFormatCallback(markdownFormatInfo_, start_);

    // 自定义 H2 标题颜色
    switch (markdownFormatInfo_.type) {
    case ImGui::MarkdownFormatType::HEADING: {
        if (markdownFormatInfo_.level == 2) {
            if (start_) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.6f, 1.0f, 1.0f));
            } else {
                ImGui::PopStyleColor();
            }
        }
        break;
    }
    default:
        break;
    }
}

/**
 * @brief 渲染 Markdown 内容
 */
void RenderMarkdown(const std::string& markdown_) {
    static ImGui::MarkdownConfig mdConfig;

    mdConfig.linkCallback = LinkCallback;
    mdConfig.tooltipCallback = ImGui::defaultMarkdownTooltipCallback;
    mdConfig.imageCallback = ImageCallback;
    mdConfig.linkIcon = "(Link)";
    mdConfig.userData = nullptr;
    mdConfig.formatCallback = MarkdownFormatCallback;

    // 配置标题字体
#ifdef IMGUI_HAS_TEXTURES
    mdConfig.headingFormats[0] = { g_font_H1, true,  24.0f };
    mdConfig.headingFormats[1] = { g_font_H2, true,  20.0f };
    mdConfig.headingFormats[2] = { g_font_H3, false, 18.0f };
#else
    mdConfig.headingFormats[0] = { g_font_H1, true };
    mdConfig.headingFormats[1] = { g_font_H2, true };
    mdConfig.headingFormats[2] = { g_font_H3, false };
#endif

    ImGui::Markdown(markdown_.c_str(), markdown_.length(), mdConfig);
}

// ============================================================================
// 示例 Markdown 内容
// ============================================================================

const std::string g_example_basic = R"(
# 基本语法示例

欢迎使用 **imgui_markdown**！这是一个用于 Dear ImGui 的 Markdown 渲染库。

## 文本样式

支持 *斜体* 和 **粗体** 文本。

## 列表

无序列表示例：
  * 第一项
  * 第二项
    * 子项 2.1
    * 子项 2.2
  * 第三项

## 缩进

普通文本
  缩进一级
    缩进二级
      缩进三级

## 水平线

使用三个星号或下划线创建水平线：

___

***

## 链接

访问 [Dear ImGui GitHub](https://github.com/ocornut/imgui) 了解更多。

访问 [imgui_markdown GitHub](https://github.com/juliettef/imgui_markdown) 了解本项目。
)";

const std::string g_example_documentation = R"(
# imgui_markdown 文档

Markdown For Dear ImGui 是一个宽松授权的单头文件库。

## 特性

  * 自动文本换行
  * 支持标题 H1, H2, H3
  * 支持强调（斜体/粗体）
  * 支持多级缩进
  * 支持无序列表和子列表
  * 支持链接和图片
  * 支持水平线

## 标题语法

```
# H1 标题
## H2 标题
### H3 标题
```

## 强调语法

```
*斜体*
_斜体_
**粗体**
__粗体__
```

## 列表语法

  * 列表项 1
    * 子项 1.1
  * 列表项 2

## 链接语法

```
[链接文字](https://example.com)
```

## 图片语法

```
![图片替代文字](图片标识符)
```

## 水平线语法

```
***
___
```
)";

const std::string g_example_chinese = R"(
# 中文文档示例

这是一个展示中文 Markdown 渲染的示例。

## 标题层级

### 三级标题

你可以看到中文显示正常，包括各种标点符号。

## 文本格式

这是 **粗体文字**，这是 *斜体文字*。

你还可以混合使用 ***粗斜体***。

## 列表

中文列表支持：
  * 第一项内容
  * 第二项内容
    * 子项 A
    * 子项 B
  * 第三项内容

## 代码块

虽然 imgui_markdown 不直接支持代码块，但你可以使用缩进：

    void hello() {
        printf("Hello, World!\n");
    }

## 链接

访问 [百度](https://www.baidu.com) 搜索更多信息。

## 技术栈示例

  * SDL3 - 窗口和输入
  * ImGui - 即时模式 GUI
  * imgui_markdown - Markdown 渲染
  * C++20 - 现代 C++ 特性
)";

const std::string g_example_readme = R"(
# DearTs Framework

> 基于 SDL3 + ImGui 的现代 C++20 应用框架

## 简介

DearTs Framework 是一个功能丰富的 C++ 应用程序开发框架，采用插件架构设计。

## 核心特性

### 🎨 现代化设计
  * **C++20** - 使用最新标准特性
  * **SDL3** - 跨平台窗口和输入管理
  * **ImGui** - 灵活的即时模式 GUI

### 🔌 插件系统
  * **IPlugin 接口** - 统一的插件抽象
  * **生命周期管理** - 加载、启用、禁用、卸载
  * **依赖解析** - 自动处理插件依赖关系

### 📡 事件驱动
  * **类型安全的事件总线** - 编译时类型检查
  * **RAII 订阅令牌** - 自动资源管理

### 🎯 内容注册表
  * **命令注册** - 统一的命令管理
  * **视图注册** - 可停靠窗口系统
  * **工具注册** - 可扩展工具集

## 快速开始

```cpp
#include "core/plugin/plugin.h"

class MyPlugin : public IPlugin {
public:
    PluginInfo get_info() const override {
        return PluginInfo{
            .name = "MyPlugin",
            .author = "Your Name",
            .description = "My awesome plugin",
            .version = "1.0.0"
        };
    }

    Result<void, std::string> on_load() override {
        LOG_INFO("Plugin loaded!");
        return Result::ok();
    }
};
```

## 文档

  * [插件系统指南](../docs/plugin_system_guide.md)
  * [API 参考](../dearts-dev/references/)

## 许可证

MIT License
)";

const std::string g_example_live_edit = R"(
# 实时编辑示例

> 在左侧输入框中编辑 Markdown，右侧实时预览效果！

## 支持的功能

  * **标题** - H1, H2, H3
  * **强调** - *斜体* 和 **粗体**
  * **列表** - 无序列表和子列表
  * **链接** - 可点击的超链接
  * **缩进** - 多级缩进文本

---

试着在左侧编辑框输入一些 Markdown 语法，看看效果如何！
)";

// 获取当前示例内容
const std::string& GetCurrentExample() {
    switch (g_current_example) {
    case 0: return g_example_basic;
    case 1: return g_example_documentation;
    case 2: return g_example_chinese;
    case 3: return g_example_readme;
    case 4: return g_example_live_edit;
    default: return g_example_basic;
    }
}

// 获取示例名称
const char* GetExampleName(int index) {
    static const char* names[] = {
        "基本语法",
        "文档说明",
        "中文支持",
        "README 风格",
        "实时编辑"
    };
    if (index >= 0 && index < 5) {
        return names[index];
    }
    return "未知";
}

// 实时编辑缓冲区
static char g_edit_buffer[4096] = "";

/**
 * @brief 初始化 SDL 和 ImGui
 */
bool init() {
    // 初始化 SDL
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return false;
    }

    // 创建窗口
    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, "Markdown 渲染示例");
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, 1280);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, 720);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);

    g_window = SDL_CreateWindowWithProperties(props);
    SDL_DestroyProperties(props);

    if (!g_window) {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        return false;
    }

    // 创建渲染器
    g_renderer = SDL_CreateRenderer(g_window, nullptr);
    if (!g_renderer) {
        std::cerr << "Failed to create renderer: " << SDL_GetError() << std::endl;
        return false;
    }

    // 初始化 ImGui
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // 加载中文字体
    io.Fonts->Clear();

    // 字体配置
    ImFontConfig font_config;
    font_config.OversampleH = 2;
    font_config.OversampleV = 2;
    font_config.PixelSnapH = true;

    // 尝试加载字体
    static const char* font_paths[] = {
        "resources/fonts/OPPOSans-M.ttf",
        "resources/fonts/Noto nerd.ttf",
        "../resources/fonts/OPPOSans-M.ttf",
        "../../resources/fonts/OPPOSans-M.ttf"
    };

    bool font_loaded = false;
    for (const char* font_path : font_paths) {
        // 基础字体
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

            font_config.MergeMode = true;

            // 加载更大尺寸的字体用于标题
            g_font_H3 = io.Fonts->AddFontFromFileTTF(font_path, 18.0f, &font_config, io.Fonts->GetGlyphRangesChineseFull());
            g_font_H2 = io.Fonts->AddFontFromFileTTF(font_path, 20.0f, &font_config, io.Fonts->GetGlyphRangesChineseFull());
            g_font_H1 = io.Fonts->AddFontFromFileTTF(font_path, 24.0f, &font_config, io.Fonts->GetGlyphRangesChineseFull());

            font_config.MergeMode = false;
            break;
        }
    }

    if (!font_loaded) {
        std::cerr << "警告：未能加载中文字体，使用默认字体" << std::endl;
        io.Fonts->AddFontDefault();
        g_font_H1 = g_font_H2 = g_font_H3 = io.Fonts->Fonts[0];
    }

    // 初始化 ImGui SDL3 后端
    ImGui_ImplSDL3_InitForSDLRenderer(g_window, g_renderer);
    ImGui_ImplSDLRenderer3_Init(g_renderer);

    // 设置样式
    ImGui::StyleColorsDark();

    // 初始化编辑缓冲区
    strncpy_s(g_edit_buffer, sizeof(g_edit_buffer), g_example_live_edit.c_str(), _TRUNCATE);

    std::cout << "初始化成功！" << std::endl;
    return true;
}

/**
 * @brief 清理资源
 */
void cleanup() {
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    if (g_renderer) {
        SDL_DestroyRenderer(g_renderer);
    }

    if (g_window) {
        SDL_DestroyWindow(g_window);
    }

    SDL_Quit();
}

/**
 * @brief 渲染主界面
 */
void render_ui() {
    // 主窗口
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar;

    ImGui::Begin("Markdown 渲染示例", nullptr, window_flags);

    // 菜单栏
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("示例")) {
            for (int i = 0; i < 5; i++) {
                if (ImGui::MenuItem(GetExampleName(i), nullptr, g_current_example == i)) {
                    g_current_example = i;
                    if (i == 4) {
                        // 实时编辑模式，初始化缓冲区
                        strncpy_s(g_edit_buffer, sizeof(g_edit_buffer), g_example_live_edit.c_str(), _TRUNCATE);
                    }
                }
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("帮助")) {
            if (ImGui::MenuItem("关于 imgui_markdown")) {
                ImGui::OpenPopup("About");
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    // 关于对话框
    if (ImGui::BeginPopupModal("About", nullptr)) {
        ImGui::Text("imgui_markdown 示例程序");
        ImGui::Separator();
        ImGui::Text("Markdown For Dear ImGui");
        ImGui::Text("https://github.com/juliettef/imgui_markdown");
        ImGui::Spacing();
        if (ImGui::Button("关闭")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // 示例选择按钮
    ImGui::Text("选择示例：");
    for (int i = 0; i < 5; i++) {
        if (i > 0) ImGui::SameLine();
        if (ImGui::Button(GetExampleName(i))) {
            g_current_example = i;
            if (i == 4) {
                strncpy_s(g_edit_buffer, sizeof(g_edit_buffer), g_example_live_edit.c_str(), _TRUNCATE);
            }
        }
    }

    // 测试 ChatManager 风格的嵌套渲染
    ImGui::Spacing();
    if (ImGui::Button("测试嵌套窗口（ChatManager 风格）")) {
        ImGui::OpenPopup("ChatManager 测试");
    }

    // ChatManager 风格测试窗口
    if (ImGui::BeginPopupModal("ChatManager 测试", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("测试 BeginChild + WindowPadding 嵌套渲染");

        // 模拟 ChatManager 的消息气泡渲染
        const char* test_markdown = R"(
# AI 回复

这是一条**包含 Markdown** 的回复：

## 缩进文本
  这是缩进一级的文本
    这是缩进二级的文本

## 列表
  * 列表项 1
  * 列表项 2
    * 列表项 2a（嵌套）
  * 列表项 3

## 强调
*斜体文本*
**粗体文本**

分隔线：
___
***

[点击这里打开链接](https://github.com)
)";

        // 左边距
        const float left_margin = 20.0f;
        const float padding_x = 16.0f;
        const float padding_y = 12.0f;
        const float bubble_width = 500.0f;

        // 保存原始光标位置
        ImVec2 original_cursor = ImGui::GetCursorPos();

        // 开始渲染（使用 ChatManager 相同的方式）
        ImGui::SetCursorPos(ImVec2(original_cursor.x + left_margin, original_cursor.y));

        // 使用 WindowPadding 添加内边距
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padding_x, padding_y));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));

        ImGui::BeginChild("##test_child", ImVec2(bubble_width, 0), false,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        // 渲染 Markdown
        static ImGui::MarkdownConfig test_mdConfig;
        test_mdConfig.linkCallback = LinkCallback;
        test_mdConfig.tooltipCallback = ImGui::defaultMarkdownTooltipCallback;
        test_mdConfig.imageCallback = ImageCallback;
        test_mdConfig.userData = nullptr;

        #ifdef IMGUI_HAS_TEXTURES
            test_mdConfig.headingFormats[0] = { g_font_H1, true,  24.0f };
            test_mdConfig.headingFormats[1] = { g_font_H2, true,  20.0f };
            test_mdConfig.headingFormats[2] = { g_font_H3, false, 18.0f };
        #else
            test_mdConfig.headingFormats[0] = { g_font_H1, true };
            test_mdConfig.headingFormats[1] = { g_font_H2, true };
            test_mdConfig.headingFormats[2] = { g_font_H3, false };
        #endif

        ImGui::Markdown(test_markdown, strlen(test_markdown), test_mdConfig);

        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(3);  // WindowPadding, ChildRounding, ChildBorderSize

        ImGui::Spacing();
        if (ImGui::Button("关闭")) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    ImGui::Spacing();
    ImGui::Separator();

    if (g_current_example == 4) {
        // 实时编辑模式 - 分屏布局
        ImGui::Columns(2, "EditPreview", true);

        // 左侧：编辑框
        ImGui::TextColored(ImVec4(0.3f, 0.6f, 1.0f, 1.0f), "编辑 Markdown：");
        ImGui::InputTextMultiline(
            "##Edit",
            g_edit_buffer,
            sizeof(g_edit_buffer),
            ImVec2(-1, -ImGui::GetFrameHeightWithSpacing()),
            ImGuiInputTextFlags_AllowTabInput
        );

        ImGui::NextColumn();

        // 右侧：预览
        ImGui::TextColored(ImVec4(0.3f, 0.6f, 1.0f, 1.0f), "预览：");
        ImGui::BeginChild("Preview", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), true);
        RenderMarkdown(std::string(g_edit_buffer));
        ImGui::EndChild();

        ImGui::Columns(1);
    } else {
        // 普通模式 - 显示预定义内容
        ImGui::BeginChild("MarkdownContent", ImVec2(0, 0), true);
        RenderMarkdown(GetCurrentExample());
        ImGui::EndChild();
    }

    ImGui::End();

    // 信息窗口
    ImGui::Begin("信息");

    ImGui::Text("当前示例: %s", GetExampleName(g_current_example));
    ImGui::Separator();
    ImGui::Text("统计信息：");
    ImGui::BulletText("ImGui 版本: %s", ImGui::GetVersion());
    ImGui::BulletText("字体: %s", g_font_H1 ? "已加载" : "使用默认");

    ImGui::Spacing();

    ImGui::Text("支持的 Markdown 语法：");
    ImGui::BulletText("标题 H1, H2, H3");
    ImGui::BulletText("强调 *斜体* 和 **粗体**");
    ImGui::BulletText("无序列表和子列表");
    ImGui::BulletText("多级缩进");
    ImGui::BulletText("链接 [文字](url)");
    ImGui::BulletText("水平线 *** 和 ___");

    ImGui::End();
}

/**
 * @brief 主循环
 */
void main_loop() {
    SDL_Event event;
    auto last_time = std::chrono::steady_clock::now();

    while (g_running) {
        // 计算 delta time
        auto current_time = std::chrono::steady_clock::now();
        std::chrono::duration<float> duration = current_time - last_time;
        last_time = current_time;

        // 处理事件
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);

            if (event.type == SDL_EVENT_QUIT) {
                g_running = false;
            }
        }

        // 开始 ImGui 帧
        ImGui_ImplSDL3_NewFrame();
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui::NewFrame();

        // 渲染 UI
        render_ui();

        // 渲染 ImGui
        ImGui::Render();

        // 清屏
        SDL_SetRenderDrawColor(g_renderer, 30, 30, 30, 255);
        SDL_RenderClear(g_renderer);

        // 渲染 ImGui
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), g_renderer);
        SDL_RenderPresent(g_renderer);

        // 限制帧率
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

int main(int argc, char* argv[]) {
    std::cout << "====================================" << std::endl;
    std::cout << "  Markdown 渲染示例程序" << std::endl;
    std::cout << "  imgui_markdown Demo" << std::endl;
    std::cout << "====================================" << std::endl;
    std::cout << std::endl;

    std::cout << "本示例展示 imgui_markdown 库的功能：" << std::endl;
    std::cout << "- 基本语法渲染" << std::endl;
    std::cout << "- 中文支持" << std::endl;
    std::cout << "- 实时编辑预览" << std::endl;
    std::cout << "- 自定义样式" << std::endl;
    std::cout << std::endl;

    // 初始化
    if (!init()) {
        std::cerr << "初始化失败！" << std::endl;
        return 1;
    }

    // 主循环
    main_loop();

    // 清理
    cleanup();

    std::cout << "程序正常退出" << std::endl;
    return 0;
}
