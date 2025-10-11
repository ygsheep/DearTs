#include "exchange_record_layout.h"
#include <Windows.h>
#include <objbase.h>
#include <Shlobj.h>
#include <shobjidl.h>
#include <SDL.h>
#include <SDL_syswm.h>
#include <iostream>
#include <algorithm>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include "../resource/font_resource.h"
#include "../utils/logger.h"
#include "../utils/file_utils.h"
#include "../window_manager.h"

namespace DearTs {
namespace Core {
namespace Window {

/**
 * @brief ExchangeRecordLayout构造函数
 */
ExchangeRecordLayout::ExchangeRecordLayout()
    : LayoutBase("ExchangeRecord")
    , currentState_(ExchangeRecordState::SEARCHING)
    , foundUrl_()
    , statusMessage_("准备搜索鸣潮游戏安装路径...")
    , manualGamePath_()
    , searchResults_()
    , autoSearchCompleted_(false)
    , showManualInput_(false)
    , isSearching_(false)
    , currentProgress_(0) {

    DEARTS_LOG_INFO("ExchangeRecordLayout构造函数");

    // 加载保存的配置
    loadConfiguration();
}

/**
 * @brief 析构函数
 */
ExchangeRecordLayout::~ExchangeRecordLayout() {
    // 确保异步搜索任务被正确停止
    if (isSearching_.load()) {
        DEARTS_LOG_INFO("等待异步搜索任务完成...");
        if (searchFuture_.valid()) {
            searchFuture_.wait();
        }
        isSearching_.store(false);
    }
}

/**
 * @brief 渲染布局
 */
void ExchangeRecordLayout::render() {
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));

    // 标题 - 使用更大的字体
    auto fontManager = DearTs::Core::Resource::FontManager::getInstance();
    auto titleFont = fontManager ? fontManager->loadTitleFont(20.0f) : nullptr;
    if (titleFont) {
        titleFont->pushFont();
        ImGui::Text("鸣潮 - 抽取记录获取工具");
        titleFont->popFont();
    } else {
        ImGui::Text("鸣潮 - 抽取记录获取工具");
    }
    ImGui::Separator();

    // 状态显示区域
    renderStatusArea();

    // 搜索结果区域
    if (!searchResults_.empty()) {
        renderSearchResults();
    }

    // 手动输入区域
    if (showManualInput_) {
        renderManualInput();
    }

    // 操作按钮区域
    renderActionButtons();

