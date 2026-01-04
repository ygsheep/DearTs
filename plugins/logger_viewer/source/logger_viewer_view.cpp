/**
 * @file logger_viewer_view.cpp
 * @brief 日志查看器视图实现
 */

#include "views/logger_viewer_view.hpp"
#include "liblogger/logger.h"
#include "core/utils/file_dialog.hpp"
#include "core/ui/icon_font.hpp"
#include "core/content/registry_base.h"
#include "core/tasks/task_manager.h"
#include "core/tasks/task_events.h"
#include "core/event/event_bus.h"
#include "plugins/toast_notification/include/toast_manager.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <imgui.h>
#include <implot.h>
#include <cmath>
#include <thread>

namespace DearTs::Plugins::LoggerViewer {
using DearTs::Core::ContentRegistry::UnlocalizedString;

LoggerViewerView::LoggerViewerView()
    : ViewWindow(UnlocalizedString("日志查看器"), ICON_LOGS) {
    // 订阅任务事件以显示 Toast 通知
    subscribe_to_task_events();

    // 查找日志目录
    std::vector<std::filesystem::path> log_dirs = {
        "logs",
        "build/bin/logs",
        "../logs",
        "../build/bin/logs"
    };

    for (const auto& dir : log_dirs) {
        if (std::filesystem::exists(dir) && std::filesystem::is_directory(dir)) {
            for (const auto& entry : std::filesystem::directory_iterator(dir)) {
                if (entry.is_regular_file() && entry.path().extension() == ".log") {
                    m_log_files.push_back(entry.path());
                }
            }
            if (!m_log_files.empty()) {
                break;  // 找到日志文件后停止
            }
        }
    }

    // 按修改时间排序（最新的在前）
    std::sort(m_log_files.begin(), m_log_files.end(),
        [](const auto& a, const auto& b) {
            return std::filesystem::last_write_time(a) > std::filesystem::last_write_time(b);
        });

    // 自动加载第一个日志文件
    if (!m_log_files.empty()) {
        m_selected_log_index = 0;
        load_log_file(m_log_files[0]);
    }
}

LoggerViewerView::~LoggerViewerView() {
    // 安全清理：在析构前重置任务指针，避免 Task 析构时触发日志死锁
    // 注意：此时 Logger 可能已经开始析构，调用 LOG 可能导致死锁
    m_loading_task.reset();
}

void LoggerViewerView::draw_content() {
    // 检查文件修改
    check_file_modification();

    draw_toolbar();

    if (m_show_filters) {
        draw_filter_panel();
    }

    draw_search_box();

    if (m_show_time_filter) {
        draw_time_filter_panel();
    }

    if (m_show_statistics) {
        draw_statistics_panel();
    }

    if (m_show_actions) {
        draw_action_panel();
    }

    // 显示图表
    draw_charts();

    ImGui::Separator();

    draw_log_list();

    // 处理刷新和筛选
    if (m_need_refresh) {
        refresh_log();
        m_need_refresh = false;
    }

    if (m_need_filter) {
        apply_filters();
        m_need_filter = false;
    }
}

bool LoggerViewerView::parse_log_line(const std::string& line, LogEntry& entry) {
    return parse_dearts_log(line, entry);
}

bool LoggerViewerView::parse_dearts_log(const std::string& line, LogEntry& entry) {
    // DearTs 日志格式: [2025-12-29 12:34:56.789] [LEVEL] [file.cpp:123] message
    static std::regex log_pattern(
        R"(\[(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{3})\]\s+\[(\w+)\]\s+\[([^:]+):(\d+)\]\s+(.*))"
    );

    std::smatch match;
    if (std::regex_search(line, match, log_pattern)) {
        entry.timestamp = match[1].str();
        entry.raw_line = line;

        // 解析日志级别
        std::string level_str = match[2].str();
        entry.level_str = level_str;

        if (level_str == "TRACE") entry.level = LogLevel::Trace;
        else if (level_str == "DEBUG") entry.level = LogLevel::Debug;
        else if (level_str == "INFO") entry.level = LogLevel::Info;
        else if (level_str == "WARN") entry.level = LogLevel::Warn;
        else if (level_str == "ERROR") entry.level = LogLevel::Error;
        else if (level_str == "FATAL") entry.level = LogLevel::Fatal;
        else entry.level = LogLevel::Unknown;

        entry.file = match[3].str();
        entry.line_number = std::stoi(match[4].str());
        entry.message = match[5].str();

        return true;
    }

    return false;
}

void LoggerViewerView::refresh_log() {
    if (!m_current_log_path.empty()) {
        load_log_file(m_current_log_path);
    }
}

std::pair<std::regex, bool> LoggerViewerView::compile_regex_pattern() const {
    std::regex regex_pattern;
    bool regex_valid = false;

    if (m_use_regex && m_regex_buffer[0] != '\0') {
        try {
            regex_pattern = std::regex(m_regex_buffer,
                m_case_sensitive ? std::regex::ECMAScript : std::regex::icase);
            regex_valid = true;
        } catch (const std::regex_error&) {
            // 正则表达式无效，忽略
        }
    }

    return {regex_pattern, regex_valid};
}

std::pair<std::tm, std::tm> LoggerViewerView::parse_time_filter() const {
    auto parse_time = [](const std::string& time_str) -> std::tm {
        std::tm tm = {};
        if (!time_str.empty()) {
            sscanf(time_str.c_str(), "%d-%d-%d %d:%d:%d",
                &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
                &tm.tm_hour, &tm.tm_min, &tm.tm_sec);
            tm.tm_year -= 1900;  // tm_year 是从1900开始的
            tm.tm_mon -= 1;     // tm_mon 是0-11
        }
        return tm;
    };

    std::tm start_tm = parse_time(m_time_filter.start_time);
    std::tm end_tm = parse_time(m_time_filter.end_time);

    return {start_tm, end_tm};
}

bool LoggerViewerView::passes_level_filter(const LogEntry& entry) const {
    return m_level_filters[static_cast<int>(entry.level)];
}

bool LoggerViewerView::passes_time_filter(const LogEntry& entry, const std::tm& start_tm, const std::tm& end_tm) const {
    if (!m_time_filter.enabled) {
        return true;
    }

    // 解析条目时间
    std::tm entry_tm = {};
    sscanf(entry.timestamp.c_str(), "%d-%d-%d %d:%d:%d",
        &entry_tm.tm_year, &entry_tm.tm_mon, &entry_tm.tm_mday,
        &entry_tm.tm_hour, &entry_tm.tm_min, &entry_tm.tm_sec);
    entry_tm.tm_year -= 1900;
    entry_tm.tm_mon -= 1;

    // 检查开始时间（需要创建非 const 副本以供 mktime 使用）
    if (!m_time_filter.start_time.empty()) {
        std::tm start_copy = start_tm;
        std::tm entry_copy = entry_tm;
        if (std::mktime(&entry_copy) < std::mktime(&start_copy)) {
            return false;
        }
    }

    // 检查结束时间（需要创建非 const 副本以供 mktime 使用）
    if (!m_time_filter.end_time.empty()) {
        std::tm end_copy = end_tm;
        std::tm entry_copy = entry_tm;
        if (std::mktime(&entry_copy) > std::mktime(&end_copy)) {
            return false;
        }
    }

    return true;
}

bool LoggerViewerView::passes_search_filter(const LogEntry& entry, const std::regex& regex_pattern, bool regex_valid) const {
    // 如果没有搜索条件，则通过筛选
    if (m_search_buffer[0] == '\0' && m_regex_buffer[0] == '\0') {
        return true;
    }

    if (m_use_regex) {
        // 正则表达式搜索
        if (!regex_valid || !std::regex_search(entry.message, regex_pattern)) {
            return false;
        }
    } else {
        // 普通关键词搜索
        std::string search_str = m_search_buffer;
        std::string message = entry.message;

        if (!m_case_sensitive) {
            // 转换为小写比较
            std::string lower_search = search_str;
            std::string lower_msg = message;

            std::transform(lower_search.begin(), lower_search.end(), lower_search.begin(), ::tolower);
            std::transform(lower_msg.begin(), lower_msg.end(), lower_msg.begin(), ::tolower);

            if (lower_msg.find(lower_search) == std::string::npos) {
                return false;
            }
        } else {
            if (message.find(search_str) == std::string::npos) {
                return false;
            }
        }
    }

    return true;
}

void LoggerViewerView::apply_filters() {
    m_filtered_indices.clear();

    // 预编译正则表达式
    auto [regex_pattern, regex_valid] = compile_regex_pattern();

    // 解析时间范围
    auto [start_tm, end_tm] = parse_time_filter();

    // 应用所有筛选条件
    for (size_t i = 0; i < m_log_entries.size(); ++i) {
        auto& entry = m_log_entries[i];
        entry.visible = true;

        // 级别筛选
        if (!passes_level_filter(entry)) {
            entry.visible = false;
            continue;
        }

        // 时间筛选
        if (!passes_time_filter(entry, start_tm, end_tm)) {
            entry.visible = false;
            continue;
        }

        // 搜索筛选
        if (!passes_search_filter(entry, regex_pattern, regex_valid)) {
            entry.visible = false;
            continue;
        }

        // 通过所有筛选条件
        if (entry.visible) {
            m_filtered_indices.push_back(i);
        }
    }

    // 去重处理
    if (m_remove_duplicates) {
        remove_duplicates();
    }

    update_statistics();
}

void LoggerViewerView::remove_duplicates() {
    std::unordered_map<std::string, size_t> message_map;
    std::vector<size_t> unique_indices;

    for (size_t idx : m_filtered_indices) {
        auto& entry = m_log_entries[idx];
        std::string key = entry.message;

        auto it = message_map.find(key);
        if (it != message_map.end()) {
            // 标记为重复
            size_t first_idx = it->second;
            m_log_entries[first_idx].duplicate_count++;
            entry.is_duplicate = true;

            if (!m_consecutive_only) {
                // 非连续模式：隐藏后续重复项
                entry.visible = false;
            }
        } else {
            message_map[key] = idx;
            unique_indices.push_back(idx);
        }
    }

    // Both branches do the same thing
    m_filtered_indices = unique_indices;

    // 重新计数可见项
    m_filtered_indices.clear();
    for (size_t i = 0; i < m_log_entries.size(); ++i) {
        if (m_log_entries[i].visible && !m_log_entries[i].is_duplicate) {
            m_filtered_indices.push_back(i);
        }
    }
}

void LoggerViewerView::update_statistics() {
    m_statistics.total_count = static_cast<int>(m_log_entries.size());
    m_statistics.trace_count = 0;
    m_statistics.debug_count = 0;
    m_statistics.info_count = 0;
    m_statistics.warn_count = 0;
    m_statistics.error_count = 0;
    m_statistics.fatal_count = 0;
    m_statistics.duplicate_count = 0;
    m_statistics.visible_count = 0;

    for (const auto& entry : m_log_entries) {
        switch (entry.level) {
            case LogLevel::Trace: m_statistics.trace_count++; break;
            case LogLevel::Debug: m_statistics.debug_count++; break;
            case LogLevel::Info: m_statistics.info_count++; break;
            case LogLevel::Warn: m_statistics.warn_count++; break;
            case LogLevel::Error: m_statistics.error_count++; break;
            case LogLevel::Fatal: m_statistics.fatal_count++; break;
            default: break;
        }

        if (entry.is_duplicate) {
            m_statistics.duplicate_count++;
        }

        if (entry.visible) {
            m_statistics.visible_count++;
        }
    }
}

ImVec4 LoggerViewerView::get_level_color(LogLevel level) const {
    if (!m_color_coded) return ImGui::GetStyleColorVec4(ImGuiCol_Text);

    switch (level) {
        case LogLevel::Trace: return ImVec4(0.6f, 0.6f, 0.6f, 1.0f);  // 灰色
        case LogLevel::Debug: return ImVec4(0.5f, 0.5f, 1.0f, 1.0f);  // 浅蓝色
        case LogLevel::Info:  return ImVec4(0.3f, 0.6f, 1.0f, 1.0f);  // 蓝色
        case LogLevel::Warn:  return ImVec4(1.0f, 0.6f, 0.0f, 1.0f);  // 橙色
        case LogLevel::Error: return ImVec4(1.0f, 0.2f, 0.2f, 1.0f);  // 红色
        case LogLevel::Fatal: return ImVec4(1.0f, 0.0f, 0.0f, 1.0f);  // 深红色
        default: return ImVec4(0.7f, 0.7f, 0.7f, 1.0f);               // 浅灰色
    }
}

void LoggerViewerView::draw_toolbar() {
    if (ImGui::Button("刷新")) {
        m_need_refresh = true;
    }

    ImGui::SameLine();
    if (ImGui::Button("清空")) {
        m_log_entries.clear();
        m_filtered_indices.clear();
        update_statistics();
    }

    ImGui::SameLine();
    if (ImGui::Button("浏览文件...")) {
        browse_log_file();
    }

    ImGui::SameLine();
    ImGui::Checkbox("筛选面板", &m_show_filters);

    ImGui::SameLine();
    ImGui::Checkbox("时间筛选", &m_show_time_filter);

    ImGui::SameLine();
    ImGui::Checkbox("统计信息", &m_show_statistics);

    ImGui::SameLine();
    ImGui::Checkbox("图表", &m_show_charts);

    ImGui::SameLine();
    ImGui::Checkbox("操作面板", &m_show_actions);

    ImGui::SameLine();
    ImGui::Checkbox("自动刷新", &m_auto_refresh);

    if (m_auto_refresh) {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100.0f);
        ImGui::InputFloat("##refresh_interval", &m_refresh_interval, 1.0f, 5.0f, "%.0f 秒");
    }

