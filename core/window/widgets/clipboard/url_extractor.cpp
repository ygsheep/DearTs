#include "url_extractor.h"
#include "../../utils/logger.h"
#include <algorithm>
#include <sstream>
#include <cctype>

namespace DearTs::Core::Window::Widgets::Clipboard {

// 静态成员初始化
const std::vector<std::string> UrlExtractor::COMMON_DOMAINS = {
    "google.com", "youtube.com", "facebook.com", "twitter.com", "instagram.com",
    "linkedin.com", "github.com", "stackoverflow.com", "reddit.com", "wikipedia.org",
    "amazon.com", "taobao.com", "tmall.com", "jd.com", "baidu.com", "qq.com",
    "weibo.com", "zhihu.com", "csdn.net", "juejin.cn", "jianshu.com"
};

// 正则表达式模式定义
const std::regex UrlExtractor::HTTP_URL_REGEX(
    R"(https?:\/\/(?:www\.)?[-a-zA-Z0-9@:%._\+~#=]{1,256}\.[a-zA-Z0-9()]{1,6}\b(?:[-a-zA-Z0-9()@:%_\+.~#?&//=]*))"
);

const std::regex UrlExtractor::FTP_URL_REGEX(
    R"(ftp:\/\/(?:www\.)?[-a-zA-Z0-9@:%._\+~#=]{1,256}\.[a-zA-Z0-9()]{1,6}\b(?:[-a-zA-Z0-9()@:%_\+.~#?&//=]*))"
);

const std::regex UrlExtractor::EMAIL_REGEX(
    R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})"
);

const std::regex UrlExtractor::FILE_URL_REGEX(
    R"(file:\/\/\/[a-zA-Z]:\/(?:[^\\/:*?"<>|\r\n]+\/)*[^\\/:*?"<>|\r\n]*)"
);

UrlExtractor::UrlExtractor() {
    DEARTS_LOG_INFO("UrlExtractor构造函数");
}

std::vector<UrlInfo> UrlExtractor::extractUrls(const std::string& text) {
    DEARTS_LOG_INFO("开始从文本中提取URL，文本长度: " + std::to_string(text.length()));

    std::vector<UrlInfo> all_urls;

    // 提取HTTP/HTTPS URL
    auto http_urls = extractUrlsWithRegex(text, HTTP_URL_REGEX, UrlInfo::Type::HTTPS);
    all_urls.insert(all_urls.end(), http_urls.begin(), http_urls.end());

    // 提取FTP URL
    auto ftp_urls = extractUrlsWithRegex(text, FTP_URL_REGEX, UrlInfo::Type::FTP);
    all_urls.insert(all_urls.end(), ftp_urls.begin(), ftp_urls.end());

    // 提取邮箱地址
    auto emails = extractUrlsWithRegex(text, EMAIL_REGEX, UrlInfo::Type::EMAIL);
    all_urls.insert(all_urls.end(), emails.begin(), emails.end());

    // 提取文件URL
    auto file_urls = extractUrlsWithRegex(text, FILE_URL_REGEX, UrlInfo::Type::FILE);
    all_urls.insert(all_urls.end(), file_urls.begin(), file_urls.end());

    // 去重和排序
    std::sort(all_urls.begin(), all_urls.end(),
              [](const UrlInfo& a, const UrlInfo& b) {
                  return a.start_pos < b.start_pos;
              });

    // 移除重复的URL
    auto last = std::unique(all_urls.begin(), all_urls.end(),
                           [](const UrlInfo& a, const UrlInfo& b) {
                               return a.url == b.url;
                           });
    all_urls.erase(last, all_urls.end());

    DEARTS_LOG_INFO("URL提取完成，共找到 " + std::to_string(all_urls.size()) + " 个URL");

    return all_urls;
}

std::vector<UrlInfo> UrlExtractor::extractUrlsWithRegex(const std::string& text,
                                                        const std::regex& regex,
                                                        UrlInfo::Type type) {
    std::vector<UrlInfo> urls;
    std::sregex_iterator iter(text.begin(), text.end(), regex);
    std::sregex_iterator end;

    for (; iter != end; ++iter) {
        UrlInfo url_info = createUrlInfoFromMatch(*iter, text, type);
        if (isValidUrl(url_info.url)) {
            urls.push_back(url_info);
        }
    }

    return urls;
}

UrlInfo UrlExtractor::createUrlInfoFromMatch(const std::smatch& match,
                                            const std::string& text,
                                            UrlInfo::Type type) {
    UrlInfo url_info;
    url_info.url = match.str();
    url_info.type = type;
    url_info.discovered_time = std::chrono::system_clock::now();

    // 计算位置
    url_info.start_pos = match.position();
    url_info.end_pos = match.position() + match.length();

    // 提取协议和域名
    url_info.protocol = extractProtocol(url_info.url);
    url_info.domain = extractDomain(url_info.url);

    // 标准化URL
    url_info.url = normalizeUrl(url_info.url);

    return url_info;
}

bool UrlExtractor::isValidUrl(const std::string& url) {
    if (url.empty() || url.length() > 2048) {
        return false;
    }

    // 检查协议
    std::string protocol = extractProtocol(url);
    if (protocol.empty()) {
        return false; // 必须有协议
    }

    // 检查域名
    std::string domain = extractDomain(url);
    if (!isValidDomain(domain)) {
        return false;
    }

    return true;
}