    ImGui::PopStyleColor(2);
}

/**
 * @brief 更新布局
 */
void ExchangeRecordLayout::updateLayout(float width, float height) {
    setSize(width, height);

    // 检查异步搜索是否完成
    checkSearchCompletion();
}

/**
 * @brief 处理事件
 */
void ExchangeRecordLayout::handleEvent(const SDL_Event& event) {
    // 将SDL事件传递给ImGui，确保ImGui能正确处理鼠标和键盘输入
    ImGui_ImplSDL2_ProcessEvent(&event);
}

/**
 * @brief 开始搜索游戏路径和URL
 */
void ExchangeRecordLayout::startSearch() {
    searchResults_.clear();

    // 如果已经有有效的URL，说明已经完成搜索，无需重复搜索
    if (!foundUrl_.empty()) {
        DEARTS_LOG_INFO("已存在有效的抽卡记录URL，跳过搜索");
        updateStatus("已存在有效的抽卡记录URL，无需重新搜索", ExchangeRecordState::FOUND_URL);
        return;
    }

    // 清空之前的URL以确保搜索结果的一致性
    foundUrl_.clear();

    // 如果已有手动设置的游戏路径，直接验证该路径
    if (!manualGamePath_.empty()) {
        DEARTS_LOG_INFO("使用已保存的游戏路径进行验证: " + manualGamePath_);
        updateStatus("正在验证已保存的游戏路径: " + manualGamePath_, ExchangeRecordState::SEARCHING);

        SearchResult result = checkGamePath(std::filesystem::path(manualGamePath_));

        if (result.found && !result.url.empty()) {
            foundUrl_ = result.url;
            updateStatus("成功从保存路径找到抽卡记录URL！", ExchangeRecordState::FOUND_URL);
            copyUrlToClipboard();
            DEARTS_LOG_INFO("从保存路径成功找到URL: " + result.url);
        } else if (result.found) {
            updateStatus("保存的游戏路径有效，但未找到抽卡记录URL。请确保已打开游戏内的抽卡记录页面。", ExchangeRecordState::FOUND_LOG);
            DEARTS_LOG_INFO("保存路径有效但未找到URL: " + result.message);
        } else {
            updateStatus("保存的游戏路径可能已失效，开始自动搜索...", ExchangeRecordState::SEARCHING);
            DEARTS_LOG_INFO("保存的路径无效，开始自动搜索: " + result.message);
            // 如果保存的路径无效，再进行自动搜索
            performAutoSearch();
        }

        searchResults_.push_back(result);
    } else {
        // 没有保存的路径，进行自动搜索
        DEARTS_LOG_INFO("没有保存的游戏路径，开始自动搜索");
        updateStatus("正在自动搜索鸣潮游戏安装路径...", ExchangeRecordState::SEARCHING);
        performAutoSearch();
    }

    // 保存配置
    saveConfiguration();
}

/**
 * @brief 执行自动搜索
 */
void ExchangeRecordLayout::performAutoSearch() {
    // 改为异步搜索以避免UI卡顿
    performAutoSearchAsync();
}

/**
 * @brief 手动设置游戏路径
 */
void ExchangeRecordLayout::setGamePath(const std::string& path) {
    manualGamePath_ = path;

    if (path.empty()) {
        updateStatus("请输入有效的游戏安装路径。", ExchangeRecordState::SEARCH_ERROR);
        return;
    }

    updateStatus("正在检查指定的游戏路径...", ExchangeRecordState::SEARCHING);

    SearchResult result = checkGamePath(std::filesystem::path(path));

    if (result.found && !result.url.empty()) {
        foundUrl_ = result.url;
        updateStatus("成功找到抽卡记录URL！", ExchangeRecordState::FOUND_URL);
        copyUrlToClipboard();
    } else if (result.found) {
        updateStatus("找到游戏安装路径，但未找到抽卡记录URL。请确保已打开游戏内的抽卡记录页面。", ExchangeRecordState::FOUND_LOG);
    } else {
        updateStatus("指定的路径不是有效的鸣潮游戏安装目录。", ExchangeRecordState::SEARCH_ERROR);
    }

    searchResults_.insert(searchResults_.begin(), result);

    // 保存配置
    saveConfiguration();
}

/**
 * @brief 复制URL到剪贴板
 */
void ExchangeRecordLayout::copyUrlToClipboard() {
    if (!foundUrl_.empty()) {
        // 打开剪贴板
        if (OpenClipboard(nullptr)) {
            EmptyClipboard();

            // 分配内存并复制URL
            HGLOBAL hClipboardData = GlobalAlloc(GMEM_MOVEABLE, foundUrl_.size() + 1);
            if (hClipboardData) {
                char* pchData = static_cast<char*>(GlobalLock(hClipboardData));
                if (pchData) {
                    strcpy_s(pchData, foundUrl_.size() + 1, foundUrl_.c_str());
                    GlobalUnlock(hClipboardData);
                    SetClipboardData(CF_TEXT, hClipboardData);
                }
            }

            CloseClipboard();

            std::string newStatus = statusMessage_ + "\n\nURL已复制到剪贴板！请访问 https://mc.appfeng.com/gachaLog 并点击'Import History'按钮导入记录。";
            updateStatus(newStatus, currentState_);
        }
    }
}

/**
 * @brief 自动搜索游戏路径
 */
SearchResult ExchangeRecordLayout::autoSearchGamePath() {
    SearchResult result;
    DEARTS_LOG_INFO("开始自动搜索鸣潮游戏路径");

    // 1. 首先尝试MUI Cache搜索（最有效的方法）
    DEARTS_LOG_INFO("步骤1: 搜索MUI Cache");
    auto muiCachePaths = searchGamePathFromMuiCache();
    DEARTS_LOG_INFO("MUI Cache搜索找到 " + std::to_string(muiCachePaths.size()) + " 个路径");
    for (size_t i = 0; i < muiCachePaths.size(); ++i) {
        DEARTS_LOG_INFO("检查MUI Cache路径 " + std::to_string(i + 1) + "/" + std::to_string(muiCachePaths.size()) + ": " + muiCachePaths[i]);
        result = checkGamePath(std::filesystem::path(muiCachePaths[i]));
        if (result.found && !result.url.empty()) {
            DEARTS_LOG_INFO("MUI Cache路径成功找到URL: " + result.url);
            return result;
        }
        if (result.found) {
            DEARTS_LOG_INFO("MUI Cache路径找到游戏但未找到URL: " + result.message);
            searchResults_.push_back(result);
        } else {
            DEARTS_LOG_INFO("MUI Cache路径无效: " + result.message);
        }
    }

    // 2. 然后尝试Firewall规则搜索
    DEARTS_LOG_INFO("步骤2: 搜索防火墙规则");
    auto firewallPaths = searchGamePathFromFirewall();
    DEARTS_LOG_INFO("防火墙规则搜索找到 " + std::to_string(firewallPaths.size()) + " 个路径");
    for (size_t i = 0; i < firewallPaths.size(); ++i) {
        DEARTS_LOG_INFO("检查防火墙路径 " + std::to_string(i + 1) + "/" + std::to_string(firewallPaths.size()) + ": " + firewallPaths[i]);
        result = checkGamePath(std::filesystem::path(firewallPaths[i]));
        if (result.found && !result.url.empty()) {
            DEARTS_LOG_INFO("防火墙路径成功找到URL: " + result.url);
            return result;
        }
        if (result.found) {
            DEARTS_LOG_INFO("防火墙路径找到游戏但未找到URL: " + result.message);
            searchResults_.push_back(result);
        } else {
            DEARTS_LOG_INFO("防火墙路径无效: " + result.message);
        }
    }

    // 3. 然后通过注册表搜索
    DEARTS_LOG_INFO("步骤3: 搜索注册表");
    auto registryPaths = searchGamePathFromRegistry();
    DEARTS_LOG_INFO("注册表搜索找到 " + std::to_string(registryPaths.size()) + " 个路径");
    for (size_t i = 0; i < registryPaths.size(); ++i) {
        DEARTS_LOG_INFO("检查注册表路径 " + std::to_string(i + 1) + "/" + std::to_string(registryPaths.size()) + ": " + registryPaths[i]);
        result = checkGamePath(std::filesystem::path(registryPaths[i]));
        if (result.found && !result.url.empty()) {
            DEARTS_LOG_INFO("注册表路径成功找到URL: " + result.url);
            return result;
        }
        if (result.found) {
            DEARTS_LOG_INFO("注册表路径找到游戏但未找到URL: " + result.message);
            searchResults_.push_back(result);
        } else {
            DEARTS_LOG_INFO("注册表路径无效: " + result.message);
        }
    }

    // 4. 最后检查常见安装位置
    DEARTS_LOG_INFO("步骤4: 检查常见安装位置");
    auto commonPaths = checkCommonInstallPaths();
    DEARTS_LOG_INFO("常见安装位置找到 " + std::to_string(commonPaths.size()) + " 个路径");
    for (size_t i = 0; i < commonPaths.size(); ++i) {
        DEARTS_LOG_INFO("检查常见路径 " + std::to_string(i + 1) + "/" + std::to_string(commonPaths.size()) + ": " + commonPaths[i]);
        result = checkGamePath(std::filesystem::path(commonPaths[i]));
        if (result.found && !result.url.empty()) {
            DEARTS_LOG_INFO("常见路径成功找到URL: " + result.url);
            return result;
        }
        if (result.found) {
            DEARTS_LOG_INFO("常见路径找到游戏但未找到URL: " + result.message);
            searchResults_.push_back(result);
        } else {
            DEARTS_LOG_INFO("常见路径无效: " + result.message);
        }
    }

    // 返回最后一个结果（如果有的话）
    if (!searchResults_.empty()) {
        DEARTS_LOG_INFO("所有搜索完成，返回最后一个搜索结果: " + searchResults_.back().message);
        return searchResults_.back();
    }

    DEARTS_LOG_ERROR("所有搜索方法都未找到鸣潮游戏安装目录");
    result.message = "无法找到鸣潮游戏安装目录";
    return result;
}

/**
 * @brief 检查给定路径是否为有效的游戏安装目录
 */
SearchResult ExchangeRecordLayout::checkGamePath(const std::filesystem::path& path) {
    SearchResult result;

    if (!std::filesystem::exists(path)) {
        result.message = "路径不存在: " + path.string();
        return result;
    }

    result.path = path.string();
    result.found = true;

    // 搜索抽卡记录URL
    std::string url = searchInClientLog(path);
    if (url.empty()) {
        url = searchInDebugLog(path);
    }

    if (!url.empty()) {
        result.url = url;
        result.message = "在 " + path.string() + " 中找到抽卡记录URL";
    } else {
        result.message = "在 " + path.string() + " 中找到游戏文件，但未找到抽卡记录URL";
    }

    return result;
}

/**
 * @brief 在日志文件中搜索抽卡记录URL
 */
std::string ExchangeRecordLayout::searchUrlInLogFile(const std::filesystem::path& logPath) {
    if (!std::filesystem::exists(logPath)) {
        return "";
    }

    std::ifstream file(logPath);
    if (!file.is_open()) {
        return "";
    }

    std::string line;
    std::regex urlRegex(R"(https://aki-gm-resources(-oversea)?\.aki-game\.(net|com)/aki/gacha/index\.html#/record[^"]*)");
    std::smatch match;
    std::string lastMatch;

