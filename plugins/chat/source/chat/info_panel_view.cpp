/**
 * @file info_panel_view.cpp
 * @brief 信息面板视图实现
 */

#include "chat/views/info_panel_view.hpp"
#include "chat/events/chat_events.hpp"
#include "chat/ui/measure_context.hpp"
#include "memory_core/memory/memory_manager.hpp"
#include "memory_core/persistence/database.hpp"
#include "memory_core/events/memory_events.hpp"
#include "core/ui/imgui_extensions.h"
#include "core/ui/icon_font.hpp"
#include "liblogger/logger.h"
#include <imgui.h>
#include <format>
#include <fstream>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace DearTs::Plugins::Chat {

// ============ 配置管理实现 ============

void InfoPanelView::load_config() {
    // 加载 LLM 配置
    m_selected_provider = m_config.get_or<std::string>("llm.provider", "ollama");
    m_selected_model = m_config.get_or<std::string>("llm.model", "llama3.2");
    m_ollama_base_url = m_config.get_or<std::string>("llm.ollama_base_url", "http://localhost:11434");
    m_temperature = m_config.get_or<double>("llm.temperature", 0.7);
    m_max_tokens = m_config.get_or<int>("llm.max_tokens", 2048);

    // 加载 Memory 配置
    m_memory_debug_data.auto_refresh = m_config.get_or<bool>("memory.auto_refresh", true);
    m_memory_debug_data.max_results = m_config.get_or<int>("memory.max_results", 5);
    m_memory_debug_data.min_similarity = m_config.get_or<double>("memory.min_similarity", 0.5);

    // 加载导出配置
    m_export_format = m_config.get_or<std::string>("export.format", "json");
    m_export_path = m_config.get_or<std::string>("export.path", "");

    // 加载 UI 配置
    m_show_advanced = m_config.get_or<bool>("ui.show_advanced", false);

    LOG_INFO("Configuration loaded for InfoPanelView");
}

void InfoPanelView::save_config() {
    // 使用防抖（500ms），避免频繁保存
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_config_save).count();
    if (elapsed < 500) {
        return;  // 距离上次保存不足 500ms，跳过
    }
    m_last_config_save = now;

    // 保存 LLM 配置
    m_config.set("llm.provider", m_selected_provider);
    m_config.set("llm.model", m_selected_model);
    m_config.set("llm.ollama_base_url", m_ollama_base_url);
    m_config.set("llm.temperature", m_temperature);
    m_config.set("llm.max_tokens", m_max_tokens);

    // 保存 Memory 配置
    m_config.set("memory.auto_refresh", m_memory_debug_data.auto_refresh);
    m_config.set("memory.max_results", m_memory_debug_data.max_results);
    m_config.set("memory.min_similarity", m_memory_debug_data.min_similarity);

    // 保存导出配置
    m_config.set("export.format", m_export_format);
    m_config.set("export.path", m_export_path);

    // 保存 UI 配置
    m_config.set("ui.show_advanced", m_show_advanced);

    LOG_DEBUG("Configuration saved for InfoPanelView");
}

InfoPanelView::InfoPanelView(std::shared_ptr<ConversationManager> manager)
    : ViewWindow(UnlocalizedString("信息"), ICON_INFO)
    , m_conversation_manager(std::move(manager)) {
    // 加载配置
    load_config();
    // 设置事件监听
    setup_event_listeners();
}

InfoPanelView::~InfoPanelView() {
    // EventToken 的 RAII 会自动取消订阅
    m_event_tokens.clear();
}

