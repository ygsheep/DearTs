#include "clipboard_manager.h"
#include "../../utils/logger.h"
#include <algorithm>
#include <sstream>
#include <fstream>

namespace DearTs::Core::Window::Widgets::Clipboard {

// 静态成员初始化
ClipboardManager* ClipboardManager::instance_ = nullptr;

ClipboardManager& ClipboardManager::getInstance() {
    static ClipboardManager instance;
    return instance;
}

ClipboardManager::ClipboardManager()
    : max_history_size_(DEFAULT_MAX_HISTORY)
    , is_initialized_(false) {
    DEARTS_LOG_INFO("ClipboardManager构造函数");

    monitor_ = std::make_unique<ClipboardMonitor>();
    url_extractor_ = std::make_unique<UrlExtractor>();

    instance_ = this;
}

ClipboardManager::~ClipboardManager() {
    DEARTS_LOG_INFO("ClipboardManager析构函数");
    shutdown();
    if (instance_ == this) {
        instance_ = nullptr;
    }
}

bool ClipboardManager::initialize(HWND hwnd) {
    if (is_initialized_) {
        DEARTS_LOG_WARN("剪切板管理器已初始化");
        return true;
    }

    if (!monitor_->startMonitoring(hwnd)) {
        DEARTS_LOG_ERROR("剪切板监听器初始化失败");
        return false;
    }

    // 设置剪切板变化回调
    monitor_->setChangeCallback([this](const std::string& content) {
        onClipboardChanged(content);
    });

    is_initialized_ = true;
    DEARTS_LOG_INFO("剪切板管理器初始化成功");

    // 加载历史记录
    loadHistory();

    return true;
}

void ClipboardManager::shutdown() {
    if (!is_initialized_) {
        return;
    }

    monitor_->stopMonitoring();
    is_initialized_ = false;

    // 保存历史记录
    saveHistory();

    DEARTS_LOG_INFO("剪切板管理器已关闭");
}

void ClipboardManager::setChangeCallback(ClipboardChangeCallback callback) {
    change_callback_ = callback;
}

std::vector<ClipboardItem> ClipboardManager::getHistory(size_t limit) {
    std::lock_guard<std::mutex> lock(history_mutex_);

    std::vector<ClipboardItem> result = history_;

    if (limit == 0 || limit >= history_.size()) {
        result = history_;
    } else {
        result.assign(history_.end() - limit, history_.end());
    }

    // 按时间戳降序排列
    std::sort(result.begin(), result.end(),
              [](const ClipboardItem& a, const ClipboardItem& b) {
                  return a.timestamp > b.timestamp;
              });

    return result;
}

std::vector<ClipboardItem> ClipboardManager::searchHistory(const std::string& keyword, size_t limit) {
    std::lock_guard<std::mutex> lock(history_mutex_);

    std::vector<ClipboardItem> result;

    for (const auto& item : history_) {
        if (item.content.find(keyword) != std::string::npos) {
            result.push_back(item);
        }
    }

    // 按时间戳降序排列
    std::sort(result.begin(), result.end(),
              [](const ClipboardItem& a, const ClipboardItem& b) {
                  return a.timestamp > b.timestamp;
              });

    if (limit > 0 && result.size() > limit) {
        result.resize(limit);
    }

    return result;
}

std::vector<ClipboardItem> ClipboardManager::getHistoryByCategory(const std::string& category, size_t limit) {
    std::lock_guard<std::mutex> lock(history_mutex_);

    std::vector<ClipboardItem> result;

    for (const auto& item : history_) {
        if (item.category == category) {
            result.push_back(item);
        }
    }

    // 按时间戳降序排列
    std::sort(result.begin(), result.end(),
              [](const ClipboardItem& a, const ClipboardItem& b) {
                  return a.timestamp > b.timestamp;
              });

    if (limit > 0 && result.size() > limit) {
        result.resize(limit);
    }

    return result;
}

void ClipboardManager::clearHistory() {
    std::lock_guard<std::mutex> lock(history_mutex_);

    history_.clear();
    DEARTS_LOG_INFO("清空剪切板历史记录");

    saveHistory();
}

bool ClipboardManager::removeItem(const std::string& id) {
    std::lock_guard<std::mutex> lock(history_mutex_);

    auto it = std::find_if(history_.begin(), history_.end(),
                          [&id](const ClipboardItem& item) {
                              return item.id == id;
                          });

    if (it != history_.end()) {
        history_.erase(it);
        DEARTS_LOG_INFO("删除剪切板项目: " + id);
        saveHistory();
        return true;
    }

    return false;
}

bool ClipboardManager::setFavorite(const std::string& id, bool favorite) {
    std::lock_guard<std::mutex> lock(history_mutex_);

    auto it = std::find_if(history_.begin(), history_.end(),
                          [&id](const ClipboardItem& item) {
                              return item.id == id;
                          });

    if (it != history_.end()) {
        it->is_favorite = favorite;
        DEARTS_LOG_INFO("设置收藏状态: " + id + " -> " + (favorite ? "收藏" : "取消收藏"));
        saveHistory();
        return true;
    }

    return false;
}

bool ClipboardManager::setCategory(const std::string& id, const std::string& category) {
    std::lock_guard<std::mutex> lock(history_mutex_);

    auto it = std::find_if(history_.begin(), history_.end(),
                          [&id](const ClipboardItem& item) {
                              return item.id == id;
                          });

    if (it != history_.end()) {
        it->category = category;
        DEARTS_LOG_INFO("设置分类: " + id + " -> " + category);
        saveHistory();
        return true;
    }

    return false;
}

std::vector<ClipboardItem> ClipboardManager::getFavorites(size_t limit) {
    std::lock_guard<std::mutex> lock(history_mutex_);

    std::vector<ClipboardItem> result;

    std::copy_if(history_.begin(), history_.end(), std::back_inserter(result),
                  [](const ClipboardItem& item) { return item.is_favorite; });

    // 按时间戳降序排列
    std::sort(result.begin(), result.end(),
              [](const ClipboardItem& a, const ClipboardItem& b) {
                  return a.timestamp > b.timestamp;
              });

    if (limit > 0 && result.size() > limit) {
        result.resize(limit);
    }

    return result;
}

std::vector<std::string> ClipboardManager::getCategories() {
    std::lock_guard<std::mutex> lock(history_mutex_);

    std::vector<std::string> categories;

    for (const auto& item : history_) {
        if (!item.category.empty() &&
            std::find(categories.begin(), categories.end(), item.category) == categories.end()) {
            categories.push_back(item.category);
        }
    }

    return categories;
}

ClipboardManager::Statistics ClipboardManager::getStatistics() {
    std::lock_guard<std::mutex> lock(history_mutex_);

    Statistics stats;
    stats.total_items = history_.size();
    stats.favorite_items = std::count_if(history_.begin(), history_.end(),
                                    [](const ClipboardItem& item) { return item.is_favorite; });
    stats.total_urls = 0;
    stats.total_text_length = 0;

    for (const auto& item : history_) {
        stats.total_urls += item.urls.size();
        stats.total_text_length += item.content_length;
    }

    stats.last_update = std::chrono::system_clock::now();

    return stats;
}

std::string ClipboardManager::getCurrentContent() {
    return monitor_->getCurrentClipboardContent();
}

bool ClipboardManager::setContent(const std::string& content) {
    if (OpenClipboard(nullptr)) {
        EmptyClipboard();

        HGLOBAL hClipboardData = GlobalAlloc(GMEM_MOVEABLE, content.length() + 1);
        if (hClipboardData) {
            char* pchData = static_cast<char*>(GlobalLock(hClipboardData));
            if (pchData) {
                strcpy_s(pchData, content.length() + 1, content.c_str());
                GlobalUnlock(hClipboardData);
                SetClipboardData(CF_TEXT, hClipboardData);
                DEARTS_LOG_INFO("设置剪切板内容: " + content.substr(0, 50) + "...");
                CloseClipboard();
                return true;
            }
        }
        CloseClipboard();
    }

    return false;
}

void ClipboardManager::onClipboardChanged(const std::string& content) {
    if (content.empty()) {
        return;
    }

    // 检查是否为重复内容
    if (isDuplicateContent(content)) {
        DEARTS_LOG_DEBUG("忽略重复的剪切板内容");
        return;
    }

    // 添加新的剪切板项目
    ClipboardItem new_item = addClipboardItem(content);

    // 调用回调函数
    {
        std::lock_guard<std::mutex> lock(history_mutex_);
        if (change_callback_) {
            change_callback_(new_item);
        }
    }

    // 保存历史记录
    saveHistory();

    DEARTS_LOG_INFO("检测到新的剪切板内容: " + content.substr(0, 50) + "...");
}

ClipboardItem ClipboardManager::addClipboardItem(const std::string& content) {
    ClipboardItem item(content);

    // 处理剪切板项目（提取URL等）
    processClipboardItem(item);

    // 添加到历史记录
    {
        std::lock_guard<std::mutex> lock(history_mutex_);
        history_.push_back(item);

        // 限制历史记录数量
        limitHistorySize();
    }

    return item;
}

void ClipboardManager::processClipboardItem(ClipboardItem& item) {
    // 提取URL
    item.urls = url_extractor_->extractUrls(item.content);

    // 可以在这里添加其他处理逻辑，比如：
    // - 提取邮箱地址
    // - 检测文件路径
    // - 分析内容类型等

    DEARTS_LOG_DEBUG("处理剪切板项目: " + std::to_string(item.urls.size()) + " 个URL");
}

bool ClipboardManager::isDuplicateContent(const std::string& content) {
    std::lock_guard<std::mutex> lock(history_mutex_);

    // 检查最近的几个项目是否有相同内容
    size_t check_count = std::min(static_cast<size_t>(5), history_.size());

    for (auto it = history_.rbegin(); it != history_.rend() && check_count > 0; ++it, --check_count) {
        if (it->content == content) {
            return true;
        }
    }

    return false;
}

bool ClipboardManager::saveHistory() {
    try {
        std::string file_path = getHistoryFilePath();
        std::ofstream file(file_path, std::ios::out | std::ios::trunc);

        if (!file.is_open()) {
            DEARTS_LOG_ERROR("无法打开历史记录文件: " + file_path);
            return false;
        }

        // 写入历史记录数量
        file << history_.size() << std::endl;

        // 写入每个历史记录
        for (const auto& item : history_) {
            file << "ID:" << item.id << std::endl;
            file << "Content:" << item.content << std::endl;
            file << "Timestamp:" << std::chrono::duration_cast<std::chrono::milliseconds>(
                         item.timestamp.time_since_epoch()).count() << std::endl;
            file << "Length:" << item.content_length << std::endl;
            file << "Favorite:" << (item.is_favorite ? "1" : "0") << std::endl;
            file << "Category:" << item.category << std::endl;
            file << "URLCount:" << item.urls.size() << std::endl;

            // 写入URL信息
            for (const auto& url : item.urls) {
                file << "URL:" << url.url << std::endl;
            }

            file << "---" << std::endl;
        }

        file.close();
        DEARTS_LOG_INFO("保存剪切板历史记录到: " + file_path);
        return true;

    } catch (const std::exception& e) {
        DEARTS_LOG_ERROR("保存历史记录时发生异常: " + std::string(e.what()));
        return false;
    }
}

bool ClipboardManager::loadHistory() {
    try {
        std::string file_path = getHistoryFilePath();
        std::ifstream file(file_path);

        if (!file.is_open()) {
            DEARTS_LOG_INFO("历史记录文件不存在，使用空白历史: " + file_path);
            return false;
        }

        std::string line;
        ClipboardItem current_item;
        bool reading_item = false;

        while (std::getline(file, line)) {
            if (line.empty()) continue;

            if (line == "---") {
                // 项目分隔符
                if (reading_item) {
                    history_.push_back(current_item);
                    current_item = ClipboardItem();
                    reading_item = false;
                }
            } else if (line.substr(0, 3) == "ID:") {
                current_item.id = line.substr(3);
                reading_item = true;
            } else if (line.substr(0, 8) == "Content:") {
                current_item.content = line.substr(8);
            } else if (line.substr(0, 9) == "Timestamp:") {
                auto ms = std::stoll(line.substr(9));
                current_item.timestamp = std::chrono::system_clock::time_point(std::chrono::milliseconds(ms));
            } else if (line.substr(0, 7) == "Length:") {
                current_item.content_length = std::stoull(line.substr(7));
            } else if (line.substr(0, 9) == "Favorite:") {
                current_item.is_favorite = (line.substr(9) == "1");
            } else if (line.substr(0, 9) == "Category:") {
                current_item.category = line.substr(9);
            } else if (line.substr(0, 8) == "URLCount:") {
                // URL数量信息，暂时不处理
            } else if (line.substr(0, 4) == "URL:") {
                // URL信息，暂时不处理
            }
        }

        // 添加最后一个项目
        if (reading_item) {
            history_.push_back(current_item);
        }

        file.close();
        DEARTS_LOG_INFO("从文件加载剪切板历史记录: " + std::to_string(history_.size()) + " 项");
        return true;

    } catch (const std::exception& e) {
        DEARTS_LOG_ERROR("加载历史记录时发生异常: " + std::string(e.what()));
        return false;
    }
}

std::string ClipboardManager::getHistoryFilePath() {
    // 获取可执行文件目录
    char path[MAX_PATH];
    GetModuleFileNameA(nullptr, path, MAX_PATH);
    std::string exe_dir = path;
    size_t last_slash = exe_dir.find_last_of("\\/");
    if (last_slash != std::string::npos) {
        exe_dir = exe_dir.substr(0, last_slash);
    }

    return exe_dir + "\\clipboard_history.txt";
}

void ClipboardManager::limitHistorySize() {
    if (history_.size() > max_history_size_) {
        // 移除最旧的项目
        history_.erase(history_.begin(),
                       history_.begin() + (history_.size() - max_history_size_));

        DEARTS_LOG_DEBUG("限制历史记录数量，移除最旧的 " +
                       std::to_string(history_.size() - max_history_size_) + " 项");
    }
}

std::string ClipboardManager::generateId() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    return std::to_string(ms);
}

std::string ClipboardManager::formatTime(const std::chrono::system_clock::time_point& time_point) {
    auto time_t = std::chrono::system_clock::to_time_t(time_point);
    std::tm tm = *std::localtime(&time_t);

    char buffer[100];
    std::strftime(buffer, sizeof(buffer), "%H:%M:%S", &tm);

    return std::string(buffer);
}

} // namespace DearTs::Core::Window::Widgets::Clipboard