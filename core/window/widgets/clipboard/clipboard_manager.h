#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <mutex>
#include <functional>
#include "clipboard_monitor.h"
#include "url_extractor.h"

namespace DearTs::Core::Window::Widgets::Clipboard {

/**
 * @brief 剪切板项目结构
 */
struct ClipboardItem {
    std::string id;                              // 唯一标识符
    std::string content;                         // 内容
    std::chrono::system_clock::time_point timestamp; // 时间戳
    size_t content_length;                       // 内容长度
    std::vector<UrlInfo> urls;                   // 提取的URL
    bool is_favorite;                            // 是否收藏
    std::string category;                        // 分类标签

    ClipboardItem() : content_length(0), is_favorite(false) {}

    // 便利构造函数
    ClipboardItem(const std::string& content)
        : content(content)
        , timestamp(std::chrono::system_clock::now())
        , content_length(content.length())
        , is_favorite(false) {
        // 生成ID
        id = std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
            timestamp.time_since_epoch()).count());
    }
};

/**
 * @brief 剪切板管理器类
 *
 * 统一管理剪切板历史记录，提供监听、存储、检索等功能。
 * 整合了剪切板监听器和URL提取器。
 */
class ClipboardManager {
public:
    /**
     * @brief 剪切板变化回调函数类型
     */
    using ClipboardChangeCallback = std::function<void(const ClipboardItem& item)>;

    /**
     * @brief 构造函数
     */
    ClipboardManager();

    /**
     * @brief 析构函数
     */
    ~ClipboardManager();

    /**
     * @brief 初始化剪切板管理器
     * @param hwnd 接收剪切板消息的窗口句柄
     * @return 是否初始化成功
     */
    bool initialize(HWND hwnd);

    /**
     * @brief 关闭剪切板管理器
     */
    void shutdown();

    /**
     * @brief 设置剪切板变化回调
     * @param callback 回调函数
     */
    void setChangeCallback(ClipboardChangeCallback callback);

    /**
     * @brief 获取剪切板历史记录
     * @param limit 最大返回数量，0表示全部
     * @return 历史记录列表
     */
    std::vector<ClipboardItem> getHistory(size_t limit = 0);

    /**
     * @brief 搜索剪切板历史
     * @param keyword 搜索关键词
     * @param limit 最大返回数量
     * @return 匹配的历史记录
     */
    std::vector<ClipboardItem> searchHistory(const std::string& keyword, size_t limit = 50);

    /**
     * @brief 根据分类获取历史记录
     * @param category 分类名称
     * @param limit 最大返回数量
     * @return 指定分类的历史记录
     */
    std::vector<ClipboardItem> getHistoryByCategory(const std::string& category, size_t limit = 50);

    /**
     * @brief 清空历史记录
     */
    void clearHistory();

    /**
     * @brief 删除指定的历史记录项
     * @param id 要删除的项目ID
     * @return 是否删除成功
     */
    bool removeItem(const std::string& id);

    /**
     * @brief 设置项目收藏状态
     * @param id 项目ID
     * @param favorite 是否收藏
     * @return 是否设置成功
     */
    bool setFavorite(const std::string& id, bool favorite);

    /**
     * @brief 设置项目分类
     * @param id 项目ID
     * @param category 分类名称
     * @return 是否设置成功
     */
    bool setCategory(const std::string& id, const std::string& category);

    /**
     * @brief 获取收藏的项目
     * @param limit 最大返回数量
     * @return 收藏的项目列表
     */
    std::vector<ClipboardItem> getFavorites(size_t limit = 50);

    /**
     * @brief 获取所有分类
     * @return 分类列表
     */
    std::vector<std::string> getCategories();

    /**
     * @brief 获取统计信息
     */
    struct Statistics {
        size_t total_items;              // 总项目数
        size_t favorite_items;           // 收藏项目数
        size_t total_urls;               // 总URL数
        size_t total_text_length;        // 总文本长度
        std::chrono::system_clock::time_point last_update; // 最后更新时间
    };

    /**
     * @brief 获取统计信息
     * @return 统计信息
     */
    Statistics getStatistics();

    /**
     * @brief 获取当前剪切板内容
     * @return 当前内容
     */
    std::string getCurrentContent();

    /**
     * @brief 设置内容到系统剪切板
     * @param content 要设置的内容
     * @return 是否设置成功
     */
    bool setContent(const std::string& content);

    /**
     * @brief 获取管理器实例（单例）
     * @return 管理器实例引用
     */
    static ClipboardManager& getInstance();

private:
    /**
     * @brief 剪切板变化处理函数
     * @param content 新的剪切板内容
     */
    void onClipboardChanged(const std::string& content);

    /**
     * @brief 添加剪切板项目到历史记录
     * @param content 剪切板内容
     * @return 创建的项目
     */
    ClipboardItem addClipboardItem(const std::string& content);

    /**
     * @brief 处理剪切板项目（提取URL等）
     * @param item 要处理的项目
     */
    void processClipboardItem(ClipboardItem& item);

    /**
     * @brief 检查内容是否重复
     * @param content 要检查的内容
     * @return 是否重复
     */
    bool isDuplicateContent(const std::string& content);

    /**
     * @brief 保存历史记录到文件
     * @return 是否保存成功
     */
    bool saveHistory();

    /**
     * @brief 从文件加载历史记录
     * @return 是否加载成功
     */
    bool loadHistory();

    /**
     * @brief 获取历史记录文件路径
     * @return 文件路径
     */
    std::string getHistoryFilePath();

    /**
     * @brief 限制历史记录数量
     */
    void limitHistorySize();

    /**
     * @brief 生成唯一ID
     * @return 唯一ID字符串
     */
    std::string generateId();

    /**
     * @brief 格式化时间显示
     * @param time_point 时间点
     * @return 格式化的时间字符串
     */
    std::string formatTime(const std::chrono::system_clock::time_point& time_point);

    // 成员变量
    std::unique_ptr<ClipboardMonitor> monitor_;     // 剪切板监听器
    std::unique_ptr<UrlExtractor> url_extractor_; // URL提取器
    std::vector<ClipboardItem> history_;           // 历史记录
    ClipboardChangeCallback change_callback_;       // 变化回调

    mutable std::mutex history_mutex_;             // 历史记录保护锁
    bool is_initialized_;                          // 是否已初始化
    size_t max_history_size_;                      // 最大历史记录数量

    static ClipboardManager* instance_;             // 单例实例

    // 配置
    static const size_t DEFAULT_MAX_HISTORY = 1000; // 默认最大历史记录数
    static const size_t MAX_CONTENT_LENGTH = 100000; // 最大内容长度
};

} // namespace DearTs::Core::Window::Widgets::Clipboard