void InfoPanelView::setup_event_listeners() {
    // 订阅 Ollama 模型列表更新事件
    m_event_tokens.push_back(DearTs::Core::Event::EventBus::instance().subscribe<Events::OllamaModelsUpdatedEvent>(
        [this](const Events::OllamaModelsUpdatedEvent& e) {
            if (!e.models.empty()) {
                // 更新模型列表
                set_available_models(e.models);
                m_ollama_refreshing = false;
                LOG_INFO("InfoPanelView: Updated {} models from {}", e.models.size(), e.base_url);
            }
        }
    ));

    // 订阅 Ollama 连接状态事件
    m_event_tokens.push_back(DearTs::Core::Event::EventBus::instance().subscribe<Events::OllamaConnectionStatusEvent>(
        [this](const Events::OllamaConnectionStatusEvent& e) {
            set_ollama_connection_status(e.is_connected, e.error_message);
            LOG_INFO("InfoPanelView: Ollama connection status: {}", e.is_connected ? "Connected" : "Failed");
        }
    ));

#ifdef SQLITE3_FOUND
    // ========== Memory Core 事件订阅 ==========
    using namespace MemoryCore::Events;

    // RAG 查询完成事件
    m_event_tokens.push_back(
        DearTs::Core::Event::EventBus::instance().subscribe<RAGQueryCompletedEvent>(
            [this](const RAGQueryCompletedEvent& e) {
                // 更新查询结果
                m_memory_debug_data.last_query_results.clear();
                for (const auto& item : e.results) {
                    m_memory_debug_data.last_query_results.push_back({
                        .content = item.content,
                        .source_conversation_id = item.source_conversation_id,
                        .similarity = item.similarity,
                        .memory_type = item.memory_type,
                        .timestamp = item.timestamp
                    });
                }
                m_memory_debug_data.last_query = e.query;

                // 更新事件统计
                auto& stats = m_memory_debug_data.event_stats["RAGQueryCompleted"];
                stats.count++;
                stats.last_triggered = std::chrono::system_clock::now();

                // 添加事件日志
                m_memory_debug_data.event_log.push_back({
                    .timestamp = format_current_time(),
                    .message = std::format("RAG 查询完成: {} 条结果", e.results.size()),
                    .color = ImVec4(0.3f, 1.0f, 0.3f, 1.0f)
                });

                // 限制日志大小
                if (m_memory_debug_data.event_log.size() > MemoryDebugData::MAX_LOG_ENTRIES) {
                    m_memory_debug_data.event_log.erase(m_memory_debug_data.event_log.begin());
                }

                LOG_DEBUG("InfoPanelView: RAG query completed with {} results", e.results.size());
            }
        )
    );

    // 记忆提取完成事件
    m_event_tokens.push_back(
        DearTs::Core::Event::EventBus::instance().subscribe<MemoryExtractedEvent>(
            [this](const MemoryExtractedEvent& e) {
                // 更新事件统计
                auto& stats = m_memory_debug_data.event_stats["MemoryExtracted"];
                stats.count++;
                stats.last_triggered = std::chrono::system_clock::now();

                // 添加事件日志
                m_memory_debug_data.event_log.push_back({
                    .timestamp = format_current_time(),
                    .message = std::format("提取了 {} 条记忆", e.memories.size()),
                    .color = ImVec4(0.3f, 0.7f, 1.0f, 1.0f)
                });

                if (m_memory_debug_data.event_log.size() > MemoryDebugData::MAX_LOG_ENTRIES) {
                    m_memory_debug_data.event_log.erase(m_memory_debug_data.event_log.begin());
                }

                LOG_DEBUG("InfoPanelView: Memory extracted: {} memories", e.memories.size());
            }
        )
    );

    // 消息保存完成事件
    m_event_tokens.push_back(
        DearTs::Core::Event::EventBus::instance().subscribe<MessageSavedEvent>(
            [this](const MessageSavedEvent& e) {
                // 更新事件统计
                auto& stats = m_memory_debug_data.event_stats["MessageSaved"];
                stats.count++;
                stats.last_triggered = std::chrono::system_clock::now();

                LOG_DEBUG("InfoPanelView: Message saved: {}", e.message_uuid);
            }
        )
    );
#endif
}

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

        if (ImGui::BeginTabItem("调试")) {
            draw_debug_section();
            ImGui::EndTabItem();
        }

        // 新增：Memory Debug 选项卡
        if (ImGui::BeginTabItem("Memory")) {
            draw_memory_debug();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}

