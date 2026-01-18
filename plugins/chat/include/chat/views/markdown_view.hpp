/**
 * @file markdown_view.hpp
 * @brief Markdown 预览视图
 * @details 用于显示完整的 Markdown 内容，支持代码高亮和格式化
 */

#pragma once

#include "core/ui/view.h"
#include "core/ui/icon_font.hpp"
#include "core/content/registry_base.h"
#include <string>

namespace DearTs::Plugins::Chat {

using Core::ContentRegistry::UnlocalizedString;
using Core::UI::ViewWindow;

/**
 * @brief Markdown 预览视图
 * @details 继承自 ViewWindow，集成到 DearTs 视图管理系统中
 */
class MarkdownView : public ViewWindow {
public:
    /**
     * @brief 构造函数
     * @param content Markdown 内容
     * @param message_id 关联的消息 ID（用于生成唯一窗口 ID）
     */
    MarkdownView(const std::string& content, const std::string& message_id);

    /**
     * @brief 析构函数
     */
    ~MarkdownView() override = default;

    /**
     * @brief 获取视图名称
     * @note 返回格式：Markdown##<message_id>，确保每个消息有独立的窗口
     */
    std::string getName() const { return m_window_name; }

    /**
     * @brief 获取视图最小尺寸
     */
    ImVec2 get_min_size() const override { return ImVec2(400, 300); }

    /**
     * @brief 绘制内容
     */
    void draw_content() override;

    /**
     * @brief 获取关联的消息 ID
     */
    const std::string& get_message_id() const { return m_message_id; }

    /**
     * @brief 更新内容
     */
    void set_content(const std::string& content) { m_content = content; }

    /**
     * @brief 获取内容
     */
    const std::string& get_content() const { return m_content; }

    // 禁止拷贝和移动
    MarkdownView(const MarkdownView&) = delete;
    MarkdownView& operator=(const MarkdownView&) = delete;
    MarkdownView(MarkdownView&&) = delete;
    MarkdownView& operator=(MarkdownView&&) = delete;

private:
    std::string m_content;      // Markdown 内容
    std::string m_message_id;   // 关联消息 ID
    std::string m_window_name;  // 窗口名称（含唯一 ID）
};

} // namespace DearTs::Plugins::Chat
