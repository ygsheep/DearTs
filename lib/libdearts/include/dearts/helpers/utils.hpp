#pragma once

#include <dearts/dearts.hpp>

#include <string>
#include <vector>
#include <span>
#include <functional>
#include <filesystem>
#include <chrono>
#include <optional>
#include <type_traits>

namespace dearts {
    
    /**
     * @brief 实用工具函数命名空间
     */
    namespace utils {
        
        /**
         * @brief 字符串工具
         */
        namespace string {
            
            /**
             * @brief 转换为小写
             * @param str 输入字符串
             * @return 小写字符串
             */
            std::string toLower(const std::string &str);
            
            /**
             * @brief 转换为大写
             * @param str 输入字符串
             * @return 大写字符串
             */
            std::string toUpper(const std::string &str);
            
            /**
             * @brief 去除首尾空白字符
             * @param str 输入字符串
             * @return 去除空白后的字符串
             */
            std::string trim(const std::string &str);
            
            /**
             * @brief 分割字符串
             * @param str 输入字符串
             * @param delimiter 分隔符
             * @return 分割后的字符串列表
             */
            std::vector<std::string> split(const std::string &str, const std::string &delimiter);
            
            /**
             * @brief 连接字符串
             * @param strings 字符串列表
             * @param delimiter 分隔符
             * @return 连接后的字符串
             */
            std::string join(const std::vector<std::string> &strings, const std::string &delimiter);
            
            /**
             * @brief 替换字符串
             * @param str 输入字符串
             * @param from 要替换的子串
             * @param to 替换为的子串
             * @return 替换后的字符串
             */
            std::string replace(const std::string &str, const std::string &from, const std::string &to);
            
            /**
             * @brief 检查字符串是否以指定前缀开始
             * @param str 输入字符串
             * @param prefix 前缀
             * @return 是否以前缀开始
             */
            bool startsWith(const std::string &str, const std::string &prefix);
            
            /**
             * @brief 检查字符串是否以指定后缀结束
             * @param str 输入字符串
             * @param suffix 后缀
             * @return 是否以后缀结束
             */
            bool endsWith(const std::string &str, const std::string &suffix);
            
            /**
             * @brief 检查字符串是否包含指定子串
             * @param str 输入字符串
             * @param substr 子串
             * @return 是否包含子串
             */
            bool contains(const std::string &str, const std::string &substr);
            
            /**
             * @brief 格式化字符串
             * @tparam Args 参数类型
             * @param format 格式字符串
             * @param args 格式参数
             * @return 格式化后的字符串
             */
            template<typename... Args>
            std::string format(const std::string &format, Args &&...args) {
                size_t size = std::snprintf(nullptr, 0, format.c_str(), args...) + 1;
                std::unique_ptr<char[]> buf(new char[size]);
                std::snprintf(buf.get(), size, format.c_str(), args...);
                return std::string(buf.get(), buf.get() + size - 1);
            }
            
        }
        
        /**
         * @brief 文件系统工具
         */
        namespace fs {
            
            /**
             * @brief 读取文件内容
             * @param path 文件路径
             * @return 文件内容，失败返回nullopt
             */
            std::optional<std::vector<u8>> readFile(const std::filesystem::path &path);
            
            /**
             * @brief 写入文件内容
             * @param path 文件路径
             * @param data 文件数据
             * @return 是否成功
             */
            bool writeFile(const std::filesystem::path &path, const std::span<const u8> &data);
            
            /**
             * @brief 读取文本文件
             * @param path 文件路径
             * @return 文件内容，失败返回nullopt
             */
            std::optional<std::string> readTextFile(const std::filesystem::path &path);
            
            /**
             * @brief 写入文本文件
             * @param path 文件路径
             * @param content 文件内容
             * @return 是否成功
             */
            bool writeTextFile(const std::filesystem::path &path, const std::string &content);
            
            /**
             * @brief 获取文件大小
             * @param path 文件路径
             * @return 文件大小，失败返回nullopt
             */
            std::optional<size_t> getFileSize(const std::filesystem::path &path);
            
            /**
             * @brief 检查文件是否存在
             * @param path 文件路径
             * @return 是否存在
             */
            bool exists(const std::filesystem::path &path);
            
            /**
             * @brief 创建目录
             * @param path 目录路径
             * @return 是否成功
             */
            bool createDirectories(const std::filesystem::path &path);
            
            /**
             * @brief 删除文件或目录
             * @param path 路径
             * @return 是否成功
             */
            bool remove(const std::filesystem::path &path);
            
            /**
             * @brief 复制文件
             * @param from 源路径
             * @param to 目标路径
             * @return 是否成功
             */
            bool copy(const std::filesystem::path &from, const std::filesystem::path &to);
            
            /**
             * @brief 移动文件
             * @param from 源路径
             * @param to 目标路径
             * @return 是否成功
             */
            bool move(const std::filesystem::path &from, const std::filesystem::path &to);
            
            /**
             * @brief 获取文件扩展名
             * @param path 文件路径
             * @return 文件扩展名
             */
            std::string getExtension(const std::filesystem::path &path);
            
            /**
             * @brief 获取文件名（不含扩展名）
             * @param path 文件路径
             * @return 文件名
             */
            std::string getStem(const std::filesystem::path &path);
            
