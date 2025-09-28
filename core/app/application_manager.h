/**
 * DearTs Application Manager Header
 * 
 * 应用程序管理器头文件 - 提供应用程序生命周期管理功能
 * 
 * @author DearTs Team
 * @version 1.0.0
 * @date 2025
 */

#pragma once

#include "dearts/dearts_config.h"
#include "../events/event_system.h"
#include "../window/window_manager.h"
// Logger removed - using simple output instead
#include "../utils/config_manager.h"
#include "../utils/profiler.h"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <atomic>
#include <mutex>
#include <thread>

namespace DearTs {
namespace Core {
namespace App {

// ============================================================================
// 前向声明
// ============================================================================

class Application;
class ApplicationManager;
class Plugin;
class PluginManager;

// ============================================================================
// 枚举定义
// ============================================================================

/**
 * @brief 应用程序状态枚举
 */
enum class ApplicationState {
    UNINITIALIZED,  ///< 未初始化
    INITIALIZING,   ///< 初始化中
    RUNNING,        ///< 运行中
    PAUSED,         ///< 暂停
    STOPPING,       ///< 停止中
    STOPPED         ///< 已停止
};

/**
 * @brief 应用程序类型枚举
 */
enum class ApplicationType {
    CONSOLE,        ///< 控制台应用
    WINDOWED,       ///< 窗口应用
    FULLSCREEN,     ///< 全屏应用
    SERVICE         ///< 服务应用
};

/**
 * @brief 插件状态枚举
 */
enum class PluginState {
    UNLOADED,       ///< 未加载
    LOADING,        ///< 加载中
    LOADED,         ///< 已加载
    INITIALIZING,   ///< 初始化中
    ACTIVE,         ///< 活跃
    INACTIVE,       ///< 非活跃
    UNLOADING,      ///< 卸载中
    ERROR_STATE     ///< 错误状态
};

/**
 * @brief 插件类型枚举
 */
enum class PluginType {
    CORE,           ///< 核心插件
    RENDERER,       ///< 渲染器插件
    AUDIO,          ///< 音频插件
    INPUT,          ///< 输入插件
    NETWORK,        ///< 网络插件
    SCRIPTING,      ///< 脚本插件
    CUSTOM          ///< 自定义插件
};

// ============================================================================
// 结构体定义
// ============================================================================

/**
 * @brief 应用程序配置结构
 */
struct ApplicationConfig {
    std::string name = "DearTs Application";           ///< 应用程序名称
    std::string version = "1.0.0";                    ///< 版本号
    std::string description = "DearTs Application";   ///< 描述
    std::string author = "DearTs Team";               ///< 作者
    std::string organization = "DearTs";              ///< 组织
    
    ApplicationType type = ApplicationType::WINDOWED; ///< 应用程序类型
    
    // 窗口配置
    DearTs::Core::Window::WindowConfig window_config;               ///< 窗口配置
    
    // 性能配置
    uint32_t target_fps = 60;                         ///< 目标帧率
    bool enable_vsync = true;                          ///< 启用垂直同步
    bool enable_profiling = false;                    ///< 启用性能分析
    
    // 日志配置
    std::string log_level = "INFO";                   ///< 日志级别 (简化为字符串)
    std::string log_file = "application.log";         ///< 日志文件
    
    // 配置文件
    std::string config_file = "config.json";          ///< 配置文件路径
    
    // 插件配置
    std::vector<std::string> plugin_paths;            ///< 插件搜索路径
    std::vector<std::string> auto_load_plugins;       ///< 自动加载的插件
    
