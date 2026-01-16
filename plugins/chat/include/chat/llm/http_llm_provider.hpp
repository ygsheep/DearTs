/**
 * @file http_llm_provider.hpp
 * @brief HTTP LLM 提供商实现
 */

#pragma once

#include "chat/llm/llm_interface.hpp"
#include <string>

namespace DearTs::Plugins::Chat::LLM {

/**
 * @brief HTTP LLM 提供商
 * @details 通过 HTTP API 调用兼容 OpenAI 的 LLM 服务（如 Ollama、LocalAI）
 */
class HTTPLLMProvider : public ILLMProvider {
public:
    /**
     * @brief 构造函数
     * @param base_url API 基础 URL
     * @param api_key API 密钥（可选）
     * @param model 模型名称
     */
    explicit HTTPLLMProvider(
        const std::string& base_url = "http://localhost:11434/v1",
        const std::string& api_key = "",
        const std::string& model = "llama3.2"
    );

    ~HTTPLLMProvider() override = default;

    // ILLMProvider 接口实现
    [[nodiscard]] std::string get_name() const override {
        return "HTTP";
    }

    [[nodiscard]] bool is_available() const override;

    [[nodiscard]] std::shared_ptr<Core::Tasks::Task> send_async(
        const LLMRequest& request,
        std::function<void(const LLMResponse&)> callback
    ) override;

    [[nodiscard]] DearTs::Core::Result<LLMResponse, std::string> send(const LLMRequest& request) override;

    [[nodiscard]] std::vector<std::string> get_models() const override;

    /**
     * @brief 设置基础 URL
     */
    void set_base_url(const std::string& base_url) override {
        m_base_url = base_url;
    }

    /**
     * @brief 设置 API 密钥
     */
    void set_api_key(const std::string& api_key) override {
        m_api_key = api_key;
    }

    /**
     * @brief 获取基础 URL
     */
    [[nodiscard]] const std::string& get_base_url() const {
        return m_base_url;
    }

private:
    /**
     * @brief 发送 HTTP POST 请求
     */
    [[nodiscard]] DearTs::Core::Result<std::string, std::string> send_http_request(
        const std::string& endpoint,
        const std::string& json_body
    ) const;

    /**
     * @brief 构建聊天完成请求的 JSON
     */
    [[nodiscard]] std::string build_chat_completion_request(const LLMRequest& request) const;

    /**
     * @brief 解析聊天完成响应
     */
    [[nodiscard]] DearTs::Core::Result<LLMResponse, std::string> parse_chat_completion_response(
        const std::string& json_body
    ) const;

    /**
     * @brief 测试连接
     */
    [[nodiscard]] bool test_connection() const;

    std::string m_base_url;
    std::string m_api_key;
};

} // namespace DearTs::Plugins::Chat::LLM
