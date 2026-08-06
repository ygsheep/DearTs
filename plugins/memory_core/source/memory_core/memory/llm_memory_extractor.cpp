/**
 * @file llm_memory_extractor.cpp
 * @brief LLM 记忆提取器实现
 */

#include "memory_core/memory/llm_memory_extractor.hpp"
#include "memory_core/memory/memory_extractor.hpp"
#include "memory_core/memory/memory_manager.hpp"
#include "core/network/http_client.hpp"
#include "liblogger/logger.h"
#include <nlohmann/json.hpp>
#include <sstream>
#include <regex>
#include <chrono>
#include <format>
#include <mutex>
#include <future>

namespace DearTs::Plugins::MemoryCore {

using json = nlohmann::json;

// ============ LLM 客户端桥接层 ============

namespace LLMIntegration {

/**
 * @brief Ollama LLM 客户端（基于 core/network）
 *
 * 使用统一的 Boost.Asio HTTP 客户端
 */
class OllamaLLMClient {
public:
    explicit OllamaLLMClient(const std::string& base_url) : m_base_url(base_url) {
        // 创建 HTTP 客户端
        DearTs::Core::Network::HttpClientConfig config;
        config.connect_timeout = std::chrono::seconds(10);
        config.request_timeout = std::chrono::seconds(60);  // LLM 响应可能较慢
        config.user_agent = "MemoryCore/1.0";

        m_client = std::make_unique<DearTs::Core::Network::BoostAsioHttpClient>(base_url, config);
    }

    /**
     * @brief 发送聊天请求（使用异步 HTTP，但保持同步接口）
     */
    DearTs::Core::Result<std::string, std::string> chat(
        const std::string& prompt,
        const std::string& model,
        double temperature = 0.7,
        int max_tokens = 500
    ) {
        // 使用 std::promise 来等待异步操作完成
        auto promise = std::make_shared<std::promise<DearTs::Core::Result<std::string, std::string>>>();
        auto future = promise->get_future();

        try {
            // 使用 /api/chat 端点（与主聊天一致）
            json j;
            j["model"] = model;
            j["stream"] = false;
            j["messages"] = {
                {
                    {"role", "user"},
                    {"content", prompt}
                }
            };
            j["options"] = {
                {"temperature", temperature},
                {"num_predict", max_tokens}
            };

            std::string request_body = j.dump();

            LOG_DEBUG("OllamaLLMClient: Sending async request to /api/chat, model={}, prompt_len={}",
                     model, prompt.length());

            // 使用异步 HTTP 请求
            send_http_request_async("/api/chat", request_body,
                [promise](const DearTs::Core::Result<std::string, std::string>& result) {
                    promise->set_value(result);
                }
            );

            // 等待异步操作完成
            return future.get();

        } catch (const std::exception& e) {
            return DearTs::Core::Result<std::string, std::string>::err(
                std::format("LLM request failed: {}", e.what())
            );
        }
    }

    /**
     * @brief 测试连接
     */
    bool is_available() {
        try {
            auto response = send_http_request("/api/tags", "");
            return response.isOk();
        } catch (...) {
            return false;
        }
    }

private:
    std::string m_base_url;
    std::unique_ptr<DearTs::Core::Network::BoostAsioHttpClient> m_client;

    DearTs::Core::Result<std::string, std::string> send_http_request(
        const std::string& endpoint,
        const std::string& json_body
    ) {
        using namespace DearTs::Core::Network;

        // 构建请求
        HttpRequest request;
        request.method = json_body.empty() ? "GET" : "POST";
        request.endpoint = endpoint;
        request.headers["Content-Type"] = "application/json";
        request.body = json_body;

        // 详细调试日志
        LOG_DEBUG("OllamaLLMClient: Sending HTTP {} request to {}{}",
                  request.method, m_base_url, endpoint);

        if (!json_body.empty()) {
            LOG_DEBUG("OllamaLLMClient: Request body: {}", json_body);
        }

        // 发送请求
        auto response_result = m_client->request(request);

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

            std::string error_msg = std::format("HTTP {}", response.status_code);
            if (!error_detail.empty()) {
                error_msg += std::format(" - {}", error_detail);
            }

            LOG_ERROR("OllamaLLMClient: Request failed: {}", error_msg);
            return DearTs::Core::Result<std::string, std::string>::err(error_msg);
        }