    ImGui::SameLine();
    ImGui::Checkbox("自动滚动", &m_auto_scroll);

    ImGui::SameLine();
    ImGui::Checkbox("显示行号", &m_show_line_numbers);

    ImGui::SameLine();
    ImGui::Checkbox("彩色标记", &m_color_coded);

    ImGui::Separator();

    // 日志文件选择
    if (!m_log_files.empty()) {
        ImGui::Text("日志文件:");
        ImGui::SameLine();

        // 修复：使用持久化字符串避免临时对象问题
        static std::string current_filename;
        current_filename = m_current_log_path.filename().string();

        if (ImGui::BeginCombo("##log_files", current_filename.c_str())) {
            for (size_t i = 0; i < m_log_files.size(); ++i) {
                bool is_selected = (m_selected_log_index == static_cast<int>(i));
                // 修复：使用持久化字符串
                std::string filename = m_log_files[i].filename().string();
                if (ImGui::Selectable(filename.c_str(), is_selected)) {
                    m_selected_log_index = static_cast<int>(i);
                    load_log_file(m_log_files[i]);
                }
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
    }

    // 显示加载进度
    if (m_is_loading) {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.3f, 0.6f, 1.0f, 1.0f), "正在加载日志...");
        ImGui::ProgressBar(m_loading_progress, ImVec2(200.0f, 0.0f));
        ImGui::SameLine();
        ImGui::Text("%.1f%%", m_loading_progress * 100);

        // 取消按钮
        if (ImGui::Button("取消##cancel_load")) {
            cancel_current_task();
        }
        ImGui::Separator();
    } else {
        ImGui::Separator();
    }
}

