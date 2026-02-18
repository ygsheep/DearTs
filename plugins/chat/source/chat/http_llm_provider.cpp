/**
 * @file http_llm_provider.cpp
 * @brief HTTP LLM 提供商实现
 */

#include "chat/llm/http_llm_provider.hpp"
#include "core/network/http_client.hpp"
#include "core/network/http_types.hpp"
#include "core/network/sse_parser.hpp"
#include "liblogger/logger.h"
#include <nlohmann/json.hpp>
#include <format>
#include <fstream>
#include <sstream>
#include <future>
#include <thread>

namespace DearTs::Plugins::Chat::LLM {

using json = nlohmann::json;

HTTPLLMProvider::HTTPLLMProvider(
    const std::string& base_url,
    const std::string& api_key,
    const std::string& model
) : m_base_url(base_url)
  , m_api_key(api_key) {
    m_current_model = model;
}

bool HTTPLLMProvider::is_available() const {
    return test_connection();
}

std::shared_ptr<Core::Tasks::Task> HTTPLLMProvider::send_async(
    const LLMRequest& request,
    std::function<void(const LLMResponse&)> callback
) {
    auto task = Core::Tasks::TaskManager::instance().launch(
        "LLM HTTP Request",
        [this, request, callback](const auto& cancel) {
            // 在后台线程发送请求
            auto result = this->send(request);

            if (cancel) {
                return;
            }

            // 调用回调（检查结果状态）
            if (result.isOk()) {
                callback(result.unwrap());
            } else {
                // 创建失败响应
                LLMResponse error_response;
                error_response.is_complete = false;
                error_response.error = result.error();
                callback(error_response);
            }
        },
        Core::Tasks::TaskType::Background
    );

    return task;
}

DearTs::Core::Result<LLMResponse, std::string> HTTPLLMProvider::send(const LLMRequest& request) {
    const auto start_time = std::chrono::steady_clock::now();

    try {
        // 如果启用流式输出，使用流式处理
        if (request.stream && request.on_chunk) {
            LOG_INFO("HTTPLLM: Using streaming mode");
            std::string full_content;

            auto result = send_streaming_request(request, [&](const std::string& content) {
                full_content += content;
                request.on_chunk(content);
            });

            if (!result.isOk()) {
                return DearTs::Core::Result<LLMResponse, std::string>::err(result.error());
            }

            LLMResponse response;
            response.content = full_content;
            response.is_complete = true;

            const auto end_time = std::chrono::steady_clock::now();
            response.duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                end_time - start_time
            );

            return DearTs::Core::Result<LLMResponse, std::string>::ok(response);
        }

        // 非流式：发送常规 HTTP 请求
        const std::string request_body = build_chat_completion_request(request);

        auto response_body = send_http_request("/v1/chat/completions", request_body);
        if (!response_body.isOk()) {
            return DearTs::Core::Result<LLMResponse, std::string>::err(response_body.error());
        }

        auto response = parse_chat_completion_response(response_body.unwrap());
        if (!response.isOk()) {
            return DearTs::Core::Result<LLMResponse, std::string>::err(response.error());
        }

        auto llm_response = response.unwrap();

        const auto end_time = std::chrono::steady_clock::now();
        llm_response.duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time
        );

        return DearTs::Core::Result<LLMResponse, std::string>::ok(llm_response);

    } catch (const std::exception& e) {
        return DearTs::Core::Result<LLMResponse, std::string>::err(std::format("HTTP request failed: {}", e.what()));
    }
}

std::vector<std::string> HTTPLLMProvider::get_models() const {
    // 尝试从 /v1/models 端点获取可用模型（OpenAI 兼容 API）
    try {
        auto response = send_http_request("/v1/models", "");
        if (response.isOk()) {
            json j = json::parse(response.unwrap());
            if (j.contains("data") && j["data"].is_array()) {
                std::vector<std::string> models;
                for (const auto& item : j["data"]) {
                    if (item.contains("id")) {
                        models.push_back(item["id"].get<std::string>());
                    }
                }
                return models;
            }
        }
    } catch (...) {
        // 忽略错误，返回默认模型列表
    }

    // 返回默认模型列表
    return {"llama3.2", "qwen2.5", "deepseek-r1", "gpt-4", "gpt-3.5-turbo"};
}

