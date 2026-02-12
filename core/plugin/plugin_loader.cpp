/**
 * @file plugin_loader.cpp
 * @brief 动态库加载器实现
 */

#include "plugin_loader.h"
#include "liblogger/logger.h"
#include <format>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

namespace DearTs::Core::Plugin {

// ================ DynamicLibraryLoader ================

std::unique_ptr<DynamicLibraryLoader> DynamicLibraryLoader::create() {
#ifdef _WIN32
    return std::make_unique<WindowsLibraryLoader>();
#elif defined(__linux__) || defined(__APPLE__)
    return std::make_unique<UnixLibraryLoader>();
#else
    #error "Unsupported platform"
#endif
}

#ifdef _WIN32

// ================ WindowsLibraryLoader ================

Result<void, std::string> WindowsLibraryLoader::load(const std::filesystem::path& path) {
    if (m_handle) {
        return Result<void, std::string>::err("Library already loaded");
    }

    // 使用 LoadLibraryW 加载 DLL（支持宽字符路径）
    m_handle = LoadLibraryW(path.wstring().c_str());

    if (!m_handle) {
        DWORD error = GetLastError();
        LPSTR errorText = nullptr;

        // 获取错误消息
        if (FormatMessageA(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                nullptr,
                error,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPSTR)&errorText,
                0,
                nullptr
            )) {
            std::string error_msg = errorText;
            LocalFree(errorText);

            return Result<void, std::string>::err(
                std::format("Failed to load library '{}': {}", path.string(), error_msg)
            );
        } else {
            return Result<void, std::string>::err(
                std::format("Failed to load library '{}': Unknown error (code: {})", path.string(), error)
            );
        }
    }

    LOG_INFO("Loaded library: {}", path.string());
    return Result<void, std::string>::ok();
}

Result<void*, std::string> WindowsLibraryLoader::get_symbol(const char* name) {
    if (!m_handle) {
        return Result<void*, std::string>::err("Library not loaded");
    }

    if (!name) {
        return Result<void*, std::string>::err("Symbol name is null");
    }

    // 获取函数地址
    void* symbol = reinterpret_cast<void*>(GetProcAddress(m_handle, name));

    if (!symbol) {
        DWORD error = GetLastError();
        return Result<void*, std::string>::err(
            std::format("Symbol '{}' not found (Error code: {})", name, error)
        );
    }

    LOG_DEBUG("Found symbol: {} at {}", name, symbol);
    return Result<void*, std::string>::ok(symbol);
}

void WindowsLibraryLoader::unload() {
    if (m_handle) {
        LOG_INFO("Unloading library");
        FreeLibrary(m_handle);
        m_handle = nullptr;
    }
}

#elif defined(__linux__) || defined(__APPLE__)

// ================ UnixLibraryLoader ================

Result<void, std::string> UnixLibraryLoader::load(const std::filesystem::path& path) {
    if (m_handle) {
        return Result<void, std::string>::err("Library already loaded");
    }

    // 使用 dlopen 加载动态库
    // RTLD_LAZY: 延迟绑定（推荐）
    // RTLD_NOW: 立即绑定（用于调试）
    m_handle = dlopen(path.string().c_str(), RTLD_LAZY);

    if (!m_handle) {
        const char* error = dlerror();
        return Result<void, std::string>::err(
            std::format("Failed to load library '{}': {}", path.string(), error ? error : "Unknown error")
        );
    }

    LOG_INFO("Loaded library: {}", path.string());
    return Result<void, std::string>::ok();
}

Result<void*, std::string> UnixLibraryLoader::get_symbol(const char* name) {
    if (!m_handle) {
        return Result<void*, std::string>::err("Library not loaded");
    }

    if (!name) {
        return Result<void*, std::string>::err("Symbol name is null");
    }

    // 清除之前的错误
    dlerror();

    // 获取符号地址
    void* symbol = dlsym(m_handle, name);

    if (!symbol) {
        const char* error = dlerror();
        return Result<void*, std::string>::err(
            std::format("Symbol '{}' not found: {}", name, error ? error : "Unknown error")
        );
    }

    LOG_DEBUG("Found symbol: {} at {}", name, symbol);
    return Result<void*, std::string>::ok(symbol);
}

void UnixLibraryLoader::unload() {
    if (m_handle) {
        LOG_INFO("Unloading library");
        dlclose(m_handle);
        m_handle = nullptr;
    }
}

#endif

} // namespace DearTs::Core::Plugin