void LoggerViewerView::draw_filter_panel() {
    ImGui::Text("级别筛选:");
    ImGui::Checkbox("TRACE", &m_level_filters[0]); ImGui::SameLine();
    ImGui::Checkbox("DEBUG", &m_level_filters[1]); ImGui::SameLine();
    ImGui::Checkbox("INFO", &m_level_filters[2]); ImGui::SameLine();
    ImGui::Checkbox("WARN", &m_level_filters[3]); ImGui::SameLine();
    ImGui::Checkbox("ERROR", &m_level_filters[4]); ImGui::SameLine();
    ImGui::Checkbox("FATAL", &m_level_filters[5]);

    ImGui::Separator();

    ImGui::Text("去重选项:");
    ImGui::Checkbox("启用去重", &m_remove_duplicates);
    ImGui::SameLine();
    ImGui::Checkbox("仅连续", &m_consecutive_only);

    if (ImGui::Button("应用筛选")) {
        m_need_filter = true;
    }

    ImGui::Separator();
}

void LoggerViewerView::draw_statistics_panel() {
    ImGui::Text("统计信息:");
    ImGui::Separator();

    ImGui::Text("总计: %d", m_statistics.total_count);
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "TRACE: %d", m_statistics.trace_count);
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 1.0f, 1.0f), "DEBUG: %d", m_statistics.debug_count);
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.3f, 0.6f, 1.0f, 1.0f), "INFO: %d", m_statistics.info_count);

    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "WARN: %d", m_statistics.warn_count);
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "ERROR: %d", m_statistics.error_count);
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "FATAL: %d", m_statistics.fatal_count);

    ImGui::Text("可见: %d", m_statistics.visible_count);
    ImGui::SameLine();
    if (m_statistics.duplicate_count > 0) {
        ImGui::TextColored(ImVec4(0.6f, 0.4f, 0.0f, 1.0f), "重复: %d", m_statistics.duplicate_count);
    }

    ImGui::Separator();
}

