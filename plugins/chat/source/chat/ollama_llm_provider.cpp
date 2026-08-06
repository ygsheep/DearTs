/**
 * @file ollama_llm_provider.cpp
 * @brief Ollama LLM 提供商实现
 */

#include "chat/llm/ollama_llm_provider.hpp"
#include "core/network/http_client.hpp"
#include "core/network/http_types.hpp"
#include "liblogger/logger.h"
#include <nlohmann/json.hpp>
#include <format>
#include <sstream>
#include <future>
#include <thread>
#include <memory>

namespace DearTs::Plugins::Chat::LLM {

using json = nlohmann::json;

// ========== 辅助函数 ==========

/**
 * @brief 获取或创建 HTTP 客户端（线程安全）
 * @details 使用延迟初始化，避免在构造函数中创建 IO 线程
 *          使用双重检查锁定模式优化性能
 *          返回原始指针（调用者应立即使用，不要存储）
 */
[[nodiscard]] DearTs::Core::Network::BoostAsioHttpClient* OllamaLLMProvider::get_client() const {
    // 第一次检查（无锁）- 快速路径
    if (m_client) {
        return m_client.get();
    }

    // 加锁保护初始化
    std::lock_guard<std::mutex> lock(m_client_mutex);

    // 第二次检查（有锁）- 防止重复初始化
    if (!m_client) {
        DearTs::Core::Network::HttpClientConfig config;
        config.connect_timeout = std::chrono::seconds(10);
        config.request_timeout = std::chrono::seconds(60);
        m_client = std::make_unique<DearTs::Core::Network::BoostAsioHttpClient>(m_base_url, config);
        LOG_INFO("OllamaLLMProvider: Created HTTP client for {}", m_base_url);
    }
    return m_client.get();
}

// ========== 辅助函数 ==========

/**
 * @brief 检测模型是否支持聊天格式
 * @return 如果模型名称包含 "chat"、"llama"、"qwen"、"mistral" 等关键词，返回 true
 */
static bool model_supports_chat(const std::string& model) {
    std::string lower_model = model;
    std::transform(lower_model.begin(), lower_model.end(), lower_model.begin(), ::tolower);

    // 这些关键词通常表示支持聊天格式的模型
    std::vector<std::string> chat_keywords = {
        "chat", "llama", "qwen", "mistral", "gemma", "phi",
        "instruct", "turbo", "gpt"
    };

    for (const auto& keyword : chat_keywords) {
        if (lower_model.find(keyword) != std::string::npos) {
            return true;
        }
    }

    // 代码专用模型通常不支持聊天格式
    std::vector<std::string> code_keywords = {
        "coder", "code"
    };

    for (const auto& keyword : code_keywords) {
        if (lower_model.find(keyword) != std::string::npos) {
            return false;
        }
    }

    // 默认支持聊天格式
    return true;
}

/**
 * @brief 判断应该使用哪个 API 端点
 * @return "/api/chat" 或 "/api/generate"
 */
static std::string get_api_endpoint(const std::string& model) {
    if (model_supports_chat(model)) {
        return "/api/chat";
    } else {
        return "/api/generate";
    }
}

// ========== OllamaLLMProvider 实现 ==========

OllamaLLMProvider::OllamaLLMProvider(
    const std::string& base_url,
    const std::string& model
) : m_base_url(base_url) {
    m_current_model = model;
    LOG_INFO("OllamaLLMProvider: Initialized with URL={}, model={}", base_url, model);
}

bool OllamaLLMProvider::is_available() const {
    bool available = test_connection();
    LOG_INFO("OllamaLLMProvider::is_available() = {}", available);
    return available;
}

std::shared_ptr<Core::Tasks::Task> OllamaLLMProvider::send_async(
    const LLMRequest& request,
    std::function<void(const LLMResponse&)> callback
) {
    // 使用 std::thread 在后台执行同步 HTTP 请求
    // 避免复杂的异步生命周期管理问题
    using namespace DearTs::Core::Network;

    // 捕获所有需要的变量
    std::string base_url = m_base_url;
    std::string current_model = m_current_model;
    std::string endpoint = get_api_endpoint(current_model);
    std::string request_body = (endpoint == "/api/chat")
        ? build_chat_request(request)
        : build_generate_request(request);

    bool has_stream = request.stream && request.on_chunk;
    bool is_chat_endpoint = (endpoint == "/api/chat");

    // 在新线程中执行请求
    std::thread([base_url, current_model, endpoint, request_body, has_stream, is_chat_endpoint, request, callback]() {
        HttpClientConfig config;
        config.connect_timeout = std::chrono::seconds(10);
        config.request_timeout = std::chrono::seconds(60);

        // 在线程中创建 HTTP 客户端（局部变量，线程结束时销毁）
        BoostAsioHttpClient client(base_url, config);

        HttpRequest http_req;
        http_req.method = "POST";
        http_req.endpoint = endpoint;
        http_req.headers["Content-Type"] = "application/json";
        http_req.body = request_body;

        if (has_stream) {
            // 流式请求 - 使用 on_chunk 回调
            auto incomplete_line_buffer = std::make_shared<std::string>();
            auto previous_sent_content = std::make_shared<std::string>();

            http_req.on_chunk = [is_chat_endpoint, incomplete_line_buffer,
                                previous_sent_content, request](const std::string& chunk) {
                // 处理 NDJSON 流式数据
                std::string data = *incomplete_line_buffer + chunk;
                incomplete_line_buffer->clear();

                std::istringstream stream(data);
                std::string line;

                while (std::getline(stream, line)) {
                    if (!line.empty()) {
                        std::string content;
                        if (is_chat_endpoint) {
                            // /api/chat：message.content 是累积的，需要去重
                            // 简化处理：直接调用 on_chunk，让调用者处理去重
                            try {
                                auto j = nlohmann::json::parse(line);
                                if (j.contains("message") && j["message"].contains("content")) {
                                    content = j["message"]["content"].get<std::string>();
                                }
                            } catch (...) {
                                content = "";
                            }
                        } else {
                            // /api/generate：response 是增量
                            try {
                                auto j = nlohmann::json::parse(line);
                                if (j.contains("response")) {
                                    content = j["response"].get<std::string>();
                                }
                            } catch (...) {
                                content = "";
                            }
                        }

                        if (!content.empty() && request.on_chunk) {
                            request.on_chunk(content);
                        }
                    }
                }

                size_t last_newline = data.find_last_of('\n');
                if (last_newline != std::string::npos && last_newline < data.length() - 1) {
                    *incomplete_line_buffer = data.substr(last_newline + 1);
                }
            };
        }

        // 发送同步请求（阻塞直到完成）
        auto result = client.request(http_req);

        LLMResponse response;
        if (result.isOk()) {
            response.is_complete = true;
            response.error = "";
        } else {
            response.is_complete = false;
            response.error = result.error();
        }
        callback(response);
    }).detach();  // detach 线程，让它独立运行

    // 返回一个虚拟任务（用于兼容现有接口）
    return nullptr;
}

DearTs::Core::Result<LLMResponse, std::string> OllamaLLMProvider::send(
    const LLMRequest& request
) {
    const auto start_time = std::chrono::steady_clock::now();

    LOG_INFO("OllamaLLMProvider::send() called, stream={}, has_on_chunk={}",
             request.stream, request.on_chunk != nullptr);

    try {
        // 如果启用流式输出，使用流式处理
        if (request.stream && request.on_chunk) {
            LOG_INFO("Ollama: Using streaming mode");
            std::string full_content;

            auto stream_result = send_streaming_request(
                request,
                [&](const std::string& chunk) {
                    full_content += chunk;
                    LOG_DEBUG("Ollama: Received chunk: '{}' (total: {} bytes)",
                              chunk.substr(0, std::min(size_t(50), chunk.length())),
                              full_content.length());
                    if (request.on_chunk) {
                        request.on_chunk(chunk);
                    }
                }
            );

            if (!stream_result.isOk()) {
                LOG_ERROR("Ollama: Streaming request failed: {}", stream_result.error());
                return DearTs::Core::Result<LLMResponse, std::string>::err(stream_result.error());
            }

            // 构建响应
            LLMResponse response;
            response.content = full_content;
            response.is_complete = true;
            response.model = m_current_model;

            const auto end_time = std::chrono::steady_clock::now();
            response.duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                end_time - start_time
            );

            return DearTs::Core::Result<LLMResponse, std::string>::ok(response);
        }

        // 非流式请求 - 根据模型类型选择端点
        const std::string endpoint = get_api_endpoint(m_current_model);
        const std::string request_body = (endpoint == "/api/chat")
            ? build_chat_request(request)
            : build_generate_request(request);

        LOG_INFO("Ollama: Using {} endpoint for model: {}", endpoint, m_current_model);
        auto response_body = send_http_request(endpoint, request_body);

        if (!response_body.isOk()) {
            return DearTs::Core::Result<LLMResponse, std::string>::err(response_body.error());
        }

        // 解析响应
        auto response = parse_response(response_body.unwrap());
        if (!response.isOk()) {
            return DearTs::Core::Result<LLMResponse, std::string>::err(response.error());
        }

        auto llm_response = response.unwrap();
        llm_response.model = m_current_model;

        // 计算耗时
        const auto end_time = std::chrono::steady_clock::now();
        llm_response.duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time
        );