void InfoPanelView::draw_ai_settings() {
    // LLM 提供商选择
    draw_llm_provider_selector();

    ImGui::Separator();

    // Ollama 专用设置
    if (m_selected_provider == "ollama") {
        draw_ollama_settings();
        ImGui::Separator();
    }

    // 模型设置
    draw_model_settings();

    ImGui::Separator();

    // 高级设置
    if (ImGui::CollapsingHeader("高级设置")) {
        // Top-p
        ImGui::Text("Top-p 采样:");
        float temp_float = static_cast<float>(m_temperature);
        if (ImGui::SliderFloat("##top_p", &temp_float, 0.0f, 1.0f, "%.2f")) {
            m_temperature = static_cast<double>(temp_float);
            // 发布配置更新事件
            DearTs::Core::Event::EventBus::instance().publish(Events::ConfigUpdatedEvent{
                .config_key = "llm.top_p",
                .old_value = "",
                .new_value = std::format("{}", m_temperature)
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
    if (m_selected_provider == "ollama") {
        ImGui::TextWrapped("本地 Ollama 服务，支持多种开源模型");
    } else if (m_selected_provider == "http") {
        ImGui::TextWrapped("通过 HTTP API 连接到 LLM 服务（如 OpenAI）");
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

void InfoPanelView::draw_ollama_settings() {
    ImGui::Text("%s Ollama 设置", ICON_SERVER);

    // 连接状态指示器
    float cursor_x = ImGui::GetCursorPosX();
    float cursor_y = ImGui::GetCursorPosY();

    // 绘制状态圆点
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec2 p_min = ImGui::GetCursorScreenPos();
    const ImVec2 p_max = ImVec2(p_min.x + 8, p_min.y + 8);
    const ImU32 status_color = m_ollama_connected ?
        IM_COL32(100, 255, 100, 255) : IM_COL32(255, 100, 100, 255);
    draw_list->AddCircleFilled(ImVec2(p_min.x + 4, p_min.y + 4), 4.0f, status_color);

    ImGui::SetCursorPosX(cursor_x + 15);
    ImGui::SetCursorPosY(cursor_y);

    ImGui::Text("%s", m_ollama_connected ? "已连接" : "未连接");

    if (!m_ollama_connected && !m_ollama_connection_error.empty()) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "(%s)", m_ollama_connection_error.c_str());
    }

    // API URL 输入
    ImGui::Text("API 地址:");
    char url_buffer[256];
    strncpy(url_buffer, m_ollama_base_url.c_str(), sizeof(url_buffer) - 1);
    url_buffer[sizeof(url_buffer) - 1] = '\0';

    if (ImGui::InputText("##ollama_url", url_buffer, sizeof(url_buffer))) {
        m_ollama_base_url = url_buffer;
    }

    ImGui::SameLine();

    // 测试连接按钮
    if (ImGui::Button("测试连接")) {
        // 直接测试连接（不通过事件系统避免循环）
        m_ollama_refreshing = true;  // 使用刷新标志作为"测试中"标志
        // 发布测试请求事件
        DearTs::Core::Event::EventBus::instance().publish(Events::OllamaConnectionStatusEvent{
            .is_connected = false,  // false 表示请求测试
            .base_url = m_ollama_base_url,
            .error_message = ""
        });
    }

    // 刷新模型列表按钮
    if (m_ollama_refreshing) {
        ImGui::BeginDisabled();
        ImGui::Button("刷新中...");
        ImGui::EndDisabled();
    } else {
        if (ImGui::Button("刷新模型列表")) {
            refresh_ollama_models();
        }
    }

    ImGui::SameLine();

    // 显示可用模型数量
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "(%zu 个模型)", m_available_models.size());

    // 连接提示
    if (!m_ollama_connected) {
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.5f, 1.0f));
        ImGui::TextWrapped("提示: 确保 Ollama 服务正在运行，默认地址为 http://localhost:11434");
        ImGui::PopStyleColor();
    }
}

void InfoPanelView::refresh_ollama_models() {
    m_ollama_refreshing = true;

    // 发布刷新请求事件（让 chat_plugin 处理）
    DearTs::Core::Event::EventBus::instance().publish(Events::OllamaModelsUpdatedEvent{
        .models = {},  // 空列表表示请求刷新
        .base_url = m_ollama_base_url
    });

    LOG_INFO("Requested Ollama model list refresh from {}", m_ollama_base_url);
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
    float temp_float2 = static_cast<float>(m_temperature);
    if (ImGui::SliderFloat("##temperature", &temp_float2, 0.0f, 2.0f, "%.2f")) {
        m_temperature = static_cast<double>(temp_float2);
        // 发布配置更新事件
        auto current_conv = m_conversation_manager->get_current_conversation();
        if (current_conv) {
            current_conv->temperature = m_temperature;
        }

        DearTs::Core::Event::EventBus::instance().publish(Events::ConfigUpdatedEvent{
            .config_key = "llm.temperature",
            .old_value = "",
            .new_value = std::format("{}", m_temperature)
        });

        // 保存配置
        save_config();
    }

    ImGui::SameLine();
    DearTs::Core::UI::ImGuiExt::HelpMarker("控制输出随机性，值越高越随机");

    // Max Tokens
    ImGui::Text("最大 Tokens:");
    if (ImGui::InputInt("##max_tokens", &m_max_tokens, 256, 1024)) {
        m_max_tokens = std::max(256, std::min(8192, m_max_tokens));

        auto current_conv = m_conversation_manager->get_current_conversation();
        if (current_conv) {
            current_conv->max_tokens = m_max_tokens;
        }

        // 保存配置
        save_config();
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
        if (ImGui::RadioButton(formats[i], m_export_format == formats[i])) {
            m_export_format = formats[i];
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
    if (ImGui::Button(std::format("{} 导出", ICON_FILE_DOWNLOAD).c_str(), ImVec2(-1, 0))) {
        export_conversation(m_export_format);
    }
}

void InfoPanelView::change_llm_provider(const std::string& provider) {
    const std::string old_provider = m_selected_provider;
    m_selected_provider = provider;

    // 发布 LLM 提供商切换事件
    DearTs::Core::Event::EventBus::instance().publish(Events::LLMProviderChangedEvent{
        .old_provider = old_provider,
        .new_provider = provider
    });

    // 保存配置
    save_config();

    LOG_INFO("Changed LLM provider from {} to {}", old_provider, provider);
}

void InfoPanelView::change_model(const std::string& model) {
    const std::string old_model = m_selected_model;
    m_selected_model = model;

    // 发布模型切换事件
    DearTs::Core::Event::EventBus::instance().publish(Events::LLMModelChangedEvent{
        .old_model = old_model,
        .new_model = model
    });

    auto current_conv = m_conversation_manager->get_current_conversation();
    if (current_conv) {
        current_conv->llm_model = model;
    }

    // 保存配置
    save_config();

    LOG_INFO("Changed LLM model from {} to {}", old_model, model);
}

void InfoPanelView::export_conversation(const std::string& format) {
    auto current_conv = m_conversation_manager->get_current_conversation();
    if (!current_conv) return;

    // 发布导出请求事件
    DearTs::Core::Event::EventBus::instance().publish(Events::ExportRequestEvent{
        .conversation_id = current_conv->id,
        .format = format,
        .output_path = m_export_path
    });

    LOG_INFO("Exporting conversation {} as {}", current_conv->id, format);
}

void InfoPanelView::set_available_models(const std::vector<std::string>& models) {
    m_available_models = models;

    // 如果当前选择的模型不在新列表中，选择第一个模型
    if (!models.empty()) {
        bool found = false;
        for (const auto& model : models) {
            if (model == m_selected_model) {
                found = true;
                break;
            }
        }
        if (!found) {
            change_model(models[0]);
        }
    }
}

void InfoPanelView::set_ollama_connection_status(bool connected, const std::string& error) {
    m_ollama_connected = connected;
    m_ollama_connection_error = error;
}

void InfoPanelView::draw_debug_section() {
    // 测试模式（可展开/折叠）
    if (ImGui::CollapsingHeader("测试消息")) {
        draw_test_messages_section();
        ImGui::Separator();
    }

    ImGui::Separator();

    // 测量缓存统计
    ImGui::Text("%s 测量缓存统计", ICON_ANALYTICS);

    ImGui::Separator();

    // 获取缓存统计信息
    auto stats = UI::MeasureContext::instance().get_cache_stats();
    auto config = UI::MeasureContext::instance().get_cache_config();

    // 显示统计信息
    ImGui::Text("缓存大小: %zu / %zu", stats.current_size, config.max_entries);
    ImGui::Text("命中率: %.1f%%", stats.hit_rate() * 100.0);
    ImGui::Text("命中次数: %zu", stats.hits);
    ImGui::Text("未命中次数: %zu", stats.misses);
    ImGui::Text("清除次数: %zu", stats.evictions);

    ImGui::Separator();

    // 显示配置信息
    ImGui::Text("最大条目数: %zu", config.max_entries);
    ImGui::Text("过期时间: %lld 秒", static_cast<long long>(config.max_age.count()));
    ImGui::Text("LRU 清除: %s", config.enable_lru ? "启用" : "禁用");

    ImGui::Separator();

    // 操作按钮
    if (ImGui::Button("清除过期缓存")) {
        UI::MeasureContext::instance().clear_expired_cache();
    }

    ImGui::SameLine();

    if (ImGui::Button("清除所有缓存")) {
        UI::MeasureContext::instance().clear_cache();
    }

    // 显示内存占用估算
    ImGui::Spacing();
    const size_t estimated_memory = stats.current_size * 100;  // 每个条目约 100 字节
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
        "估算内存占用: ~%zu KB", (estimated_memory + 1023) / 1024);
}

void InfoPanelView::draw_test_messages_section() {
    auto current_conv = m_conversation_manager->get_current_conversation();
    if (!current_conv) {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "请先选择一个会话");
        return;
    }

    ImGui::TextWrapped("测试消息：点击按钮添加不同类型的测试消息");

    ImGui::Spacing();

    // 测试按钮
    if (ImGui::Button("添加用户消息")) {
        add_test_message("这是一条用户消息，用于测试气泡显示效果。", MessageRole::User);
    }

    ImGui::SameLine();

    if (ImGui::Button("添加纯文本 AI 回复")) {
        add_test_message("这是一条纯文本 AI 回复，用于测试文本对齐和间距。", MessageRole::Assistant);
    }

    ImGui::Spacing();

    if (ImGui::Button("添加 Markdown AI 回复")) {
        std::string markdown_content =
            "这是一条**包含 Markdown** 的 AI 回复：\n\n"
            "```cpp\n"
            "void hello() {\n"
            "    printf(\"Hello World\");\n"
            "}\n"
            "```\n\n"
            "* 列表项 1\n"
            "* 列表项 2\n"
            "* 列表项 3";
        add_test_message(markdown_content, MessageRole::Assistant);
    }

    ImGui::SameLine();

    if (ImGui::Button("imgui_markdown 文档")) {
        const std::string markdown_text =R"(
# imgui_markdown 文档

Markdown For Dear ImGui 是一个宽松授权的单头文件库。

## 特性

  * 自动文本换行
  * 支持标题 H1, H2, H3
  * 支持强调（斜体/粗体）
  * 支持多级缩进
  * 支持无序列表和子列表
  * 支持链接和图片
  * 支持水平线

## 标题语法

```
# H1 标题
## H2 标题
### H3 标题
```

## 强调语法

```
*斜体*
_斜体_
**粗体**
__粗体__
```

## 列表语法

  * 列表项 1
    * 子项 1.1
  * 列表项 2

## 链接语法

```
[链接文字](https://example.com)
```
[链接文字](https://example.com)

## 图片语法

```
![图片替代文字](图片标识符)
```
![图片替代文字](图片标识符)

## 水平线语法

```
***
___
```

***
___
)";
        add_test_message(markdown_text, MessageRole::Assistant);
    }

    ImGui::Spacing();

    if (ImGui::Button("添加长消息")) {
        std::string long_msg;
        for (int i = 0; i < 20; i++) {
            long_msg += "这是一段很长的文本，用于测试多行消息的换行和间距问题。";
        }
        add_test_message(long_msg, MessageRole::Assistant);
    }

    ImGui::SameLine();

    if (ImGui::Button("添加多行代码块")) {
        std::string code_content =
            "这是一个包含代码块的示例：\n\n"
            "```cpp\n"
            "// Calculate factorial\n"
            "unsigned long factorial(unsigned long n) {\n"
            "    if (n == 0) return 1;\n"
            "    return n * factorial(n - 1);\n"
            "}\n"
            "```\n\n"
            "这段代码展示了递归计算阶乘的实现方式。";
        add_test_message(code_content, MessageRole::Assistant);
    }

    ImGui::SameLine();

    if (ImGui::Button("清空消息")) {
        current_conv->messages.clear();
        current_conv->touch();
        LOG_INFO("Cleared all messages in conversation {}", current_conv->id);
    }
}

void InfoPanelView::add_test_message(const std::string& content, MessageRole role) {
    auto current_conv = m_conversation_manager->get_current_conversation();
    if (!current_conv) {
        LOG_WARN("No current conversation when adding test message");
        return;
    }

    Message test_msg(content, role);
    current_conv->add_message(test_msg);
    current_conv->touch();

    // 发布滚动到底部事件
    DearTs::Core::Event::EventBus::instance().publish(Events::ScrollToBottomEvent{
        .conversation_id = current_conv->id
    });
}

// ========== Memory Debug UI 实现 ==========

void InfoPanelView::draw_memory_debug() {
#ifdef SQLITE3_FOUND
    // 顶部操作栏
    if (ImGui::Button("刷新")) {
        refresh_memory_debug_data();
    }
    ImGui::SameLine();
    ImGui::Checkbox("自动刷新", &m_memory_debug_data.auto_refresh);
    ImGui::SameLine();
    if (ImGui::Button("导出统计")) {
        export_memory_debug_stats();
    }

    // 自动刷新（每 5 秒）
    if (m_memory_debug_data.auto_refresh) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_last_refresh).count();
        if (elapsed >= 5) {
            refresh_memory_debug_data();
            m_last_refresh = now;
        }
    }

    ImGui::Separator();

    // 1. 记忆统计
    if (ImGui::CollapsingHeader("记忆统计", ImGuiTreeNodeFlags_DefaultOpen)) {
        draw_memory_stats_section();
    }

    // 2. 数据库状态
    if (ImGui::CollapsingHeader("数据库状态")) {
        draw_database_status_section();
    }

    // 3. RAG 查询
    if (ImGui::CollapsingHeader("RAG 查询测试")) {
        draw_rag_query_section();
    }

    // 4. 事件监控
    if (ImGui::CollapsingHeader("事件监控")) {
        draw_event_monitor_section();
    }

    // 5. 一致性管理
    if (ImGui::CollapsingHeader("一致性管理")) {
        draw_consistency_section();
    }

