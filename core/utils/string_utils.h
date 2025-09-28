/**
 * DearTs String Utilities Header
 * 
 * 字符串工具类头文件 - 提供字符串处理功能
 * 
 * @author DearTs Team
 * @version 1.0.0
 * @date 2025
 */

#pragma once

// 直接包含必要的标准库头文件以避免预编译头文件问题
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <regex>
#include <locale>
#include <codecvt>
#include <iomanip>
#include <random>
#include <unordered_map>
#include <functional>

namespace DearTs {
namespace Core {
namespace Utils {

/**
 * @brief 字符串比较选项
 */
enum class StringCompareOptions {
    NONE = 0,
    IGNORE_CASE = 1 << 0,
    IGNORE_WHITESPACE = 1 << 1,
    IGNORE_ACCENTS = 1 << 2,
    NATURAL_ORDER = 1 << 3  // 自然排序(数字部分按数值比较)
};

/**
 * @brief 字符串格式化选项
 */
struct StringFormatOptions {
    int width = 0;              // 字段宽度
    int precision = -1;         // 精度
    char fill_char = ' ';       // 填充字符
    bool left_align = false;    // 左对齐
    bool show_positive = false; // 显示正号
    bool uppercase = false;     // 大写
    int base = 10;             // 进制(2, 8, 10, 16)
};

/**
 * @brief 字符串分割选项
 */
struct StringSplitOptions {
    bool remove_empty = true;           // 移除空字符串
    bool trim_whitespace = true;        // 去除空白字符
    size_t max_splits = SIZE_MAX;       // 最大分割数
    bool case_sensitive = true;         // 大小写敏感
    bool use_regex = false;             // 使用正则表达式
};

/**
 * @brief 字符串替换选项
 */
struct StringReplaceOptions {
    bool case_sensitive = true;         // 大小写敏感
    bool replace_all = true;            // 替换所有匹配
    bool use_regex = false;             // 使用正则表达式
    size_t max_replacements = SIZE_MAX; // 最大替换次数
};

/**
 * @brief 字符串编码类型
 */
enum class StringEncoding {
    UTF8,
    UTF16,
    UTF32,
    ASCII,
    LATIN1,
    GBK,
    BIG5
};

/**
 * @brief 字符串哈希选项
 */
enum class StringHashAlgorithm {
    FNV1A,      // FNV-1a hash
    MURMUR3,    // MurmurHash3
    CRC32,      // CRC32
    MD5,        // MD5 (需要外部库)
    SHA1,       // SHA1 (需要外部库)
    SHA256      // SHA256 (需要外部库)
};

/**
 * @brief 字符串工具类
 * 
 * 提供各种字符串处理功能，包括格式化、分割、替换、
 * 编码转换、模式匹配等
 */
class StringUtils {
public:
    // ========================================================================
    // 基本字符串操作
    // ========================================================================
    
    /**
     * @brief 去除字符串两端的空白字符
     * @param str 输入字符串
     * @param chars 要去除的字符集(默认为空白字符)
     * @return 处理后的字符串
     */
    static std::string trim(const std::string& str, const std::string& chars = " \t\n\r\f\v");
    
    /**
     * @brief 去除字符串左端的空白字符
     * @param str 输入字符串
     * @param chars 要去除的字符集
     * @return 处理后的字符串
     */
    static std::string trimLeft(const std::string& str, const std::string& chars = " \t\n\r\f\v");
    
    /**
     * @brief 去除字符串右端的空白字符
     * @param str 输入字符串
     * @param chars 要去除的字符集
     * @return 处理后的字符串
     */
    static std::string trimRight(const std::string& str, const std::string& chars = " \t\n\r\f\v");
    
    /**
     * @brief 转换为小写
     * @param str 输入字符串
     * @return 小写字符串
     */
    static std::string toLower(const std::string& str);
    
    /**
     * @brief 转换为大写
     * @param str 输入字符串
     * @return 大写字符串
     */
    static std::string toUpper(const std::string& str);
    