        return DearTs::Core::Result<LLMResponse, std::string>::ok(llm_response);

    } catch (const std::exception& e) {
        return DearTs::Core::Result<LLMResponse, std::string>::err(
            std::format("Ollama request failed: {}", e.what())
        );
    }
}

std::vector<std::string> OllamaLLMProvider::get_models() const {
    try {
        auto response = send_http_request("/api/tags", "");
        if (response.isOk()) {
            json j = json::parse(response.unwrap());
            if (j.contains("models") && j["models"].is_array()) {
                std::vector<std::string> models;
                for (const auto& m : j["models"]) {
                    if (m.contains("name")) {
                        models.push_back(m["name"].get<std::string>());
                    }
                }
                LOG_INFO("OllamaLLMProvider: Found {} models", models.size());
                return models;
            }
        }
    } catch (const std::exception& e) {
        LOG_WARN("OllamaLLMProvider: Failed to get models: {}", e.what());
    }

    // 返回默认模型列表
    return {"llama3.2", "llama3.1", "qwen2.5", "deepseek-r1", "gemma2"};
}

// ========== 私有方法实现 ==========

std::string OllamaLLMProvider::build_chat_request(const LLMRequest& request) const {
    json j;
    j["model"] = m_current_model;
    j["stream"] = request.stream;

    // 构建消息数组
    json messages = json::array();

    // 系统提示
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

    // 当前提示
    messages.push_back({
        {"role", "user"},
        {"content", request.prompt}
    });

    j["messages"] = messages;

    // Ollama options
    j["options"] = {
        {"temperature", request.temperature},
        {"num_predict", request.max_tokens},
        {"top_p", request.top_p}
    };

    return j.dump();
}