        LOG_DEBUG("OllamaLLMClient: Received {} bytes from endpoint '{}'",
                  response.body.length(), endpoint);

        return DearTs::Core::Result<std::string, std::string>::ok(response.body);
    }

    /**
     * @brief 异步发送 HTTP 请求
     */
    void send_http_request_async(
        const std::string& endpoint,
        const std::string& json_body,
        std::function<void(DearTs::Core::Result<std::string, std::string>)> callback
    ) {
        using namespace DearTs::Core::Network;

        // 构建请求（使用 shared_ptr 避免复制）
        auto request = std::make_shared<HttpRequest>();
        request->method = json_body.empty() ? "GET" : "POST";
        request->endpoint = endpoint;
        request->headers["Content-Type"] = "application/json";
        request->body = json_body;

        // 详细调试日志
        LOG_DEBUG("OllamaLLMClient: Sending async HTTP {} request to {}{}",
                  request->method, m_base_url, endpoint);

        if (!json_body.empty()) {
            LOG_DEBUG("OllamaLLMClient: Request body: {}", json_body);
        }

        // 异步发送请求（传递 shared_ptr）
        m_client->request_async(request,
            [callback, endpoint](const DearTs::Core::Result<HttpResponse, std::string>& response_result) {
                if (response_result.isErr()) {
                    callback(DearTs::Core::Result<std::string, std::string>::err(response_result.error()));
                    return;
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

                    std::string error_msg = std::format("HTTP {}", response.status_code);
                    if (!error_detail.empty()) {
                        error_msg += std::format(" - {}", error_detail);
                    }

                    LOG_ERROR("OllamaLLMClient: Async request failed: {}", error_msg);
                    callback(DearTs::Core::Result<std::string, std::string>::err(error_msg));
                    return;
                }

                LOG_DEBUG("OllamaLLMClient: Received {} bytes from endpoint '{}'",
                          response.body.length(), endpoint);

                callback(DearTs::Core::Result<std::string, std::string>::ok(response.body));
            }
        );
    }
};

} // namespace LLMIntegration

// ============ LLMMemoryExtractor 实现 ============

LLMMemoryExtractor& LLMMemoryExtractor::instance() {
    static LLMMemoryExtractor instance;
    return instance;
}

DearTs::Core::Result<void, std::string> LLMMemoryExtractor::initialize(
    const LLMExtractorConfig& config
) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_initialized) {
        LOG_WARN("LLMMemoryExtractor already initialized");
        return DearTs::Core::Result<void, std::string>::ok();
    }

    m_config = config;
    m_stats = LLMExtractorStats::empty();

    // 创建 LLM 客户端
    m_llm_client = std::make_unique<LLMIntegration::OllamaLLMClient>(config.ollama_url);

    // 检查可用性
    if (!m_llm_client->is_available()) {
        LOG_WARN("LLM service not available at {}, extraction will use fallback", config.ollama_url);
    }

    m_initialized = true;
    LOG_INFO("LLMMemoryExtractor initialized: url={}, chat_model={}",
             config.ollama_url, config.chat_model);
    return DearTs::Core::Result<void, std::string>::ok();
}

void LLMMemoryExtractor::shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_initialized) {
        return;
    }

    log_stats();
    m_llm_client.reset();
    LOG_INFO("LLMMemoryExtractor shutdown");
    m_initialized = false;
}