void LoggerViewerView::draw_search_box() {
    ImGui::Text("搜索:");
    ImGui::SameLine();
    ImGui::InputText("##search", m_search_buffer, sizeof(m_search_buffer));

    ImGui::SameLine();
    ImGui::Checkbox("正则", &m_use_regex);

    if (m_use_regex) {
        ImGui::SameLine();
        ImGui::InputText("##regex", m_regex_buffer, sizeof(m_regex_buffer));
        ImGui::SameLine();
        ImGui::Checkbox("区分大小写", &m_case_sensitive);
    }

    ImGui::SameLine();
    if (ImGui::Button("搜索")) {
        m_need_filter = true;
    }

    ImGui::SameLine();
    if (ImGui::Button("清除")) {
        m_search_buffer[0] = '\0';
        m_regex_buffer[0] = '\0';
        m_need_filter = true;
    }

    ImGui::Separator();
}

void LoggerViewerView::draw_log_list() {
    if (m_filtered_indices.empty()) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "没有日志条目");
        return;
    }

    // 计算表格列宽
    float timestamp_width = 140.0f;
    float level_width = 70.0f;
    float location_width = 150.0f;
    float line_number_width = 60.0f;

    // 表头
    if (ImGui::BeginTable("LogTable", 5, ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable
        | ImGuiTableFlags_Hideable | ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersOuter)) {

        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("时间", ImGuiTableColumnFlags_WidthFixed, timestamp_width);
        ImGui::TableSetupColumn("级别", ImGuiTableColumnFlags_WidthFixed, level_width);
        ImGui::TableSetupColumn("位置", ImGuiTableColumnFlags_WidthFixed, location_width);
        ImGui::TableSetupColumn("行号", ImGuiTableColumnFlags_WidthFixed, line_number_width);
        ImGui::TableSetupColumn("消息");
        ImGui::TableHeadersRow();

        // 自动滚动到底部
        if (m_scroll_to_bottom) {
            ImGui::SetScrollHereY(1.0f);
            m_scroll_to_bottom = false;
        }

        // 日志条目
        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(m_filtered_indices.size()));

        while (clipper.Step()) {
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
                size_t idx = m_filtered_indices[row];
                auto& entry = m_log_entries[idx];

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(entry.timestamp.c_str());

                ImGui::TableSetColumnIndex(1);
                ImVec4 level_color = get_level_color(entry.level);
                ImGui::TextColored(level_color, "%s", entry.level_str.c_str());

                ImGui::TableSetColumnIndex(2);
                ImGui::TextUnformatted(entry.file.c_str());

                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%d", entry.line_number);

                ImGui::TableSetColumnIndex(4);
                ImGui::TextColored(level_color, "%s", entry.message.c_str());

                // 显示重复次数
                if (entry.duplicate_count > 0) {
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0.6f, 0.4f, 0.0f, 1.0f), " (x%d)", entry.duplicate_count + 1);
                }
            }
        }

        clipper.End();

        ImGui::EndTable();
    }
}

