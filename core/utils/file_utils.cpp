/**
 * DearTs File System Utilities Implementation
 * 
 * 文件系统工具类实现 - 提供跨平台文件操作功能
 * 
 * @author DearTs Team
 * @version 1.0.0
 * @date 2025
 */

#include "file_utils.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <random>
#include <cstring>
#include <cstdlib>
#include <sys/stat.h>

#ifdef _WIN32
    #include <windows.h>
    #include <shlobj.h>
    #include <io.h>
    #include <direct.h>
    #define PATH_SEPARATOR '\\'
    #define ALT_PATH_SEPARATOR '/'
#else
    #include <unistd.h>
    #include <dirent.h>
    #include <fcntl.h>
    #include <sys/types.h>
    #include <sys/statvfs.h>
    #include <pwd.h>
    #define PATH_SEPARATOR '/'
    #define ALT_PATH_SEPARATOR '\\'
#endif

namespace DearTs {
namespace Core {
namespace Utils {

// ============================================================================
// FileInfo Implementation
// ============================================================================

/**
 * @brief FileInfo默认构造函数
 */
FileInfo::FileInfo() 
    : type(FileType::UNKNOWN)
    , size(0)
    , permissions(0)
    , is_hidden(false)
    , is_readonly(false) {
}

/**
 * @brief 从路径构造FileInfo
 * @param file_path 文件路径
 */
FileInfo::FileInfo(const std::string& file_path) : FileInfo() {
    path = file_path;
    name = FileUtils::getFileName(file_path);
    extension = FileUtils::getFileExtension(file_path);
    
    if (FileUtils::exists(file_path)) {
        if (FileUtils::isDirectory(file_path)) {
            type = FileType::DIRECTORY;
        } else if (FileUtils::isFile(file_path)) {
            type = FileType::REGULAR_FILE;
            size = FileUtils::getFileSize(file_path);
        } else if (FileUtils::isSymlink(file_path)) {
            type = FileType::SYMLINK;
        }
        
        permissions = FileUtils::getPermissions(file_path);
        
#ifdef _WIN32
        DWORD attrs = GetFileAttributesA(file_path.c_str());
        if (attrs != INVALID_FILE_ATTRIBUTES) {
            is_hidden = (attrs & FILE_ATTRIBUTE_HIDDEN) != 0;
            is_readonly = (attrs & FILE_ATTRIBUTE_READONLY) != 0;
            
            HANDLE hFile = CreateFileA(file_path.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                     nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (hFile != INVALID_HANDLE_VALUE) {
                FILETIME ftCreate, ftAccess, ftWrite;
                if (GetFileTime(hFile, &ftCreate, &ftAccess, &ftWrite)) {
                    auto convertTime = [](const FILETIME& ft) {
                        ULARGE_INTEGER ull;
                        ull.LowPart = ft.dwLowDateTime;
                        ull.HighPart = ft.dwHighDateTime;
                        return std::chrono::system_clock::from_time_t(
                            (ull.QuadPart - 116444736000000000ULL) / 10000000ULL);
                    };
                    
                    created_time = convertTime(ftCreate);
                    accessed_time = convertTime(ftAccess);
                    modified_time = convertTime(ftWrite);
                }
                CloseHandle(hFile);
            }
        }
#else
        struct stat st;
        if (stat(file_path.c_str(), &st) == 0) {
            created_time = std::chrono::system_clock::from_time_t(st.st_ctime);
            accessed_time = std::chrono::system_clock::from_time_t(st.st_atime);
            modified_time = std::chrono::system_clock::from_time_t(st.st_mtime);
            
            is_hidden = name.length() > 0 && name[0] == '.';
            is_readonly = (st.st_mode & S_IWUSR) == 0;
        }
#endif
    }
}

/**
 * @brief 获取可读的文件大小字符串
 * @return 文件大小字符串
 */
std::string FileInfo::getReadableSize() const {
    return FileUtils::getReadableSize(size);
}

/**
 * @brief 检查权限
 * @param permission 权限标志
 * @return 是否具有权限
 */
bool FileInfo::hasPermission(FilePermission permission) const {
    return (permissions & static_cast<uint32_t>(permission)) != 0;
}

// ============================================================================
// FileWatcher Implementation
// ============================================================================

/**
 * @brief FileWatcher构造函数
 */
FileWatcher::FileWatcher() : running_(false) {
}

/**
 * @brief FileWatcher析构函数
 */
FileWatcher::~FileWatcher() {
    stop();
}

/**
 * @brief 添加监控路径
 * @param path 监控路径
 * @param callback 回调函数
 * @param recursive 是否递归监控
 * @return 是否成功
 */
bool FileWatcher::addWatch(const std::string& path, FileWatchCallback callback, bool recursive) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!FileUtils::exists(path)) {
        // FileWatcher: Path does not exist - removed logging
        return false;
    }
    
