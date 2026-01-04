/**
 * @file logger_viewer_view.hpp
 * @brief 日志查看器视图
 * @details 提供日志文件查看、筛选、搜索、去重等功能
 * @author DearTs Team
 * @date 2025
 * @version 1.0.0
 */

#pragma once

#include "core/ui/view.h"
#include "core/tasks/task_manager.h"
#include "core/tasks/task_events.h"
#include "core/event/event_bus.h"
#include <string>
#include <vector>
#include <filesystem>
#include <regex>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <atomic>

namespace DearTs::Plugins::LoggerViewer {

/**
 * @brief 日志级别枚举
 */
enum class LogLevel {
    Trace,
    Debug,
    Info,
    Warn,
    Error,
    Fatal,
    Unknown
};

/**
 * @brief 日志条目结构
 */
struct LogEntry {
    std::string timestamp;      // 时间戳
    LogLevel level;             // 日志级别
    std::string level_str;      // 级别字符串（大写）
    std::string file;           // 文件名
    int line_number;            // 行号
    std::string message;        // 日志消息
    std::string raw_line;       // 原始行内容

    // UI 状态
    bool visible = true;        // 是否可见（筛选后）
    bool is_duplicate = false;  // 是否是重复日志
    int duplicate_count = 0;    // 重复次数
};

/**
 * @brief 日志统计信息
 */
struct LogStatistics {
    int total_count = 0;
    int trace_count = 0;
    int debug_count = 0;
    int info_count = 0;
    int warn_count = 0;
    int error_count = 0;
    int fatal_count = 0;
    int duplicate_count = 0;
    int visible_count = 0;
};

/**
 * @brief 时间范围筛选
 */
struct TimeFilter {
    bool enabled = false;
    std::string start_time;  // 格式: "YYYY-MM-DD HH:MM:SS"
    std::string end_time;    // 格式: "YYYY-MM-DD HH:MM:SS"
};

/**
 * @brief 日志查看器视图类
 */
class LoggerViewerView : public Core::UI::ViewWindow {
public:
    LoggerViewerView();
    ~LoggerViewerView() override;

    /**
     * @brief 绘制视图内容
     */
    void draw_content() override;

private:
    /**
     * @brief 解析日志行
     */
    bool parse_log_line(const std::string& line, LogEntry& entry);

    /**
     * @brief 解析 DearTs 日志格式
     * 格式: [2025-12-29 12:34:56.789] [LEVEL] [file.cpp:123] message
     */
    bool parse_dearts_log(const std::string& line, LogEntry& entry);

    /**
     * @brief 加载日志文件（异步，使用 TaskManager）
     */
    void load_log_file(const std::filesystem::path& path);

    /**
     * @brief 异步加载日志文件的实际实现
     * @param path 日志文件路径
     * @param should_cancel 取消标志
     * @param task 任务对象（用于更新进度）
     */
    void load_log_file_async(
        const std::filesystem::path& path,
        const std::atomic<bool>& should_cancel,
        std::shared_ptr<Core::Tasks::Task> task
    );

    /**
     * @brief 取消当前加载任务
     */
    void cancel_current_task();

    /**
     * @brief 刷新日志文件
     */
    void refresh_log();

    /**
     * @brief 订阅任务事件以显示 Toast 通知
     */
    void subscribe_to_task_events();

    /**
     * @brief 编译正则表达式模式
     * @return 编译后的正则表达式和有效性标志
     */
    std::pair<std::regex, bool> compile_regex_pattern() const;

    /**
     * @brief 解析时间筛选范围
     * @return 开始和结束时间的 tm 结构体对
     */
    std::pair<std::tm, std::tm> parse_time_filter() const;

    /**
     * @brief 检查日志条目是否通过级别筛选
     */
    bool passes_level_filter(const LogEntry& entry) const;

    /**
     * @brief 检查日志条目是否通过时间筛选
     */
    bool passes_time_filter(const LogEntry& entry, const std::tm& start_tm, const std::tm& end_tm) const;

    /**
     * @brief 检查日志条目是否通过搜索筛选
     */
    bool passes_search_filter(const LogEntry& entry, const std::regex& regex_pattern, bool regex_valid) const;