/**
 * @brief 构建用于 /api/generate 端点的请求（用于代码模型）
 * @note /api/generate 使用 "prompt" 字段而不是 "messages" 数组
 */
std::string OllamaLLMProvider::build_generate_request(const LLMRequest& request) const {
    json j;
    j["model"] = m_current_model;
    j["stream"] = request.stream;

    // 构建提示词（合并系统提示和上下文）
    std::string full_prompt;

    if (!request.system_prompt.empty()) {
        full_prompt += "System: " + request.system_prompt + "\n\n";
    }

    // 添加上下文消息
    for (const auto& msg : request.context) {
        full_prompt += msg + "\n";
    }

    // 添加当前提示
    full_prompt += request.prompt;

    j["prompt"] = full_prompt;

    // Ollama options
    j["options"] = {
        {"temperature", request.temperature},
        {"num_predict", request.max_tokens},
        {"top_p", request.top_p}
    };

    return j.dump();
}

DearTs::Core::Result<LLMResponse, std::string> OllamaLLMProvider::parse_response(
    const std::string& ndjson_body
) const {
    try {
        // Ollama 返回 NDJSON（每行一个 JSON）
        std::istringstream stream(ndjson_body);
        std::string line;
        std::string full_content;

        while (std::getline(stream, line)) {
            if (!line.empty()) {
                std::string content = parse_ndjson_line(line);
                if (!content.empty()) {
                    full_content += content;
                }
            }
        }

        if (full_content.empty()) {
            return DearTs::Core::Result<LLMResponse, std::string>::err("Empty response from Ollama");
        }

        LLMResponse response;
        response.content = full_content;
        response.is_complete = true;

        return DearTs::Core::Result<LLMResponse, std::string>::ok(response);

    } catch (const json::exception& e) {
        return DearTs::Core::Result<LLMResponse, std::string>::err(
            std::format("JSON parse error: {}", e.what())
        );
    } catch (const std::exception& e) {
        return DearTs::Core::Result<LLMResponse, std::string>::err(
            std::format("Parse error: {}", e.what())
        );
    }
}

