/**
 * @file file_dialog.cpp
 * @brief 跨平台文件对话框实现
 * @details Windows 实现使用 COM IFileOpenDialog/IFileSaveDialog
 *          与任务系统集成，支持异步文件处理
 */

#include "file_dialog.hpp"
#include "core/tasks/task_manager.h"
#include "liblogger/logger.h"
#include <windows.h>
#include <shlobj.h>
#include <comdef.h>
#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>
#include <atomic>

namespace DearTs::Core::Utils {

// ==================== Windows 实现 ====================

namespace impl {

/**
 * @brief 将过滤器转换为 Windows 格式
 * @param filters 文件过滤器列表
 * @return Windows 过滤器字符串，格式如 "Text Files\0*.txt\0All Files\0*.*\0\0"
 */
std::string convert_filters_to_windows_format(const std::vector<FileFilter>& filters) {
    std::string result;

    for (const auto& filter : filters) {
        result += filter.name;
        result += '\0';
        result += filter.extension;
        result += '\0';
    }

    // 双空终止
    result += '\0';
    result += '\0';

    return result;
}

/**
 * @brief COM 智能指针包装
 */
template<typename T>
class ComPtr {
public:
    ComPtr() : m_ptr(nullptr) {}
    ComPtr(T* ptr) : m_ptr(ptr) {}
    ~ComPtr() {
        if (m_ptr) {
            m_ptr->Release();
        }
    }

    T** operator&() { Release(); m_ptr = nullptr; return &m_ptr; }
    T* operator->() { return m_ptr; }
    operator T*() { return m_ptr; }
    T* get() { return m_ptr; }

    void Release() {
        if (m_ptr) {
            m_ptr->Release();
            m_ptr = nullptr;
        }
    }

private:
    T* m_ptr;
};

/**
 * @brief 初始化 COM
 */
class ComInitializer {
public:
    ComInitializer() {
        HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
            LOG_ERROR("Failed to initialize COM: 0x{:X}", hr);
        }
    }

    ~ComInitializer() {
        CoUninitialize();
    }
};

/**
 * @brief 获取 Shell 项路径
 */
std::filesystem::path get_item_path(IShellItem* item) {
    if (!item) return {};

    PWSTR path_ptr = nullptr;
    HRESULT hr = item->GetDisplayName(SIGDN_FILESYSPATH, &path_ptr);

    if (SUCCEEDED(hr) && path_ptr) {
        std::filesystem::path path = path_ptr;
        CoTaskMemFree(path_ptr);
        return path;
    }

    return {};
}

/**
 * @brief 打开文件对话框（Windows 实现）
 */
FileDialogResult open_file_dialog_windows(const FileDialogOptions& options) {
    FileDialogResult result;
    ComInitializer com_init;

    // 创建文件打开对话框
    ComPtr<IFileOpenDialog> dialog;
    HRESULT hr = CoCreateInstance(
        CLSID_FileOpenDialog,
        nullptr,
        CLSCTX_ALL,
        IID_IFileOpenDialog,
        reinterpret_cast<void**>(&dialog)
    );

    if (FAILED(hr)) {
        result.error_message = "Failed to create file open dialog";
        LOG_ERROR("Failed to create FileOpenDialog: 0x{:X}", hr);
        return result;
    }

    // 设置标题
    if (!options.title.empty()) {
        std::wstring wtitle(options.title.begin(), options.title.end());
        dialog->SetTitle(wtitle.c_str());
    }

    // 设置默认路径
    if (!options.default_path.empty()) {
        ComPtr<IShellItem> default_item;
        std::wstring wpath(options.default_path.wstring());

        // 尝试从路径创建 Shell Item
        hr = SHCreateItemFromParsingName(
            wpath.c_str(),
            nullptr,
            IID_IShellItem,
            reinterpret_cast<void**>(&default_item)
        );

        if (SUCCEEDED(hr)) {
            dialog->SetFolder(default_item.get());
        }
    }

    // 设置文件过滤器
    if (!options.filters.empty()) {
        std::vector<COMDLG_FILTERSPEC> specs;
        std::vector<std::wstring> name_storage;
        std::vector<std::wstring> ext_storage;

        for (const auto& filter : options.filters) {
            name_storage.push_back(std::wstring(filter.name.begin(), filter.name.end()));
            ext_storage.push_back(std::wstring(filter.extension.begin(), filter.extension.end()));

            specs.push_back({ name_storage.back().c_str(), ext_storage.back().c_str() });
        }

        dialog->SetFileTypes(static_cast<UINT>(specs.size()), specs.data());
    }

    // 设置多选
    if (options.allow_multiple) {
        DWORD opts = 0;
        dialog->GetOptions(&opts);
        dialog->SetOptions(opts | FOS_ALLOWMULTISELECT);
    }

    // 设置文件必须存在
    if (options.must_exist) {
        DWORD opts = 0;
        dialog->GetOptions(&opts);
        dialog->SetOptions(opts | FOS_FILEMUSTEXIST);
    }

    // 显示对话框
    hr = dialog->Show(nullptr);

    if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
        // 用户取消
        result.success = false;
        return result;
    }

