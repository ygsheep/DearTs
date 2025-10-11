#pragma once

#include "layout_base.h"
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <regex>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <future>
#include <imgui.h>
#include "../../utils/config_manager.h"

namespace DearTs {
namespace Core {
namespace Window {

/**
 * @brief 鸣潮换取记录状态
 */
enum class ExchangeRecordState {
    SEARCHING,    ///< 正在搜索游戏路径
    FOUND_LOG,    ///< 找到日志文件但未找到URL
    FOUND_URL,    ///< 找到URL
    SEARCH_ERROR  ///< 发生错误
};

/**
 * @brief 搜索结果信息
 */
struct SearchResult {
    bool found;
    std::string path;
    std::string message;
    std::string url;

    SearchResult() : found(false) {}
};

/**
 * @brief 鸣潮换取记录布局类
 * 用于提取鸣潮游戏的抽卡记录URL
 */
class ExchangeRecordLayout : public LayoutBase {
public:
    /**
     * @brief 构造函数
     */
    ExchangeRecordLayout();

    /**
     * @brief 析构函数
     */
    ~ExchangeRecordLayout() override;

    /**
     * @brief 渲染布局
     */
    void render() override;

    /**
     * @brief 更新布局
     * @param width 可用宽度
     * @param height 可用高度
     */
    void updateLayout(float width, float height) override;

    /**
     * @brief 处理事件
     * @param event SDL事件
     */
    void handleEvent(const SDL_Event& event) override;

    /**
     * @brief 开始搜索游戏路径和URL
     */
    void startSearch();

    /**
     * @brief 执行自动搜索
     */
    void performAutoSearch();

    /**
     * @brief 手动设置游戏路径
     * @param path 游戏安装路径
     */
    void setGamePath(const std::string& path);

    /**
     * @brief 复制URL到剪贴板
     */
    void copyUrlToClipboard();

    /**
     * @brief 浏览并选择游戏路径
     * @return 是否成功选择了路径
     */
    bool browseForGamePath();

    /**
     * @brief 获取当前状态
     */
    ExchangeRecordState getState() const { return currentState_; }

    /**
     * @brief 获取找到的URL
     */
    const std::string& getFoundUrl() const { return foundUrl_; }

    /**
     * @brief 获取状态消息
     */
    const std::string& getStatusMessage() const { return statusMessage_; }

    /**
     * @brief 加载配置
     */
    void loadConfiguration();

    /**
     * @brief 保存配置
     */
    void saveConfiguration();

    /**
     * @brief 检查是否有游戏路径配置
     * @return 是否有保存的游戏路径
     */
    bool hasGamePathConfiguration() const;

    /**
     * @brief 从保存的路径刷新URL（重新搜索最新URL）
     */
    void refreshUrlFromSavedPath();

  
private:
    ExchangeRecordState currentState_;        ///< 当前状态
    std::string foundUrl_;                   ///< 找到的URL
    std::string statusMessage_;              ///< 状态消息
    std::string manualGamePath_;             ///< 手动输入的游戏路径
    std::vector<SearchResult> searchResults_;///< 搜索结果列表
    bool autoSearchCompleted_;               ///< 自动搜索是否完成
    bool showManualInput_;                   ///< 是否显示手动输入框

    // 异步搜索相关
    std::atomic<bool> isSearching_{false};   ///< 是否正在搜索
    std::future<SearchResult> searchFuture_; ///< 异步搜索结果
    std::mutex searchMutex_;                 ///< 搜索结果保护锁
    std::string currentSearchPhase_;         ///< 当前搜索阶段描述
    int currentProgress_ = 0;                ///< 当前搜索进度

    /**
     * @brief 自动搜索游戏路径
     * @return 搜索结果
     */
    SearchResult autoSearchGamePath();

    /**
     * @brief 检查给定路径是否为有效的游戏安装目录
     * @param path 要检查的路径
     * @return 搜索结果
     */
    SearchResult checkGamePath(const std::filesystem::path& path);

    /**
     * @brief 在日志文件中搜索抽卡记录URL
     * @param logPath 日志文件路径
     * @return 找到的URL，如果未找到返回空字符串
     */
    std::string searchUrlInLogFile(const std::filesystem::path& logPath);

    /**
     * @brief 在Client.log中搜索URL
     * @param gamePath 游戏路径
     * @return 找到的URL
     */
    std::string searchInClientLog(const std::filesystem::path& gamePath);

    /**
     * @brief 在debug.log中搜索URL
     * @param gamePath 游戏路径
     * @return 找到的URL
     */
    std::string searchInDebugLog(const std::filesystem::path& gamePath);

    /**
     * @brief 通过MUI Cache搜索游戏路径（最有效的方法）
     * @return 找到的游戏路径列表
     */
    std::vector<std::string> searchGamePathFromMuiCache();

    /**
     * @brief 通过防火墙规则搜索游戏路径
     * @return 找到的游戏路径列表
     */
    std::vector<std::string> searchGamePathFromFirewall();

    /**
     * @brief 通过注册表搜索游戏路径（包含32位和64位）
     * @return 找到的游戏路径列表
     */
    std::vector<std::string> searchGamePathFromRegistry();

    /**
     * @brief 检查常见安装位置
     * @return 找到的游戏路径列表
     */
    std::vector<std::string> checkCommonInstallPaths();

    /**
     * @brief 验证URL格式是否正确
     * @param url 要验证的URL
     * @return 是否为有效的抽卡记录URL
     */
    bool isValidGachaUrl(const std::string& url);

    /**
     * @brief 更新状态消息
     * @param message 新的状态消息
     * @param state 新的状态
     */
    void updateStatus(const std::string& message, ExchangeRecordState state);

    /**
     * @brief 渲染状态显示区域
     */
    void renderStatusArea();

    /**
     * @brief 渲染搜索结果区域
     */
    void renderSearchResults();

    /**
     * @brief 渲染手动输入区域
     */
    void renderManualInput();

    /**
     * @brief 渲染操作按钮区域
     */
    void renderActionButtons();

    /**
     * @brief 异步执行自动搜索
     */
    void performAutoSearchAsync();

    /**
     * @brief 异步搜索主函数（在后台线程执行）
     * @return 搜索结果
     */
    SearchResult autoSearchGamePathAsync();

    /**
     * @brief 检查异步搜索是否完成并更新结果
     */
    void checkSearchCompletion();

    /**
     * @brief 更新搜索进度
     * @param phase 搜索阶段描述
     * @param progress 进度百分比
     */
    void updateSearchProgress(const std::string& phase, int progress);
};

} // namespace Window
} // namespace Core
} // namespace DearTs