#include "imgui_layer.h"
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <SDL3/SDL_render.h>
#include "liblogger/logger.h"

namespace DearTs {
namespace Core {
namespace UI {

struct ImGuiLayer::Impl {
    ImGuiContext* context = nullptr;
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    float font_scale = 1.0f;
};

ImGuiLayer::ImGuiLayer()
    : m_impl(std::make_unique<Impl>()) {
}

ImGuiLayer::~ImGuiLayer() {
    shutdown();
}

bool ImGuiLayer::initialize(SDL_Window* window) {
    if (!window) {
        LOG_ERROR("Failed to initialize ImGuiLayer: null window");
        return false;
    }

    m_impl->window = window;

    // 获取渲染器
    m_impl->renderer = SDL_GetRenderer(window);
    if (!m_impl->renderer) {
        LOG_ERROR("Failed to get SDL renderer from window");
        return false;
    }

    // 创建 ImGui 上下文
    IMGUI_CHECKVERSION();
    m_impl->context = ImGui::CreateContext();
    if (!m_impl->context) {
        LOG_ERROR("Failed to create ImGui context");
        return false;
    }

    ImGui::SetCurrentContext(m_impl->context);

    // 配置 ImGui
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // 启用键盘控制
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // 启用游戏手柄控制
    // 注意：在 ImGui 1.92.x 中，停靠和多视口功能已默认启用，不再需要设置对应的 ConfigFlags

    // 设置ImGui样式
    ImGui::StyleColorsDark();

    // 初始化 ImGui SDL3 后端 (针对 SDLRenderer)
    if (!ImGui_ImplSDL3_InitForSDLRenderer(m_impl->window, m_impl->renderer)) {
        LOG_ERROR("Failed to initialize ImGui SDL3 backend");
        return false;
    }

    // 初始化 ImGui SDLRenderer3 后端
    if (!ImGui_ImplSDLRenderer3_Init(m_impl->renderer)) {
        LOG_ERROR("Failed to initialize ImGui SDLRenderer3 backend");
        return false;
    }

    LOG_INFO("ImGui layer initialized successfully");
    return true;
}

void ImGuiLayer::shutdown() {
    if (!m_impl->context) {
        return;
    }

    // 清理后端
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();

    // 销毁上下文
    if (ImGui::GetCurrentContext() == m_impl->context) {
        ImGui::DestroyContext();
    }

    m_impl->context = nullptr;
    m_impl->window = nullptr;
    m_impl->renderer = nullptr;

    LOG_INFO("ImGui layer shut down");
}

void ImGuiLayer::begin_frame() {
    if (!m_impl->context) {
        return;
    }

    ImGui::SetCurrentContext(m_impl->context);

    // 开始新帧
    ImGui_ImplSDL3_NewFrame();
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui::NewFrame();
}

void ImGuiLayer::render() {
    if (!m_impl->context) {
        return;
    }

    ImGui::SetCurrentContext(m_impl->context);

    // 渲染 ImGui
    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), m_impl->renderer);
}

bool ImGuiLayer::process_event(const SDL_Event& event) {
    if (!m_impl->context) {
        return false;
    }

    ImGui::SetCurrentContext(m_impl->context);

    // 将事件传递给 ImGui
    return ImGui_ImplSDL3_ProcessEvent(&event);
}

ImGuiContext* ImGuiLayer::get_context() const {
    return m_impl->context;
}

void ImGuiLayer::set_font_scale(float scale) {
    m_impl->font_scale = scale;

    if (m_impl->context) {
        ImGui::SetCurrentContext(m_impl->context);
        ImGuiIO& io = ImGui::GetIO();
        io.FontGlobalScale = scale;
        LOG_DEBUG("ImGui font scale set to {}", scale);
    }
}

float ImGuiLayer::get_font_scale() const {
    return m_impl->font_scale;
}

} // namespace UI
} // namespace Core
} // namespace DearTs