    WatchEntry entry;
    entry.path = path;
    entry.callback = callback;
    entry.recursive = recursive;
    entry.platform_handle = nullptr;
    
#ifdef _WIN32
    DWORD flags = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME |
                  FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE |
                  FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION;
    
    HANDLE hDir = CreateFileA(path.c_str(), FILE_LIST_DIRECTORY,
                             FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                             nullptr, OPEN_EXISTING,
                             FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
                             nullptr);
    
    if (hDir == INVALID_HANDLE_VALUE) {
        // FileWatcher: Failed to open directory - removed logging
        return false;
    }
    
    entry.platform_handle = hDir;
#else
    // Linux inotify implementation would go here
    // For now, we'll use a simple polling mechanism
    entry.platform_handle = nullptr;
#endif
    
    watches_[path] = entry;
    // FileWatcher: Added watch for path - removed logging
    return true;
}

/**
 * @brief 移除监控路径
 * @param path 监控路径
 * @return 是否成功
 */
bool FileWatcher::removeWatch(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = watches_.find(path);
    if (it == watches_.end()) {
        return false;
    }
    
#ifdef _WIN32
    if (it->second.platform_handle) {
        CloseHandle(static_cast<HANDLE>(it->second.platform_handle));
    }
#endif
    
    watches_.erase(it);
    // FileWatcher: Removed watch for path - removed logging
    return true;
}

/**
 * @brief 启动监控
 * @return 是否成功
 */
bool FileWatcher::start() {
    if (running_) {
        return true;
    }
    
    running_ = true;
    watch_thread_ = std::thread(&FileWatcher::watchThread, this);
    // FileWatcher: Started monitoring - removed logging
    return true;
}

/**
 * @brief 停止监控
 */
void FileWatcher::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    
    if (watch_thread_.joinable()) {
        watch_thread_.join();
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& [path, entry] : watches_) {
#ifdef _WIN32
        if (entry.platform_handle) {
            CloseHandle(static_cast<HANDLE>(entry.platform_handle));
        }
#endif
    }
    watches_.clear();
    
    // FileWatcher: Stopped monitoring - removed logging
}

/**
 * @brief 获取监控路径列表
 * @return 路径列表
 */
std::vector<std::string> FileWatcher::getWatchedPaths() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> paths;
    
    for (const auto& [path, entry] : watches_) {
        paths.push_back(path);
    }
    
    return paths;
}

/**
 * @brief 监控线程函数
 */
void FileWatcher::watchThread() {
    while (running_) {
        // Simple polling implementation
        // In a real implementation, you would use platform-specific APIs
        // like inotify on Linux or ReadDirectoryChangesW on Windows
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Check for changes (simplified implementation)
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& [path, entry] : watches_) {
            // This is a placeholder - real implementation would detect actual changes
            // handleFileEvent(path, FileWatchEvent::MODIFIED);
        }
    }
}

/**
 * @brief 处理文件事件
 * @param path 文件路径
 * @param event 事件类型
 */
void FileWatcher::handleFileEvent(const std::string& path, FileWatchEvent event) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (const auto& [watch_path, entry] : watches_) {
        if (path.find(watch_path) == 0) {
            if (entry.callback) {
                entry.callback(path, event);
            }
        }
    }
}

// ============================================================================
// TempFileManager Implementation
// ============================================================================

/**
 * @brief TempFileManager构造函数
 */
TempFileManager::TempFileManager() {
    temp_directory_ = getSystemTempDirectory();
}

/**
 * @brief TempFileManager析构函数
 */
TempFileManager::~TempFileManager() {
    cleanupAll();
}

/**
 * @brief 获取单例实例
 * @return 管理器实例
 */
TempFileManager& TempFileManager::getInstance() {
    static TempFileManager instance;
    return instance;
}

/**
 * @brief 创建临时文件
 * @param prefix 文件名前缀
 * @param suffix 文件名后缀
 * @return 临时文件路径
 */
std::string TempFileManager::createTempFile(const std::string& prefix, const std::string& suffix) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string filename = generateUniqueFileName(prefix, suffix);
    std::string filepath = FileUtils::joinPath(temp_directory_, filename);
    
    // Create the file
    std::ofstream file(filepath);
    if (!file.is_open()) {
        // TempFileManager: Failed to create temp file - removed logging
        return "";
    }
    file.close();
    
    temp_files_.push_back(filepath);
    // TempFileManager: Created temp file - removed logging
    return filepath;
}

/**
 * @brief 创建临时目录
 * @param prefix 目录名前缀
 * @return 临时目录路径
 */