    /**
     * @brief 转换为标题格式(首字母大写)
     * @param str 输入字符串
     * @return 标题格式字符串
     */
    static std::string toTitle(const std::string& str);
    
    /**
     * @brief 反转字符串
     * @param str 输入字符串
     * @return 反转后的字符串
     */
    static std::string reverse(const std::string& str);
    
    /**
     * @brief 重复字符串
     * @param str 输入字符串
     * @param count 重复次数
     * @return 重复后的字符串
     */
    static std::string repeat(const std::string& str, size_t count);
    
    /**
     * @brief 填充字符串到指定长度
     * @param str 输入字符串
     * @param width 目标宽度
     * @param fill_char 填充字符
     * @param left_align 是否左对齐
     * @return 填充后的字符串
     */
    static std::string pad(const std::string& str, size_t width, char fill_char = ' ', bool left_align = false);
    
    // ========================================================================
    // 字符串比较和搜索
    // ========================================================================
    
    /**
     * @brief 比较字符串
     * @param str1 字符串1
     * @param str2 字符串2
     * @param options 比较选项
     * @return 比较结果(-1, 0, 1)
     */
    static int compare(const std::string& str1, const std::string& str2, StringCompareOptions options = StringCompareOptions::NONE);
    
    /**
     * @brief 检查字符串是否相等
     * @param str1 字符串1
     * @param str2 字符串2
     * @param options 比较选项
     * @return 是否相等
     */
    static bool equals(const std::string& str1, const std::string& str2, StringCompareOptions options = StringCompareOptions::NONE);
    
    /**
     * @brief 检查字符串是否以指定前缀开始
     * @param str 输入字符串
     * @param prefix 前缀
     * @param ignore_case 是否忽略大小写
     * @return 是否以前缀开始
     */
    static bool startsWith(const std::string& str, const std::string& prefix, bool ignore_case = false);
    
    /**
     * @brief 检查字符串是否以指定后缀结束
     * @param str 输入字符串
     * @param suffix 后缀
     * @param ignore_case 是否忽略大小写
     * @return 是否以后缀结束
     */
    static bool endsWith(const std::string& str, const std::string& suffix, bool ignore_case = false);
    
    /**
     * @brief 检查字符串是否包含子字符串
     * @param str 输入字符串
     * @param substr 子字符串
     * @param ignore_case 是否忽略大小写
     * @return 是否包含
     */
    static bool contains(const std::string& str, const std::string& substr, bool ignore_case = false);
    
    /**
     * @brief 查找子字符串位置
     * @param str 输入字符串
     * @param substr 子字符串
     * @param start_pos 开始位置
     * @param ignore_case 是否忽略大小写
     * @return 位置索引(未找到返回string::npos)
     */
    static size_t find(const std::string& str, const std::string& substr, size_t start_pos = 0, bool ignore_case = false);
    
    /**
     * @brief 从右侧查找子字符串位置
     * @param str 输入字符串
     * @param substr 子字符串
     * @param start_pos 开始位置
     * @param ignore_case 是否忽略大小写
     * @return 位置索引(未找到返回string::npos)
     */
    static size_t findLast(const std::string& str, const std::string& substr, size_t start_pos = std::string::npos, bool ignore_case = false);
    
    /**
     * @brief 查找所有匹配位置
     * @param str 输入字符串
     * @param substr 子字符串
     * @param ignore_case 是否忽略大小写
     * @return 位置列表
     */
    static std::vector<size_t> findAll(const std::string& str, const std::string& substr, bool ignore_case = false);
    
    /**
     * @brief 计算子字符串出现次数
     * @param str 输入字符串
     * @param substr 子字符串
     * @param ignore_case 是否忽略大小写
     * @return 出现次数
     */
    static size_t count(const std::string& str, const std::string& substr, bool ignore_case = false);
    
    // ========================================================================
    // 字符串分割和连接
    // ========================================================================
    
