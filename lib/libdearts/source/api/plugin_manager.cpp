/**
 * DearTs Plugin Manager Implementation
 * 
 * 插件管理器实现 - 负责插件的加载、卸载、管理和生命周期控制
 * 基于ImHex的插件架构设计，支持动态加载和热重载
 * 
 * @author DearTs Team
 * @version 2.0.0
 * @date 2025
 */

#include "../../include/dearts/api/plugin_manager.hpp"
#include "../../include/dearts/api/event_manager.hpp"
#include "../../include/dearts/helpers/utils.hpp"
#include "../../include/dearts/dearts.hpp"

#include <filesystem>
#include <fstream>
#include <algorithm>
#include <regex>
#include <nlohmann/json.hpp>

#ifdef _WIN32
    #include <Windows.h>
    #define PLUGIN_EXTENSION ".dll"
    #define LOAD_LIBRARY(path) LoadLibraryA(path)
    #define GET_PROC_ADDRESS(handle, name) GetProcAddress(handle, name)
    #define FREE_LIBRARY(handle) FreeLibrary(handle)
    #define LIBRARY_HANDLE HMODULE
#else
    #include <dlfcn.h>
    #define PLUGIN_EXTENSION ".so"
    #define LOAD_LIBRARY(path) dlopen(path, RTLD_LAZY)
    #define GET_PROC_ADDRESS(handle, name) dlsym(handle, name)
    #define FREE_LIBRARY(handle) dlclose(handle)
    #define LIBRARY_HANDLE void*
#endif

namespace dearts {

    // 静态成员变量定义
    std::vector<std::unique_ptr<Plugin>> PluginManager::s_plugins;
    std::vector<std::filesystem::path> PluginManager::s_pluginSearchPaths;
    std::map<std::string, Plugin*> PluginManager::s_pluginNameMap;

    // Plugin类实现
    Plugin::Plugin(const std::filesystem::path& path) 
        : m_path(path), m_handle(nullptr), m_initialized(false) {
    }

    Plugin::~Plugin() {
        if (m_handle) {
            FREE_LIBRARY(static_cast<LIBRARY_HANDLE>(m_handle));
        }
    }

    bool Plugin::initializePlugin() {
        if (m_initialized) {
            return true;
        }

        // 加载动态库
        m_handle = LOAD_LIBRARY(m_path.string().c_str());
        if (!m_handle) {
            return false;
        }

        // 获取函数指针
        m_functions.initializePlugin = getFunctionPointer("initializePlugin");
        m_functions.getPluginName = getFunctionPointer("getPluginName");
        m_functions.getPluginAuthor = getFunctionPointer("getPluginAuthor");
        m_functions.getPluginDescription = getFunctionPointer("getPluginDescription");
        m_functions.getCompatibleVersion = getFunctionPointer("getCompatibleVersion");
        m_functions.setImGuiContext = getFunctionPointer("setImGuiContext");
        m_functions.isBuiltinPlugin = getFunctionPointer("isBuiltinPlugin");
        m_functions.getSubCommands = getFunctionPointer("getSubCommands");
        m_functions.getFeatures = getFunctionPointer("getFeatures");

        // 调用插件初始化函数
        if (m_functions.initializePlugin) {
            auto initFunc = reinterpret_cast<bool(*)()>(m_functions.initializePlugin);
            m_initialized = initFunc();
        }

        return m_initialized;
    }

    std::string Plugin::getPluginName() const {
        if (m_functions.getPluginName) {
            auto nameFunc = reinterpret_cast<const char*(*)()>(m_functions.getPluginName);
            return std::string(nameFunc());
        }
        return "";
    }

    std::string Plugin::getPluginAuthor() const {
        if (m_functions.getPluginAuthor) {
            auto authorFunc = reinterpret_cast<const char*(*)()>(m_functions.getPluginAuthor);
            return std::string(authorFunc());
        }
        return "";
    }

    std::string Plugin::getPluginDescription() const {
        if (m_functions.getPluginDescription) {
            auto descFunc = reinterpret_cast<const char*(*)()>(m_functions.getPluginDescription);
            return std::string(descFunc());
        }
        return "";
    }

    std::string Plugin::getCompatibleVersion() const {
        if (m_functions.getCompatibleVersion) {
            auto versionFunc = reinterpret_cast<const char*(*)()>(m_functions.getCompatibleVersion);
            return std::string(versionFunc());
        }
        return "";
    }

    void Plugin::setImGuiContext(void* ctx) {
        if (m_functions.setImGuiContext) {
            auto contextFunc = reinterpret_cast<void(*)(void*)>(m_functions.setImGuiContext);
            contextFunc(ctx);
        }
    }

    bool Plugin::isBuiltinPlugin() const {
        if (m_functions.isBuiltinPlugin) {
            auto builtinFunc = reinterpret_cast<bool(*)()>(m_functions.isBuiltinPlugin);
            return builtinFunc();
        }
        return false;
    }

    const std::filesystem::path& Plugin::getPath() const {
        return m_path;
    }