#else
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
        "Memory Core 功能未启用");
    ImGui::TextWrapped("编译时未找到 SQLite3，Memory Core 功能不可用。");
    ImGui::TextWrapped("请在 CMake 配置时启用 SQLite3 支持。");
#endif
}

void InfoPanelView::draw_memory_stats_section() {
#ifdef SQLITE3_FOUND
    using namespace MemoryCore::Memory;

    ImGui::Text("总记忆数: %zu", m_memory_debug_data.total_memories);

    if (m_memory_debug_data.total_memories > 0) {
        ImGui::Text("按类型分布:");
        for (const auto& type_count : m_memory_debug_data.memory_type_counts) {
            float percentage = (float)type_count.count / m_memory_debug_data.total_memories;

            ImGui::ProgressBar(percentage, ImVec2(-1, 0),
                std::format("{}: {} ({:.1f}%)",
                    type_count.name, type_count.count, percentage * 100).c_str());
        }
    } else {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "暂无记忆数据");
    }
#endif
}

void InfoPanelView::draw_database_status_section() {
#ifdef SQLITE3_FOUND
    using namespace MemoryCore::Persistence;

    // 连接状态指示器
    bool is_open = SQLiteDatabase::instance().is_open();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImU32 color = is_open ? IM_COL32(100, 255, 100, 255) : IM_COL32(255, 100, 100, 255);
    draw_list->AddCircleFilled(ImVec2(p.x + 6, p.y + 6), 4.0f, color);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 15);
    ImGui::Text("%s", is_open ? "已连接" : "未连接");

    // 数据库路径
    if (is_open) {
        ImGui::Text("路径: %s", SQLiteDatabase::instance().get_db_path().c_str());
    }
