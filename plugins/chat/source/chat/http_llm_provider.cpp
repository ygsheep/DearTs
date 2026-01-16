/**
 * @file http_llm_provider.cpp
 * @brief HTTP LLM 提供商实现
 */

#include "chat/llm/http_llm_provider.hpp"
#include "liblogger/logger.h"
#include <nlohmann/json.hpp>
#include <format>
#include <fstream>
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
        // 构建请求 JSON
        const std::string request_body = build_chat_completion_request(request);

        // 发送 HTTP 请求
        auto response_body = send_http_request("/chat/completions", request_body);
        if (!response_body.isOk()) {
            return DearTs::Core::Result<LLMResponse, std::string>::err(response_body.error());
        }

        // 解析响应
        auto response = parse_chat_completion_response(response_body.unwrap());
        if (!response.isOk()) {
            return DearTs::Core::Result<LLMResponse, std::string>::err(response.error());
        }

        auto llm_response = response.unwrap();

        // 计算耗时
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
    // 尝试从 /models 端点获取可用模型
    try {
        auto response = send_http_request("/models", "");
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
#ifdef _WIN32
    // Windows 使用 WinHTTP
    HINTERNET hSession = nullptr;
    HINTERNET hConnect = nullptr;
    HINTERNET hRequest = nullptr;

    // 解析 URL
    std::wstring url_base = std::wstring(m_base_url.begin(), m_base_url.end());
    std::wstring url_endpoint = std::wstring(endpoint.begin(), endpoint.end());

    URL_COMPONENTS urlComp = {0};
    urlComp.dwStructSize = sizeof(urlComp);
    urlComp.dwSchemeLength = 256;
    urlComp.dwHostNameLength = 256;
    urlComp.dwUrlPathLength = 256;

    if (!WinHttpCrackUrl(url_base.c_str(), url_base.length(), 0, &urlComp)) {
        return DearTs::Core::Result<std::string, std::string>::err("Failed to parse URL");
    }

    // 打开会话
    hSession = WinHttpOpen(
        L"ChatManager/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );

    if (!hSession) {
        return DearTs::Core::Result<std::string, std::string>::err("Failed to open WinHTTP session");
    }

    // 连接服务器
    std::wstring host_name(urlComp.lpszHostName, urlComp.dwHostNameLength);
    hConnect = WinHttpConnect(
        hSession,
        host_name.c_str(),
        urlComp.nPort,
        0
    );

    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return DearTs::Core::Result<std::string, std::string>::err("Failed to connect to server");
    }

    // 创建请求
    std::wstring full_path = std::wstring(urlComp.lpszUrlPath, urlComp.dwUrlPathLength) + url_endpoint;
    hRequest = WinHttpOpenRequest(
        hConnect,
        L"POST",
        full_path.c_str(),
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0
    );

    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return DearTs::Core::Result<std::string, std::string>::err("Failed to open request");
    }

    // 添加请求头
    std::wstring headers = L"Content-Type: application/json\r\n";
    if (!m_api_key.empty()) {
        headers += std::format(L"Authorization: Bearer {}\r\n", std::wstring(m_api_key.begin(), m_api_key.end()));
    }

    WinHttpAddRequestHeaders(
        hRequest,
        headers.c_str(),
        -1,
        WINHTTP_ADDREQ_FLAG_ADD
    );

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
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return DearTs::Core::Result<std::string, std::string>::err("Failed to send request");
    }

    // 接收响应
    if (!WinHttpReceiveResponse(hRequest, nullptr)) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return DearTs::Core::Result<std::string, std::string>::err("Failed to receive response");
    }

    // 读取响应数据
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

    return DearTs::Core::Result<std::string, std::string>::ok(response_data);

#else
    // 非 Windows 使用 libcurl
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
    if (!m_api_key.empty()) {
        headers = curl_slist_append(headers, std::format("Authorization: Bearer {}", m_api_key).c_str());
    }
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // 设置响应回调
    std::string response_data;
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
        return DearTs::Core::Result<std::string, std::string>::err(std::format("Curl request failed: {}", curl_easy_strerror(res)));
    }

    return DearTs::Core::Result<std::string, std::string>::ok(response_data);
#endif
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
        // 发送一个简单的请求测试连接
        auto response = send_http_request("/models", "");
        return response.isOk();
    } catch (...) {
        return false;
    }
}

} // namespace DearTs::Plugins::Chat::LLM