void LoggerViewerView::draw_time_filter_panel() {
    ImGui::Text("时间筛选:");
    ImGui::Checkbox("启用", &m_time_filter.enabled);
    ImGui::SameLine();
    if (ImGui::Button("应用时间筛选")) {
        m_need_filter = true;
    }

    ImGui::Separator();

    ImGui::Text("开始时间:");
    ImGui::InputText("##start_time", m_start_time_buffer, sizeof(m_start_time_buffer));
    ImGui::SameLine();
    ImGui::Text("(格式: YYYY-MM-DD HH:MM:SS)");

    ImGui::Text("结束时间:");
    ImGui::InputText("##end_time", m_end_time_buffer, sizeof(m_end_time_buffer));
    ImGui::SameLine();
    ImGui::Text("(格式: YYYY-MM-DD HH:MM:SS)");

    ImGui::Separator();

    ImGui::Text("快速选择:");
    ImGui::SameLine();
    if (ImGui::Button("最近1分钟")) {
        auto now = std::time(nullptr);
        auto* tm_now = std::localtime(&now);
        std::strftime(m_end_time_buffer, sizeof(m_end_time_buffer), "%Y-%m-%d %H:%M:%S", tm_now);
        auto past = now - 60;  // 1分钟前
        auto* tm_past = std::localtime(&past);
        std::strftime(m_start_time_buffer, sizeof(m_start_time_buffer), "%Y-%m-%d %H:%M:%S", tm_past);
        m_time_filter.enabled = true;
        m_time_filter.start_time = m_start_time_buffer;
        m_time_filter.end_time = m_end_time_buffer;
        m_need_filter = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("最近5分钟")) {
        auto now = std::time(nullptr);
        auto* tm_now = std::localtime(&now);
        std::strftime(m_end_time_buffer, sizeof(m_end_time_buffer), "%Y-%m-%d %H:%M:%S", tm_now);
        auto past = now - 300;  // 5分钟前
        auto* tm_past = std::localtime(&past);
        std::strftime(m_start_time_buffer, sizeof(m_start_time_buffer), "%Y-%m-%d %H:%M:%S", tm_past);
        m_time_filter.enabled = true;
        m_time_filter.start_time = m_start_time_buffer;
        m_time_filter.end_time = m_end_time_buffer;
        m_need_filter = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("最近1小时")) {
        auto now = std::time(nullptr);
        auto* tm_now = std::localtime(&now);
        std::strftime(m_end_time_buffer, sizeof(m_end_time_buffer), "%Y-%m-%d %H:%M:%S", tm_now);
        auto past = now - 3600;  // 1小时前
        auto* tm_past = std::localtime(&past);
        std::strftime(m_start_time_buffer, sizeof(m_start_time_buffer), "%Y-%m-%d %H:%M:%S", tm_past);
        m_time_filter.enabled = true;
        m_time_filter.start_time = m_start_time_buffer;
        m_time_filter.end_time = m_end_time_buffer;
        m_need_filter = true;
    }

    ImGui::Separator();
}

void LoggerViewerView::draw_action_panel() {
    ImGui::Text("日志操作:");

    if (ImGui::Button("导出所有日志")) {
        export_logs(false);
    }
    ImGui::SameLine();
    if (ImGui::Button("导出筛选后的日志")) {
        export_logs(true);
    }

    ImGui::Separator();

    if (ImGui::Button("删除当前日志文件")) {
        delete_log_file();
    }

    ImGui::Separator();

    if (!m_current_log_path.empty()) {
        ImGui::Text("文件: %s", m_current_log_path.filename().string().c_str());
        try {
            auto file_size = std::filesystem::file_size(m_current_log_path);
            ImGui::Text("大小: %.2f MB", file_size / (1024.0 * 1024.0));

            auto ftime = std::filesystem::last_write_time(m_current_log_path);
            auto sctp = ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now();
            auto stime = std::chrono::system_clock::to_time_t(sctp);
            std::tm ltm = {};
            #ifdef _WIN32
                localtime_s(&ltm, &stime);
            #else
                localtime_r(&stime, &ltm);
            #endif
            char time_buf[64];
            std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &ltm);
            ImGui::Text("修改时间: %s", time_buf);
        } catch (...) {
            // 忽略错误
        }
    }

    ImGui::Separator();
}

void LoggerViewerView::check_file_modification() {
    // 检查自动刷新
    if (m_auto_refresh && !m_current_log_path.empty()) {
        double current_time = ImGui::GetTime();
        if (current_time - m_last_refresh_time >= m_refresh_interval) {
            m_last_refresh_time = current_time;

            try {
                auto current_write_time = std::filesystem::last_write_time(m_current_log_path);
                if (current_write_time != m_last_write_time) {
                    // 文件已修改，自动刷新
                    m_need_refresh = true;
                    LOG_INFO("Log file modified, auto-refreshing");
                }
            } catch (...) {
                // 忽略错误
            }
        }
    }
}

void LoggerViewerView::export_logs(bool filtered_only) {
    if (m_log_entries.empty()) {
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "没有日志可导出");
        return;
    }

    // 生成导出文件名
    auto now = std::time(nullptr);
    auto* tm = std::localtime(&now);
    char time_buf[64];
    std::strftime(time_buf, sizeof(time_buf), "%Y%m%d_%H%M%S", tm);

    std::filesystem::path export_path;
    if (filtered_only) {
        export_path = m_current_log_path.parent_path() / (m_current_log_path.stem().string() + "_filtered_" + time_buf + ".log");
    } else {
        export_path = m_current_log_path.parent_path() / (m_current_log_path.stem().string() + "_" + time_buf + ".log");
    }

    std::ofstream file(export_path);
    if (!file.is_open()) {
        LOG_ERROR("Failed to create export file: {}", export_path.string());
        return;
    }

    int exported_count = 0;
    if (filtered_only) {
        // 导出筛选后的日志
        for (const size_t idx : m_filtered_indices) {
            const auto& entry = m_log_entries[idx];
            file << entry.raw_line << "\n";
            exported_count++;
        }
    } else {
        // 导出所有日志
        for (const auto& entry : m_log_entries) {
            file << entry.raw_line << "\n";
            exported_count++;
        }
    }

    file.close();

    LOG_INFO("Exported {} log entries to {}", exported_count, export_path.string());
    ImGui::TextColored(ImVec4(0.3f, 0.6f, 1.0f, 1.0f), "已导出 %d 条日志到: %s", exported_count, export_path.filename().string().c_str());
}

