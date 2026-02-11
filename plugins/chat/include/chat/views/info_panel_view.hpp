/**
 * @file info_panel_view.hpp
 * @brief 信息面板视图（右侧）
 */

#pragma once

#include "core/ui/view.h"
#include "core/ui/icon_font.hpp"
#include "core/event/event_bus.h"
#include "core/config/config_manager.h"
#include "chat/models/conversation.hpp"
#include <memory>
#include <chrono>

namespace DearTs::Plugins::Chat {

using Core::ContentRegistry::UnlocalizedString;
using Core::UI::ViewWindow;

/**
 * @brief LLM 供应商预设配置
 */
struct LLMProviderConfig {
    std::string id;              // "openai", "deepseek", "qwen", "zhipu", "zai", "ollama"
    std::string display_name;    // "OpenAI", "DeepSeek (深度求索)", "通义千问 (阿里云)", etc.
    std::string default_base_url; // 默认 API 地址
    bool requires_api_key;       // 是否需要 API 密钥
    std::string default_model;   // 默认模型
    bool is_chinese_provider;    // 中国供应商标识
};

/**
 * @brief 所有预设 LLM 供应商配置
 */
inline const std::vector<LLMProviderConfig> PRESET_LLM_PROVIDERS = {
    {
        .id = "openai",
        .display_name = "OpenAI",
        .default_base_url = "https://api.openai.com/v1",
        .requires_api_key = true,
        .default_model = "gpt-4o",
        .is_chinese_provider = false
    },
    {
        .id = "deepseek",
        .display_name = "DeepSeek (深度求索)",
        .default_base_url = "https://api.deepseek.com/v1",
        .requires_api_key = true,
        .default_model = "deepseek-chat",
        .is_chinese_provider = true
    },
    {
        .id = "qwen",
        .display_name = "通义千问 (阿里云)",
        .default_base_url = "https://dashscope.aliyuncs.com/compatible-mode/v1",
        .requires_api_key = true,
        .default_model = "qwen-turbo",
        .is_chinese_provider = true
    },
    {
        .id = "zhipu",
        .display_name = "智谱 AI (GLM)",
        .default_base_url = "https://open.bigmodel.cn/api/paas/v4",
        .requires_api_key = true,
        .default_model = "glm-4",
        .is_chinese_provider = true
    },
    {
        .id = "zai",
        .display_name = "Z.AI",
        .default_base_url = "https://api.z.ai/v1",
        .requires_api_key = true,
        .default_model = "zai-gpt",
        .is_chinese_provider = true
    },
    {
        .id = "llmstudio",
        .display_name = "LLM Studio (本地)",
        .default_base_url = "http://localhost:1234/v1",
        .requires_api_key = false,
        .default_model = "qwen/qwen3-4b",
        .is_chinese_provider = false
    },
    {
        .id = "ollama",
        .display_name = "Ollama (本地)",
        .default_base_url = "http://localhost:11434",
        .requires_api_key = false,
        .default_model = "llama3.2",
        .is_chinese_provider = false
    }
};

/**
 * @brief 信息面板视图
 * @details 显示 AI 设置、会话信息、导出选项等
 */
class InfoPanelView : public ViewWindow {
public:
    explicit InfoPanelView(std::shared_ptr<ConversationManager> manager);
    ~InfoPanelView() override;

    void draw_content() override;
    ImVec2 get_min_size() const override { return ImVec2(300, 400); }

private:
    /**
     * @brief 设置事件监听器
     */
    void setup_event_listeners();
    /**
     * @brief 绘制 AI 设置部分
     */
    void draw_ai_settings();

    /**
     * @brief 绘制 LLM 提供商选择器
     */
    void draw_llm_provider_selector();

    /**
     * @brief 绘制 Ollama 设置
     */
    void draw_ollama_settings();

    /**
     * @brief 绘制 LLM Studio 设置
     */
    void draw_llm_studio_settings();

    /**
     * @brief 刷新 Ollama 模型列表
     */
    void refresh_ollama_models();

    /**
     * @brief 刷新 LLM Studio 模型列表
     */
    void refresh_llm_studio_models();

    /**
     * @brief 设置可用模型列表
     */
    void set_available_models(const std::vector<std::string>& models);

    /**
     * @brief 设置 Ollama 连接状态
     */
    void set_ollama_connection_status(bool connected, const std::string& error = "");

    /**
     * @brief 绘制模型设置
     */
    void draw_model_settings();

    /**
     * @brief 绘制会话信息部分
     */
    void draw_conversation_info();

    /**
     * @brief 绘制导出部分
     */
    void draw_export_section();

