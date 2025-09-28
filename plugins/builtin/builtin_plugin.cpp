/**
 * DearTs Builtin Plugin Base Class Implementation
 * 
 * 内置插件基类实现 - 为所有内置插件提供通用功能和接口的具体实现
 * 基于ImHex的插件架构设计，简化插件开发流程
 * 
 * @author DearTs Team
 * @version 2.0.0
 * @date 2025
 */

#include "builtin_plugin.hpp"
#include "../../lib/libdearts/include/dearts/api/event_manager.hpp"
#include "../../lib/libdearts/include/dearts/helpers/utils.hpp"

#include <imgui.h>
#include <algorithm>
#include <sstream>
#include <any>
#include <filesystem>

namespace DearTs::Plugins {

    BuiltinPlugin::BuiltinPlugin(const std::string& name, 
                                 const std::string& description,
                                 const std::string& version)
        : dearts::Plugin(std::filesystem::path(name)), m_description(description), m_version(version), 
          m_author("DearTs Team"), m_enabled(true) {
    }

    bool BuiltinPlugin::onInitialize() {
        try {
            // 加载插件配置
            loadConfig();
            
            // 注册所有组件
            registerAllComponents();
            
            return true;
        } catch (const std::exception& e) {
            // 记录错误日志
            return false;
        }
    }

    void BuiltinPlugin::onDeinitialize() {
        try {
            // 保存插件配置
            saveConfig();
            
            // 注销所有组件
            unregisterAllComponents();
            
        } catch (const std::exception& e) {
            // 记录错误日志
        }
    }

    void BuiltinPlugin::onDrawContent() {
        if (!m_enabled) {
            return;
        }

        try {
            // 绘制工具窗口
            drawToolWindows();
            
            // 处理快捷键
            handleShortcuts();
            
            // 绘制状态栏项目
            drawStatusBarItems();
            
        } catch (const std::exception& e) {
            // 记录错误日志
        }
    }

    void BuiltinPlugin::onHandleEvent(const std::string& eventName, const std::any& eventData) {
        // 简单的事件处理实现
        // 子类可以重写此方法来处理特定事件
    }

    void BuiltinPlugin::registerMenuItem(const std::string& menuPath, 
                                        std::function<void()> callback,
                                        const std::string& shortcut) {
        MenuItem item;
        item.path = menuPath;
        item.callback = std::move(callback);
        item.shortcut = shortcut;
        m_menuItems.push_back(std::move(item));
    }

    void BuiltinPlugin::registerToolWindow(const std::string& windowName,
                                          std::function<void()> drawCallback,
                                          bool defaultOpen) {
        ToolWindow window;
        window.name = windowName;
        window.drawCallback = std::move(drawCallback);
        window.isOpen = defaultOpen;
        window.defaultOpen = defaultOpen;
        m_toolWindows.push_back(std::move(window));
    }

    void BuiltinPlugin::registerSettingsPage(const std::string& categoryName,
                                             std::function<void()> drawCallback) {
        SettingsPage page;
        page.category = categoryName;
        page.drawCallback = std::move(drawCallback);
        m_settingsPages.push_back(std::move(page));
    }

    void BuiltinPlugin::registerShortcut(const std::string& keyCombo,
                                         std::function<void()> callback,
                                         const std::string& description) {
        Shortcut shortcut;
        shortcut.keyCombo = keyCombo;
        shortcut.callback = std::move(callback);
        shortcut.description = description;
        m_shortcuts.push_back(std::move(shortcut));
    }

    // 注释掉这两个方法的实现，因为它们在头文件中没有正确声明
    /*
    void BuiltinPlugin::subscribeToEvent(const std::string& eventName,
                                        std::function<void(const std::any&)> callback) {
        // TODO: 实现基于字符串的事件订阅，需要创建通用事件类型
        // 暂时注释掉以避免编译错误
        // EventManager::subscribe(eventName, std::move(callback));
        m_subscribedEvents.push_back(eventName);
    }

    void BuiltinPlugin::publishEvent(const std::string& eventName, const std::any& eventData) {
        // TODO: 实现基于字符串的事件发布，需要创建通用事件类型
        // 暂时注释掉以避免编译错误
        // EventManager::publish(eventName, eventData);
    }
    */

