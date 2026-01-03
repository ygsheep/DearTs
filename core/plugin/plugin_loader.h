/**
 * @file plugin_loader.h
 * @brief 动态库加载器抽象层
 * @details 提供跨平台的动态库加载接口
 * @author DearTs Team
 * @date 2025
 * @version 1.0.0
 */

#pragma once

#include "core/result.h"
#include <filesystem>
#include <memory>
#include <string>

// 平台特定的头文件
#ifdef _WIN32
    #include <windows.h>
#elif defined(__linux__) || defined(__APPLE__)
    #include <dlfcn.h>
#endif

namespace DearTs::Core::Plugin {

/**
 * @brief 动态库加载器抽象类
 * @details 提供跨平台的动态库加载接口，支持 Windows、Linux 和 macOS
 */
class DynamicLibraryLoader {
public:
    virtual ~DynamicLibraryLoader() = default;

    /**
     * @brief 加载动态库
     * @param path 动态库文件路径
     * @return 成功返回 void，失败返回错误信息
     */
    virtual Result<void, std::string> load(const std::filesystem::path& path) = 0;

    /**
     * @brief 获取符号地址
     * @param name 符号名称
     * @return 成功返回符号地址，失败返回错误信息
     */
    virtual Result<void*, std::string> get_symbol(const char* name) = 0;

    /**
     * @brief 卸载动态库
     */
    virtual void unload() = 0;

    /**
     * @brief 创建平台特定的加载器
     * @return 平台特定的加载器实例
     */
    static std::unique_ptr<DynamicLibraryLoader> create();
};

#ifdef _WIN32

/**
 * @brief Windows 动态库加载器实现
 * @details 使用 Win32 API: LoadLibraryW, GetProcAddress, FreeLibrary
 */
class WindowsLibraryLoader : public DynamicLibraryLoader {
public:
    WindowsLibraryLoader() = default;
    ~WindowsLibraryLoader() override {
        unload();
    }

    // 禁止拷贝和移动
    WindowsLibraryLoader(const WindowsLibraryLoader&) = delete;
    WindowsLibraryLoader& operator=(const WindowsLibraryLoader&) = delete;
    WindowsLibraryLoader(WindowsLibraryLoader&&) noexcept = delete;
    WindowsLibraryLoader& operator=(WindowsLibraryLoader&&) noexcept = delete;

    Result<void, std::string> load(const std::filesystem::path& path) override;
    Result<void*, std::string> get_symbol(const char* name) override;
    void unload() override;

private:
    HMODULE m_handle = nullptr;
};

#elif defined(__linux__) || defined(__APPLE__)

/**
 * @brief Unix/Linux/macOS 动态库加载器实现
 * @details使用 POSIX API: dlopen, dlsym, dlclose
 */
class UnixLibraryLoader : public DynamicLibraryLoader {
public:
    UnixLibraryLoader() = default;
    ~UnixLibraryLoader() override {
        unload();
    }

    // 禁止拷贝和移动
    UnixLibraryLoader(const UnixLibraryLoader&) = delete;
    UnixLibraryLoader& operator=(const UnixLibraryLoader&) = delete;
    UnixLibraryLoader(UnixLibraryLoader&&) noexcept = delete;
    UnixLibraryLoader& operator=(UnixLibraryLoader&&) noexcept = delete;

    Result<void, std::string> load(const std::filesystem::path& path) override;
    Result<void*, std::string> get_symbol(const char* name) override;
    void unload() override;

private:
    void* m_handle = nullptr;
};

#endif

} // namespace DearTs::Core::Plugin
