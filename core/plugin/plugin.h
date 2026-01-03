/**
 * @file plugin.h
 * @brief 插件系统接口和实现
 * @details 提供标准化的插件加载和管理机制
 * @author DearTs Team
 * @date 2025
 * @version 1.0.0
 */

#pragma once

#include "core/result.h"
#include "core/plugin/plugin_dependency.h"
#include <string>
#include <vector>
#include <memory>
#include <filesystem>
#include <unordered_map>

// 前向声明
namespace DearTs::Core::Plugin {
class DynamicLibraryLoader;

// 依赖解析相关前向声明
enum class DependencyResolutionMode;
struct DependencyResolutionResult;
}

#ifdef _WIN32
    #define PLUGIN_EXPORT __declspec(dllexport)
    #define PLUGIN_IMPORT __declspec(dllimport)
#else
    #define PLUGIN_EXPORT __attribute__((visibility("default")))
    #define PLUGIN_IMPORT
#endif

namespace DearTs::Core::Plugin {

/**
 * @brief 插件信息结构
 */
struct PluginInfo {
    std::string name;           ///< 插件名称
    std::string author;         ///< 插件作者
    std::string description;    ///< 插件描述
    std::string version;        ///< 插件版本
    std::string api_version;    ///< 需要的 API 版本

    /**
     * @brief 验证插件 API 版本兼容性
     */
    [[nodiscard]] bool is_api_compatible(const std::string& current_api_version) const;
};

/**
 * @brief 插件基类
 *
 * 所有插件都应该继承此类并实现相应的虚函数
 */
class IPlugin {
public:
    virtual ~IPlugin() = default;

    /**
     * @brief 获取插件信息
     */
    [[nodiscard]] virtual PluginInfo get_info() const = 0;

    /**
     * @brief 获取插件依赖列表
     * @details 声明此插件依赖的其他插件及其版本要求
     * @return 依赖列表（默认为空，保证向后兼容）
     *
     * @example
     * std::vector<PluginDependency> get_dependencies() const override {
     *     return {
     *         PluginDependency::required("Live2DPlugin", ">=1.0.0"),
     *         PluginDependency::optional("FFmpegPlugin", "^2.1.0"),
     *         PluginDependency::soft("AudioPlugin", "~1.2.0")
     *     };
     * }
     */
    [[nodiscard]] virtual std::vector<PluginDependency> get_dependencies() const {
        return {};  // 默认：无依赖（向后兼容）
    }

    /**
     * @brief 插件加载时调用
     * @details 在此函数中注册命令、视图、工具等
     * @return 成功返回 void，失败返回错误信息
     */
    virtual Result<void, std::string> on_load() {
        return Result<void, std::string>::ok();
    }

    /**
     * @brief 插件卸载时调用
     * @details 在此函数中清理资源
     */
    virtual void on_unload() {}

    /**
     * @brief 插件启用时调用
     */
    virtual void on_enable() {}

    /**
     * @brief 插件禁用时调用
     */
    virtual void on_disable() {}
};

/**
 * @brief 插件创建函数类型
 */
using CreatePluginFunc = IPlugin* (*)();

/**
 * @brief 插件销毁函数类型
 */
using DestroyPluginFunc = void (*)(IPlugin*);

/**
 * @brief 插件删除器（用于 unique_ptr）
 */
struct PluginDeleter {
    DestroyPluginFunc destroy_func = nullptr;

    void operator()(IPlugin* plugin) const {
        if (plugin) {
            if (destroy_func) {
                destroy_func(plugin);
            } else {
                delete plugin;
            }
        }
    }
};

/**
 * @brief 插件状态
 */
enum class PluginState {
    Unloaded,   ///< 未加载
    Loaded,     ///< 已加载
    Enabled,    ///< 已启用
    Disabled,   ///< 已禁用
    Error       ///< 错误状态
};

/**
 * @brief 插件包装类
 */
class PluginWrapper {
public:
    explicit PluginWrapper(std::unique_ptr<IPlugin, PluginDeleter> plugin);
    virtual ~PluginWrapper();

    // 删除拷贝
    PluginWrapper(const PluginWrapper&) = delete;
    PluginWrapper& operator=(const PluginWrapper&) = delete;

    // 支持移动
    PluginWrapper(PluginWrapper&&) noexcept;
    PluginWrapper& operator=(PluginWrapper&&) noexcept;

    /**
     * @brief 获取插件
     */
    [[nodiscard]] IPlugin* get() const { return m_plugin.get(); }

    /**
     * @brief 获取插件状态
     */
    [[nodiscard]] PluginState get_state() const { return m_state; }

    /**
     * @brief 获取错误信息
     */
    [[nodiscard]] const std::string& get_error() const { return m_error; }

    /**
     * @brief 加载插件
     */
    Result<void, std::string> load();

    /**
     * @brief 卸载插件
     */
    virtual void unload();

    /**
     * @brief 启用插件
     */
    void enable();

    /**
     * @brief 禁用插件
     */
    void disable();

protected:
    // 默认构造函数，供派生类使用
    PluginWrapper() = default;

    std::unique_ptr<IPlugin, PluginDeleter> m_plugin;
    PluginState m_state = PluginState::Unloaded;
    std::string m_error;
};

/**
 * @brief 动态加载插件包装器
 * @details 管理动态库插件的完整生命周期，包括动态库的加载和卸载
 */
class DynamicPluginWrapper : public PluginWrapper {
public:
    /**
     * @brief 构造函数
     * @param plugin 插件原始指针（使用自定义 deleter）
     * @param destroy_func 插件销毁函数
     * @param loader 动态库加载器
     * @param source_path 插件源文件路径
     */
    DynamicPluginWrapper(
        IPlugin* plugin,
        DestroyPluginFunc destroy_func,
        std::unique_ptr<DynamicLibraryLoader> loader,
        std::string source_path
    );