std::string OllamaLLMProvider::parse_ndjson_line(const std::string& line) const {
    try {
        json j = json::parse(line);

        // /api/chat 响应格式: {"message": {"role": "assistant", "content": "..."}}
        if (j.contains("message") && j["message"].contains("content")) {
            return j["message"]["content"].get<std::string>();
        }

        // /api/generate 响应格式: {"response": "..."}
        if (j.contains("response")) {
            return j["response"].get<std::string>();
        }

    } catch (const json::exception&) {
        // 忽略解析错误的行
    }
    return "";
}

DearTs::Core::Result<void, std::string> OllamaLLMProvider::send_streaming_request(
    const LLMRequest& request,
    const std::function<void(const std::string&)>& on_chunk
) const {
    try {
        LOG_INFO("Ollama: Starting streaming request to {}, model={}", m_base_url, m_current_model);

        // 根据模型类型选择端点和请求格式
        const std::string endpoint = get_api_endpoint(m_current_model);
        const std::string request_body = (endpoint == "/api/chat")
            ? build_chat_request(request)
            : build_generate_request(request);

        LOG_INFO("Ollama: Using {} endpoint for streaming", endpoint);
        LOG_DEBUG("Ollama: Request body: {}", request_body);

        // 判断端点类型
        const bool is_chat_endpoint = (endpoint == "/api/chat");
        LOG_INFO("Ollama: {} endpoint - {} processing mode",
                 endpoint,
                 is_chat_endpoint ? "cumulative" : "incremental");

        // 使用流式回调发送 HTTP 请求
        std::string incomplete_line_buffer;  // 缓冲不完整的 NDJSON 行
        std::string previous_sent_content;   // 上次发送给调用者的完整内容（仅 /api/chat 使用）

        auto result = send_http_request(
            endpoint,
            request_body,
            [&](const std::string& chunk) {
                // 按行分割 NDJSON，处理可能的不完整行
                std::string data = incomplete_line_buffer + chunk;
                incomplete_line_buffer.clear();

                std::istringstream stream(data);
                std::string line;

                // 逐条处理每个 NDJSON 行
                while (std::getline(stream, line)) {
                    if (!line.empty()) {
                        std::string content = parse_ndjson_line(line);
                        if (!content.empty() && on_chunk) {
                            if (is_chat_endpoint) {
                                // /api/chat：message.content 是累积的，需要去重
                                if (content.length() > previous_sent_content.length() &&
                                    content.compare(0, previous_sent_content.length(), previous_sent_content) == 0) {
                                    // 内容是累积的，只发送新增部分
                                    std::string delta = content.substr(previous_sent_content.length());
                                    if (!delta.empty()) {
                                        on_chunk(delta);
                                        previous_sent_content = content;
                                    }
                                } else if (previous_sent_content.empty()) {
                                    // 第一次发送，发送完整内容
                                    // LOG_INFO("Streaming: first content='{}' (len={})",
                                    //         content.substr(0, std::min(size_t(30), content.length())),
                                    //         content.length());
                                    on_chunk(content);
                                    previous_sent_content = content;
                                } else {
                                    // 异常情况：内容不是累积的
                                    // LOG_WARN("Streaming: non-cumulative content detected, prev='{}...', curr='{}...'",
                                    //          previous_sent_content.substr(0, 20),
                                    //          content.substr(0, 20));
                                    on_chunk(content);
                                    previous_sent_content = content;
                                }
                            } else {
                                // /api/generate：response 已经是增量片段，直接发送，不做任何处理
                                // 不要假设任何累积关系，完全信任 Ollama 发送的内容
                                // LOG_INFO("Streaming: forwarding incremental content='{}' (len={})",
                                //         content.substr(0, std::min(size_t(30), content.length())),
                                //         content.length());
                                on_chunk(content);
                            }
                        }
                    }
                }

                // 检查是否有未完成的行（没有换行符的最后一行）
                size_t last_newline = data.find_last_of('\n');
                if (last_newline != std::string::npos && last_newline < data.length() - 1) {
                    // 保存未完成的行，等待下一个 chunk
                    incomplete_line_buffer = data.substr(last_newline + 1);
                }
            }
        );

        if (!result.isOk()) {
            LOG_ERROR("Ollama: HTTP request failed: {}", result.error());
            return DearTs::Core::Result<void, std::string>::err(result.error());
        }

        LOG_INFO("Ollama: Streaming request completed successfully, total content length: {}",
                 on_chunk ? "(streaming)" : "unknown");
        return DearTs::Core::Result<void, std::string>::ok();

    } catch (const std::exception& e) {
        return DearTs::Core::Result<void, std::string>::err(
            std::format("Streaming request failed: {}", e.what())
        );
    }
}