std::string TempFileManager::createTempDirectory(const std::string& prefix) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string dirname = generateUniqueFileName(prefix, "");
    std::string dirpath = FileUtils::joinPath(temp_directory_, dirname);
    
    if (!FileUtils::createDirectory(dirpath)) {
        // TempFileManager: Failed to create temp directory - removed logging
        return "";
    }
    
    temp_files_.push_back(dirpath);
    // TempFileManager: Created temp directory - removed logging
    return dirpath;
}

/**
 * @brief 注册临时文件
 * @param path 文件路径
 */
void TempFileManager::registerTempFile(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    temp_files_.push_back(path);
}

/**
 * @brief 注销临时文件
 * @param path 文件路径
 */
void TempFileManager::unregisterTempFile(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::find(temp_files_.begin(), temp_files_.end(), path);
    if (it != temp_files_.end()) {
        temp_files_.erase(it);
    }
}

/**
 * @brief 清理所有临时文件
 */
void TempFileManager::cleanupAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (const auto& path : temp_files_) {
        if (FileUtils::exists(path)) {
            FileUtils::remove(path, true);
            // TempFileManager: Cleaned up - removed logging
        }
    }
    
    temp_files_.clear();
}

/**
 * @brief 清理过期的临时文件
 * @param max_age_hours 最大存在时间(小时)
 */
void TempFileManager::cleanupExpired(int max_age_hours) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto now = std::chrono::system_clock::now();
    auto max_age = std::chrono::hours(max_age_hours);
    
    auto it = temp_files_.begin();
    while (it != temp_files_.end()) {
        if (FileUtils::exists(*it)) {
            FileInfo info(*it);
            if (now - info.modified_time > max_age) {
                FileUtils::remove(*it, true);
                // TempFileManager: Cleaned up expired - removed logging
                it = temp_files_.erase(it);
            } else {
                ++it;
            }
        } else {
            it = temp_files_.erase(it);
        }
    }
}

/**
 * @brief 设置临时目录
 * @param temp_dir 临时目录路径
 */
void TempFileManager::setTempDirectory(const std::string& temp_dir) {
    std::lock_guard<std::mutex> lock(mutex_);
    temp_directory_ = temp_dir;
    
    if (!FileUtils::exists(temp_directory_)) {
        FileUtils::createDirectory(temp_directory_, true);
    }
}

/**
 * @brief 获取临时目录
 * @return 临时目录路径
 */
std::string TempFileManager::getTempDirectory() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return temp_directory_;
}

/**
 * @brief 获取系统临时目录
 * @return 系统临时目录路径
 */
std::string TempFileManager::getSystemTempDirectory() {
#ifdef _WIN32
    char temp_path[MAX_PATH];
    DWORD result = GetTempPathA(MAX_PATH, temp_path);
    if (result > 0 && result < MAX_PATH) {
        return std::string(temp_path);
    }
    return "C:\\temp";
#else
    const char* temp_dir = std::getenv("TMPDIR");
    if (!temp_dir) temp_dir = std::getenv("TMP");
    if (!temp_dir) temp_dir = std::getenv("TEMP");
    if (!temp_dir) temp_dir = "/tmp";
    return std::string(temp_dir);
#endif
}

/**
 * @brief 生成唯一文件名
 * @param prefix 前缀
 * @param suffix 后缀
 * @return 唯一文件名
 */
std::string TempFileManager::generateUniqueFileName(const std::string& prefix, const std::string& suffix) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    ss << prefix << "_";
    
    // Add timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    ss << std::hex << time_t << "_";
    
    // Add random component
    for (int i = 0; i < 8; ++i) {
        ss << std::hex << dis(gen);
    }
    
    ss << suffix;
    return ss.str();
}

// ============================================================================
// FileLock Implementation
// ============================================================================

/**
 * @brief FileLock构造函数
 * @param file_path 文件路径
 */
FileLock::FileLock(const std::string& file_path)
    : file_path_(file_path)
    , platform_handle_(nullptr)
    , locked_(false)
    , shared_lock_(false) {
}

/**
 * @brief FileLock析构函数
 */
FileLock::~FileLock() {
    unlock();
}

/**
 * @brief 获取独占锁
 * @param timeout_ms 超时时间(毫秒)
 * @return 是否成功
 */
bool FileLock::lock(int timeout_ms) {
    if (locked_) {
        return true;
    }
    
#ifdef _WIN32
    HANDLE hFile = CreateFileA(file_path_.c_str(),
                              GENERIC_READ | GENERIC_WRITE,
                              0, // No sharing
                              nullptr,
                              OPEN_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL,
                              nullptr);
    
    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    platform_handle_ = hFile;
    locked_ = true;
    shared_lock_ = false;
    return true;
#else
    int fd = open(file_path_.c_str(), O_CREAT | O_RDWR, 0644);
    if (fd == -1) {
        return false;
    }
    
    struct flock fl;
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;
    
    if (fcntl(fd, F_SETLK, &fl) == -1) {
        close(fd);
        return false;
    }
    
    platform_handle_ = reinterpret_cast<void*>(static_cast<intptr_t>(fd));
    locked_ = true;
    shared_lock_ = false;
    return true;
#endif
}

