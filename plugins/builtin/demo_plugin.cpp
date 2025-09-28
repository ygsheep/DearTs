/**
 * DearTs Demo Plugin Implementation
 * 
 * 演示插件实现 - 展示如何使用DearTs插件框架开发功能插件的完整实现
 * 包含基本的UI组件、菜单项、工具窗口和设置页面的具体实现
 * 
 * @author DearTs Team
 * @version 1.0.0
 * @date 2025
 */

#include "demo_plugin.hpp"
#include "../../lib/libdearts/include/dearts/api/event_manager.hpp"
#include "../../lib/libdearts/include/dearts/helpers/utils.hpp"

#include <imgui.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <filesystem>

namespace DearTs::Plugins {

    DemoPlugin::DemoPlugin() 
        : BuiltinPlugin("DemoPlugin", "DearTs Framework Demo Plugin", "1.0.0") {
        
        // 设置配置文件路径
        m_configPath = "plugins/config/demo_plugin.json";
        
        // 初始化表格数据
        m_tableData.columnHeaders = {"ID", "名称", "类型", "大小", "修改时间"};
        m_tableData.tableData = {
            {"1", "文档.txt", "文本文件", "1.2 KB", "2025-01-15 10:30"},
            {"2", "图片.png", "图像文件", "256 KB", "2025-01-15 11:45"},
            {"3", "音频.mp3", "音频文件", "3.5 MB", "2025-01-15 12:15"},
            {"4", "视频.mp4", "视频文件", "125 MB", "2025-01-15 13:20"},
            {"5", "压缩包.zip", "压缩文件", "45 MB", "2025-01-15 14:10"}
        };
    }

    bool DemoPlugin::onInitialize() {
        // 调用基类初始化
        if (!BuiltinPlugin::onInitialize()) {
            return false;
        }

        try {
            // 注册菜单项
            registerMenuItem("文件/新建", [this]() { onMenuNew(); }, "Ctrl+N");
            registerMenuItem("文件/打开", [this]() { onMenuOpen(); }, "Ctrl+O");
            registerMenuItem("文件/保存", [this]() { onMenuSave(); }, "Ctrl+S");
            registerMenuItem("文件/-", nullptr); // 分隔符
            registerMenuItem("文件/退出", [this]() { onMenuExit(); }, "Ctrl+Q");
            
            registerMenuItem("工具/演示窗口", [this]() { m_windowStates.showDemoWindow = true; });
            registerMenuItem("工具/设置", [this]() { m_windowStates.showSettingsWindow = true; });
            
            registerMenuItem("帮助/关于", [this]() { onMenuAbout(); });

            // 注册工具窗口
            registerToolWindow("演示插件主窗口", [this]() { drawMainWindow(); }, true);
            registerToolWindow("工具箱", [this]() { drawToolsWindow(); }, false);
            registerToolWindow("设置", [this]() { drawSettingsWindow(); }, false);
            registerToolWindow("关于", [this]() { drawAboutWindow(); }, false);

            // 注册设置页面
            registerSettingsPage("演示插件", [this]() { drawGeneralSettings(); });
            registerSettingsPage("演示插件/外观", [this]() { drawAppearanceSettings(); });
            registerSettingsPage("演示插件/高级", [this]() { drawAdvancedSettings(); });

            // 注册快捷键
            registerShortcut("Ctrl+N", [this]() { onShortcutNew(); }, "新建文件");
            registerShortcut("Ctrl+O", [this]() { onShortcutOpen(); }, "打开文件");
            registerShortcut("Ctrl+S", [this]() { onShortcutSave(); }, "保存文件");
            registerShortcut("Ctrl+Q", [this]() { onShortcutQuit(); }, "退出应用");

            // 注册状态栏项目
            addStatusBarItem("demo_info", [this]() { drawStatusInfo(); });
            addStatusBarItem("demo_progress", [this]() { drawStatusProgress(); });
            addStatusBarItem("demo_memory", [this]() { drawStatusMemory(); });

            // TODO: Subscribe to events when event system is implemented
            // subscribeToEvent("ApplicationInitialized", [this](const auto& data) { onApplicationEvent(data); });
            // subscribeToEvent("WindowTitleChanged", [this](const auto& data) { onWindowEvent(data); });
            // subscribeToEvent("PluginLoaded", [this](const auto& data) { onPluginEvent(data); });

            // 显示初始化完成通知
            showNotification("演示插件初始化完成", "success");
            
            return true;
            
        } catch (const std::exception& e) {
            showNotification("演示插件初始化失败: " + std::string(e.what()), "error");
            return false;
        }
    }