DearTs::Core::Result<std::string, std::string> OllamaLLMProvider::send_http_request(
    const std::string& endpoint,
    const std::string& json_body,
    const std::function<void(const std::string&)>& on_stream_chunk
) const {
    using namespace DearTs::Core::Network;

    // 配置 HTTP 客户端
    HttpClientConfig config;
    config.connect_timeout = std::chrono::seconds(10);
    config.request_timeout = std::chrono::seconds(30);

    // 创建 HTTP 客户端
    BoostAsioHttpClient client(m_base_url, config);

    // 构建请求
    HttpRequest request;
    request.method = json_body.empty() ? "GET" : "POST";
    request.endpoint = endpoint;
    request.headers["Content-Type"] = "application/json";
    request.body = json_body;
    request.on_chunk = on_stream_chunk;

    // 详细调试日志：显示完整请求
    LOG_DEBUG("Ollama: Sending HTTP {} request to {}{}",
              request.method, m_base_url, endpoint);
    LOG_DEBUG("Ollama: Request body: {}", json_body);

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
                if (j.contains("error")) {
                    error_detail = j["error"].get<std::string>();
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

        // 对于 404 错误，始终记录完整响应体以帮助调试
        if (response.status_code == 404) {
            LOG_ERROR("Ollama: HTTP 404 Not Found");
            LOG_ERROR("Ollama: Full response body: {}", response.body.empty() ? "(empty)" : response.body);
        } else {
            LOG_ERROR("Ollama: HTTP request failed: {}", error_msg);
        }
        return DearTs::Core::Result<std::string, std::string>::err(error_msg);
    }

    return DearTs::Core::Result<std::string, std::string>::ok(response.body);
}

bool OllamaLLMProvider::test_connection() const {
    try {
        // 发送一个简单的 GET 请求测试连接
        auto response = send_http_request("/api/tags", "");
        return response.isOk();
    } catch (...) {
        return false;
    }
}

// ========== Embed API 实现 ==========

DearTs::Core::Result<EmbeddingResult, std::string> OllamaLLMProvider::generate_embedding(
    const std::string& text,
    const std::string& model
) const {
    LOG_INFO("OllamaLLMProvider::generate_embedding() called, model={}, text_length={}",
             model, text.length());

    try {
        const std::string request_body = build_embedding_request(text, model);
        auto response_body = send_http_request("/api/embed", request_body);

        if (!response_body.isOk()) {
            return DearTs::Core::Result<EmbeddingResult, std::string>::err(response_body.error());
        }

        auto result = parse_embedding_response(response_body.unwrap());
        if (result.isOk()) {
            auto& embedding = result.unwrap();
            LOG_INFO("Ollama: Generated embedding: dimension={}, duration={} ms",
                     embedding.dimension, embedding.total_duration_ms);
        }

        return result;

    } catch (const std::exception& e) {
        return DearTs::Core::Result<EmbeddingResult, std::string>::err(
            std::format("Ollama embed request failed: {}", e.what())
        );
    }
}

DearTs::Core::Result<std::vector<EmbeddingResult>, std::string>
OllamaLLMProvider::generate_embeddings_batch(
    const std::vector<std::string>& texts,
    const std::string& model
) const {
    LOG_INFO("OllamaLLMProvider::generate_embeddings_batch() called, count={}, model={}",
             texts.size(), model);

    std::vector<EmbeddingResult> results;
    results.reserve(texts.size());

    for (const auto& text : texts) {
        auto result = generate_embedding(text, model);
        if (result.isErr()) {
            return DearTs::Core::Result<std::vector<EmbeddingResult>, std::string>::err(result.error());
        }
        results.push_back(result.unwrap());
    }

    LOG_INFO("Ollama: Generated {} embeddings", results.size());
    return DearTs::Core::Result<std::vector<EmbeddingResult>, std::string>::ok(results);
}