/**
 * @brief 尝试获取独占锁(非阻塞)
 * @return 是否成功
 */
bool FileLock::tryLock() {
    return lock(0);
}

/**
 * @brief 获取共享锁
 * @param timeout_ms 超时时间(毫秒)
 * @return 是否成功
 */
bool FileLock::lockShared(int timeout_ms) {
    if (locked_) {
        return shared_lock_;
    }
    
#ifdef _WIN32
    HANDLE hFile = CreateFileA(file_path_.c_str(),
                              GENERIC_READ,
                              FILE_SHARE_READ,
                              nullptr,
                              OPEN_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL,
                              nullptr);
    
    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    platform_handle_ = hFile;
    locked_ = true;
    shared_lock_ = true;
    return true;
#else
    int fd = open(file_path_.c_str(), O_CREAT | O_RDONLY, 0644);
    if (fd == -1) {
        return false;
    }
    
    struct flock fl;
    fl.l_type = F_RDLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;
    
    if (fcntl(fd, F_SETLK, &fl) == -1) {
        close(fd);
        return false;
    }
    
    platform_handle_ = reinterpret_cast<void*>(static_cast<intptr_t>(fd));
    locked_ = true;
    shared_lock_ = true;
    return true;
#endif
}

/**
 * @brief 尝试获取共享锁(非阻塞)
 * @return 是否成功
 */
bool FileLock::tryLockShared() {
    return lockShared(0);
}

/**
 * @brief 释放锁
 */
void FileLock::unlock() {
    if (!locked_) {
        return;
    }
    
#ifdef _WIN32
    if (platform_handle_) {
        CloseHandle(static_cast<HANDLE>(platform_handle_));
        platform_handle_ = nullptr;
    }
#else
    if (platform_handle_) {
        int fd = static_cast<int>(reinterpret_cast<intptr_t>(platform_handle_));
        close(fd);
        platform_handle_ = nullptr;
    }
#endif
    
    locked_ = false;
    shared_lock_ = false;
}

// ============================================================================
// FileUtils Implementation
// ============================================================================

/**
 * @brief 检查文件是否存在
 * @param path 文件路径
 * @return 是否存在
 */
bool FileUtils::exists(const std::string& path) {
    return std::filesystem::exists(path);
}

/**
 * @brief 检查是否为文件
 * @param path 路径
 * @return 是否为文件
 */
bool FileUtils::isFile(const std::string& path) {
    return std::filesystem::is_regular_file(path);
}

/**
 * @brief 检查是否为目录
 * @param path 路径
 * @return 是否为目录
 */
bool FileUtils::isDirectory(const std::string& path) {
    return std::filesystem::is_directory(path);
}

/**
 * @brief 检查是否为符号链接
 * @param path 路径
 * @return 是否为符号链接
 */
bool FileUtils::isSymlink(const std::string& path) {
    return std::filesystem::is_symlink(path);
}

/**
 * @brief 获取文件大小
 * @param path 文件路径
 * @return 文件大小(字节)
 */
size_t FileUtils::getFileSize(const std::string& path) {
    try {
        return std::filesystem::file_size(path);
    } catch (const std::filesystem::filesystem_error&) {
        return 0;
    }
}

/**
 * @brief 获取文件信息
 * @param path 文件路径
 * @return 文件信息
 */
FileInfo FileUtils::getFileInfo(const std::string& path) {
    return FileInfo(path);
}

/**
 * @brief 创建目录
 * @param path 目录路径
 * @param recursive 是否递归创建
 * @return 是否成功
 */
bool FileUtils::createDirectory(const std::string& path, bool recursive) {
    try {
        if (recursive) {
            return std::filesystem::create_directories(path);
        } else {
            return std::filesystem::create_directory(path);
        }
    } catch (const std::filesystem::filesystem_error& e) {
        // FileUtils: Failed to create directory - removed logging
        return false;
    }
}

/**
 * @brief 删除文件或目录
 * @param path 路径
 * @param recursive 是否递归删除(目录)
 * @return 是否成功
 */
bool FileUtils::remove(const std::string& path, bool recursive) {
    try {
        if (recursive) {
            return std::filesystem::remove_all(path) > 0;
        } else {
            return std::filesystem::remove(path);
        }
    } catch (const std::filesystem::filesystem_error& e) {
        // FileUtils: Failed to remove - removed logging
        return false;
    }
}

/**
 * @brief 复制文件或目录
 * @param source 源路径
 * @param destination 目标路径
 * @param overwrite 是否覆盖
 * @return 是否成功
 */
