/**
 * @file core.cpp
 * @brief DearTs核心系统实现文件
 * @details 实现核心系统的初始化、关闭和信息查询功能
 * @author DearTs Team
 * @date 2024
 */

#include "core.h"
#include "utils/config_manager.h"
#include "utils/profiler.h"
#include "events/event_system.h"
#include "window/window_manager.h"
#include "app/application_manager.h"
#include "render/renderer.h"
#include "input/input_manager.h"
#include "resource/resource_manager.h"
#include "audio/audio_manager.h"
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>

namespace DearTs {

// 全局状态
static bool g_coreInitialized = false;
static std::string g_buildInfo;

/**
 * @brief 生成构建信息字符串
 * @return 构建信息
 */
static std::string GenerateBuildInfo() {
    std::stringstream ss;
    ss << "DearTs Core " << Version::STRING << "\n";
    ss << "Build Date: " << __DATE__ << " " << __TIME__ << "\n";
    
    // 编译器信息
#ifdef DEARTS_COMPILER_MSVC
    ss << "Compiler: Microsoft Visual C++ " << _MSC_VER << "\n";
#elif defined(DEARTS_COMPILER_GCC)
    ss << "Compiler: GCC " << __GNUC__ << "." << __GNUC_MINOR__ << "." << __GNUC_PATCHLEVEL__ << "\n";
#elif defined(DEARTS_COMPILER_CLANG)
    ss << "Compiler: Clang " << __clang_major__ << "." << __clang_minor__ << "." << __clang_patchlevel__ << "\n";
#endif
    
    // 平台信息
#ifdef DEARTS_PLATFORM_WINDOWS
    ss << "Platform: Windows\n";
#elif defined(DEARTS_PLATFORM_LINUX)
    ss << "Platform: Linux\n";
#elif defined(DEARTS_PLATFORM_MACOS)
    ss << "Platform: macOS\n";
#endif
    
    // 架构信息
#ifdef _WIN64
    ss << "Architecture: x64\n";
#elif defined(_WIN32)
    ss << "Architecture: x86\n";
#elif defined(__x86_64__)
    ss << "Architecture: x64\n";
#elif defined(__i386__)
    ss << "Architecture: x86\n";
#elif defined(__aarch64__)
    ss << "Architecture: ARM64\n";
#elif defined(__arm__)
    ss << "Architecture: ARM\n";
#endif
    
    // 构建配置
#ifdef DEARTS_DEBUG
    ss << "Configuration: Debug\n";
#else
    ss << "Configuration: Release\n";
#endif
    
    // SDL版本信息
    SDL_version compiled, linked;
    SDL_VERSION(&compiled);
    SDL_GetVersion(&linked);
    ss << "SDL Compiled: " << (int)compiled.major << "." << (int)compiled.minor << "." << (int)compiled.patch << "\n";
    ss << "SDL Linked: " << (int)linked.major << "." << (int)linked.minor << "." << (int)linked.patch << "\n";
    
    return ss.str();
}

bool InitializeCore(const std::string& config) {
    if (g_coreInitialized) {
        // 使用新的日志API
        DearTs::Utils::getLogger().warn("核心系统已初始化");
        return true;
    }
    
    try {
        // 生成构建信息
        g_buildInfo = GenerateBuildInfo();
        
        // 初始化日志系统 - 启用文件输出
        auto& logger = DearTs::Utils::getLogger();
        logger.setLevel(DearTs::Utils::LogLevel::LOG_INFO);
        logger.enableFileOutput("logs/dearts.log");
        logger.info("正在初始化DearTs核心系统...");
        
        // 初始化配置管理器
        if (!config.empty()) {
            Core::Utils::ConfigManager::getInstance().loadFromFile(config);
        }
        
        // 初始化性能分析器
        Core::Utils::Profiler::getInstance().initialize();
        
        // 初始化SDL
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC | SDL_INIT_TIMER) < 0) {
            DearTs::Utils::getLogger().error("SDL初始化失败: " + std::string(SDL_GetError()));
            return false;
        }
        
        // 初始化SDL_image
        int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG | IMG_INIT_TIF | IMG_INIT_WEBP;
        if (!(IMG_Init(imgFlags) & imgFlags)) {
            DearTs::Utils::getLogger().error("SDL_image初始化失败: " + std::string(IMG_GetError()));
            SDL_Quit();
            return false;
        }
        
