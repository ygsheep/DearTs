#pragma once

#include <dearts/dearts.hpp>
#include <dearts/api/event_manager.hpp>

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <filesystem>

namespace dearts {
    
    /**
     * @brief 插件子命令结构
     */
    struct SubCommand {
        std::string commandKey;           ///< 命令键
        UnlocalizedString unlocalizedName; ///< 未本地化名称
        UnlocalizedString unlocalizedDescription; ///< 未本地化描述
        std::function<void(const std::vector<std::string>&)> callback; ///< 回调函数
        
        // 默认构造函数
        SubCommand() : unlocalizedName(""), unlocalizedDescription("") {}
        
        // 带参数构造函数
        SubCommand(const std::string& key, const std::string& name, const std::string& desc, 
                  std::function<void(const std::vector<std::string>&)> cb = nullptr)
            : commandKey(key), unlocalizedName(name), unlocalizedDescription(desc), callback(std::move(cb)) {}
    };
    
    /**
     * @brief 插件功能特性结构
     */
    struct Feature {
        std::string name;        ///< 功能名称
        bool enabled;           ///< 是否启用
    };
    
    /**
     * @brief 插件函数指针结构
     */
    struct PluginFunctions {
        void* initializePlugin;     ///< 初始化插件函数
        void* getPluginName;        ///< 获取插件名称函数
        void* getPluginAuthor;      ///< 获取插件作者函数
        void* getPluginDescription; ///< 获取插件描述函数
        void* getCompatibleVersion; ///< 获取兼容版本函数
        void* setImGuiContext;      ///< 设置ImGui上下文函数
        void* isBuiltinPlugin;      ///< 是否内置插件函数
        void* getSubCommands;       ///< 获取子命令函数
        void* getFeatures;          ///< 获取功能特性函数
    };
    
    /**
     * @brief 插件类
     */
    class Plugin {
    public:
        /**
         * @brief 构造函数
         * @param path 插件路径
         */
        explicit Plugin(const std::filesystem::path &path);
        
        /**
         * @brief 析构函数
         */
        ~Plugin();
        
        /**
         * @brief 初始化插件
         * @return 是否成功
         */
        bool initializePlugin();
        
        /**
         * @brief 获取插件名称
         * @return 插件名称
         */
        std::string getPluginName() const;
        
        /**
         * @brief 获取插件作者
         * @return 插件作者
         */
        std::string getPluginAuthor() const;
        
        /**
         * @brief 获取插件描述
         * @return 插件描述
         */
        std::string getPluginDescription() const;
        
        /**
         * @brief 获取兼容版本
         * @return 兼容版本
         */
        std::string getCompatibleVersion() const;
        
        /**
         * @brief 设置ImGui上下文
         * @param ctx ImGui上下文
         */
        void setImGuiContext(void* ctx);
        
        /**
         * @brief 检查是否为内置插件
         * @return 是否为内置插件
         */
        bool isBuiltinPlugin() const;
        
        /**
         * @brief 获取插件路径
         * @return 插件路径
         */
        const std::filesystem::path& getPath() const;
        
        /**
         * @brief 检查插件是否已加载
         * @return 是否已加载
         */
        bool isLoaded() const;
        
        /**
         * @brief 获取子命令列表
         * @return 子命令列表
         */
        std::vector<SubCommand> getSubCommands() const;
        
        /**
         * @brief 获取功能特性列表
         * @return 功能特性列表
         */
        std::vector<Feature> getFeatures() const;
        
        /**
         * @brief 模板函数：调用插件函数
         * @tparam Args 参数类型
         * @param symbol 函数符号名
         * @param args 函数参数
         * @return 函数返回值
         */
        template<typename Ret = void, typename... Args>
        std::conditional_t<std::is_void_v<Ret>, void, std::optional<Ret>> 
        callFunction(const std::string &symbol, Args... args) const {
            if (!isLoaded()) {
                if constexpr (!std::is_void_v<Ret>) {
                    return std::nullopt;
                } else {
                    return;
                }
            }
            
            // 获取函数指针并调用
            auto func = getFunctionPointer(symbol);
            if (func == nullptr) {
                if constexpr (!std::is_void_v<Ret>) {
                    return std::nullopt;
                } else {
                    return;
                }
            }
            
            using FuncType = std::conditional_t<std::is_void_v<Ret>, 
                                              void(*)(Args...), 
                                              Ret(*)(Args...)>;
            auto typedFunc = reinterpret_cast<FuncType>(func);
            
            if constexpr (std::is_void_v<Ret>) {
                typedFunc(args...);
            } else {
                return typedFunc(args...);
            }
        }
        
    private:
        /**
         * @brief 获取函数指针
         * @param symbol 函数符号名
         * @return 函数指针
         */
        void* getFunctionPointer(const std::string &symbol) const;
        
        std::filesystem::path m_path;     ///< 插件路径
        void* m_handle = nullptr;         ///< 动态库句柄
        PluginFunctions m_functions = {}; ///< 插件函数指针
        bool m_initialized = false;       ///< 是否已初始化
    };
    
    /**
     * @brief 插件管理器类
     */
    class PluginManager {
    public:
        /**
         * @brief 加载插件
         * @param path 插件路径
         * @return 是否成功
         */
        static bool load(const std::filesystem::path &path);
        
        /**
         * @brief 卸载插件
         * @param path 插件路径
         */
        static void unload(const std::filesystem::path &path);
        
        /**
         * @brief 重新加载插件
         * @param path 插件路径
         */
        static void reload(const std::filesystem::path &path);
        
        /**
         * @brief 卸载所有插件
         */
        static void unloadAll();
        
        /**
         * @brief 获取已加载的插件列表
         * @return 插件列表
         */
        static std::vector<Plugin*> getLoadedPlugins();
        
        /**
         * @brief 根据名称获取插件
         * @param name 插件名称
         * @return 插件指针，未找到返回nullptr
         */
        static Plugin* getPlugin(const std::string &name);
        
        /**
         * @brief 检查插件是否已加载
         * @param path 插件路径
         * @return 是否已加载
         */
        static bool isPluginLoaded(const std::filesystem::path &path);
        
        /**
         * @brief 添加插件搜索路径
         * @param path 搜索路径
         */
        static void addPluginSearchPath(const std::filesystem::path &path);
        
        /**
         * @brief 获取插件搜索路径列表
         * @return 搜索路径列表
         */
        static std::vector<std::filesystem::path> getPluginSearchPaths();
        
        /**
         * @brief 扫描并加载插件目录中的所有插件
         * @param loadBuiltins 是否加载内置插件
         */
        static void loadAllPlugins(bool loadBuiltins = true);
        
        /**
         * @brief 获取所有子命令
         * @return 子命令映射表
         */
        static std::map<std::string, SubCommand> getAllSubCommands();
        
        /**
         * @brief 获取所有功能特性
         * @return 功能特性映射表
         */
        static std::map<std::string, std::vector<Feature>> getAllFeatures();
        
    private:
        static std::vector<std::unique_ptr<Plugin>> s_plugins;           ///< 插件列表
        static std::vector<std::filesystem::path> s_pluginSearchPaths;   ///< 插件搜索路径
        static std::map<std::string, Plugin*> s_pluginNameMap;          ///< 插件名称映射表
    };
    
}