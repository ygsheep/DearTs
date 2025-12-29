/**
 * @file plugin.h
 * @brief 插件系统接口和实现
 * @details 提供标准化的插件加载和管理机制
 * @author DearTs Team
 * @date 2024
 * @version 1.0.0
 */

#pragma once

#include "core/result.h"
#include <string>
#include <vector>
#include <memory>
#include <filesystem>
#include <unordered_map>

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
    explicit PluginWrapper(std::unique_ptr<IPlugin> plugin);
    ~PluginWrapper();

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
    void unload();

    /**
     * @brief 启用插件
     */
    void enable();

    /**
     * @brief 禁用插件
     */
    void disable();

private:
    std::unique_ptr<IPlugin> m_plugin;
    PluginState m_state = PluginState::Unloaded;
    std::string m_error;
};

/**
 * @brief 插件管理器
 *
 * 负责管理所有插件的生命周期
 */
class PluginManager {
public:
    /**
     * @brief 获取单例实例
     */
    static PluginManager& instance();

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

    // 删除拷贝和移动
    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;
    PluginManager(PluginManager&&) = delete;
    PluginManager& operator=(PluginManager&&) = delete;

    std::unordered_map<std::string, std::unique_ptr<PluginWrapper>> m_plugins;
};

/**
 * @brief 插件创建函数类型
 */
using CreatePluginFunc = IPlugin* (*)();

/**
 * @brief 插件销毁函数类型
 */
using DestroyPluginFunc = void (*)(IPlugin*);

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