    while (std::getline(file, line)) {
        if (std::regex_search(line, match, urlRegex)) {
            lastMatch = match[0].str();
        }
    }

    return isValidGachaUrl(lastMatch) ? lastMatch : "";
}

/**
 * @brief 在Client.log中搜索URL
 */
std::string ExchangeRecordLayout::searchInClientLog(const std::filesystem::path& gamePath) {
    std::filesystem::path clientLogPath = gamePath / "Client" / "Saved" / "Logs" / "Client.log";
    return searchUrlInLogFile(clientLogPath);
}

/**
 * @brief 在debug.log中搜索URL
 */
std::string ExchangeRecordLayout::searchInDebugLog(const std::filesystem::path& gamePath) {
    std::filesystem::path debugLogPath = gamePath / "Client" / "Binaries" / "Win64" / "ThirdParty" /
                                        "KrPcSdk_Global" / "KRSDKRes" / "KRSDKWebView" / "debug.log";

    if (!std::filesystem::exists(debugLogPath)) {
        return "";
    }

    std::ifstream file(debugLogPath);
    if (!file.is_open()) {
        return "";
    }

    std::string line;
    std::regex urlRegex("\"#url\":\\s*\"(https://aki-gm-resources(-oversea)?\\.aki-game\\.(net|com)/aki/gacha/index\\.html#/record[^\"]*)\"");
    std::smatch match;
    std::string lastMatch;

    while (std::getline(file, line)) {
        if (std::regex_search(line, match, urlRegex)) {
            if (match.size() > 1) {
                lastMatch = match[1].str();
            }
        }
    }

    return isValidGachaUrl(lastMatch) ? lastMatch : "";
}

/**
 * @brief 通过MUI Cache搜索游戏路径（最有效的方法）
 */
std::vector<std::string> ExchangeRecordLayout::searchGamePathFromMuiCache() {
    std::vector<std::string> paths;

    DEARTS_LOG_INFO("开始搜索MUI Cache: HKEY_CURRENT_USER\\Software\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\Shell\\MuiCache");

    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER,
        "Software\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\Shell\\MuiCache",
        0, KEY_READ, &hKey) == ERROR_SUCCESS) {

        DEARTS_LOG_INFO("MUI Cache注册表打开成功");

        char valueName[MAX_PATH];
        char valueData[MAX_PATH];
        DWORD valueNameSize = sizeof(valueName);
        DWORD valueDataSize = sizeof(valueData);
        DWORD index = 0;
        int totalEntries = 0;
        int wutheringEntries = 0;

        while (RegEnumValueA(hKey, index++, valueName, &valueNameSize,
                           nullptr, nullptr, (LPBYTE)valueData, &valueDataSize) == ERROR_SUCCESS) {

            totalEntries++;
            std::string valueNameStr(valueName);
            std::string valueDataStr(valueData);

            // 检查是否包含wuthering且是client-win64-shipping.exe
            if (valueDataStr.find("wuthering") != std::string::npos &&
                valueNameStr.find("client-win64-shipping.exe") != std::string::npos) {

                wutheringEntries++;
                DEARTS_LOG_INFO("找到MUI Cache条目: " + std::string(valueData));
                DEARTS_LOG_INFO("对应的可执行文件: " + valueNameStr);

                // 从注册表值名称中提取路径
                size_t clientPos = valueNameStr.find("\\client\\");
                if (clientPos != std::string::npos) {
                    std::string gamePath = valueNameStr.substr(0, clientPos);
                    DEARTS_LOG_INFO("提取的游戏路径: " + gamePath);

                    // 跳过OneDrive路径
                    if (gamePath.find("OneDrive") == std::string::npos) {
                        DEARTS_LOG_INFO("添加有效路径: " + gamePath);
                        paths.push_back(gamePath);
                    } else {
                        DEARTS_LOG_INFO("跳过OneDrive路径: " + gamePath);
                    }
                } else {
                    DEARTS_LOG_WARN("无法从路径中提取游戏目录: " + valueNameStr);
                }
            }

            valueNameSize = sizeof(valueName);
            valueDataSize = sizeof(valueData);
        }

        DEARTS_LOG_INFO("MUI Cache搜索完成: 总共检查 " + std::to_string(totalEntries) + " 个条目，找到 " + std::to_string(wutheringEntries) + " 个鸣潮相关条目");
        RegCloseKey(hKey);
    } else {
        DEARTS_LOG_ERROR("无法打开MUI Cache注册表");
    }

    return paths;
}

/**
 * @brief 通过防火墙规则搜索游戏路径
 */
std::vector<std::string> ExchangeRecordLayout::searchGamePathFromFirewall() {
    std::vector<std::string> paths;

    DEARTS_LOG_INFO("开始搜索防火墙规则: HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\SharedAccess\\Parameters\\FirewallPolicy\\FirewallRules");

    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
        "SYSTEM\\CurrentControlSet\\Services\\SharedAccess\\Parameters\\FirewallPolicy\\FirewallRules",
        0, KEY_READ, &hKey) == ERROR_SUCCESS) {

        DEARTS_LOG_INFO("防火墙规则注册表打开成功");

        char valueName[MAX_PATH];
        char valueData[MAX_PATH];
        DWORD valueNameSize = sizeof(valueName);
        DWORD valueDataSize = sizeof(valueData);
        DWORD index = 0;
        int totalRules = 0;
        int wutheringRules = 0;

        while (RegEnumValueA(hKey, index++, valueName, &valueNameSize,
                           nullptr, nullptr, (LPBYTE)valueData, &valueDataSize) == ERROR_SUCCESS) {

            totalRules++;
            std::string valueNameStr(valueName);
            std::string valueDataStr(valueData);

            // 检查是否包含wuthering且包含client-win64-shipping
            if (valueDataStr.find("wuthering") != std::string::npos &&
                valueNameStr.find("client-win64-shipping") != std::string::npos) {

                wutheringRules++;
                DEARTS_LOG_INFO("找到防火墙规则: " + valueNameStr);
                DEARTS_LOG_INFO("规则数据: " + valueDataStr);

                // 从防火墙规则数据中提取路径
                size_t appPos = valueDataStr.find("App=");
                if (appPos != std::string::npos) {
                    size_t pathStart = appPos + 4;
                    size_t clientPos = valueDataStr.find("\\client\\", pathStart);
                    if (clientPos != std::string::npos) {
                        std::string gamePath = valueDataStr.substr(pathStart, clientPos - pathStart);
                        DEARTS_LOG_INFO("从防火墙规则提取的游戏路径: " + gamePath);

                        // 跳过OneDrive路径
                        if (gamePath.find("OneDrive") == std::string::npos) {
                            DEARTS_LOG_INFO("添加有效路径: " + gamePath);
                            paths.push_back(gamePath);
                        } else {
                            DEARTS_LOG_INFO("跳过OneDrive路径: " + gamePath);
                        }
                    } else {
                        DEARTS_LOG_WARN("无法从防火墙规则中提取游戏目录: " + valueDataStr);
                    }
                } else {
                    DEARTS_LOG_WARN("防火墙规则中未找到App=路径: " + valueDataStr);
                }
            }

            valueNameSize = sizeof(valueName);
            valueDataSize = sizeof(valueData);
        }

        DEARTS_LOG_INFO("防火墙规则搜索完成: 总共检查 " + std::to_string(totalRules) + " 个规则，找到 " + std::to_string(wutheringRules) + " 个鸣潮相关规则");
        RegCloseKey(hKey);
    } else {
        DEARTS_LOG_ERROR("无法打开防火墙规则注册表");
    }

    return paths;
}