    /**
     * @brief 分割字符串
     * @param str 输入字符串
     * @param delimiter 分隔符
     * @param options 分割选项
     * @return 分割后的字符串列表
     */
    static std::vector<std::string> split(const std::string& str, const std::string& delimiter, const StringSplitOptions& options = {});
    
    /**
     * @brief 按字符分割字符串
     * @param str 输入字符串
     * @param delimiters 分隔符字符集
     * @param options 分割选项
     * @return 分割后的字符串列表
     */
    static std::vector<std::string> splitByChars(const std::string& str, const std::string& delimiters, const StringSplitOptions& options = {});
    
    /**
     * @brief 按行分割字符串
     * @param str 输入字符串
     * @param keep_empty_lines 是否保留空行
     * @return 行列表
     */
    static std::vector<std::string> splitLines(const std::string& str, bool keep_empty_lines = false);
    
    /**
     * @brief 连接字符串列表
     * @param strings 字符串列表
     * @param delimiter 分隔符
     * @return 连接后的字符串
     */
    static std::string join(const std::vector<std::string>& strings, const std::string& delimiter);
    
    /**
     * @brief 连接字符串列表(带格式化)
     * @param strings 字符串列表
     * @param delimiter 分隔符
     * @param prefix 前缀
     * @param suffix 后缀
     * @return 连接后的字符串
     */
    static std::string joinFormatted(const std::vector<std::string>& strings, const std::string& delimiter, const std::string& prefix = "", const std::string& suffix = "");
    
    // ========================================================================
    // 字符串替换
    // ========================================================================
    
    /**
     * @brief 替换字符串
     * @param str 输入字符串
     * @param search 搜索字符串
     * @param replacement 替换字符串
     * @param options 替换选项
     * @return 替换后的字符串
     */
    static std::string replace(const std::string& str, const std::string& search, const std::string& replacement, const StringReplaceOptions& options = {});
    
    /**
     * @brief 批量替换字符串
     * @param str 输入字符串
     * @param replacements 替换映射表
     * @param case_sensitive 是否大小写敏感
     * @return 替换后的字符串
     */
    static std::string replaceMultiple(const std::string& str, const std::unordered_map<std::string, std::string>& replacements, bool case_sensitive = true);
    
    /**
     * @brief 替换字符
     * @param str 输入字符串
     * @param old_char 旧字符
     * @param new_char 新字符
     * @return 替换后的字符串
     */
    static std::string replaceChar(const std::string& str, char old_char, char new_char);
    
    /**
     * @brief 移除字符串中的指定字符
     * @param str 输入字符串
     * @param chars 要移除的字符集
     * @return 处理后的字符串
     */
    static std::string removeChars(const std::string& str, const std::string& chars);
    
    /**
     * @brief 保留字符串中的指定字符
     * @param str 输入字符串
     * @param chars 要保留的字符集
     * @return 处理后的字符串
     */
    static std::string keepChars(const std::string& str, const std::string& chars);
    
    // ========================================================================
    // 字符串格式化
    // ========================================================================
    
    /**
     * @brief 格式化字符串(类似printf)
     * @param format 格式字符串
     * @param args 参数
     * @return 格式化后的字符串
     */
    template<typename... Args>
    static std::string format(const std::string& format, Args&&... args);
    
    /**
     * @brief 格式化数字
     * @param value 数值
     * @param options 格式化选项
     * @return 格式化后的字符串
     */
    template<typename T>
    static std::string formatNumber(T value, const StringFormatOptions& options = {});
    
    /**
     * @brief 格式化浮点数
     * @param value 浮点数
     * @param precision 精度
     * @param fixed 是否固定精度
     * @return 格式化后的字符串
     */
    static std::string formatFloat(double value, int precision = 2, bool fixed = true);
    
    /**
     * @brief 格式化百分比
     * @param value 数值(0.0-1.0)
     * @param precision 精度
     * @return 格式化后的字符串
     */
    static std::string formatPercent(double value, int precision = 1);
    
