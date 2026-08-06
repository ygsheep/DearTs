/**
 * @file gacha_record_view.hpp
 * @brief 鸣潮抽卡记录视图
 */

#pragma once

#include "core/ui/view.h"
#include <string>
#include <vector>

namespace DearTs::Plugins::WutheringWaves {

/**
 * @brief 鸣潮抽卡记录获取视图
 *
 * @details 功能：
 * - 自动扫描游戏安装路径
 * - 从日志文件中提取抽卡记录 URL
 * - 显示扫描进度和结果
 * - 一键复制 URL 或在浏览器中打开
 */
class GachaRecordView : public Core::UI::ViewWindow {
public:
    GachaRecordView() : Core::UI::ViewWindow("鸣潮抽卡记录") {}
    ~GachaRecordView() override = default;

    void draw_content() override;

private:
    /**
     * @brief 扫描游戏安装路径
     */
    void scan_game_paths();

    /**
     * @brief 从指定路径查找抽卡记录 URL
     * @param path 游戏安装路径
     * @return 找到的 URL，空字符串表示未找到
     */
    std::string find_gacha_url(const std::string& path);

    /**
     * @brief 复制文本到剪贴板
     * @param text 要复制的文本
     */
    void copy_to_clipboard(const std::string& text);

    /**
     * @brief 打开 URL 在浏览器中
     * @param url 要打开的 URL
     */
    void open_url(const std::string& url);

    /**
     * @brief 绘制帮助信息
     */
    void draw_help_info();

    // 状态变量
    std::vector<std::string> m_scanned_paths;      // 已扫描的路径
    std::vector<std::string> m_scan_logs;         // 扫描日志
    std::string m_found_url;                       // 找到的 URL
    std::string m_selected_path;                   // 当前选中的路径
    std::string m_manual_path;                     // 手动输入的路径
    bool m_scanning = false;                       // 是否正在扫描
    bool m_url_found = false;                      // 是否找到 URL
    bool m_show_help = true;                       // 是否显示帮助信息
    int m_scan_method = 0;                         // 扫描方式：0=全部, 1=MUI缓存, 2=防火墙, 3=注册表, 4=常见路径
};

} // namespace DearTs::Plugins::WutheringWaves