    // 其他配置
    bool enable_crash_handler = true;                 ///< 启用崩溃处理
    bool enable_hot_reload = false;                   ///< 启用热重载
    uint32_t max_frame_skip = 5;                      ///< 最大跳帧数
};

/**
 * @brief 应用程序统计信息
 */
struct ApplicationStats {
    std::chrono::steady_clock::time_point start_time; ///< 启动时间
    std::chrono::duration<double> uptime;             ///< 运行时间
    uint64_t frame_count = 0;                         ///< 帧计数
    double current_fps = 0.0;                         ///< 当前帧率
    double average_fps = 0.0;                         ///< 平均帧率
    double frame_time = 0.0;                          ///< 帧时间（毫秒）
    size_t memory_usage = 0;                          ///< 内存使用量（字节）
    size_t peak_memory_usage = 0;                     ///< 峰值内存使用量
};

/**
 * @brief 插件信息结构
 */
struct PluginInfo {
    std::string name;                                  ///< 插件名称
    std::string version;                               ///< 版本号
    std::string description;                           ///< 描述
    std::string author;                                ///< 作者
    std::string file_path;                             ///< 文件路径
    PluginType type = PluginType::CUSTOM;              ///< 插件类型
    std::vector<std::string> dependencies;            ///< 依赖项
    std::unordered_map<std::string, std::string> metadata; ///< 元数据
};

// ============================================================================
// 接口定义
// ============================================================================

/**
 * @brief 应用程序接口
 */
class DEARTS_API IApplication {
public:
    virtual ~IApplication() = default;
    
    /**
     * @brief 初始化应用程序
     * @param config 应用程序配置
     * @return 是否成功
     */
    virtual bool initialize(const ApplicationConfig& config) = 0;
    
    /**
     * @brief 运行应用程序
     * @return 退出代码
     */
    virtual int run() = 0;
    
    /**
     * @brief 关闭应用程序
     */
    virtual void shutdown() = 0;
    
    /**
     * @brief 更新应用程序
     * @param delta_time 时间增量（秒）
     */
    virtual void update(double delta_time) = 0;
    
    /**
     * @brief 渲染应用程序
     */
    virtual void render() = 0;
    
    /**
     * @brief 处理事件
     * @param event 事件
     */
    virtual void handleEvent(const DearTs::Core::Events::Event& event) = 0;
    
    /**
     * @brief 获取应用程序状态
     * @return 应用程序状态
     */
    virtual ApplicationState getState() const = 0;
    
    /**
     * @brief 获取应用程序配置
     * @return 应用程序配置
     */
    virtual const ApplicationConfig& getConfig() const = 0;
};

/**
 * @brief 插件接口
 */
class DEARTS_API IPlugin {
public:
    virtual ~IPlugin() = default;
    
    /**
     * @brief 获取插件信息
     * @return 插件信息
     */
    virtual PluginInfo getInfo() const = 0;
    
    /**
     * @brief 初始化插件
     * @param app 应用程序实例
     * @return 是否成功
     */
    virtual bool initialize(IApplication* app) = 0;
    
    /**
     * @brief 关闭插件
     */
    virtual void shutdown() = 0;
    
    /**
     * @brief 更新插件
     * @param delta_time 时间增量（秒）
     */
    virtual void update(double delta_time) = 0;
    
    /**
     * @brief 获取插件状态
     * @return 插件状态
     */
    virtual PluginState getState() const = 0;
};

// ============================================================================
// 应用程序类
// ============================================================================

/**
 * @brief 应用程序基类
 */

class DEARTS_API Application : public IApplication {
public:
    Application();
    virtual ~Application();
    
    // IApplication 接口实现
    bool initialize(const ApplicationConfig& config) override;
    int run() override;
    void shutdown() override;
    void update(double delta_time) override;
    void render() override;
    void handleEvent(const DearTs::Core::Events::Event& event) override;
    
    ApplicationState getState() const override { return state_; }
    const ApplicationConfig& getConfig() const override { return config_; }
    
    // 应用程序控制
    void requestExit(int exit_code = 0);
    void pause();
    void resume();
    
    // 统计信息
    const ApplicationStats& getStats() const { return stats_; }
    
    // 配置管理
    void setConfig(const ApplicationConfig& config);
    void loadConfig(const std::string& file_path);
    void saveConfig(const std::string& file_path);
    