bool FileUtils::copy(const std::string& source, const std::string& destination, bool overwrite) {
    try {
        std::filesystem::copy_options options = std::filesystem::copy_options::recursive;
        if (overwrite) {
            options |= std::filesystem::copy_options::overwrite_existing;
        }
        
        std::filesystem::copy(source, destination, options);
        return true;
    } catch (const std::filesystem::filesystem_error& e) {
        // FileUtils: Failed to copy - removed logging
        return false;
    }
}

/**
 * @brief 移动文件或目录
 * @param source 源路径
 * @param destination 目标路径
 * @param overwrite 是否覆盖
 * @return 是否成功
 */
bool FileUtils::move(const std::string& source, const std::string& destination, bool overwrite) {
    try {
        if (!overwrite && exists(destination)) {
            // FileUtils: Destination already exists - removed logging
            return false;
        }
        
        std::filesystem::rename(source, destination);
        return true;
    } catch (const std::filesystem::filesystem_error& e) {
        // FileUtils: Failed to move - removed logging
        return false;
    }
}

/**
 * @brief 重命名文件或目录
 * @param old_path 旧路径
 * @param new_path 新路径
 * @return 是否成功
 */
bool FileUtils::rename(const std::string& old_path, const std::string& new_path) {
    return move(old_path, new_path, false);
}

/**
 * @brief 列出目录内容
 * @param path 目录路径
 * @param recursive 是否递归
 * @return 文件信息列表
 */
std::vector<FileInfo> FileUtils::listDirectory(const std::string& path, bool recursive) {
    std::vector<FileInfo> results;
    
    try {
        if (recursive) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
                results.emplace_back(entry.path().string());
            }
        } else {
            for (const auto& entry : std::filesystem::directory_iterator(path)) {
                results.emplace_back(entry.path().string());
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        // FileUtils: Failed to list directory - removed logging
    }
    
    return results;
}

/**
 * @brief 搜索文件
 * @param root_path 根路径
 * @param pattern 搜索模式
 * @param options 搜索选项
 * @return 匹配的文件信息列表
 */
std::vector<FileInfo> FileUtils::searchFiles(const std::string& root_path,
                                           const std::string& pattern,
                                           const FileSearchOptions& options) {
    std::vector<FileInfo> results;
    searchFilesRecursive(root_path, pattern, options, results, 0);
    return results;
}

/**
 * @brief 读取文件内容
 * @param path 文件路径
 * @return 文件内容
 */
std::string FileUtils::readFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        // FileUtils: Failed to open file for reading - removed logging
        return "";
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    return content;
}

/**
 * @brief 读取文件内容(二进制)
 * @param path 文件路径
 * @return 文件内容
 */
std::vector<uint8_t> FileUtils::readBinaryFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        // FileUtils: Failed to open file for reading - removed logging
        return {};
    }
    
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());
    return data;
}

/**
 * @brief 写入文件内容
 * @param path 文件路径
 * @param content 内容
 * @param append 是否追加
 * @return 是否成功
 */
bool FileUtils::writeFile(const std::string& path, const std::string& content, bool append) {
    std::ios::openmode mode = std::ios::binary;
    if (append) {
        mode |= std::ios::app;
    }
    
    std::ofstream file(path, mode);
    if (!file.is_open()) {
        // FileUtils: Failed to open file for writing - removed logging
        return false;
    }
    
    file << content;
    return file.good();
}

/**
 * @brief 写入文件内容(二进制)
 * @param path 文件路径
 * @param data 数据
 * @param append 是否追加
 * @return 是否成功
 */
bool FileUtils::writeBinaryFile(const std::string& path, const std::vector<uint8_t>& data, bool append) {
    std::ios::openmode mode = std::ios::binary;
    if (append) {
        mode |= std::ios::app;
    }
    
    std::ofstream file(path, mode);
    if (!file.is_open()) {
        // FileUtils: Failed to open file for writing - removed logging
        return false;
    }
    
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    return file.good();
}

/**
 * @brief 按行读取文件
 * @param path 文件路径
 * @return 行列表
 */