/**
 * @brief 通过注册表搜索游戏路径（包含32位和64位）
 */
std::vector<std::string> ExchangeRecordLayout::searchGamePathFromRegistry() {
    std::vector<std::string> paths;

    // 搜索64位和32位的卸载信息
    const char* registryPaths[] = {
        "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
        "SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall"
    };

    for (const char* regPath : registryPaths) {
        DEARTS_LOG_INFO("搜索注册表路径: HKEY_LOCAL_MACHINE\\" + std::string(regPath));

        HKEY hKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, regPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            DEARTS_LOG_INFO("注册表路径打开成功: " + std::string(regPath));

            char subkeyName[256];
            DWORD subkeyNameSize = sizeof(subkeyName);
            DWORD index = 0;
            int totalPrograms = 0;
            int wutheringPrograms = 0;

            while (RegEnumKeyExA(hKey, index++, subkeyName, &subkeyNameSize,
                               nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {

                totalPrograms++;
                HKEY hSubKey;
                if (RegOpenKeyExA(hKey, subkeyName, 0, KEY_READ, &hSubKey) == ERROR_SUCCESS) {
                    char displayName[256] = {0};
                    char installPath[MAX_PATH] = {0};
                    DWORD displayNameSize = sizeof(displayName);
                    DWORD installPathSize = sizeof(installPath);

                    if (RegQueryValueExA(hSubKey, "DisplayName", nullptr, nullptr,
                        (LPBYTE)displayName, &displayNameSize) == ERROR_SUCCESS) {

                        std::string displayNameStr(displayName);
                        std::transform(displayNameStr.begin(), displayNameStr.end(),
                                     displayNameStr.begin(), ::tolower);

                        if (displayNameStr.find("wuthering") != std::string::npos) {
                            wutheringPrograms++;
                            DEARTS_LOG_INFO("找到鸣潮程序: " + std::string(displayName));
                            DEARTS_LOG_INFO("程序ID: " + std::string(subkeyName));

                            if (RegQueryValueExA(hSubKey, "InstallLocation", nullptr, nullptr,
                                (LPBYTE)installPath, &installPathSize) == ERROR_SUCCESS) {

                                std::string installPathStr(installPath);
                                DEARTS_LOG_INFO("安装路径: " + installPathStr);

                                // 跳过OneDrive路径
                                if (installPathStr.find("OneDrive") == std::string::npos) {
                                    DEARTS_LOG_INFO("添加有效路径: " + installPathStr);
                                    paths.push_back(installPathStr);
                                } else {
                                    DEARTS_LOG_INFO("跳过OneDrive路径: " + installPathStr);
                                }
                            } else {
                                DEARTS_LOG_WARN("未找到安装路径: " + std::string(subkeyName));
                            }
                        }
                    } else {
                        // 没有DisplayName，尝试其他方式
                        DEARTS_LOG_DEBUG("程序无DisplayName: " + std::string(subkeyName));
                    }

                    RegCloseKey(hSubKey);
                }

                subkeyNameSize = sizeof(subkeyName);
            }

            DEARTS_LOG_INFO("注册表路径 " + std::string(regPath) + " 搜索完成: 总共 " + std::to_string(totalPrograms) + " 个程序，找到 " + std::to_string(wutheringPrograms) + " 个鸣潮程序");
            RegCloseKey(hKey);
        } else {
            DEARTS_LOG_ERROR("无法打开注册表路径: " + std::string(regPath));
        }
    }

    return paths;
}

/**
 * @brief 检查常见安装位置
 */
std::vector<std::string> ExchangeRecordLayout::checkCommonInstallPaths() {
    std::vector<std::string> commonPaths;

    DEARTS_LOG_INFO("开始检查常见安装位置");

    // 获取所有可用的驱动器
    DWORD drives = GetLogicalDrives();
    int totalDrives = 0;
    int foundPaths = 0;

    for (int i = 0; i < 26; ++i) {
        if (drives & (1 << i)) {
            totalDrives++;
            char driveLetter = static_cast<char>('A' + i);
            std::string drivePath = std::string(1, driveLetter) + ":";
            DEARTS_LOG_INFO("检查驱动器: " + drivePath);

            // 常见安装路径
            std::vector<std::string> paths = {
                drivePath + "\\Wuthering Waves Game",
                drivePath + "\\Wuthering Waves\\Wuthering Waves Game",
                drivePath + "\\Program Files\\Epic Games\\WutheringWavesj3oFh",
                drivePath + "\\Program Files\\Epic Games\\WutheringWavesj3oFh\\Wuthering Waves Game"
            };

            for (const auto& path : paths) {
                DEARTS_LOG_INFO("检查路径: " + path);
                if (std::filesystem::exists(path)) {
                    DEARTS_LOG_INFO("找到存在的路径: " + path);
                    commonPaths.push_back(path);
                    foundPaths++;
                } else {
                    DEARTS_LOG_DEBUG("路径不存在: " + path);
                }
            }
        }
    }

    DEARTS_LOG_INFO("常见安装位置检查完成: 检查了 " + std::to_string(totalDrives) + " 个驱动器，找到 " + std::to_string(foundPaths) + " 个存在的路径");
    return commonPaths;
}

/**
 * @brief 验证URL格式是否正确
 */
bool ExchangeRecordLayout::isValidGachaUrl(const std::string& url) {
    if (url.empty()) {
        return false;
    }

    // 简单的URL格式验证
    return url.find("aki-gm-resources") != std::string::npos &&
           url.find("aki-game") != std::string::npos &&
           url.find("gacha") != std::string::npos &&
           url.find("record") != std::string::npos;
}

/**
 * @brief 更新状态消息
 */
void ExchangeRecordLayout::updateStatus(const std::string& message, ExchangeRecordState state) {
    statusMessage_ = message;
    currentState_ = state;
}

/**
 * @brief 渲染状态显示区域
 */
void ExchangeRecordLayout::renderStatusArea() {
    ImGui::BeginChild("StatusArea", ImVec2(0, 120), true);

    // 根据状态设置不同的颜色
    ImVec4 textColor;
    switch (currentState_) {
        case ExchangeRecordState::SEARCHING:
            textColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); // 黄色
            break;
        case ExchangeRecordState::FOUND_LOG:
            textColor = ImVec4(1.0f, 0.5f, 0.0f, 1.0f); // 橙色
            break;
        case ExchangeRecordState::FOUND_URL:
            textColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // 绿色
            break;
        case ExchangeRecordState::SEARCH_ERROR:
            textColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // 红色
            break;
    }

    // 显示状态消息
    ImGui::PushStyleColor(ImGuiCol_Text, textColor);
    ImGui::TextWrapped("%s", statusMessage_.c_str());
    ImGui::PopStyleColor();

    // 如果正在搜索，显示进度条
    if (isSearching_.load()) {
        ImGui::Separator();
        std::lock_guard<std::mutex> lock(searchMutex_);
        ImGui::Text("搜索进度: %s", currentSearchPhase_.c_str());
        ImGui::ProgressBar(currentProgress_ / 100.0f, ImVec2(0, 0));
    }

    // 如果找到URL，显示URL
    if (currentState_ == ExchangeRecordState::FOUND_URL && !foundUrl_.empty()) {
        ImGui::Separator();
        ImGui::Text("抽卡记录URL:");

        // URL文本区域 - 确保可读性
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 1.0f, 1.0f));
        ImGui::TextWrapped("%s", foundUrl_.c_str());
        ImGui::PopStyleColor();

        // 添加复制提示
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "提示: URL已复制到剪贴板，请访问 https://mc.appfeng.com/gachaLog 导入记录");
    }

    ImGui::EndChild();
}