    void BuiltinPlugin::addStatusBarItem(const std::string& name,
                                        std::function<void()> drawCallback) {
        StatusBarItem item;
        item.name = name;
        item.drawCallback = std::move(drawCallback);
        m_statusBarItems.push_back(std::move(item));
    }

    void BuiltinPlugin::createImGuiWindow(const std::string& windowName,
                                         bool* isOpen,
                                         ImGuiWindowFlags flags,
                                         std::function<void()> drawCallback) {
        if (isOpen && *isOpen) {
            if (ImGui::Begin(windowName.c_str(), isOpen, flags)) {
                drawCallback();
            }
            ImGui::End();
        }
    }

    void BuiltinPlugin::showHelpMarker(const std::string& description) {
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(description.c_str());
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }

    void BuiltinPlugin::showTooltip(const std::string& text) {
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(text.c_str());
            ImGui::EndTooltip();
        }
    }

    void BuiltinPlugin::createSettingsGroup(const std::string& groupName,
                                           std::function<void()> drawCallback,
                                           bool defaultOpen) {
        if (ImGui::CollapsingHeader(groupName.c_str(), 
                                  defaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0)) {
            ImGui::Indent();
            drawCallback();
            ImGui::Unindent();
        }
    }

    void BuiltinPlugin::registerAllComponents() {
        // 注册菜单项
        for (const auto& menuItem : m_menuItems) {
            // TODO: 实现菜单注册到内容注册表
            // ContentRegistry::Interface::addMenuItem(menuItem.path, menuItem.callback, menuItem.shortcut);
        }

        // 注册工具窗口
        for (const auto& toolWindow : m_toolWindows) {
            // TODO: 实现工具窗口注册到内容注册表
            // ContentRegistry::Views::add(toolWindow.name, toolWindow.drawCallback);
        }

        // 注册设置页面
        for (const auto& settingsPage : m_settingsPages) {
            // TODO: 实现设置页面注册到内容注册表
            // ContentRegistry::Settings::add(settingsPage.category, settingsPage.drawCallback);
        }

        // 注册快捷键
        for (const auto& shortcut : m_shortcuts) {
            // TODO: 实现快捷键注册到内容注册表
            // ContentRegistry::Interface::addShortcut(shortcut.keyCombo, shortcut.callback);
        }

        // 注册状态栏项目
        for (const auto& statusBarItem : m_statusBarItems) {
            // TODO: 实现状态栏项目注册到内容注册表
            // ContentRegistry::Interface::addStatusBarItem(statusBarItem.name, statusBarItem.drawCallback);
        }
    }

    void BuiltinPlugin::unregisterAllComponents() {
        // 注销所有注册的组件
        // TODO: 实现组件注销逻辑
        
        // 取消事件订阅
        for (const auto& eventName : m_subscribedEvents) {
            // TODO: 实现事件取消订阅
            // EventManager::unsubscribe(eventName);
        }
        m_subscribedEvents.clear();
    }

    void BuiltinPlugin::drawToolWindows() {
        for (auto& toolWindow : m_toolWindows) {
            if (toolWindow.isOpen) {
                createImGuiWindow(toolWindow.name, &toolWindow.isOpen, 
                                ImGuiWindowFlags_None, toolWindow.drawCallback);
            }
        }
    }

    void BuiltinPlugin::drawSettingsPages() {
        for (const auto& settingsPage : m_settingsPages) {
            // 设置页面通常在设置窗口中绘制，这里提供接口
            // 实际绘制由应用程序的设置管理器调用
        }
    }

    void BuiltinPlugin::handleShortcuts() {
        for (const auto& shortcut : m_shortcuts) {
            // TODO: 实现快捷键检测和处理
            // 这需要与输入管理器集成
            /*
            if (InputManager::isKeyComboPressed(shortcut.keyCombo)) {
                shortcut.callback();
            }
            */
        }
    }

    void BuiltinPlugin::drawStatusBarItems() {
        for (const auto& statusBarItem : m_statusBarItems) {
            // 状态栏项目通常在主窗口的状态栏中绘制
            // 这里提供接口，实际绘制由应用程序的状态栏管理器调用
            statusBarItem.drawCallback();
        }
    }

} // namespace DearTs::Plugins