    if (FAILED(hr)) {
        result.error_message = "Failed to show dialog";
        LOG_ERROR("FileOpenDialog::Show failed: 0x{:X}", hr);
        return result;
    }

    // 获取结果
    if (options.allow_multiple) {
        // 多选
        ComPtr<IShellItemArray> items;
        hr = dialog->GetResults(&items);

        if (SUCCEEDED(hr)) {
            DWORD count = 0;
            items->GetCount(&count);

            for (DWORD i = 0; i < count; ++i) {
                ComPtr<IShellItem> item;
                hr = items->GetItemAt(i, &item);

                if (SUCCEEDED(hr)) {
                    std::filesystem::path path = get_item_path(item.get());
                    if (!path.empty()) {
                        result.paths.push_back(path);
                    }
                }
            }
        }
    } else {
        // 单选
        ComPtr<IShellItem> item;
        hr = dialog->GetResult(&item);

        if (SUCCEEDED(hr)) {
            std::filesystem::path path = get_item_path(item.get());
            if (!path.empty()) {
                result.paths.push_back(path);
            }
        }
    }

    result.success = !result.paths.empty();
    return result;
}

/**
 * @brief 保存文件对话框（Windows 实现）
 */
FileDialogResult save_file_dialog_windows(const FileDialogOptions& options) {
    FileDialogResult result;
    ComInitializer com_init;

    // 创建文件保存对话框
    ComPtr<IFileSaveDialog> dialog;
    HRESULT hr = CoCreateInstance(
        CLSID_FileSaveDialog,
        nullptr,
        CLSCTX_ALL,
        IID_IFileSaveDialog,
        reinterpret_cast<void**>(&dialog)
    );

    if (FAILED(hr)) {
        result.error_message = "Failed to create file save dialog";
        LOG_ERROR("Failed to create FileSaveDialog: 0x{:X}", hr);
        return result;
    }

    // 设置标题
    if (!options.title.empty()) {
        std::wstring wtitle(options.title.begin(), options.title.end());
        dialog->SetTitle(wtitle.c_str());
    }

    // 设置默认文件名
    if (!options.default_path.empty() && options.default_path.has_filename()) {
        std::wstring wfilename = options.default_path.filename().wstring();
        dialog->SetFileName(wfilename.c_str());
    }

    // 设置默认路径
    if (!options.default_path.empty() && options.default_path.has_parent_path()) {
        ComPtr<IShellItem> default_item;
        std::wstring wpath = options.default_path.parent_path().wstring();

        hr = SHCreateItemFromParsingName(
            wpath.c_str(),
            nullptr,
            IID_IShellItem,
            reinterpret_cast<void**>(&default_item)
        );

        if (SUCCEEDED(hr)) {
            dialog->SetFolder(default_item.get());
        }
    }

    // 设置文件过滤器
    if (!options.filters.empty()) {
        std::vector<COMDLG_FILTERSPEC> specs;
        std::vector<std::wstring> name_storage;
        std::vector<std::wstring> ext_storage;

        for (const auto& filter : options.filters) {
            name_storage.push_back(std::wstring(filter.name.begin(), filter.name.end()));
            ext_storage.push_back(std::wstring(filter.extension.begin(), filter.extension.end()));

            specs.push_back({ name_storage.back().c_str(), ext_storage.back().c_str() });
        }

        dialog->SetFileTypes(static_cast<UINT>(specs.size()), specs.data());

        // 设置默认过滤器
        if (!specs.empty()) {
            dialog->SetFileTypeIndex(1);
        }
    }

    // 显示对话框
    hr = dialog->Show(nullptr);

    if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
        // 用户取消
        result.success = false;
        return result;
    }

    if (FAILED(hr)) {
        result.error_message = "Failed to show dialog";
        LOG_ERROR("FileSaveDialog::Show failed: 0x{:X}", hr);
        return result;
    }

    // 获取结果
    ComPtr<IShellItem> item;
    hr = dialog->GetResult(&item);

    if (SUCCEEDED(hr)) {
        std::filesystem::path path = get_item_path(item.get());
        if (!path.empty()) {
            result.paths.push_back(path);
            result.success = true;
        }
    }

    return result;
}

