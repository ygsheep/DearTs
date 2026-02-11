/**
 * @file http_client.cpp
 * @brief 基于 Boost.Asio + Boost.Beast 的 HTTP 客户端实现
 *        支持同步和异步请求
 */

#include "http_client.hpp"

#ifdef DEARTS_BOOST_ASIO_SUPPORT

// 防止 Windows.h 宏冲突
#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
#endif

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <regex>
#include <thread>
#include <atomic>

// 检查是否有 OpenSSL (用于 HTTPS 支持)
#if __has_include(<openssl/ssl.h>)
    #define HAVE_OPENSSL 1
    #include <boost/beast/ssl.hpp>
#else
    #define HAVE_OPENSSL 0
#endif

// 取消可能干扰 logger.h 的 Windows 宏
#ifdef _WIN32
    #ifdef ERROR
        #undef ERROR
    #endif
    #ifdef DEBUG
        #undef DEBUG
    #endif
    #ifdef INFO
        #undef INFO
    #endif
    #ifdef WARNING
        #undef WARNING
    #endif
#endif

#include "liblogger/logger.h"

namespace DearTs::Core::Network {

// 在命名空间内声明别名
namespace beast = boost::beast;
namespace http = beast::http;
namespace asio = boost::asio;
namespace bsys = boost::system;
using tcp = asio::ip::tcp;

// 注意：不在这里定义 tcp_stream 和 ssl_stream 的别名，避免与 Boost.Beast 内部代码冲突
// 直接使用完整的 beast::tcp_stream 和 beast::ssl_stream<beast::tcp_stream> 类型名

#if HAVE_OPENSSL
namespace ssl = asio::ssl;
#endif

// Pimpl 实现类
class BoostAsioHttpClient::Impl {
public:
    std::string m_base_url;
    HttpClientConfig m_config;
    std::atomic<bool> m_cancelled{false};

    Impl(const std::string& base_url, const HttpClientConfig& config)
        : m_base_url(base_url)
        , m_config(config)
    {
        LOG_DEBUG("BoostAsioHttpClient::Impl created for: {}", base_url);
    }

    ~Impl() {
        LOG_DEBUG("BoostAsioHttpClient::Impl destroyed");
    }

    /**
     * @brief 解析 URL，提取主机、端口和协议
     */
    bool parse_url(const std::string& url, std::string& host, int& port, bool& use_https) {
        std::regex url_regex(R"(^(https?)://([^:/]+)(?::(\d+))?(/.*)?$)");
        std::smatch match;

        if (!std::regex_match(url, match, url_regex)) {
            return false;
        }

        use_https = (match[1].str() == "https");
        host = match[2].str();

        if (match[3].matched) {
            port = std::stoi(match[3].str());
        } else {
            port = use_https ? 443 : 80;
        }

        return true;
    }

    /**
     * @brief 在 io_context 线程中执行异步 HTTP 请求
     * @note 不再使用持久化的 io_context，而是为每次请求创建临时 io_context
     */
    void execute_async_http_request(
        std::shared_ptr<HttpRequest> req,
        const std::string& host,
        int port,
        AsyncHttpCallback callback
    ) {
        // 在新线程中执行同步请求（简化实现，避免复杂的异步生命周期管理）
        std::thread([this, req, host, port, callback]() {
            // 创建临时 io_context
            asio::io_context temp_io_ctx;
            beast::tcp_stream stream(temp_io_ctx);

            // 解析并连接
            auto const results = tcp::resolver(temp_io_ctx).resolve(host, std::to_string(port));
            stream.connect(results);
            stream.expires_after(m_config.request_timeout);

            // 构建请求
            http::request<http::string_body> http_req;
            http_req.version(11);

            if (req->method == "GET") {
                http_req.method(http::verb::get);
            } else if (req->method == "POST") {
                http_req.method(http::verb::post);
            } else {
                callback(DearTs::Core::Result<HttpResponse, std::string>::err("Unsupported HTTP method"));
                return;
            }

            http_req.target(req->endpoint);
            http_req.set(http::field::host, host);
            http_req.set(http::field::user_agent, m_config.user_agent);
            http_req.body() = req->body;

            for (const auto& [key, value] : req->headers) {
                http_req.set(key, value);
            }

            if (!req->body.empty()) {
                http_req.set(http::field::content_length, std::to_string(req->body.length()));
            }

            HttpResponse response;
            beast::flat_buffer buffer;
            http::response<http::dynamic_body> http_res;
            bsys::error_code ec;

            if (req->on_chunk) {
                // 流式处理
                http::response_parser<http::dynamic_body> parser;
                parser.eager(true);
                http::read_header(stream, buffer, parser);
                response.status_code = parser.get().result_int();

                while (!parser.is_done() && !m_cancelled) {
                    std::size_t bytes_read = http::read_some(stream, buffer, parser);
                    if (bytes_read == 0) break;

                    const auto& body = parser.get().body();
                    const auto buffers = body.data();
                    std::string chunk(boost::asio::buffers_begin(buffers), boost::asio::buffers_end(buffers));

                    if (!chunk.empty() && req->on_chunk) {
                        req->on_chunk(chunk);
                    }
                }

                if (m_cancelled) {
                    stream.socket().shutdown(tcp::socket::shutdown_both);
                    callback(DearTs::Core::Result<HttpResponse, std::string>::err("Request cancelled"));
                    return;
                }
            } else {
                // 非流式
                http::read(stream, buffer, http_res, ec);
                if (ec) {
                    response.error_message = std::format("Failed to read response: {}", ec.message());
                    callback(DearTs::Core::Result<HttpResponse, std::string>::err(response.error_message));
                    return;
                }
                response.status_code = http_res.result_int();
                response.body = beast::buffers_to_string(http_res.body().data());
            }

            // 优雅关闭
            bsys::error_code ec_shutdown;
            stream.socket().shutdown(tcp::socket::shutdown_both, ec_shutdown);

            callback(DearTs::Core::Result<HttpResponse, std::string>::ok(response));
        }).detach();
    }