DearTs::Core::Result<std::string, std::string> HTTPLLMProvider::send_http_request(
    const std::string& endpoint,
    const std::string& json_body
) const {
    using namespace DearTs::Core::Network;

    // 配置 HTTP 客户端
    HttpClientConfig config;
    config.connect_timeout = std::chrono::seconds(10);
    config.request_timeout = std::chrono::seconds(60);  // OpenAI API 可能较慢

    // 创建 HTTP 客户端
    BoostAsioHttpClient client(m_base_url, config);

    // 构建请求
    HttpRequest request;
    request.method = "POST";
    request.endpoint = endpoint;
    request.headers["Content-Type"] = "application/json";

    // Bearer Token 认证（用于 OpenAI 兼容 API）
    if (!m_api_key.empty()) {
        request.headers["Authorization"] = "Bearer " + m_api_key;
    }

    request.body = json_body;

    // 发送请求
    auto response_result = client.request(request);

    if (response_result.isErr()) {
        return DearTs::Core::Result<std::string, std::string>::err(response_result.error());
    }

    auto response = response_result.unwrap();

    // 检查 HTTP 状态码
    if (response.status_code != 200) {
        std::string error_detail;

        // 尝试从响应体中提取错误信息
        if (!response.body.empty()) {
            try {
                json j = json::parse(response.body);
                // OpenAI 格式错误: {"error": {"message": "...", "type": "...", "code": "..."}}
                if (j.contains("error")) {
                    auto& err = j["error"];
                    if (err.contains("message")) {
                        error_detail = err["message"].get<std::string>();
                    } else if (err.is_string()) {
                        error_detail = err.get<std::string>();
                    } else {
                        error_detail = err.dump();
                    }
                } else {
                    error_detail = response.body;
                }
            } catch (...) {
                error_detail = response.body;
            }
        }

        std::string error_msg = response.error_message.empty()
            ? std::format("HTTP {}", response.status_code)
            : std::format("HTTP {}: {}", response.status_code, response.error_message);

        if (!error_detail.empty()) {
            error_msg += std::format(" - {}", error_detail);
        }

        LOG_ERROR("HTTP LLM: request failed: {}", error_msg);
        return DearTs::Core::Result<std::string, std::string>::err(error_msg);
    }

    return DearTs::Core::Result<std::string, std::string>::ok(response.body);
}

std::string HTTPLLMProvider::build_chat_completion_request(const LLMRequest& request) const {
    json j;

    j["model"] = m_current_model;
    j["temperature"] = request.temperature;
    j["max_tokens"] = request.max_tokens;

    // 构建消息列表
    json messages = json::array();

    // 系统提示词
    if (!request.system_prompt.empty()) {
        messages.push_back({
            {"role", "system"},
            {"content", request.system_prompt}
        });
    }

    // 上下文消息
    for (const auto& msg : request.context) {
        messages.push_back({
            {"role", "user"},
            {"content", msg}
        });
    }

    // 当前提示词
    messages.push_back({
        {"role", "user"},
        {"content", request.prompt}
    });

    j["messages"] = messages;

    // 流式响应
    j["stream"] = request.stream;

    return j.dump(2);
}

DearTs::Core::Result<LLMResponse, std::string> HTTPLLMProvider::parse_chat_completion_response(
    const std::string& json_body
) const {
    try {
        json j = json::parse(json_body);

        // 检查错误
        if (j.contains("error")) {
            std::string error_msg = j["error"];
            return DearTs::Core::Result<LLMResponse, std::string>::err(error_msg);
        }

        // 提取内容
        if (j.contains("choices") && j["choices"].is_array() && !j["choices"].empty()) {
            const auto& choice = j["choices"][0];

            if (choice.contains("message") && choice["message"].contains("content")) {
                LLMResponse response;
                response.content = choice["message"]["content"].get<std::string>();
                response.is_complete = true;

                // 提取使用量信息
                if (j.contains("usage")) {
                    response.tokens_used = j["usage"]["total_tokens"].get<int>();
                }

                return DearTs::Core::Result<LLMResponse, std::string>::ok(response);
            }
        }

        return DearTs::Core::Result<LLMResponse, std::string>::err("Invalid response format");

    } catch (const json::exception& e) {
        return DearTs::Core::Result<LLMResponse, std::string>::err(std::format("JSON parse error: {}", e.what()));
    }
}

