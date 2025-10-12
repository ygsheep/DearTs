#pragma once

#include <string>
#include <vector>
#include <regex>
#include <chrono>
#include <imgui.h>

namespace DearTs::Core::Window::Widgets::Clipboard {

/**
 * @brief URL信息结构
 */
struct UrlInfo {
    std::string url;                                             // 完整URL
    std::string title;                                           // URL标题（如果可以获取）
    std::string domain;                                          // 域名
    std::string protocol;                                        // 协议（http/https等）
    size_t start_pos;                                            // 在原文中的起始位置
    size_t end_pos;                                              // 在原文中的结束位置
    std::chrono::system_clock::time_point discovered_time;      // 发现时间

    // URL类型
    enum class Type {
        HTTP,
        HTTPS,
        FTP,
        EMAIL,
        FILE,
        OTHER
    } type;

    // UI交互相关成员
    ImVec2 position;                                             // 渲染位置
    ImVec2 size;                                                 // 渲染大小
    bool is_hovered = false;                                     // 是否悬停
    bool is_selected = false;                                    // 是否选中
    ImVec4 bg_color;                                             // 背景颜色
    ImVec4 border_color;                                         // 边框颜色
    ImVec4 text_color;                                           // 文本颜色
    float border_width = 1.0f;                                   // 边框宽度
    int index;                                                   // 在原文中的索引

    // 便利构造函数
    UrlInfo() : start_pos(0), end_pos(0), type(Type::OTHER) {
        // 初始化UI交互状态
        position = ImVec2(0, 0);
        size = ImVec2(0, 0);
        bg_color = ImVec4(0, 0, 0, 0);
        border_color = ImVec4(0, 0, 0, 0);
        text_color = ImVec4(1, 1, 1, 1); // 默认白色文本
        border_width = 1.0f;
        index = 0;
        discovered_time = std::chrono::system_clock::now();
    }
};

/**
 * @brief URL提取器类
 *
 * 负责从文本中提取URL链接，支持多种URL格式和协议。
 * 提供URL验证、域名提取、类型识别等功能。
 */
class UrlExtractor {
public:
    /**
     * @brief 构造函数
     */
    UrlExtractor();

    /**
     * @brief 析构函数
     */
    ~UrlExtractor() = default;

    /**
     * @brief 从文本中提取所有URL
     * @param text 要分析的文本
     * @return 提取到的URL信息列表
     */
    std::vector<UrlInfo> extractUrls(const std::string& text);

    /**
     * @brief 验证URL是否有效
     * @param url 要验证的URL
     * @return 是否为有效URL
     */
    bool isValidUrl(const std::string& url);

    /**
     * @brief 从URL中提取域名
     * @param url URL字符串
     * @return 域名，如果无法提取则返回空字符串
     */
    std::string extractDomain(const std::string& url);

    /**
     * @brief 获取URL的类型
     * @param url URL字符串
     * @return URL类型
     */
    UrlInfo::Type getUrlType(const std::string& url);

    /**
     * @brief 标准化URL格式
     * @param url 原始URL
     * @return 标准化后的URL
     */
    std::string normalizeUrl(const std::string& url);

    /**
     * @brief 获取URL的协议
     * @param url URL字符串
     * @return 协议字符串，如果没有协议则返回空字符串
     */
    std::string extractProtocol(const std::string& url);

    /**
     * @brief 移除URL中的追踪参数
     * @param url 原始URL
     * @return 清理后的URL
     */
    std::string cleanUrl(const std::string& url);

    /**
     * @brief 检查URL是否为常见网站
     * @param url URL字符串
     * @return 是否为常见网站
     */
    bool isCommonWebsite(const std::string& url);

    /**
     * @brief 获取URL的简短显示名称
     * @param url URL字符串
     * @param max_length 最大显示长度
     * @return 简短显示名称
     */
    std::string getDisplayName(const std::string& url, size_t max_length = 50);

private:
    /**
     * @brief 正则表达式匹配URL
     * @param text 文本
     * @param regex 正则表达式
     * @param type URL类型
     * @return 匹配的URL信息列表
     */
    std::vector<UrlInfo> extractUrlsWithRegex(const std::string& text,
                                              const std::regex& regex,
                                              UrlInfo::Type type);

    /**
     * @brief 从正则匹配结果创建UrlInfo
     * @param match 正则匹配结果
     * @param text 原始文本
     * @param type URL类型
     * @return UrlInfo对象
     */
    UrlInfo createUrlInfoFromMatch(const std::smatch& match,
                                  const std::string& text,
                                  UrlInfo::Type type);

    /**
     * @brief 计算URL在原文中的位置
     * @param text 原始文本
     * @param url URL字符串
     * @return 起始位置
     */
    size_t findUrlPosition(const std::string& text, const std::string& url);

    /**
     * @brief 验证域名格式
     * @param domain 域名字符串
     * @return 是否为有效域名
     */
    bool isValidDomain(const std::string& domain);

    /**
     * @brief 清理域名（移除www前缀等）
     * @param domain 原始域名
     * @return 清理后的域名
     */
    std::string cleanDomain(const std::string& domain);

    // 常用域名列表（用于识别常见网站）
    static const std::vector<std::string> COMMON_DOMAINS;

    // 正则表达式模式
    static const std::regex HTTP_URL_REGEX;        // HTTP/HTTPS URL
    static const std::regex FTP_URL_REGEX;         // FTP URL
    static const std::regex EMAIL_REGEX;           // 邮箱地址
    static const std::regex FILE_URL_REGEX;        // 文件URL
};

} // namespace DearTs::Core::Window::Widgets::Clipboard