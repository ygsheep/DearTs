/**
 * DearTs Application Manager Implementation
 * 
 * 应用程序管理器实现文件 - 提供应用程序生命周期管理功能实现
 * 
 * @author DearTs Team
 * @version 1.0.0
 * @date 2025
 */

#include "application_manager.h"
#include "utils/config_manager.h"
#include "../core.h"
#include "../utils/string_utils.h"
#include "../utils/file_utils.h"
#include <SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <thread>
#include <csignal>

#ifdef DEARTS_PLATFORM_WINDOWS
    #include <windows.h>
    #include <dbghelp.h>
    #include <psapi.h>
    #pragma comment(lib, "dbghelp.lib")
    #pragma comment(lib, "psapi.lib")
#else
    #include <execinfo.h>
    #include <unistd.h>
    #include <sys/resource.h>
    #include <dlfcn.h>
#endif

namespace DearTs {
namespace Core {
namespace App {

// ============================================================================
// 全局变量
// ============================================================================

static std::function<void(const std::string&)> g_crash_callback;
static bool g_crash_handler_installed = false;

// ============================================================================
// 崩溃处理函数
// ============================================================================

#ifdef DEARTS_PLATFORM_WINDOWS
LONG WINAPI UnhandledExceptionFilter(EXCEPTION_POINTERS* exception_info) {
    std::ostringstream oss;
    oss << "Unhandled exception occurred:\n";
    oss << "Exception Code: 0x" << std::hex << exception_info->ExceptionRecord->ExceptionCode << "\n";
    oss << "Exception Address: 0x" << std::hex << exception_info->ExceptionRecord->ExceptionAddress << "\n";
    
    // 生成调用栈
    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();
    
    CONTEXT* context = exception_info->ContextRecord;
    STACKFRAME64 stack_frame;
    memset(&stack_frame, 0, sizeof(STACKFRAME64));
    
#ifdef _M_IX86
    DWORD machine_type = IMAGE_FILE_MACHINE_I386;
    stack_frame.AddrPC.Offset = context->Eip;
    stack_frame.AddrPC.Mode = AddrModeFlat;
    stack_frame.AddrFrame.Offset = context->Ebp;
    stack_frame.AddrFrame.Mode = AddrModeFlat;
    stack_frame.AddrStack.Offset = context->Esp;
    stack_frame.AddrStack.Mode = AddrModeFlat;
#elif _M_X64
    DWORD machine_type = IMAGE_FILE_MACHINE_AMD64;
    stack_frame.AddrPC.Offset = context->Rip;
    stack_frame.AddrPC.Mode = AddrModeFlat;
    stack_frame.AddrFrame.Offset = context->Rsp;
    stack_frame.AddrFrame.Mode = AddrModeFlat;
    stack_frame.AddrStack.Offset = context->Rsp;
    stack_frame.AddrStack.Mode = AddrModeFlat;
#endif
    
    SymInitialize(process, NULL, TRUE);
    
    oss << "\nCall Stack:\n";
    int frame_count = 0;
    while (StackWalk64(machine_type, process, thread, &stack_frame, context, NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL)) {
        if (frame_count++ > 20) break; // 限制调用栈深度
        
        DWORD64 address = stack_frame.AddrPC.Offset;
        oss << "  [" << frame_count << "] 0x" << std::hex << address;
        
        // 获取符号信息
        char symbol_buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
        SYMBOL_INFO* symbol = (SYMBOL_INFO*)symbol_buffer;
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = MAX_SYM_NAME;
        
        DWORD64 displacement = 0;
        if (SymFromAddr(process, address, &displacement, symbol)) {
            oss << " " << symbol->Name;
        }
        
        oss << "\n";
    }
    
    SymCleanup(process);
    
    if (g_crash_callback) {
        g_crash_callback(oss.str());
    }
    
    return EXCEPTION_EXECUTE_HANDLER;
}
#else
void SignalHandler(int signal) {
    std::ostringstream oss;
    oss << "Signal received: " << signal << " (" << strsignal(signal) << ")\n";
    
    // 生成调用栈
    void* array[20];
    size_t size = backtrace(array, 20);
    char** strings = backtrace_symbols(array, size);
    
    oss << "\nCall Stack:\n";
    for (size_t i = 0; i < size; ++i) {
        oss << "  [" << i << "] " << strings[i] << "\n";
    }
    
    free(strings);
    
    if (g_crash_callback) {
        g_crash_callback(oss.str());
    }
    
    // 恢复默认信号处理并重新发送信号
    signal(signal, SIG_DFL);
    raise(signal);
}
#endif

// ============================================================================
// Application 实现
// ============================================================================

Application::Application()
    : state_(ApplicationState::UNINITIALIZED)
    , should_exit_(false)
    , exit_code_(0)
    , fps_frame_count_(0) {
    
    stats_.start_time = std::chrono::steady_clock::now();
    last_frame_time_ = stats_.start_time;
    fps_timer_ = stats_.start_time;
    
    DEARTS_LOG_DEBUG("Application instance created");
}

Application::~Application() {
    if (state_ != ApplicationState::STOPPED && state_ != ApplicationState::UNINITIALIZED) {
        shutdown();
    }
    
    DEARTS_LOG_DEBUG("Application instance destroyed");
}

bool Application::initialize(const ApplicationConfig& config) {
    if (state_ != ApplicationState::UNINITIALIZED) {
        DEARTS_LOG_ERROR("应用程序已初始化");
        return false;
    }
    
    state_ = ApplicationState::INITIALIZING;
    config_ = config;
    
    DEARTS_LOG_INFO("正在初始化应用程序: " + config_.name);
    
    try {
        // 初始化子系统
        initializeSubsystems();
        
        // 调用子类初始化
        if (!onInitialize()) {
            DEARTS_LOG_ERROR("应用程序特定初始化失败");
            state_ = ApplicationState::UNINITIALIZED;
            return false;
        }
        
        state_ = ApplicationState::RUNNING;
        
        DEARTS_LOG_INFO("应用程序初始化成功");
        return true;
        
    } catch (const std::exception& e) {
        DEARTS_LOG_ERROR("应用程序初始化期间发生异常: " + std::string(e.what()));
        state_ = ApplicationState::UNINITIALIZED;
        return false;
    }
}

// Application class implementations

void DearTs::Core::App::Application::shutdown() {
    if (state_ == ApplicationState::STOPPED || state_ == ApplicationState::UNINITIALIZED) {
        return;
    }
    
    DEARTS_LOG_INFO("Shutting down application");
    
    state_ = ApplicationState::STOPPING;
    
    // 调用子类关闭
    onShutdown();
    
    // 关闭子系统
    shutdownSubsystems();
    
    state_ = ApplicationState::STOPPED;
    
    DEARTS_LOG_INFO("Application shutdown completed");
}

void DearTs::Core::App::Application::update(double delta_time) {
    if (profiler_) {
        // TODO: Replace with modern C++ profiling
    }
    
    // 更新插件
    auto& plugin_manager = PluginManager::getInstance();
    plugin_manager.updateAllPlugins(delta_time);
    
    // 更新窗口
    auto& window_manager = Window::WindowManager::getInstance();
    window_manager.updateAllWindows();
}

void DearTs::Core::App::Application::render() {
    if (profiler_) {
        // TODO: Replace with modern C++ profiling
    }
    
    // 渲染窗口
    auto& window_manager = Window::WindowManager::getInstance();
    window_manager.renderAllWindows();
}

void DearTs::Core::App::Application::handleEvent(const Events::Event& event) {
    // 处理应用程序级别的事件
    switch (event.getType()) {
        case Events::EventType::EVT_APPLICATION_QUIT:
            requestExit();
            break;
            
        case Events::EventType::EVT_APPLICATION_PAUSE:
            pause();
            break;
            
        case Events::EventType::EVT_APPLICATION_RESUME:
            resume();
            break;
            
        default:
            break;
    }
    
    // 调用注册的事件处理器
    {
        std::lock_guard<std::mutex> lock(event_handlers_mutex_);
        auto it = event_handlers_.find(event.getType());
        if (it != event_handlers_.end()) {
            it->second(event);
        }
    }
    
    // 调用子类事件处理
    onEvent(event);
}



void DearTs::Core::App::Application::requestExit(int exit_code) {
    exit_code_ = exit_code;
    should_exit_ = true;
    
    DEARTS_LOG_INFO("Application exit requested with code: " + std::to_string(exit_code));
}

void DearTs::Core::App::Application::pause() {
    if (state_ == ApplicationState::RUNNING) {
        state_ = ApplicationState::PAUSED;
        onPause();
        
        DEARTS_LOG_INFO("Application paused");
    }
}

void DearTs::Core::App::Application::resume() {
    if (state_ == ApplicationState::PAUSED) {
        state_ = ApplicationState::RUNNING;
        onResume();
        
        DEARTS_LOG_INFO("Application resumed");
    }
}

void DearTs::Core::App::Application::setConfig(const ApplicationConfig& config) {
    config_ = config;
    
    // 应用新配置
    if (config_manager_) {
        // 这里可以更新配置管理器的设置
    }
    
    DEARTS_LOG_DEBUG("Application config updated");
}

void DearTs::Core::App::Application::loadConfig(const std::string& file_path) {
    if (!config_manager_) {
        DEARTS_LOG_ERROR("Config manager not initialized");
        return;
    }
    
    if (config_manager_->loadFromFile(file_path)) {
        // 从配置文件更新应用程序配置
        config_.name = config_manager_->getValue<std::string>("app.name", config_.name);
        config_.version = config_manager_->getValue<std::string>("app.version", config_.version);
        config_.target_fps = config_manager_->getValue<uint32_t>("app.target_fps", config_.target_fps);
        config_.enable_vsync = config_manager_->getValue<bool>("app.enable_vsync", config_.enable_vsync);
        
        DEARTS_LOG_INFO("Config loaded from: " + file_path);
    } else {
        DEARTS_LOG_ERROR("Failed to load config from: " + file_path);
    }
}

void DearTs::Core::App::Application::saveConfig(const std::string& file_path) {
    if (!config_manager_) {
        DEARTS_LOG_ERROR("Config manager not initialized");
        return;
    }
    
    // 保存当前配置到配置管理器
    config_manager_->setValue("app.name", config_.name);
    config_manager_->setValue("app.version", config_.version);
    config_manager_->setValue("app.target_fps", config_.target_fps);
    config_manager_->setValue("app.enable_vsync", config_.enable_vsync);
    
    config_manager_->saveToFile(file_path);
    std::ostringstream oss;
    oss << "Config saved to: " << file_path;
    DEARTS_LOG_INFO(oss.str());
}

void DearTs::Core::App::Application::addEventListener(DearTs::Core::Events::EventType type, std::function<void(const DearTs::Core::Events::Event&)> handler) {
    std::lock_guard<std::mutex> lock(event_handlers_mutex_);
    event_handlers_[type] = handler;
}

void DearTs::Core::App::Application::removeEventListener(DearTs::Core::Events::EventType type) {
    std::lock_guard<std::mutex> lock(event_handlers_mutex_);
    event_handlers_.erase(type);
}

void DearTs::Core::App::Application::initializeSubsystems() {
    // 初始化SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0) {
        throw std::runtime_error("Failed to initialize SDL: " + std::string(SDL_GetError()));
    }
    
