#pragma once

#include <cstdint>
#include <cstddef>
#include <climits>
#include <string>
#include <memory>
#include <functional>
#include <vector>
#include <map>
#include <optional>
#include <stdexcept>

// 基础类型定义
using u8  = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using i8  = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using color_t = u32;

// 前向声明
struct ImVec2;
struct ImGuiContext;
struct SDL_Window;

namespace dearts {
    
    /**
     * @brief 区域结构，表示内存或数据的一个区间
     */
    struct Region {
        u64 address = 0;
        size_t size = 0;
        
        Region() = default;
        Region(u64 addr, size_t sz) : address(addr), size(sz) {}
        
        [[nodiscard]] bool isValid() const { return size > 0; }
        [[nodiscard]] bool contains(u64 addr) const { 
            return addr >= address && addr < address + size; 
        }
        
        static const Region Invalid;
    };
    
    /**
     * @brief 非空指针模板，确保指针不为空
     */
    template<typename T>
    class NonNull {
    public:
        explicit NonNull(T ptr) : m_ptr(ptr) {
            if (!ptr) throw std::invalid_argument("Pointer cannot be null");
        }
        
        T operator->() const { return m_ptr; }
        T get() const { return m_ptr; }
        operator T() const { return m_ptr; }
        
    private:
        T m_ptr;
    };
    
    /**
     * @brief 未本地化字符串，用于国际化支持
     */
    class UnlocalizedString {
    public:
        explicit UnlocalizedString(std::string str) : m_string(std::move(str)) {}
        
        [[nodiscard]] const std::string& get() const { return m_string; }
        operator const std::string&() const { return m_string; }
        
    private:
        std::string m_string;
    };
    
}