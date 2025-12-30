/**
 * @file base_settings_widget.hpp
 * @brief 设置组件基类
 * @details 提供统一的配置修改跟踪和管理接口
 */

#pragma once

#include <string>
#include <vector>
#include <algorithm>

namespace DearTs::Plugins::Settings {

/**
 * @brief 设置组件基类
 *
 * 提供所有设置组件的通用功能：
 * - 配置修改跟踪
 * - 修改键管理
 * - 统一的数据访问接口
 *
 * 使用方法：
 * @code
 * class MySettingsWidget : public BaseSettingsWidget {
 * public:
 *     void render() override {
 *         // 渲染 UI
 *         if (ImGui::Button("Apply")) {
 *             mark_modified("my.setting");
 *         }
 *     }
 * };
 * @endcode
 */
class BaseSettingsWidget {
public:
    BaseSettingsWidget() = default;
    virtual ~BaseSettingsWidget() = default;

    /**
     * @brief 渲染设置界面
     * @details 子类必须实现此方法来渲染其 UI
     */
    virtual void render() = 0;

    /**
     * @brief 获取已修改的配置键列表
     * @return 常量引用到已修改配置键向量
     */
    [[nodiscard]] const std::vector<std::string>& get_modified_keys() const {
        return m_modified_keys;
    }

    /**
     * @brief 清空已修改配置键列表
     * @details 通常在保存配置后调用
     */
    void clear_modified_keys() {
        m_modified_keys.clear();
    }

    /**
     * @brief 检查是否有配置被修改
     * @return 如果有配置被修改返回 true
     */
    [[nodiscard]] bool has_modifications() const {
        return !m_modified_keys.empty();
    }

    /**
     * @brief 获取已修改配置的数量
     * @return 已修改配置键的数量
     */
    [[nodiscard]] size_t get_modification_count() const {
        return m_modified_keys.size();
    }

protected:
    /**
     * @brief 标记配置键为已修改
     * @details 自动去重，如果键已存在则不会重复添加
     * @param key 配置键
     */
    void mark_modified(const std::string& key) {
        // 使用 std::find 检查是否已存在
        auto it = std::find(m_modified_keys.begin(), m_modified_keys.end(), key);
        if (it == m_modified_keys.end()) {
            m_modified_keys.push_back(key);
        }
    }

    /**
     * @brief 批量标记配置键为已修改
     * @param keys 配置键列表
     */
    void mark_modified_batch(const std::vector<std::string>& keys) {
        for (const auto& key : keys) {
            mark_modified(key);
        }
    }

    /**
     * @brief 检查特定配置键是否已被修改
     * @param key 配置键
     * @return 如果配置键已被修改返回 true
     */
    [[nodiscard]] bool is_key_modified(const std::string& key) const {
        return std::find(m_modified_keys.begin(), m_modified_keys.end(), key) != m_modified_keys.end();
    }

private:
    std::vector<std::string> m_modified_keys;  ///< 已修改的配置键列表
};

} // namespace DearTs::Plugins::Settings