    // 初始化事件系统
    auto event_system = DearTs::Core::Events::EventSystem::getInstance();
    event_system->initialize();
    
    // 初始化窗口管理器
    auto& window_manager = DearTs::Core::Window::WindowManager::getInstance();
    if (!window_manager.initialize()) {
        throw std::runtime_error("Failed to initialize window manager");
    }
    
    // 初始化配置管理器
    config_manager_ = &Utils::ConfigManager::getInstance();
    
    // 初始化性能分析器
    if (config_.enable_profiling) {
        profiler_ = &Utils::Profiler::getInstance();
        profiler_->initialize();
    } else {
        profiler_ = nullptr;
    }
    
    // 初始化插件管理器
    auto& plugin_manager = PluginManager::getInstance();
    for (const auto& path : config_.plugin_paths) {
        plugin_manager.addPluginPath(path);
    }
    plugin_manager.setAutoLoadPlugins(config_.auto_load_plugins);
    plugin_manager.scanAndLoadPlugins();
    plugin_manager.initializeAllPlugins(this);
    
    DEARTS_LOG_DEBUG("Application subsystems initialized");
}

void DearTs::Core::App::Application::shutdownSubsystems() {
    // 关闭插件管理器
    auto& plugin_manager = PluginManager::getInstance();
    plugin_manager.shutdownAllPlugins();
    
    // 关闭性能分析器
    if (profiler_) {
        profiler_->shutdown();
        profiler_ = nullptr;
    }
    
    // 关闭配置管理器
    config_manager_ = nullptr;
    
    // 关闭窗口管理器
    auto& window_manager = DearTs::Core::Window::WindowManager::getInstance();
    window_manager.shutdown();
    
    // 关闭事件系统
    auto event_system = DearTs::Core::Events::EventSystem::getInstance();
    event_system->shutdown();
    
    // 关闭SDL
    SDL_Quit();
    
    DEARTS_LOG_DEBUG("Application subsystems shutdown");
}

void DearTs::Core::App::Application::updateStats() {
    auto current_time = std::chrono::steady_clock::now();
    
    // 更新运行时间
    stats_.uptime = current_time - stats_.start_time;
    
    // 更新帧计数
    stats_.frame_count++;
    fps_frame_count_++;
    
    // 计算帧时间
    auto frame_duration = current_time - last_frame_time_;
    stats_.frame_time = std::chrono::duration<double, std::milli>(frame_duration).count();
    
    // 每秒更新一次FPS
    auto fps_duration = current_time - fps_timer_;
    if (fps_duration >= std::chrono::seconds(1)) {
        stats_.current_fps = fps_frame_count_ / std::chrono::duration<double>(fps_duration).count();
        stats_.average_fps = stats_.frame_count / std::chrono::duration<double>(stats_.uptime).count();
        
        fps_frame_count_ = 0;
        fps_timer_ = current_time;
    }
    
    // 更新内存使用量
#ifdef DEARTS_PLATFORM_WINDOWS
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        stats_.memory_usage = pmc.WorkingSetSize;
        stats_.peak_memory_usage = std::max(stats_.peak_memory_usage, stats_.memory_usage);
    }
#else
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        stats_.memory_usage = usage.ru_maxrss * 1024; // Linux返回KB，转换为字节
        stats_.peak_memory_usage = std::max(stats_.peak_memory_usage, stats_.memory_usage);
    }
#endif
}