    void DemoPlugin::onDeinitialize() {
        // 保存配置
        saveConfig();
        
        // 显示清理完成通知
        showNotification("演示插件已清理", "info");
        
        // 调用基类清理
        BuiltinPlugin::onDeinitialize();
    }

    void DemoPlugin::onDrawContent() {
        // 更新时间
        static auto lastTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        m_deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        m_totalTime += m_deltaTime;
        lastTime = currentTime;
        
        // 更新状态
        m_status.fps = static_cast<int>(1.0f / m_deltaTime);
        
        // 模拟进度更新
        if (m_status.isProcessing) {
            m_status.progress += m_deltaTime * 0.1f; // 10秒完成
            if (m_status.progress >= 1.0f) {
                m_status.progress = 1.0f;
                m_status.isProcessing = false;
                m_status.currentTask = "完成";
                showNotification("任务处理完成", "success");
            }
        }
        
        // 清理过期通知
        auto now = m_totalTime;
        for (int i = static_cast<int>(m_notifications.timestamps.size()) - 1; i >= 0; --i) {
            if (now - m_notifications.timestamps[i] > m_notifications.notificationDuration) {
                m_notifications.messages.erase(m_notifications.messages.begin() + i);
                m_notifications.types.erase(m_notifications.types.begin() + i);
                m_notifications.timestamps.erase(m_notifications.timestamps.begin() + i);
            }
        }
        
        // 调用基类绘制
        BuiltinPlugin::onDrawContent();
        
        // TODO: Draw notifications when implemented
        // drawNotifications();
    }

    void DemoPlugin::onHandleEvent(const std::string& eventName, const std::any& eventData) {
        // 调用基类事件处理
        BuiltinPlugin::onHandleEvent(eventName, eventData);
        
        // 处理特定事件
        if (eventName == "ApplicationInitialized") {
            onApplicationEvent(eventData);
        } else if (eventName == "WindowTitleChanged") {
            onWindowEvent(eventData);
        } else if (eventName == "PluginLoaded") {
            onPluginEvent(eventData);
        }
    }

    void DemoPlugin::loadConfig() {
        try {
            if (std::filesystem::exists(m_configPath)) {
                std::ifstream file(m_configPath);
                if (file.is_open()) {
                    // TODO: 实现JSON配置加载
                    // 这里使用简单的文本格式作为示例
                    std::string line;
                    while (std::getline(file, line)) {
                        if (line.find("fontSize=") == 0) {
                            m_settings.fontSize = std::stof(line.substr(9));
                        } else if (line.find("theme=") == 0) {
                            m_settings.theme = std::stoi(line.substr(6));
                        } else if (line.find("enableAnimations=") == 0) {
                            m_settings.enableAnimations = (line.substr(17) == "true");
                        }
                    }
                    file.close();
                }
            }
        } catch (const std::exception& e) {
            showNotification("配置加载失败: " + std::string(e.what()), "error");
        }
    }

    void DemoPlugin::saveConfig() {
        try {
            // 确保配置目录存在
            std::filesystem::create_directories(std::filesystem::path(m_configPath).parent_path());
            
            std::ofstream file(m_configPath);
            if (file.is_open()) {
                // TODO: 实现JSON配置保存
                // 这里使用简单的文本格式作为示例
                file << "fontSize=" << m_settings.fontSize << "\n";
                file << "theme=" << m_settings.theme << "\n";
                file << "enableAnimations=" << (m_settings.enableAnimations ? "true" : "false") << "\n";
                file.close();
            }
        } catch (const std::exception& e) {
            showNotification("配置保存失败: " + std::string(e.what()), "error");
        }
    }