        // 初始化SDL_mixer
        if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
            DearTs::Utils::getLogger().error("SDL_mixer初始化失败: " + std::string(Mix_GetError()));
            IMG_Quit();
            SDL_Quit();
            return false;
        }
        
        // 初始化SDL_ttf
        if (TTF_Init() == -1) {
            DearTs::Utils::getLogger().error("SDL_ttf初始化失败: " + std::string(TTF_GetError()));
            Mix_CloseAudio();
            IMG_Quit();
            SDL_Quit();
            return false;
        }
        
        // 初始化事件系统
        Core::Events::EventSystem::getInstance()->initialize();
        
        // 初始化窗口管理器
        Core::Window::WindowManager::getInstance().initialize();
        
        // 初始化渲染管理器
        Core::Render::RenderManager::getInstance().initialize();
        
        // 初始化输入管理器
        Core::Input::InputManager::getInstance().initialize();
        
        // 初始化资源管理器 - 将在窗口创建后初始化
        // Core::Resource::ResourceManager::getInstance()->initialize(nullptr);
        
        // 初始化音频管理器
        Core::Audio::AudioManager::getInstance().initialize();
        
        // 初始化应用程序管理器
        DearTs::Core::App::ApplicationManager::getInstance().initialize();
        
        g_coreInitialized = true;
        DearTs::Utils::getLogger().info("DearTs核心系统初始化成功");
        DearTs::Utils::getLogger().info("构建信息:\n" + g_buildInfo);
        
        return true;
    }
    catch (const std::exception& e) {
        DearTs::Utils::getLogger().fatal("核心系统初始化失败: " + std::string(e.what()));
        return false;
    }
}

void ShutdownCore() {
    if (!g_coreInitialized) {
        return;
    }
    
    // 使用新的日志API
    DearTs::Utils::getLogger().info("Shutting down DearTs Core System...");
    
    try {
        // 按相反顺序关闭各个管理器
        DearTs::Utils::getLogger().info("Shutting down ApplicationManager...");
        DearTs::Core::App::ApplicationManager::getInstance().shutdown();
        DearTs::Utils::getLogger().info("Shutting down AudioManager...");
        DearTs::Core::Audio::AudioManager::getInstance().shutdown();
        // 关闭资源管理器
        DearTs::Utils::getLogger().info("Shutting down ResourceManager...");
        DearTs::Core::Resource::ResourceManager::getInstance()->shutdown();
        DearTs::Utils::getLogger().info("Shutting down InputManager...");
        DearTs::Core::Input::InputManager::getInstance().shutdown();
        DearTs::Utils::getLogger().info("Shutting down RenderManager...");
        DearTs::Core::Render::RenderManager::getInstance().shutdown();
        DearTs::Utils::getLogger().info("Shutting down WindowManager...");
        DearTs::Core::Window::WindowManager::getInstance().shutdown();
        DearTs::Utils::getLogger().info("Shutting down EventSystem...");
        DearTs::Core::Events::EventSystem::getInstance()->shutdown();
        
        // 关闭SDL子系统
        DearTs::Utils::getLogger().info("Shutting down SDL subsystems...");
        TTF_Quit();
        Mix_CloseAudio();
        IMG_Quit();
        SDL_Quit();
        
        // 关闭性能分析器
        DearTs::Utils::getLogger().info("Shutting down Profiler...");
        Core::Utils::Profiler::getInstance().shutdown();
        
        // 关闭配置管理器
        DearTs::Utils::getLogger().info("Saving config and shutting down ConfigManager...");
        Core::Utils::ConfigManager::getInstance().saveToFile("config.json");
        
        std::cout << "DearTs Core System shut down successfully" << std::endl;
        
        g_coreInitialized = false;
        DearTs::Utils::getLogger().info("DearTs Core System shut down completed");
    }
    catch (const std::exception& e) {
        // 在关闭过程中出现异常，尽量记录但不抛出
        if (g_coreInitialized) {
            DearTs::Utils::getLogger().error("Error during core shutdown: " + std::string(e.what()));
        }
    }
}

const char* GetVersion() {
    return DearTs::Version::STRING;
}

const char* GetBuildInfo() {
    if (g_buildInfo.empty()) {
        g_buildInfo = GenerateBuildInfo();
    }
    return g_buildInfo.c_str();
}

bool IsInitialized() {
    return g_coreInitialized;
}

} // namespace DearTs