void DearTs::Core::App::Application::processEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        // 将事件传递给ImGui SDL2绑定
        ImGui_ImplSDL2_ProcessEvent(&event);

        // 转发所有事件给窗口管理器（用于处理标题栏事件）
        auto& window_manager = Window::WindowManager::getInstance();
        window_manager.handleSDLEvent(event);

        // 处理SDL事件
        switch (event.type) {
            case SDL_QUIT:
                DEARTS_LOG_INFO("SDL_QUIT event received, requesting exit and closing all windows");
                requestExit();
                // 手动关闭所有窗口，确保窗口关闭流程被触发
                {
                    auto& window_manager = Window::WindowManager::getInstance();
                    auto windows = window_manager.getAllWindows();
                    for (auto& window : windows) {
                        if (window) {
                            DEARTS_LOG_INFO("SDL_QUIT: Closing window ID: " + std::to_string(window->getId()));
                            window->close();
                        }
                    }
                }
                break;
                
            default:
                break;
        }
    }
    
    // 检查是否有窗口请求关闭
    auto& window_manager = Window::WindowManager::getInstance();
    if (window_manager.hasWindowsToClose()) {
        DEARTS_LOG_INFO("Found windows to close, closing them");
        window_manager.closeWindowsToClose();
        if (window_manager.getWindowCount() == 0) {
            DEARTS_LOG_INFO("No windows left, requesting exit");
            requestExit();
        }
    }
}