std::vector<std::string> FileUtils::readLines(const std::string& path) {
    std::vector<std::string> lines;
    std::ifstream file(path);
    
    if (!file.is_open()) {
        // FileUtils: Failed to open file for reading - removed logging
        return lines;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    
    return lines;
}

/**
 * @brief 按行写入文件
 * @param path 文件路径
 * @param lines 行列表
 * @param append 是否追加
 * @return 是否成功
 */
bool FileUtils::writeLines(const std::string& path, const std::vector<std::string>& lines, bool append) {
    std::ios::openmode mode = std::ios::out;
    if (append) {
        mode |= std::ios::app;
    }
    
    std::ofstream file(path, mode);
    if (!file.is_open()) {
        // FileUtils: Failed to open file for writing - removed logging
        return false;
    }
    
    for (const auto& line : lines) {
        file << line << '\n';
    }
    
    return file.good();
}

/**
 * @brief 获取文件权限
 * @param path 文件路径
 * @return 权限标志
 */
uint32_t FileUtils::getPermissions(const std::string& path) {
    try {
        auto perms = std::filesystem::status(path).permissions();
        return static_cast<uint32_t>(perms);
    } catch (const std::filesystem::filesystem_error&) {
        return 0;
    }
}

/**
 * @brief 设置文件权限
 * @param path 文件路径
 * @param permissions 权限标志
 * @return 是否成功
 */
bool FileUtils::setPermissions(const std::string& path, uint32_t permissions) {
    try {
        std::filesystem::permissions(path, static_cast<std::filesystem::perms>(permissions));
        return true;
    } catch (const std::filesystem::filesystem_error& e) {
        // FileUtils: Failed to set permissions - removed logging
        return false;
    }
}

/**
 * @brief 获取当前工作目录
 * @return 当前工作目录
 */
std::string FileUtils::getCurrentDirectory() {
    try {
        return std::filesystem::current_path().string();
    } catch (const std::filesystem::filesystem_error&) {
        return "";
    }
}

/**
 * @brief 设置当前工作目录
 * @param path 目录路径
 * @return 是否成功
 */
bool FileUtils::setCurrentDirectory(const std::string& path) {
    try {
        std::filesystem::current_path(path);
        return true;
    } catch (const std::filesystem::filesystem_error& e) {
        // FileUtils: Failed to set current directory - removed logging
        return false;
    }
}

/**
 * @brief 获取绝对路径
 * @param path 相对路径
 * @return 绝对路径
 */
std::string FileUtils::getAbsolutePath(const std::string& path) {
    try {
        return std::filesystem::absolute(path).string();
    } catch (const std::filesystem::filesystem_error&) {
        return path;
    }
}

/**
 * @brief 获取相对路径
 * @param path 绝对路径
 * @param base 基准路径
 * @return 相对路径
 */
std::string FileUtils::getRelativePath(const std::string& path, const std::string& base) {
    try {
        std::filesystem::path base_path = base.empty() ? std::filesystem::current_path() : std::filesystem::path(base);
        return std::filesystem::relative(path, base_path).string();
    } catch (const std::filesystem::filesystem_error&) {
        return path;
    }
}

/**
 * @brief 规范化路径
 * @param path 路径
 * @return 规范化后的路径
 */
std::string FileUtils::normalizePath(const std::string& path) {
    try {
        return std::filesystem::weakly_canonical(path).string();
    } catch (const std::filesystem::filesystem_error&) {
        return cleanPath(path);
    }
}

/**
 * @brief 连接路径
 * @param paths 路径列表
 * @return 连接后的路径
 */
std::string FileUtils::joinPath(const std::vector<std::string>& paths) {
    if (paths.empty()) {
        return "";
    }
    
    std::filesystem::path result(paths[0]);
    for (size_t i = 1; i < paths.size(); ++i) {
        result /= paths[i];
    }
    
    return result.string();
}

/**
 * @brief 连接路径(可变参数)
 * @param path1 路径1
 * @param path2 路径2
 * @return 连接后的路径
 */
std::string FileUtils::joinPath(const std::string& path1, const std::string& path2) {
    return (std::filesystem::path(path1) / path2).string();
}

/**
 * @brief 获取目录名
 * @param path 文件路径
 * @return 目录名
 */
std::string FileUtils::getDirectoryName(const std::string& path) {
    return std::filesystem::path(path).parent_path().string();
}

/**
 * @brief 获取文件名
 * @param path 文件路径
 * @param include_extension 是否包含扩展名
 * @return 文件名
 */
std::string FileUtils::getFileName(const std::string& path, bool include_extension) {
    std::filesystem::path p(path);
    if (include_extension) {
        return p.filename().string();
    } else {
        return p.stem().string();
    }
}

/**
 * @brief 获取文件扩展名
 * @param path 文件路径
 * @return 扩展名
 */
std::string FileUtils::getFileExtension(const std::string& path) {
    std::string ext = std::filesystem::path(path).extension().string();
    if (!ext.empty() && ext[0] == '.') {
        ext = ext.substr(1);
    }
    return ext;
}

/**
 * @brief 更改文件扩展名
 * @param path 文件路径
 * @param new_extension 新扩展名
 * @return 新路径
 */
std::string FileUtils::changeExtension(const std::string& path, const std::string& new_extension) {
    std::filesystem::path p(path);
    std::string ext = new_extension;
    if (!ext.empty() && ext[0] != '.') {
        ext = "." + ext;
    }
    return p.replace_extension(ext).string();
}

/**
 * @brief 计算文件哈希值
 * @param path 文件路径
 * @param algorithm 哈希算法
 * @return 哈希值(十六进制字符串)
 */
std::string FileUtils::calculateFileHash(const std::string& path, const std::string& algorithm) {
    // This is a placeholder implementation
    // In a real implementation, you would use a proper hash library like OpenSSL
    // FileUtils: Hash calculation not implemented - removed logging
    return "";
}

/**
 * @brief 比较文件内容
 * @param path1 文件路径1
 * @param path2 文件路径2
 * @return 是否相同
 */
bool FileUtils::compareFiles(const std::string& path1, const std::string& path2) {
    if (!exists(path1) || !exists(path2)) {
        return false;
    }
    
    if (getFileSize(path1) != getFileSize(path2)) {
        return false;
    }
    
    std::ifstream file1(path1, std::ios::binary);
    std::ifstream file2(path2, std::ios::binary);
    
    if (!file1.is_open() || !file2.is_open()) {
        return false;
    }
    
    const size_t buffer_size = 4096;
    char buffer1[buffer_size];
    char buffer2[buffer_size];
    
    while (file1.read(buffer1, buffer_size) && file2.read(buffer2, buffer_size)) {
        if (file1.gcount() != file2.gcount() || 
            std::memcmp(buffer1, buffer2, file1.gcount()) != 0) {
            return false;
        }
    }
    
    return file1.eof() && file2.eof();
}

/**
 * @brief 获取磁盘空间信息
 * @param path 路径
 * @param total_space 总空间(字节)
 * @param free_space 可用空间(字节)
 * @return 是否成功
 */
bool FileUtils::getDiskSpace(const std::string& path, size_t& total_space, size_t& free_space) {
    try {
        auto space_info = std::filesystem::space(path);
        total_space = space_info.capacity;
        free_space = space_info.available;
        return true;
    } catch (const std::filesystem::filesystem_error& e) {
        // FileUtils: Failed to get disk space - removed logging
        return false;
    }
}

/**
 * @brief 创建符号链接
 * @param target 目标路径
 * @param link_path 链接路径
 * @return 是否成功
 */
bool FileUtils::createSymlink(const std::string& target, const std::string& link_path) {
    try {
        std::filesystem::create_symlink(target, link_path);
        return true;
    } catch (const std::filesystem::filesystem_error& e) {
        // FileUtils: Failed to create symlink - removed logging
        return false;
    }
}

/**
 * @brief 读取符号链接目标
 * @param link_path 链接路径
 * @return 目标路径
 */
std::string FileUtils::readSymlink(const std::string& link_path) {
    try {
        return std::filesystem::read_symlink(link_path).string();
    } catch (const std::filesystem::filesystem_error& e) {
        // FileUtils: Failed to read symlink - removed logging
        return "";
    }
}

/**
 * @brief 获取可读的文件大小字符串
 * @param size 文件大小(字节)
 * @return 可读字符串
 */
std::string FileUtils::getReadableSize(size_t size) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB"};
    const size_t num_units = sizeof(units) / sizeof(units[0]);
    
    double readable_size = static_cast<double>(size);
    size_t unit_index = 0;
    
    while (readable_size >= 1024.0 && unit_index < num_units - 1) {
        readable_size /= 1024.0;
        ++unit_index;
    }
    
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << readable_size << " " << units[unit_index];
    return ss.str();
}

