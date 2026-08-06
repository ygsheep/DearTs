/**
 * @file http_client.hpp
 * @brief 基于 Boost.Asio + Boost.Beast 的 HTTP 客户端
 */

#pragma once

#include "core/network/http_types.hpp"
#include "core/result.h"
#include <string>
#include <memory>

namespace DearTs::Core::Network {

/**
 * @brief 基于 Boost.Asio + Boost.Beast 的 HTTP 客户端
 *
 * 特性:
 * - 跨平台 (Windows/Linux/macOS)
 * - HTTP/HTTPS 支持
 * - 流式响应 (NDJSON/SSE)
 * - 超时控制
 * - 同步 API (简化集成)
 *
 * 使用示例:
 * @code
 * BoostAsioHttpClient client("http://localhost:11434");
 *
 * HttpRequest request;
 * request.method = "POST";
 * request.endpoint = "/api/chat";
 * request.headers["Content-Type"] = "application/json";
 * request.body = R"({"model":"llama3.2","prompt":"hello"})";
 *
 * auto result = client.request(request);
 * if (result.isOk()) {
 *     auto response = result.unwrap();
 *     std::cout << "Status: " << response.status_code << "\n";
 *     std::cout << "Body: " << response.body << "\n";
 * }
 * @endcode
 */
class BoostAsioHttpClient {
public:
    /**
     * @brief 构造函数
     * @param base_url 基础 URL（例如：http://localhost:11434）
     * @param config 客户端配置
     */
    explicit BoostAsioHttpClient(
        const std::string& base_url,
        const HttpClientConfig& config = {}
    );

    /**
     * @brief 析构函数
     */
    ~BoostAsioHttpClient();

    // 允许拷贝和移动（拷贝时创建新的客户端实例）
    BoostAsioHttpClient(const BoostAsioHttpClient& other);
    BoostAsioHttpClient& operator=(const BoostAsioHttpClient& other);
    BoostAsioHttpClient(BoostAsioHttpClient&&) noexcept = default;
    BoostAsioHttpClient& operator=(BoostAsioHttpClient&&) noexcept = default;

    /**
     * @brief 发送 HTTP 请求（同步）
     * @param request 请求参数
     * @return Result<HttpResponse, std::string>
     *
     * 如果 request.on_chunk 被设置，则会处理流式响应:
     * - NDJSON: 逐行解析 JSON 对象
     * - SSE: 逐事件处理
     *
     * 注意：此方法是同步的，会阻塞直到请求完成或超时
     */
    DearTs::Core::Result<HttpResponse, std::string> request(
        const HttpRequest& request
    );

    /**
     * @brief 异步发送 HTTP 请求（非阻塞）
     * @param request 请求参数（使用 shared_ptr 避免复制）
     * @param callback 完成回调，在请求完成后调用
     *
     * 此方法立即返回，请求在后台线程中执行。
     * 使用 Boost.Asio 的异步操作，不会阻塞调用线程。
     *
     * 注意：
     * - on_chunk 回调会在 io_context 线程中被调用
     * - 确保 on_chunk 中的操作是线程安全的
     * - 使用 shared_ptr 避免复制大请求体，只增加引用计数开销
     *
     * @example
     * auto req = std::make_shared<HttpRequest>();
     * req->method = "POST";
     * req->body = large_json_string;
     * client->request_async(req, callback);
     */
    void request_async(
        std::shared_ptr<HttpRequest> request,
        AsyncHttpCallback callback
    );

    /**
     * @brief 取消正在进行的请求
     */
    void cancel();

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace DearTs::Core::Network