void DearTs::Core::App::Application::limitFrameRate() {
    if (config_.target_fps > 0) {
        auto target_frame_time = std::chrono::duration<double>(1.0 / config_.target_fps);
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed = current_time - last_frame_time_;
        
        if (elapsed < target_frame_time) {
            auto sleep_time = target_frame_time - elapsed;
            std::this_thread::sleep_for(sleep_time);
        }
    }
}

// ============================================================================
// PluginManager 实现
// ============================================================================

DearTs::Core::App::PluginManager& DearTs::Core::App::PluginManager::getInstance() {
    static DearTs::Core::App::PluginManager instance;
    return instance;
}

bool DearTs::Core::App::PluginManager::loadPlugin(const std::string& file_path) {
    std::lock_guard<std::mutex> lock(plugins_mutex_);
    
    if (!Utils::FileUtils::exists(file_path)) {
        DEARTS_LOG_ERROR("Plugin file not found: " + file_path);
        return false;
    }
    
    DearTs::Core::App::PluginManager::PluginEntry entry;
    if (!loadPluginFromFile(file_path, entry)) {
        return false;
    }
    
    // 检查是否已经加载了同名插件
    const std::string& name = entry.info.name;
    if (plugins_.find(name) != plugins_.end()) {
        DEARTS_LOG_WARN("Plugin already loaded: " + name);
        unloadPluginEntry(entry);
        return false;
    }
    
    plugins_[name] = std::move(entry);
    
    DEARTS_LOG_INFO("Plugin loaded: " + name + " (" + file_path + ")");
    return true;
}

bool DearTs::Core::App::PluginManager::unloadPlugin(const std::string& name) {
    std::lock_guard<std::mutex> lock(plugins_mutex_);
    
    auto it = plugins_.find(name);
    if (it == plugins_.end()) {
        DEARTS_LOG_WARN("Plugin not found: " + name);
        return false;
    }
    
    unloadPluginEntry(it->second);
    plugins_.erase(it);
    
    DEARTS_LOG_INFO("Plugin unloaded: " + name);
    return true;
}

bool DearTs::Core::App::PluginManager::reloadPlugin(const std::string& name) {
    std::lock_guard<std::mutex> lock(plugins_mutex_);
    
    auto it = plugins_.find(name);
    if (it == plugins_.end()) {
        DEARTS_LOG_WARN("Plugin not found for reload: " + name);
        return false;
    }
    
    std::string file_path = it->second.info.file_path;
    unloadPluginEntry(it->second);
    plugins_.erase(it);
    
    // 重新加载
    lock.~lock_guard();
    return loadPlugin(file_path);
}

std::shared_ptr<DearTs::Core::App::IPlugin> DearTs::Core::App::PluginManager::getPlugin(const std::string& name) const {
    std::lock_guard<std::mutex> lock(plugins_mutex_);
    
    auto it = plugins_.find(name);
    return (it != plugins_.end()) ? it->second.plugin : nullptr;
}

std::vector<std::shared_ptr<DearTs::Core::App::IPlugin>> DearTs::Core::App::PluginManager::getPluginsByType(DearTs::Core::App::PluginType type) const {
    std::lock_guard<std::mutex> lock(plugins_mutex_);
    
    std::vector<std::shared_ptr<DearTs::Core::App::IPlugin>> result;
    for (const auto& [name, entry] : plugins_) {
        if (entry.info.type == type && entry.plugin) {
            result.push_back(entry.plugin);
        }
    }
    
    return result;
}

std::vector<std::string> DearTs::Core::App::PluginManager::getLoadedPluginNames() const {
    std::lock_guard<std::mutex> lock(plugins_mutex_);
    
    std::vector<std::string> names;
    names.reserve(plugins_.size());
    
    for (const auto& [name, entry] : plugins_) {
        names.push_back(name);
    }
    
    return names;
}

bool DearTs::Core::App::PluginManager::isPluginLoaded(const std::string& name) const {
    std::lock_guard<std::mutex> lock(plugins_mutex_);
    return plugins_.find(name) != plugins_.end();
}

