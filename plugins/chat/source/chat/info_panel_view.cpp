/**
 * @file info_panel_view.cpp
 * @brief 信息面板视图实现
 */

#include "chat/views/info_panel_view.hpp"
#include "chat/events/chat_events.hpp"
#include "liblogger/logger.h"
#include <imgui.h>
#include <fmt/format.h>

namespace DearTs::Plugins::Chat {

void InfoPanelView::draw_content() {
    if (ImGui::BeginTabBar("##info_tabs")) {
        if (ImGui::BeginTabItem("AI 设置")) {
            draw_ai_settings();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("会话信息")) {
            draw_conversation_info();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("导出")) {
            draw_export_section();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}

void InfoPanelView::draw_ai_settings() {
    // LLM 提供商选择
    draw_llm_provider_selector();

    ImGui::Separator();

    // 模型设置
    draw_model_settings();

    ImGui::Separator();

    // 高级设置
    if (ImGui::CollapsingHeader("高级设置")) {
        // Top-p
        ImGui::Text("Top-p 采样:");
        if (ImGui::SliderFloat("##top_p", &m_temperature, 0.0f, 1.0f, "%.2f")) {
            // 发布配置更新事件
            EventBus::instance().publish(Events::ConfigUpdatedEvent{
                .config_key = "llm.top_p",
                .old_value = "",
                .new_value = fmt::format("{}", m_temperature)
            });
        }

        // 停止序列
        static char stop_sequence[256] = "";
        ImGui::Text("停止序列:");
        if (ImGui::InputText("##stop_sequence", stop_sequence, sizeof(stop_sequence))) {
            // 更新停止序列
        }

        // 系统提示词
        static char system_prompt[1024] = "你是一个友好、专业的 AI 助手。";
        ImGui::Text("系统提示词:");
        if (ImGui::InputTextMultiline("##system_prompt", system_prompt, sizeof(system_prompt), ImVec2(-1, 60))) {
            // 更新系统提示词
        }
    }
}

void InfoPanelView::draw_llm_provider_selector() {
    ImGui::Text("%s LLM 提供商", ICON_SETTINGS);

    // 提供商下拉选择
    const char* current_provider = m_selected_provider.c_str();
    if (ImGui::BeginCombo("##provider", current_provider)) {
        for (const auto& provider : m_available_providers) {
            const bool is_selected = (m_selected_provider == provider);
            if (ImGui::Selectable(provider.c_str(), is_selected)) {
                change_llm_provider(provider);
            }
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    // 提供商说明
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
    if (m_selected_provider == "http") {
        ImGui::TextWrapped("通过 HTTP API 连接到 LLM 服务（如 Ollama、OpenAI）");
    } else if (m_selected_provider == "python") {
        ImGui::TextWrapped("使用 Python 脚本调用本地 LLM");
    } else if (m_selected_provider == "cli") {
        ImGui::TextWrapped("通过命令行工具调用 LLM");
    }
    ImGui::PopStyleColor();

    // API 密钥（仅 HTTP）
    if (m_selected_provider == "http") {
        ImGui::Text("API 密钥:");
        static char api_key[256] = "";
        if (ImGui::InputText("##api_key", api_key, sizeof(api_key), ImGuiInputTextFlags_Password)) {
            // 更新 API 密钥
        }
    }
}

void InfoPanelView::draw_model_settings() {
    ImGui::Text("%s 模型设置", ICON_SMART_TOY);

    // 模型选择
    const char* current_model = m_selected_model.c_str();
    if (ImGui::BeginCombo("##model", current_model)) {
        for (const auto& model : m_available_models) {
            const bool is_selected = (m_selected_model == model);
            if (ImGui::Selectable(model.c_str(), is_selected)) {
                change_model(model);
            }
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    // Temperature
    ImGui::Text("温度:");
    if (ImGui::SliderFloat("##temperature", &m_temperature, 0.0f, 2.0f, "%.2f")) {
        // 发布配置更新事件
        auto current_conv = m_conversation_manager->get_current_conversation();
        if (current_conv) {
            current_conv->temperature = m_temperature;
        }

        EventBus::instance().publish(Events::ConfigUpdatedEvent{
            .config_key = "llm.temperature",
            .old_value = "",
            .new_value = fmt::format("{}", m_temperature)
        });
    }

    ImGui::SameLine();
    ImGui::HelpMarker("控制输出随机性，值越高越随机");

    // Max Tokens
    ImGui::Text("最大 Tokens:");
    if (ImGui::InputInt("##max_tokens", &m_max_tokens, 256, 1024)) {
        m_max_tokens = std::max(256, std::min(8192, m_max_tokens));

        auto current_conv = m_conversation_manager->get_current_conversation();
        if (current_conv) {
            current_conv->max_tokens = m_max_tokens;
        }
    }
}

void InfoPanelView::draw_conversation_info() {
    auto current_conv = m_conversation_manager->get_current_conversation();

    if (!current_conv) {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "未选择会话");
        return;
    }

    ImGui::Text("%s 会话信息", ICON_INFO);

    ImGui::Separator();

    // 基本信息
    ImGui::Text("标题: %s", current_conv->title.c_str());
    ImGui::Text("ID: %s", current_conv->id.c_str());

    // 类型
    const char* type_str = "未知";
    switch (current_conv->type) {
        case ConversationType::Chat: type_str = "单聊"; break;
        case ConversationType::Group: type_str = "群聊"; break;
        case ConversationType::AI: type_str = "AI 对话"; break;
        case ConversationType::System: type_str = "系统"; break;
    }
    ImGui::Text("类型: %s", type_str);

    ImGui::Separator();

    // 统计信息
    ImGui::Text("%s 统计", ICON_ANALYTICS);
    ImGui::Text("消息数: %zu", current_conv->get_message_count());
    ImGui::Text("Token 数: ~%zu", current_conv->estimate_total_tokens());

    // 时间信息
    const auto created_time = std::chrono::system_clock::to_time_t(current_conv->created_at);
    const auto updated_time = std::chrono::system_clock::to_time_t(current_conv->updated_at);
    ImGui::Text("创建: %s", std::ctime(&created_time));
    ImGui::Text("更新: %s", std::ctime(&updated_time));

    ImGui::Separator();

    // 操作按钮
    if (ImGui::Button("清空消息")) {
        current_conv->clear_messages();
        LOG_INFO("Cleared messages in conversation {}", current_conv->id);
    }

    ImGui::SameLine();

    if (ImGui::Button("删除会话")) {
        m_conversation_manager->delete_conversation(current_conv->id);
        LOG_INFO("Deleted conversation {}", current_conv->id);
    }
}

void InfoPanelView::draw_export_section() {
    auto current_conv = m_conversation_manager->get_current_conversation();

    ImGui::Text("%s 导出会话", ICON_DOWNLOAD);

    if (!current_conv) {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "未选择会话");
        return;
    }

    ImGui::Separator();

    // 导出格式
    ImGui::Text("导出格式:");
    const char* formats[] = {"JSON", "Markdown", "TXT"};
    for (int i = 0; i < 3; i++) {
        if (i > 0) ImGui::SameLine();
        if (ImGui::RadioButton(formats[i], m_export_format == fmt::format("{}", formats[i])[0])) {
            m_export_format = fmt::format("{}", formats[i]);
        }
    }

    // 导出路径
    ImGui::Text("导出路径:");
    static char export_path[256] = "conversation_export";
    ImGui::InputText("##export_path", export_path, sizeof(export_path));

    ImGui::SameLine();

    if (ImGui::Button("浏览...")) {
        // 打开文件选择对话框
    }

    ImGui::Separator();

    // 导出按钮
    if (ImGui::Button(fmt::format("{} 导出", ICON_FILE_DOWNLOAD).c_str(), ImVec2(-1, 0))) {
        export_conversation(m_export_format);
    }
}

void InfoPanelView::change_llm_provider(const std::string& provider) {
    const std::string old_provider = m_selected_provider;
    m_selected_provider = provider;

    // 发布 LLM 提供商切换事件
    EventBus::instance().publish(Events::LLMProviderChangedEvent{
        .old_provider = old_provider,
        .new_provider = provider
    });

    LOG_INFO("Changed LLM provider from {} to {}", old_provider, provider);
}

void InfoPanelView::change_model(const std::string& model) {
    const std::string old_model = m_selected_model;
    m_selected_model = model;

    // 发布模型切换事件
    EventBus::instance().publish(Events::LLMModelChangedEvent{
        .old_model = old_model,
        .new_model = model
    });

    auto current_conv = m_conversation_manager->get_current_conversation();
    if (current_conv) {
        current_conv->llm_model = model;
    }

    LOG_INFO("Changed LLM model from {} to {}", old_model, model);
}

void InfoPanelView::export_conversation(const std::string& format) {
    auto current_conv = m_conversation_manager->get_current_conversation();
    if (!current_conv) return;

    // 发布导出请求事件
    EventBus::instance().publish(Events::ExportRequestEvent{
        .conversation_id = current_conv->id,
        .format = format,
        .output_path = m_export_path
    });

    LOG_INFO("Exporting conversation {} as {}", current_conv->id, format);
}

} // namespace DearTs::Plugins::Chat