/**
 * @brief 渲染搜索结果区域
 */
void ExchangeRecordLayout::renderSearchResults() {
    ImGui::BeginChild("SearchResults", ImVec2(0, 150), true);

    // 搜索结果标题
    ImGui::Text("搜索结果:");
    ImGui::Separator();

    for (const auto& result : searchResults_) {
        if (result.found) {
            ImVec4 resultColor = result.url.empty() ?
                ImVec4(1.0f, 0.5f, 0.0f, 1.0f) : // 橙色：找到路径但未找到URL
                ImVec4(0.0f, 1.0f, 0.0f, 1.0f);  // 绿色：找到URL

            ImGui::PushStyleColor(ImGuiCol_Text, resultColor);
            ImGui::TextWrapped("%s", result.message.c_str());
            ImGui::PopStyleColor();
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
            ImGui::TextWrapped("%s", result.message.c_str());
            ImGui::PopStyleColor();
        }
        ImGui::Separator();
    }

    ImGui::EndChild();
}

/**
 * @brief 渲染手动输入区域
 */
void ExchangeRecordLayout::renderManualInput() {
    ImGui::BeginChild("ManualInput", ImVec2(0, 100), true);

    // 手动输入标题
    ImGui::Text("手动选择游戏安装路径:");

    static char pathBuffer[MAX_PATH] = {0};
    if (!manualGamePath_.empty()) {
        strcpy_s(pathBuffer, sizeof(pathBuffer), manualGamePath_.c_str());
    }

    // 输入框
    ImGui::InputText("##GamePath", pathBuffer, sizeof(pathBuffer));
    ImGui::SameLine();

    // 按钮区域
    if (ImGui::Button("浏览")) {
        // 打开Windows文件夹选择对话框
        if (browseForGamePath()) {
            updateStatus("已选择游戏路径: " + manualGamePath_, ExchangeRecordState::SEARCHING);
            // 自动检查选择路径
            setGamePath(manualGamePath_);
            // 更新输入框内容
            strcpy_s(pathBuffer, sizeof(pathBuffer), manualGamePath_.c_str());
        } else {
            // 如果文件夹选择失败，提示用户手动输入
            updateStatus("文件夹选择失败，请手动输入游戏路径或重试", ExchangeRecordState::SEARCH_ERROR);
        }
    }
    ImGui::SameLine();

    if (ImGui::Button("确认路径")) {
        setGamePath(std::string(pathBuffer));
    }

    ImGui::EndChild();
}

/**
 * @brief 渲染操作按钮区域
 */
void ExchangeRecordLayout::renderActionButtons() {
    ImGui::Separator();

    // 按钮区域

    if (currentState_ != ExchangeRecordState::FOUND_URL) {
        if (ImGui::Button("开始搜索")) {
            startSearch();
        }
        ImGui::SameLine();
    }

    if (!showManualInput_) {
        if (ImGui::Button("手动选择路径")) {
            showManualInput_ = true;
        }
        ImGui::SameLine();
    }

    if (currentState_ == ExchangeRecordState::FOUND_URL && !foundUrl_.empty()) {
        if (ImGui::Button("重新复制URL")) {
            copyUrlToClipboard();
        }
        ImGui::SameLine();
    }

    if (ImGui::Button("重置")) {
        currentState_ = ExchangeRecordState::SEARCHING;
        foundUrl_.clear();
        statusMessage_ = "准备搜索鸣潮游戏安装路径...";
        searchResults_.clear();
        autoSearchCompleted_ = false;
        showManualInput_ = false;
        manualGamePath_.clear();
    }

    // 添加使用提示
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "提示: 点击按钮开始搜索，或手动输入游戏路径");
}

/**
 * @brief 浏览并选择游戏路径
 */
// 浏览文件夹回调函数
static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData) {
    switch (uMsg) {
        case BFFM_INITIALIZED:
            DEARTS_LOG_INFO("文件夹选择对话框已初始化");
            // 设置初始目录
            SendMessageA(hwnd, BFFM_SETSELECTIONA, TRUE, lpData);
            break;
        case BFFM_SELCHANGED:
            DEARTS_LOG_INFO("文件夹选择已改变");
            // 获取当前选择的文件夹并显示
            char szPath[MAX_PATH];
            if (SHGetPathFromIDListA(reinterpret_cast<LPITEMIDLIST>(lParam), szPath)) {
                DEARTS_LOG_INFO("当前选择: " + std::string(szPath));
            }
            break;
        case BFFM_VALIDATEFAILED:
            DEARTS_LOG_INFO("文件夹选择验证失败");
            break;
    }
    return 0;
}

