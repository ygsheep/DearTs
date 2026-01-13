/**
 * @file python_llm_provider.hpp
 * @brief Python LLM 提供商实现
 */

#pragma once

#include "chat/llm/llm_interface.hpp"
#include <string>

namespace DearTs::Plugins::Chat::LLM {

/**
 * @brief Python LLM 提供商
 * @details 通过调用 Python 脚本使用本地 LLM
 */
class PythonLLMProvider : public ILLMProvider {
public:
    /**
     * @brief 构造函数
     * @param script_path Python 脚本路径
     * @param python_path Python 解释器路径
     */
    explicit PythonLLMProvider(
        const std::string& script_path = "scripts/chat_llm.py",
        const std::string& python_path = "python"
    );

    ~PythonLLMProvider() override = default;

    // ILLMProvider 接口实现
    [[nodiscard]] std::string get_name() const override {
        return "Python";
    }

    [[nodiscard]] bool is_available() const override;

    [[nodiscard]] std::shared_ptr<Core::Tasks::Task> send_async(
        const LLMRequest& request,
        std::function<void(const LLMResponse&)> callback
    ) override;

    [[nodiscard]] Result<LLMResponse, std::string> send(const LLMRequest& request) override;

    [[nodiscard]] std::vector<std::string> get_models() const override;

    /**
     * @brief 设置脚本路径
     */
    void set_script_path(const std::string& script_path) {
        m_script_path = script_path;
    }

    /**
     * @brief 设置 Python 解释器路径
     */
    void set_python_path(const std::string& python_path) {
        m_python_path = python_path;
    }

private:
    /**
     * @brief 执行 Python 脚本
     */
    [[nodiscard]] Result<std::string, std::string> execute_python_script(
        const std::string& json_input
    ) const;

    std::string m_script_path;
    std::string m_python_path;
};

} // namespace DearTs::Plugins::Chat::LLM
