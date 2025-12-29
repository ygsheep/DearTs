/**
 * @file title_bar.h
 * @brief 自定义窗口标题栏组件
 * @details 参考 ImHex 设计，提供跨平台的自定义标题栏
 * @author DearTs Team
 * @date 2024
 * @version 1.0.0
 */

#pragma once

#include <string>
#include <functional>
#include <imgui.h>

// Forward declarations
struct SDL_Window;

namespace DearTs::Core::UI {

/**
 * @brief 标题栏按钮类型
 */
enum class TitleBarButton {
    Minimize,
    Maximize,
    Close,
    Custom
};

/**
 * @brief 自定义标题栏类
 *
 * 提供类似 ImHex 的自定义标题栏功能：
 * - 无边框窗口模式
 * - 可拖拽标题栏
 * - 自定义按钮
 * - Logo 和搜索功能
 */
class TitleBar {
public:
    /**
     * @brief 标题栏按钮回调类型
     */
    using ButtonCallback = std::function<void()>;

    /**
     * @brief 构造函数
     */
    TitleBar();

    /**
     * @brief 渲染标题栏
     * @param window_title 窗口标题
     * @param window_width 窗口宽度
     * @return 是否请求关闭窗口
     */
    [[nodiscard]] bool render(const std::string& window_title, float window_width);

    /**
     * @brief 设置是否无边框模式
     */
    void set_borderless(bool enabled) { m_borderless = enabled; }

    /**
     * @brief 获取是否无边框模式
     */
    [[nodiscard]] bool is_borderless() const { return m_borderless; }

    /**
     * @brief 添加自定义按钮
     * @param icon 图标（使用 UTF-8 字符或 FontAwesome）
     * @param tooltip 工具提示
     * @param callback 点击回调
     * @param color 按钮颜色（可选）
     */
    void add_button(const char* icon, const char* tooltip, ButtonCallback callback,
                   const ImVec4& color = ImVec4(0, 0, 0, 0));

    /**
     * @brief 清空所有自定义按钮
     */
    void clear_buttons();

    /**
     * @brief 处理鼠标拖拽
     * @return 是否正在拖拽
     */
    [[nodiscard]] bool is_dragging() const { return m_is_dragging; }

private:
    /**
     * @brief 渲染标题栏按钮
     */
    bool render_button(const char* label, const ImVec2& size, const ImVec4& color = ImVec4(0, 0, 0, 0));

    /**
     * @brief 渲染系统控制按钮（最小化、最大化、关闭）
     */
    bool render_system_buttons(float window_width, float title_bar_height);

    /**
     * @brief 处理标题栏拖拽
     */
    void handle_dragging(const ImVec2& mouse_pos, float title_bar_height, const ImVec2& window_pos, float window_width);

    /**
     * @brief 获取标题栏高度
     */
    [[nodiscard]] float get_title_bar_height() const;

private:
    bool m_borderless = false;
    bool m_is_dragging = false;
    ImVec2 m_drag_start_pos;
    ImVec2 m_window_start_pos;
    ImVec2 m_drag_offset;  // 鼠标相对窗口的偏移量

    struct CustomButton {
        const char* icon;
        const char* tooltip;
        ButtonCallback callback;
        ImVec4 color;
    };

    std::vector<CustomButton> m_custom_buttons;
};

/**
 * @brief 平台特定的窗口控制函数
 */
namespace WindowControls {

/**
 * @brief 设置当前窗口指针（需要在 Application 中调用）
 */
void set_current_window(SDL_Window* window);

/**
 * @brief 最小化窗口
 */
void minimize_window();

/**
 * @brief 最大化/还原窗口
 */
void maximize_window();

/**
 * @brief 关闭窗口
 */
void close_window();

/**
 * @brief 设置窗口位置
 */
void set_window_position(int x, int y);

/**
 * @brief 获取窗口位置
 */
[[nodiscard]] ImVec2 get_window_position();

} // namespace WindowControls

} // namespace DearTs::Core::UI
