/**
 * @file http_types.hpp
 * @brief HTTP 客户端类型定义
 */

#pragma once

#include "core/result.h"
#include <string>
#include <map>
#include <functional>
#include <chrono>

namespace DearTs::Core::Network {

/**
 * @brief HTTP 客户端配置
 */
struct HttpClientConfig {
    std::string user_agent = "DearTs/1.0";
    std::chrono::milliseconds connect_timeout{10000};      // 连接超时（默认 10 秒）
    std::chrono::milliseconds request_timeout{30000};      // 请求超时（默认 30 秒）
    size_t max_redirects = 5;                             // 最大重定向次数
    bool verify_ssl = true;                                // 是否验证 SSL 证书
};

/**
 * @brief HTTP 请求
 */
struct HttpRequest {
    std::string method;  // "GET" or "POST"
    std::string endpoint;
    std::map<std::string, std::string> headers;
    std::string body;

    /**
     * @brief 流式响应回调 (用于 NDJSON/SSE)
     *
     * 当设置此回调时，响应数据会逐块传递给回调函数
     * - NDJSON: 每次回调接收一行 JSON 对象
     * - SSE: 每次回调接收一个事件
     */
    std::function<void(const std::string&)> on_chunk;
};

/**
 * @brief HTTP 响应
 */
struct HttpResponse {
    int status_code = 0;                                   // HTTP 状态码（200, 404, 500 等）
    std::map<std::string, std::string> headers;            // 响应头
    std::string body;                                      // 响应体
    std::string error_message;                             // 错误信息（如果失败）
};

/**
 * @brief 异步 HTTP 请求回调类型
 * @param Result<HttpResponse, std::string> 请求结果
 */
using AsyncHttpCallback = std::function<void(DearTs::Core::Result<HttpResponse, std::string>)>;

/**
 * @brief 异步流式 HTTP 请求回调类型（用于流式响应）
 * @param Result<void, std::string> 请求结果（成功表示流结束）
 */
using AsyncHttpStreamCallback = std::function<void(DearTs::Core::Result<void, std::string>)>;

} // namespace DearTs::Core::Network
