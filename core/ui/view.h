#pragma once

#include "../content/registry_base.h"
#include <imgui.h>
#include <string>
#include <memory>
#include <map>

namespace DearTs {
namespace Core {
namespace UI {

// 引入 UnlocalizedString 类型以便在 UI 命名空间中使用
using ContentRegistry::UnlocalizedString;

/**
 * @brief View 基类
 *
 * 所有自定义视图都应该继承此类
 */
class View {
public:
    explicit View(UnlocalizedString unlocalized_name, const char* icon = nullptr);
    virtual ~View() = default;

    /**
     * @brief 绘制视图
     * @param extra_flags 额外的窗口标志
     */
    virtual void draw(ImGuiWindowFlags extra_flags = ImGuiWindowFlags_None) = 0;

    /**
     * @brief 绘制视图内容
     */
    virtual void draw_content() = 0;

    /**
     * @brief 绘制帮助文本（可选）
     */
    virtual void draw_help_text() {}

    /**
     * @brief 是否应该绘制此视图
     */
    [[nodiscard]] virtual bool should_draw() const;

    /**
     * @brief 是否应该处理此视图
     */
    [[nodiscard]] virtual bool should_process() const;

    /**
     * @brief 是否在视图菜单中显示
     */
    [[nodiscard]] virtual bool has_view_menu_entry() const;

    /**
     * @brief 获取最小窗口大小
     */
    [[nodiscard]] virtual ImVec2 get_min_size() const;

    /**
     * @brief 获取最大窗口大小
     */
    [[nodiscard]] virtual ImVec2 get_max_size() const;

    /**
     * @brief 获取额外的窗口标志
     */
    [[nodiscard]] virtual ImGuiWindowFlags get_window_flags() const;

    /**
     * @brief 是否应该保存窗口状态
     */
    [[nodiscard]] virtual bool should_store_window_state() const {
        return true;
    }

    /**
     * @brief 获取图标
     */
    [[nodiscard]] const char* get_icon() const {
        return m_icon;
    }

    /**
     * @brief 获取非本地化名称
     */
    [[nodiscard]] const UnlocalizedString& get_unlocalized_name() const {
        return m_unlocalized_name;
    }

    /**
     * @brief 获取本地化名称
     */
    [[nodiscard]] std::string get_name() const;

    /**
     * @brief 获取窗口打开状态
     */
    [[nodiscard]] bool& get_window_open_state();

    /**
     * @brief 获取窗口打开状态（const）
     */
    [[nodiscard]] const bool& get_window_open_state() const;

    /**
     * @brief 是否聚焦
     */
    [[nodiscard]] bool is_focused() const {
        return m_focused;
    }

    /**
     * @brief 窗口是否刚刚打开
     */
    [[nodiscard]] bool did_window_just_open();

    /**
     * @brief 窗口是否刚刚关闭
     */
    [[nodiscard]] bool did_window_just_close();

    /**
     * @brief 将窗口置于最前
     */
    void bring_to_front();

    /**
     * @brief 跟踪窗口状态
     */
    void track_window_state();

    /**
     * @brief 设置聚焦状态
     */
    void set_focused(bool focused);

    /**
     * @brief 设置初始窗口位置
     */
    void set_initial_position(const ImVec2& pos) {
        m_initial_position = pos;
        m_initial_position_set = true;
    }

    /**
     * @brief 转换为窗口名称
     */
    [[nodiscard]] static std::string to_window_name(const UnlocalizedString& unlocalized_name);

protected:
    /**
     * @brief 视图打开时调用
     */
    virtual void on_open() {}

    /**
     * @brief 视图关闭时调用
     */
    virtual void on_close() {}

protected:
    UnlocalizedString m_unlocalized_name;
    bool m_window_open = false;
    bool m_prev_window_open = false;
    const char* m_icon = nullptr;
    bool m_focused = false;
    bool m_window_just_opened = false;
    bool m_window_just_closed = false;
    bool m_initial_position_set = false;  // 是否已设置初始位置
    ImVec2 m_initial_position = ImVec2(0, 0);  // 初始位置
};

/**
 * @brief 普通窗口视图
 *
 * 标准的停靠窗口视图
 */
class ViewWindow : public View {
public:
    explicit ViewWindow(UnlocalizedString unlocalized_name, const char* icon = nullptr);

    /**
     * @brief 绘制视图
     */
    void draw(ImGuiWindowFlags extra_flags = ImGuiWindowFlags_None) override;
};

/**
 * @brief 特殊视图
 *
 * 不处理窗口创建，只绘制内容
 */
class ViewSpecial : public View {
public:
    explicit ViewSpecial(UnlocalizedString unlocalized_name);

    /**
     * @brief 绘制视图
     */
    void draw(ImGuiWindowFlags extra_flags = ImGuiWindowFlags_None) override;
};

/**
 * @brief 浮动窗口视图
 *
 * 无法停靠的浮动窗口
 */
class ViewFloating : public ViewWindow {
public:
    explicit ViewFloating(UnlocalizedString unlocalized_name, const char* icon = nullptr);

    /**
     * @brief 绘制视图
     */
    void draw(ImGuiWindowFlags extra_flags = ImGuiWindowFlags_None) override;

    /**
     * @brief 不保存窗口状态
     */
    [[nodiscard]] bool should_store_window_state() const override {
        return false;
    }
};

/**
 * @brief 模态窗口视图
 *
 * 始终置顶且阻止其他窗口输入
 */
class ViewModal : public View {
public:
    explicit ViewModal(UnlocalizedString unlocalized_name, const char* icon = nullptr);

    /**
     * @brief 绘制视图
     */
    void draw(ImGuiWindowFlags extra_flags = ImGuiWindowFlags_None) override;

    /**
     * @brief 是否有关闭按钮
     */
    [[nodiscard]] virtual bool has_close_button() const {
        return true;
    }

    /**
     * @brief 不保存窗口状态
     */
    [[nodiscard]] bool should_store_window_state() const override {
        return false;
    }
};

} // namespace UI

// ================ Views Registry ================

namespace ContentRegistry {
namespace Views {

// 前向声明 impl 命名空间
namespace impl {
    void add(std::unique_ptr<UI::View>&& view);
}

/**
 * @brief 添加视图
 * @tparam T 视图类型（必须继承自View）
 * @tparam Args 构造函数参数类型
 * @param args 构造函数参数
 */
template<std::derived_from<UI::View> T, typename... Args>
void add(Args&&... args) {
    impl::add(std::make_unique<T>(std::forward<Args>(args)...));
}

/**
 * @brief 移除视图
 * @param unlocalized_name 视图的非本地化名称
 * @return 成功返回 true，失败返回 false
 */
bool remove(const UnlocalizedString& unlocalized_name);

/**
 * @brief 通过名称获取视图
 * @param unlocalized_name 视图的非本地化名称
 * @return 视图指针，如果不存在则返回nullptr
 */
UI::View* get_by_name(const UnlocalizedString& unlocalized_name);

/**
 * @brief 获取当前聚焦的视图
 * @return 视图指针，如果没有聚焦的视图则返回nullptr
 */
UI::View* get_focused();

/**
 * @brief 获取所有视图
 */
const std::map<UnlocalizedString, std::unique_ptr<UI::View>>& get_all();

} // namespace Views
} // namespace ContentRegistry

} // namespace Core
} // namespace DearTs
