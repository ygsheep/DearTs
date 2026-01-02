/**
 * @file live2d_settings_widget.cpp
 * @brief Live2D 设置界面组件实现
 */

#include "widgets/live2d_settings_widget.hpp"
#include "core/config/config_manager.h"
#include "liblogger/logger.h"
#include <imgui.h>
#include <cstring>

namespace DearTs::Plugins::Live2D {

Live2DSettingsWidget::Live2DSettingsWidget(Live2DPlugin* plugin)
    : m_plugin(plugin)
{
}

void Live2DSettingsWidget::render() {
    if (!ImGui::CollapsingHeader("Live2D 设置", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }

    ImGui::PushID("Live2DSettings");

    // 模型设置
    render_model_settings();

    ImGui::Separator();

    // 渲染设置
    render_rendering_settings();

    ImGui::Separator();

    // 动画设置
    render_animation_settings();

    ImGui::Separator();

    // 交互设置
    render_interaction_settings();

    ImGui::Separator();

    // 调试设置
    render_debug_settings();

    ImGui::PopID();
}

void Live2DSettingsWidget::render_model_settings() {
    auto& config = m_plugin->get_config();
    auto& config_manager = DearTs::Core::Config::ConfigManager::instance();

    ImGui::Text("=== 模型设置 ===");

    // 模型目录
    ImGui::Text("模型目录:");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", config.models_directory.c_str());

    // 活动模型
    ImGui::Text("活动模型:");
    ImGui::SameLine();
    auto model_names = m_plugin->get_model_names();
    const auto* active_model = m_plugin->get_active_model();

    std::string current_model = active_model ? active_model->get_info().model_name : "未加载";

    if (ImGui::BeginCombo("##ActiveModel", current_model.c_str())) {
        for (const auto& name : model_names) {
            const bool is_selected = (active_model && active_model->get_info().model_name == name);
            if (ImGui::Selectable(name.c_str(), is_selected)) {
                m_plugin->set_active_model(name);
                mark_modified("live2d.active_model");
            }
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    // 模型缩放
    ImGui::Text("模型缩放:");
    ImGui::SameLine();
    float scale = config.model_scale;
    if (ImGui::SliderFloat("##ModelScale", &scale, 0.1f, 3.0f, "%.2f")) {
        config.model_scale = scale;
        config_manager.set("live2d.model_scale", static_cast<double>(scale));
        mark_modified("live2d.model_scale");
    }

    // 模型位置
    ImGui::Text("模型位置:");
    ImGui::SameLine();
    int position_index = static_cast<int>(config.model_position);
    constexpr const char* position_names[] = {
        "左上角", "顶部居中", "右上角",
        "左侧居中", "居中", "右侧居中",
        "左下角", "底部居中", "右下角"
    };

    if (ImGui::Combo("##ModelPosition", &position_index, position_names, 9)) {
        config.model_position = static_cast<Live2DModelPosition>(position_index);
        mark_modified("live2d.model_position");
    }

    // X 偏移
    ImGui::Text("X 偏移:");
    ImGui::SameLine();
    int x_offset = config.model_x_offset;
    if (ImGui::SliderInt("##ModelXOffset", &x_offset, -500, 500)) {
        config.model_x_offset = x_offset;
        mark_modified("live2d.model_x_offset");
    }

    // Y 偏移
    ImGui::Text("Y 偏移:");
    ImGui::SameLine();
    int y_offset = config.model_y_offset;
    if (ImGui::SliderInt("##ModelYOffset", &y_offset, -500, 500)) {
        config.model_y_offset = y_offset;
        mark_modified("live2d.model_y_offset");
    }

    // 模型透明度
    ImGui::Text("模型透明度:");
    ImGui::SameLine();
    float opacity = config.model_opacity;
    if (ImGui::SliderFloat("##ModelOpacity", &opacity, 0.0f, 1.0f, "%.2f")) {
        config.model_opacity = opacity;
        config_manager.set("live2d.model_opacity", static_cast<double>(opacity));
        mark_modified("live2d.model_opacity");
    }
}

void Live2DSettingsWidget::render_rendering_settings() {
    auto& config = m_plugin->get_config();
    auto& config_manager = DearTs::Core::Config::ConfigManager::instance();

    ImGui::Text("=== 渲染设置 ===");

    // 使用 FBO
    ImGui::Text("使用帧缓冲 (FBO):");
    ImGui::SameLine();
    bool use_fbo = config.use_fbo;
    if (ImGui::Checkbox("##UseFBO", &use_fbo)) {
        config.use_fbo = use_fbo;
        mark_modified("live2d.use_fbo");
    }

    // FBO 降采样
    if (config.use_fbo) {
        ImGui::Text("FBO 降采样:");
        ImGui::SameLine();
        int downsample = config.fbo_downsample;
        if (ImGui::Combo("##FBODownsample", &downsample, "1x\0" "2x\0" "4x\0")) {
            config.fbo_downsample = downsample;
            mark_modified("live2d.fbo_downsample");
        }
    }

    // 性能分析
    ImGui::Text("启用性能分析:");
    ImGui::SameLine();
    bool profiling = config.enable_profiling;
    if (ImGui::Checkbox("##EnableProfiling", &profiling)) {
        config.enable_profiling = profiling;
        mark_modified("live2d.enable_profiling");
    }
}

void Live2DSettingsWidget::render_animation_settings() {
    auto& config = m_plugin->get_config();
    auto& config_manager = DearTs::Core::Config::ConfigManager::instance();

    ImGui::Text("=== 动画设置 ===");

    // 自动播放待机动作
    ImGui::Text("自动播放待机动作:");
    ImGui::SameLine();
    bool auto_play = config.auto_play_idle_motion;
    if (ImGui::Checkbox("##AutoPlayIdleMotion", &auto_play)) {
        config.auto_play_idle_motion = auto_play;
        mark_modified("live2d.auto_play_idle_motion");
    }

    // 动作速度
    ImGui::Text("动作速度:");
    ImGui::SameLine();
    float motion_speed = config.motion_speed;
    if (ImGui::SliderFloat("##MotionSpeed", &motion_speed, 0.1f, 3.0f, "%.2f")) {
        config.motion_speed = motion_speed;
        config_manager.set("live2d.motion_speed", static_cast<double>(motion_speed));
        mark_modified("live2d.motion_speed");
    }

    // 启用呼吸效果
    ImGui::Text("启用呼吸效果:");
    ImGui::SameLine();
    bool breath = config.enable_breath;
    if (ImGui::Checkbox("##EnableBreath", &breath)) {
        config.enable_breath = breath;
        mark_modified("live2d.enable_breath");
    }

    // 启用眨眼
    ImGui::Text("启用眨眼:");
    ImGui::SameLine();
    bool eye_blink = config.enable_eye_blink;
    if (ImGui::Checkbox("##EnableEyeBlink", &eye_blink)) {
        config.enable_eye_blink = eye_blink;
        mark_modified("live2d.enable_eye_blink");
    }

    // 启用物理模拟
    ImGui::Text("启用物理模拟:");
    ImGui::SameLine();
    bool physics = config.enable_physics;
    if (ImGui::Checkbox("##EnablePhysics", &physics)) {
        config.enable_physics = physics;
        mark_modified("live2d.enable_physics");
    }
}

void Live2DSettingsWidget::render_interaction_settings() {
    auto& config = m_plugin->get_config();

    ImGui::Text("=== 交互设置 ===");

    // 启用鼠标跟随
    ImGui::Text("启用鼠标跟随:");
    ImGui::SameLine();
    bool mouse_follow = config.enable_mouse_follow;
    if (ImGui::Checkbox("##EnableMouseFollow", &mouse_follow)) {
        config.enable_mouse_follow = mouse_follow;
        mark_modified("live2d.enable_mouse_follow");
    }

    // 鼠标跟随强度
    if (config.enable_mouse_follow) {
        ImGui::Text("鼠标跟随强度:");
        ImGui::SameLine();
        float intensity = config.mouse_follow_intensity;
        if (ImGui::SliderFloat("##MouseFollowIntensity", &intensity, 0.0f, 1.0f, "%.2f")) {
            config.mouse_follow_intensity = intensity;
            mark_modified("live2d.mouse_follow_intensity");
        }
    }

    // 启用拖拽
    ImGui::Text("启用拖拽:");
    ImGui::SameLine();
    bool drag = config.enable_drag;
    if (ImGui::Checkbox("##EnableDrag", &drag)) {
        config.enable_drag = drag;
        mark_modified("live2d.enable_drag");
    }
}

void Live2DSettingsWidget::render_debug_settings() {
    auto& config = m_plugin->get_config();

    ImGui::Text("=== 调试设置 ===");

    // 显示碰撞区域
    ImGui::Text("显示碰撞区域:");
    ImGui::SameLine();
    bool hit_areas = config.show_hit_areas;
    if (ImGui::Checkbox("##ShowHitAreas", &hit_areas)) {
        config.show_hit_areas = hit_areas;
        mark_modified("live2d.show_hit_areas");
    }

    // 显示参数列表
    ImGui::Text("显示参数列表:");
    ImGui::SameLine();
    bool parameters = config.show_parameters;
    if (ImGui::Checkbox("##ShowParameters", &parameters)) {
        config.show_parameters = parameters;
        mark_modified("live2d.show_parameters");
    }
}

const char* Live2DSettingsWidget::get_position_name(Live2DModelPosition position) {
    switch (position) {
        case Live2DModelPosition::TopLeft:     return "左上角";
        case Live2DModelPosition::TopCenter:   return "顶部居中";
        case Live2DModelPosition::TopRight:    return "右上角";
        case Live2DModelPosition::CenterLeft:  return "左侧居中";
        case Live2DModelPosition::Center:      return "居中";
        case Live2DModelPosition::CenterRight: return "右侧居中";
        case Live2DModelPosition::BottomLeft:  return "左下角";
        case Live2DModelPosition::BottomCenter:return "底部居中";
        case Live2DModelPosition::BottomRight: return "右下角";
        default: return "未知";
    }
}

} // namespace DearTs::Plugins::Live2D
