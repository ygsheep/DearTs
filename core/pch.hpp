// DearTs Framework - Precompiled Header
// 此文件包含常用的系统和第三方库头文件，用于加速编译

#ifndef DEARTS_PCH_HPP
#define DEARTS_PCH_HPP

// ==================== C++ 标准库 ====================
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <codecvt>
#include <condition_variable>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <deque>
#include <exception>
#include <filesystem>
#include <format>
#include <forward_list>
#include <fstream>
#include <functional>
#include <future>
#include <iomanip>
#include <ios>
#include <iostream>
#include <istream>
#include <iterator>
#include <limits>
#include <list>
#include <locale>
#include <map>
#include <memory>
#include <mutex>
#include <new>
#include <numeric>
#include <optional>
#include <ostream>
#include <queue>
#include <random>
#include <ratio>
#include <regex>
#include <scoped_allocator>
#include <set>
#include <shared_mutex>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <streambuf>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

// ==================== Windows 特定头文件 ====================
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <comdef.h>
#include <shlobj.h>
#endif

// ==================== 第三方库 ====================

// nlohmann/json
#include <nlohmann/json.hpp>

// fmtlib (如果使用)
#ifdef FMT_VERSION
#include <fmt/format.h>
#include <fmt/chrono.h>
#include <fmt/std.h>
#endif

// ==================== ImGui ====================
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <imgui_internal.h>

// ==================== DearTs Framework 核心头文件 ====================
// 注意：这些可以根据实际需要调整，避免循环依赖

// ConfigManager
#include "core/config/config_manager.h"

// Plugin System
#include "core/plugin/plugin.h"

// EventBus
#include "core/events/event_bus.h"

// TaskManager
#include "core/tasks/task_manager.h"

// Logger
#include "liblogger/logger.h"

// ==================== 常用宏定义 ====================
#define DEARTS_VERSION_MAJOR 1
#define DEARTS_VERSION_MINOR 0
#define DEARTS_VERSION_PATCH 0

// 调试宏
#ifdef _DEBUG
    #define DEARTS_DEBUG 1
#else
    #define DEARTS_DEBUG 0
#endif

// 平台检测
#if defined(_WIN32) || defined(_WIN64)
    #define DEARTS_PLATFORM_WINDOWS 1
#elif defined(__APPLE__)
    #define DEARTS_PLATFORM_APPLE 1
#elif defined(__linux__)
    #define DEARTS_PLATFORM_LINUX 1
#endif

// 编译器检测
#if defined(_MSC_VER)
    #define DEARTS_COMPILER_MSVC 1
#elif defined(__GNUC__)
    #define DEARTS_COMPILER_GCC 1
#elif defined(__clang__)
    #define DEARTS_COMPILER_CLANG 1
#endif

// 导出/导入宏（Windows DLL）
#ifdef DEARTS_PLATFORM_WINDOWS
    #ifdef DEARTS_EXPORTS
        #define DEARTS_API __declspec(dllexport)
    #else
        #define DEARTS_API __declspec(dllimport)
    #endif
#else
    #define DEARTS_API
#endif

// 便捷宏
#define DEARTS_UNUSED(x) (void)(x)
#define DEARTS_STRINGIFY(x) #x
#define DEARTS_CONCAT(a, b) a##b

// 智能指针别名
template<typename T>
using UniquePtr = std::unique_ptr<T>;

template<typename T>
using SharedPtr = std::shared_ptr<T>;

template<typename T>
using WeakPtr = std::weak_ptr<T>;

// 容器别名
template<typename T>
using Vector = std::vector<T>;

template<typename T>
using List = std::list<T>;

template<typename T>
using Deque = std::deque<T>;

template<typename K, typename V>
using Map = std::map<K, V>;

template<typename K, typename V>
using UnorderedMap = std::unordered_map<K, V>;

template<typename T>
using Set = std::set<T>;

template<typename T>
using UnorderedSet = std::unordered_set<T>;

// 字符串别名
using String = std::string;
using StringView = std::string_view;
using WString = std::wstring;

// 文件系统别名
namespace fs = std::filesystem;

// 时间别名
namespace chrono = std::chrono;

// ==================== 单例模式宏（未来可能使用）====================
// 注意：当前项目不使用此宏，仅作为参考保留
// 理由：
//   1. 手写单例更清晰，便于调试
//   2. 避免宏展开影响 IDE 代码提示
//   3. 当前项目单例数量不多，手动维护成本可接受
//
// 如果未来单例数量大量增加，可以考虑启用以下宏：
//

#if 0  // 暂时禁用，未来可能使用

/// @brief 声明单例模式的类
/// @param ClassName 类名
/// @usage 在类定义中使用：class MyClass { DECLARE_SINGLETON(MyClass) ... };
#define DEARTS_DECLARE_SINGLETON(ClassName) \
public: \
    static ClassName& instance() noexcept { \
        static ClassName inst; \
        return inst; \
    } \
    ClassName(const ClassName&) = delete; \
    ClassName& operator=(const ClassName&) = delete; \
    ClassName(ClassName&&) = delete; \
    ClassName& operator=(ClassName&&) = delete; \
private: \
    ClassName() = default; \
    ~ClassName() = default;

/// @brief 单例模式基类（CRTP）
/// @warning 当前项目不使用，仅作为参考保留
/// @usage class MyManager : public DearTs::Singleton<MyManager> { ... };
template<typename T>
class Singleton {
public:
    static T& instance() noexcept {
        static T inst;
        return inst;
    }

    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    Singleton(Singleton&&) = delete;
    Singleton& operator=(Singleton&&) = delete;

protected:
    Singleton() = default;
    ~Singleton() = default;
};

#endif  // 暂时禁用单例宏

#endif // DEARTS_PCH_HPP
