#pragma once

#include "../../window_base.h"
#include <memory>
#include <string>
#include <vector>

namespace DearTs::Core::Window::Widgets::Clipboard {

/**
 * @brief 分词助手窗口类
 *
 * 独立的无边框窗口，用于文本分词分析
 * - 无边框窗口设计，可自由移动和调整大小
 * - 专门的文本分词和语义分析功能
 * - 支持URL提取、关键词分析
 * - 提供完整的用户交互界面
 */
class TextSegmentationWindow : public WindowBase {
public:
    /**
     * @brief 构造函数
     * @param title 窗口标题
     * @param content 要分析的内容
     */
    explicit TextSegmentationWindow(const std::string& title = "分词助手", const std::string& content = "");

    /**
     * @brief 析构函数
     */
    ~TextSegmentationWindow() override;

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

    /**
     * @brief 重写WindowBase的显示方法
     */
    void show();

    /**
     * @brief 重写WindowBase的隐藏方法
     */
    void hide();

    // 窗口显示/隐藏
    void showWindow();
    void hideWindow();
    void toggleWindow();
    
    // 内容操作
    void setContent(const std::string& content);
    std::string getContent() const;

    // 静态创建方法
    static std::unique_ptr<TextSegmentationWindow> create(const std::string& content = "");

private:
    // 成员变量
    std::string content_;                                                  // 要分析的内容
    bool initialized_;                                                    // 是否已初始化

    // 布局注册
    void registerDefaultLayouts() override;

    // 布局计算
    void calculateLayout();

    // 事件处理
    void handleMouseEvents(const SDL_Event& event);
    void handleKeyboardEvents(const SDL_Event& event);
    void handleWindowEvents(const SDL_Event& event);
};

} // namespace DearTs::Core::Window::Widgets::Clipboard