/**
 * @file llm_memory_extractor.cpp
 * @brief LLM 记忆提取器实现
 */

#include "memory_core/memory/llm_memory_extractor.hpp"
#include "memory_core/memory/memory_extractor.hpp"
#include "memory_core/memory/memory_manager.hpp"
#include "liblogger/logger.h"
#include <nlohmann/json.hpp>
#include <sstream>
#include <regex>
#include <chrono>
#include <format>
#include <mutex>

#ifdef _WIN32
    #include <windows.h>
    #include <winhttp.h>
    #pragma comment(lib, "winhttp.lib")
#else
    // 简化：仅支持 Windows
#endif

namespace DearTs::Plugins::MemoryCore {

using json = nlohmann::json;

// ============ LLM 客户端桥接层 ============

namespace LLMIntegration {

/**
 * @brief Ollama LLM 客户端（轻量级实现）
 *
 * 独立实现，避免直接依赖 Chat 插件
 */
class OllamaLLMClient {
public:
    explicit OllamaLLMClient(const std::string& base_url) : m_base_url(base_url) {}

    /**
     * @brief 发送聊天请求
     */
    DearTs::Core::Result<std::string, std::string> chat(
        const std::string& prompt,
        const std::string& model,
        double temperature = 0.7,
        int max_tokens = 500
    ) {
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

            LOG_DEBUG("OllamaLLMClient: Sending request to /api/chat, model={}, prompt_len={}",
                     model, prompt.length());

            auto response = send_http_request("/api/chat", request_body);

            if (!response.isOk()) {
                LOG_ERROR("OllamaLLMClient: Request failed: {}", response.error());
                return response;
            }

            std::string response_body = response.unwrap();

            LOG_DEBUG("OllamaLLMClient: Received response, body_len={}", response_body.length());

            // 解析响应（/api/chat 格式）
            json resp = json::parse(response_body);
            if (resp.contains("message") && resp["message"].contains("content")) {
                return DearTs::Core::Result<std::string, std::string>::ok(
                    resp["message"]["content"].get<std::string>()
                );
            }

            // 响应格式不对，记录完整响应用于调试
            LOG_ERROR("OllamaLLMClient: Response missing 'message.content' field. Full response: {}",
                     response_body);
            return DearTs::Core::Result<std::string, std::string>::err("No response in output");

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

    DearTs::Core::Result<std::string, std::string> send_http_request(
        const std::string& endpoint,
        const std::string& json_body
    ) {
#ifdef _WIN32
        HINTERNET hSession = nullptr;
        HINTERNET hConnect = nullptr;
        HINTERNET hRequest = nullptr;

        // 解析 URL
        std::string host_name = "localhost";
        int port = 11434;

        if (m_base_url.find("://") != std::string::npos) {
            size_t host_start = m_base_url.find("://") + 3;
            size_t colon_pos = m_base_url.find(':', host_start);
            if (colon_pos != std::string::npos) {
                host_name = m_base_url.substr(host_start, colon_pos - host_start);
                size_t port_end = m_base_url.find('/', colon_pos);
                if (port_end == std::string::npos) port_end = m_base_url.length();
                port = std::stoi(m_base_url.substr(colon_pos + 1, port_end - colon_pos - 1));
            } else {
                size_t slash_pos = m_base_url.find('/', host_start);
                if (slash_pos == std::string::npos) slash_pos = m_base_url.length();
                host_name = m_base_url.substr(host_start, slash_pos - host_start);
            }
        }

        std::wstring host_name_w(host_name.begin(), host_name.end());
        std::wstring endpoint_w(endpoint.begin(), endpoint.end());

        hSession = WinHttpOpen(
            L"MemoryCore/1.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0
        );

        if (!hSession) {
            return DearTs::Core::Result<std::string, std::string>::err("Failed to open WinHTTP session");
        }

        hConnect = WinHttpConnect(
            hSession,
            host_name_w.c_str(),
            port,
            0
        );

        if (!hConnect) {
            WinHttpCloseHandle(hSession);
            return DearTs::Core::Result<std::string, std::string>::err("Failed to connect to server");
        }

        hRequest = WinHttpOpenRequest(
            hConnect,
            json_body.empty() ? L"GET" : L"POST",
            endpoint_w.c_str(),
            nullptr,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            0
        );

        if (!hRequest) {
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return DearTs::Core::Result<std::string, std::string>::err("Failed to open request");
        }

        if (!json_body.empty()) {
            std::wstring headers = L"Content-Type: application/json\r\n";
            WinHttpAddRequestHeaders(hRequest, headers.c_str(), -1, WINHTTP_ADDREQ_FLAG_ADD);

            if (!WinHttpSendRequest(
                hRequest,
                WINHTTP_NO_ADDITIONAL_HEADERS,
                0,
                (LPVOID)json_body.data(),
                json_body.length(),
                json_body.length(),
                0
            )) {
                WinHttpCloseHandle(hRequest);
                WinHttpCloseHandle(hConnect);
                WinHttpCloseHandle(hSession);
                return DearTs::Core::Result<std::string, std::string>::err("Failed to send request");
            }
        } else {
            if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
                WinHttpCloseHandle(hRequest);
                WinHttpCloseHandle(hConnect);
                WinHttpCloseHandle(hSession);
                return DearTs::Core::Result<std::string, std::string>::err("Failed to send request");
            }
        }

        if (!WinHttpReceiveResponse(hRequest, nullptr)) {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return DearTs::Core::Result<std::string, std::string>::err("Failed to receive response");
        }

        // 检查 HTTP 状态码
        DWORD status_code = 0;
        DWORD status_size = sizeof(status_code);
        WinHttpQueryHeaders(
            hRequest,
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX,
            &status_code,
            &status_size,
            WINHTTP_NO_HEADER_INDEX
        );

        std::string response_data;
        DWORD bytes_available = 0;

        while (WinHttpQueryDataAvailable(hRequest, &bytes_available) && bytes_available > 0) {
            std::vector<char> buffer(bytes_available + 1);
            DWORD bytes_read = 0;

            if (WinHttpReadData(hRequest, buffer.data(), bytes_available, &bytes_read)) {
                buffer[bytes_read] = '\0';
                response_data += buffer.data();
            }
        }

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);

        // 检查 HTTP 状态码
        if (status_code != 200) {
            LOG_ERROR("OllamaLLMClient: HTTP {} error for endpoint '{}', response: {}",
                     status_code, endpoint, response_data.empty() ? "(empty)" : response_data);
            return DearTs::Core::Result<std::string, std::string>::err(
                std::format("HTTP {}", status_code)
            );
        }

        LOG_DEBUG("OllamaLLMClient: Received {} bytes from endpoint '{}'",
                  response_data.length(), endpoint);

        return DearTs::Core::Result<std::string, std::string>::ok(response_data);
#else
        return DearTs::Core::Result<std::string, std::string>::err("Not implemented on this platform");
#endif
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