bool ExchangeRecordLayout::browseForGamePath() {
    DEARTS_LOG_INFO("开始现代化文件夹选择过程");

    // 初始化COM
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        DEARTS_LOG_ERROR("COM初始化失败: " + std::to_string(hr));
        return false;
    }

    bool result = false;

    try {
        // 清空SDL事件队列
        SDL_Event event;
        while (SDL_PollEvent(&event)) {}

        // 获取主窗口句柄作为父窗口
        HWND hwndOwner = nullptr;
        SDL_Window* sdlWindow = nullptr;

        // 尝试获取主窗口的SDL句柄
        auto& windowManager = DearTs::Core::Window::WindowManager::getInstance();
        auto windows = windowManager.getAllWindows();
        if (!windows.empty() && windows[0]) {
            sdlWindow = windows[0]->getSDLWindow();
            if (sdlWindow) {
                SDL_SysWMinfo wmInfo;
                SDL_VERSION(&wmInfo.version);
                if (SDL_GetWindowWMInfo(sdlWindow, &wmInfo)) {
                    if (wmInfo.subsystem == SDL_SYSWM_WINDOWS) {
                        hwndOwner = wmInfo.info.win.window;
                        DEARTS_LOG_INFO("获取到主窗口句柄: " + std::to_string(reinterpret_cast<uintptr_t>(hwndOwner)));
                    }
                }
            }
        }

        // 创建现代化的文件夹选择对话框
        IFileOpenDialog* pFileOpenDialog = nullptr;
        hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL,
                            IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpenDialog));

        if (SUCCEEDED(hr)) {
            DEARTS_LOG_INFO("成功创建IFileOpenDialog实例");

            // 配置对话框为文件夹选择模式
            DWORD dwOptions;
            hr = pFileOpenDialog->GetOptions(&dwOptions);
            if (SUCCEEDED(hr)) {
                hr = pFileOpenDialog->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
                DEARTS_LOG_INFO("设置对话框选项为文件夹选择模式");
            }

            // 设置对话框标题
            hr = pFileOpenDialog->SetTitle(L"选择鸣潮游戏安装目录");
            if (SUCCEEDED(hr)) {
                DEARTS_LOG_INFO("设置对话框标题成功");
            }

            // 如果有之前的路径，设置为默认文件夹
            if (!manualGamePath_.empty()) {
                IShellItem* pItem = nullptr;
                std::wstring widePath(manualGamePath_.begin(), manualGamePath_.end());
                hr = SHCreateItemFromParsingName(widePath.c_str(), nullptr, IID_IShellItem,
                                               reinterpret_cast<void**>(&pItem));
                if (SUCCEEDED(hr)) {
                    pFileOpenDialog->SetFolder(pItem);
                    pItem->Release();
                    DEARTS_LOG_INFO("设置默认文件夹: " + manualGamePath_);
                }
            }

            // 显示对话框
            hr = pFileOpenDialog->Show(hwndOwner);

            if (SUCCEEDED(hr)) {
                DEARTS_LOG_INFO("用户确认了文件夹选择");

                // 获取选择的结果
                IShellItem* pItem;
                hr = pFileOpenDialog->GetResult(&pItem);
                if (SUCCEEDED(hr)) {
                    PWSTR pszFilePath;
                    hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
                    if (SUCCEEDED(hr)) {
                        // 转换为UTF-8字符串
                        std::wstring widePath(pszFilePath);
                        manualGamePath_ = std::string(widePath.begin(), widePath.end());
                        result = true;

                        DEARTS_LOG_INFO("成功获取用户选择的文件夹路径: " + manualGamePath_);
                        CoTaskMemFree(pszFilePath);
                    } else {
                        DEARTS_LOG_ERROR("无法获取文件夹显示名称");
                    }
                    pItem->Release();
                } else {
                    DEARTS_LOG_ERROR("无法获取对话框结果");
                }
            } else if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
                DEARTS_LOG_INFO("用户取消了文件夹选择");
            } else {
                DEARTS_LOG_ERROR("显示对话框失败: " + std::to_string(hr));
            }

            pFileOpenDialog->Release();
        } else {
            DEARTS_LOG_ERROR("创建IFileOpenDialog失败: " + std::to_string(hr));

            // 如果现代对话框创建失败，回退到传统方法
            DEARTS_LOG_INFO("回退到传统文件夹选择对话框");

            // 配置传统文件夹选择对话框
            BROWSEINFOA bi = {0};
            char szPath[MAX_PATH] = {0};

            bi.hwndOwner = hwndOwner;
            bi.lpszTitle = "选择鸣潮游戏安装目录";
            bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
            bi.lpfn = BrowseCallbackProc;
            bi.lParam = reinterpret_cast<LPARAM>(manualGamePath_.c_str());
            bi.iImage = 0;

            LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
            if (pidl != nullptr) {
                if (SHGetPathFromIDListA(pidl, szPath)) {
                    manualGamePath_ = std::string(szPath);
                    result = true;
                    DEARTS_LOG_INFO("传统方式成功获取文件夹路径: " + manualGamePath_);
                }
                CoTaskMemFree(pidl);
            }
        }

        // 清空可能积压的SDL事件
        while (SDL_PollEvent(&event)) {}

    } catch (const std::exception& e) {
        DEARTS_LOG_ERROR("文件夹选择过程中发生异常: " + std::string(e.what()));
        result = false;
    }

    // 清理COM
    CoUninitialize();

    DEARTS_LOG_INFO("文件夹选择过程结束，结果: " + std::string(result ? "成功" : "失败"));
    return result;
}

/**
 * @brief 加载配置
 */
void ExchangeRecordLayout::loadConfiguration() {
    auto& config = DearTs::Core::Utils::ConfigManager::getInstance();

    // 获取可执行文件目录并构建配置文件路径
    std::string configDir = DearTs::Core::Utils::FileUtils::getExecutableDirectory();
    std::string configPath = configDir + "/config.txt";

    // 加载配置文件
    if (config.loadFromFile(configPath)) {
        DEARTS_LOG_INFO("成功加载配置文件: " + configPath);

        // 读取保存的游戏路径
        std::string savedPath = config.getString("exchange_record.game_path", "");
        if (!savedPath.empty()) {
            manualGamePath_ = savedPath;
            showManualInput_ = true;
            DEARTS_LOG_INFO("从配置文件加载游戏路径: " + savedPath);

            // 如果有保存的URL，也一并加载
            std::string savedUrl = config.getString("exchange_record.last_url", "");
            if (!savedUrl.empty()) {
                foundUrl_ = savedUrl;
                updateStatus("已加载上次保存的游戏路径和URL，点击'重新复制URL'可重新复制", ExchangeRecordState::FOUND_URL);
                DEARTS_LOG_INFO("从配置文件加载抽卡记录URL: " + savedUrl);
            } else {
                updateStatus("已加载上次保存的游戏路径，点击'开始搜索'验证路径或搜索URL", ExchangeRecordState::FOUND_LOG);
            }
        }
    } else {
        DEARTS_LOG_INFO("配置文件不存在或加载失败，将使用默认设置: " + configPath);
    }
}