    /**
     * @brief 执行 HTTP 请求（同步，保持向后兼容）
     */
    DearTs::Core::Result<HttpResponse, std::string> execute_http_request(
        const HttpRequest& req,
        const std::string& host,
        int port
    ) {
        HttpResponse response;

        // 创建临时 io_context 用于同步请求
        asio::io_context sync_io_ctx;
        beast::tcp_stream stream(sync_io_ctx);

        auto const results = tcp::resolver(sync_io_ctx).resolve(host, std::to_string(port));
        stream.connect(results);

        // 设置超时
        stream.expires_after(m_config.request_timeout);

        beast::flat_buffer buffer;
        http::request<http::string_body> http_req;

        http_req.version(11);
        if (req.method == "GET") {
            http_req.method(http::verb::get);
        } else if (req.method == "POST") {
            http_req.method(http::verb::post);
        } else {
            return DearTs::Core::Result<HttpResponse, std::string>::err("Unsupported HTTP method");
        }

        http_req.target(req.endpoint);
        http_req.set(http::field::host, host);
        http_req.set(http::field::user_agent, m_config.user_agent);
        http_req.body() = req.body;

        for (const auto& [key, value] : req.headers) {
            http_req.set(key, value);
        }

        if (!req.body.empty()) {
            http_req.set(http::field::content_length, std::to_string(req.body.length()));
        }

        // 发送请求
        http::write(stream, http_req);

        // 接收响应
        http::response<http::dynamic_body> http_res;
        bsys::error_code ec;

        // 如果有流式回调，逐块读取
        if (req.on_chunk) {
            beast::flat_buffer chunk_buffer;
            http::response_parser<http::dynamic_body> parser;
            parser.eager(true);

            // 读取响应头
            http::read_header(stream, chunk_buffer, parser);
            const auto& header_res = parser.get();

            response.status_code = header_res.result_int();

            // 读取响应体（逐块）
            std::size_t bytes_read = 0;
            while (!parser.is_done() && !m_cancelled) {
                bytes_read = http::read_some(stream, chunk_buffer, parser);
                if (bytes_read == 0) break;

                // 获取当前已读取的数据
                const auto& body = parser.get().body();
                const auto buffers = body.data();
                std::string chunk;
                auto buffer_ptr = boost::asio::buffers_begin(buffers);
                auto buffer_end = boost::asio::buffers_end(buffers);
                chunk.assign(buffer_ptr, buffer_end);

                // 调用流式回调
                if (!chunk.empty()) {
                    req.on_chunk(chunk);
                }
            }

            if (m_cancelled) {
                stream.socket().shutdown(tcp::socket::shutdown_both);
                return DearTs::Core::Result<HttpResponse, std::string>::err("Request cancelled");
            }
        } else {
            // 非流式：一次性读取完整响应
            http::read(stream, buffer, http_res, ec);
            if (ec) {
                return DearTs::Core::Result<HttpResponse, std::string>::err(
                    std::format("Failed to read response: {}", ec.message())
                );
            }

            response.status_code = http_res.result_int();
            response.body = beast::buffers_to_string(http_res.body().data());
        }

        // 优雅关闭连接
        bsys::error_code ec_shutdown;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec_shutdown);

