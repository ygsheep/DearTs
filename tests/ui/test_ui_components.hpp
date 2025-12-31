/**
 * @file test_ui_components.hpp
 * @brief 测试用 UI 组件 - 为 Phase 4 UI 测试提供可测试的界面元素
 */

#ifndef DEARTS_TEST_UI_COMPONENTS_HPP
#define DEARTS_TEST_UI_COMPONENTS_HPP

#include <imgui.h>
#include <string>
#include <vector>
#include <functional>

namespace DearTs::TestUI {

/**
 * @brief Toast 通知数据结构
 */
struct Toast {
    std::string title;
    std::string message;
    int type;  // 0=Info, 1=Warning, 2=Error, 3=Success
    
    Toast(const char* t, const char* m, int ty) 
        : title(t), message(m), type(ty) {}
};

/**
 * @brief 测试用 TitleBar 组件
 */
class TestTitleBar {
public:
    static void render();
    static bool isSettingsClicked();
    static bool isTasksClicked();
    static bool showFileMenu();
    static bool showViewMenu();
    static bool showThemeMenu();
    
    static void reset() { s_settingsClicked = s_tasksClicked = false; }
    
private:
    static inline bool s_settingsClicked = false;
    static inline bool s_tasksClicked = false;
};

/**
 * @brief 测试用 CommandPalette 组件
 */
class TestCommandPalette {
public:
    static void render();
    static void open();
    static void close();
    static bool isOpen();
    static bool isFiltering();
    
    static void setCommands(const std::vector<std::string>& cmds);
    static const std::vector<std::string>& getCommands() { return s_commands; }
    
private:
    static inline bool s_open = false;
    static inline char s_filterBuffer[256] = "";
    static inline int s_selectedIndex = 0;
    static inline std::vector<std::string> s_commands;
};

/**
 * @brief Toast 管理器
 */
class TestToastManager {
public:
    static void render();
    static void show(const Toast& toast);
    static void showInfo(const char* title, const char* msg);
    static void showWarning(const char* title, const char* msg);
    static void showError(const char* title, const char* msg);
    static void showSuccess(const char* title, const char* msg);
    
    static size_t getActiveCount() { return s_toasts.size(); }
    static void clear() { s_toasts.clear(); }
    
private:
    static inline std::vector<Toast> s_toasts;
};

/**
 * @brief 测试 View 组件
 */
class TestView {
public:
    static void render(const char* name, bool& open);
    static void renderAll();
    
    static inline struct ViewState {
        bool open = false;
        bool focused = false;
        float size[2] = {400, 300};
    } s_states[4];
};

/**
 * @brief 全局测试 UI 渲染器
 */
class TestUIRenderer {
public:
    static void renderAll();
    static void showDemoWindow();
    static void showTestWindow();
    
    static inline bool s_demoWindow = false;
    static inline bool s_testWindow = true;
};

} // namespace DearTs::TestUI

#endif // DEARTS_TEST_UI_COMPONENTS_HPP