    // 事件处理
    void addEventListener(DearTs::Core::Events::EventType type, std::function<void(const DearTs::Core::Events::Event&)> handler);
    void removeEventListener(DearTs::Core::Events::EventType type);
    
protected:
    // 虚函数，子类可重写
    virtual bool onInitialize() { return true; }
    virtual void onShutdown() {}
    virtual void onUpdate(double delta_time) {}
    virtual void onRender() {}
    virtual void onEvent(const DearTs::Core::Events::Event& event) {}
    virtual void onPause() {}
    virtual void onResume() {}
    
private:
    void initializeSubsystems();
    void shutdownSubsystems();
    void updateStats();
    void processEvents();
    void limitFrameRate();
    
    ApplicationConfig config_;                         ///< 应用程序配置
    ApplicationState state_;                           ///< 应用程序状态
    ApplicationStats stats_;                           ///< 统计信息
    
    std::atomic<bool> should_exit_;                   ///< 是否应该退出
    std::atomic<int> exit_code_;                      ///< 退出代码
    
    std::chrono::steady_clock::time_point last_frame_time_; ///< 上一帧时间
    std::chrono::steady_clock::time_point fps_timer_;       ///< FPS计时器
    uint32_t fps_frame_count_;                              ///< FPS帧计数
    
    // 子系统
    Utils::ConfigManager* config_manager_; ///< 配置管理器
    Utils::Profiler* profiler_;            ///< 性能分析器
    
    // 事件处理
    std::unordered_map<DearTs::Core::Events::EventType, std::function<void(const DearTs::Core::Events::Event&)>> event_handlers_;
    mutable std::mutex event_handlers_mutex_;
};


// ============================================================================
// 插件管理器类
// ============================================================================

/**
 * @brief 插件管理器类
 */
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251) // 需要有 dll 接口
#endif

class DEARTS_API PluginManager {
public:
    static PluginManager& getInstance();
    
    // 插件管理
    bool loadPlugin(const std::string& file_path);
    bool unloadPlugin(const std::string& name);
    bool reloadPlugin(const std::string& name);
    
    // 插件查询
    std::shared_ptr<IPlugin> getPlugin(const std::string& name) const;
    std::vector<std::shared_ptr<IPlugin>> getPluginsByType(PluginType type) const;
    std::vector<std::string> getLoadedPluginNames() const;
    bool isPluginLoaded(const std::string& name) const;
    
    // 插件信息
    PluginInfo getPluginInfo(const std::string& name) const;
    std::vector<PluginInfo> getAllPluginInfos() const;
    
    // 插件路径管理
    void addPluginPath(const std::string& path);
    void removePluginPath(const std::string& path);
    std::vector<std::string> getPluginPaths() const;
    
    // 自动加载
    void scanAndLoadPlugins();
    void setAutoLoadPlugins(const std::vector<std::string>& plugins);
    
    // 依赖管理
    bool checkDependencies(const std::string& plugin_name) const;
    std::vector<std::string> resolveDependencies(const std::string& plugin_name) const;
    
    // 生命周期管理
    void initializeAllPlugins(IApplication* app);
    void shutdownAllPlugins();
    void updateAllPlugins(double delta_time);
    
private:
    PluginManager() = default;
    ~PluginManager() = default;
    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;
    
    struct PluginEntry {
        std::shared_ptr<IPlugin> plugin;
        PluginInfo info;
        PluginState state = PluginState::UNLOADED;
        void* library_handle = nullptr;
        std::chrono::steady_clock::time_point load_time;
    };
    
    bool loadPluginFromFile(const std::string& file_path, PluginEntry& entry);
    void unloadPluginEntry(PluginEntry& entry);
    
    std::unordered_map<std::string, PluginEntry> plugins_; ///< 插件映射
    std::vector<std::string> plugin_paths_;                ///< 插件搜索路径
    std::vector<std::string> auto_load_plugins_;           ///< 自动加载插件
    mutable std::mutex plugins_mutex_;                     ///< 插件互斥锁
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

// ============================================================================
// 应用程序管理器类
// ============================================================================

/**
 * @brief 应用程序管理器类
 */
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251) // 需要有 dll 接口
#endif

class DEARTS_API ApplicationManager {
public:
    static ApplicationManager& getInstance();
    
    // 应用程序管理
    bool initialize();
    void shutdown();
    
