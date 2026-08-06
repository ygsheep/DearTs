/**
 * @file cli_llm_provider.hpp
 * @brief CLI LLM 提供商实现
 */

#pragma once

#include "chat/llm/llm_interface.hpp"
#include <string>

namespace DearTs::Plugins::Chat::LLM {

/**
 * @brief CLI LLM 提供商
 * @details 通过调用命令行工具使用 LLM（如 ollama、llama.cpp）
 */
class CLLILLMProvider : public ILLMProvider {
public:
    /**
     * @brief 构造函数
     * @param command_template 命令模板，使用 {prompt}、{model}、{temperature} 等占位符
     *
     * @example
     * // Ollama
     * CLLILLMProvider("ollama run {model} {prompt}");
     *
     * // llama.cpp
     * CLLILLMProvider("./main -m {model} -p \"{prompt}\"");
     */
    explicit CLLILLMProvider(const std::string& command_template);

    ~CLLILLMProvider() override = default;

    // ILLMProvider 接口实现
    [[nodiscard]] std::string get_name() const override {
        return "CLI";
    }

    [[nodiscard]] bool is_available() const override;

    [[nodiscard]] std::shared_ptr<Core::Tasks::Task> send_async(
        const LLMRequest& request,
        std::function<void(const LLMResponse&)> callback
    ) override;

    [[nodiscard]] DearTs::Core::Result<LLMResponse, std::string> send(const LLMRequest& request) override;

    [[nodiscard]] std::vector<std::string> get_models() const override;

    /**
     * @brief 设置命令模板
     */
    void set_command_template(const std::string& command_template) {
        m_command_template = command_template;
    }

private:
    /**
     * @brief 替换命令模板中的占位符
     */
    [[nodiscard]] std::string build_command(const LLMRequest& request) const;

    /**
     * @brief 执行命令
     */
    [[nodiscard]] DearTs::Core::Result<std::string, std::string> execute_command(
        const std::string& command
    ) const;

    std::string m_command_template;
};

} // namespace DearTs::Plugins::Chat::LLM