/**
 * @brief 检查路径是否匹配模式
 * @param path 路径
 * @param pattern 模式(支持通配符)
 * @param case_sensitive 是否大小写敏感
 * @return 是否匹配
 */
bool FileUtils::matchPattern(const std::string& path, const std::string& pattern, bool case_sensitive) {
    // Convert glob pattern to regex
    std::string regex_pattern = pattern;
    
    // Escape special regex characters except * and ?
    std::string special_chars = ".^$+{}[]|()\\";
    for (char c : special_chars) {
        size_t pos = 0;
        std::string char_str(1, c);
        std::string escaped = "\\" + char_str;
        while ((pos = regex_pattern.find(char_str, pos)) != std::string::npos) {
            regex_pattern.replace(pos, 1, escaped);
            pos += escaped.length();
        }
    }
    
    // Convert glob wildcards to regex
    size_t pos = 0;
    while ((pos = regex_pattern.find("*", pos)) != std::string::npos) {
        regex_pattern.replace(pos, 1, ".*");
        pos += 2;
    }
    
    pos = 0;
    while ((pos = regex_pattern.find("?", pos)) != std::string::npos) {
        regex_pattern.replace(pos, 1, ".");
        pos += 1;
    }
    
    try {
        std::regex::flag_type flags = std::regex::ECMAScript;
        if (!case_sensitive) {
            flags |= std::regex::icase;
        }
        
        std::regex regex(regex_pattern, flags);
        return std::regex_match(path, regex);
    } catch (const std::regex_error&) {
        return false;
    }
}