    /**
     * @brief 格式化文件大小
     * @param bytes 字节数
     * @param binary 是否使用二进制单位(1024)
     * @return 格式化后的字符串
     */
    static std::string formatFileSize(size_t bytes, bool binary = true);
    
    /**
     * @brief 格式化时间间隔
     * @param seconds 秒数
     * @param show_milliseconds 是否显示毫秒
     * @return 格式化后的字符串
     */
    static std::string formatDuration(double seconds, bool show_milliseconds = false);
    
    // ========================================================================
    // 字符串验证
    // ========================================================================
    
    /**
     * @brief 检查是否为空或只包含空白字符
     * @param str 输入字符串
     * @return 是否为空白
     */
    static bool isBlank(const std::string& str);
    
    /**
     * @brief 检查是否为数字
     * @param str 输入字符串
     * @param allow_decimal 是否允许小数点
     * @param allow_negative 是否允许负数
     * @return 是否为数字
     */
    static bool isNumeric(const std::string& str, bool allow_decimal = true, bool allow_negative = true);
    
    /**
     * @brief 检查是否为整数
     * @param str 输入字符串
     * @param allow_negative 是否允许负数
     * @return 是否为整数
     */
    static bool isInteger(const std::string& str, bool allow_negative = true);
    
    /**
     * @brief 检查是否为浮点数
     * @param str 输入字符串
     * @param allow_negative 是否允许负数
     * @return 是否为浮点数
     */
    static bool isFloat(const std::string& str, bool allow_negative = true);
    
    /**
     * @brief 检查是否为字母
     * @param str 输入字符串
     * @return 是否为字母
     */
    static bool isAlpha(const std::string& str);
    
    /**
     * @brief 检查是否为字母数字
     * @param str 输入字符串
     * @return 是否为字母数字
     */
    static bool isAlphaNumeric(const std::string& str);
    
    /**
     * @brief 检查是否为有效的邮箱地址
     * @param str 输入字符串
     * @return 是否为有效邮箱
     */
    static bool isValidEmail(const std::string& str);
    
    /**
     * @brief 检查是否为有效的URL
     * @param str 输入字符串
     * @return 是否为有效URL
     */
    static bool isValidUrl(const std::string& str);
    
    /**
     * @brief 检查是否为有效的IP地址
     * @param str 输入字符串
     * @param ipv6 是否检查IPv6
     * @return 是否为有效IP
     */
    static bool isValidIpAddress(const std::string& str, bool ipv6 = false);
    
    // ========================================================================
    // 字符串转换
    // ========================================================================
    
    /**
     * @brief 字符串转整数
     * @param str 输入字符串
     * @param default_value 默认值
     * @param base 进制
     * @return 整数值
     */
    static int toInt(const std::string& str, int default_value = 0, int base = 10);
    
    /**
     * @brief 字符串转长整数
     * @param str 输入字符串
     * @param default_value 默认值
     * @param base 进制
     * @return 长整数值
     */
    static long long toLong(const std::string& str, long long default_value = 0, int base = 10);
    
    /**
     * @brief 字符串转浮点数
     * @param str 输入字符串
     * @param default_value 默认值
     * @return 浮点数值
     */
    static float toFloat(const std::string& str, float default_value = 0.0f);
    
    /**
     * @brief 字符串转双精度浮点数
     * @param str 输入字符串
     * @param default_value 默认值
     * @return 双精度浮点数值
     */
    static double toDouble(const std::string& str, double default_value = 0.0);
    
    /**
     * @brief 字符串转布尔值
     * @param str 输入字符串
     * @param default_value 默认值
     * @return 布尔值
     */
    static bool toBool(const std::string& str, bool default_value = false);
    
    /**
     * @brief 整数转字符串
     * @param value 整数值
     * @param base 进制
     * @param uppercase 是否大写(16进制)
     * @return 字符串
     */
    static std::string fromInt(int value, int base = 10, bool uppercase = false);
    