            /**
             * @brief 获取父目录路径
             * @param path 文件路径
             * @return 父目录路径
             */
            std::filesystem::path getParentPath(const std::filesystem::path &path);
            
        }
        
        /**
         * @brief 数学工具
         */
        namespace math {
            
            /**
             * @brief 限制值在指定范围内
             * @tparam T 数值类型
             * @param value 输入值
             * @param min 最小值
             * @param max 最大值
             * @return 限制后的值
             */
            template<typename T>
            constexpr T clamp(T value, T min, T max) {
                return (value < min) ? min : (value > max) ? max : value;
            }
            
            /**
             * @brief 线性插值
             * @tparam T 数值类型
             * @param a 起始值
             * @param b 结束值
             * @param t 插值参数 [0, 1]
             * @return 插值结果
             */
            template<typename T>
            constexpr T lerp(T a, T b, float t) {
                return a + t * (b - a);
            }
            
            /**
             * @brief 获取最小值
             * @tparam T 数值类型
             * @param a 值A
             * @param b 值B
             * @return 最小值
             */
            template<typename T>
            constexpr T min(T a, T b) {
                return (a < b) ? a : b;
            }
            
            /**
             * @brief 获取最大值
             * @tparam T 数值类型
             * @param a 值A
             * @param b 值B
             * @return 最大值
             */
            template<typename T>
            constexpr T max(T a, T b) {
                return (a > b) ? a : b;
            }
            
            /**
             * @brief 获取绝对值
             * @tparam T 数值类型
             * @param value 输入值
             * @return 绝对值
             */
            template<typename T>
            constexpr T abs(T value) {
                return (value < 0) ? -value : value;
            }
            
            /**
             * @brief 四舍五入到最近的整数
             * @param value 输入值
             * @return 四舍五入后的整数
             */
            int round(float value);
            
            /**
             * @brief 向下取整
             * @param value 输入值
             * @return 向下取整后的整数
             */
            int floor(float value);
            
            /**
             * @brief 向上取整
             * @param value 输入值
             * @return 向上取整后的整数
             */
            int ceil(float value);
            
        }
        
        /**
         * @brief 时间工具
         */
        namespace time {
            
            /**
             * @brief 获取当前时间戳（毫秒）
             * @return 时间戳
             */
            u64 getCurrentTimeMillis();
            
            /**
             * @brief 获取当前时间戳（微秒）
             * @return 时间戳
             */
            u64 getCurrentTimeMicros();
            
            /**
             * @brief 格式化时间
             * @param timePoint 时间点
             * @param format 格式字符串
             * @return 格式化后的时间字符串
             */
            std::string formatTime(const std::chrono::system_clock::time_point &timePoint, const std::string &format = "%Y-%m-%d %H:%M:%S");
            
            /**
             * @brief 获取当前格式化时间
             * @param format 格式字符串
             * @return 格式化后的时间字符串
             */
            std::string getCurrentTimeString(const std::string &format = "%Y-%m-%d %H:%M:%S");
            
            /**
             * @brief 计时器类
             */
            class Timer {
            public:
                /**
                 * @brief 构造函数
                 */
                Timer();
                
                /**
                 * @brief 重置计时器
                 */
                void reset();
                
                /**
                 * @brief 获取经过的时间（毫秒）
                 * @return 经过的时间
                 */
                u64 getElapsedMillis() const;
                
                /**
                 * @brief 获取经过的时间（微秒）
                 * @return 经过的时间
                 */
                u64 getElapsedMicros() const;
                
                /**
                 * @brief 获取经过的时间（秒）
                 * @return 经过的时间
                 */
                double getElapsedSeconds() const;
                
            private:
                std::chrono::high_resolution_clock::time_point m_startTime;
            };
            
        }
        
        /**
         * @brief 内存工具
         */
        namespace memory {
            
            /**
             * @brief 格式化字节大小
             * @param bytes 字节数
             * @return 格式化后的字符串
             */
            std::string formatByteSize(u64 bytes);
            
            /**
             * @brief 安全内存复制
             * @param dest 目标地址
             * @param src 源地址
             * @param size 复制大小
             * @return 是否成功
             */
            bool safeCopy(void *dest, const void *src, size_t size);
            
            /**
             * @brief 安全内存设置
             * @param dest 目标地址
             * @param value 设置值
             * @param size 设置大小
             * @return 是否成功
             */
            bool safeSet(void *dest, int value, size_t size);
            
            /**
             * @brief 内存比较
             * @param ptr1 指针1
             * @param ptr2 指针2
             * @param size 比较大小
             * @return 比较结果
             */
            int compare(const void *ptr1, const void *ptr2, size_t size);
            
        }
        
        /**
         * @brief 随机数工具
         */
        namespace random {
            
            /**
             * @brief 生成随机整数
             * @param min 最小值
             * @param max 最大值
             * @return 随机整数
             */
            int randomInt(int min, int max);
            
            /**
             * @brief 生成随机浮点数
             * @param min 最小值
             * @param max 最大值
             * @return 随机浮点数
             */
            float randomFloat(float min = 0.0f, float max = 1.0f);
            
            /**
             * @brief 生成随机布尔值
             * @return 随机布尔值
             */
            bool randomBool();
            
            /**
             * @brief 生成随机字符串
             * @param length 字符串长度
             * @param charset 字符集
             * @return 随机字符串
             */
            std::string randomString(size_t length, const std::string &charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
            
        }
        
    }
    
}