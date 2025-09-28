/**
 * DearTs File System Utilities
 * 
 * 文件系统工具类 - 提供文件和目录操作的便利功能
 * 支持跨平台文件操作、路径处理、文件监控、压缩解压等功能
 * 
 * @author DearTs Team
 * @version 1.0.0
 * @date 2025
 * 
 * Features:
 * - 跨平台文件操作 (Cross-platform File Operations)
 * - 路径处理和规范化 (Path Processing and Normalization)
 * - 文件和目录监控 (File and Directory Monitoring)
 * - 文件压缩和解压 (File Compression and Decompression)
 * - 文件搜索和过滤 (File Search and Filtering)
 * - 文件权限管理 (File Permission Management)
 * - 临时文件管理 (Temporary File Management)
 * - 文件锁定机制 (File Locking Mechanism)
 */

#pragma once

#ifndef DEARTS_FILE_UTILS_H
#define DEARTS_FILE_UTILS_H

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <chrono>
#include <fstream>
#include <filesystem>
#include <regex>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>

namespace DearTs {
namespace Core {
namespace Utils {

/**
 * @brief 文件类型枚举
 */
enum class FileType {
    UNKNOWN,        ///< 未知类型
    REGULAR_FILE,   ///< 普通文件
    DIRECTORY,      ///< 目录
    SYMLINK,        ///< 符号链接
    BLOCK_DEVICE,   ///< 块设备
    CHAR_DEVICE,    ///< 字符设备
    FIFO,           ///< 命名管道
    SOCKET          ///< 套接字
};

/**
 * @brief 文件权限枚举
 */
enum class FilePermission {
    NONE = 0,
    OWNER_READ = 1 << 0,
    OWNER_WRITE = 1 << 1,
    OWNER_EXEC = 1 << 2,
    GROUP_READ = 1 << 3,
    GROUP_WRITE = 1 << 4,
    GROUP_EXEC = 1 << 5,
    OTHER_READ = 1 << 6,
    OTHER_WRITE = 1 << 7,
    OTHER_EXEC = 1 << 8,
    ALL_READ = OWNER_READ | GROUP_READ | OTHER_READ,
    ALL_WRITE = OWNER_WRITE | GROUP_WRITE | OTHER_WRITE,
    ALL_EXEC = OWNER_EXEC | GROUP_EXEC | OTHER_EXEC,
    ALL_PERMISSIONS = ALL_READ | ALL_WRITE | ALL_EXEC
};

/**
 * @brief 文件监控事件类型
 */
enum class FileWatchEvent {
    CREATED,        ///< 文件创建
    MODIFIED,       ///< 文件修改
    DELETED,        ///< 文件删除
    RENAMED,        ///< 文件重命名
    MOVED           ///< 文件移动
};

/**
 * @brief 文件信息结构
 */
struct FileInfo {
    std::string path;                                          ///< 文件路径
    std::string name;                                          ///< 文件名
    std::string extension;                                     ///< 文件扩展名
    FileType type;                                             ///< 文件类型
    size_t size;                                               ///< 文件大小(字节)
    std::chrono::system_clock::time_point created_time;       ///< 创建时间
    std::chrono::system_clock::time_point modified_time;      ///< 修改时间
    std::chrono::system_clock::time_point accessed_time;      ///< 访问时间
    uint32_t permissions;                                      ///< 文件权限
    bool is_hidden;                                            ///< 是否隐藏文件
    bool is_readonly;                                          ///< 是否只读
    
    /**
     * @brief 构造函数
     */
    FileInfo();
    
    /**
     * @brief 从路径构造
     * @param file_path 文件路径
     */
    explicit FileInfo(const std::string& file_path);
    
    /**
     * @brief 检查是否为目录
     * @return 是否为目录
     */
    bool isDirectory() const { return type == FileType::DIRECTORY; }
    
    /**
     * @brief 检查是否为普通文件
     * @return 是否为普通文件
     */
    bool isRegularFile() const { return type == FileType::REGULAR_FILE; }
    
    /**
     * @brief 检查是否为符号链接
     * @return 是否为符号链接
     */
    bool isSymlink() const { return type == FileType::SYMLINK; }
    
    /**
     * @brief 获取可读的文件大小字符串
     * @return 文件大小字符串
     */
    std::string getReadableSize() const;
    