#endif
}

void InfoPanelView::draw_rag_query_section() {
#ifdef SQLITE3_FOUND
    // 查询输入框
    static char query_buffer[512] = "";
    ImGui::Text("查询文本:");
    ImGui::InputTextMultiline("##rag_query", query_buffer, sizeof(query_buffer), ImVec2(-1, 60));

    // 查询选项
    ImGui::SliderInt("最大结果数", &m_memory_debug_data.max_results, 1, 20);
    float sim_float = static_cast<float>(m_memory_debug_data.min_similarity);
    if (ImGui::SliderFloat("最小相似度", &sim_float, 0.0f, 1.0f, "%.2f")) {
        m_memory_debug_data.min_similarity = static_cast<double>(sim_float);
    }

    // 执行查询按钮
    if (ImGui::Button("执行查询")) {
        // 发布 RAG 查询事件
        MemoryCore::Events::RAGQueryRequestedEvent event;
        event.query = query_buffer;
        event.max_results = m_memory_debug_data.max_results;
        event.min_similarity = m_memory_debug_data.min_similarity;
        event.init_base("InfoPanelView");

        DearTs::Core::Event::EventBus::instance().publish(event);

        LOG_INFO("Published RAG query request: {}", query_buffer);
    }

    // 显示查询结果
    if (!m_memory_debug_data.last_query_results.empty()) {
        ImGui::Separator();
        ImGui::Text("查询结果 (%zu 条):", m_memory_debug_data.last_query_results.size());

        for (const auto& result : m_memory_debug_data.last_query_results) {
            if (ImGui::CollapsingHeader(result.content.c_str())) {
                ImGui::Text("相似度: %.2f", result.similarity);
                ImGui::Text("类型: %s", result.memory_type.c_str());
                if (!result.source_conversation_id.empty()) {
                    ImGui::Text("来源会话: %s", result.source_conversation_id.c_str());
                }
            }
        }
    }
#endif
}