    /**
     * @brief 长整数转字符串
     * @param value 长整数值
     * @param base 进制
     * @param uppercase 是否大写(16进制)
     * @return 字符串
     */
    static std::string fromLong(long long value, int base = 10, bool uppercase = false);
    
    /**
     * @brief 浮点数转字符串
     * @param value 浮点数值
     * @param precision 精度
     * @param fixed 是否固定精度
     * @return 字符串
     */
    static std::string fromFloat(float value, int precision = 6, bool fixed = false);
    
    /**
     * @brief 双精度浮点数转字符串
     * @param value 双精度浮点数值
     * @param precision 精度
     * @param fixed 是否固定精度
     * @return 字符串
     */
    static std::string fromDouble(double value, int precision = 6, bool fixed = false);
    
    /**
     * @brief 布尔值转字符串
     * @param value 布尔值
     * @param true_str 真值字符串
     * @param false_str 假值字符串
     * @return 字符串
     */
    static std::string fromBool(bool value, const std::string& true_str = "true", const std::string& false_str = "false");
    
    // ========================================================================
    // 编码转换
    // ========================================================================
    
    /**
     * @brief UTF-8转UTF-16
     * @param utf8_str UTF-8字符串
     * @return UTF-16字符串
     */
    static std::u16string utf8ToUtf16(const std::string& utf8_str);
    
    /**
     * @brief UTF-16转UTF-8
     * @param utf16_str UTF-16字符串
     * @return UTF-8字符串
     */
    static std::string utf16ToUtf8(const std::u16string& utf16_str);
    
    /**
     * @brief UTF-8转UTF-32
     * @param utf8_str UTF-8字符串
     * @return UTF-32字符串
     */
    static std::u32string utf8ToUtf32(const std::string& utf8_str);
    
    /**
     * @brief UTF-32转UTF-8
     * @param utf32_str UTF-32字符串
     * @return UTF-8字符串
     */
    static std::string utf32ToUtf8(const std::u32string& utf32_str);
    
    /**
     * @brief 转换字符串编码
     * @param str 输入字符串
     * @param from_encoding 源编码
     * @param to_encoding 目标编码
     * @return 转换后的字符串
     */
    static std::string convertEncoding(const std::string& str, StringEncoding from_encoding, StringEncoding to_encoding);
    
    /**
     * @brief 检测字符串编码
     * @param str 输入字符串
     * @return 编码类型
     */
    static StringEncoding detectEncoding(const std::string& str);
    
    // ========================================================================
    // 正则表达式
    // ========================================================================
    
    /**
     * @brief 正则表达式匹配
     * @param str 输入字符串
     * @param pattern 正则表达式模式
     * @param ignore_case 是否忽略大小写
     * @return 是否匹配
     */
    static bool regexMatch(const std::string& str, const std::string& pattern, bool ignore_case = false);
    
    /**
     * @brief 正则表达式搜索
     * @param str 输入字符串
     * @param pattern 正则表达式模式
     * @param ignore_case 是否忽略大小写
     * @return 匹配结果列表
     */
    static std::vector<std::string> regexSearch(const std::string& str, const std::string& pattern, bool ignore_case = false);
    
    /**
     * @brief 正则表达式替换
     * @param str 输入字符串
     * @param pattern 正则表达式模式
     * @param replacement 替换字符串
     * @param replace_all 是否替换所有匹配
     * @param ignore_case 是否忽略大小写
     * @return 替换后的字符串
     */
    static std::string regexReplace(const std::string& str, const std::string& pattern, const std::string& replacement, bool replace_all = true, bool ignore_case = false);
    
    /**
     * @brief 正则表达式分割
     * @param str 输入字符串
     * @param pattern 正则表达式模式
     * @param ignore_case 是否忽略大小写
     * @return 分割后的字符串列表
     */
    static std::vector<std::string> regexSplit(const std::string& str, const std::string& pattern, bool ignore_case = false);
    
