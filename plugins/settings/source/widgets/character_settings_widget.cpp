/**
 * @file character_settings_widget.cpp
 * @brief 角色选择设置组件实现
 */

#include "widgets/character_settings_widget.hpp"
#include "core/ui/character_manager.h"
#include "core/ui/character_renderer.h"
#include "core/config/config_manager.h"
#include "liblogger/logger.h"

#include <imgui.h>

namespace DearTs::Plugins::Settings {

void CharacterSettingsWidget::render() {
    if (!ImGui::CollapsingHeader("角色设置", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }

    ImGui::PushID("CharacterSettings");

    // 配置已在应用启动时加载，这里直接渲染 UI

    ImGui::Separator();

    // 角色选择
    render_character_selection();

    ImGui::Separator();

    // 角色位置
    render_position_selection();

    ImGui::Separator();

    // 缩放和透明度
    render_scale_and_opacity();

    ImGui::Separator();

    // 动画设置
    render_animation_settings();

    ImGui::PopID();
}

void CharacterSettingsWidget::render_character_selection() {
    auto& config = Core::Config::ConfigManager::instance();
    auto& character_manager = Core::UI::CharacterManager::instance();
    const auto& characters = character_manager.get_characters();

    if (characters.empty()) {
        ImGui::Text("没有可用的角色");
        return;
    }

    ImGui::Text("选择角色:");
    ImGui::SameLine();

    // 获取当前活动角色
    const Core::UI::CharacterInfo* current_character = character_manager.get_active_character();
    std::string current_preview = current_character ? current_character->name : "未选择";

    if (ImGui::BeginCombo("##CharacterSelect", current_preview.c_str())) {
        for (const auto& character : characters) {
            if (!character.enabled) {
                continue;  // 跳过禁用的角色
            }

            const bool is_selected = (current_character && current_character->id == character.id);

            // 显示角色名称
            std::string label = character.name;
            if (character.type == Core::UI::CharacterType::Animated) {
                label += " [动画]";
            }

            if (ImGui::Selectable(label.c_str(), is_selected)) {
                if (character_manager.set_active_character(character.id)) {
                    // 保存到 config
                    config.set("character.active_id", character.id);

                    // 切换角色
                    auto& character_renderer = Core::UI::CharacterRenderer::instance();

                    // 释放旧角色
                    character_renderer.clear();

                    // 加载新角色
                    if (character.type == Core::UI::CharacterType::Single) {
                        // 单张图片模式
                        if (std::vector<std::string> paths = {character.image_path};
                            character_renderer.load_character_frames(paths)) {
                            character_renderer.set_enabled(true);
                            character_renderer.set_position(Core::UI::CharacterPosition::BottomRight);
                            character_renderer.set_scale(character.scale);
                            character_renderer.set_opacity(character.opacity);
                            character_renderer.set_animation_mode(Core::UI::AnimationMode::None);
                            LOG_INFO("Switched to character: {} (Single)", character.name);
                        }
                    } else if (character.type == Core::UI::CharacterType::Animated) {
                        // 动画模式
                        if (character_renderer.load_character_frames(character.frame_paths)) {
                            character_renderer.set_enabled(true);
                            character_renderer.set_position(Core::UI::CharacterPosition::BottomRight);
                            character_renderer.set_scale(character.scale);
                            character_renderer.set_opacity(character.opacity);
                            character_renderer.set_animation_mode(Core::UI::AnimationMode::FrameLoop);
                            character_renderer.set_frame_interval(character.frame_interval);
                            LOG_INFO("Switched to character: {} (Animated)", character.name);
                        }
                    }

                    mark_modified("character.active_id");
                }
            }
        }

        ImGui::EndCombo();
    }

    // 显示角色统计信息
    ImGui::SameLine();
    ImGui::TextDisabled("(共 %zu 个角色)", characters.size());
}

void CharacterSettingsWidget::render_position_selection() {
    auto& config = Core::Config::ConfigManager::instance();
    auto& character_renderer = Core::UI::CharacterRenderer::instance();

    // 从 config 读取位置，默认为右下角 (0)
    int position_index = config.get_or<int>("character.position", 0);

    constexpr const char* const position_names[] = {"右下角", "左下角", "右上角", "左上角", "居中"};
    ImGui::Text("角色位置:");

    if (ImGui::Combo("##CharacterPosition", &position_index, position_names, 5)) {
        const auto new_position = static_cast<Core::UI::CharacterPosition>(position_index);
        character_renderer.set_position(new_position);
        config.set("character.position", position_index);
        mark_modified("character.position");
    }
}

void CharacterSettingsWidget::render_scale_and_opacity() {
    auto& config = Core::Config::ConfigManager::instance();
    auto& character_renderer = Core::UI::CharacterRenderer::instance();

    // 缩放比例 - 从 config 读取，默认 0.8
    float scale = static_cast<float>(config.get_or<double>("character.scale", 0.5));
    ImGui::Text("缩放比例: %.2f", scale);

    if (ImGui::SliderFloat("##CharacterScale", &scale, 0.1F, 2.0F, "%.2f")) {
        character_renderer.set_scale(scale);
        config.set("character.scale", static_cast<double>(scale));
        mark_modified("character.scale");
    }

    ImGui::SameLine();
    if (ImGui::Button("重置缩放##ResetScale")) {
        character_renderer.set_scale(0.5F);
        config.set("character.scale", 0.5);
        mark_modified("character.scale");
    }

    // 透明度 - 从 config 读取，默认 0.3
    float opacity = static_cast<float>(config.get_or<double>("character.opacity", .3F));
    ImGui::Text("透明度: %.2f", opacity);

    if (ImGui::SliderFloat("##CharacterOpacity", &opacity, 0.0f, 1.0f, "%.2f")) {
        character_renderer.set_opacity(opacity);
        config.set("character.opacity", static_cast<double>(opacity));
        mark_modified("character.opacity");
    }

    ImGui::SameLine();
    if (ImGui::Button("重置透明度##ResetOpacity")) {
        character_renderer.set_opacity(1.0F);
        config.set("character.opacity", 1.0);
        mark_modified("character.opacity");
    }
}

void CharacterSettingsWidget::render_animation_settings() {
    auto& config = Core::Config::ConfigManager::instance();
    auto& character_manager = Core::UI::CharacterManager::instance();
    auto& character_renderer = Core::UI::CharacterRenderer::instance();
    const Core::UI::CharacterInfo* current_character = character_manager.get_active_character();

    // 只有动画模式才显示动画设置
    if ((current_character == nullptr) || current_character->type != Core::UI::CharacterType::Animated) {
        ImGui::TextDisabled("当前角色不是动画模式");
        return;
    }

    // 动画模式 - 从 config 读取，默认为 FrameLoop (1)
    int current_mode_index = config.get_or<int>("character.animation_mode", 1);
    constexpr const char* const animation_modes[] = {"无动画", "循环播放", "往复播放", "随机切换"};

    ImGui::Text("动画模式:");
    if (ImGui::Combo("##AnimationMode", &current_mode_index, animation_modes, 4)) {
        Core::UI::AnimationMode new_mode = static_cast<Core::UI::AnimationMode>(current_mode_index);
        character_renderer.set_animation_mode(new_mode);
        config.set("character.animation_mode", current_mode_index);
        mark_modified("character.animation_mode");
    }

    // 帧切换间隔 - 从 config 读取，默认 0.5 秒
    float interval = static_cast<float>(config.get_or<double>("character.frame_interval", 0.5));
    ImGui::Text("帧间隔: %.1f 秒", interval);

    if (ImGui::SliderFloat("##FrameInterval", &interval, 0.1f, 5.0f, "%.1f")) {
        character_renderer.set_frame_interval(interval);
        config.set("character.frame_interval", static_cast<double>(interval));
        mark_modified("character.frame_interval");
    }

    // 显示当前帧信息
    const size_t current_frame = character_renderer.get_current_frame();
    const size_t total_frames = character_renderer.get_frame_count();
    ImGui::Text("当前帧: %zu / %zu", current_frame + 1, total_frames);
}

void CharacterSettingsWidget::mark_modified(const std::string& key) {
    // 检查是否已经存在
    for (const auto& k : m_modified_keys) {
        if (k == key) {
            return;  // 已存在
        }
    }
    m_modified_keys.push_back(key);
}

} // namespace DearTs::Plugins::Settings