DearTs::Core::Result<std::vector<Memory::ExtractionResult>, std::string>
LLMMemoryExtractor::extract_from_conversation(
    const std::string& user_message,
    const std::string& assistant_response,
    const std::string& conversation_id
) {
    auto start_time = std::chrono::steady_clock::now();

    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_initialized) {
        return DearTs::Core::Result<std::vector<Memory::ExtractionResult>, std::string>::err(
            "LLMMemoryExtractor not initialized"
        );
    }

    LOG_INFO("LLM extract: user_msg_len={}, assistant_msg_len={}, conv_id={}",
             user_message.length(), assistant_response.length(), conversation_id);

    // 检查 LLM 可用性
    if (!check_llm_available()) {
        return DearTs::Core::Result<std::vector<Memory::ExtractionResult>, std::string>::err(
            "LLM service not available"
        );
    }

    // 构建提示词
    std::string prompt = build_extraction_prompt(user_message, assistant_response);

    // 调用 LLM
    auto llm_result = m_llm_client->chat(
        prompt,
        m_config.chat_model,
        m_config.temperature,
        m_config.max_tokens
    );

    if (!llm_result.isOk()) {
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time
        ).count();
        update_stats(false, duration);
        return DearTs::Core::Result<std::vector<Memory::ExtractionResult>, std::string>::err(
            "LLM request failed: " + llm_result.error()
        );
    }

    // 解析响应
    auto parse_result = parse_llm_response(llm_result.unwrap(), conversation_id);

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start_time
    ).count();

    if (parse_result.isOk()) {
        update_stats(true, duration);
        LOG_INFO("LLM extraction successful: {} memories extracted, duration={} ms",
                 parse_result.unwrap().size(), duration);
    } else {
        update_stats(false, duration);
    }

    return parse_result;
}

DearTs::Core::Result<std::vector<Memory::ExtractionResult>, std::string>
LLMMemoryExtractor::extract_from_messages(
    const std::vector<std::string>& messages,
    const std::string& conversation_id
) {
    // 简化实现：合并所有消息并提取
    std::ostringstream oss;
    for (const auto& msg : messages) {
        oss << msg << "\n";
    }

    // 将整个对话作为用户消息处理
    return extract_from_conversation(oss.str(), "", conversation_id);
}

DearTs::Core::Result<std::vector<Memory::ExtractionResult>, std::string>
LLMMemoryExtractor::extract_batch(
    const std::vector<std::pair<std::string, std::string>>& conversations,
    const std::string& conversation_id
) {
    std::vector<Memory::ExtractionResult> all_results;

    for (const auto& [user_msg, assistant_resp] : conversations) {
        auto result = extract_from_conversation(user_msg, assistant_resp, conversation_id);
        if (result.isOk()) {
            auto& results = result.unwrap();
            all_results.insert(all_results.end(), results.begin(), results.end());
        }
    }

    return DearTs::Core::Result<std::vector<Memory::ExtractionResult>, std::string>::ok(all_results);
}

LLMExtractorStats LLMMemoryExtractor::get_stats() const {
    return m_stats;
}

void LLMMemoryExtractor::log_stats() const {
    LOG_INFO("LLMExtractor Stats: total={}, success={}, failed={}, "
             "avg_duration={:.1f} ms",
             m_stats.total_extractions, m_stats.successful_extractions,
             m_stats.failed_extractions, m_stats.average_duration_ms);
}

void LLMMemoryExtractor::set_config(const LLMExtractorConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_config = config;

    // 重新创建客户端
    m_llm_client = std::make_unique<LLMIntegration::OllamaLLMClient>(config.ollama_url);
}

// ============ 私有辅助方法 ============

std::string LLMMemoryExtractor::build_extraction_prompt(
    const std::string& user_message,
    const std::string& assistant_response
) {
    std::ostringstream oss;

    oss << "你是一个专业的记忆提取助手。请从以下对话中提取重要的记忆信息。\n\n";
    oss << "对话内容：\n";
    oss << "用户: " << user_message << "\n";
    if (!assistant_response.empty()) {
        oss << "助手: " << assistant_response << "\n";
    }
    oss << "\n";
    oss << "请提取以下类型的记忆（JSON格式）：\n";
    oss << "[\n";
    oss << "  {\n";
    oss << "    \"type\": \"preference|fact|qa|context|skill\",\n";
    oss << "    \"content\": \"记忆内容\",\n";
    oss << "    \"importance\": 0.0-1.0\n";
    oss << "  }\n";
    oss << "]\n\n";
    oss << "只返回JSON数组，不要有其他内容。";

    return oss.str();
}