    /**
     * @brief 检查权限
     * @param permission 权限标志
     * @return 是否具有权限
     */
    bool hasPermission(FilePermission permission) const;
};

/**
 * @brief 文件搜索选项
 */
struct FileSearchOptions {
    bool recursive = true;                                     ///< 是否递归搜索
    bool include_hidden = false;                              ///< 是否包含隐藏文件
    bool case_sensitive = true;                               ///< 是否大小写敏感
    size_t max_depth = std::numeric_limits<size_t>::max();    ///< 最大搜索深度
    size_t max_results = std::numeric_limits<size_t>::max();  ///< 最大结果数量
    std::vector<std::string> include_extensions;              ///< 包含的扩展名
    std::vector<std::string> exclude_extensions;              ///< 排除的扩展名
    std::vector<std::string> include_patterns;                ///< 包含的模式
    std::vector<std::string> exclude_patterns;                ///< 排除的模式
    std::function<bool(const FileInfo&)> custom_filter;       ///< 自定义过滤器
};

/**
 * @brief 文件监控回调函数类型
 */
using FileWatchCallback = std::function<void(const std::string& path, FileWatchEvent event)>;

/**
 * @brief 文件监控器类
 */
class FileWatcher {
public:
    /**
     * @brief 构造函数
     */
    FileWatcher();
    
    /**
     * @brief 析构函数
     */
    ~FileWatcher();
    
    /**
     * @brief 禁用拷贝构造
     */
    FileWatcher(const FileWatcher&) = delete;
    
    /**
     * @brief 禁用拷贝赋值
     */
    FileWatcher& operator=(const FileWatcher&) = delete;
    
    /**
     * @brief 添加监控路径
     * @param path 监控路径
     * @param callback 回调函数
     * @param recursive 是否递归监控
     * @return 是否成功
     */
    bool addWatch(const std::string& path, FileWatchCallback callback, bool recursive = false);
    
    /**
     * @brief 移除监控路径
     * @param path 监控路径
     * @return 是否成功
     */
    bool removeWatch(const std::string& path);
    
    /**
     * @brief 启动监控
     * @return 是否成功
     */
    bool start();
    
    /**
     * @brief 停止监控
     */
    void stop();
    
    /**
     * @brief 检查是否正在运行
     * @return 是否运行中
     */
    bool isRunning() const { return running_; }
    
    /**
     * @brief 获取监控路径列表
     * @return 路径列表
     */
    std::vector<std::string> getWatchedPaths() const;
    
private:
    struct WatchEntry {
        std::string path;
        FileWatchCallback callback;
        bool recursive;
        void* platform_handle;  // 平台特定的句柄
    };
    
    std::unordered_map<std::string, WatchEntry> watches_;     ///< 监控条目
    std::atomic<bool> running_;                               ///< 运行状态
    std::thread watch_thread_;                                ///< 监控线程
    mutable std::mutex mutex_;                                ///< 线程安全锁
    
    /**
     * @brief 监控线程函数
     */
    void watchThread();
    
    /**
     * @brief 处理文件事件
     * @param path 文件路径
     * @param event 事件类型
     */
    void handleFileEvent(const std::string& path, FileWatchEvent event);
};

/**
 * @brief 临时文件管理器类
 */
class TempFileManager {
public:
    /**
     * @brief 构造函数
     */
    TempFileManager();
    
    /**
     * @brief 析构函数
     */
    ~TempFileManager();
    
    /**
     * @brief 禁用拷贝构造
     */
    TempFileManager(const TempFileManager&) = delete;
    
    /**
     * @brief 禁用拷贝赋值
     */
    TempFileManager& operator=(const TempFileManager&) = delete;
    
    /**
     * @brief 获取单例实例
     * @return 管理器实例
     */
    static TempFileManager& getInstance();
    
    /**
     * @brief 创建临时文件
     * @param prefix 文件名前缀
     * @param suffix 文件名后缀
     * @return 临时文件路径
     */
    std::string createTempFile(const std::string& prefix = "temp", const std::string& suffix = ".tmp");
    
    /**
     * @brief 创建临时目录
     * @param prefix 目录名前缀
     * @return 临时目录路径
     */
    std::string createTempDirectory(const std::string& prefix = "temp");
    
    /**
     * @brief 注册临时文件(用于自动清理)
     * @param path 文件路径
     */
    void registerTempFile(const std::string& path);
    