    ~DynamicPluginWrapper() override;

    // 禁止拷贝和移动
    DynamicPluginWrapper(const DynamicPluginWrapper&) = delete;
    DynamicPluginWrapper& operator=(const DynamicPluginWrapper&) = delete;
    DynamicPluginWrapper(DynamicPluginWrapper&&) noexcept = delete;
    DynamicPluginWrapper& operator=(DynamicPluginWrapper&&) noexcept = delete;

    /**
     * @brief 获取插件源路径
     */
    [[nodiscard]] const std::string& get_source_path() const { return m_source_path; }

    /**
     * @brief 卸载插件和动态库
     */
    void unload() override;

private:
    DestroyPluginFunc m_destroy_func;
    std::unique_ptr<DynamicLibraryLoader> m_loader;
    std::string m_source_path;
};

/**
 * @brief 插件管理器
 *
 * 负责管理所有插件的生命周期
 */
class PluginManager final {  // 单例类，禁止继承
public:
    /**
     * @brief 获取单例实例（线程安全，Magic Statics）
     */
    static PluginManager& instance() noexcept {
        static PluginManager instance;
        return instance;
    }

    // 删除所有拷贝和移动操作
    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;
    PluginManager(PluginManager&&) = delete;
    PluginManager& operator=(PluginManager&&) = delete;

    /**
     * @brief 从动态库加载插件
     * @param path 插件库路径
     * @return 成功返回 void，失败返回错误信息
     */
    Result<void, std::string> load_from_file(const std::filesystem::path& path);

    /**
     * @brief 添加内置插件
     * @param plugin 插件实例
     * @return 成功返回 void，失败返回错误信息
     */
    Result<void, std::string> add_builtin(std::unique_ptr<IPlugin> plugin);

    /**
     * @brief 从目录加载所有插件
     * @param directory 目录路径
     * @return 成功返回插件数量，失败返回错误信息
     */
    Result<size_t, std::string> load_from_directory(const std::filesystem::path& directory);

    /**
     * @brief 卸载插件
     * @param name 插件名称
     * @return 成功返回 true，失败返回 false
     */
    bool unload(const std::string& name);

    /**
     * @brief 启用插件
     * @param name 插件名称
     * @return 成功返回 void，失败返回错误信息
     */
    Result<void, std::string> enable(const std::string& name);

    /**
     * @brief 禁用插件
     * @param name 插件名称
     * @return 成功返回 void，失败返回错误信息
     */
    Result<void, std::string> disable(const std::string& name);

    /**
     * @brief 初始化依赖配置 (NEW)
     * @details 从 ConfigManager 读取依赖解析模式配置
     * @note 应在应用初始化时调用，在使用 load_all_with_dependencies() 之前
     *
     * @example
     * // 在应用启动时调用
     * PluginManager::instance().initialize_dependency_config();
     *
     * // 配置文件 (config.json):
     * // {
     * //   "plugins": {
     * //     "dependency_mode": "strict"  // 或 "lenient"
     * //   }
     * // }
     */
    void initialize_dependency_config();

    /**
     * @brief 设置依赖解析模式 (NEW)
     * @param mode 宽松模式或严格模式
     */
    void set_dependency_mode(DependencyResolutionMode mode);

    /**
     * @brief 获取依赖解析模式 (NEW)
     */
    [[nodiscard]] DependencyResolutionMode get_dependency_mode() const;

    /**
     * @brief 获取最后一次依赖解析结果 (NEW)
     */
    [[nodiscard]] DependencyResolutionResult get_last_resolution_result() const;

    /**
     * @brief 解析并加载所有插件依赖 (NEW)
     * @details 在添加所有插件后调用此方法，会按照依赖顺序加载插件
     * @return 成功返回 void，失败返回错误信息（仅严格模式）
     */
    Result<void, std::string> load_all_with_dependencies();

    /**
     * @brief 重载插件
     * @param name 插件名称
     * @return 成功返回 void，失败返回错误信息
     */
    Result<void, std::string> reload(const std::string& name);

    /**
     * @brief 获取插件
     * @param name 插件名称
     * @return 插件指针，如果不存在返回 nullptr
     */
    [[nodiscard]] IPlugin* get_plugin(const std::string& name);

    /**
     * @brief 获取所有插件信息
     */
    [[nodiscard]] std::vector<PluginInfo> get_all_plugins_info() const;

    /**
     * @brief 获取插件状态
     * @param name 插件名称
     */
    [[nodiscard]] Result<PluginState, std::string> get_plugin_state(const std::string& name) const;

    /**
     * @brief 清空所有插件
     */
    void clear();

private:
    PluginManager() = default;
    ~PluginManager() = default;

    std::unordered_map<std::string, std::unique_ptr<PluginWrapper>> m_plugins;

    // 依赖解析相关成员 (NEW)
    // 使用 int 避免前向声明问题 (0 = Lenient, 1 = Strict)
    int m_dependency_mode = 0;  // 0 = Lenient, 1 = Strict
};

} // namespace DearTs::Core::Plugin

/**
 * @brief 插件导出宏（用于内置插件）
 */
#define DEARTS_BUILTIN_PLUGIN(PluginClass) \
    extern "C" { \
        DearTs::Core::Plugin::IPlugin* dearts_create_plugin() { \
            return new PluginClass(); \
        } \
        void dearts_destroy_plugin(DearTs::Core::Plugin::IPlugin* plugin) { \
            delete plugin; \
        } \
    }