void InfoPanelView::draw_event_monitor_section() {
    // 事件统计表格
    if (ImGui::BeginTable("EventStats", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable)) {
        ImGui::TableSetupColumn("事件类型");
        ImGui::TableSetupColumn("触发次数");
        ImGui::TableSetupColumn("最后触发");
        ImGui::TableHeadersRow();

        for (const auto& [event_name, stats] : m_memory_debug_data.event_stats) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", event_name.c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%zu", stats.count);
            ImGui::TableSetColumnIndex(2);

            // 格式化时间
            auto time_t = std::chrono::system_clock::to_time_t(stats.last_triggered);
            std::tm tm;
            localtime_s(&tm, &time_t);
            char buffer[64];
            std::strftime(buffer, sizeof(buffer), "%H:%M:%S", &tm);
            ImGui::Text("%s", buffer);
        }
        ImGui::EndTable();
    }

    // 实时事件日志
    ImGui::Text("实时事件日志:");
    static bool auto_scroll = true;
    ImGui::Checkbox("自动滚动", &auto_scroll);

    if (ImGui::BeginChild("EventLog", ImVec2(-1, 200), true)) {
        for (const auto& event : m_memory_debug_data.event_log) {
            ImGui::TextColored(event.color, "[%s] %s",
                event.timestamp.c_str(), event.message.c_str());
            if (auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
                ImGui::SetScrollHereY(1.0f);
            }
        }
    }
    ImGui::EndChild();

    if (ImGui::Button("清除日志")) {
        m_memory_debug_data.event_log.clear();
    }
}