    /**
     * @brief 注销临时文件
     * @param path 文件路径
     */
    void unregisterTempFile(const std::string& path);
    
    /**
     * @brief 清理所有临时文件
     */
    void cleanupAll();
    
    /**
     * @brief 清理过期的临时文件
     * @param max_age_hours 最大存在时间(小时)
     */
    void cleanupExpired(int max_age_hours = 24);
    
    /**
     * @brief 设置临时目录
     * @param temp_dir 临时目录路径
     */
    void setTempDirectory(const std::string& temp_dir);
    
    /**
     * @brief 获取临时目录
     * @return 临时目录路径
     */
    std::string getTempDirectory() const;
    
    /**
     * @brief 获取系统临时目录
     * @return 系统临时目录路径
     */
    static std::string getSystemTempDirectory();
    
private:
    std::string temp_directory_;                               ///< 临时目录
    std::vector<std::string> temp_files_;                      ///< 临时文件列表
    mutable std::mutex mutex_;                                 ///< 线程安全锁
    
    /**
     * @brief 生成唯一文件名
     * @param prefix 前缀
     * @param suffix 后缀
     * @return 唯一文件名
     */
    std::string generateUniqueFileName(const std::string& prefix, const std::string& suffix);
};

/**
 * @brief 文件锁类
 */
class FileLock {
public:
    /**
     * @brief 构造函数
     * @param file_path 文件路径
     */
    explicit FileLock(const std::string& file_path);
    
    /**
     * @brief 析构函数
     */
    ~FileLock();
    
    /**
     * @brief 禁用拷贝构造
     */
    FileLock(const FileLock&) = delete;
    
    /**
     * @brief 禁用拷贝赋值
     */
    FileLock& operator=(const FileLock&) = delete;
    
    /**
     * @brief 获取独占锁
     * @param timeout_ms 超时时间(毫秒)
     * @return 是否成功
     */
    bool lock(int timeout_ms = -1);
    
    /**
     * @brief 尝试获取独占锁(非阻塞)
     * @return 是否成功
     */
    bool tryLock();
    
    /**
     * @brief 获取共享锁
     * @param timeout_ms 超时时间(毫秒)
     * @return 是否成功
     */
    bool lockShared(int timeout_ms = -1);
    
    /**
     * @brief 尝试获取共享锁(非阻塞)
     * @return 是否成功
     */
    bool tryLockShared();
    
    /**
     * @brief 释放锁
     */
    void unlock();
    
    /**
     * @brief 检查是否已锁定
     * @return 是否已锁定
     */
    bool isLocked() const { return locked_; }
    
    /**
     * @brief 获取文件路径
     * @return 文件路径
     */
    const std::string& getFilePath() const { return file_path_; }
    
private:
    std::string file_path_;                                    ///< 文件路径
    void* platform_handle_;                                   ///< 平台特定句柄
    bool locked_;                                              ///< 锁定状态
    bool shared_lock_;                                         ///< 是否为共享锁
};

/**
 * @brief 文件系统工具类
 */
class FileUtils {
public:
    /**
     * @brief 检查文件是否存在
     * @param path 文件路径
     * @return 是否存在
     */
    static bool exists(const std::string& path);
    
    /**
     * @brief 检查是否为文件
     * @param path 路径
     * @return 是否为文件
     */
    static bool isFile(const std::string& path);
    
    /**
     * @brief 检查是否为目录
     * @param path 路径
     * @return 是否为目录
     */
    static bool isDirectory(const std::string& path);
    
    /**
     * @brief 检查是否为符号链接
     * @param path 路径
     * @return 是否为符号链接
     */
    static bool isSymlink(const std::string& path);
    
    /**
     * @brief 获取文件大小
     * @param path 文件路径
     * @return 文件大小(字节)
     */
    static size_t getFileSize(const std::string& path);
    
    /**
     * @brief 获取文件信息
     * @param path 文件路径
     * @return 文件信息
     */
    static FileInfo getFileInfo(const std::string& path);
    
    /**
     * @brief 创建目录
     * @param path 目录路径
     * @param recursive 是否递归创建
     * @return 是否成功
     */
    static bool createDirectory(const std::string& path, bool recursive = true);
    
    /**
     * @brief 删除文件或目录
     * @param path 路径
     * @param recursive 是否递归删除(目录)
     * @return 是否成功
     */
    static bool remove(const std::string& path, bool recursive = false);
    
