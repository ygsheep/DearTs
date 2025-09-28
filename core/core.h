/**
 * @file core.h
 * @brief DearTs核心系统统一头文件
 * @details 包含所有核心模块的头文件，提供完整的核心功能访问
 * @author DearTs Team
 * @date 2024
 */

#pragma once

// 直接包含必要的标准库头文件以避免预编译头文件问题
#include <string>
#include <iostream>
#include <cassert>

// 版本信息 - 使用现代C++constexpr变量
namespace DearTs {
    namespace Version {
        constexpr int MAJOR = 1;
        constexpr int MINOR = 0;
        constexpr int PATCH = 0;
        constexpr const char* STRING = "1.0.0";
        constexpr const char* GIT_COMMIT_HASH = "unknown";
        
        /**
         * @brief 获取完整版本字符串
         * @return 版本字符串
         */
        constexpr const char* getVersionString() noexcept {
            return STRING;
        }
        
        /**
         * @brief 获取Git提交哈希
         * @return Git提交哈希字符串
         */
        constexpr const char* getGitCommitHash() noexcept {
            return GIT_COMMIT_HASH;
        }
    }
}

// 平台检测
#ifndef DEARTS_PLATFORM_WINDOWS
    #ifdef _WIN32
        #define DEARTS_PLATFORM_WINDOWS 1
    #else
        #define DEARTS_PLATFORM_WINDOWS 0
    #endif
#endif

#ifndef DEARTS_PLATFORM_LINUX
    #ifdef __linux__
        #define DEARTS_PLATFORM_LINUX 1
    #else
        #define DEARTS_PLATFORM_LINUX 0
    #endif
#endif

#ifndef DEARTS_PLATFORM_MACOS
    #ifdef __APPLE__
        #define DEARTS_PLATFORM_MACOS 1
    #else
        #define DEARTS_PLATFORM_MACOS 0
    #endif
#endif

// 编译器检测
#ifndef DEARTS_COMPILER_MSVC
    #ifdef _MSC_VER
        #define DEARTS_COMPILER_MSVC 1
        #define DEARTS_COMPILER_GCC 0
        #define DEARTS_COMPILER_CLANG 0
    #elif defined(__GNUC__) && !defined(__clang__)
        #define DEARTS_COMPILER_MSVC 0
        #define DEARTS_COMPILER_GCC 1
        #define DEARTS_COMPILER_CLANG 0
    #elif defined(__clang__)
        #define DEARTS_COMPILER_MSVC 0
        #define DEARTS_COMPILER_GCC 0
        #define DEARTS_COMPILER_CLANG 1
    #else
        #define DEARTS_COMPILER_MSVC 0
        #define DEARTS_COMPILER_GCC 0
        #define DEARTS_COMPILER_CLANG 0
    #endif
#endif

// 调试模式检测
#ifndef DEARTS_DEBUG
    #ifdef _DEBUG
        #define DEARTS_DEBUG
    #endif
#endif

// API导出宏
#ifndef DEARTS_API
    #ifdef _WIN32
        #ifdef DEARTS_EXPORTS
            #define DEARTS_API __declspec(dllexport)
        #elif defined(DEARTS_SHARED)
            #define DEARTS_API __declspec(dllimport)
        #else
            #define DEARTS_API
        #endif
    #else
        #ifdef DEARTS_SHARED
            #define DEARTS_API __attribute__((visibility("default")))
        #else
            #define DEARTS_API
        #endif
    #endif
#endif

// 强制内联宏
#ifdef DEARTS_COMPILER_MSVC
    #define DEARTS_FORCE_INLINE __forceinline
#else
    #define DEARTS_FORCE_INLINE __attribute__((always_inline)) inline
#endif

// 禁用警告宏
#ifdef DEARTS_COMPILER_MSVC
    #define DEARTS_DISABLE_WARNING_PUSH __pragma(warning(push))
    #define DEARTS_DISABLE_WARNING_POP __pragma(warning(pop))
    #define DEARTS_DISABLE_WARNING(warningNumber) __pragma(warning(disable: warningNumber))
#else
    #define DEARTS_DISABLE_WARNING_PUSH _Pragma("GCC diagnostic push")
    #define DEARTS_DISABLE_WARNING_POP _Pragma("GCC diagnostic pop")
    #define DEARTS_DISABLE_WARNING(warningName) _Pragma("GCC diagnostic ignored \"" #warningName "\"")
#endif

// 断言宏
#ifdef DEARTS_DEBUG
    #include <cassert>
    #define DEARTS_ASSERT(condition, message) assert((condition) && (message))