void InfoPanelView::draw_consistency_section() {
    ImGui::Text("一致性管理功能开发中...");

    // 操作按钮（占位符）
    if (ImGui::Button("立即同步")) {
        LOG_INFO("Manual sync requested");
    }
    ImGui::SameLine();
    if (ImGui::Button("处理离线队列")) {
        LOG_INFO("Process offline queue requested");
    }

    // 嵌入缓存统计（占位符）
    ImGui::Separator();
    ImGui::Text("嵌入缓存:");
    ImGui::Text("缓存命中: 0");
    ImGui::Text("缓存未命中: 0");
    ImGui::Text("命中率: N/A");

    if (ImGui::Button("清除缓存")) {
        LOG_INFO("Clear cache requested");
    }
}

void InfoPanelView::refresh_memory_debug_data() {
#ifdef SQLITE3_FOUND
    using namespace MemoryCore::Memory;

    try {
        // 获取记忆统计
        auto count_result = MemoryManager::instance().get_memory_count();
        if (count_result.isOk()) {
            m_memory_debug_data.total_memories = count_result.unwrap();
        } else {
            LOG_ERROR("Failed to get memory count: {}", count_result.error());
        }

        auto type_counts_result = MemoryManager::instance().get_memory_count_by_type();
        if (type_counts_result.isOk()) {
            auto counts = type_counts_result.unwrap();
            m_memory_debug_data.memory_type_counts.clear();
            for (const auto& [type, count] : counts) {
                m_memory_debug_data.memory_type_counts.push_back({
                    count, Memory::type_to_string(type)
                });
            }
        } else {
            LOG_ERROR("Failed to get memory count by type: {}", type_counts_result.error());
        }

        LOG_DEBUG("Refreshed memory debug data: {} total memories", m_memory_debug_data.total_memories);
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to refresh memory debug data: {}", e.what());
        m_memory_debug_data.event_log.push_back({
            .timestamp = format_current_time(),
            .message = std::format("刷新失败: {}", e.what()),
            .color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f)
        });
    }