/**
 * @brief 选择文件夹对话框（Windows 实现）
 */
FileDialogResult select_folder_dialog_windows(const FileDialogOptions& options) {
    FileDialogResult result;
    ComInitializer com_init;

    // 创建文件夹浏览对话框
    ComPtr<IFileOpenDialog> dialog;
    HRESULT hr = CoCreateInstance(
        CLSID_FileOpenDialog,
        nullptr,
        CLSCTX_ALL,
        IID_IFileOpenDialog,
        reinterpret_cast<void**>(&dialog)
    );

    if (FAILED(hr)) {
        result.error_message = "Failed to create folder dialog";
        LOG_ERROR("Failed to create FileOpenDialog for folder: 0x{:X}", hr);
        return result;
    }

    // 设置为选择文件夹模式
    DWORD opts = 0;
    dialog->GetOptions(&opts);
    dialog->SetOptions(opts | FOS_PICKFOLDERS);

    // 设置标题
    if (!options.title.empty()) {
        std::wstring wtitle(options.title.begin(), options.title.end());
        dialog->SetTitle(wtitle.c_str());
    }

    // 设置默认路径
    if (!options.default_path.empty()) {
        ComPtr<IShellItem> default_item;
        std::wstring wpath(options.default_path.wstring());

        hr = SHCreateItemFromParsingName(
            wpath.c_str(),
            nullptr,
            IID_IShellItem,
            reinterpret_cast<void**>(&default_item)
        );

        if (SUCCEEDED(hr)) {
            dialog->SetFolder(default_item.get());
        }
    }

    // 显示对话框
    hr = dialog->Show(nullptr);

    if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
        // 用户取消
        result.success = false;
        return result;
    }

    if (FAILED(hr)) {
        result.error_message = "Failed to show dialog";
        LOG_ERROR("Folder dialog::Show failed: 0x{:X}", hr);
        return result;
    }

    // 获取结果
    ComPtr<IShellItem> item;
    hr = dialog->GetResult(&item);

    if (SUCCEEDED(hr)) {
        std::filesystem::path path = get_item_path(item.get());
        if (!path.empty()) {
            result.paths.push_back(path);
            result.success = true;
        }
    }

    return result;
}

} // namespace impl

// ==================== 公共接口 ====================

FileDialogResult FileDialog::open_file(const FileDialogOptions& options) {
    return impl::open_file_dialog_windows(options);
}

FileDialogResult FileDialog::save_file(const FileDialogOptions& options) {
    return impl::save_file_dialog_windows(options);
}

FileDialogResult FileDialog::select_folder(const FileDialogOptions& options) {
    return impl::select_folder_dialog_windows(options);
}

void FileDialog::open_file_async(
    const FileDialogOptions& options,
    std::function<void(const FileDialogResult&)> callback
) {
    // 在后台线程执行，避免阻塞 UI
    std::thread([options, callback]() {
        auto result = impl::open_file_dialog_windows(options);
        callback(result);
    }).detach();
}

void FileDialog::save_file_async(
    const FileDialogOptions& options,
    std::function<void(const FileDialogResult&)> callback
) {
    // 在后台线程执行，避免阻塞 UI
    std::thread([options, callback]() {
        auto result = impl::save_file_dialog_windows(options);
        callback(result);
    }).detach();
}

void FileDialog::select_folder_async(
    const FileDialogOptions& options,
    std::function<void(const FileDialogResult&)> callback
) {
    // 在后台线程执行，避免阻塞 UI
    std::thread([options, callback]() {
        auto result = impl::select_folder_dialog_windows(options);
        callback(result);
    }).detach();
}

