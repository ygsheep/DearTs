/**
 * @file shortcut_manager.h
 * @brief 快捷键管理器
 * @details 管理全局和局部快捷键绑定
 * @author DearTs Team
 * @date 2025
 * @version 1.0.0
 */

#pragma once

#include <imgui.h>
#include <string>
#include <functional>
#include <unordered_map>
#include <vector>

namespace DearTs::Core::UI {

/**
 * @brief 快捷键结构
 */
struct Shortcut {
    int key;           ///< ImGuiKey 键码
    bool ctrl;         ///< Ctrl 键
    bool shift;        ///< Shift 键
    bool alt;          ///< Alt 键
    bool super_;       ///< Super/Windows 键

    /**
     * @brief 默认构造
     */
    Shortcut() : key(0), ctrl(false), shift(false), alt(false), super_(false) {}

    /**
     * @brief 构造函数
     */
    Shortcut(int k, bool c = false, bool s = false, bool a = false, bool sup = false)
        : key(k), ctrl(c), shift(s), alt(a), super_(sup) {}

    /**
     * @brief 比较运算符
     */
    bool operator==(const Shortcut& other) const {
        return key == other.key &&
               ctrl == other.ctrl &&
               shift == other.shift &&
               alt == other.alt &&
               super_ == other.super_;
    }

    /**
     * @brief 转换为字符串
     */
    [[nodiscard]] std::string toString() const;

    /**
     * @brief 从字符串解析
     */
    static Shortcut fromString(const std::string& str);
};

/**
 * @brief 快捷键类型
 */
enum class ShortcutType {
    Global,     ///< 全局快捷键（整个应用）
    Local,      ///< 局部快捷键（特定窗口）
    Command     ///< 命令快捷键
};

/**
 * @brief 快捷键绑定
 */
struct ShortcutBinding {
    std::string name;           ///< 绑定名称
    std::string description;    ///< 描述
    Shortcut shortcut;          ///< 快捷键
    ShortcutType type;          ///< 类型
    std::function<void()> callback; ///< 回调函数
    bool enabled;               ///< 是否启用
};

/**
 * @brief 快捷键管理器
 *
 * 管理应用程序的快捷键绑定和分发
 */
class ShortcutManager final {  // 单例类，禁止继承
public:
    /**
     * @brief 获取单例实例（线程安全，Magic Statics）
     */
    static ShortcutManager& instance() noexcept {
        static ShortcutManager inst;
        return inst;
    }

    // 删除所有拷贝和移动操作
    ShortcutManager(const ShortcutManager&) = delete;
    ShortcutManager& operator=(const ShortcutManager&) = delete;
    ShortcutManager(ShortcutManager&&) = delete;
    ShortcutManager& operator=(ShortcutManager&&) = delete;

    /**
     * @brief 添加快捷键
     * @param name 快捷键名称
     * @param shortcut 快捷键
     * @param callback 回调函数
     * @param type 类型
     * @return 成功返回 true
     */
    bool addShortcut(
        const std::string& name,
        const Shortcut& shortcut,
        std::function<void()> callback,
        ShortcutType type = ShortcutType::Global
    );

    /**
     * @brief 移除快捷键
     */
    void removeShortcut(const std::string& name);

    /**
     * @brief 检查快捷键是否被按下
     * @param name 快捷键名称
     * @return 是否被按下
     */
    [[nodiscard]] bool isPressed(const std::string& name) const;

    /**
     * @brief 处理快捷键输入
     * @details 在 ImGui 输入处理中调用
     */
    void handleShortcuts();

    /**
     * @brief 获取所有快捷键绑定
     */
    [[nodiscard]] const std::vector<ShortcutBinding>& getBindings() const {
        return m_bindings;
    }

    /**
     * @brief 启用/禁用快捷键
     */
    void setEnabled(const std::string& name, bool enabled);

    /**
     * @brief 检查快捷键冲突
     * @return 冲突的绑定名称，如果没有冲突返回空字符串
     */
    [[nodiscard]] std::string checkConflict(const Shortcut& shortcut) const;

    /**
     * @brief 渲染快捷键设置界面
     */
    static void renderSettings();

private:
    ShortcutManager() = default;
    ~ShortcutManager() = default;

private:
    std::vector<ShortcutBinding> m_bindings;
};

} // namespace DearTs::Core::UI