/**
 * @brief 保存配置
 */
void ExchangeRecordLayout::saveConfiguration() {
    auto& config = DearTs::Core::Utils::ConfigManager::getInstance();

    // 保存游戏路径
    if (!manualGamePath_.empty()) {
        config.setString("exchange_record.game_path", manualGamePath_);
        DEARTS_LOG_INFO("保存游戏路径到配置: " + manualGamePath_);
    }

    // 保存找到的URL
    if (!foundUrl_.empty()) {
        config.setString("exchange_record.last_url", foundUrl_);
        DEARTS_LOG_INFO("保存抽卡记录URL到配置: " + foundUrl_);
    }

    // 保存其他状态信息
    config.setBool("exchange_record.auto_search_completed", autoSearchCompleted_);
    config.setString("exchange_record.last_status_message", statusMessage_);
    config.setInt("exchange_record.current_state", static_cast<int>(currentState_));

    // 获取可执行文件目录并构建配置文件路径
    std::string configDir = DearTs::Core::Utils::FileUtils::getExecutableDirectory();
    std::string configPath = configDir + "/config.txt";

    // 保存到文件
    config.saveToFile(configPath);
    DEARTS_LOG_INFO("配置已保存到文件: " + configPath);
}

/**
 * @brief 检查是否有游戏路径配置
 */
bool ExchangeRecordLayout::hasGamePathConfiguration() const {
    return !manualGamePath_.empty();
}

/**
 * @brief 从保存的路径刷新URL（重新搜索最新URL）
 */
void ExchangeRecordLayout::refreshUrlFromSavedPath() {
    if (manualGamePath_.empty()) {
        DEARTS_LOG_WARN("没有保存的游戏路径，无法刷新URL");
        return;
    }

    if (isSearching_.load()) {
        DEARTS_LOG_WARN("搜索已在进行中，跳过重复请求");
        return;
    }

    DEARTS_LOG_INFO("从保存路径刷新URL: " + manualGamePath_);

    // 清空搜索结果和之前的URL
    searchResults_.clear();
    foundUrl_.clear();

    // 更新状态为搜索中
    updateStatus("正在从保存路径重新搜索最新URL: " + manualGamePath_, ExchangeRecordState::SEARCHING);

    // 异步执行路径验证和URL搜索
    isSearching_.store(true);
    updateSearchProgress("验证保存的游戏路径...", 10);

    // 启动异步搜索任务
    searchFuture_ = std::async(std::launch::async, [this, savedPath = manualGamePath_]() -> SearchResult {
        try {
            updateSearchProgress("验证游戏路径...", 20);
            SearchResult result = checkGamePath(std::filesystem::path(savedPath));

            if (result.found && !result.url.empty()) {
                updateSearchProgress("成功找到URL!", 100);
                DEARTS_LOG_INFO("异步路径验证成功找到URL: " + result.url);
            } else if (result.found) {
                updateSearchProgress("路径验证成功，但未找到URL", 90);
                DEARTS_LOG_INFO("异步路径验证成功但未找到URL: " + result.message);
            } else {
                updateSearchProgress("路径验证失败", 50);
                DEARTS_LOG_WARN("异步路径验证失败: " + result.message);
            }

            return result;
        } catch (const std::exception& e) {
            DEARTS_LOG_ERROR("异步路径验证过程中发生异常: " + std::string(e.what()));
            SearchResult errorResult;
            errorResult.message = "路径验证过程中发生错误: " + std::string(e.what());
            return errorResult;
        }
    });

    DEARTS_LOG_INFO("异步路径验证任务已启动");
}

/**
 * @brief 异步执行自动搜索
 */
void ExchangeRecordLayout::performAutoSearchAsync() {
    if (isSearching_.load()) {
        DEARTS_LOG_WARN("搜索已在进行中，跳过重复请求");
        return;
    }

    isSearching_.store(true);
    updateSearchProgress("启动搜索...", 0);

    // 启动异步搜索任务
    searchFuture_ = std::async(std::launch::async, [this]() -> SearchResult {
        return autoSearchGamePathAsync();
    });

    DEARTS_LOG_INFO("异步搜索任务已启动");
}

/**
 * @brief 异步搜索主函数（在后台线程执行）
 */