    /**
     * @brief 复制文件或目录
     * @param source 源路径
     * @param destination 目标路径
     * @param overwrite 是否覆盖
     * @return 是否成功
     */
    static bool copy(const std::string& source, const std::string& destination, bool overwrite = false);
    
    /**
     * @brief 移动文件或目录
     * @param source 源路径
     * @param destination 目标路径
     * @param overwrite 是否覆盖
     * @return 是否成功
     */
    static bool move(const std::string& source, const std::string& destination, bool overwrite = false);
    
    /**
     * @brief 重命名文件或目录
     * @param old_path 旧路径
     * @param new_path 新路径
     * @return 是否成功
     */
    static bool rename(const std::string& old_path, const std::string& new_path);
    
    /**
     * @brief 列出目录内容
     * @param path 目录路径
     * @param recursive 是否递归
     * @return 文件信息列表
     */
    static std::vector<FileInfo> listDirectory(const std::string& path, bool recursive = false);
    
    /**
     * @brief 搜索文件
     * @param root_path 根路径
     * @param pattern 搜索模式
     * @param options 搜索选项
     * @return 匹配的文件信息列表
     */
    static std::vector<FileInfo> searchFiles(const std::string& root_path, 
                                            const std::string& pattern, 
                                            const FileSearchOptions& options = {});
    
    /**
     * @brief 读取文件内容
     * @param path 文件路径
     * @return 文件内容
     */
    static std::string readFile(const std::string& path);
    
    /**
     * @brief 读取文件内容(二进制)
     * @param path 文件路径
     * @return 文件内容
     */
    static std::vector<uint8_t> readBinaryFile(const std::string& path);
    
    /**
     * @brief 写入文件内容
     * @param path 文件路径
     * @param content 内容
     * @param append 是否追加
     * @return 是否成功
     */
    static bool writeFile(const std::string& path, const std::string& content, bool append = false);
    
    /**
     * @brief 写入文件内容(二进制)
     * @param path 文件路径
     * @param data 数据
     * @param append 是否追加
     * @return 是否成功
     */
    static bool writeBinaryFile(const std::string& path, const std::vector<uint8_t>& data, bool append = false);
    
    /**
     * @brief 按行读取文件
     * @param path 文件路径
     * @return 行列表
     */
    static std::vector<std::string> readLines(const std::string& path);
    
    /**
     * @brief 按行写入文件
     * @param path 文件路径
     * @param lines 行列表
     * @param append 是否追加
     * @return 是否成功
     */
    static bool writeLines(const std::string& path, const std::vector<std::string>& lines, bool append = false);
    
    /**
     * @brief 获取文件权限
     * @param path 文件路径
     * @return 权限标志
     */
    static uint32_t getPermissions(const std::string& path);
    
    /**
     * @brief 设置文件权限
     * @param path 文件路径
     * @param permissions 权限标志
     * @return 是否成功
     */
    static bool setPermissions(const std::string& path, uint32_t permissions);
    
    /**
     * @brief 获取当前工作目录
     * @return 当前工作目录
     */
    static std::string getCurrentDirectory();
    
    /**
     * @brief 设置当前工作目录
     * @param path 目录路径
     * @return 是否成功
     */
    static bool setCurrentDirectory(const std::string& path);
    
    /**
     * @brief 获取绝对路径
     * @param path 相对路径
     * @return 绝对路径
     */
    static std::string getAbsolutePath(const std::string& path);
    
    /**
     * @brief 获取相对路径
     * @param path 绝对路径
     * @param base 基准路径
     * @return 相对路径
     */
    static std::string getRelativePath(const std::string& path, const std::string& base = "");
    
    /**
     * @brief 规范化路径
     * @param path 路径
     * @return 规范化后的路径
     */
    static std::string normalizePath(const std::string& path);
    
    /**
     * @brief 连接路径
     * @param paths 路径列表
     * @return 连接后的路径
     */
    static std::string joinPath(const std::vector<std::string>& paths);
    
    /**
     * @brief 连接路径(可变参数)
     * @param path1 路径1
     * @param path2 路径2
     * @return 连接后的路径
     */
    static std::string joinPath(const std::string& path1, const std::string& path2);
    
    /**
     * @brief 获取目录名
     * @param path 文件路径
     * @return 目录名
     */
    static std::string getDirectoryName(const std::string& path);
    