    // ========================================================================
    // 字符串哈希和校验
    // ========================================================================
    
    /**
     * @brief 计算字符串哈希值
     * @param str 输入字符串
     * @param algorithm 哈希算法
     * @return 哈希值
     */
    static uint64_t hash(const std::string& str, StringHashAlgorithm algorithm = StringHashAlgorithm::FNV1A);
    
    /**
     * @brief 计算字符串哈希值(十六进制字符串)
     * @param str 输入字符串
     * @param algorithm 哈希算法
     * @return 哈希值(十六进制字符串)
     */
    static std::string hashHex(const std::string& str, StringHashAlgorithm algorithm = StringHashAlgorithm::FNV1A);
    
    /**
     * @brief 计算字符串校验和
     * @param str 输入字符串
     * @return 校验和
     */
    static uint32_t checksum(const std::string& str);
    
    // ========================================================================
    // 字符串生成
    // ========================================================================
    
    /**
     * @brief 生成随机字符串
     * @param length 长度
     * @param charset 字符集
     * @return 随机字符串
     */
    static std::string generateRandom(size_t length, const std::string& charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
    
    /**
     * @brief 生成UUID字符串
     * @param use_hyphens 是否使用连字符
     * @param uppercase 是否大写
     * @return UUID字符串
     */
    static std::string generateUuid(bool use_hyphens = true, bool uppercase = false);
    
    /**
     * @brief 生成密码
     * @param length 长度
     * @param include_symbols 是否包含符号
     * @param exclude_ambiguous 是否排除易混淆字符
     * @return 密码字符串
     */
    static std::string generatePassword(size_t length, bool include_symbols = true, bool exclude_ambiguous = true);
    
    // ========================================================================
    // 字符串距离和相似度
    // ========================================================================
    
    /**
     * @brief 计算编辑距离(Levenshtein距离)
     * @param str1 字符串1
     * @param str2 字符串2
     * @return 编辑距离
     */
    static size_t editDistance(const std::string& str1, const std::string& str2);
    
    /**
     * @brief 计算字符串相似度(0.0-1.0)
     * @param str1 字符串1
     * @param str2 字符串2
     * @return 相似度
     */
    static double similarity(const std::string& str1, const std::string& str2);
    
    /**
     * @brief 计算Jaro-Winkler相似度
     * @param str1 字符串1
     * @param str2 字符串2
     * @return 相似度
     */
    static double jaroWinklerSimilarity(const std::string& str1, const std::string& str2);
    
    // ========================================================================
    // 实用工具函数
    // ========================================================================
    
    /**
     * @brief 转义HTML字符
     * @param str 输入字符串
     * @return 转义后的字符串
     */
    static std::string escapeHtml(const std::string& str);
    
    /**
     * @brief 反转义HTML字符
     * @param str 输入字符串
     * @return 反转义后的字符串
     */
    static std::string unescapeHtml(const std::string& str);
    
    /**
     * @brief 转义JSON字符
     * @param str 输入字符串
     * @return 转义后的字符串
     */
    static std::string escapeJson(const std::string& str);
    
    /**
     * @brief 反转义JSON字符
     * @param str 输入字符串
     * @return 反转义后的字符串
     */
    static std::string unescapeJson(const std::string& str);
    
    /**
     * @brief 转义XML字符
     * @param str 输入字符串
     * @return 转义后的字符串
     */
    static std::string escapeXml(const std::string& str);
    
    /**
     * @brief 反转义XML字符
     * @param str 输入字符串
     * @return 反转义后的字符串
     */
    static std::string unescapeXml(const std::string& str);
    
    /**
     * @brief 转义URL字符
     * @param str 输入字符串
     * @return 转义后的字符串
     */
    static std::string escapeUrl(const std::string& str);
    
    /**
     * @brief 反转义URL字符
     * @param str 输入字符串
     * @return 反转义后的字符串
     */
    static std::string unescapeUrl(const std::string& str);
    
    /**
     * @brief 单词换行
     * @param str 输入字符串
     * @param width 行宽
     * @param break_long_words 是否断开长单词
     * @return 换行后的字符串
     */
    static std::string wordWrap(const std::string& str, size_t width, bool break_long_words = true);
    
    /**
     * @brief 缩略字符串
     * @param str 输入字符串
     * @param max_length 最大长度
     * @param ellipsis 省略符
     * @return 缩略后的字符串
     */
    static std::string truncate(const std::string& str, size_t max_length, const std::string& ellipsis = "...");
    
    /**
     * @brief 智能缩略(保留重要部分)
     * @param str 输入字符串
     * @param max_length 最大长度
     * @param ellipsis 省略符
     * @return 缩略后的字符串
     */
    static std::string smartTruncate(const std::string& str, size_t max_length, const std::string& ellipsis = "...");
    
private:
    // 内部辅助函数
    static std::regex::flag_type getRegexFlags(bool ignore_case);
    static uint64_t fnv1aHash(const std::string& str);
    static uint64_t murmur3Hash(const std::string& str);
    static uint32_t crc32Hash(const std::string& str);
    static double jaroSimilarity(const std::string& str1, const std::string& str2);
    static std::string naturalSort(const std::string& str);
};

// ============================================================================
// 模板函数实现
// ============================================================================

/**
 * @brief 格式化字符串模板实现
 */
template<typename... Args>
std::string StringUtils::format(const std::string& format, Args&&... args) {
    // 使用snprintf计算所需缓冲区大小
    int size = std::snprintf(nullptr, 0, format.c_str(), args...) + 1;
    if (size <= 0) {
        return "";
    }
    
    std::unique_ptr<char[]> buffer(new char[size]);
    std::snprintf(buffer.get(), size, format.c_str(), args...);
    return std::string(buffer.get(), buffer.get() + size - 1);
}

/**
 * @brief 格式化数字模板实现
 */
template<typename T>
std::string StringUtils::formatNumber(T value, const StringFormatOptions& options) {
    std::ostringstream oss;
    
    // 设置填充字符
    oss.fill(options.fill_char);
    
    // 设置对齐方式
    if (options.left_align) {
        oss << std::left;
    } else {
        oss << std::right;
    }
    
    // 设置字段宽度
    if (options.width > 0) {
        oss << std::setw(options.width);
    }
    
    // 设置精度
    if (options.precision >= 0) {
        oss << std::setprecision(options.precision);
        if constexpr (std::is_floating_point_v<T>) {
            oss << std::fixed;
        }
    }
    
    // 设置进制
    if (options.base == 8) {
        oss << std::oct;
    } else if (options.base == 16) {
        oss << std::hex;
        if (options.uppercase) {
            oss << std::uppercase;
        }
    }
    
    // 设置正号显示
    if (options.show_positive) {
        oss << std::showpos;
    }
    
    // 设置大小写
    if (options.uppercase) {
        oss << std::uppercase;
    }
    
    oss << value;
    return oss.str();
}

// ============================================================================
// 操作符重载
// ============================================================================

/**
 * @brief StringCompareOptions按位或操作符
 */
inline StringCompareOptions operator|(StringCompareOptions lhs, StringCompareOptions rhs) {
    return static_cast<StringCompareOptions>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

/**
 * @brief StringCompareOptions按位与操作符
 */
inline StringCompareOptions operator&(StringCompareOptions lhs, StringCompareOptions rhs) {
    return static_cast<StringCompareOptions>(static_cast<int>(lhs) & static_cast<int>(rhs));
}

/**
 * @brief 检查StringCompareOptions是否包含指定标志
 */
inline bool hasFlag(StringCompareOptions options, StringCompareOptions flag) {
    return (options & flag) == flag;
}

} // namespace Utils
} // namespace Core
} // namespace DearTs