        return DearTs::Core::Result<HttpResponse, std::string>::ok(response);
    }

#if HAVE_OPENSSL
    /**
     * @brief 执行异步 HTTPS 请求（SSL）
     */
    void execute_async_https_request(
        std::shared_ptr<HttpRequest> req,
        const std::string& host,
        int port,
        AsyncHttpCallback callback
    ) {
        // HTTPS 异步实现（与 HTTP 类似，但使用 SSL stream）
        // 暂未实现，回退到同步
        auto result = execute_https_request(*req, host, port);
        callback(result);
    }

    /**
     * @brief 执行 HTTPS 请求（同步）
     */
    DearTs::Core::Result<HttpResponse, std::string> execute_https_request(
        const HttpRequest& req,
        const std::string& host,
        int port
    ) {
        HttpResponse response;

        // SSL 上下文
        ssl::context ctx(ssl::context::tlsv12_client);

        if (!m_config.verify_ssl) {
            ctx.set_verify_mode(ssl::verify_none);
        }

        // 创建临时 io_context 用于同步请求
        asio::io_context sync_io_ctx;

        // 解析主机地址
        auto const results = tcp::resolver(sync_io_ctx).resolve(host, std::to_string(port));

        // 创建 SSL stream (直接用 io_context 和 ssl context 构造)
        beast::ssl_stream<beast::tcp_stream> ssl_stream(sync_io_ctx, ctx);

        // 设置 SNI hostname
        if (!SSL_set_tlsext_host_name(ssl_stream.native_handle(), host.c_str())) {
            bsys::error_code ec{static_cast<int>(::ERR_get_error()), asio::error::get_ssl_category()};
            return DearTs::Core::Result<HttpResponse, std::string>::err(
                std::format("Failed to set SNI hostname: {}", ec.message())
            );
        }

        // 连接到服务器
        beast::get_lowest_layer(ssl_stream).connect(results);

        // 执行 SSL 握手
        ssl_stream.handshake(ssl::stream_base::client);

        // 构建请求（与 HTTP 版本相同）
        beast::flat_buffer buffer;
        http::request<http::string_body> http_req;

        http_req.version(11);
        if (req.method == "GET") {
            http_req.method(http::verb::get);
        } else if (req.method == "POST") {
            http_req.method(http::verb::post);
        } else {
            return DearTs::Core::Result<HttpResponse, std::string>::err("Unsupported HTTP method");
        }

        http_req.target(req.endpoint);
        http_req.set(http::field::host, host);
        http_req.set(http::field::user_agent, m_config.user_agent);
        http_req.body() = req.body;

        for (const auto& [key, value] : req.headers) {
            http_req.set(key, value);
        }

        if (!req.body.empty()) {
            http_req.set(http::field::content_length, std::to_string(req.body.length()));
        }

        // 发送请求
        http::write(ssl_stream, http_req);

        // 接收响应
        http::response<http::dynamic_body> http_res;
        bsys::error_code ec;

        if (req.on_chunk) {
            // 流式响应（与 HTTP 版本相同逻辑）
            beast::flat_buffer chunk_buffer;
            http::response_parser<http::dynamic_body> parser;
            parser.eager(true);

            http::read_header(ssl_stream, chunk_buffer, parser);
            const auto& header_res = parser.get();

            response.status_code = header_res.result_int();

            std::size_t bytes_read = 0;
            while (!parser.is_done() && !m_cancelled) {
                bytes_read = http::read_some(ssl_stream, chunk_buffer, parser);
                if (bytes_read == 0) break;

                const auto& body = parser.get().body();
                const auto buffers = body.data();
                std::string chunk;
                auto buffer_ptr = boost::asio::buffers_begin(buffers);
                auto buffer_end = boost::asio::buffers_end(buffers);
                chunk.assign(buffer_ptr, buffer_end);

                if (!chunk.empty()) {
                    req.on_chunk(chunk);
                }
            }

            if (m_cancelled) {
                beast::get_lowest_layer(ssl_stream).socket().shutdown(tcp::socket::shutdown_both);
                return DearTs::Core::Result<HttpResponse, std::string>::err("Request cancelled");
            }
        } else {
            http::read(ssl_stream, buffer, http_res, ec);
            if (ec) {
                return DearTs::Core::Result<HttpResponse, std::string>::err(
                    std::format("Failed to read response: {}", ec.message())
                );
            }

            response.status_code = http_res.result_int();
            response.body = beast::buffers_to_string(http_res.body().data());
        }

        // 优雅关闭
        bsys::error_code ec_shutdown;
        beast::get_lowest_layer(ssl_stream).socket().shutdown(tcp::socket::shutdown_both, ec_shutdown);

        return DearTs::Core::Result<HttpResponse, std::string>::ok(response);
    }
#endif // HAVE_OPENSSL
};