DearTs::Core::App::PluginInfo DearTs::Core::App::PluginManager::getPluginInfo(const std::string& name) const {
    std::lock_guard<std::mutex> lock(plugins_mutex_);
    
    auto it = plugins_.find(name);
    return (it != plugins_.end()) ? it->second.info : DearTs::Core::App::PluginInfo{};
}

std::vector<DearTs::Core::App::PluginInfo> DearTs::Core::App::PluginManager::getAllPluginInfos() const {
    std::lock_guard<std::mutex> lock(plugins_mutex_);
    
    std::vector<DearTs::Core::App::PluginInfo> infos;
    infos.reserve(plugins_.size());
    
    for (const auto& [name, entry] : plugins_) {
        infos.push_back(entry.info);
    }
    
    return infos;
}

void DearTs::Core::App::PluginManager::addPluginPath(const std::string& path) {
    auto it = std::find(plugin_paths_.begin(), plugin_paths_.end(), path);
    if (it == plugin_paths_.end()) {
        plugin_paths_.push_back(path);
        DEARTS_LOG_DEBUG("Plugin path added: " + path);
    }
}

void DearTs::Core::App::PluginManager::removePluginPath(const std::string& path) {
    auto it = std::find(plugin_paths_.begin(), plugin_paths_.end(), path);
    if (it != plugin_paths_.end()) {
        plugin_paths_.erase(it);
        DEARTS_LOG_DEBUG("Plugin path removed: " + path);
    }
}

std::vector<std::string> DearTs::Core::App::PluginManager::getPluginPaths() const {
    return plugin_paths_;
}

