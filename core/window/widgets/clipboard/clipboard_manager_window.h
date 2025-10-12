#pragma once

#include "../../window_base.h"
#include "clipboard_history_layout.h"
#include <memory>
#include <string>

namespace DearTs::Core::Window::Widgets::Clipboard {

/**
 * @brief 剪切板管理器窗口类
 *
 * 独立的无边框窗口，用于管理剪切板历史记录和文本分词分析
 * - 无边框窗口设计，可自由移动和调整大小
 * - 集成剪切板历史记录和分词分析功能
 * - 支持实时剪切板监听
 * - 提供完整的用户交互界面
 */
class ClipboardManagerWindow : public WindowBase {
public:
    /**
     * @brief 构造函数
     */
    explicit ClipboardManagerWindow(const std::string& title = "剪切板管理器");

    /**
     * @brief 析构函数
     */
    ~ClipboardManagerWindow() override;

    /**
     * @brief 初始化窗口
     * @return 初始化是否成功
     */
    bool initialize() override;

    /**
     * @brief 渲染窗口内容
     */
    void render() override;

    /**
     * @brief 更新窗口逻辑
     */
    void update() override;

    /**
     * @brief 处理窗口事件
     * @param event SDL事件
     */
    void handleEvent(const SDL_Event& event) override;

    // 窗口显示/隐藏
    void showWindow();
    void hideWindow();
    void toggleWindow();
    bool isVisible() const { return is_visible_; }

    // 剪切板管理功能
    void refreshHistory();
    void clearHistory();

private:
    // 成员变量
    std::unique_ptr<ClipboardHistoryLayout> clipboard_layout_;  // 剪切板历史布局
    bool is_visible_;  // 窗口可见性
    bool initialized_;  // 是否已初始化

    // 布局计算
    void calculateLayout();
    ImVec2 getWindowPosition() const;
    ImVec2 getWindowSize() const;

    // 渲染自定义标题栏
    void renderCustomTitleBar();

    // 事件处理
    void handleMouseEvents(const SDL_Event& event);
    void handleKeyboardEvents(const SDL_Event& event);
    void handleWindowEvents(const SDL_Event& event);
};

} // namespace DearTs::Core::Window::Widgets::Clipboard