std::string UrlExtractor::extractDomain(const std::string& url) {
    std::string domain = url;

    // 移除协议
    size_t protocol_end = url.find("://");
    if (protocol_end != std::string::npos) {
        domain = url.substr(protocol_end + 3);
    }

    // 移除路径
    size_t path_start = domain.find('/');
    if (path_start != std::string::npos) {
        domain = domain.substr(0, path_start);
    }

    // 移除端口
    size_t port_start = domain.find(':');
    if (port_start != std::string::npos) {
        domain = domain.substr(0, port_start);
    }

    // 移除用户信息
    size_t user_end = domain.find('@');
    if (user_end != std::string::npos) {
        domain = domain.substr(user_end + 1);
    }

    return cleanDomain(domain);
}

std::string UrlExtractor::extractProtocol(const std::string& url) {
    size_t protocol_end = url.find("://");
    if (protocol_end != std::string::npos) {
        return url.substr(0, protocol_end);
    }
    return "";
}

UrlInfo::Type UrlExtractor::getUrlType(const std::string& url) {
    std::string protocol = extractProtocol(url);
    std::string lower_protocol = protocol;
    std::transform(lower_protocol.begin(), lower_protocol.end(), lower_protocol.begin(), ::tolower);

    if (lower_protocol == "http") {
        return UrlInfo::Type::HTTP;
    } else if (lower_protocol == "https") {
        return UrlInfo::Type::HTTPS;
    } else if (lower_protocol == "ftp") {
        return UrlInfo::Type::FTP;
    } else if (lower_protocol == "file") {
        return UrlInfo::Type::FILE;
    } else if (url.find('@') != std::string::npos && url.find('.') != std::string::npos) {
        return UrlInfo::Type::EMAIL;
    } else {
        return UrlInfo::Type::OTHER;
    }
}

std::string UrlExtractor::normalizeUrl(const std::string& url) {
    std::string normalized = url;

    // 确保协议小写
    size_t protocol_end = normalized.find("://");
    if (protocol_end != std::string::npos) {
        for (size_t i = 0; i < protocol_end; ++i) {
            normalized[i] = std::tolower(normalized[i]);
        }
    }

    // 移除尾部斜杠（除非是根路径）
    if (normalized.length() > 1 && normalized.back() == '/' &&
        normalized.find('/', normalized.find("://") + 3) != std::string::npos) {
        normalized.pop_back();
    }

    return normalized;
}

std::string UrlExtractor::cleanUrl(const std::string& url) {
    std::string cleaned = url;

    // 移除常见的追踪参数
    static const std::vector<std::string> tracking_params = {
        "?utm_source=", "?utm_medium=", "?utm_campaign=", "?utm_term=", "?utm_content=",
        "?fbclid=", "?gclid=", "?msclkid=", "?_ga=", "?_gid="
    };

    for (const auto& param : tracking_params) {
        size_t pos = cleaned.find(param);
        if (pos != std::string::npos) {
            size_t end = cleaned.find('&', pos);
            if (end != std::string::npos) {
                cleaned.erase(pos, end - pos + 1);
            } else {
                cleaned.erase(pos);
            }
        }
    }

    // 移除末尾的&或?
    if (!cleaned.empty() && (cleaned.back() == '&' || cleaned.back() == '?')) {
        cleaned.pop_back();
    }

    return cleaned;
}

bool UrlExtractor::isCommonWebsite(const std::string& url) {
    std::string domain = extractDomain(url);
    std::string lower_domain = domain;
    std::transform(lower_domain.begin(), lower_domain.end(), lower_domain.begin(), ::tolower);

    for (const auto& common_domain : COMMON_DOMAINS) {
        if (lower_domain.find(common_domain) != std::string::npos) {
            return true;
        }
    }

    return false;
}

std::string UrlExtractor::getDisplayName(const std::string& url, size_t max_length) {
    if (url.length() <= max_length) {
        return url;
    }

    // 尝试保留协议和域名
    std::string domain = extractDomain(url);
    std::string protocol = extractProtocol(url) + "://";

    if (domain.length() + protocol.length() + 10 <= max_length) {
        return protocol + domain + "/...";
    }

    // 如果还是太长，直接截断
    return url.substr(0, max_length - 3) + "...";
}

std::string UrlExtractor::cleanDomain(const std::string& domain) {
    std::string cleaned = domain;

    // 移除www前缀
    if (cleaned.substr(0, 4) == "www.") {
        cleaned = cleaned.substr(4);
    }

    // 转换为小写
    std::transform(cleaned.begin(), cleaned.end(), cleaned.begin(), ::tolower);

    return cleaned;
}

bool UrlExtractor::isValidDomain(const std::string& domain) {
    if (domain.empty() || domain.length() > 253) {
        return false;
    }

    // 基本格式检查
    static const std::regex domain_regex(
        R"([a-zA-Z0-9]([a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?(\.[a-zA-Z0-9]([a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?)*)"
    );

    return std::regex_match(domain, domain_regex);
}

size_t UrlExtractor::findUrlPosition(const std::string& text, const std::string& url) {
    size_t pos = text.find(url);
    return (pos == std::string::npos) ? 0 : pos;
}

} // namespace DearTs::Core::Window::Widgets::Clipboard