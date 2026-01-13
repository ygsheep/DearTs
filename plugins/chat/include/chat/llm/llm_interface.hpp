/**
 * @file llm_interface.hpp
 * @brief LLM 提供商抽象接口
 */

#pragma once

#include "chat/models/ai_suggestion.hpp"
#include "core/result.h"
#include "core/tasks/task_manager.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace DearTs::Plugins::Chat::LLM {

/**
 * @brief LLM 请求结构
 */
struct LLMRequest {
    std::string prompt;                           // 主提示词
    std::vector<std::string> context;             // 上下文消息列表
    std::string system_prompt;                    // 系统提示词

    // 生成参数
    float temperature = 0.7f;
    int max_tokens = 2048;
    float top_p = 1.0f;
    std::string stop_sequence;

    // 是否流式响应
    bool stream = false;

    // 流式响应回调（当 stream=true 时使用）
    std::function<void(const std::string&)> on_chunk;
};

/**
 * @brief LLM 响应结构
 */
struct LLMResponse {
    std::string content;                          // 生成的内容
    bool is_complete = true;                      // 是否完整响应
    std::string error;                            // 错误信息（如果失败）

    // 元数据
    std::string model;                            // 使用的模型
    int tokens_used = 0;                          // 使用的 token 数
    std::chrono::milliseconds duration{0};        // 处理耗时

    /**
     * @brief 判断响应是否成功
     */
    [[nodiscard]] bool is_success() const {
        return is_complete && error.empty();
    }

    /**
     * @brief 创建成功响应
     */
    [[nodiscard]] static LLMResponse success(std::string content) {
        LLMResponse response;
        response.content = std::move(content);
        response.is_complete = true;
        return response;
    }

    /**
     * @brief 创建失败响应
     */
    [[nodiscard]] static LLMResponse failure(std::string error) {
        LLMResponse response;
        response.error = std::move(error);
        response.is_complete = false;
        return response;
    }
};

/**
 * @brief LLM 提供商信息
 */
struct LLMProviderInfo {
    std::string name;                             // 提供商名称
    std::string description;                      // 描述
    bool is_available = false;                    // 是否可用
    std::vector<std::string> available_models;    // 可用模型列表
};

/**
 * @brief LLM 提供商抽象接口
 * @details 所有 LLM 提供商（HTTP、Python、CLI 等）都需要实现此接口
 */
class ILLMProvider {
public:
    virtual ~ILLMProvider() = default;

    /**
     * @brief 获取提供商名称
     */
    [[nodiscard]] virtual std::string get_name() const = 0;

    /**
     * @brief 获取提供商信息
     */
    [[nodiscard]] virtual LLMProviderInfo get_info() const {
        LLMProviderInfo info;
        info.name = get_name();
        info.is_available = is_available();
        info.available_models = get_models();
        return info;
    }

    /**
     * @brief 检查提供商是否可用
     */
    [[nodiscard]] virtual bool is_available() const = 0;

    /**
     * @brief 发送异步请求
     * @param request LLM 请求
     * @param callback 响应回调函数
     * @return Task 可用于跟踪进度和取消
     */
    [[nodiscard]] virtual std::shared_ptr<Core::Tasks::Task> send_async(
        const LLMRequest& request,
        std::function<void(const LLMResponse&)> callback
    ) = 0;

    /**
     * @brief 发送同步请求
     * @param request LLM 请求
     * @return Result 包含响应或错误信息
     */
    [[nodiscard]] virtual Result<LLMResponse, std::string> send(
        const LLMRequest& request
    ) = 0;

    /**
     * @brief 获取可用模型列表
     */
    [[nodiscard]] virtual std::vector<std::string> get_models() const = 0;

    /**
     * @brief 设置当前模型
     */
    virtual void set_model(const std::string& model) {
        m_current_model = model;
    }

    /**
     * @brief 获取当前模型
     */
    [[nodiscard]] virtual const std::string& get_model() const {
        return m_current_model;
    }

    /**
     * @brief 设置 API 密钥（如果需要）
     */
    virtual void set_api_key(const std::string& api_key) {
        m_api_key = api_key;
    }

    /**
     * @brief 设置基础 URL（用于 HTTP 提供商）
     */
    virtual void set_base_url(const std::string& base_url) {
        m_base_url = base_url;
    }

protected:
    std::string m_current_model;
    std::string m_api_key;
    std::string m_base_url;
};

/**
 * @brief LLM 提供商工厂
 * @details 用于创建不同类型的 LLM 提供商
 */
class LLMProviderFactory {
public:
    /**
     * @brief 创建 HTTP 提供商
     */
    [[nodiscard]] static std::unique_ptr<ILLMProvider> create_http_provider(
        const std::string& base_url,
        const std::string& api_key = "",
        const std::string& model = "gpt-3.5-turbo"
    );

    /**
     * @brief 创建 Python 提供商
     */
    [[nodiscard]] static std::unique_ptr<ILLMProvider> create_python_provider(
        const std::string& script_path,
        const std::string& python_path = "python"
    );

    /**
     * @brief 创建 CLI 提供商
     */
    [[nodiscard]] static std::unique_ptr<ILLMProvider> create_cli_provider(
        const std::string& command_template
    );
};

/**
 * @brief LLM 管理器
 * @details 管理当前的 LLM 提供商和处理请求
 */
class LLMManager {
public:
    /**
     * @brief 获取单例
     */
    [[nodiscard]] static LLMManager& instance();

    /**
     * @brief 设置当前提供商
     */
    void set_provider(std::unique_ptr<ILLMProvider> provider);

    /**
     * @brief 获取当前提供商
     */
    [[nodiscard]] ILLMProvider* get_provider() const { return m_provider.get(); }

    /**
     * @brief 发送异步请求
     */
    [[nodiscard]] std::shared_ptr<Core::Tasks::Task> send_async(
        const LLMRequest& request,
        std::function<void(const LLMResponse&)> callback
    );

    /**
     * @brief 发送同步请求
     */
    [[nodiscard]] Result<LLMResponse, std::string> send(const LLMRequest& request);

private:
    LLMManager() = default;

    std::unique_ptr<ILLMProvider> m_provider;
};

} // namespace DearTs::Plugins::Chat::LLM