    // 应用程序创建和运行
    template<typename T, typename... Args>
    std::unique_ptr<T> createApplication(Args&&... args) {
        static_assert(std::is_base_of_v<IApplication, T>, "T must inherit from IApplication");
        return std::make_unique<T>(std::forward<Args>(args)...);
    }
    
    int runApplication(std::unique_ptr<IApplication> app);
    
    // 全局配置
    void setGlobalConfig(const ApplicationConfig& config);
    const ApplicationConfig& getGlobalConfig() const { return global_config_; }
    
    // 崩溃处理
    void enableCrashHandler(bool enable);
    void setCrashCallback(std::function<void(const std::string&)> callback);
    
    // 热重载
    void enableHotReload(bool enable);
    void checkForReload();
    
    // 系统信息
    std::string getSystemInfo() const;
    std::string getVersionInfo() const;
    
    // 实用工具
    static std::string getExecutablePath();
    static std::string getExecutableDirectory();
    static std::string getWorkingDirectory();
    static bool setWorkingDirectory(const std::string& path);
    
private:
    ApplicationManager() = default;
    ~ApplicationManager() = default;
    ApplicationManager(const ApplicationManager&) = delete;
    ApplicationManager& operator=(const ApplicationManager&) = delete;
    
    void setupCrashHandler();
    void cleanupCrashHandler();
    void setupSignalHandlers();
    void cleanupSignalHandlers();
    
    bool initialized_ = false;                         ///< 是否已初始化
    ApplicationConfig global_config_;                  ///< 全局配置
    std::function<void(const std::string&)> crash_callback_; ///< 崩溃回调
    bool crash_handler_enabled_ = false;              ///< 崩溃处理器启用状态
    bool hot_reload_enabled_ = false;                 ///< 热重载启用状态
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

// ============================================================================
// 宏定义
// ============================================================================
// 便利宏
#define DEARTS_APP_MANAGER() DearTs::Core::App::ApplicationManager::getInstance()
#define DEARTS_PLUGIN_MANAGER() DearTs::Core::App::PluginManager::getInstance()

#define DEARTS_CREATE_APP(AppClass, ...) \
    DEARTS_APP_MANAGER().createApplication<AppClass>(__VA_ARGS__)

#define DEARTS_RUN_APP(app) \
    DEARTS_APP_MANAGER().runApplication(std::move(app))

#define DEARTS_MAIN_FUNCTION(AppClass, ...) \
    int main(int argc, char* argv[]) { \
        auto& app_manager = DEARTS_APP_MANAGER(); \
        if (!app_manager.initialize()) { \
            return -1; \
        } \
        auto app = DEARTS_CREATE_APP(AppClass, ##__VA_ARGS__); \
        int result = DEARTS_RUN_APP(app); \
        app_manager.shutdown(); \
        return result; \
    }

// 插件导出宏
#ifdef DEARTS_PLATFORM_WINDOWS
    #define DEARTS_PLUGIN_EXPORT extern "C" __declspec(dllexport)
#else
    #define DEARTS_PLUGIN_EXPORT extern "C" __attribute__((visibility("default")))
#endif

#define DEARTS_PLUGIN_CREATE_FUNC "dearts_plugin_create"
#define DEARTS_PLUGIN_DESTROY_FUNC "dearts_plugin_destroy"
#define DEARTS_PLUGIN_INFO_FUNC "dearts_plugin_info"

#define DEARTS_DECLARE_PLUGIN(PluginClass) \
    DEARTS_PLUGIN_EXPORT DearTs::Core::App::IPlugin* dearts_plugin_create() { \
        return new PluginClass(); \
    } \
    DEARTS_PLUGIN_EXPORT void dearts_plugin_destroy(DearTs::Core::App::IPlugin* plugin) { \
        delete plugin; \
    } \
    DEARTS_PLUGIN_EXPORT DearTs::Core::App::PluginInfo dearts_plugin_info() { \
        PluginClass temp; \
        return temp.getInfo(); \
    }

} // namespace App
} // namespace Core
} // namespace DearTs

