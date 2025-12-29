/**
 * @file shortcut_manager.cpp
 * @brief 快捷键管理器实现
 */

#include "shortcut_manager.h"
#include "logger.h"
#include <algorithm>

namespace DearTs::Core::UI {

// ================ Shortcut Implementation ================

std::string Shortcut::toString() const {
    std::string result;

    if (ctrl) result += "Ctrl+";
    if (shift) result += "Shift+";
    if (alt) result += "Alt+";
    if (super_) result += "Super+";

    // 添加键名
    const char* keyName = nullptr;

    // 常用按键映射
    switch (key) {
        case ImGuiKey_Tab:        keyName = "Tab"; break;
        case ImGuiKey_LeftArrow:  keyName = "Left"; break;
        case ImGuiKey_RightArrow: keyName = "Right"; break;
        case ImGuiKey_UpArrow:    keyName = "Up"; break;
        case ImGuiKey_DownArrow:  keyName = "Down"; break;
        case ImGuiKey_PageUp:     keyName = "PageUp"; break;
        case ImGuiKey_PageDown:   keyName = "PageDown"; break;
        case ImGuiKey_Home:       keyName = "Home"; break;
        case ImGuiKey_End:        keyName = "End"; break;
        case ImGuiKey_Insert:     keyName = "Insert"; break;
        case ImGuiKey_Delete:     keyName = "Delete"; break;
        case ImGuiKey_Backspace:  keyName = "Backspace"; break;
        case ImGuiKey_Space:      keyName = "Space"; break;
        case ImGuiKey_Enter:      keyName = "Enter"; break;
        case ImGuiKey_Escape:     keyName = "Escape"; break;
        case ImGuiKey_F1:         keyName = "F1"; break;
        case ImGuiKey_F2:         keyName = "F2"; break;
        case ImGuiKey_F3:         keyName = "F3"; break;
        case ImGuiKey_F4:         keyName = "F4"; break;
        case ImGuiKey_F5:         keyName = "F5"; break;
        case ImGuiKey_F6:         keyName = "F6"; break;
        case ImGuiKey_F7:         keyName = "F7"; break;
        case ImGuiKey_F8:         keyName = "F8"; break;
        case ImGuiKey_F9:         keyName = "F9"; break;
        case ImGuiKey_F10:        keyName = "F10"; break;
        case ImGuiKey_F11:        keyName = "F11"; break;
        case ImGuiKey_F12:        keyName = "F12"; break;
        default:
            // 字母和数字
            if (key >= ImGuiKey_A && key <= ImGuiKey_Z) {
                char c = 'A' + (key - ImGuiKey_A);
                result += c;
                return result;
            } else if (key >= ImGuiKey_0 && key <= ImGuiKey_9) {
                char c = '0' + (key - ImGuiKey_0);
                result += c;
                return result;
            }
            keyName = "Unknown";
            break;
    }

    if (keyName) {
        result += keyName;
    }

    return result;
}

Shortcut Shortcut::fromString(const std::string& str) {
    Shortcut result;

    std::string remaining = str;

    // 解析修饰键
    while (true) {
        if (remaining.find("Ctrl+") == 0) {
            result.ctrl = true;
            remaining = remaining.substr(5);
        } else if (remaining.find("Shift+") == 0) {
            result.shift = true;
            remaining = remaining.substr(6);
        } else if (remaining.find("Alt+") == 0) {
            result.alt = true;
            remaining = remaining.substr(4);
        } else if (remaining.find("Super+") == 0) {
            result.super_ = true;
            remaining = remaining.substr(6);
        } else {
            break;
        }
    }

    // 解析主键
    if (!remaining.empty()) {
        char c = remaining[0];

        // 字母
        if (c >= 'A' && c <= 'Z') {
            result.key = ImGuiKey_A + (c - 'A');
        }
        // 小写字母
        else if (c >= 'a' && c <= 'z') {
            result.key = ImGuiKey_A + (c - 'a');
        }
        // 数字
        else if (c >= '0' && c <= '9') {
            result.key = ImGuiKey_0 + (c - '0');
        }
        // 命名按键
        else if (remaining == "Tab" || remaining == "tab") {
            result.key = ImGuiKey_Tab;
        } else if (remaining == "Enter" || remaining == "enter") {
            result.key = ImGuiKey_Enter;
        } else if (remaining == "Space" || remaining == "space") {
            result.key = ImGuiKey_Space;
        } else if (remaining == "Escape" || remaining == "escape") {
            result.key = ImGuiKey_Escape;
        } else if (remaining == "Backspace" || remaining == "backspace") {
            result.key = ImGuiKey_Backspace;
        } else if (remaining == "Delete" || remaining == "delete") {
            result.key = ImGuiKey_Delete;
        } else if (remaining == "Up" || remaining == "up") {
            result.key = ImGuiKey_UpArrow;
        } else if (remaining == "Down" || remaining == "down") {
            result.key = ImGuiKey_DownArrow;
        } else if (remaining == "Left" || remaining == "left") {
            result.key = ImGuiKey_LeftArrow;
        } else if (remaining == "Right" || remaining == "right") {
            result.key = ImGuiKey_RightArrow;
        }
        // F1-F12
        else if (remaining.size() == 2 && remaining[0] == 'F') {
            int f_num = remaining[1] - '0';
            if (f_num >= 1 && f_num <= 12) {
                result.key = ImGuiKey_F1 + (f_num - 1);
            }
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
    ImGuiIO& io = ImGui::GetIO();

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
    ImGuiIO& io = ImGui::GetIO();

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
    auto& manager = instance();
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