// ==================== 文件读取函数 ====================

FileReadResult read_file(const std::filesystem::path& path, size_t max_size_mb) {
    FileReadResult result;
    result.path = path;

    try {
        // 检查文件是否存在
        if (!std::filesystem::exists(path)) {
            result.error_message = "文件不存在: " + path.string();
            LOG_ERROR("File not found: {}", path.string());
            return result;
        }

        // 获取文件大小
        result.file_size = std::filesystem::file_size(path);
        size_t max_size_bytes = max_size_mb * 1024 * 1024;

        // 检查文件大小
        if (result.file_size > max_size_bytes) {
            result.error_message = std::format("文件过大 ({} MB)，超过限制 ({} MB)",
                result.file_size / (1024.0 * 1024.0), max_size_mb);
            LOG_ERROR("File too large: {}", result.error_message);
            return result;
        }

        // 读取文件
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            result.error_message = "无法打开文件: " + path.string();
            LOG_ERROR("Failed to open file: {}", path.string());
            return result;
        }

        // 分配内存
        std::string content;
        content.resize(result.file_size);

        // 读取内容
        file.read(content.data(), result.file_size);
        if (!file) {
            result.error_message = "读取文件失败: " + path.string();
            LOG_ERROR("Failed to read file: {}", path.string());
            return result;
        }

        result.content = std::move(content);
        result.success = true;

        LOG_INFO("Successfully read file: {} ({} bytes)", path.string(), result.file_size);

    } catch (const std::exception& e) {
        result.error_message = std::format("读取文件异常: {}", e.what());
        LOG_ERROR("Exception reading file: {}", e.what());
    }

    return result;
}

std::shared_ptr<DearTs::Core::Tasks::Task> read_file_async(
    const std::filesystem::path& path,
    std::function<void(const FileReadResult&)> on_success,
    std::function<void(const std::string&)> on_error,
    FileProgressCallback progress_callback,
    size_t max_size_mb
) {
    // 创建异步读取任务
    auto task = DearTs::Core::Tasks::TaskManager::instance().launch(
        "读取文件: " + path.filename().string(),
        [path, on_success, on_error, progress_callback, max_size_mb]
        (const std::atomic<bool>& should_cancel) mutable {
            FileReadResult result;
            result.path = path;

            try {
                // 检查文件是否存在
                if (!std::filesystem::exists(path)) {
                    result.error_message = "文件不存在: " + path.string();
                    if (on_error) on_error(result.error_message);
                    LOG_ERROR("File not found: {}", path.string());
                    return;
                }

                // 获取文件大小
                result.file_size = std::filesystem::file_size(path);
                size_t max_size_bytes = max_size_mb * 1024 * 1024;

                // 检查文件大小
                if (result.file_size > max_size_bytes) {
                    result.error_message = std::format("文件过大 ({} MB)，超过限制 ({} MB)",
                        result.file_size / (1024.0 * 1024.0), max_size_mb);
                    if (on_error) on_error(result.error_message);
                    LOG_ERROR("File too large: {}", result.error_message);
                    return;
                }

                // 检查取消
                if (should_cancel) {
                    LOG_INFO("File reading cancelled");
                    return;
                }

                // 分块读取文件（支持进度跟踪）
                std::ifstream file(path, std::ios::binary);
                if (!file.is_open()) {
                    result.error_message = "无法打开文件: " + path.string();
                    if (on_error) on_error(result.error_message);
                    LOG_ERROR("Failed to open file: {}", path.string());
                    return;
                }

                // 分配内存
                result.content.resize(result.file_size);

                // 分块读取（每块 4MB）
                const size_t chunk_size = 4 * 1024 * 1024;
                size_t bytes_read = 0;

                while (bytes_read < result.file_size && !should_cancel) {
                    size_t bytes_to_read = std::min(chunk_size, result.file_size - bytes_read);
                    file.read(result.content.data() + bytes_read, bytes_to_read);

                    if (!file) {
                        result.error_message = "读取文件失败: " + path.string();
                        if (on_error) on_error(result.error_message);
                        LOG_ERROR("Failed to read file chunk");
                        return;
                    }

                    bytes_read += bytes_to_read;

                    // 报告进度
                    if (progress_callback) {
                        float progress = static_cast<float>(bytes_read) / result.file_size;
                        progress_callback(bytes_read, result.file_size, progress);
                    }
                }

                // 检查是否被取消
                if (should_cancel) {
                    LOG_INFO("File reading cancelled");
                    return;
                }

                result.success = true;
                LOG_INFO("Successfully read file: {} ({} bytes)", path.string(), bytes_read);

                // 调用成功回调
                if (on_success) {
                    on_success(result);
                }

            } catch (const std::exception& e) {
                result.error_message = std::format("读取文件异常: {}", e.what());
                if (on_error) on_error(result.error_message);
                LOG_ERROR("Exception reading file: {}", e.what());
            }
        },
        DearTs::Core::Tasks::TaskType::Background  // 后台任务，不影响 UI
    );

    return task;
}