#else
    #define DEARTS_ASSERT(condition, message) ((void)0)
#endif

// 现代C++日志系统 - 已移除宏定义，使用Logger类
// 日志功能现在通过 DearTs::Log 命名空间提供

// 性能分析宏
#ifdef DEARTS_DEBUG
    #define DEARTS_PROFILE_SCOPE(name) DearTs::ProfilerScope _prof_scope(name)
    #define DEARTS_PROFILE_FUNCTION() DEARTS_PROFILE_SCOPE(__FUNCTION__)
#else
    #define DEARTS_PROFILE_SCOPE(name) ((void)0)
    #define DEARTS_PROFILE_FUNCTION() ((void)0)
#endif

// 标准库头文件
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <functional>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <cstdint>
#include <cstring>
#include <cmath>

// 包含工具模块
#include "utils/logger.h"
#include "utils/config_manager.h"
#include "utils/file_utils.h"
#include "utils/string_utils.h"
#include "utils/profiler.h"

// 事件系统
#include "events/event_system.h"

// 应用程序管理
#include "app/application_manager.h"

// 窗口管理
#include "window/window_manager.h"

// 渲染系统
#include "render/renderer.h"

// 输入系统
#include "input/input_manager.h"

// 资源管理
#include "resource/resource_manager.h"

// 音频系统
#include "audio/audio_manager.h"

/**
 * @namespace DearTs
 * @brief DearTs核心命名空间
 */
namespace DearTs {

/**
 * @brief 初始化DearTs核心系统
 * @param config 配置参数
 * @return 是否初始化成功
 */
DEARTS_API bool InitializeCore(const std::string& config = "");

/**
 * @brief 关闭DearTs核心系统
 */
DEARTS_API void ShutdownCore();

/**
 * @brief 获取DearTs版本字符串
 * @return 版本字符串
 */
DEARTS_API const char* GetVersion();

/**
 * @brief 获取DearTs构建信息
 * @return 构建信息字符串
 */
DEARTS_API const char* GetBuildInfo();

/**
 * @brief 检查核心系统是否已初始化
 * @return 是否已初始化
 */
DEARTS_API bool IsInitialized();

} // namespace DearTs

// 现代C++便捷函数 - 替代宏定义
namespace DearTs {
    namespace Core {
        /**
         * @brief 初始化核心系统
         * @param config 配置字符串
         * @return 是否成功初始化
         */
        inline bool init(const std::string& config = "") {
            return InitializeCore(config);
        }
        
        /**
         * @brief 关闭核心系统
         */
        inline void shutdown() {
            ShutdownCore();
        }
        
        /**
         * @brief 获取版本信息
         * @return 版本字符串
         */
        inline const char* version() {
            return GetVersion();
        }
        
        /**
         * @brief 获取构建信息
         * @return 构建信息字符串
         */
        inline const char* buildInfo() {
            return GetBuildInfo();
        }
        
        /**
         * @brief 检查是否已初始化
         * @return 是否已初始化
         */
        inline bool isInitialized() {
            return IsInitialized();
        }
        
        // 管理器访问函数 - 替代宏定义
        namespace Managers {
            /**
             * @brief 获取应用程序管理器实例
             */
            inline auto& app() {
                return App::ApplicationManager::getInstance();
            }
            
            /**
             * @brief 获取窗口管理器实例
             */
            inline auto& window() {
                return DearTs::Core::Window::WindowManager::getInstance();
            }
            
            /**
             * @brief 获取渲染管理器实例
             */
            inline auto& render() {
                return Core::Render::RenderManager::getInstance();
            }
            
            /**
             * @brief 获取输入管理器实例
             */
            inline auto& input() {
                return Core::Input::InputManager::getInstance();
            }
            
            /**
             * @brief 获取资源管理器实例
             */
            inline Core::Resource::ResourceManager* resource() {
                return Core::Resource::ResourceManager::getInstance();
            }
            
            /**
             * @brief 获取音频管理器实例
             */
            inline Core::Audio::AudioManager& audio() {
                return Core::Audio::AudioManager::getInstance();
            }
            
            /**
             * @brief 获取事件系统实例
             */
            inline Core::Events::EventSystem* events() {
                return Core::Events::EventSystem::getInstance();
            }
            
            /**
             * @brief 获取配置管理器实例
             */
            inline Core::Utils::ConfigManager& config() {
                return Core::Utils::ConfigManager::getInstance();
            }
            
            /**
             * @brief 获取性能分析器实例
             */
            inline Core::Utils::Profiler* profiler() {
                return &Core::Utils::Profiler::getInstance();
            }
        }
    }
}