    /**
     * @brief 获取文件名
     * @param path 文件路径
     * @param include_extension 是否包含扩展名
     * @return 文件名
     */
    static std::string getFileName(const std::string& path, bool include_extension = true);
    
    /**
     * @brief 获取文件扩展名
     * @param path 文件路径
     * @return 扩展名
     */
    static std::string getFileExtension(const std::string& path);
    
    /**
     * @brief 更改文件扩展名
     * @param path 文件路径
     * @param new_extension 新扩展名
     * @return 新路径
     */
    static std::string changeExtension(const std::string& path, const std::string& new_extension);
    
    /**
     * @brief 计算文件哈希值
     * @param path 文件路径
     * @param algorithm 哈希算法("md5", "sha1", "sha256")
     * @return 哈希值(十六进制字符串)
     */
    static std::string calculateFileHash(const std::string& path, const std::string& algorithm = "md5");
    
    /**
     * @brief 比较文件内容
     * @param path1 文件路径1
     * @param path2 文件路径2
     * @return 是否相同
     */
    static bool compareFiles(const std::string& path1, const std::string& path2);
    
    /**
     * @brief 获取磁盘空间信息
     * @param path 路径
     * @param total_space 总空间(字节)
     * @param free_space 可用空间(字节)
     * @return 是否成功
     */
    static bool getDiskSpace(const std::string& path, size_t& total_space, size_t& free_space);
    
    /**
     * @brief 创建符号链接
     * @param target 目标路径
     * @param link_path 链接路径
     * @return 是否成功
     */
    static bool createSymlink(const std::string& target, const std::string& link_path);
    
    /**
     * @brief 读取符号链接目标
     * @param link_path 链接路径
     * @return 目标路径
     */
    static std::string readSymlink(const std::string& link_path);
    
    /**
     * @brief 获取可读的文件大小字符串
     * @param size 文件大小(字节)
     * @return 可读字符串
     */
    static std::string getReadableSize(size_t size);
    
    /**
     * @brief 检查路径是否匹配模式
     * @param path 路径
     * @param pattern 模式(支持通配符)
     * @param case_sensitive 是否大小写敏感
     * @return 是否匹配
     */
    static bool matchPattern(const std::string& path, const std::string& pattern, bool case_sensitive = true);
    
    /**
     * @brief 获取路径分隔符
     * @return 路径分隔符
     */
    static char getPathSeparator();
    
    /**
     * @brief 检查路径是否为绝对路径
     * @param path 路径
     * @return 是否为绝对路径
     */
    static bool isAbsolutePath(const std::string& path);
    
    /**
     * @brief 清理路径(移除多余的分隔符等)
     * @param path 路径
     * @return 清理后的路径
     */
    static std::string cleanPath(const std::string& path);
    
    /**
     * @brief 获取可执行文件路径
     * @return 可执行文件路径
     */
    static std::string getExecutablePath();
    
    /**
     * @brief 获取可执行文件目录
     * @return 可执行文件目录
     */
    static std::string getExecutableDirectory();
    
private:
    /**
     * @brief 递归复制目录
     * @param source 源目录
     * @param destination 目标目录
     * @param overwrite 是否覆盖
     * @return 是否成功
     */
    static bool copyDirectoryRecursive(const std::string& source, const std::string& destination, bool overwrite);
    
    /**
     * @brief 递归删除目录
     * @param path 目录路径
     * @return 是否成功
     */
    static bool removeDirectoryRecursive(const std::string& path);
    
    /**
     * @brief 递归搜索文件
     * @param root_path 根路径
     * @param pattern 搜索模式
     * @param options 搜索选项
     * @param results 结果列表
     * @param current_depth 当前深度
     */
    static void searchFilesRecursive(const std::string& root_path,
                                   const std::string& pattern,
                                   const FileSearchOptions& options,
                                   std::vector<FileInfo>& results,
                                   size_t current_depth = 0);
    
    /**
     * @brief 检查文件是否匹配搜索条件
     * @param file_info 文件信息
     * @param pattern 搜索模式
     * @param options 搜索选项
     * @return 是否匹配
     */
    static bool matchesSearchCriteria(const FileInfo& file_info,
                                     const std::string& pattern,
                                     const FileSearchOptions& options);
};

} // namespace Utils
} // namespace Core
} // namespace DearTs

#endif // DEARTS_FILE_UTILS_H