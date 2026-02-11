/**
 * @file embedding_provider.cpp
 * @brief 嵌入向量提供者实现
 */

#include "memory_core/rag/embedding_provider.hpp"
#include "core/network/http_client.hpp"
#include "core/network/http_types.hpp"
#include "liblogger/logger.h"
#include <nlohmann/json.hpp>
#include <format>
#include <sstream>

// SQLite3 头文件 - 条件编译
#ifdef SQLITE3_FOUND
    #include <sqlite3.h>
#endif

namespace DearTs::Plugins::MemoryCore::RAG {

using json = nlohmann::json;

// ============ EmbeddingVector 序列化 ============

std::string EmbeddingVector::to_json() const {
    json j;
    j["model"] = model;
    j["dimension"] = dimension;
    j["embedding"] = data;
    j["duration_ms"] = duration_ms;
    return j.dump();
}

DearTs::Core::Result<EmbeddingVector, std::string>
EmbeddingVector::from_json(const std::string& json_str) {
    try {
        json j = json::parse(json_str);

        EmbeddingVector result;
        if (j.contains("model")) {
            result.model = j["model"].get<std::string>();
        }
        if (j.contains("dimension")) {
            result.dimension = j["dimension"].get<int>();
        }
        if (j.contains("embedding")) {
            result.data = j["embedding"].get<std::vector<float>>();
        }
        if (j.contains("duration_ms")) {
            result.duration_ms = j["duration_ms"].get<int64_t>();
        }

        // 验证数据
        if (result.data.empty()) {
            return DearTs::Core::Result<EmbeddingVector, std::string>::err(
                "Empty embedding vector"
            );
        }
        if (static_cast<int>(result.data.size()) != result.dimension) {
            return DearTs::Core::Result<EmbeddingVector, std::string>::err(
                std::format("Dimension mismatch: expected {}, got {}",
                           result.dimension, result.data.size())
            );
        }

        return DearTs::Core::Result<EmbeddingVector, std::string>::ok(result);

    } catch (const json::exception& e) {
        return DearTs::Core::Result<EmbeddingVector, std::string>::err(
            std::format("JSON parse error: {}", e.what())
        );
    } catch (const std::exception& e) {
        return DearTs::Core::Result<EmbeddingVector, std::string>::err(
            std::format("Parse error: {}", e.what())
        );
    }
}

// ============ OllamaEmbeddingProvider 实现 ============

OllamaEmbeddingProvider::OllamaEmbeddingProvider(
    const std::string& base_url,
    const std::string& model
) : m_base_url(base_url), m_model(model) {
    LOG_INFO("OllamaEmbeddingProvider initialized: url={}, model={}", base_url, model);
}

DearTs::Core::Result<EmbeddingVector, std::string>
OllamaEmbeddingProvider::generate_embedding(const std::string& text) {
    using namespace DearTs::Core::Network;

    LOG_INFO("OllamaEmbeddingProvider::generate_embedding(): text_length={}", text.length());

    try {
        // 构建 Ollama Embed API 请求
        json request_json;
        request_json["model"] = m_model;
        request_json["input"] = text;

        std::string request_body = request_json.dump();
        LOG_DEBUG("Ollama embed request: {}", request_body);

        // 配置 HTTP 客户端
        HttpClientConfig config;
        config.connect_timeout = std::chrono::seconds(10);
        config.request_timeout = std::chrono::seconds(60);  // 嵌入生成可能较慢

        // 创建 HTTP 客户端
        BoostAsioHttpClient client(m_base_url, config);

        // 构建 HTTP 请求
        HttpRequest request;
        request.method = "POST";
        request.endpoint = "/api/embed";
        request.headers["Content-Type"] = "application/json";
        request.body = request_body;

        // 发送请求
        auto response_result = client.request(request);

        if (response_result.isErr()) {
            return DearTs::Core::Result<EmbeddingVector, std::string>::err(response_result.error());
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

            std::string error_msg = std::format("HTTP {} - {}",
                response.status_code,
                error_detail.empty() ? "Unknown error" : error_detail);

            LOG_ERROR("Ollama embed request failed: {}", error_msg);
            return DearTs::Core::Result<EmbeddingVector, std::string>::err(error_msg);
        }

        // 解析响应
        json response_json = json::parse(response.body);

        EmbeddingVector result;
        result.model = m_model;

        // 提取向量数据
        if (response_json.contains("embeddings") && response_json["embeddings"].is_array()) {
            auto& embeddings = response_json["embeddings"];
            if (!embeddings.empty() && embeddings[0].is_array()) {
                for (const auto& val : embeddings[0]) {
                    result.data.push_back(val.get<float>());
                }
            }
        }

        result.dimension = static_cast<int>(result.data.size());

        if (result.dimension == 0) {
            return DearTs::Core::Result<EmbeddingVector, std::string>::err(
                "Empty embedding vector in response"
            );
        }

        // 提取耗时信息
        if (response_json.contains("total_duration")) {
            result.duration_ms = response_json["total_duration"].get<int64_t>() / 1000000;  // 纳秒转毫秒
        }

        LOG_INFO("Ollama embed success: dimension={}, duration={} ms",
                 result.dimension, result.duration_ms);

        return DearTs::Core::Result<EmbeddingVector, std::string>::ok(result);

    } catch (const json::exception& e) {
        return DearTs::Core::Result<EmbeddingVector, std::string>::err(
            std::format("JSON parse error: {}", e.what())
        );
    } catch (const std::exception& e) {
        return DearTs::Core::Result<EmbeddingVector, std::string>::err(
            std::format("Embedding generation failed: {}", e.what())
        );
    }
}

DearTs::Core::Result<std::vector<EmbeddingVector>, std::string>
OllamaEmbeddingProvider::generate_embeddings_batch(const std::vector<std::string>& texts) {
    LOG_INFO("OllamaEmbeddingProvider::generate_embeddings_batch(): count={}", texts.size());

    std::vector<EmbeddingVector> results;
    results.reserve(texts.size());

    // 逐个生成（Ollama 的批量 embed API 可能不稳定）
    for (size_t i = 0; i < texts.size(); ++i) {
        LOG_DEBUG("Generating embedding {}/{}", i + 1, texts.size());

        auto result = generate_embedding(texts[i]);
        if (result.isErr()) {
            return DearTs::Core::Result<std::vector<EmbeddingVector>, std::string>::err(
                std::format("Batch failed at index {}: {}", i, result.error())
            );
        }
        results.push_back(result.unwrap());
    }

    LOG_INFO("Ollama batch embed completed: {} embeddings", results.size());
    return DearTs::Core::Result<std::vector<EmbeddingVector>, std::string>::ok(results);
}

bool OllamaEmbeddingProvider::test_connection() const {
    using namespace DearTs::Core::Network;

    try {
        HttpClientConfig config;
        config.connect_timeout = std::chrono::seconds(5);
        config.request_timeout = std::chrono::seconds(10);

        BoostAsioHttpClient client(m_base_url, config);

        HttpRequest request;
        request.method = "GET";
        request.endpoint = "/api/tags";

        auto response_result = client.request(request);
        return response_result.isOk() &&
               response_result.unwrap().status_code == 200;

    } catch (...) {
        return false;
    }
}

// ============ HTTPEmbeddingProvider 实现 ============

HTTPEmbeddingProvider::HTTPEmbeddingProvider(
    const std::string& base_url,
    const std::string& model,
    const std::string& api_key
) : m_base_url(base_url), m_model(model), m_api_key(api_key) {
    LOG_INFO("HTTPEmbeddingProvider initialized: url={}, model={}", base_url, model);
}

DearTs::Core::Result<EmbeddingVector, std::string>
HTTPEmbeddingProvider::generate_embedding(const std::string& text) {
    LOG_INFO("HTTPEmbeddingProvider::generate_embedding(): text_length={}", text.length());

    try {
        // 构建 OpenAI 兼容 Embed API 请求
        json request_json;
        request_json["input"] = text;
        request_json["model"] = m_model;

        std::string request_body = request_json.dump();
        LOG_DEBUG("HTTP embed request: {}", request_body);

        // 发送请求
        auto response_result = send_request("/embeddings", request_body);

        if (response_result.isErr()) {
            return DearTs::Core::Result<EmbeddingVector, std::string>::err(response_result.error());
        }

        std::string response_body = response_result.unwrap();

        // 解析响应
        json response_json = json::parse(response_body);

        EmbeddingVector result;
        result.model = m_model;

        // OpenAI 兼容格式: {"data": [{"embedding": [...]}]}
        if (response_json.contains("data") && response_json["data"].is_array()) {
            auto& data = response_json["data"];
            if (!data.empty() && data[0].contains("embedding")) {
                auto& embedding = data[0]["embedding"];
                if (embedding.is_array()) {
                    for (const auto& val : embedding) {
                        result.data.push_back(val.get<float>());
                    }
                }
            }
        }

        result.dimension = static_cast<int>(result.data.size());

        if (result.dimension == 0) {
            return DearTs::Core::Result<EmbeddingVector, std::string>::err(
                "Empty embedding vector in response"
            );
        }

        LOG_INFO("HTTP embed success: dimension={}", result.dimension);

        return DearTs::Core::Result<EmbeddingVector, std::string>::ok(result);

    } catch (const json::exception& e) {
        return DearTs::Core::Result<EmbeddingVector, std::string>::err(
            std::format("JSON parse error: {}", e.what())
        );
    } catch (const std::exception& e) {
        return DearTs::Core::Result<EmbeddingVector, std::string>::err(
            std::format("Embedding generation failed: {}", e.what())
        );
    }
}

DearTs::Core::Result<std::vector<EmbeddingVector>, std::string>
HTTPEmbeddingProvider::generate_embeddings_batch(const std::vector<std::string>& texts) {
    LOG_INFO("HTTPEmbeddingProvider::generate_embeddings_batch(): count={}", texts.size());

    if (texts.empty()) {
        return DearTs::Core::Result<std::vector<EmbeddingVector>, std::string>::ok({});
    }

    try {
        // 构建批量嵌入请求
        json request_json;
        request_json["input"] = texts;  // 批量输入
        request_json["model"] = m_model;

        std::string request_body = request_json.dump();

        // 发送请求
        auto response_result = send_request("/embeddings", request_body);

        if (response_result.isErr()) {
            return DearTs::Core::Result<std::vector<EmbeddingVector>, std::string>::err(response_result.error());
        }

        std::string response_body = response_result.unwrap();

        // 解析响应
        json response_json = json::parse(response_body);

        std::vector<EmbeddingVector> results;

        if (response_json.contains("data") && response_json["data"].is_array()) {
            auto& data = response_json["data"];

            for (const auto& item : data) {
                if (item.contains("embedding") && item["embedding"].is_array()) {
                    EmbeddingVector result;
                    result.model = m_model;

                    for (const auto& val : item["embedding"]) {
                        result.data.push_back(val.get<float>());
                    }

                    result.dimension = static_cast<int>(result.data.size());
                    results.push_back(std::move(result));
                }
            }
        }

        if (results.empty()) {
            return DearTs::Core::Result<std::vector<EmbeddingVector>, std::string>::err(
                "No embedding vectors in response"
            );
        }

        LOG_INFO("HTTP batch embed success: {} embeddings", results.size());

        return DearTs::Core::Result<std::vector<EmbeddingVector>, std::string>::ok(results);

    } catch (const json::exception& e) {
        return DearTs::Core::Result<std::vector<EmbeddingVector>, std::string>::err(
            std::format("JSON parse error: {}", e.what())
        );
    } catch (const std::exception& e) {
        return DearTs::Core::Result<std::vector<EmbeddingVector>, std::string>::err(
            std::format("Batch embedding generation failed: {}", e.what())
        );
    }
}

bool HTTPEmbeddingProvider::test_connection() const {
    using namespace DearTs::Core::Network;

    try {
        HttpClientConfig config;
        config.connect_timeout = std::chrono::seconds(5);
        config.request_timeout = std::chrono::seconds(10);

        BoostAsioHttpClient client(m_base_url, config);

        HttpRequest request;
        request.method = "GET";
        request.endpoint = "/models";

        auto response_result = client.request(request);
        return response_result.isOk() &&
               response_result.unwrap().status_code == 200;

    } catch (...) {
        return false;
    }
}

DearTs::Core::Result<std::string, std::string>
HTTPEmbeddingProvider::send_request(
    const std::string& endpoint,
    const std::string& json_body
) const {
    using namespace DearTs::Core::Network;

    // 配置 HTTP 客户端
    HttpClientConfig config;
    config.connect_timeout = std::chrono::seconds(10);
    config.request_timeout = std::chrono::seconds(60);  // 嵌入生成可能较慢

    // 创建 HTTP 客户端
    BoostAsioHttpClient client(m_base_url, config);

    // 构建 HTTP 请求
    HttpRequest request;
    request.method = "POST";
    request.endpoint = endpoint;
    request.headers["Content-Type"] = "application/json";

    // 添加 API Key（如果有）
    if (!m_api_key.empty()) {
        request.headers["Authorization"] = std::format("Bearer {}", m_api_key);
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
                if (j.contains("error")) {
                    if (j["error"].is_object() && j["error"].contains("message")) {
                        error_detail = j["error"]["message"].get<std::string>();
                    } else if (j["error"].is_string()) {
                        error_detail = j["error"].get<std::string>();
                    }
                } else if (j.contains("message")) {
                    error_detail = j["message"].get<std::string>();
                } else {
                    error_detail = response.body;
                }
            } catch (...) {
                error_detail = response.body;
            }
        }

