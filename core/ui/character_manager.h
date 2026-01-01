/**
 * @file character_manager.h
 * @brief 角色管理器
 * @details 管理多个角色，支持独立图片和动画帧两种模式
 * @author DearTs Team
 * @date 2025
 * @version 1.0.0
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace DearTs::Core::UI {

/**
 * @brief 角色类型
 */
enum class CharacterType {
    Single,      ///< 单张图片（静态角色）
    Animated     ///< 多帧动画
};

/**
 * @brief 角色信息
 */
struct CharacterInfo {
    std::string id;              ///< 角色唯一标识
    std::string name;            ///< 角色名称
    std::string image_path;      ///< 图片路径（Single 模式）
    std::vector<std::string> frame_paths;  ///< 动画帧路径（Animated 模式）
    CharacterType type;          ///< 角色类型
    float scale = 0.5F;          ///< 缩放比例
    float opacity = 1.0F;        ///< 透明度
    float frame_interval = 1.0F; ///< 帧间隔（Animated 模式）
    bool enabled = true;         ///< 是否启用
};

/**
 * @brief 角色管理器
 *
 * 提供角色管理功能：
 * - 支持多个角色注册
 * - 支持独立图片和动画帧两种模式
 * - 角色切换和选择
 * - 角色配置管理
 */
class CharacterManager {
public:
    /**
     * @brief 获取单例实例
     */
    static CharacterManager& instance() {
        static CharacterManager inst;
        return inst;
    }

    /**
     * @brief 注册角色
     * @param info 角色信息
     * @return 成功返回 true
     */
    bool register_character(const CharacterInfo& info);

    /**
     * @brief 取消注册角色
     * @param id 角色 ID
     */
    void unregister_character(const std::string& id);

    /**
     * @brief 获取所有角色
     */
    [[nodiscard]] const std::vector<CharacterInfo>& get_characters() const {
        return m_characters;
    }

    /**
     * @brief 获取角色数量
     */
    [[nodiscard]] size_t get_character_count() const {
        return m_characters.size();
    }

    /**
     * @brief 根据 ID 获取角色
     * @param id 角色 ID
     * @return 角色指针，未找到返回 nullptr
     */
    [[nodiscard]] const CharacterInfo* get_character(const std::string& id) const;

    /**
     * @brief 设置当前活动角色
     * @param id 角色 ID
     * @return 成功返回 true
     */
    bool set_active_character(const std::string& id);

    /**
     * @brief 获取当前活动角色
     */
    [[nodiscard]] const CharacterInfo* get_active_character() const {
        if (m_active_character_id.empty()) {
            return nullptr;
        }
        for (const auto& ch : m_characters) {
            if (ch.id == m_active_character_id) {
                return &ch;
            }
        }
        return nullptr;
    }

    /**
     * @brief 获取当前活动角色 ID
     */
    [[nodiscard]] std::string get_active_character_id() const;

    /**
     * @brief 启用/禁用角色
     * @param id 角色 ID
     * @param enabled 是否启用
     */
    void set_character_enabled(const std::string& id, bool enabled);

    /**
     * @brief 注册角色变更回调
     * @param callback 回调函数
     */
    void on_character_changed(std::function<void(const CharacterInfo*)> callback) {
        m_character_changed_callbacks.push_back(std::move(callback));
    }

    /**
     * @brief 加载默认角色（菲比系列）
     */
    void load_default_characters();

private:
    CharacterManager() = default;
    ~CharacterManager() = default;

    // 禁止拷贝
    CharacterManager(const CharacterManager&) = delete;
    CharacterManager& operator=(const CharacterManager&) = delete;

    /**
     * @brief 触发角色变更回调
     */
    void notify_character_changed() const;

private:
    std::vector<CharacterInfo> m_characters;                     ///< 角色列表
    std::string m_active_character_id;                           ///< 当前活动角色 ID
    std::vector<std::function<void(const CharacterInfo*)>> m_character_changed_callbacks;  ///< 角色变更回调
};

} // namespace DearTs::Core::UI