    bool Plugin::isLoaded() const {
        return m_handle != nullptr && m_initialized;
    }

    std::vector<SubCommand> Plugin::getSubCommands() const {
        if (m_functions.getSubCommands) {
            auto commandsFunc = reinterpret_cast<std::vector<SubCommand>*(*)()>(m_functions.getSubCommands);
            auto* commands = commandsFunc();
            return commands ? *commands : std::vector<SubCommand>();
        }
        return {};
    }

    std::vector<Feature> Plugin::getFeatures() const {
        if (m_functions.getFeatures) {
            auto featuresFunc = reinterpret_cast<std::vector<Feature>*(*)()>(m_functions.getFeatures);
            auto* features = featuresFunc();
            return features ? *features : std::vector<Feature>();
        }
        return {};
    }

    void* Plugin::getFunctionPointer(const std::string& symbol) const {
        if (!m_handle) {
            return nullptr;
        }
        return GET_PROC_ADDRESS(static_cast<LIBRARY_HANDLE>(m_handle), symbol.c_str());
    }

    // PluginManager类实现
    bool PluginManager::load(const std::filesystem::path& path) {
        // 检查文件是否存在
        if (!std::filesystem::exists(path)) {
            return false;
        }

        // 检查是否已经加载
        if (isPluginLoaded(path)) {
            return true;
        }

        try {
            // 创建插件实例
            auto plugin = std::make_unique<Plugin>(path);
            
            // 初始化插件
            if (!plugin->initializePlugin()) {
                return false;
            }

            // 获取插件名称并添加到映射表
            std::string pluginName = plugin->getPluginName();
            s_pluginNameMap[pluginName] = plugin.get();
            
            // 添加到插件列表
            s_plugins.push_back(std::move(plugin));
            
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }

    void PluginManager::unload(const std::filesystem::path& path) {
        auto it = std::find_if(s_plugins.begin(), s_plugins.end(),
            [&path](const std::unique_ptr<Plugin>& plugin) {
                return plugin->getPath() == path;
            });
        
        if (it != s_plugins.end()) {
            // 从名称映射表中移除
            std::string pluginName = (*it)->getPluginName();
            s_pluginNameMap.erase(pluginName);
            
            // 从插件列表中移除
            s_plugins.erase(it);
        }
    }

    void PluginManager::reload(const std::filesystem::path& path) {
        unload(path);
        load(path);
    }

    void PluginManager::unloadAll() {
        s_pluginNameMap.clear();
        s_plugins.clear();
    }

    std::vector<Plugin*> PluginManager::getLoadedPlugins() {
        std::vector<Plugin*> plugins;
        plugins.reserve(s_plugins.size());
        
        for (const auto& plugin : s_plugins) {
            plugins.push_back(plugin.get());
        }
        
        return plugins;
    }

    Plugin* PluginManager::getPlugin(const std::string& name) {
        auto it = s_pluginNameMap.find(name);
        return (it != s_pluginNameMap.end()) ? it->second : nullptr;
    }

    bool PluginManager::isPluginLoaded(const std::filesystem::path& path) {
        return std::any_of(s_plugins.begin(), s_plugins.end(),
            [&path](const std::unique_ptr<Plugin>& plugin) {
                return plugin->getPath() == path;
            });
    }

    void PluginManager::addPluginSearchPath(const std::filesystem::path& path) {
        if (std::find(s_pluginSearchPaths.begin(), s_pluginSearchPaths.end(), path) == s_pluginSearchPaths.end()) {
            s_pluginSearchPaths.push_back(path);
        }
    }

    std::vector<std::filesystem::path> PluginManager::getPluginSearchPaths() {
        return s_pluginSearchPaths;
    }

    void PluginManager::loadAllPlugins(bool loadBuiltins) {
        // 从搜索路径加载插件
        for (const auto& searchPath : s_pluginSearchPaths) {
            if (std::filesystem::exists(searchPath) && std::filesystem::is_directory(searchPath)) {
                for (const auto& entry : std::filesystem::directory_iterator(searchPath)) {
                    if (entry.is_regular_file()) {
                        std::string extension = entry.path().extension().string();
                        if (extension == PLUGIN_EXTENSION) {
                            load(entry.path());
                        }
                    }
                }
            }
        }
    }

    std::map<std::string, SubCommand> PluginManager::getAllSubCommands() {
        std::map<std::string, SubCommand> allCommands;
        
        for (const auto& plugin : s_plugins) {
            auto commands = plugin->getSubCommands();
            for (const auto& command : commands) {
                allCommands[command.commandKey] = command;
            }
        }
        
        return allCommands;
    }

    std::map<std::string, std::vector<Feature>> PluginManager::getAllFeatures() {
        std::map<std::string, std::vector<Feature>> allFeatures;
        
        for (const auto& plugin : s_plugins) {
            std::string pluginName = plugin->getPluginName();
            allFeatures[pluginName] = plugin->getFeatures();
        }
        
        return allFeatures;
    }

} // namespace dearts