    void DemoPlugin::resetConfig() {
        m_settings.backgroundColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
        m_settings.textColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        m_settings.accentColor = ImVec4(0.26f, 0.59f, 0.98f, 1.0f);
        m_settings.fontSize = 16.0f;
        m_settings.enableAnimations = true;
        m_settings.showTooltips = true;
        m_settings.theme = 0;
        
        showNotification("配置已重置为默认值", "info");
    }

    void DemoPlugin::drawMainWindow() {
        if (ImGui::Begin("演示插件主窗口", &m_windowStates.showMainWindow)) {
            
            // 工具栏
            if (ImGui::Button("新建")) onToolbarNew();
            ImGui::SameLine();
            if (ImGui::Button("打开")) onToolbarOpen();
            ImGui::SameLine();
            if (ImGui::Button("保存")) onToolbarSave();
            ImGui::SameLine();
            ImGui::Separator();
            ImGui::SameLine();
            if (ImGui::Button("设置")) onToolbarSettings();
            
            ImGui::Separator();
            
            // 标签页
            if (ImGui::BeginTabBar("MainTabs")) {
                
                if (ImGui::BeginTabItem("文本编辑器")) {
                    drawTextEditor();
                    ImGui::EndTabItem();
                }
                
                if (ImGui::BeginTabItem("颜色选择器")) {
                    drawColorPicker();
                    ImGui::EndTabItem();
                }
                
                if (ImGui::BeginTabItem("数据表格")) {
                    drawDataTable();
                    ImGui::EndTabItem();
                }
                
                if (ImGui::BeginTabItem("进度条")) {
                    drawProgressBar();
                    ImGui::EndTabItem();
                }
                
                if (ImGui::BeginTabItem("演示组件")) {
                    drawDemoComponents();
                    ImGui::EndTabItem();
                }
                
                ImGui::EndTabBar();
            }
        }
        ImGui::End();
    }