FileReadResult read_file_lines(const std::filesystem::path& path, size_t max_lines, size_t max_size_mb) {
    FileReadResult result;
    result.path = path;

    try {
        // 检查文件是否存在
        if (!std::filesystem::exists(path)) {
            result.error_message = "文件不存在: " + path.string();
            LOG_ERROR("File not found: {}", path.string());
            return result;
        }

        // 获取文件大小
        result.file_size = std::filesystem::file_size(path);
        size_t max_size_bytes = max_size_mb * 1024 * 1024;

        // 检查文件大小
        if (result.file_size > max_size_bytes) {
            result.error_message = std::format("文件过大 ({} MB)，超过限制 ({} MB)",
                result.file_size / (1024.0 * 1024.0), max_size_mb);
            LOG_ERROR("File too large: {}", result.error_message);
            return result;
        }

        // 读取文件行
        std::ifstream file(path);
        if (!file.is_open()) {
            result.error_message = "无法打开文件: " + path.string();
            LOG_ERROR("Failed to open file: {}", path.string());
            return result;
        }

        std::ostringstream content;
        std::string line;
        size_t line_count = 0;

        while (std::getline(file, line)) {
            // 检查行数限制
            if (max_lines > 0 && line_count >= max_lines) {
                LOG_WARN("Reached maximum line limit ({})", max_lines);
                break;
            }

            content << line << '\n';
            line_count++;
        }

        result.content = content.str();
        result.success = true;

        LOG_INFO("Successfully read {} lines from file: {}", line_count, path.string());

    } catch (const std::exception& e) {
        result.error_message = std::format("读取文件异常: {}", e.what());
        LOG_ERROR("Exception reading file: {}", e.what());
    }

    return result;
}

std::shared_ptr<DearTs::Core::Tasks::Task> read_file_lines_async(
    const std::filesystem::path& path,
    std::function<void(const FileReadResult&)> on_success,
    std::function<void(const std::string&)> on_error,
    FileProgressCallback progress_callback,
    size_t max_lines,
    size_t max_size_mb
) {
    // 创建异步读取任务
    auto task = DearTs::Core::Tasks::TaskManager::instance().launch(
        "读取文件行: " + path.filename().string(),
        [path, on_success, on_error, progress_callback, max_lines, max_size_mb]
        (const std::atomic<bool>& should_cancel) mutable {
            FileReadResult result;
            result.path = path;

            try {
                // 检查文件是否存在
                if (!std::filesystem::exists(path)) {
                    result.error_message = "文件不存在: " + path.string();
                    if (on_error) on_error(result.error_message);
                    LOG_ERROR("File not found: {}", path.string());
                    return;
                }

                // 获取文件大小
                result.file_size = std::filesystem::file_size(path);
                size_t max_size_bytes = max_size_mb * 1024 * 1024;

                // 检查文件大小
                if (result.file_size > max_size_bytes) {
                    result.error_message = std::format("文件过大 ({} MB)，超过限制 ({} MB)",
                        result.file_size / (1024.0 * 1024.0), max_size_mb);
                    if (on_error) on_error(result.error_message);
                    LOG_ERROR("File too large: {}", result.error_message);
                    return;
                }

                // 读取文件行
                std::ifstream file(path);
                if (!file.is_open()) {
                    result.error_message = "无法打开文件: " + path.string();
                    if (on_error) on_error(result.error_message);
                    LOG_ERROR("Failed to open file: {}", path.string());
                    return;
                }

                std::ostringstream content;
                std::string line;
                size_t line_count = 0;
                size_t total_lines_estimated = result.file_size / 80;  // 估算总行数

                while (std::getline(file, line) && !should_cancel) {
                    // 检查行数限制
                    if (max_lines > 0 && line_count >= max_lines) {
                        LOG_WARN("Reached maximum line limit ({})", max_lines);
                        break;
                    }

                    content << line << '\n';
                    line_count++;

                    // 每隔一定行数报告进度
                    if (progress_callback && line_count % 1000 == 0) {
                        float progress = std::min(1.0f, static_cast<float>(line_count) / total_lines_estimated);
                        progress_callback(line_count, total_lines_estimated, progress);
                    }
                }

                // 检查是否被取消
                if (should_cancel) {
                    LOG_INFO("File reading cancelled");
                    return;
                }

                result.content = content.str();
                result.success = true;

                LOG_INFO("Successfully read {} lines from file: {}", line_count, path.string());

                // 调用成功回调
                if (on_success) {
                    on_success(result);
                }

            } catch (const std::exception& e) {
                result.error_message = std::format("读取文件异常: {}", e.what());
                if (on_error) on_error(result.error_message);
                LOG_ERROR("Exception reading file: {}", e.what());
            }
        },
        DearTs::Core::Tasks::TaskType::Background
    );

    return task;
}

