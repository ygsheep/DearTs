/**
 * @file character_manager.cpp
 * @brief 角色管理器实现
 */

#include "character_manager.h"
#include "liblogger/logger.h"
#include <algorithm>

namespace DearTs::Core::UI {

bool CharacterManager::register_character(const CharacterInfo& info) {
    // 检查 ID 是否已存在
    for (const auto& ch : m_characters) {
        if (ch.id == info.id) {
            LOG_WARN("Character with id '{}' already registered", info.id);
            return false;
        }
    }

    // 验证角色信息
    if (info.type == CharacterType::Single && info.image_path.empty()) {
        LOG_ERROR("Single character must have image_path");
        return false;
    }

    if (info.type == CharacterType::Animated && info.frame_paths.empty()) {
        LOG_ERROR("Animated character must have frame_paths");
        return false;
    }

    m_characters.push_back(info);
    LOG_INFO("Registered character: {} ({})", info.name, info.id);

    // 如果是第一个角色，自动设为活动角色
    if (m_characters.size() == 1) {
        set_active_character(info.id);
    }

    return true;
}

void CharacterManager::unregister_character(const std::string& id) {
    auto it = std::remove_if(m_characters.begin(), m_characters.end(),
        [&id](const CharacterInfo& info) { return info.id == id; });

    if (it != m_characters.end()) {
        LOG_INFO("Unregistered character: {}", id);
        m_characters.erase(it, m_characters.end());

        // 如果删除的是活动角色，切换到第一个角色
        if (m_active_character && m_active_character->id == id) {
            if (!m_characters.empty()) {
                set_active_character(m_characters[0].id);
            } else {
                m_active_character = nullptr;
                notify_character_changed();
            }
        }
    }
}

const CharacterInfo* CharacterManager::get_character(const std::string& id) const {
    auto it = std::find_if(m_characters.begin(), m_characters.end(),
        [&id](const CharacterInfo& info) { return info.id == id; });

    if (it != m_characters.end()) {
        return &(*it);
    }
    return nullptr;
}

bool CharacterManager::set_active_character(const std::string& id) {
    const CharacterInfo* character = get_character(id);
    if (!character) {
        LOG_WARN("Character '{}' not found", id);
        return false;
    }

    if (!character->enabled) {
        LOG_WARN("Character '{}' is disabled", id);
        return false;
    }

    if (m_active_character != character) {
        m_active_character = character;
        LOG_INFO("Active character changed to: {} ({})", character->name, character->id);
        notify_character_changed();
    }

    return true;
}

std::string CharacterManager::get_active_character_id() const {
    if (m_active_character) {
        return m_active_character->id;
    }
    return "";
}

void CharacterManager::set_character_enabled(const std::string& id, bool enabled) {
    for (auto& ch : m_characters) {
        if (ch.id == id) {
            ch.enabled = enabled;
            LOG_INFO("Character '{}' {}", id, enabled ? "enabled" : "disabled");

            // 如果禁用的是当前活动角色，切换到其他角色
            if (!enabled && m_active_character && m_active_character->id == id) {
                // 查找第一个启用的角色
                for (const auto& c : m_characters) {
                    if (c.enabled) {
                        set_active_character(c.id);
                        break;
                    }
                }
            }

            return;
        }
    }
}

void CharacterManager::load_default_characters() {
    LOG_INFO("Loading default characters...");

    // 注册四个菲比角色（独立图片模式）
    std::vector<CharacterInfo> default_characters = {
        {
            .id = "feibi_0",
            .name = "菲比 - 造型 1",
            .image_path = "0-菲比.png",
            .frame_paths = {},
            .type = CharacterType::Single,
            .scale = 0.5f,
            .opacity = 1.0f
        },
        {
            .id = "feibi_1",
            .name = "菲比 - 造型 2",
            .image_path = "1-菲比.png",
            .frame_paths = {},
            .type = CharacterType::Single,
            .scale = 0.5f,
            .opacity = 1.0f
        },
        {
            .id = "feibi_2",
            .name = "菲比 - 造型 3",
            .image_path = "2-菲比.png",
            .frame_paths = {},
            .type = CharacterType::Single,
            .scale = 0.5f,
            .opacity = 1.0f
        },
        {
            .id = "feibi_3",
            .name = "菲比 - 造型 4",
            .image_path = "3-菲比.png",
            .frame_paths = {},
            .type = CharacterType::Single,
            .scale = 0.5f,
            .opacity = 1.0f
        },
        // 可选：注册一个动画模式的角色
        {
            .id = "feibi_animated",
            .name = "菲比 - 动画模式",
            .image_path = "",
            .frame_paths = {"0-菲比.png", "1-菲比.png", "2-菲比.png", "3-菲比.png"},
            .type = CharacterType::Animated,
            .scale = 0.5f,
            .opacity = 1.0f,
            .frame_interval = 1.0f
        }
    };

    for (const auto& info : default_characters) {
        register_character(info);
    }

    LOG_INFO("Loaded {} default characters", default_characters.size());
}

void CharacterManager::notify_character_changed() {
    for (auto& callback : m_character_changed_callbacks) {
        callback(m_active_character);
    }
}

} // namespace DearTs::Core::UI