std::vector<std::string> OllamaLLMProvider::get_embedding_models() const {
    // 返回常见的嵌入模型列表
    return {
        "nomic-embed-text",
        "mxbai-embed-large",
        "all-minilm"
    };
}

// ========== Embed 辅助方法 ==========

std::string OllamaLLMProvider::build_embedding_request(
    const std::string& text,
    const std::string& model
) const {
    json j;
    j["model"] = model;
    j["input"] = text;

    return j.dump();
}

DearTs::Core::Result<EmbeddingResult, std::string> OllamaLLMProvider::parse_embedding_response(
    const std::string& json_body
) const {
    try {
        json j = json::parse(json_body);

        EmbeddingResult result;

        // 解析模型名称
        if (j.contains("model")) {
            result.model = j["model"].get<std::string>();
        }

        // 解析嵌入向量
        if (j.contains("embeddings") && j["embeddings"].is_array() && !j["embeddings"].empty()) {
            auto& embedding_array = j["embeddings"][0];
            if (embedding_array.is_array()) {
                for (const auto& val : embedding_array) {
                    result.embedding.push_back(val.get<float>());
                }
            }
        }

        result.dimension = static_cast<int>(result.embedding.size());

        if (result.dimension == 0) {
            return DearTs::Core::Result<EmbeddingResult, std::string>::err(
                "Empty embedding vector in response"
            );
        }

        // 解析耗时信息
        if (j.contains("total_duration")) {
            // Ollama 返回纳秒，转换为毫秒
            result.total_duration_ms = j["total_duration"].get<int64_t>() / 1000000;
        }

        if (j.contains("load_duration")) {
            result.load_duration_ms = j["load_duration"].get<int64_t>() / 1000000;
        }

        if (j.contains("prompt_eval_count")) {
            result.prompt_eval_count = j["prompt_eval_count"].get<int64_t>();
        }

        return DearTs::Core::Result<EmbeddingResult, std::string>::ok(result);

    } catch (const json::exception& e) {
        return DearTs::Core::Result<EmbeddingResult, std::string>::err(
            std::format("JSON parse error: {}", e.what())
        );
    } catch (const std::exception& e) {
        return DearTs::Core::Result<EmbeddingResult, std::string>::err(
            std::format("Parse error: {}", e.what())
        );
    }
}

// ========== EmbeddingResult 序列化 ==========

std::string EmbeddingResult::to_json() const {
    json j;
    j["model"] = model;
    j["embedding"] = embedding;
    j["dimension"] = dimension;
    j["total_duration_ms"] = total_duration_ms;
    j["load_duration_ms"] = load_duration_ms;
    j["prompt_eval_count"] = prompt_eval_count;
    return j.dump();
}

DearTs::Core::Result<EmbeddingResult, std::string>
EmbeddingResult::from_json(const std::string& json_str) {
    try {
        json j = json::parse(json_str);

        EmbeddingResult result;
        if (j.contains("model")) {
            result.model = j["model"].get<std::string>();
        }
        if (j.contains("embedding")) {
            result.embedding = j["embedding"].get<std::vector<float>>();
        }
        if (j.contains("dimension")) {
            result.dimension = j["dimension"].get<int>();
        }
        if (j.contains("total_duration_ms")) {
            result.total_duration_ms = j["total_duration_ms"].get<int64_t>();
        }
        if (j.contains("load_duration_ms")) {
            result.load_duration_ms = j["load_duration_ms"].get<int64_t>();
        }
        if (j.contains("prompt_eval_count")) {
            result.prompt_eval_count = j["prompt_eval_count"].get<int64_t>();
        }

        return DearTs::Core::Result<EmbeddingResult, std::string>::ok(result);

    } catch (const json::exception& e) {
        return DearTs::Core::Result<EmbeddingResult, std::string>::err(
            std::format("JSON parse error: {}", e.what())
        );
    }
}

} // namespace DearTs::Plugins::Chat::LLM