bool HTTPLLMProvider::test_connection() const {
    try {
        // 发送一个简单的请求测试连接（OpenAI 兼容 API）
        auto response = send_http_request("/v1/models", "");
        return response.isOk();
    } catch (...) {
        return false;
    }
}

DearTs::Core::Result<void, std::string> HTTPLLMProvider::send_streaming_request(
    const LLMRequest& request,
    const std::function<void(const std::string&)>& on_chunk
) const {
    using namespace DearTs::Core::Network;

    try {
        LOG_INFO("HTTPLLM: Starting SSE streaming request to {}", m_base_url);
        const std::string request_body = build_chat_completion_request(request);

        // 创建 SSE 解析器
        SSEParser sse_parser([&](const SSEEvent& event) {
            std::string content = parse_sse_data(event.data);
            if (!content.empty() && on_chunk) {
                on_chunk(content);
            }
        });

        // 配置 HTTP 客户端
        HttpClientConfig config;
        config.connect_timeout = std::chrono::seconds(10);
        config.request_timeout = std::chrono::seconds(60);

        // 创建 HTTP 客户端
        BoostAsioHttpClient client(m_base_url, config);

        // 构建请求
        HttpRequest http_req;
        http_req.method = "POST";
        http_req.endpoint = "/v1/chat/completions";
        http_req.headers["Content-Type"] = "application/json";

        if (!m_api_key.empty()) {
            http_req.headers["Authorization"] = "Bearer " + m_api_key;
        }

        http_req.body = request_body;

        // 设置流式回调
        http_req.on_chunk = [&](const std::string& chunk) {
            sse_parser.parse(chunk);
        };

        // 发送请求
        auto response_result = client.request(http_req);

        if (response_result.isErr()) {
            return DearTs::Core::Result<void, std::string>::err(response_result.error());
        }

        auto response = response_result.unwrap();

        if (response.status_code != 200) {
            std::string error_msg = response.error_message.empty()
                ? std::format("HTTP {}", response.status_code)
                : std::format("HTTP {}: {}", response.status_code, response.error_message);
            LOG_ERROR("HTTPLLM: Streaming request failed: {}", error_msg);
            return DearTs::Core::Result<void, std::string>::err(error_msg);
        }

        LOG_INFO("HTTPLLM: SSE streaming completed, {} events processed", sse_parser.get_event_count());
        return DearTs::Core::Result<void, std::string>::ok();

    } catch (const std::exception& e) {
        return DearTs::Core::Result<void, std::string>::err(
            std::format("SSE streaming request failed: {}", e.what())
        );
    }
}

std::string HTTPLLMProvider::parse_sse_data(const std::string& data) {
    try {
        // SSE 的 data 字段可能包含多行 JSON
        // OpenAI 格式: data: {"choices": [...]}
        // 需要提取 content 字段

        json j = json::parse(data);

        // 检查 choices 数组
        if (j.contains("choices") && j["choices"].is_array() && !j["choices"].empty()) {
            const auto& choice = j["choices"][0];

            // 检查 delta (增量内容)
            if (choice.contains("delta")) {
                const auto& delta = choice["delta"];
                if (delta.contains("content")) {
                    return delta["content"].get<std::string>();
                }
            }

            // 检查完整的 message
            if (choice.contains("message") && choice["message"].contains("content")) {
                return choice["message"]["content"].get<std::string>();
            }
        }

        return "";

    } catch (const json::exception&) {
        // 忽略解析错误的数据
        return "";
    }
}

} // namespace DearTs::Plugins::Chat::LLM