// ==================== 与任务系统集成 ====================

std::shared_ptr<DearTs::Core::Tasks::Task> FileDialog::open_and_read_file(
    const FileDialogOptions& options,
    std::function<void(const FileReadResult&)> on_success,
    std::function<void(const std::string&)> on_error,
    FileProgressCallback progress_callback
) {
    // 在主线程显示对话框
    auto dialog_result = impl::open_file_dialog_windows(options);

    if (!dialog_result.success || !dialog_result.has_paths()) {
        // 用户取消或失败
        if (on_error && !dialog_result.error_message.empty()) {
            on_error(dialog_result.error_message);
        }
        return nullptr;
    }

    // 获取选择的文件
    auto path = dialog_result.get_first_path();

    // 创建异步读取任务
    return read_file_async(path, on_success, on_error, progress_callback);
}

std::shared_ptr<DearTs::Core::Tasks::Task> FileDialog::open_and_read_files(
    const FileDialogOptions& options,
    std::function<void(const std::vector<FileReadResult>&)> on_success,
    std::function<void(const std::string&)> on_error,
    FileProgressCallback progress_callback
) {
    // 在主线程显示对话框
    auto dialog_result = impl::open_file_dialog_windows(options);

    if (!dialog_result.success || !dialog_result.has_paths()) {
        // 用户取消或失败
        if (on_error && !dialog_result.error_message.empty()) {
            on_error(dialog_result.error_message);
        }
        return nullptr;
    }

    // 获取选择的文件
    const auto& paths = dialog_result.paths;

    // 创建批量读取任务
    auto task = DearTs::Core::Tasks::TaskManager::instance().launch(
        "读取多个文件",
        [paths, on_success, on_error, progress_callback]
        (const std::atomic<bool>& should_cancel) mutable {
            std::vector<FileReadResult> results;
            results.reserve(paths.size());

            size_t total_files = paths.size();
            for (size_t i = 0; i < total_files && !should_cancel; ++i) {
                const auto& path = paths[i];

                // 读取单个文件
                auto result = read_file(path);

                if (result.success) {
                    results.push_back(std::move(result));
                } else {
                    // 任何一个文件失败，整体失败
                    if (on_error) {
                        on_error(result.error_message);
                    }
                    return;
                }

                // 报告进度
                if (progress_callback) {
                    float progress = static_cast<float>(i + 1) / total_files;
                    progress_callback(i + 1, total_files, progress);
                }
            }

            // 检查是否被取消
            if (should_cancel) {
                LOG_INFO("Multi-file reading cancelled");
                return;
            }

            // 调用成功回调
            if (on_success) {
                on_success(results);
            }

            LOG_INFO("Successfully read {} files", results.size());
        },
        DearTs::Core::Tasks::TaskType::Background
    );

    return task;
}

} // namespace DearTs::Core::Utils