void DearTs::Core::App::PluginManager::scanAndLoadPlugins() {
    for (const auto& path : plugin_paths_) {
        if (!Utils::FileUtils::exists(path)) {
            continue;
        }
        
        if (Utils::FileUtils::isDirectory(path)) {
            // 扫描目录中的插件文件
            auto files = Utils::FileUtils::listDirectory(path, true);
            for (const auto& file_info : files) {
#ifdef DEARTS_PLATFORM_WINDOWS
                if (Utils::StringUtils::endsWith(file_info.path, ".dll")) {
#else
                if (Utils::StringUtils::endsWith(file_info.path, ".so")) {
#endif
                    loadPlugin(file_info.path);
                }
            }
        } else {
            // 直接加载插件文件
            loadPlugin(path);
        }
    }
}

void DearTs::Core::App::PluginManager::setAutoLoadPlugins(const std::vector<std::string>& plugins) {
    auto_load_plugins_ = plugins;
}

bool DearTs::Core::App::PluginManager::checkDependencies(const std::string& plugin_name) const {
    std::lock_guard<std::mutex> lock(plugins_mutex_);
    
    auto it = plugins_.find(plugin_name);
    if (it == plugins_.end()) {
        return false;
    }
    
    for (const auto& dep : it->second.info.dependencies) {
        if (plugins_.find(dep) == plugins_.end()) {
            DEARTS_LOG_ERROR("Plugin dependency not found: " + plugin_name + " requires " + dep);
            return false;
        }
    }
    
    return true;
}

std::vector<std::string> DearTs::Core::App::PluginManager::resolveDependencies(const std::string& plugin_name) const {
    std::lock_guard<std::mutex> lock(plugins_mutex_);
    
    std::vector<std::string> resolved;
    std::vector<std::string> to_resolve = {plugin_name};
    
    while (!to_resolve.empty()) {
        std::string current = to_resolve.back();
        to_resolve.pop_back();
        
        if (std::find(resolved.begin(), resolved.end(), current) != resolved.end()) {
            continue; // 已经解析过
        }
        
        auto it = plugins_.find(current);
        if (it == plugins_.end()) {
            continue; // 插件不存在
        }
        
        // 添加依赖项到待解析列表
        for (const auto& dep : it->second.info.dependencies) {
            to_resolve.push_back(dep);
        }
        
        resolved.push_back(current);
    }
    
    return resolved;
}

void DearTs::Core::App::PluginManager::initializeAllPlugins(DearTs::Core::App::IApplication* app) {
    std::lock_guard<std::mutex> lock(plugins_mutex_);
    
    for (auto& [name, entry] : plugins_) {
        if (entry.plugin && entry.state == PluginState::LOADED) {
            entry.state = PluginState::INITIALIZING;
            
            if (entry.plugin->initialize(app)) {
                entry.state = PluginState::ACTIVE;
                std::ostringstream oss;
                oss << "Plugin initialized: " << name;
                DEARTS_LOG_INFO(oss.str());
            } else {
                entry.state = PluginState::ERROR_STATE;
                std::ostringstream oss;
                oss << "Failed to initialize plugin: " << name;
                DEARTS_LOG_ERROR(oss.str());
            }
        }
    }
}

void DearTs::Core::App::PluginManager::shutdownAllPlugins() {
    std::lock_guard<std::mutex> lock(plugins_mutex_);
    
    for (auto& [name, entry] : plugins_) {
        if (entry.plugin && entry.state == DearTs::Core::App::PluginState::ACTIVE) {
            entry.plugin->shutdown();
            entry.state = DearTs::Core::App::PluginState::INACTIVE;
            DEARTS_LOG_INFO("Plugin shutdown: " + name);
        }
    }
}

void DearTs::Core::App::PluginManager::updateAllPlugins(double delta_time) {
    std::lock_guard<std::mutex> lock(plugins_mutex_);
    
    for (auto& [name, entry] : plugins_) {
        if (entry.plugin && entry.state == DearTs::Core::App::PluginState::ACTIVE) {
            entry.plugin->update(delta_time);
        }
    }
}

bool DearTs::Core::App::PluginManager::loadPluginFromFile(const std::string& file_path, DearTs::Core::App::PluginManager::PluginEntry& entry) {
    entry.info.file_path = file_path;
    entry.state = DearTs::Core::App::PluginState::LOADING;
    
#ifdef DEARTS_PLATFORM_WINDOWS
    HMODULE handle = LoadLibraryA(file_path.c_str());
    if (!handle) {
        DEARTS_LOG_ERROR("Failed to load plugin library: " + file_path + " (Error: " + std::to_string(GetLastError()) + ")");
        return false;
    }
    
    // 获取插件信息函数
    typedef DearTs::Core::App::PluginInfo (*GetPluginInfoFunc)();
    GetPluginInfoFunc get_info = (GetPluginInfoFunc)GetProcAddress(handle, DEARTS_PLUGIN_INFO_FUNC);
    
    // 获取插件创建函数
    typedef DearTs::Core::App::IPlugin* (*CreatePluginFunc)();
    CreatePluginFunc create_plugin = (CreatePluginFunc)GetProcAddress(handle, DEARTS_PLUGIN_CREATE_FUNC);
    
    if (!get_info || !create_plugin) {
        DEARTS_LOG_ERROR("Plugin missing required functions: " + file_path);
        FreeLibrary(handle);
        return false;
    }
    
    entry.library_handle = handle;
#else
    void* handle = dlopen(file_path.c_str(), RTLD_LAZY);
    if (!handle) {
        DEARTS_LOG_ERROR("Failed to load plugin library: {} ({})", file_path, dlerror());
        return false;
    }
    
    // 获取插件信息函数
    typedef DearTs::Core::App::PluginInfo (*GetPluginInfoFunc)();
    GetPluginInfoFunc get_info = (GetPluginInfoFunc)dlsym(handle, DEARTS_PLUGIN_INFO_FUNC);
    
    // 获取插件创建函数
    typedef DearTs::Core::App::IPlugin* (*CreatePluginFunc)();
    CreatePluginFunc create_plugin = (CreatePluginFunc)dlsym(handle, DEARTS_PLUGIN_CREATE_FUNC);
    
    if (!get_info || !create_plugin) {
        DEARTS_LOG_ERROR("Plugin missing required functions: {}", file_path);
        dlclose(handle);
        return false;
    }
    
    entry.library_handle = handle;
#endif
    
    try {
        // 获取插件信息
        entry.info = get_info();
        
        // 创建插件实例
        entry.plugin = std::shared_ptr<DearTs::Core::App::IPlugin>(create_plugin());
        
        entry.state = DearTs::Core::App::PluginState::LOADED;
        entry.load_time = std::chrono::steady_clock::now();
        
        return true;
        
    } catch (const std::exception& e) {
        DEARTS_LOG_ERROR("Exception while loading plugin " + file_path + ": " + e.what());
        unloadPluginEntry(entry);
        return false;
    }
}

void DearTs::Core::App::PluginManager::unloadPluginEntry(DearTs::Core::App::PluginManager::PluginEntry& entry) {
    if (entry.plugin) {
        if (entry.state == DearTs::Core::App::PluginState::ACTIVE) {
            entry.plugin->shutdown();
        }
        entry.plugin.reset();
    }
    
    if (entry.library_handle) {
#ifdef DEARTS_PLATFORM_WINDOWS
        FreeLibrary((HMODULE)entry.library_handle);
#else
        dlclose(entry.library_handle);
#endif
        entry.library_handle = nullptr;
    }
    
    entry.state = DearTs::Core::App::PluginState::UNLOADED;
}

// ============================================================================
// ApplicationManager 实现
// ============================================================================

DearTs::Core::App::ApplicationManager& DearTs::Core::App::ApplicationManager::getInstance() {
    static DearTs::Core::App::ApplicationManager instance;
    return instance;
}

bool DearTs::Core::App::ApplicationManager::initialize() {
    if (initialized_) {
        return true;
    }
    
    DEARTS_LOG_INFO("Initializing Application Manager");
    
    // 设置默认全局配置
    global_config_ = DearTs::Core::App::ApplicationConfig();
    
    // 设置崩溃处理器
    if (global_config_.enable_crash_handler) {
        enableCrashHandler(true);
    }
    
    initialized_ = true;
    
    DEARTS_LOG_INFO("Application Manager initialized successfully");
    return true;
}

void DearTs::Core::App::ApplicationManager::shutdown() {
    if (!initialized_) {
        return;
    }
    
    DEARTS_LOG_INFO("Shutting down Application Manager");
    
    // 清理崩溃处理器
    if (crash_handler_enabled_) {
        enableCrashHandler(false);
    }
    
    initialized_ = false;
    
    DEARTS_LOG_INFO("Application Manager shutdown completed");
}

int DearTs::Core::App::ApplicationManager::runApplication(std::unique_ptr<DearTs::Core::App::IApplication> app) {
    if (!app) {
        DEARTS_LOG_ERROR("Invalid application instance");
        return -1;
    }
    
    if (!initialized_) {
        DEARTS_LOG_ERROR("Application Manager not initialized");
        return -1;
    }
    
    // 初始化应用程序
    if (!app->initialize(global_config_)) {
        DEARTS_LOG_ERROR("Failed to initialize application");
        return -1;
    }
    
    DEARTS_LOG_INFO("Running application: " + app->getConfig().name);
    
    int result = app->run();
    
    DEARTS_LOG_INFO("Application finished with exit code: " + std::to_string(result));
    return result;
}

void DearTs::Core::App::ApplicationManager::setGlobalConfig(const DearTs::Core::App::ApplicationConfig& config) {
    global_config_ = config;
    DEARTS_LOG_DEBUG("Global application config updated");
}

void DearTs::Core::App::ApplicationManager::enableCrashHandler(bool enable) {
    if (enable && !crash_handler_enabled_) {
        setupCrashHandler();
        crash_handler_enabled_ = true;
        DEARTS_LOG_INFO("Crash handler enabled");
    } else if (!enable && crash_handler_enabled_) {
        cleanupCrashHandler();
        crash_handler_enabled_ = false;
        DEARTS_LOG_INFO("Crash handler disabled");
    }
}

void DearTs::Core::App::ApplicationManager::setCrashCallback(std::function<void(const std::string&)> callback) {
    crash_callback_ = callback;
    g_crash_callback = callback;
}

void DearTs::Core::App::ApplicationManager::enableHotReload(bool enable) {
    hot_reload_enabled_ = enable;
    DEARTS_LOG_INFO("Hot reload " + std::string(enable ? "enabled" : "disabled"));
}

void DearTs::Core::App::ApplicationManager::checkForReload() {
    if (!hot_reload_enabled_) {
        return;
    }
    
    // 这里可以实现热重载检查逻辑
    // 例如检查文件修改时间，重新加载插件等
}

std::string DearTs::Core::App::ApplicationManager::getSystemInfo() const {
    std::ostringstream oss;
    
    oss << "System Information:\n";
    oss << "  Platform: ";
#ifdef DEARTS_PLATFORM_WINDOWS
    oss << "Windows";
#elif defined(DEARTS_PLATFORM_LINUX)
    oss << "Linux";
#elif defined(DEARTS_PLATFORM_MACOS)
    oss << "macOS";
#else
    oss << "Unknown";
#endif
    oss << "\n";
    
    oss << "  Architecture: ";
#ifdef DEARTS_ARCH_X64
    oss << "x64";
#elif defined(DEARTS_ARCH_X86)
    oss << "x86";
#elif defined(DEARTS_ARCH_ARM64)
    oss << "ARM64";
#else
    oss << "Unknown";
#endif
    oss << "\n";
    
    oss << "  Compiler: ";
#ifdef DEARTS_COMPILER_MSVC
    oss << "MSVC " << DEARTS_COMPILER_VERSION;
#elif defined(DEARTS_COMPILER_GCC)
    oss << "GCC " << DEARTS_COMPILER_VERSION;
#elif defined(DEARTS_COMPILER_CLANG)
    oss << "Clang " << DEARTS_COMPILER_VERSION;
#else
    oss << "Unknown";
#endif
    oss << "\n";
    
    oss << "  Build Type: ";
#ifdef DEARTS_DEBUG
    oss << "Debug";
#else
    oss << "Release";
#endif
    oss << "\n";
    
    return oss.str();
}

std::string DearTs::Core::App::ApplicationManager::getVersionInfo() const {
    std::ostringstream oss;
    
    oss << "DearTs Framework Version Information:\n";
    oss << "  Version: " << DearTs::Version::STRING << "\n";
    oss << "  Build Date: " << __DATE__ << " " << __TIME__ << "\n";
    oss << "  Git Commit: " << DearTs::Version::GIT_COMMIT_HASH << "\n";
    
    return oss.str();
}

std::string DearTs::Core::App::ApplicationManager::getExecutablePath() {
#ifdef DEARTS_PLATFORM_WINDOWS
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    return std::string(path);
#else
    char path[1024];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len != -1) {
        path[len] = '\0';
        return std::string(path);
    }
    return "";
#endif
}

