/**
 * DearTs Demo Plugin
 * 
 * 演示插件 - 展示如何使用DearTs插件框架开发功能插件
 * 包含基本的UI组件、菜单项、工具窗口和设置页面
 * 
 * @author DearTs Team
 * @version 1.0.0
 * @date 2025
 */

#pragma once

#include "builtin_plugin.hpp"
#include <imgui.h>
#include <string>
#include <vector>

namespace DearTs::Plugins {

    /**
     * 演示插件类
     * 展示插件框架的各种功能和用法
     */
    class DemoPlugin : public BuiltinPlugin {
    public:
        /**
         * 构造函数
         */
        DemoPlugin();
        
        /**
         * 析构函数
         */
        ~DemoPlugin() override = default;

        // 插件生命周期重写
        bool onInitialize() override;
        void onDeinitialize() override;
        void onDrawContent() override;
        void onHandleEvent(const std::string& eventName, const std::any& eventData) override;

        // 配置管理
        void loadConfig() override;
        void saveConfig() override;
        void resetConfig() override;

    private:
        // UI绘制方法
        void drawMainWindow();
        void drawToolsWindow();
        void drawSettingsWindow();
        void drawAboutWindow();
        void drawDemoComponents();
        void drawTextEditor();
        void drawColorPicker();
        void drawFileDialog();
        void drawProgressBar();
        void drawDataTable();
        
        // 功能方法
        void showNotification(const std::string& message, const std::string& type = "info");
        void exportData();
        void importData();
        void resetAllSettings();
        void showHelpDialog();
        
        // 事件处理方法
        void onApplicationEvent(const std::any& data);
        void onWindowEvent(const std::any& data);
        void onPluginEvent(const std::any& data);
        
        // 菜单回调方法
        void onMenuNew();
        void onMenuOpen();
        void onMenuSave();
        void onMenuExit();
        void onMenuAbout();
        
        // 工具栏回调方法
        void onToolbarNew();
        void onToolbarOpen();
        void onToolbarSave();
        void onToolbarSettings();
        
        // 快捷键回调方法
        void onShortcutNew();
        void onShortcutOpen();
        void onShortcutSave();
        void onShortcutQuit();
        
        // 设置页面绘制方法
        void drawGeneralSettings();
        void drawAppearanceSettings();
        void drawAdvancedSettings();
        
        // 状态栏项目绘制方法
        void drawStatusInfo();
        void drawStatusProgress();
        void drawStatusMemory();

        // 私有成员变量
        struct {
            bool showMainWindow = true;
            bool showToolsWindow = false;
            bool showSettingsWindow = false;
            bool showAboutWindow = false;
            bool showDemoWindow = false;
        } m_windowStates;
        
        struct {
            std::string currentFile = "";
            std::string textContent = "Hello, DearTs Plugin Framework!\n\n这是一个演示插件，展示了如何使用DearTs插件框架开发功能丰富的插件。\n\n功能特性：\n• 菜单项和工具栏\n• 工具窗口和对话框\n• 设置页面和配置管理\n• 事件系统和快捷键\n• 状态栏和通知系统";
            bool modified = false;
            int lineCount = 0;
        } m_fileData;
        
        struct {
            ImVec4 backgroundColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
            ImVec4 textColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            ImVec4 accentColor = ImVec4(0.26f, 0.59f, 0.98f, 1.0f);
            float fontSize = 16.0f;
            bool enableAnimations = true;
            bool showTooltips = true;
            int theme = 0; // 0=Dark, 1=Light, 2=Classic
        } m_settings;
        
        struct {
            float progress = 0.0f;
            std::string currentTask = "就绪";
            bool isProcessing = false;
            size_t memoryUsage = 0;
            int fps = 0;
        } m_status;
        
        struct {
            std::vector<std::string> messages;
            std::vector<std::string> types;
            std::vector<float> timestamps;
            float notificationDuration = 3.0f;
        } m_notifications;
        
        struct {
            std::vector<std::vector<std::string>> tableData;
            std::vector<std::string> columnHeaders;
            int selectedRow = -1;
            bool showHeaders = true;
        } m_tableData;
        
        // 配置文件路径
        std::string m_configPath;
        
        // 计时器
        float m_deltaTime = 0.0f;
        float m_totalTime = 0.0f;
        
        // 临时变量
        char m_inputBuffer[1024] = {0};
        char m_searchBuffer[256] = {0};
        int m_selectedItem = 0;
        bool m_showColorPicker = false;
        bool m_showFileDialog = false;
    };

} // namespace DearTs::Plugins