SearchResult ExchangeRecordLayout::autoSearchGamePathAsync() {
    SearchResult result;
    DEARTS_LOG_INFO("开始异步自动搜索鸣潮游戏路径");

    try {
        // 1. 首先尝试MUI Cache搜索（最有效的方法）
        updateSearchProgress("搜索MUI Cache...", 20);
        auto muiCachePaths = searchGamePathFromMuiCache();
        DEARTS_LOG_INFO("MUI Cache搜索找到 " + std::to_string(muiCachePaths.size()) + " 个路径");

        for (size_t i = 0; i < muiCachePaths.size(); ++i) {
            updateSearchProgress("检查MUI Cache路径 " + std::to_string(i + 1) + "/" + std::to_string(muiCachePaths.size()), 25);
            DEARTS_LOG_INFO("检查MUI Cache路径 " + std::to_string(i + 1) + "/" + std::to_string(muiCachePaths.size()) + ": " + muiCachePaths[i]);
            result = checkGamePath(std::filesystem::path(muiCachePaths[i]));
            if (result.found && !result.url.empty()) {
                DEARTS_LOG_INFO("MUI Cache路径成功找到URL: " + result.url);
                return result;
            }
            if (result.found) {
                std::lock_guard<std::mutex> lock(searchMutex_);
                searchResults_.push_back(result);
            }
        }

        // 2. 然后尝试Firewall规则搜索
        updateSearchProgress("搜索防火墙规则...", 50);
        auto firewallPaths = searchGamePathFromFirewall();
        DEARTS_LOG_INFO("防火墙规则搜索找到 " + std::to_string(firewallPaths.size()) + " 个路径");

        for (size_t i = 0; i < firewallPaths.size(); ++i) {
            updateSearchProgress("检查防火墙路径 " + std::to_string(i + 1) + "/" + std::to_string(firewallPaths.size()), 55);
            DEARTS_LOG_INFO("检查防火墙路径 " + std::to_string(i + 1) + "/" + std::to_string(firewallPaths.size()) + ": " + firewallPaths[i]);
            result = checkGamePath(std::filesystem::path(firewallPaths[i]));
            if (result.found && !result.url.empty()) {
                DEARTS_LOG_INFO("防火墙路径成功找到URL: " + result.url);
                return result;
            }
            if (result.found) {
                std::lock_guard<std::mutex> lock(searchMutex_);
                searchResults_.push_back(result);
            }
        }

        // 3. 然后通过注册表搜索
        updateSearchProgress("搜索注册表...", 75);
        auto registryPaths = searchGamePathFromRegistry();
        DEARTS_LOG_INFO("注册表搜索找到 " + std::to_string(registryPaths.size()) + " 个路径");

        for (size_t i = 0; i < registryPaths.size(); ++i) {
            updateSearchProgress("检查注册表路径 " + std::to_string(i + 1) + "/" + std::to_string(registryPaths.size()), 80);
            DEARTS_LOG_INFO("检查注册表路径 " + std::to_string(i + 1) + "/" + std::to_string(registryPaths.size()) + ": " + registryPaths[i]);
            result = checkGamePath(std::filesystem::path(registryPaths[i]));
            if (result.found && !result.url.empty()) {
                DEARTS_LOG_INFO("注册表路径成功找到URL: " + result.url);
                return result;
            }
            if (result.found) {
                std::lock_guard<std::mutex> lock(searchMutex_);
                searchResults_.push_back(result);
            }
        }

        // 4. 最后检查常见安装位置
        updateSearchProgress("检查常见安装位置...", 90);
        auto commonPaths = checkCommonInstallPaths();
        DEARTS_LOG_INFO("常见安装位置找到 " + std::to_string(commonPaths.size()) + " 个路径");

        for (size_t i = 0; i < commonPaths.size(); ++i) {
            updateSearchProgress("检查常见路径 " + std::to_string(i + 1) + "/" + std::to_string(commonPaths.size()), 95);
            DEARTS_LOG_INFO("检查常见路径 " + std::to_string(i + 1) + "/" + std::to_string(commonPaths.size()) + ": " + commonPaths[i]);
            result = checkGamePath(std::filesystem::path(commonPaths[i]));
            if (result.found && !result.url.empty()) {
                DEARTS_LOG_INFO("常见路径成功找到URL: " + result.url);
                return result;
            }
            if (result.found) {
                std::lock_guard<std::mutex> lock(searchMutex_);
                searchResults_.push_back(result);
            }
        }

        // 返回最后一个结果（如果有的话）
        {
            std::lock_guard<std::mutex> lock(searchMutex_);
            if (!searchResults_.empty()) {
                DEARTS_LOG_INFO("所有搜索完成，返回最后一个搜索结果: " + searchResults_.back().message);
                return searchResults_.back();
            }
        }

        DEARTS_LOG_ERROR("所有搜索方法都未找到鸣潮游戏安装目录");
        result.message = "无法找到鸣潮游戏安装目录";
        return result;

    } catch (const std::exception& e) {
        DEARTS_LOG_ERROR("异步搜索过程中发生异常: " + std::string(e.what()));
        result.message = "搜索过程中发生错误: " + std::string(e.what());
        return result;
    }
}

/**
 * @brief 检查异步搜索是否完成并更新结果
 */
void ExchangeRecordLayout::checkSearchCompletion() {
    if (!isSearching_.load()) {
        return;
    }

    // 检查异步任务是否完成
    if (searchFuture_.valid() && searchFuture_.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
        try {
            SearchResult result = searchFuture_.get();

            // 处理搜索结果
            if (result.found && !result.url.empty()) {
                foundUrl_ = result.url;
                // 如果路径验证成功且是自动搜索，保存路径
                if (result.path.empty() && !manualGamePath_.empty()) {
                    result.path = manualGamePath_;
                }
                if (!result.path.empty()) {
                    manualGamePath_ = result.path; // 保存找到的路径
                }
                showManualInput_ = true;
                updateStatus("成功找到抽卡记录URL！", ExchangeRecordState::FOUND_URL);
                copyUrlToClipboard();
                DEARTS_LOG_INFO("异步搜索成功找到URL: " + result.url);
            } else if (result.found) {
                // 如果路径验证成功且是自动搜索，保存路径
                if (result.path.empty() && !manualGamePath_.empty()) {
                    result.path = manualGamePath_;
                }
                if (!result.path.empty()) {
                    manualGamePath_ = result.path; // 保存找到的路径
                }
                showManualInput_ = true;
                updateStatus("游戏路径有效，但未找到抽卡记录URL。请确保已打开游戏内的抽卡记录页面。", ExchangeRecordState::FOUND_LOG);
                DEARTS_LOG_INFO("异步搜索找到路径但未找到URL: " + result.message);
            } else {
                // 路径验证失败，如果是路径验证，启动自动搜索
                if (statusMessage_.find("正在从保存路径重新搜索") != std::string::npos) {
                    DEARTS_LOG_INFO("保存的路径验证失败，启动自动搜索");
                    performAutoSearchAsync();
                    // 不重置搜索状态，让自动搜索继续
                    return;
                } else {
                    updateStatus("未能自动找到游戏安装路径，请手动选择游戏安装目录。", ExchangeRecordState::SEARCH_ERROR);
                    showManualInput_ = true;
                    DEARTS_LOG_WARN("异步搜索未找到有效路径: " + result.message);
                }
            }

            // 添加最终搜索结果
            {
                std::lock_guard<std::mutex> lock(searchMutex_);
                searchResults_.push_back(result);
            }
            autoSearchCompleted_ = true;

            // 保存配置
            saveConfiguration();

        } catch (const std::exception& e) {
            DEARTS_LOG_ERROR("获取异步搜索结果时发生异常: " + std::string(e.what()));
            updateStatus("搜索过程中发生错误，请手动选择游戏路径。", ExchangeRecordState::SEARCH_ERROR);
            showManualInput_ = true;
        }

        // 重置搜索状态
        isSearching_.store(false);
        currentSearchPhase_.clear();
        currentProgress_ = 0;
    }
}

/**
 * @brief 更新搜索进度
 */
void ExchangeRecordLayout::updateSearchProgress(const std::string& phase, int progress) {
    std::lock_guard<std::mutex> lock(searchMutex_);
    currentSearchPhase_ = phase;
    currentProgress_ = progress;

    // 更新状态消息以显示进度
    std::string progressMsg = "正在搜索: " + phase + " (" + std::to_string(progress) + "%)";
    statusMessage_ = progressMsg;

    DEARTS_LOG_DEBUG("搜索进度更新: " + progressMsg);
}

} // namespace Window
} // namespace Core
} // namespace DearTs