    void DemoPlugin::drawTextEditor() {
        ImGui::Text("当前文件: %s", m_fileData.currentFile.empty() ? "未命名" : m_fileData.currentFile.c_str());
        
        if (m_fileData.modified) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "*");
        }
        
        ImGui::Separator();
        
        // 文本编辑器
        ImGui::InputTextMultiline("##TextEditor", 
                                 const_cast<char*>(m_fileData.textContent.c_str()),
                                 m_fileData.textContent.capacity(),
                                 ImVec2(-1, -50));
        
        if (ImGui::IsItemEdited()) {
            m_fileData.modified = true;
        }
        
        ImGui::Separator();
        
        // 状态信息
        m_fileData.lineCount = static_cast<int>(std::count(m_fileData.textContent.begin(), m_fileData.textContent.end(), '\n')) + 1;
        ImGui::Text("行数: %d | 字符数: %zu", m_fileData.lineCount, m_fileData.textContent.length());
    }

    void DemoPlugin::drawColorPicker() {
        ImGui::Text("颜色主题设置");
        ImGui::Separator();
        
        ImGui::ColorEdit4("背景色", &m_settings.backgroundColor.x);
        ImGui::ColorEdit4("文本色", &m_settings.textColor.x);
        ImGui::ColorEdit4("强调色", &m_settings.accentColor.x);
        
        ImGui::Separator();
        
        if (ImGui::Button("应用主题")) {
            // TODO: 应用颜色主题到应用程序
            showNotification("主题已应用", "success");
        }
        
        ImGui::SameLine();
        if (ImGui::Button("重置颜色")) {
            m_settings.backgroundColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
            m_settings.textColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            m_settings.accentColor = ImVec4(0.26f, 0.59f, 0.98f, 1.0f);
            showNotification("颜色已重置", "info");
        }
    }

    void DemoPlugin::drawDataTable() {
        ImGui::Text("数据表格演示");
        ImGui::Separator();
        
        // 搜索框
        ImGui::InputText("搜索", m_searchBuffer, sizeof(m_searchBuffer));
        ImGui::SameLine();
        if (ImGui::Button("清除")) {
            memset(m_searchBuffer, 0, sizeof(m_searchBuffer));
        }
        
        ImGui::Checkbox("显示表头", &m_tableData.showHeaders);
        
        ImGui::Separator();
        
        // 表格
        if (ImGui::BeginTable("DataTable", static_cast<int>(m_tableData.columnHeaders.size()), 
                             ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            
            // 表头
            if (m_tableData.showHeaders) {
                for (const auto& header : m_tableData.columnHeaders) {
                    ImGui::TableSetupColumn(header.c_str());
                }
                ImGui::TableHeadersRow();
            }
            
            // 数据行
            for (int row = 0; row < static_cast<int>(m_tableData.tableData.size()); ++row) {
                ImGui::TableNextRow();
                
                bool isSelected = (m_tableData.selectedRow == row);
                
                for (int col = 0; col < static_cast<int>(m_tableData.tableData[row].size()); ++col) {
                    ImGui::TableSetColumnIndex(col);
                    
                    if (ImGui::Selectable(m_tableData.tableData[row][col].c_str(), 
                                        isSelected, ImGuiSelectableFlags_SpanAllColumns)) {
                        m_tableData.selectedRow = row;
                    }
                }
            }
            
            ImGui::EndTable();
        }
        
        if (m_tableData.selectedRow >= 0) {
            ImGui::Text("选中行: %d", m_tableData.selectedRow + 1);
        }
    }

    void DemoPlugin::drawProgressBar() {
        ImGui::Text("进度条演示");
        ImGui::Separator();
        
        ImGui::Text("当前任务: %s", m_status.currentTask.c_str());
        ImGui::ProgressBar(m_status.progress, ImVec2(-1, 0), 
                          (m_status.progress >= 1.0f) ? "完成" : nullptr);
        
        ImGui::Separator();
        
        if (ImGui::Button("开始处理") && !m_status.isProcessing) {
            m_status.isProcessing = true;
            m_status.progress = 0.0f;
            m_status.currentTask = "正在处理...";
            showNotification("开始处理任务", "info");
        }
        
        ImGui::SameLine();
        if (ImGui::Button("重置进度")) {
            m_status.isProcessing = false;
            m_status.progress = 0.0f;
            m_status.currentTask = "就绪";
        }
    }

    void DemoPlugin::drawDemoComponents() {
        ImGui::Text("各种UI组件演示");
        ImGui::Separator();
        
        // 输入组件
        ImGui::InputText("文本输入", m_inputBuffer, sizeof(m_inputBuffer));
        ImGui::SliderFloat("滑块", &m_settings.fontSize, 8.0f, 32.0f);
        ImGui::Combo("下拉框", &m_selectedItem, "选项1\0选项2\0选项3\0");
        
        ImGui::Separator();
        
        // 按钮组件
        if (ImGui::Button("普通按钮")) {
            showNotification("按钮被点击", "info");
        }
        
        ImGui::SameLine();
        if (ImGui::SmallButton("小按钮")) {
            showNotification("小按钮被点击", "info");
        }
        
        ImGui::Separator();
        
        // 复选框和单选按钮
        ImGui::Checkbox("启用动画", &m_settings.enableAnimations);
        ImGui::Checkbox("显示工具提示", &m_settings.showTooltips);
        
        ImGui::RadioButton("深色主题", &m_settings.theme, 0); ImGui::SameLine();
        ImGui::RadioButton("浅色主题", &m_settings.theme, 1); ImGui::SameLine();
        ImGui::RadioButton("经典主题", &m_settings.theme, 2);
        
        ImGui::Separator();
        
        // 帮助标记
        ImGui::Text("帮助标记示例");
        ImGui::SameLine();
        showHelpMarker("这是一个帮助标记，鼠标悬停时显示详细信息。");
        
        ImGui::Separator();
        
        // 通知系统演示
        if (ImGui::CollapsingHeader("通知系统")) {
            if (ImGui::Button("显示信息通知")) {
                showNotification("这是一个信息通知", "info");
            }
            ImGui::SameLine();
            if (ImGui::Button("显示成功通知")) {
                showNotification("操作成功完成！", "success");
            }
            ImGui::SameLine();
            if (ImGui::Button("显示警告通知")) {
                showNotification("这是一个警告信息", "warning");
            }
            ImGui::SameLine();
            if (ImGui::Button("显示错误通知")) {
                showNotification("发生了一个错误！", "error");
            }
            
            // TODO: Draw notifications when implemented
            // drawNotifications();
        }
    }



    void DemoPlugin::showNotification(const std::string& message, const std::string& type) {
        // TODO: Implement notification system
        // Simplified implementation to avoid compilation errors
    }

    // 其他方法的简化实现...
    void DemoPlugin::onMenuNew() { showNotification("新建文件", "info"); }
    void DemoPlugin::onMenuOpen() { showNotification("打开文件", "info"); }
    void DemoPlugin::onMenuSave() { showNotification("保存文件", "success"); }
    void DemoPlugin::onMenuExit() { /* TODO: Implement exit event */ }
    void DemoPlugin::onMenuAbout() { m_windowStates.showAboutWindow = true; }
    
    void DemoPlugin::onApplicationEvent(const std::any& data) {
        showNotification("应用程序事件", "info");
    }
    
    void DemoPlugin::onWindowEvent(const std::any& data) {
        showNotification("窗口事件", "info");
    }
    
    void DemoPlugin::onPluginEvent(const std::any& data) {
        showNotification("插件事件", "info");
    }

    // 状态栏绘制方法
    void DemoPlugin::drawStatusInfo() {
        ImGui::Text("演示插件 | FPS: %d", m_status.fps);
    }
    
    void DemoPlugin::drawStatusProgress() {
        if (m_status.isProcessing) {
            ImGui::ProgressBar(m_status.progress, ImVec2(100, 0));
        }
    }
    
    void DemoPlugin::drawStatusMemory() {
        ImGui::Text("内存: %.1f MB", m_status.memoryUsage / 1024.0f / 1024.0f);
    }

    void DemoPlugin::drawGeneralSettings() {
        // TODO: Implement general settings
        ImGui::Text("通用设置页面");
    }

    void DemoPlugin::drawAppearanceSettings() {
        // TODO: Implement appearance settings
        ImGui::Text("外观设置页面");
    }

    void DemoPlugin::drawAdvancedSettings() {
        // TODO: Implement advanced settings
        ImGui::Text("高级设置页面");
    }

    // 工具栏回调方法
    void DemoPlugin::onToolbarNew() { onMenuNew(); }
    void DemoPlugin::onToolbarOpen() { onMenuOpen(); }
    void DemoPlugin::onToolbarSave() { onMenuSave(); }
    void DemoPlugin::onToolbarSettings() { m_windowStates.showSettingsWindow = true; }

    // 快捷键回调方法
    void DemoPlugin::onShortcutNew() { onMenuNew(); }
    void DemoPlugin::onShortcutOpen() { onMenuOpen(); }
    void DemoPlugin::onShortcutSave() { onMenuSave(); }
    void DemoPlugin::onShortcutQuit() { onMenuExit(); }

    // 窗口绘制方法
    void DemoPlugin::drawToolsWindow() {
        // TODO: Implement tools window
        ImGui::Text("工具窗口");
    }

    void DemoPlugin::drawSettingsWindow() {
        // TODO: Implement settings window
        ImGui::Text("设置窗口");
    }

    void DemoPlugin::drawAboutWindow() {
        // TODO: Implement about window
        ImGui::Text("关于窗口");
        ImGui::Text("DearTs Framework Demo Plugin v1.0.0");
    }

} // namespace DearTs::Plugins

// 插件导出
DEARTS_PLUGIN_SETUP(DearTs::Plugins::DemoPlugin, "DemoPlugin", "DearTs Framework Demo Plugin", "1.0.0")