void LoggerViewerView::delete_log_file() {
    if (m_current_log_path.empty()) {
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "没有选中的日志文件");
        return;
    }

    // 确认对话框（简化版，实际应该使用 ImGui 弹窗）
    static bool show_confirm = false;
    if (!show_confirm) {
        show_confirm = true;
        return;
    }

    if (ImGui::Button("确认删除")) {
        try {
            std::filesystem::remove(m_current_log_path);
            LOG_INFO("Deleted log file: {}", m_current_log_path.string());

            // 从列表中移除
            m_log_files.erase(
                std::remove_if(m_log_files.begin(), m_log_files.end(),
                    [this](const auto& p) { return p == m_current_log_path; }
                ),
                m_log_files.end()
            );

            // 清空显示
            m_log_entries.clear();
            m_filtered_indices.clear();
            m_current_log_path.clear();
            m_selected_log_index = -1;

            update_statistics();

            // 加载下一个文件（如果有）
            if (!m_log_files.empty()) {
                m_selected_log_index = 0;
                load_log_file(m_log_files[0]);
            }

            show_confirm = false;
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to delete log file: {}", e.what());
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("取消")) {
        show_confirm = false;
    }
}

void LoggerViewerView::draw_charts() {
    if (!m_show_charts) {
        return;
    }

    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.3f, 0.6f, 1.0f, 1.0f), "📊 统计图表");
    ImGui::Separator();

    // 显示两个图表
    if (ImGui::BeginTabBar("ChartTabs")) {
        if (ImGui::BeginTabItem("级别分布")) {
            draw_level_pie_chart();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("时间线")) {
            draw_timeline_chart();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

void LoggerViewerView::draw_level_pie_chart() {
    if (m_log_entries.empty()) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "暂无日志数据");
        return;
    }

    // 准备数据
    const char* level_labels[] = {"Trace", "Debug", "Info", "Warn", "Error", "Fatal"};
    int level_counts[] = {
        m_statistics.trace_count,
        m_statistics.debug_count,
        m_statistics.info_count,
        m_statistics.warn_count,
        m_statistics.error_count,
        m_statistics.fatal_count
    };

    // 计算总数（仅显示有数据的级别）
    int total = 0;
    std::vector<int> valid_counts;
    std::vector<const char*> valid_labels;
    std::vector<ImVec4> valid_colors;

    ImVec4 level_colors[] = {
        ImVec4(0.6f, 0.6f, 0.6f, 1.0f),  // Trace - 灰色
        ImVec4(0.5f, 0.5f, 1.0f, 1.0f),  // Debug - 浅蓝
        ImVec4(0.3f, 0.6f, 1.0f, 1.0f),  // Info - 蓝色
        ImVec4(1.0f, 0.6f, 0.0f, 1.0f),  // Warn - 橙色
        ImVec4(1.0f, 0.2f, 0.2f, 1.0f),  // Error - 红色
        ImVec4(1.0f, 0.0f, 0.0f, 1.0f)   // Fatal - 深红
    };

    for (int i = 0; i < 6; i++) {
        if (level_counts[i] > 0) {
            valid_counts.push_back(level_counts[i]);
            valid_labels.push_back(level_labels[i]);
            valid_colors.push_back(level_colors[i]);
            total += level_counts[i];
        }
    }

    if (valid_counts.empty()) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "暂无日志数据");
        return;
    }

    // 显示饼图
    if (ImPlot::BeginPlot("日志级别分布", ImVec2(-1, 300))) {
        ImPlot::PlotPieChart(
            valid_labels.data(),
            valid_counts.data(),
            static_cast<int>(valid_counts.size()),
            0.5,
            0.5,
            0.4,
            "%.0f",
            90.0
        );
        ImPlot::EndPlot();
    }

    // 显示统计表格
    ImGui::Separator();
    ImGui::Text("详细统计:");
    if (ImGui::BeginTable("LevelStatsTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("级别");
        ImGui::TableSetupColumn("数量");
        ImGui::TableSetupColumn("百分比");
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < valid_counts.size(); i++) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextColored(valid_colors[i], "%s", valid_labels[i]);
            ImGui::TableNextColumn();
            ImGui::Text("%d", valid_counts[i]);
            ImGui::TableNextColumn();
            ImGui::Text("%.1f%%", (valid_counts[i] * 100.0f) / total);
        }
        ImGui::EndTable();
    }
}

