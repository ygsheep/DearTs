#pragma once

#include "../../../core/window/layouts/layout_base.h"
#include "../../../core/window/layouts/layout_manager.h"
#include "../../../core/window/widgets/clipboard/text_segmenter.h"
#include <memory>
#include <string>

namespace DearTs::Examples {

/**
 * @brief 分词测试布局类
 *
 * 用于测试文本分词功能的简单布局
 * 显示测试文本和分词结果
 */
class SegmentationTestLayout : public DearTs::Core::Window::LayoutBase {
public:
    /**
     * @brief 构造函数
     */
    SegmentationTestLayout();

    /**
     * @brief 析构函数
     */
    ~SegmentationTestLayout() override;

    /**
     * @brief 设置测试文本
     * @param text 要测试的文本
     */
    void setTestText(const std::string& text);

    /**
     * @brief 渲染布局内容
     */
    void render() override;

    /**
     * @brief 更新布局
     */
    void updateLayout(float width, float height) override;

    /**
     * @brief 处理事件
     */
    void handleEvent(const SDL_Event& event) override;

private:
    std::unique_ptr<DearTs::Core::Window::Widgets::Clipboard::TextSegmenter> textSegmenter_;
    std::string testText_;
    bool initialized_;
};

} // namespace DearTs::Examples