/**
 * @file ffmpeg_view.hpp
 * @brief FFmpeg 视频处理视图
 * @details 提供 TS 文件合并等功能，使用现代化 UI
 */

#pragma once

#include "core/ui/view.h"
#include "core/tasks/task_manager.h"
#include "core/event/event_bus.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <mutex>
#include <atomic>
#include <unordered_set>

#if DEARTS_FFMPEG_SUPPORT

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}

// 前向声明 ToastManager
namespace DearTs::Plugins::Toast {
    class ToastManager;
}

namespace DearTs::Plugins::FFmpeg {

/**
 * @brief 编码器类型
 */
enum class EncoderType {
    Auto,           // 自动选择（优先 GPU）
    CPU,            // CPU 软件编码（libx264/libx265）
    NVIDIA_NVENC,   // NVIDIA NVENC 硬件编码（h264_nvenc/hevc_nvenc）
    AMD_VCE,        // AMD VCE 硬件编码（h264_amf/hevc_amf）
    Intel_QSV       // Intel Quick Sync（h264_qsv/hevc_qsv）
};

/**
 * @brief 硬件信息
 */
struct HardwareInfo {
    bool has_nvidia_gpu = false;
    bool has_amd_gpu = false;
    bool has_intel_gpu = false;
    std::string nvidia_gpu_name;
    std::string amd_gpu_name;
    std::string intel_gpu_name;
    EncoderType recommended_encoder = EncoderType::CPU;
};

/**
 * @brief TS 文件信息
 */
struct TSFile {
    std::string path;           // 文件路径
    std::string name;           // 文件名
    size_t size;                // 文件大小（字节）
    double duration;            // 时长（秒）
    bool valid;                 // 是否有效
};

/**
 * @brief 合并进度信息
 */
struct MergeProgress {
    std::atomic<bool> active{false};
    std::atomic<double> progress{0.0};  // 0.0 - 1.0
    std::atomic<int> current_file{0};
    std::atomic<int> total_files{0};
    std::string current_action;
    std::string error_message;
};

/**
 * @brief FFmpeg 视频处理视图
 *
 * 功能：
 * - TS 文件合并（第一个功能）
 * - 拖拽排序
 * - 进度显示
 * - 错误处理
 */
class FFmpegView : public Core::UI::ViewWindow {
public:
    FFmpegView();
    ~FFmpegView() override;

    // View 接口实现
    void draw_content() override;

private:
    // UI 绘制方法
    void draw_header();
    void draw_file_list();
    void draw_control_buttons();
    void draw_progress_dialog();
    void draw_file_selector();

    // 功能方法
    void add_files();
    void add_folder();
    void remove_file(size_t index);
    void clear_files();
    void move_file_up(size_t index);
    void move_file_down(size_t index);
    void start_merge();
    void cancel_merge();
    void confirm_file_selection();

    // 文件选择方法
    void select_all_files();
    void deselect_all_files();
    void remove_selected_files();
    bool is_file_selected(size_t index) const;
    void toggle_file_selection(size_t index);

    // FFmpeg 合并方法
    bool validate_files();
    std::string create_concat_list();
    bool merge_ts_files(const std::string& output_path, const std::atomic<bool>& should_cancel);

    // 硬件检测
    HardwareInfo detect_hardware();
    bool check_encoder_available(EncoderType type);
    const char* get_encoder_name(EncoderType type);
    EncoderType resolve_encoder_type(EncoderType type);

    // 文件夹扫描
    void scan_folder(const std::string& folder_path);
    bool is_ts_file(const std::string& filename);

    // 辅助方法
    std::string format_file_size(size_t size);
    std::string format_duration(double seconds);
    void load_file_info(TSFile& file);
    static std::string get_short_path_name(const std::string& long_path);

    // 成员变量
    std::vector<TSFile> m_files;
    std::vector<TSFile> m_detected_files;  // 检测到的文件（待确认）
    size_t m_selected_file = SIZE_MAX;
    std::unordered_set<size_t> m_selected_files_set;  // 多选文件索引集合

    MergeProgress m_merge_progress;
    std::string m_output_path;

    bool m_show_progress = false;
    bool m_show_file_selector = false;     // 文件选择对话框
    std::string m_current_folder;          // 当前扫描的文件夹

    // 硬件和编码器设置
    HardwareInfo m_hardware_info;
    EncoderType m_selected_encoder = EncoderType::Auto;
    bool m_hardware_detected = false;

    // 任务系统
    std::shared_ptr<Core::Tasks::Task> m_merge_task;

    // 事件订阅（RAII 自动管理）
    Core::Event::EventToken m_task_completed_token;
    Core::Event::EventToken m_task_failed_token;
    Core::Event::EventToken m_task_cancelled_token;

    // 合并设置
    bool m_auto_open_output = true;  // 合并后自动打开输出目录
    int m_output_bitrate = 0;        // 输出码率（0 = 保持原样）
};

} // namespace DearTs::Plugins::FFmpeg

#endif // DEARTS_FFMPEG_SUPPORT
