/**
 * DearTs Builtin Plugin Base Class
 * 
 * 内置插件基类 - 为所有内置插件提供通用功能和接口
 * 基于ImHex的插件架构设计，简化插件开发流程
 * 
 * @author DearTs Team
 * @version 2.0.0
 * @date 2025
 */

#pragma once

#include "../../lib/libdearts/include/dearts/api/plugin_manager.hpp"
#include "../../lib/libdearts/include/dearts/api/content_registry.hpp"
#include "../../lib/libdearts/include/dearts/api/event_manager.hpp"
#include "../../lib/libdearts/include/dearts/dearts.hpp"

#include <imgui.h>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <any>

namespace DearTs::Plugins {

    /**
     * 内置插件基类
     * 提供插件开发的通用功能和便利方法
     */
    class BuiltinPlugin : public dearts::Plugin {
    public:
        /**
         * 构造函数
         * @param name 插件名称
         * @param description 插件描述
         * @param version 插件版本
         */
        explicit BuiltinPlugin(const std::string& name, 
                               const std::string& description = "",
                               const std::string& version = "1.0.0");
        
        /**
         * 析构函数
         */
        virtual ~BuiltinPlugin() = default;

        // 基础插件信息
        const std::string& getDescription() const { return m_description; }
        const std::string& getVersion() const { return m_version; }
        const std::string& getAuthor() const { return m_author; }
        
        // 插件状态管理
        bool isEnabled() const { return m_enabled; }
        void setEnabled(bool enabled) { m_enabled = enabled; }
        
        // 插件配置
        virtual void loadConfig() {}
        virtual void saveConfig() {}
        virtual void resetConfig() {}
        
        // 插件生命周期（子类可重写）
        virtual bool onInitialize();
        virtual void onDeinitialize();
        virtual void onDrawContent();
        virtual void onHandleEvent(const std::string& eventName, const std::any& eventData);

    protected:
        // 便利方法
        
        /**
         * 注册菜单项
         * @param menuPath 菜单路径（如 "Tools/My Tool"）
         * @param callback 回调函数
         * @param shortcut 快捷键（可选）
         */
        void registerMenuItem(const std::string& menuPath, 
                            std::function<void()> callback,
                            const std::string& shortcut = "");
        
        /**
         * 注册工具窗口
         * @param windowName 窗口名称
         * @param drawCallback 绘制回调函数
         * @param defaultOpen 默认是否打开
         */
        void registerToolWindow(const std::string& windowName,
                              std::function<void()> drawCallback,
                              bool defaultOpen = false);
        
        /**
         * 注册设置页面
         * @param categoryName 设置分类名称
         * @param drawCallback 绘制回调函数
         */
        void registerSettingsPage(const std::string& categoryName,
                                 std::function<void()> drawCallback);
        
        /**
         * 注册快捷键
         * @param keyCombo 快捷键组合
         * @param callback 回调函数
         * @param description 描述
         */
        void registerShortcut(const std::string& keyCombo,
                             std::function<void()> callback,
                             const std::string& description = "");
        
        /**
         * 添加状态栏项目
         * @param name 项目名称
         * @param drawCallback 绘制回调函数
         */
        void addStatusBarItem(const std::string& name,
                            std::function<void()> drawCallback);
        
        /**
         * 创建ImGui窗口
         * @param windowName 窗口名称
         * @param isOpen 窗口开关状态指针
         * @param flags 窗口标志
         * @param drawCallback 绘制回调函数
         */
        void createImGuiWindow(const std::string& windowName,
                             bool* isOpen,
                             ImGuiWindowFlags flags,
                             std::function<void()> drawCallback);
        
        /**
         * 显示帮助标记
         * @param description 帮助文本
         */
        void showHelpMarker(const std::string& description);
        
        /**
         * 显示工具提示
         * @param text 提示文本
         */
        void showTooltip(const std::string& text);
        
        /**
         * 创建可折叠的设置组
         * @param groupName 组名称
         * @param drawCallback 绘制回调函数
         * @param defaultOpen 默认是否展开
         */
        void createSettingsGroup(const std::string& groupName,
                               std::function<void()> drawCallback,
                               bool defaultOpen = true);

        // 插件信息
        std::string m_description;
        std::string m_version;
        std::string m_author;
        bool m_enabled;
        
        // 注册的组件
        struct MenuItem {
            std::string path;
            std::function<void()> callback;
            std::string shortcut;
        };
        
        struct ToolWindow {
            std::string name;
            std::function<void()> drawCallback;
            bool isOpen;
            bool defaultOpen;
        };
        
        struct SettingsPage {
            std::string category;
            std::function<void()> drawCallback;
        };
        
        struct Shortcut {
            std::string keyCombo;
            std::function<void()> callback;
            std::string description;
        };
        
        struct StatusBarItem {
            std::string name;
            std::function<void()> drawCallback;
        };
        
        std::vector<MenuItem> m_menuItems;
        std::vector<ToolWindow> m_toolWindows;
        std::vector<SettingsPage> m_settingsPages;
        std::vector<Shortcut> m_shortcuts;
        std::vector<StatusBarItem> m_statusBarItems;
        std::vector<std::string> m_subscribedEvents;

    private:
        // 内部方法
        void registerAllComponents();
        void unregisterAllComponents();
        void drawToolWindows();
        void drawSettingsPages();
        void handleShortcuts();
        void drawStatusBarItems();
    };

    /**
     * 插件工厂宏
     * 用于简化插件创建代码
     */
    #define DEARTS_PLUGIN_SETUP(PluginClass, pluginName, pluginDesc, pluginVer) \
        extern "C" { \
            __declspec(dllexport) dearts::Plugin* createPlugin() { \
                return new PluginClass(); \
            } \
            __declspec(dllexport) void destroyPlugin(dearts::Plugin* plugin) { \
                delete plugin; \
            } \
            __declspec(dllexport) const char* getPluginName() { \
                return pluginName; \
            } \
            __declspec(dllexport) const char* getPluginVersion() { \
                return pluginVer; \
            } \
            __declspec(dllexport) const char* getPluginDescription() { \
                return pluginDesc; \
            } \
        }

    /**
     * 简化的插件宏（使用默认信息）
     */
    #define DEARTS_PLUGIN(PluginClass, pluginName) \
        DEARTS_PLUGIN_SETUP(PluginClass, pluginName, "", "1.0.0")

} // namespace DearTs::Plugins