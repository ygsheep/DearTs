/**
 * @file gacha_record_view.cpp
 * @brief 鸣潮抽卡记录视图实现
 */

#include "wuthering_waves/gacha_record_view.hpp"
#include "liblogger/logger.h"
#include <imgui.h>
#include <SDL3/SDL.h>
#include <fstream>
#include <filesystem>
#include <regex>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#endif

namespace DearTs::Plugins::WutheringWaves {

void GachaRecordView::draw_content() {
    ImGui::Text("鸣潮抽卡记录获取工具");
    ImGui::Separator();

    // 帮助信息
    if (m_show_help) {
        draw_help_info();
        ImGui::Separator();
    }

    // 扫描方式选择
    ImGui::Text("扫描方式:");
    ImGui::RadioButton("全部 (推荐)", &m_scan_method, 0);
    ImGui::SameLine();
    ImGui::RadioButton("MUI缓存", &m_scan_method, 1);
    ImGui::SameLine();
    ImGui::RadioButton("防火墙", &m_scan_method, 2);
    ImGui::RadioButton("注册表", &m_scan_method, 3);
    ImGui::SameLine();
    ImGui::RadioButton("常见路径", &m_scan_method, 4);

    // 扫描按钮
    if (ImGui::Button("扫描游戏路径", ImVec2(150, 30))) {
        scan_game_paths();
    }

    // 手动输入路径
    ImGui::Separator();
    ImGui::Text("手动指定路径:");
    char buffer[512];
    std::strncpy(buffer, m_manual_path.c_str(), sizeof(buffer));
    if (ImGui::InputText("##ManualPath", buffer, sizeof(buffer))) {
        m_manual_path = buffer;
    }
    ImGui::SameLine();
    if (ImGui::Button("查找", ImVec2(80, 24))) {
        if (!m_manual_path.empty()) {
            m_scan_logs.clear();
            m_scan_logs.push_back("检查手动路径: " + m_manual_path);
            std::string url = find_gacha_url(m_manual_path);
            if (!url.empty()) {
                m_found_url = url;
                m_url_found = true;
                m_scan_logs.push_back("✓ 找到抽卡记录 URL!");
            } else {
                m_scan_logs.push_back("✗ 未找到抽卡记录 URL，请确保已打开抽卡记录界面");
            }
        }
    }

    // 扫描日志
    if (!m_scan_logs.empty()) {
        ImGui::Separator();
        ImGui::Text("扫描日志:");
        if (ImGui::BeginChild("ScanLogs", ImVec2(0, 150), true)) {
            for (const auto& log : m_scan_logs) {
                ImGui::TextUnformatted(log.c_str());
            }
            // 自动滚动到底部
            if (m_scanning) {
                ImGui::SetScrollHereY(1.0f);
            }
        }
        ImGui::EndChild();
    }

    // 结果显示
    if (m_url_found) {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "✓ 成功找到抽卡记录 URL!");
        ImGui::Separator();

        // URL 显示
        ImGui::Text("抽卡记录 URL:");
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
        if (ImGui::BeginChild("URLDisplay", ImVec2(0, 60), true)) {
            ImGui::TextWrapped("%s", m_found_url.c_str());
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();

        // 操作按钮
        if (ImGui::Button("复制到剪贴板", ImVec2(150, 30))) {
            copy_to_clipboard(m_found_url);
            m_scan_logs.push_back("✓ URL 已复制到剪贴板");
        }
        ImGui::SameLine();
        if (ImGui::Button("在浏览器中打开", ImVec2(150, 30))) {
            open_url("https://mc.appfeng.com/gachaLog");
            m_scan_logs.push_back("✓ 已打开抽卡记录分析网站");
        }
        ImGui::SameLine();
        if (ImGui::Button("清空", ImVec2(80, 30))) {
            m_found_url.clear();
            m_url_found = false;
            m_scan_logs.clear();
        }

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.5f, 0.7f, 1.0f, 1.0f), "使用说明:");
        ImGui::BulletText("点击 '复制到剪贴板' 复制 URL");
        ImGui::BulletText("打开抽卡记录分析网站");
        ImGui::BulletText("点击 '导入记录' 按钮");
        ImGui::BulletText("粘贴 URL 并确认");
    }
}