DearTs::Core::Result<std::vector<Memory::ExtractionResult>, std::string>
LLMMemoryExtractor::parse_llm_response(
    const std::string& response,
    const std::string& conversation_id
) {
    try {
        // 清理响应（提取 JSON）
        std::string json_str = response;

        // 尝试查找 JSON 数组
        size_t array_start = json_str.find('[');
        size_t array_end = json_str.rfind(']');

        if (array_start != std::string::npos && array_end != std::string::npos) {
            json_str = json_str.substr(array_start, array_end - array_start + 1);
        }

        json j = json::parse(json_str);

        if (!j.is_array()) {
            return DearTs::Core::Result<std::vector<Memory::ExtractionResult>, std::string>::err(
                "Response is not a JSON array"
            );
        }

        std::vector<Memory::ExtractionResult> results;

        for (const auto& item : j) {
            Memory::ExtractionResult result;

            // 解析类型
            if (item.contains("type")) {
                std::string type_str = item["type"].get<std::string>();
                if (type_str == "preference") {
                    result.memory.type = Memory::MemoryType::Preference;
                } else if (type_str == "fact") {
                    result.memory.type = Memory::MemoryType::Fact;
                } else if (type_str == "qa") {
                    result.memory.type = Memory::MemoryType::QA;
                } else if (type_str == "context") {
                    result.memory.type = Memory::MemoryType::Context;
                } else if (type_str == "skill") {
                    result.memory.type = Memory::MemoryType::Skill;
                } else {
                    result.memory.type = Memory::MemoryType::Context;  // 默认
                }
            }

            // 解析内容
            if (item.contains("content")) {
                result.memory.content = item["content"].get<std::string>();
            }

            // 解析重要性
            if (item.contains("importance")) {
                result.memory.importance = item["importance"].get<double>();
                // 限制在 [0, 1] 范围
                result.memory.importance = std::max(0.0, std::min(1.0, result.memory.importance));
            } else {
                result.memory.importance = 0.5;  // 默认
            }

            // 设置来源
            if (!conversation_id.empty()) {
                result.memory.source_conversation_id = conversation_id;
            }

            result.memory.created_at = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count();

            result.rule_name = "llm_extraction";
            result.confidence = 0.8;  // LLM 提取的置信度

            // 验证结果
            if (m_config.enable_verification && validate_extraction(result)) {
                results.push_back(result);
            } else if (!m_config.enable_verification) {
                results.push_back(result);
            }
        }

        LOG_INFO("Parsed {} extraction results from LLM response", results.size());
        return DearTs::Core::Result<std::vector<Memory::ExtractionResult>, std::string>::ok(results);

    } catch (const json::exception& e) {
        return DearTs::Core::Result<std::vector<Memory::ExtractionResult>, std::string>::err(
            std::format("JSON parse error: {}", e.what())
        );
    } catch (const std::exception& e) {
        return DearTs::Core::Result<std::vector<Memory::ExtractionResult>, std::string>::err(
            std::format("Parse error: {}", e.what())
        );
    }
}

bool LLMMemoryExtractor::validate_extraction(const Memory::ExtractionResult& result) {
    // 验证内容非空
    if (result.memory.content.empty()) {
        return false;
    }

    // 验证内容长度（至少 5 个字符）
    if (result.memory.content.length() < 5) {
        return false;
    }

    // 验证重要性范围
    if (result.memory.importance < 0.0 || result.memory.importance > 1.0) {
        return false;
    }

    return true;
}

void LLMMemoryExtractor::update_stats(bool success, int64_t duration_ms) {
    m_stats.total_extractions++;
    m_stats.total_duration_ms += duration_ms;

    if (success) {
        m_stats.successful_extractions++;
    } else {
        m_stats.failed_extractions++;
    }

    m_stats.average_duration_ms =
        static_cast<double>(m_stats.total_duration_ms) / m_stats.total_extractions;
}

bool LLMMemoryExtractor::check_llm_available() {
    return m_llm_client && m_llm_client->is_available();
}

} // namespace DearTs::Plugins::MemoryCore