// BoostAsioHttpClient 实现

BoostAsioHttpClient::BoostAsioHttpClient(
    const std::string& base_url,
    const HttpClientConfig& config
)
    : m_impl(std::make_unique<Impl>(base_url, config))
{
    LOG_DEBUG("BoostAsioHttpClient created for: {}", base_url);
}

// 拷贝构造函数 - 创建新的客户端实例
BoostAsioHttpClient::BoostAsioHttpClient(const BoostAsioHttpClient& other)
    : m_impl(std::make_unique<Impl>(other.m_impl->m_base_url, other.m_impl->m_config))
{
    LOG_DEBUG("BoostAsioHttpClient copied for: {}", m_impl->m_base_url);
}

// 拷贝赋值运算符
BoostAsioHttpClient& BoostAsioHttpClient::operator=(const BoostAsioHttpClient& other) {
    if (this != &other) {
        m_impl = std::make_unique<Impl>(other.m_impl->m_base_url, other.m_impl->m_config);
        LOG_DEBUG("BoostAsioHttpClient copy-assigned for: {}", m_impl->m_base_url);
    }
    return *this;
}

BoostAsioHttpClient::~BoostAsioHttpClient() {
    LOG_DEBUG("BoostAsioHttpClient destroyed");
}

DearTs::Core::Result<HttpResponse, std::string> BoostAsioHttpClient::request(
    const HttpRequest& req
) {
    m_impl->m_cancelled = false;

    // 解析 URL
    std::string host;
    int port;
    bool use_https;

    if (!m_impl->parse_url(m_impl->m_base_url, host, port, use_https)) {
        return DearTs::Core::Result<HttpResponse, std::string>::err(
            std::format("Invalid base_url: {}", m_impl->m_base_url)
        );
    }

    LOG_DEBUG("HTTP request: {} {}:{}{}", use_https ? "HTTPS" : "HTTP", host, port, req.endpoint);

    // 执行请求
    if (use_https) {
#if HAVE_OPENSSL
        auto result = m_impl->execute_https_request(req, host, port);
        if (result.isOk()) {
            LOG_DEBUG("HTTP response: status={}", result.unwrap().status_code);
        }
        return result;
#else
        return DearTs::Core::Result<HttpResponse, std::string>::err(
            "HTTPS support is not available. OpenSSL was not found during compilation. "
            "Either use HTTP URLs or install OpenSSL and rebuild."
        );
#endif
    } else {
        auto result = m_impl->execute_http_request(req, host, port);
        if (result.isOk()) {
            LOG_DEBUG("HTTP response: status={}", result.unwrap().status_code);
        }
        return result;
    }
}

void BoostAsioHttpClient::request_async(
    std::shared_ptr<HttpRequest> req,  // 使用 shared_ptr 避免复制
    AsyncHttpCallback callback
) {
    m_impl->m_cancelled = false;

    // 解析 URL
    std::string host;
    int port;
    bool use_https;

    if (!m_impl->parse_url(m_impl->m_base_url, host, port, use_https)) {
        std::string error_msg = std::format("Invalid base_url: {}", m_impl->m_base_url);
        callback(DearTs::Core::Result<HttpResponse, std::string>::err(error_msg));
        return;
    }

    LOG_DEBUG("Async HTTP request: {} {}:{}{}", use_https ? "HTTPS" : "HTTP", host, port, req->endpoint);

    // 执行异步请求（传递 shared_ptr，避免复制）
    if (use_https) {
#if HAVE_OPENSSL
        m_impl->execute_async_https_request(req, host, port, callback);
#else
        std::string error_msg = "HTTPS support is not available";
        callback(DearTs::Core::Result<HttpResponse, std::string>::err(error_msg));
#endif
    } else {
        m_impl->execute_async_http_request(req, host, port, callback);
    }
}

void BoostAsioHttpClient::cancel() {
    m_impl->m_cancelled = true;
}

} // namespace DearTs::Core::Network

#endif // DEARTS_BOOST_ASIO_SUPPORT