void LoggerViewerView::draw_timeline_chart() {
    if (m_log_entries.empty()) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "暂无日志数据");
        return;
    }

    // 使用筛选后的数据
    std::vector<std::pair<double, LogLevel>> time_level_pairs;

    for (size_t idx : m_filtered_indices) {
        const auto& entry = m_log_entries[idx];
        if (!entry.visible) continue;

        // 解析时间戳
        std::tm tm = {};
        if (sscanf(entry.timestamp.c_str(), "%d-%d-%d %d:%d:%d",
            &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
            &tm.tm_hour, &tm.tm_min, &tm.tm_sec) == 6) {
            tm.tm_year -= 1900;
            tm.tm_mon -= 1;
            double timestamp = static_cast<double>(std::mktime(&tm));
            time_level_pairs.emplace_back(timestamp, entry.level);
        }
    }

    if (time_level_pairs.empty()) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "暂无有效时间数据");
        return;
    }

    // 按时间排序
    std::sort(time_level_pairs.begin(), time_level_pairs.end());

    // 聚合数据（每60秒一个桶）
    double start_time = time_level_pairs.front().first;
    double end_time = time_level_pairs.back().first;
    double bucket_size = 60.0; // 60秒

    size_t num_buckets = static_cast<size_t>((end_time - start_time) / bucket_size) + 1;

    std::vector<double> bucket_trace(num_buckets, 0.0);
    std::vector<double> bucket_debug(num_buckets, 0.0);
    std::vector<double> bucket_info(num_buckets, 0.0);
    std::vector<double> bucket_warn(num_buckets, 0.0);
    std::vector<double> bucket_error(num_buckets, 0.0);
    std::vector<double> bucket_fatal(num_buckets, 0.0);

    for (const auto& [timestamp, level] : time_level_pairs) {
        size_t bucket = static_cast<size_t>((timestamp - start_time) / bucket_size);
        if (bucket >= num_buckets) bucket = num_buckets - 1;

        switch (level) {
            case LogLevel::Trace: bucket_trace[bucket]++; break;
            case LogLevel::Debug: bucket_debug[bucket]++; break;
            case LogLevel::Info:  bucket_info[bucket]++; break;
            case LogLevel::Warn:  bucket_warn[bucket]++; break;
            case LogLevel::Error: bucket_error[bucket]++; break;
            case LogLevel::Fatal: bucket_fatal[bucket]++; break;
            default: break;
        }
    }

    // 准备 X 轴标签（时间）
    std::vector<double> x_data(num_buckets);
    std::vector<std::string> x_labels;

    for (size_t i = 0; i < num_buckets; i++) {
        x_data[i] = start_time + i * bucket_size;

        // 只在合理间隔显示标签
        if (i % 5 == 0 || i == num_buckets - 1) {
            std::time_t timestamp = static_cast<std::time_t>(x_data[i]);
            std::tm* tm = std::localtime(&timestamp);
            char buffer[32];
            std::strftime(buffer, sizeof(buffer), "%H:%M", tm);
            x_labels.push_back(buffer);
        } else {
            x_labels.push_back("");
        }
    }

    // 显示线图（更简单的版本）
    if (ImPlot::BeginPlot("日志频率时间线", ImVec2(-1, 400))) {
        ImPlot::SetupAxes("时间", "日志数量");
        ImPlot::SetupAxisLimits(ImAxis_X1, start_time, end_time + bucket_size);

        // 绘制各级别日志数量线条
        ImPlot::PlotLine("Fatal", x_data.data(), bucket_fatal.data(), static_cast<int>(num_buckets));
        ImPlot::PlotLine("Error", x_data.data(), bucket_error.data(), static_cast<int>(num_buckets));
        ImPlot::PlotLine("Warn", x_data.data(), bucket_warn.data(), static_cast<int>(num_buckets));
        ImPlot::PlotLine("Info", x_data.data(), bucket_info.data(), static_cast<int>(num_buckets));
        ImPlot::PlotLine("Debug", x_data.data(), bucket_debug.data(), static_cast<int>(num_buckets));
        ImPlot::PlotLine("Trace", x_data.data(), bucket_trace.data(), static_cast<int>(num_buckets));

        ImPlot::EndPlot();
    }

    // 显示图表说明
    ImGui::Separator();
    ImGui::TextWrapped("图表显示各时间段内的日志数量分布。堆叠区域表示累积数量，不同颜色代表不同级别。");
}

void LoggerViewerView::browse_log_file() {
    using namespace DearTs::Core::Utils;

    // 配置文件对话框
    FileDialogOptions options;
    options.title = "选择日志文件";
    options.filters = {
        FileFilter("日志文件", "*.log"),
        FileFilter("文本文件", "*.txt"),
        FileFilter("所有文件", "*.*")
    };

    // 设置默认路径为当前日志文件所在目录
    if (!m_current_log_path.empty() && m_current_log_path.has_parent_path()) {
        options.default_path = m_current_log_path.parent_path();
    } else {
        options.default_path = "build/bin/logs";
    }

    // 打开文件对话框
    auto result = FileDialog::open_file(options);

    if (result.success && result.has_paths()) {
        auto selected_path = result.get_first_path();

        // 检查是否已经在列表中
        auto it = std::find(m_log_files.begin(), m_log_files.end(), selected_path);
        if (it == m_log_files.end()) {
            // 添加到列表
            m_log_files.push_back(selected_path);
            m_selected_log_index = static_cast<int>(m_log_files.size() - 1);
        } else {
            // 已存在，选择它
            m_selected_log_index = static_cast<int>(it - m_log_files.begin());
        }

        // 加载文件（会自动取消之前的任务）
        load_log_file(selected_path);

        LOG_INFO("Opened log file: {}", selected_path.string());
    } else if (!result.error_message.empty()) {
        LOG_ERROR("Failed to open file dialog: {}", result.error_message);
    }
}

// ================ 异步加载相关实现 ================

