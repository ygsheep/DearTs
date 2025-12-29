/**
 * @file file_dialog.hpp
 * @brief 跨平台文件对话框工具
 * @details 提供文件打开、保存、文件夹选择对话框，与任务系统集成支持异步文件处理
 * @author DearTs Team
 * @date 2025
 * @version 1.0.0
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <filesystem>
#include <memory>
#include "core/result.h"
#include "core/tasks/task_manager.h"

namespace DearTs::Core::Utils {

// 简化命名空间
using TasksTaskManager = DearTs::Core::Tasks::TaskManager;
using TasksTask = DearTs::Core::Tasks::Task;
using TasksTaskType = DearTs::Core::Tasks::TaskType;

/**
 * @brief 文件过滤器
 */
struct FileFilter {
    std::string name;           // 显示名称，如 "Text Files"
    std::string extension;      // 扩展名，如 "*.txt" 或 "*.log;*.txt"

    FileFilter(const std::string& n, const std::string& e)
        : name(n), extension(e) {}
};

/**
 * @brief 文件对话框选项
 */
struct FileDialogOptions {
    std::string title;                      // 对话框标题
    std::vector<FileFilter> filters;        // 文件过滤器
    std::filesystem::path default_path;     // 默认路径
    bool allow_multiple = false;            // 允许多选
    bool must_exist = true;                 // 文件必须存在

    FileDialogOptions() = default;
};

/**
 * @brief 文件对话框结果
 */
struct FileDialogResult {
    bool success = false;                   // 是否成功
    std::vector<std::filesystem::path> paths;  // 选择的文件路径
    std::string error_message;              // 错误信息

    /**
     * @brief 获取第一个路径（单选时的便捷方法）
     */
    std::filesystem::path get_first_path() const {
        return paths.empty() ? std::filesystem::path() : paths[0];
    }

    /**
     * @brief 是否选择了文件
     */
    bool has_paths() const {
        return !paths.empty();
    }
};

/**
 * @brief 文件读取结果
 */
struct FileReadResult {
    bool success = false;                   // 是否成功
    std::string content;                   // 文件内容
    std::string error_message;              // 错误信息
    std::filesystem::path path;            // 文件路径
    size_t file_size = 0;                   // 文件大小（字节）

    /**
     * @brief 是否成功读取
     */
    explicit operator bool() const { return success; }
};

/**
 * @brief 文件读取进度回调
 * @param bytes_read 已读取字节数
 * @param total_bytes 总字节数
 * @param progress_percent 进度百分比 (0.0 - 1.0)
 */
using FileProgressCallback = std::function<void(size_t bytes_read, size_t total_bytes, float progress_percent)>;

/**
 * @brief 文件对话框类
 * @details 跨平台文件对话框实现，支持同步和异步操作
 */
class FileDialog {
public:
    /**
     * @brief 打开文件对话框（同步）
     * @param options 对话框选项
     * @return 对话框结果
     */
    static FileDialogResult open_file(const FileDialogOptions& options);

    /**
     * @brief 保存文件对话框（同步）
     * @param options 对话框选项
     * @return 对话框结果
     */
    static FileDialogResult save_file(const FileDialogOptions& options);

    /**
     * @brief 选择文件夹对话框（同步）
     * @param options 对话框选项
     * @return 对话框结果
     */
    static FileDialogResult select_folder(const FileDialogOptions& options);

    // ==================== 异步方法（简单回调）====================

    /**
     * @brief 打开文件对话框（异步，简单回调）
     * @param options 对话框选项
     * @param callback 完成回调（在主线程调用）
     * @note 在 Windows 上，对话框必须在主线程显示，但回调在后台线程
     */
    static void open_file_async(
        const FileDialogOptions& options,
        std::function<void(const FileDialogResult&)> callback
    );

    /**
     * @brief 保存文件对话框（异步，简单回调）
     * @param options 对话框选项
     * @param callback 完成回调
     */
    static void save_file_async(
        const FileDialogOptions& options,
        std::function<void(const FileDialogResult&)> callback
    );

    /**
     * @brief 选择文件夹对话框（异步，简单回调）
     * @param options 对话框选项
     * @param callback 完成回调
     */
    static void select_folder_async(
        const FileDialogOptions& options,
        std::function<void(const FileDialogResult&)> callback
    );

    // ==================== 与任务系统集成 ====================

    /**
     * @brief 打开并读取文件（与任务系统集成）
     * @param options 对话框选项
     * @param on_success 成功回调（在 UI 线程）
     * @param on_error 错误回调（在 UI 线程）
     * @param progress_callback 进度回调（可选，在 UI 线程）
     * @return 任务指针（可用于取消）
     * @details
     * 1. 在主线程显示文件对话框
     * 2. 在后台任务中读取文件内容
     * 3. 支持进度跟踪和取消
     * 4. 适合大文件处理
     */
    static std::shared_ptr<TasksTask> open_and_read_file(
        const FileDialogOptions& options,
        std::function<void(const FileReadResult&)> on_success,
        std::function<void(const std::string&)> on_error = nullptr,
        FileProgressCallback progress_callback = nullptr
    );