std::string DearTs::Core::App::ApplicationManager::getExecutableDirectory() {
    std::string exe_path = getExecutablePath();
    if (exe_path.empty()) {
        return "";
    }
    
    std::filesystem::path path(exe_path);
    return path.parent_path().string();
}

std::string DearTs::Core::App::ApplicationManager::getWorkingDirectory() {
    return std::filesystem::current_path().string();
}

bool DearTs::Core::App::ApplicationManager::setWorkingDirectory(const std::string& path) {
    std::error_code ec;
    std::filesystem::current_path(path, ec);
    return !ec;
}

void DearTs::Core::App::ApplicationManager::setupCrashHandler() {
    if (g_crash_handler_installed) {
        return;
    }
    
#ifdef DEARTS_PLATFORM_WINDOWS
    SetUnhandledExceptionFilter(UnhandledExceptionFilter);
#else
    signal(SIGSEGV, SignalHandler);
    signal(SIGABRT, SignalHandler);
    signal(SIGFPE, SignalHandler);
    signal(SIGILL, SignalHandler);
#endif
    
    g_crash_handler_installed = true;
}

void DearTs::Core::App::ApplicationManager::cleanupCrashHandler() {
    if (!g_crash_handler_installed) {
        return;
    }
    
#ifdef DEARTS_PLATFORM_WINDOWS
    SetUnhandledExceptionFilter(NULL);
#else
    signal(SIGSEGV, SIG_DFL);
    signal(SIGABRT, SIG_DFL);
    signal(SIGFPE, SIG_DFL);
    signal(SIGILL, SIG_DFL);
#endif
    
    g_crash_handler_installed = false;
}