void LoggerViewerView::subscribe_to_task_events() {
    using namespace Core::Tasks;
    using namespace Core::Event;

    auto& bus = EventBus::instance();

    // 订阅任务开始事件
    m_task_started_token = bus.subscribe<TaskStartedEvent>(
        [this](const TaskStartedEvent& e) {
            // 只处理我们自己的任务
            std::lock_guard<std::mutex> lock(m_loading_mutex);
            if (m_loading_task && m_loading_task == e.task) {
                m_is_loading = true;
                m_loading_progress = 0.0F;
                m_loaded_entries = 0;
            }
        }
    );

    // 订阅任务进度事件
    m_task_progress_token = bus.subscribe<TaskProgressEvent>(
        [this](const TaskProgressEvent& e) {
            std::lock_guard<std::mutex> lock(m_loading_mutex);
            if (m_loading_task && m_loading_task == e.task) {
                m_loading_progress = e.progress_percent / 100.0F;
            }
        }
    );

    // 订阅任务完成事件
    m_task_completed_token = bus.subscribe<TaskCompletedEvent>(
        [this](const TaskCompletedEvent& e) {
            std::lock_guard<std::mutex> lock(m_loading_mutex);
            if (m_loading_task && m_loading_task == e.task) {
                m_is_loading = false;
                m_loading_progress = 1.0F;
            }
        }
    );

    // 订阅任务失败事件
    m_task_failed_token = bus.subscribe<TaskFailedEvent>(
        [this](const TaskFailedEvent& e) {
            std::lock_guard<std::mutex> lock(m_loading_mutex);
            if (m_loading_task && m_loading_task == e.task) {
                m_is_loading = false;
                DearTs::Plugins::Toast::ToastManager::instance().error(
                    "加载失败",
                    e.error_message
                );
            }
        }
    );

    // 订阅任务取消事件
    m_task_cancelled_token = bus.subscribe<TaskCancelledEvent>(
        [this](const TaskCancelledEvent& e) {
            std::lock_guard<std::mutex> lock(m_loading_mutex);
            if (m_loading_task && m_loading_task == e.task) {
                m_is_loading = false;
                DearTs::Plugins::Toast::ToastManager::instance().warning(
                    "加载已取消",
                    "日志文件加载已被取消"
                );
            }
        }
    );
}

void LoggerViewerView::cancel_current_task() {
    std::lock_guard<std::mutex> lock(m_loading_mutex);
    if (m_loading_task && m_loading_task->isRunning()) {
        Core::Tasks::TaskManager::instance().cancel(m_loading_task);
        LOG_INFO("Cancelled previous loading task");
    }
}

void LoggerViewerView::load_log_file(const std::filesystem::path& path) {
    using namespace Core::Tasks;

    // 取消之前的任务
    cancel_current_task();

    m_current_log_path = path;

    if (!std::filesystem::exists(path)) {
        LOG_WARN("Log file not found: {}", path.string());
        DearTs::Plugins::Toast::ToastManager::instance().error(
            "文件不存在",
            path.string()
        );
        return;
    }

    // 创建新的异步加载任务
    {
        std::lock_guard<std::mutex> lock(m_loading_mutex);
        m_loading_task = TaskManager::instance().launch(
            std::format("加载日志: {}", path.filename().string()),
            [this, path](const std::atomic<bool>& should_cancel) {
                // 获取当前任务指针用于更新进度
                auto& tm = TaskManager::instance();
                auto tasks = tm.getTasks();
                std::shared_ptr<Task> current_task;
                for (const auto& t : tasks) {
                    if (t->isRunning()) {
                        current_task = t;
                        break;
                    }
                }

                load_log_file_async(path, should_cancel, current_task);
            }
        );
    }
}

void LoggerViewerView::load_log_file_async(
    const std::filesystem::path& path,
    const std::atomic<bool>& should_cancel,
    std::shared_ptr<Core::Tasks::Task> task
) {
    // 临时存储加载的日志条目
    std::vector<LogEntry> new_entries;

    try {
        // 获取文件大小用于进度估算
        size_t file_size = std::filesystem::file_size(path);

        std::ifstream file(path);
        if (!file.is_open()) {
            LOG_ERROR("Failed to open log file: {}", path.string());
            return;
        }

        std::string line;
        size_t bytes_read = 0;
        size_t total_lines = 0;

        // 分段读取文件
        while (std::getline(file, line) && !should_cancel) {
            LogEntry entry;
            if (parse_log_line(line, entry)) {
                new_entries.push_back(entry);
            } else if (!line.empty() && line[0] != '[') {
                // 处理多行日志消息
                if (!new_entries.empty()) {
                    new_entries.back().message += "\n" + line;
                }
            }

            // 更新进度
            bytes_read += line.size() + 1; // +1 for newline
            total_lines++;

            if (task) {
                float progress = static_cast<float>(bytes_read) /
                                static_cast<float>(file_size) * 100.0f;
                task->setProgress(progress);
            }

            // 每处理 1000 行让出 CPU 时间
            if (total_lines % 1000 == 0) {
                std::this_thread::yield();
            }
        }

        file.close();

        if (should_cancel) {
            LOG_INFO("Log loading cancelled: {}", path.string());
            return;
        }

        // 在主线程中更新 UI 数据
        std::lock_guard<std::mutex> lock(m_loading_mutex);
        m_log_entries = std::move(new_entries);
        m_loaded_entries = m_log_entries.size();

        // 记录文件修改时间
        try {
            m_last_write_time = std::filesystem::last_write_time(path);
        } catch (...) {
            // 忽略错误
        }

        // 更新统计和筛选
        update_statistics();
        m_need_filter = true;

        LOG_INFO("Loaded {} log entries from {}", m_log_entries.size(), path.string());

    } catch (const std::exception& e) {
        LOG_ERROR("Error loading log file: {}", e.what());
    }
}

} // namespace DearTs::Plugins::LoggerViewer
