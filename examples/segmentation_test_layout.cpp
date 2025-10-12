#include "segmentation_test_layout.h"
#include <imgui.h>
#include <iostream>

namespace DearTs::Examples {

/**
 * SegmentationTestLayout构造函数
 */
SegmentationTestLayout::SegmentationTestLayout()
    : DearTs::Core::Window::LayoutBase("SegmentationTest")
    , initialized_(false) {
    textSegmenter_ = std::make_unique<DearTs::Core::Window::Widgets::Clipboard::TextSegmenter>();
    testText_ = "这是一个测试文本，用于验证分词助手窗口的渲染功能。";
}

/**
 * SegmentationTestLayout析构函数
 */
SegmentationTestLayout::~SegmentationTestLayout() {
    // 资源自动清理
}

/**
 * 设置测试文本
 */
void SegmentationTestLayout::setTestText(const std::string& text) {
    testText_ = text;
}

/**
 * 渲染布局内容
 */
void SegmentationTestLayout::render() {
    if (!isVisible()) {
        return;
    }

    // 计算可用内容区域（排除标题栏）
    float titleBarHeight = 40.0f;
    float contentY = titleBarHeight;
    float contentHeight = ImGui::GetIO().DisplaySize.y - titleBarHeight;

    // 设置内容区域
    ImGui::SetNextWindowPos(ImVec2(0, contentY));
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, contentHeight));
    ImGui::Begin("##SegmentationContent", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

    // 显示测试文本
    ImGui::Text("测试文本:");
    ImGui::TextWrapped("%s", testText_.c_str());
    ImGui::Separator();

    // 执行分词
    if (textSegmenter_) {
        auto segments = textSegmenter_->segmentText(testText_,
            DearTs::Core::Window::Widgets::Clipboard::TextSegmenter::Method::MIXED_MODE);

        ImGui::Text("分词结果 (%zu 个片段):", segments.size());

        for (size_t i = 0; i < segments.size(); ++i) {
            const auto& segment = segments[i];
            ImGui::Text("[%zu] \"%s\" (%s)",
                i + 1,
                segment.text.c_str(),
                segment.tag.c_str());
        }
    } else {
        ImGui::Text("分词器未初始化");
    }

    ImGui::Separator();
    ImGui::Text("按 ESC 键退出");

    ImGui::End();
}

/**
 * 更新布局
 */
void SegmentationTestLayout::updateLayout(float width, float height) {
    if (!initialized_ && textSegmenter_) {
        if (textSegmenter_->initialize()) {
            initialized_ = true;
            std::cout << "分词测试布局初始化成功" << std::endl;
        }
    }
}

/**
 * 处理事件
 */
void SegmentationTestLayout::handleEvent(const SDL_Event& event) {
    if (!isVisible()) {
        return;
    }

    switch (event.type) {
        case SDL_KEYDOWN:
            if (event.key.keysym.sym == SDLK_ESCAPE) {
                // 通知窗口管理器退出
                // 注释掉，因为会导致编译错误
                // auto& windowManager = DearTs::Core::Window::WindowManager::getInstance();
                // windowManager.shutdown();
            }
            break;
    }
}

} // namespace DearTs::Examples