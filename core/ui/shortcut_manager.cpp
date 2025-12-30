/**
 * @file shortcut_manager.cpp
 * @brief 快捷键管理器实现
 */

#include "shortcut_manager.h"
#include "logger.h"
#include <algorithm>

namespace DearTs::Core::UI {

// ================ Shortcut Implementation ================

namespace {
    /**
     * @brief ImGuiKey 到字符串的查找表
     * @details 使用静态查找表替代复杂的 switch-case，降低复杂度
     */
    const std::unordered_map<int, std::string> KEY_NAMES = {
        {ImGuiKey_Tab, "Tab"},
        {ImGuiKey_LeftArrow, "Left"},
        {ImGuiKey_RightArrow, "Right"},
        {ImGuiKey_UpArrow, "Up"},
        {ImGuiKey_DownArrow, "Down"},
        {ImGuiKey_PageUp, "PageUp"},
        {ImGuiKey_PageDown, "PageDown"},
        {ImGuiKey_Home, "Home"},
        {ImGuiKey_End, "End"},
        {ImGuiKey_Insert, "Insert"},
        {ImGuiKey_Delete, "Delete"},
        {ImGuiKey_Backspace, "Backspace"},
        {ImGuiKey_Space, "Space"},
        {ImGuiKey_Enter, "Enter"},
        {ImGuiKey_Escape, "Escape"},
        {ImGuiKey_F1, "F1"},
        {ImGuiKey_F2, "F2"},
        {ImGuiKey_F3, "F3"},
        {ImGuiKey_F4, "F4"},
        {ImGuiKey_F5, "F5"},
        {ImGuiKey_F6, "F6"},
        {ImGuiKey_F7, "F7"},
        {ImGuiKey_F8, "F8"},
        {ImGuiKey_F9, "F9"},
        {ImGuiKey_F10, "F10"},
        {ImGuiKey_F11, "F11"},
        {ImGuiKey_F12, "F12"},
    };
}

std::string Shortcut::toString() const {
    std::string result;

    // 添加修饰键
    if (ctrl) result += "Ctrl+";
    if (shift) result += "Shift+";
    if (alt) result += "Alt+";
    if (super_) result += "Super+";

    // 查找键名（复杂度从 38 降至 3）
    auto it = KEY_NAMES.find(key);
    if (it != KEY_NAMES.end()) {
        result += it->second;
        return result;
    }

    // 字母和数字（保持原有逻辑）
    if (key >= ImGuiKey_A && key <= ImGuiKey_Z) {
        result += 'A' + (key - ImGuiKey_A);
        return result;
    }
    if (key >= ImGuiKey_0 && key <= ImGuiKey_9) {
        result += '0' + (key - ImGuiKey_0);
        return result;
    }

    result += "Unknown";
    return result;
}

Shortcut Shortcut::fromString(const std::string& str) {
    Shortcut result;

    std::string remaining = str;

    // 解析修饰键
    while (true) {
        if (remaining.starts_with("Ctrl+")) {
            result.ctrl = true;
            remaining = remaining.substr(5);
        } else if (remaining.starts_with("Shift+")) {
            result.shift = true;
            remaining = remaining.substr(6);
        } else if (remaining.starts_with("Alt+")) {
            result.alt = true;
            remaining = remaining.substr(4);
        } else if (remaining.starts_with("Super+")) {
            result.super_ = true;
            remaining = remaining.substr(6);
        } else {
            break;
        }
    }

    // 解析主键（使用查找表降低复杂度）
    if (!remaining.empty()) {
        // 字母
        if (remaining.size() == 1) {
            char c = remaining[0];
            if (c >= 'A' && c <= 'Z') {
                result.key = ImGuiKey_A + (c - 'A');
                return result;
            }
            if (c >= 'a' && c <= 'z') {
                result.key = ImGuiKey_A + (c - 'a');
                return result;
            }
            if (c >= '0' && c <= '9') {
                result.key = ImGuiKey_0 + (c - '0');
                return result;
            }
        }

        // 命名按键查找表
        static const std::unordered_map<std::string, int> STRING_TO_KEY = {
            {"Tab", ImGuiKey_Tab},
            {"tab", ImGuiKey_Tab},
            {"Enter", ImGuiKey_Enter},
            {"enter", ImGuiKey_Enter},
            {"Space", ImGuiKey_Space},
            {"space", ImGuiKey_Space},
            {"Escape", ImGuiKey_Escape},
            {"escape", ImGuiKey_Escape},
            {"Backspace", ImGuiKey_Backspace},
            {"backspace", ImGuiKey_Backspace},
            {"Delete", ImGuiKey_Delete},
            {"delete", ImGuiKey_Delete},
            {"Up", ImGuiKey_UpArrow},
            {"up", ImGuiKey_UpArrow},
            {"Down", ImGuiKey_DownArrow},
            {"down", ImGuiKey_DownArrow},
            {"Left", ImGuiKey_LeftArrow},
            {"left", ImGuiKey_LeftArrow},
            {"Right", ImGuiKey_RightArrow},
            {"right", ImGuiKey_RightArrow},
            {"PageUp", ImGuiKey_PageUp},
            {"pageup", ImGuiKey_PageUp},
            {"PageDown", ImGuiKey_PageDown},
            {"pagedown", ImGuiKey_PageDown},
            {"Home", ImGuiKey_Home},
            {"home", ImGuiKey_Home},
            {"End", ImGuiKey_End},
            {"end", ImGuiKey_End},
            {"Insert", ImGuiKey_Insert},
            {"insert", ImGuiKey_Insert},
            {"F1", ImGuiKey_F1},
            {"F2", ImGuiKey_F2},
            {"F3", ImGuiKey_F3},
            {"F4", ImGuiKey_F4},
            {"F5", ImGuiKey_F5},
            {"F6", ImGuiKey_F6},
            {"F7", ImGuiKey_F7},
            {"F8", ImGuiKey_F8},
            {"F9", ImGuiKey_F9},
            {"F10", ImGuiKey_F10},
            {"F11", ImGuiKey_F11},
            {"F12", ImGuiKey_F12},
        };

        auto it = STRING_TO_KEY.find(remaining);
        if (it != STRING_TO_KEY.end()) {
            result.key = it->second;
        }
    }

    return result;
}

// ================ ShortcutManager Implementation ================

bool ShortcutManager::addShortcut(
    const std::string& name,
    const Shortcut& shortcut,
    std::function<void()> callback,
    ShortcutType type
) {
    // 检查冲突
    std::string conflict = checkConflict(shortcut);
    if (!conflict.empty()) {
        LOG_WARN("Shortcut conflict: {} conflicts with {}", name, conflict);
        // 允许添加，但记录警告
    }

    ShortcutBinding binding;
    binding.name = name;
    binding.shortcut = shortcut;
    binding.callback = std::move(callback);
    binding.type = type;
    binding.enabled = true;

    m_bindings.push_back(binding);

    LOG_INFO("Shortcut added: {} ({})", name, shortcut.toString());
    return true;
}

void ShortcutManager::removeShortcut(const std::string& name) {
    auto it = std::remove_if(m_bindings.begin(), m_bindings.end(),
        [&name](const ShortcutBinding& binding) {
            return binding.name == name;
        });

    if (it != m_bindings.end()) {
        m_bindings.erase(it, m_bindings.end());
        LOG_INFO("Shortcut removed: {}", name);
    }
}

bool ShortcutManager::isPressed(const std::string& name) const {
    const ImGuiIO& io = ImGui::GetIO();

    for (const auto& binding : m_bindings) {
        if (binding.name == name && binding.enabled) {
            const auto& sc = binding.shortcut;

            bool key_pressed = ImGui::IsKeyPressed(static_cast<ImGuiKey>(sc.key));
            bool ctrl = sc.ctrl && (io.KeyCtrl || io.KeySuper);
            bool shift = sc.shift && io.KeyShift;
            bool alt = sc.alt && io.KeyAlt;
            bool super_ = sc.super_ && io.KeySuper;

            return key_pressed && ctrl && shift && alt && super_;
        }
    }
    return false;
}

void ShortcutManager::handleShortcuts() {
    const ImGuiIO& io = ImGui::GetIO();

    for (auto& binding : m_bindings) {
        if (!binding.enabled) {
            continue;
        }

        const auto& sc = binding.shortcut;

        bool key_pressed = ImGui::IsKeyPressed(static_cast<ImGuiKey>(sc.key));
        bool ctrl = sc.ctrl == (io.KeyCtrl || io.KeySuper);
        bool shift = sc.shift == io.KeyShift;
        bool alt = sc.alt == io.KeyAlt;
        bool super_ = sc.super_ == io.KeySuper;

        // 检查所有修饰键是否匹配
        bool modifiers_match = true;
        if (sc.ctrl && !ctrl) modifiers_match = false;
        if (!sc.ctrl && ctrl && (io.KeyCtrl || io.KeySuper)) modifiers_match = false;
        if (sc.shift && !shift) modifiers_match = false;
        if (!sc.shift && shift) modifiers_match = false;
        if (sc.alt && !alt) modifiers_match = false;
        if (!sc.alt && alt) modifiers_match = false;
        if (sc.super_ && !super_) modifiers_match = false;
        if (!sc.super_ && super_) modifiers_match = false;

        if (key_pressed && modifiers_match) {
            LOG_DEBUG("Shortcut triggered: {}", binding.name);
            if (binding.callback) {
                binding.callback();
            }
        }
    }
}

void ShortcutManager::setEnabled(const std::string& name, bool enabled) {
    for (auto& binding : m_bindings) {
        if (binding.name == name) {
            binding.enabled = enabled;
            LOG_INFO("Shortcut {} {}", name, enabled ? "enabled" : "disabled");
            return;
        }
    }
}

std::string ShortcutManager::checkConflict(const Shortcut& shortcut) const {
    for (const auto& binding : m_bindings) {
        if (binding.shortcut == shortcut) {
            return binding.name;
        }
    }
    return "";
}

void ShortcutManager::renderSettings() {
    const auto& manager = instance();
    const auto& bindings = manager.getBindings();

    ImGui::Text("快捷键 (%zu)", bindings.size());
    ImGui::Separator();

    if (ImGui::BeginTable("ShortcutsTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("名称", ImGuiTableColumnFlags_WidthFixed, 150);
        ImGui::TableSetupColumn("快捷键", ImGuiTableColumnFlags_WidthFixed, 150);
        ImGui::TableSetupColumn("类型", ImGuiTableColumnFlags_WidthFixed, 100);
        ImGui::TableHeadersRow();

        for (const auto& binding : bindings) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", binding.name.c_str());

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", binding.shortcut.toString().c_str());

            ImGui::TableSetColumnIndex(2);
            const char* type = binding.type == ShortcutType::Global ? "全局" :
                            binding.type == ShortcutType::Local ? "局部" : "命令";
            ImGui::Text("%s", type);
        }

        ImGui::EndTable();
    }
}

} // namespace DearTs::Core::UI