#endif
}

void InfoPanelView::export_memory_debug_stats() {
    // 导出统计数据到 JSON 文件
    nlohmann::json export_data;
    export_data["total_memories"] = m_memory_debug_data.total_memories;

    nlohmann::json type_counts;
    for (const auto& type_count : m_memory_debug_data.memory_type_counts) {
        type_counts[type_count.name] = type_count.count;
    }
    export_data["memory_type_counts"] = type_counts;

    nlohmann::json event_stats_json;
    for (const auto& [event_name, stats] : m_memory_debug_data.event_stats) {
        event_stats_json[event_name] = {
            {"count", stats.count}
        };
    }
    export_data["event_stats"] = event_stats_json;

    std::string export_path = "memory_debug_stats.json";
    try {
        std::ofstream out(export_path);
        out << export_data.dump(2);
        out.close();

        LOG_INFO("Exported memory debug stats to {}", export_path);
        m_memory_debug_data.event_log.push_back({
            .timestamp = format_current_time(),
            .message = std::format("统计数据已导出到 {}", export_path),
            .color = ImVec4(0.3f, 1.0f, 0.3f, 1.0f)
        });
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to export memory debug stats: {}", e.what());
        m_memory_debug_data.event_log.push_back({
            .timestamp = format_current_time(),
            .message = std::format("导出失败: {}", e.what()),
            .color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f)
        });
    }
}

std::string InfoPanelView::format_current_time() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_s(&tm, &time_t);
    char buffer[64];
    std::strftime(buffer, sizeof(buffer), "%H:%M:%S", &tm);
    return std::string(buffer);
}

} // namespace DearTs::Plugins::Chat
