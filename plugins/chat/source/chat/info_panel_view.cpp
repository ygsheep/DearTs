/**
 * @file info_panel_view.cpp
 * @brief 信息面板视图实现
 */

#include "chat/views/info_panel_view.hpp"
#include "chat/events/chat_events.hpp"
#include "chat/ui/measure_context.hpp"
#include "core/ui/imgui_extensions.h"
#include "core/ui/icon_font.hpp"
#include "liblogger/logger.h"
#include <imgui.h>
#include <format>

namespace DearTs::Plugins::Chat {

InfoPanelView::InfoPanelView(std::shared_ptr<ConversationManager> manager)
    : ViewWindow(UnlocalizedString("信息"), ICON_INFO)
    , m_conversation_manager(std::move(manager)) {
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
        if (ImGui::SliderFloat("##top_p", &m_temperature, 0.0f, 1.0f, "%.2f")) {
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
    if (ImGui::SliderFloat("##temperature", &m_temperature, 0.0f, 2.0f, "%.2f")) {
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

} // namespace DearTs::Plugins::Chat
