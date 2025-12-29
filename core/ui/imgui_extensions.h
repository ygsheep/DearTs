/**
 * @file imgui_extensions.h
 * @brief ImGui 扩展工具函数
 * @details 参考 ImHex 的 ImGui 扩展
 */

#pragma once

#include <imgui.h>
#include <string>

namespace DearTs::Core::UI {

/**
 * @brief ImGui 扩展工具命名空间
 */
namespace ImGuiExt {

/**
 * @brief 标题栏按钮（类似 ImHex 的 TitleBarButton）
 * @param label 按钮标签/图标
 * @param size 按钮大小
 * @return 是否被点击
 */
bool TitleBarButton(const char* label, const ImVec2& size);

/**
 * @brief 带颜色的按钮
 * @param label 按钮标签
 * @param color 按钮颜色
 * @param size 按钮大小（可选）
 */
bool ButtonColored(const char* label, const ImVec4& color, const ImVec2& size = ImVec2(0, 0));

/**
 * @brief 带颜色的按钮（背景和文本）
 * @param label 按钮标签
 * @param bg_color 背景色
 * @param text_color 文本色
 */
bool ButtonColoredEx(const char* label, const ImVec4& bg_color, const ImVec4& text_color);

/**
 * @brief 图标按钮
 * @param icon_id 图标 ID（FontAwesome 等图标字体）
 * @param size 按钮大小
 * @param tooltip 工具提示（可选）
 */
bool IconButton(const char* icon_id, const ImVec2& size, const char* tooltip = nullptr);

/**
 * @brief HelpMarker - 显示问号帮助图标
 * @param desc 帮助文本
 */
void HelpMarker(const char* desc);

/**
 * @brief BulletText - 带项目符号的文本
 * @param text 文本内容
 */
void BulletText(const char* text);

/**
 * @brief Spacing - 添加水平间距
 * @param width 间距宽度
 */
void Spacing(float width);

} // namespace ImGuiExt

} // namespace DearTs::Core::UI