    /**
     * @brief 应用筛选
     */
    void apply_filters();

    /**
     * @brief 去重处理
     */
    void remove_duplicates();

    /**
     * @brief 更新统计信息
     */
    void update_statistics();

    /**
     * @brief 获取日志级别颜色
     */
    ImVec4 get_level_color(LogLevel level) const;

    /**
     * @brief 绘制工具栏
     */
    void draw_toolbar();

    /**
     * @brief 绘制筛选面板
     */
    void draw_filter_panel();

    /**
     * @brief 绘制统计面板
     */
    void draw_statistics_panel();

    /**
     * @brief 绘制日志列表
     */
    void draw_log_list();

    /**
     * @brief 绘制搜索框
     */
    void draw_search_box();

    /**
     * @brief 绘制时间筛选面板
     */
    void draw_time_filter_panel();

    /**
     * @brief 绘制操作面板
     */
    void draw_action_panel();

    /**
     * @brief 检查文件是否有更新
     */
    void check_file_modification();

    /**
     * @brief 导出日志
     */
    void export_logs(bool filtered_only);

    /**
     * @brief 删除日志文件
     */
    void delete_log_file();

    /**
     * @brief 浏览并选择日志文件
     */
    void browse_log_file();

    /**
     * @brief 绘制统计图表
     */
    void draw_charts();

    /**
     * @brief 绘制日志级别分布饼图
     */
    void draw_level_pie_chart();

    /**
     * @brief 绘制日志频率时间线图
     */
    void draw_timeline_chart();

private:
    // 日志数据
    std::vector<LogEntry> m_log_entries;
    std::vector<size_t> m_filtered_indices;  // 筛选后的索引列表
    LogStatistics m_statistics;

    // 文件信息
    std::filesystem::path m_current_log_path;
    std::vector<std::filesystem::path> m_log_files;
    int m_selected_log_index = 0;

    // 筛选状态
    bool m_show_filters = true;
    bool m_level_filters[6] = {true, true, true, true, true, true};  // 全部启用
    char m_search_buffer[256] = "";
    char m_regex_buffer[256] = "";
    bool m_use_regex = false;
    bool m_case_sensitive = false;

    // 去重选项
    bool m_remove_duplicates = false;
    bool m_consecutive_only = false;  // 仅合并连续重复

    // 时间筛选
    TimeFilter m_time_filter;
    char m_start_time_buffer[32] = "";
    char m_end_time_buffer[32] = "";

    // 显示选项
    bool m_auto_scroll = false;
    bool m_show_line_numbers = true;
    bool m_color_coded = true;
    bool m_auto_refresh = false;  // 自动刷新
    float m_refresh_interval = 5.0f;  // 刷新间隔（秒）
    double m_last_refresh_time = 0.0;

    // 文件监控
    std::filesystem::file_time_type m_last_write_time;

    // UI 状态
    bool m_show_statistics = false;
    bool m_show_time_filter = false;
    bool m_show_actions = false;
    bool m_show_charts = false;  // 显示图表
    int m_selected_entry = -1;
    bool m_scroll_to_bottom = false;

    // 性能优化
    bool m_need_refresh = true;
    bool m_need_filter = true;

    // 任务管理
    std::shared_ptr<Core::Tasks::Task> m_loading_task;      // 当前加载任务
    std::mutex m_loading_mutex;                             // 保护加载状态的互斥锁
    std::atomic<bool> m_is_loading{false};                  // 是否正在加载
    float m_loading_progress = 0.0f;                        // 加载进度 (0.0 - 1.0)
    size_t m_loaded_entries = 0;                            // 已加载的条目数
    size_t m_total_entries = 0;                             // 总条目数估计

    // 任务事件订阅（RAII 自动取消订阅）
    Core::Event::EventToken m_task_started_token;
    Core::Event::EventToken m_task_progress_token;
    Core::Event::EventToken m_task_completed_token;
    Core::Event::EventToken m_task_failed_token;
    Core::Event::EventToken m_task_cancelled_token;
};

} // namespace DearTs::Plugins::LoggerViewer
