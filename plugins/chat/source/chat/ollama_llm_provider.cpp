/**
 * @file ollama_llm_provider.cpp
 * @brief Ollama LLM 提供商实现
 */

#include "chat/llm/ollama_llm_provider.hpp"
#include "liblogger/logger.h"
#include <nlohmann/json.hpp>
#include <format>
#include <sstream>
#include <future>
#include <thread>

#ifdef _WIN32
    #include <windows.h>
    #include <winhttp.h>
    #pragma comment(lib, "winhttp.lib")
#else
    #include <curl/curl.h>
#endif

namespace DearTs::Plugins::Chat::LLM {

using json = nlohmann::json;

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
    auto task = Core::Tasks::TaskManager::instance().launch(
        "Ollama LLM Request",
        [this, request, callback](const auto& cancel) {
            // 在后台线程发送请求
            auto result = this->send(request);

            if (cancel) {
                return;
            }

            // 调用回调
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
            bool completed = false;

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

        // 非流式请求
        const std::string request_body = build_chat_request(request);
        auto response_body = send_http_request("/api/chat", request_body);

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
        LOG_INFO("Ollama: Starting streaming request to {}", m_base_url);
        const std::string request_body = build_chat_request(request);
        LOG_DEBUG("Ollama: Request body: {}", request_body);

        // 使用流式回调发送 HTTP 请求
        auto result = send_http_request(
            "/api/chat",
            request_body,
            [&](const std::string& chunk) {
                // 按行分割 NDJSON
                std::istringstream stream(chunk);
                std::string line;

                while (std::getline(stream, line)) {
                    if (!line.empty()) {
                        std::string content = parse_ndjson_line(line);
                        if (!content.empty() && on_chunk) {
                            on_chunk(content);
                        }
                    }
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
#ifdef _WIN32
    // Windows 使用 WinHTTP
    HINTERNET hSession = nullptr;
    HINTERNET hConnect = nullptr;
    HINTERNET hRequest = nullptr;

    // 手动解析 URL (简化版)
    std::string host_name = "localhost";
    int port = 11434;
    bool use_https = false;

    // 解析 base_url
    if (m_base_url.find("https://") == 0) {
        use_https = true;
        size_t host_start = 8;  // 跳过 "https://"
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
    } else if (m_base_url.find("http://") == 0) {
        use_https = false;
        size_t host_start = 7;  // 跳过 "http://"
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

    LOG_INFO("Ollama: Connecting to {}:{} (use_https={})", host_name, port, use_https);

    // 转换为宽字符串
    std::wstring host_name_w(host_name.begin(), host_name.end());
    std::wstring endpoint_w(endpoint.begin(), endpoint.end());

    // 打开会话
    hSession = WinHttpOpen(
        L"ChatManager/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );

    if (!hSession) {
        LOG_ERROR("Ollama: WinHttpOpen failed");
        return DearTs::Core::Result<std::string, std::string>::err("Failed to open WinHTTP session");
    }

    // 连接服务器
    hConnect = WinHttpConnect(
        hSession,
        host_name_w.c_str(),
        port,
        0
    );

    if (!hConnect) {
        LOG_ERROR("Ollama: WinHttpConnect failed to {}:{}", host_name, port);
        WinHttpCloseHandle(hSession);
        return DearTs::Core::Result<std::string, std::string>::err("Failed to connect to Ollama server");
    }

    LOG_INFO("Ollama: Connected successfully");

    // 创建请求
    hRequest = WinHttpOpenRequest(
        hConnect,
        L"POST",
        endpoint_w.c_str(),
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        use_https ? WINHTTP_FLAG_SECURE : 0
    );

    if (!hRequest) {
        LOG_ERROR("Ollama: WinHttpOpenRequest failed");
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return DearTs::Core::Result<std::string, std::string>::err("Failed to open request");
    }

    // 添加请求头
    std::wstring headers = L"Content-Type: application/json\r\n";
    WinHttpAddRequestHeaders(
        hRequest,
        headers.c_str(),
        -1,
        WINHTTP_ADDREQ_FLAG_ADD
    );

    LOG_DEBUG("Ollama: Sending request body: {}", json_body.substr(0, std::min(size_t(200), json_body.length())));

    // 发送请求
    if (!WinHttpSendRequest(
        hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        (LPVOID)json_body.data(),
        json_body.length(),
        json_body.length(),
        0
    )) {
        LOG_ERROR("Ollama: WinHttpSendRequest failed");
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return DearTs::Core::Result<std::string, std::string>::err("Failed to send request to Ollama");
    }

    LOG_INFO("Ollama: Request sent, waiting for response...");

    // 接收响应
    if (!WinHttpReceiveResponse(hRequest, nullptr)) {
        LOG_ERROR("Ollama: WinHttpReceiveResponse failed");
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return DearTs::Core::Result<std::string, std::string>::err("Failed to receive response from Ollama");
    }

    LOG_INFO("Ollama: Response received, reading data...");

    // 读取响应数据
    std::string response_data;
    DWORD bytes_available = 0;

    if (on_stream_chunk) {
        // 流式读取
        std::vector<char> buffer(4096);
        DWORD bytes_read = 0;
        size_t total_read = 0;

        while (WinHttpReadData(hRequest, buffer.data(), buffer.size(), &bytes_read) && bytes_read > 0) {
            total_read += bytes_read;
            LOG_DEBUG("Ollama: Read {} bytes (total: {})", bytes_read, total_read);
            std::string chunk(buffer.data(), bytes_read);
            on_stream_chunk(chunk);
        }

        LOG_INFO("Ollama: Streaming complete, total bytes read: {}", total_read);

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);

        return DearTs::Core::Result<std::string, std::string>::ok("");  // 流式模式不需要返回完整响应

    } else {
        // 非流式：读取全部响应
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

        return DearTs::Core::Result<std::string, std::string>::ok(response_data);
    }

#else
    // Linux/macOS 使用 libcurl
    CURL* curl = curl_easy_init();
    if (!curl) {
        return DearTs::Core::Result<std::string, std::string>::err("Failed to initialize curl");
    }

    const std::string full_url = m_base_url + endpoint;

    curl_easy_setopt(curl, CURLOPT_URL, full_url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_body.c_str());

    // 设置请求头
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // 设置响应回调
    std::string response_data;

    if (on_stream_chunk) {
        // 流式模式
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](void* contents, size_t size, size_t nmemb, void* userp) {
            size_t total = size * nmemb;
            std::string chunk((char*)contents, total);

            if (auto* callback = (std::function<void(const std::string&)>*)userp) {
                (*callback)(chunk);
            }

            return total;
        });

        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &on_stream_chunk);

        // 执行请求
        const CURLcode res = curl_easy_perform(curl);

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            return DearTs::Core::Result<std::string, std::string>::err(
                std::format("Curl request failed: {}", curl_easy_strerror(res))
            );
        }

        return DearTs::Core::Result<std::string, std::string>::ok("");

    } else {
        // 非流式模式
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](void* contents, size_t size, size_t nmemb, std::string* s) {
            const size_t new_length = size * nmemb;
            s->append((char*)contents, new_length);
            return new_length;
        });

        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);

        // 执行请求
        const CURLcode res = curl_easy_perform(curl);

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            return DearTs::Core::Result<std::string, std::string>::err(
                std::format("Curl request failed: {}", curl_easy_strerror(res))
            );
        }

        return DearTs::Core::Result<std::string, std::string>::ok(response_data);
    }
#endif
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

} // namespace DearTs::Plugins::Chat::LLM