void GachaRecordView::scan_game_paths() {
    m_scan_logs.clear();
    m_scanned_paths.clear();
    m_found_url.clear();
    m_url_found = false;
    m_scanning = true;

    m_scan_logs.push_back("开始扫描游戏路径...");

    // 常见游戏路径列表
    std::vector<std::string> common_paths;

    // 根据选择的扫描方式添加路径
    if (m_scan_method == 0 || m_scan_method == 4) {
        // 获取所有可用盘符
        std::string drives = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        for (char drive : drives) {
            std::string drivePath = std::string(1, drive) + ":\\";
            // 检查盘符是否存在
            if (GetDriveTypeA(drivePath.c_str()) > 1) {  // DRIVE_FIXED 或其他有效类型
                common_paths.push_back(drivePath + "Wuthering Waves Game");
                common_paths.push_back(drivePath + "Wuthering Waves\\Wuthering Waves Game");
                common_paths.push_back(drivePath + "Program Files\\Epic Games\\WutheringWavesj3oFh");
                common_paths.push_back(drivePath + "Program Files\\Epic Games\\WutheringWavesj3oFh\\Wuthering Waves Game");
            }
        }
    }

    if (m_scan_method == 0 || m_scan_method == 1) {
        m_scan_logs.push_back("[1/4] 检查 MUI 缓存...");
        // TODO: 实现注册表 MUI 缓存查询
        m_scan_logs.push_back("    MUI 缓存扫描: 跳过 (需要实现)");
    }

    if (m_scan_method == 0 || m_scan_method == 2) {
        m_scan_logs.push_back("[2/4] 检查防火墙规则...");
        // TODO: 实现防火墙规则查询
        m_scan_logs.push_back("    防火墙扫描: 跳过 (需要实现)");
    }

    if (m_scan_method == 0 || m_scan_method == 3) {
        m_scan_logs.push_back("[3/4] 检查注册表...");
        // TODO: 实现注册表查询
        m_scan_logs.push_back("    注册表扫描: 跳过 (需要实现)");
    }

    if (m_scan_method == 0 || m_scan_method == 4) {
        m_scan_logs.push_back("[4/4] 检查常见安装路径...");
        for (const auto& path : common_paths) {
            if (std::filesystem::exists(path)) {
                m_scan_logs.push_back("    检查: " + path);
                std::string url = find_gacha_url(path);
                if (!url.empty()) {
                    m_found_url = url;
                    m_url_found = true;
                    m_scan_logs.push_back("    ✓ 找到抽卡记录 URL!");
                    m_scanning = false;
                    return;
                }
            }
        }
        m_scan_logs.push_back("    常见路径扫描完成，未找到游戏");
    }

    if (!m_url_found) {
        m_scan_logs.push_back("✗ 未找到抽卡记录 URL");
        m_scan_logs.push_back("  请确保:");
        m_scan_logs.push_back("  1. 游戏已正确安装");
        m_scan_logs.push_back("  2. 已在游戏中打开抽卡记录界面");
        m_scan_logs.push_back("  3. 尝试手动指定游戏路径");
    }

    m_scanning = false;
}

std::string GachaRecordView::find_gacha_url(const std::string& path) {
    // 构建日志文件路径
    std::filesystem::path gamePath(path);
    std::vector<std::string> log_files = {
        (gamePath / "Client" / "Saved" / "Logs" / "Client.log").string(),
        (gamePath / "Client" / "Binaries" / "Win64" / "ThirdParty" / "KrPcSdk_Global" /
         "KRSDKRes" / "KRSDKWebView" / "debug.log").string()
    };

    // 正则表达式匹配 URL
    std::regex url_regex(
        R"(https://aki-gm-resources(-oversea)?\.aki-game\.(net|com)/aki/gacha/index\.html#/record[^"]*)"
    );

    for (const auto& logFile : log_files) {
        if (!std::filesystem::exists(logFile)) {
            continue;
        }

        try {
            // 从文件末尾开始读取（最新的 URL 在后面）
            std::ifstream file(logFile, std::ios::binary | std::ios::ate);
            if (!file.is_open()) {
                continue;
            }

            auto fileSize = file.tellg();
            const size_t maxReadSize = 50 * 1024;  // 只读取最后 50KB
            size_t readSize = std::min(static_cast<size_t>(fileSize), maxReadSize);
            file.seekg(-static_cast<std::streamoff>(readSize), std::ios::end);

            std::string content;
            content.resize(readSize);
            file.read(&content[0], readSize);
            file.close();

            // 搜索 URL
            std::smatch match;
            if (std::regex_search(content, match, url_regex)) {
                return match.str(1);
            }
        } catch (const std::exception& e) {
            LOG_WARN("读取日志文件失败: {} - {}", logFile, e.what());
        }
    }

    return "";
}

void GachaRecordView::copy_to_clipboard(const std::string& text) {
#ifdef _WIN32
    if (OpenClipboard(nullptr)) {
        EmptyClipboard();
        HGLOBAL hglb = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
        if (hglb) {
            char* lptstr = static_cast<char*>(GlobalLock(hglb));
            if (lptstr) {
                memcpy(lptstr, text.c_str(), text.size() + 1);
                GlobalUnlock(hglb);
                SetClipboardData(CF_TEXT, hglb);
            }
        }
        CloseClipboard();
        LOG_INFO("URL 已复制到剪贴板: {}", text);
    }
#endif
}

void GachaRecordView::open_url(const std::string& url) {
#ifdef _WIN32
    ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    LOG_INFO("打开 URL: {}", url);
#endif
}

void GachaRecordView::draw_help_info() {
    if (ImGui::CollapsingHeader("使用帮助", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextWrapped("此工具用于从《鸣潮》游戏日志中提取抽卡记录 URL。");
        ImGui::TextWrapped("使用步骤:");
        ImGui::BulletText("1. 确保游戏已安装");
        ImGui::BulletText("2. 在游戏中打开抽卡记录界面");
        ImGui::BulletText("3. 点击下方 '扫描游戏路径' 按钮");
        ImGui::BulletText("4. 找到 URL 后，点击 '复制到剪贴板'");
        ImGui::BulletText("5. 在抽卡分析网站粘贴导入");

        ImGui::Spacing();
        ImGui::TextWrapped("支持的扫描方式:");
        ImGui::BulletText("MUI 缓存 - 从系统缓存查找");
        ImGui::BulletText("防火墙 - 从防火墙规则查找");
        ImGui::BulletText("注册表 - 从注册表查找");
        ImGui::BulletText("常见路径 - 扫描常见安装位置");

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.5f, 0.7f, 1.0f, 1.0f), "提示: 如果自动扫描失败，可以手动指定游戏路径");
    }
}

} // namespace DearTs::Plugins::WutheringWaves