void DearTs::Core::App::ApplicationManager::setupSignalHandlers() {
    // 这里可以设置其他信号处理器
}

void DearTs::Core::App::ApplicationManager::cleanupSignalHandlers() {
    // 这里可以清理其他信号处理器
}

// Application class implementations

int DearTs::Core::App::Application::run() {
    if (state_ != DearTs::Core::App::ApplicationState::INITIALIZING && 
        state_ != DearTs::Core::App::ApplicationState::STOPPED && 
        state_ != DearTs::Core::App::ApplicationState::RUNNING) {
        DEARTS_LOG_ERROR("Application not in valid state to run");
        return -1;
    }
    
    // 如果状态是INITIALIZING或STOPPED，设置为RUNNING
    if (state_ == DearTs::Core::App::ApplicationState::INITIALIZING || 
        state_ == DearTs::Core::App::ApplicationState::STOPPED) {
        state_ = DearTs::Core::App::ApplicationState::RUNNING;
    }
    
    DEARTS_LOG_INFO("Starting application main loop");
    
    // 获取核心系统管理器实例
    auto& window_manager = DearTs::Core::Window::WindowManager::getInstance();
    
    // 主循环
    int frame_count = 0;
    while (!should_exit_ && state_ == DearTs::Core::App::ApplicationState::RUNNING) {
        frame_count++;
        if (frame_count % 100 == 0) {
            DEARTS_LOG_DEBUG("Application main loop running, frame count: " + std::to_string(frame_count));
            DEARTS_LOG_DEBUG("should_exit_: " + std::to_string(should_exit_.load()) + ", window count: " + std::to_string(window_manager.getWindowCount()));
        }
        
        auto current_time = std::chrono::steady_clock::now();
        auto delta_time = std::chrono::duration<double>(current_time - last_frame_time_).count();
        last_frame_time_ = current_time;
        
        // 处理事件
        DEARTS_LOG_DEBUG("Processing events");
        processEvents();
        DEARTS_LOG_DEBUG("Events processed");
        
        // 检查窗口是否需要关闭
        DEARTS_LOG_DEBUG("Checking windows to close");
        if (window_manager.hasWindowsToClose()) {
            window_manager.closeWindowsToClose();
            if (window_manager.getWindowCount() == 0) {
                DEARTS_LOG_INFO("No windows left, requesting exit");
                requestExit();
            }
        }
        DEARTS_LOG_DEBUG("Windows check completed");
        
        // 更新应用程序
        if (state_ == DearTs::Core::App::ApplicationState::RUNNING) {
            DEARTS_LOG_DEBUG("Updating application");
            update(delta_time);
            DEARTS_LOG_DEBUG("Application update completed");
            onUpdate(delta_time);
            DEARTS_LOG_DEBUG("Application onUpdate completed");
        }
        
        // 渲染应用程序
        if (state_ == DearTs::Core::App::ApplicationState::RUNNING) {
            DEARTS_LOG_DEBUG("Rendering application");
            render();
            DEARTS_LOG_DEBUG("Application render completed");
            onRender();
            DEARTS_LOG_DEBUG("Application onRender completed");
        }
        
        // 更新统计信息
        DEARTS_LOG_DEBUG("Updating stats");
        updateStats();
        DEARTS_LOG_DEBUG("Stats updated");
        
        // 限制帧率
        DEARTS_LOG_DEBUG("Limiting frame rate");
        limitFrameRate();
        DEARTS_LOG_DEBUG("Frame rate limited");
        
        // 处理事件队列
        // event_system->processEvents(); // TODO: Implement proper event processing
    }
    
    DEARTS_LOG_INFO("Application main loop ended, exit code: " + std::to_string(exit_code_.load()));
    return exit_code_;
}

} // namespace App
} // namespace Core
} // namespace DearTs