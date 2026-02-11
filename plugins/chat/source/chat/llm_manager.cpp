/**
 * @file llm_manager.cpp
 * @brief LLM 管理器实现
 */

#include "chat/llm/llm_interface.hpp"
#include "chat/llm/http_llm_provider.hpp"
#include "chat/llm/ollama_llm_provider.hpp"
#include "chat/llm/python_llm_provider.hpp"
#include "chat/llm/cli_llm_provider.hpp"
#include "liblogger/logger.h"
#include <memory>

namespace DearTs::Plugins::Chat::LLM {

// LLMProviderFactory 实现
std::unique_ptr<ILLMProvider> LLMProviderFactory::create_http_provider(
    const std::string& base_url,
    const std::string& api_key,
    const std::string& model
) {
    return std::make_unique<HTTPLLMProvider>(base_url, api_key, model);
}

std::unique_ptr<ILLMProvider> LLMProviderFactory::create_python_provider(
    const std::string& script_path,
    const std::string& python_path
) {
    return std::make_unique<PythonLLMProvider>(script_path, python_path);
}

std::unique_ptr<ILLMProvider> LLMProviderFactory::create_cli_provider(
    const std::string& command_template
) {
    return std::make_unique<CLLILLMProvider>(command_template);
}

std::unique_ptr<ILLMProvider> LLMProviderFactory::create_ollama_provider(
    const std::string& base_url,
    const std::string& model
) {
    return std::make_unique<OllamaLLMProvider>(base_url, model);
}

std::unique_ptr<ILLMProvider> LLMProviderFactory::create_llm_studio_provider(
    const std::string& base_url,
    const std::string& model
) {
    // LLM Studio 使用 OpenAI 兼容 API，复用 HTTPLLMProvider
    return std::make_unique<HTTPLLMProvider>(base_url, "", model);
}

// LLMManager 实现
LLMManager& LLMManager::instance() {
    static LLMManager instance;
    return instance;
}

void LLMManager::set_provider(std::unique_ptr<ILLMProvider> provider) {
    m_provider = std::move(provider);
    LOG_INFO("LLM provider set to: {}", m_provider ? m_provider->get_name() : "None");
}

std::shared_ptr<Core::Tasks::Task> LLMManager::send_async(
    const LLMRequest& request,
    std::function<void(const LLMResponse&)> callback
) {
    if (!m_provider) {
        // 如果没有设置提供商，返回失败任务
        auto task = Core::Tasks::TaskManager::instance().launch(
            "LLM Error",
            [callback](const auto&) {
                callback(LLMResponse::failure("No LLM provider configured"));
            },
            Core::Tasks::TaskType::Normal
        );
        return task;
    }

    return m_provider->send_async(request, callback);
}

DearTs::Core::Result<LLMResponse, std::string> LLMManager::send(const LLMRequest& request) {
    if (!m_provider) {
        return DearTs::Core::Result<LLMResponse, std::string>::err("No LLM provider configured");
    }

    return m_provider->send(request);
}

} // namespace DearTs::Plugins::Chat::LLM