    /**
     * @brief 绘制调试部分（缓存统计）
     */
    void draw_debug_section();

    /**
     * @brief 绘制测试消息部分
     */
    void draw_test_messages_section();

    /**
     * @brief 添加测试消息
     */
    void add_test_message(const std::string& content, MessageRole role);

    /**
     * @brief 切换 LLM 提供商
     */
    void change_llm_provider(const std::string& provider);

    /**
     * @brief 切换模型
     */
    void change_model(const std::string& model);

    /**
     * @brief 导出会话
     */
    void export_conversation(const std::string& format);

    /**
     * @brief 保存配置到 ConfigManager
     */
    void save_config();

    /**
     * @brief 从 ConfigManager 加载配置
     */
    void load_config();

    /**
     * @brief 获取当前供应商配置
     */
    [[nodiscard]] const LLMProviderConfig* get_current_provider_config() const;

    // ========== Memory Debug UI ==========

    /**
     * @brief Memory Debug 数据结构
     */
    struct MemoryDebugData {
        // 记忆统计
        size_t total_memories = 0;
        struct TypeCount {
            size_t count;
            std::string name;
        };
        std::vector<TypeCount> memory_type_counts;

        // RAG 查询结果
        struct RAGResultItem {
            std::string content;
            std::string source_conversation_id;
            double similarity;
            std::string memory_type;
            int64_t timestamp;
        };
        std::vector<RAGResultItem> last_query_results;
        std::string last_query;

        // 事件日志
        struct EventLogEntry {
            std::string timestamp;
            std::string message;
            ImVec4 color;
        };
        std::vector<EventLogEntry> event_log;
        static constexpr size_t MAX_LOG_ENTRIES = 100;

        // 事件统计
        struct EventStats {
            size_t count = 0;
            std::chrono::system_clock::time_point last_triggered;
        };
        std::unordered_map<std::string, EventStats> event_stats;

        // UI 状态
        int max_results = 5;
        double min_similarity = 0.5;
        bool auto_refresh = true;
    };

    /**
     * @brief 绘制 Memory Debug 选项卡
     */
    void draw_memory_debug();

    /**
     * @brief 绘制记忆统计区域
     */
    void draw_memory_stats_section();

    /**
     * @brief 绘制数据库状态区域
     */
    void draw_database_status_section();

    /**
     * @brief 绘制 RAG 查询区域
     */
    void draw_rag_query_section();

    /**
     * @brief 绘制事件监控区域
     */
    void draw_event_monitor_section();

    /**
     * @brief 绘制一致性管理区域
     */
    void draw_consistency_section();

    /**
     * @brief 刷新 Memory Debug 数据
     */
    void refresh_memory_debug_data();

    /**
     * @brief 导出 Memory Debug 统计
     */
    void export_memory_debug_stats();

    /**
     * @brief 格式化当前时间
     */
    std::string format_current_time() const;

    // 成员变量
    std::shared_ptr<ConversationManager> m_conversation_manager;

    // 配置管理（使用 ConfigScope 自动添加 "chat." 前缀）
    Core::Config::ConfigScope m_config{"chat"};
    std::chrono::steady_clock::time_point m_last_config_save = std::chrono::steady_clock::now();

    // Memory Debug 数据
    MemoryDebugData m_memory_debug_data;
    std::chrono::steady_clock::time_point m_last_refresh = std::chrono::steady_clock::now();

    // LLM 设置
    std::string m_selected_provider_id = "ollama";  // 当前选中的供应商 ID
    std::string m_selected_model = "llama3.2";
    std::vector<std::string> m_available_models = {"llama3.2"};

    // LLM 连接配置
    std::string m_custom_base_url = "";  // 用户自定义的 base URL（空则使用默认值）
    std::string m_api_key = "";          // API 密钥

    // Ollama 设置（保留用于向后兼容）
    std::string m_ollama_base_url = "http://localhost:11434";
    bool m_ollama_connected = false;
    std::string m_ollama_connection_error;
    bool m_ollama_refreshing = false;

    // LLM Studio 设置
    std::string m_llm_studio_base_url = "http://localhost:8080/v1";
    bool m_llm_studio_connected = false;
    std::string m_llm_studio_connection_error;
    bool m_llm_studio_refreshing = false;

    // 参数设置
    float m_temperature = 0.7f;
    int m_max_tokens = 2048;

    // 导出格式
    std::string m_export_format = "json";
    std::string m_export_path = "";

    // UI 状态
    bool m_show_advanced = false;

    // 事件订阅 Token（RAII 自动清理）
    std::vector<DearTs::Core::Event::EventToken> m_event_tokens;
};

} // namespace DearTs::Plugins::Chat
