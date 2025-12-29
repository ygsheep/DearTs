/**
 * @file layout_manager.h
 * @brief 布局管理器
 * @details 参考 ImHex 实现窗口布局的保存、加载和管理
 * @author DearTs Team
 * @date 2024
 * @version 1.0.0
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <imgui.h>

namespace DearTs::Core::UI {

/**
 * @brief 布局管理器
 *
 * 管理窗口布局的保存、加载和恢复功能
 * 基于 ImGui 的 INI 设置持久化系统
 */
class LayoutManager {
public:
    /**
     * @brief 获取单例实例
     */
    static LayoutManager& instance() {
        static LayoutManager inst;
        return inst;
    }

    /**
     * @brief 保存当前布局到文件
     * @param name 布局名称
     * @param filepath 文件路径（为空则使用默认路径）
     * @return 成功返回 true
     */
    bool saveLayout(const std::string& name, const std::string& filepath = "");

    /**
     * @brief 从文件加载布局
     * @param filepath 文件路径
     * @return 成功返回 true
     */
    bool loadLayout(const std::string& filepath);

    /**
     * @brief 保存当前布局到字符串
     * @return 布局数据字符串
     */
    [[nodiscard]] std::string saveToString() const;

    /**
     * @brief 从字符串加载布局
     * @param content 布局数据字符串
     * @return 成功返回 true
     */
    bool loadFromString(const std::string& content);

    /**
     * @brief 重置为默认布局
     */
    void resetToDefault();

    /**
     * @brief 锁定布局（防止窗口移动和停靠）
     * @param locked 是否锁定
     */
    void lockLayout(bool locked);

    /**
     * @brief 检查布局是否锁定
     */
    [[nodiscard]] bool isLayoutLocked() const { return m_locked; }

    /**
     * @brief 关闭所有视图
     */
    void closeAllViews();

    /**
     * @brief 获取已保存的布局列表
     */
    [[nodiscard]] std::vector<std::string> getSavedLayouts() const;

    /**
     * @brief 删除布局
     * @param name 布局名称
     */
    bool deleteLayout(const std::string& name);

    /**
     * @brief 设置布局变化回调
     * @param callback 回调函数
     */
    void setLayoutChangedCallback(std::function<void()> callback) {
        m_layout_changed_callback = std::move(callback);
    }

    /**
     * @brief 在每一帧中调用（用于延迟布局操作）
     */
    void update();

    /**
     * @brief 标记布局需要保存
     */
    void markDirty() { m_dirty = true; }

    /**
     * @brief 是否需要保存
     */
    [[nodiscard]] bool isDirty() const { return m_dirty; }

private:
    LayoutManager() = default;
    ~LayoutManager() = default;

    // 禁止拷贝
    LayoutManager(const LayoutManager&) = delete;
    LayoutManager& operator=(const LayoutManager&) = delete;

    /**
     * @brief 获取默认布局文件路径
     */
    [[nodiscard]] std::string getDefaultLayoutPath() const;

    /**
     * @brief 应用布局到 ImGui
     */
    void applyLayout();

private:
    bool m_locked = false;
    bool m_dirty = false;
    std::string m_current_layout;
    std::function<void()> m_layout_changed_callback;
};

} // namespace DearTs::Core::UI