        std::string error_msg = std::format("HTTP {} - {}",
            response.status_code,
            error_detail.empty() ? "Unknown error" : error_detail);

        LOG_ERROR("HTTP embed request failed: {}", error_msg);
        return DearTs::Core::Result<std::string, std::string>::err(error_msg);
    }

    return DearTs::Core::Result<std::string, std::string>::ok(response.body);
}

// ============ EmbeddingProviderFactory 实现 ============

std::unique_ptr<IEmbeddingProvider>
EmbeddingProviderFactory::create_ollama_provider(
    const std::string& base_url,
    const std::string& model
) {
    return std::make_unique<OllamaEmbeddingProvider>(base_url, model);
}

std::unique_ptr<IEmbeddingProvider>
EmbeddingProviderFactory::create_http_provider(
    const std::string& base_url,
    const std::string& model,
    const std::string& api_key
) {
    return std::make_unique<HTTPEmbeddingProvider>(base_url, model, api_key);
}

std::unique_ptr<IEmbeddingProvider>
EmbeddingProviderFactory::create_custom_provider(
    std::function<DearTs::Core::Result<EmbeddingVector, std::string>(const std::string&)> generate_func,
    const std::string& model_name
) {
    return std::make_unique<CustomEmbeddingProvider>(std::move(generate_func), model_name);
}

} // namespace DearTs::Plugins::MemoryCore::RAG