/**
 * @brief 获取路径分隔符
 * @return 路径分隔符
 */
char FileUtils::getPathSeparator() {
    return PATH_SEPARATOR;
}

/**
 * @brief 检查路径是否为绝对路径
 * @param path 路径
 * @return 是否为绝对路径
 */
bool FileUtils::isAbsolutePath(const std::string& path) {
    return std::filesystem::path(path).is_absolute();
}

/**
 * @brief 清理路径(移除多余的分隔符等)
 * @param path 路径
 * @return 清理后的路径
 */
std::string FileUtils::cleanPath(const std::string& path) {
    if (path.empty()) {
        return path;
    }
    
    std::string cleaned = path;
    
    // Replace alternate separators
    std::replace(cleaned.begin(), cleaned.end(), ALT_PATH_SEPARATOR, PATH_SEPARATOR);
    
    // Remove duplicate separators
    std::string double_sep(2, PATH_SEPARATOR);
    std::string single_sep(1, PATH_SEPARATOR);
    
    size_t pos = 0;
    while ((pos = cleaned.find(double_sep, pos)) != std::string::npos) {
        cleaned.replace(pos, 2, single_sep);
    }
    
    // Remove trailing separator (except for root)
    if (cleaned.length() > 1 && cleaned.back() == PATH_SEPARATOR) {
        cleaned.pop_back();
    }
    
    return cleaned;
}

/**
 * @brief 递归搜索文件
 * @param root_path 根路径
 * @param pattern 搜索模式
 * @param options 搜索选项
 * @param results 结果列表
 * @param current_depth 当前深度
 */
void FileUtils::searchFilesRecursive(const std::string& root_path,
                                   const std::string& pattern,
                                   const FileSearchOptions& options,
                                   std::vector<FileInfo>& results,
                                   size_t current_depth) {
    if (current_depth >= options.max_depth || results.size() >= options.max_results) {
        return;
    }
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(root_path)) {
            if (results.size() >= options.max_results) {
                break;
            }
            
            FileInfo file_info(entry.path().string());
            
            // Skip hidden files if not included
            if (!options.include_hidden && file_info.is_hidden) {
                continue;
            }
            
            // Check if matches search criteria
            if (matchesSearchCriteria(file_info, pattern, options)) {
                results.push_back(file_info);
            }
            
            // Recurse into directories if recursive search is enabled
            if (options.recursive && file_info.isDirectory()) {
                searchFilesRecursive(file_info.path, pattern, options, results, current_depth + 1);
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        // FileUtils: Error searching directory - removed logging
    }
}

/**
 * @brief 检查文件是否匹配搜索条件
 * @param file_info 文件信息
 * @param pattern 搜索模式
 * @param options 搜索选项
 * @return 是否匹配
 */
bool FileUtils::matchesSearchCriteria(const FileInfo& file_info,
                                     const std::string& pattern,
                                     const FileSearchOptions& options) {
    // Check custom filter first
    if (options.custom_filter && !options.custom_filter(file_info)) {
        return false;
    }
    
    // Check extension filters
    if (!options.include_extensions.empty()) {
        bool found = false;
        for (const auto& ext : options.include_extensions) {
            if (file_info.extension == ext) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }
    
    if (!options.exclude_extensions.empty()) {
        for (const auto& ext : options.exclude_extensions) {
            if (file_info.extension == ext) {
                return false;
            }
        }
    }
    
    // Check include patterns
    if (!options.include_patterns.empty()) {
        bool found = false;
        for (const auto& pat : options.include_patterns) {
            if (matchPattern(file_info.name, pat, options.case_sensitive)) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }
    
    // Check exclude patterns
    if (!options.exclude_patterns.empty()) {
        for (const auto& pat : options.exclude_patterns) {
            if (matchPattern(file_info.name, pat, options.case_sensitive)) {
                return false;
            }
        }
    }
    
    // Check main pattern
    if (!pattern.empty()) {
        return matchPattern(file_info.name, pattern, options.case_sensitive);
    }
    
    return true;
}

/**
 * @brief 获取可执行文件路径
 * @return 可执行文件路径
 */
std::string FileUtils::getExecutablePath() {
#ifdef _WIN32
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    return std::string(buffer);
#else
    char buffer[1024];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer)-1);
    if (len != -1) {
        buffer[len] = '\0';
        return std::string(buffer);
    }
    return "";
#endif
}

/**
 * @brief 获取可执行文件目录
 * @return 可执行文件目录
 */
std::string FileUtils::getExecutableDirectory() {
    std::string exePath = getExecutablePath();
    if (!exePath.empty()) {
        size_t pos = exePath.find_last_of("\\/");
        if (pos != std::string::npos) {
            return exePath.substr(0, pos);
        }
    }
    return "";
}

} // namespace Utils
} // namespace Core
} // namespace DearTs