    /**
     * @brief 打开多个文件并读取（与任务系统集成）
     * @param options 对话框选项（allow_multiple = true）
     * @param on_success 成功回调（在 UI 线程）
     * @param on_error 错误回调（在 UI 线程）
     * @param progress_callback 进度回调（可选，在 UI 线程）
     * @return 任务指针
     */
    static std::shared_ptr<TasksTask> open_and_read_files(
        const FileDialogOptions& options,
        std::function<void(const std::vector<FileReadResult>&)> on_success,
        std::function<void(const std::string&)> on_error = nullptr,
        FileProgressCallback progress_callback = nullptr
    );
};

// ==================== 文件读取辅助函数 ====================

/**
 * @brief 读取文件内容（同步）
 * @param path 文件路径
 * @param max_size_mb 最大文件大小（MB），默认 100MB
 * @return 读取结果
 */
FileReadResult read_file(
    const std::filesystem::path& path,
    size_t max_size_mb = 100
);

/**
 * @brief 异步读取文件（与任务系统集成）
 * @param path 文件路径
 * @param on_success 成功回调
 * @param on_error 错误回调
 * @param progress_callback 进度回调（可选）
 * @param max_size_mb 最大文件大小（MB），默认 100MB
 * @return 任务指针
 */
std::shared_ptr<TasksTask> read_file_async(
    const std::filesystem::path& path,
    std::function<void(const FileReadResult&)> on_success,
    std::function<void(const std::string&)> on_error = nullptr,
    FileProgressCallback progress_callback = nullptr,
    size_t max_size_mb = 100
);

/**
 * @brief 读取文件行（同步，适合文本文件）
 * @param path 文件路径
 * @param max_lines 最大行数（0 = 无限制）
 * @param max_size_mb 最大文件大小（MB），默认 100MB
 * @return 读取结果
 */
FileReadResult read_file_lines(
    const std::filesystem::path& path,
    size_t max_lines = 0,
    size_t max_size_mb = 100
);

/**
 * @brief 异步读取文件行（与任务系统集成）
 * @param path 文件路径
 * @param on_success 成功回调
 * @param on_error 错误回调
 * @param progress_callback 进度回调（可选）
 * @param max_lines 最大行数（0 = 无限制）
 * @param max_size_mb 最大文件大小（MB）
 * @return 任务指针
 */
std::shared_ptr<TasksTask> read_file_lines_async(
    const std::filesystem::path& path,
    std::function<void(const FileReadResult&)> on_success,
    std::function<void(const std::string&)> on_error = nullptr,
    FileProgressCallback progress_callback = nullptr,
    size_t max_lines = 0,
    size_t max_size_mb = 100
);

// ==================== 便捷函数 ====================

/**
 * @brief 便捷函数：打开单个文件
 * @param title 对话框标题
 * @param filter 文件过滤器
 * @param default_path 默认路径
 * @return 选择的文件路径，如果取消则为空
 */
inline std::filesystem::path open_single_file(
    const std::string& title = "打开文件",
    const FileFilter& filter = FileFilter("所有文件", "*.*"),
    const std::filesystem::path& default_path = "."
) {
    FileDialogOptions options;
    options.title = title;
    options.filters.push_back(filter);
    options.default_path = default_path;
    options.allow_multiple = false;

    auto result = FileDialog::open_file(options);
    return result.get_first_path();
}

/**
 * @brief 便捷函数：打开并读取单个文件（异步）
 * @param title 对话框标题
 * @param filter 文件过滤器
 * @param on_success 成功回调
 * @param on_error 错误回调
 * @return 任务指针
 */
inline std::shared_ptr<TasksTask> open_and_read_single_file(
    const std::string& title,
    const FileFilter& filter,
    std::function<void(const FileReadResult&)> on_success,
    std::function<void(const std::string&)> on_error = nullptr
) {
    FileDialogOptions options;
    options.title = title;
    options.filters.push_back(filter);
    options.allow_multiple = false;

    return FileDialog::open_and_read_file(options, on_success, on_error);
}

/**
 * @brief 便捷函数：保存单个文件
 * @param title 对话框标题
 * @param filter 文件过滤器
 * @param default_path 默认路径
 * @return 保存的文件路径，如果取消则为空
 */
inline std::filesystem::path save_single_file(
    const std::string& title = "保存文件",
    const FileFilter& filter = FileFilter("所有文件", "*.*"),
    const std::filesystem::path& default_path = "."
) {
    FileDialogOptions options;
    options.title = title;
    options.filters.push_back(filter);
    options.default_path = default_path;

    auto result = FileDialog::save_file(options);
    return result.get_first_path();
}

/**
 * @brief 便捷函数：选择文件夹
 * @param title 对话框标题
 * @param default_path 默认路径
 * @return 选择的文件夹路径，如果取消则为空
 */
inline std::filesystem::path select_single_folder(
    const std::string& title = "选择文件夹",
    const std::filesystem::path& default_path = "."
) {
    FileDialogOptions options;
    options.title = title;
    options.default_path = default_path;

    auto result = FileDialog::select_folder(options);
    return result.get_first_path();
}

} // namespace DearTs::Core::Utils

