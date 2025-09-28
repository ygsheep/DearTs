/**
 * DearTs String Utilities Implementation
 * 
 * 字符串工具类实现文件 - 提供字符串处理功能的具体实现
 * 
 * @author DearTs Team
 * @version 1.0.0
 * @date 2025
 */

#include "string_utils.h"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <random>
#include <chrono>
#include <regex>
#include <locale>
#include <codecvt>

#ifdef _WIN32
#include <windows.h>
#else
#include <iconv.h>
#endif

namespace DearTs {
namespace Core {
namespace Utils {

// ============================================================================
// 基本字符串操作实现
// ============================================================================

/**
 * @brief 去除字符串两端的指定字符
 */
std::string StringUtils::trim(const std::string& str, const std::string& chars) {
    size_t start = str.find_first_not_of(chars);
    if (start == std::string::npos) {
        return "";
    }
    
    size_t end = str.find_last_not_of(chars);
    return str.substr(start, end - start + 1);
}

/**
 * @brief 去除字符串左端的指定字符
 */
std::string StringUtils::trimLeft(const std::string& str, const std::string& chars) {
    size_t start = str.find_first_not_of(chars);
    if (start == std::string::npos) {
        return "";
    }
    return str.substr(start);
}

/**
 * @brief 去除字符串右端的指定字符
 */
std::string StringUtils::trimRight(const std::string& str, const std::string& chars) {
    size_t end = str.find_last_not_of(chars);
    if (end == std::string::npos) {
        return "";
    }
    return str.substr(0, end + 1);
}

/**
 * @brief 转换为小写
 */
std::string StringUtils::toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), 
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

/**
 * @brief 转换为大写
 */
std::string StringUtils::toUpper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), 
                   [](unsigned char c) { return std::toupper(c); });
    return result;
}

/**
 * @brief 转换为标题格式
 */
std::string StringUtils::toTitle(const std::string& str) {
    std::string result = str;
    bool capitalize_next = true;
    
    for (char& c : result) {
        if (std::isalpha(c)) {
            if (capitalize_next) {
                c = std::toupper(c);
                capitalize_next = false;
            } else {
                c = std::tolower(c);
            }
        } else {
            capitalize_next = true;
        }
    }
    
    return result;
}

/**
 * @brief 反转字符串
 */
std::string StringUtils::reverse(const std::string& str) {
    std::string result = str;
    std::reverse(result.begin(), result.end());
    return result;
}

/**
 * @brief 重复字符串
 */
std::string StringUtils::repeat(const std::string& str, size_t count) {
    if (count == 0 || str.empty()) {
        return "";
    }
    
    std::string result;
    result.reserve(str.length() * count);
    
    for (size_t i = 0; i < count; ++i) {
        result += str;
    }
    
    return result;
}

/**
 * @brief 填充字符串到指定长度
 */
std::string StringUtils::pad(const std::string& str, size_t width, char fill_char, bool left_align) {
    if (str.length() >= width) {
        return str;
    }
    
    size_t padding = width - str.length();
    
    if (left_align) {
        return str + std::string(padding, fill_char);
    } else {
        return std::string(padding, fill_char) + str;
    }
}

// ============================================================================
// 字符串比较和搜索实现
// ============================================================================

/**
 * @brief 比较字符串
 */
int StringUtils::compare(const std::string& str1, const std::string& str2, StringCompareOptions options) {
    std::string s1 = str1;
    std::string s2 = str2;
    
    // 处理忽略大小写
    if (hasFlag(options, StringCompareOptions::IGNORE_CASE)) {
        s1 = toLower(s1);
        s2 = toLower(s2);
    }
    
    // 处理忽略空白字符
    if (hasFlag(options, StringCompareOptions::IGNORE_WHITESPACE)) {
        s1 = removeChars(s1, " \t\n\r\f\v");
        s2 = removeChars(s2, " \t\n\r\f\v");
    }
    
    // 处理自然排序
    if (hasFlag(options, StringCompareOptions::NATURAL_ORDER)) {
        s1 = naturalSort(s1);
        s2 = naturalSort(s2);
    }
    
    return s1.compare(s2);
}

/**
 * @brief 检查字符串是否相等
 */
bool StringUtils::equals(const std::string& str1, const std::string& str2, StringCompareOptions options) {
    return compare(str1, str2, options) == 0;
}

/**
 * @brief 检查字符串是否以指定前缀开始
 */
bool StringUtils::startsWith(const std::string& str, const std::string& prefix, bool ignore_case) {
    if (prefix.length() > str.length()) {
        return false;
    }
    
    if (ignore_case) {
        return toLower(str.substr(0, prefix.length())) == toLower(prefix);
    } else {
        return str.substr(0, prefix.length()) == prefix;
    }
}

/**
 * @brief 检查字符串是否以指定后缀结束
 */
bool StringUtils::endsWith(const std::string& str, const std::string& suffix, bool ignore_case) {
    if (suffix.length() > str.length()) {
        return false;
    }
    
    size_t start_pos = str.length() - suffix.length();
    
    if (ignore_case) {
        return toLower(str.substr(start_pos)) == toLower(suffix);
    } else {
        return str.substr(start_pos) == suffix;
    }
}

/**
 * @brief 检查字符串是否包含子字符串
 */
bool StringUtils::contains(const std::string& str, const std::string& substr, bool ignore_case) {
    return find(str, substr, 0, ignore_case) != std::string::npos;
}

/**
 * @brief 查找子字符串位置
 */
size_t StringUtils::find(const std::string& str, const std::string& substr, size_t start_pos, bool ignore_case) {
    if (ignore_case) {
        std::string lower_str = toLower(str);
        std::string lower_substr = toLower(substr);
        return lower_str.find(lower_substr, start_pos);
    } else {
        return str.find(substr, start_pos);
    }
}

/**
 * @brief 从右侧查找子字符串位置
 */
size_t StringUtils::findLast(const std::string& str, const std::string& substr, size_t start_pos, bool ignore_case) {
    if (ignore_case) {
        std::string lower_str = toLower(str);
        std::string lower_substr = toLower(substr);
        return lower_str.rfind(lower_substr, start_pos);
    } else {
        return str.rfind(substr, start_pos);
    }
}

/**
 * @brief 查找所有匹配位置
 */
std::vector<size_t> StringUtils::findAll(const std::string& str, const std::string& substr, bool ignore_case) {
    std::vector<size_t> positions;
    size_t pos = 0;
    
    while ((pos = find(str, substr, pos, ignore_case)) != std::string::npos) {
        positions.push_back(pos);
        pos += substr.length();
    }
    
    return positions;
}

/**
 * @brief 计算子字符串出现次数
 */
size_t StringUtils::count(const std::string& str, const std::string& substr, bool ignore_case) {
    return findAll(str, substr, ignore_case).size();
}

// ============================================================================
// 字符串分割和连接实现
// ============================================================================

/**
 * @brief 分割字符串
 */
std::vector<std::string> StringUtils::split(const std::string& str, const std::string& delimiter, const StringSplitOptions& options) {
    std::vector<std::string> result;
    
    if (str.empty()) {
        return result;
    }
    
    if (options.use_regex) {
        return regexSplit(str, delimiter, !options.case_sensitive);
    }
    
    size_t start = 0;
    size_t pos = 0;
    size_t splits = 0;
    
    while ((pos = find(str, delimiter, start, !options.case_sensitive)) != std::string::npos && 
           splits < options.max_splits) {
        std::string token = str.substr(start, pos - start);
        
        if (options.trim_whitespace) {
            token = trim(token);
        }
        
        if (!options.remove_empty || !token.empty()) {
            result.push_back(token);
        }
        
        start = pos + delimiter.length();
        splits++;
    }
    
    // 添加最后一个部分
    if (start < str.length()) {
        std::string token = str.substr(start);
        
        if (options.trim_whitespace) {
            token = trim(token);
        }
        
        if (!options.remove_empty || !token.empty()) {
            result.push_back(token);
        }
    }
    
    return result;
}

/**
 * @brief 按字符分割字符串
 */
std::vector<std::string> StringUtils::splitByChars(const std::string& str, const std::string& delimiters, const StringSplitOptions& options) {
    std::vector<std::string> result;
    
    if (str.empty()) {
        return result;
    }
    
    size_t start = 0;
    size_t splits = 0;
    
    for (size_t i = 0; i < str.length() && splits < options.max_splits; ++i) {
        if (delimiters.find(str[i]) != std::string::npos) {
            if (i > start) {
                std::string token = str.substr(start, i - start);
                
                if (options.trim_whitespace) {
                    token = trim(token);
                }
                
                if (!options.remove_empty || !token.empty()) {
                    result.push_back(token);
                }
                
                splits++;
            }
            start = i + 1;
        }
    }
    
    // 添加最后一个部分
    if (start < str.length()) {
        std::string token = str.substr(start);
        
        if (options.trim_whitespace) {
            token = trim(token);
        }
        
        if (!options.remove_empty || !token.empty()) {
            result.push_back(token);
        }
    }
    
    return result;
}

/**
 * @brief 按行分割字符串
 */
std::vector<std::string> StringUtils::splitLines(const std::string& str, bool keep_empty_lines) {
    std::vector<std::string> result;
    std::istringstream stream(str);
    std::string line;
    
    while (std::getline(stream, line)) {
        // 处理Windows风格的换行符
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        
        if (keep_empty_lines || !line.empty()) {
            result.push_back(line);
        }
    }
    
    return result;
}

/**
 * @brief 连接字符串列表
 */
std::string StringUtils::join(const std::vector<std::string>& strings, const std::string& delimiter) {
    if (strings.empty()) {
        return "";
    }
    
    std::ostringstream oss;
    for (size_t i = 0; i < strings.size(); ++i) {
        if (i > 0) {
            oss << delimiter;
        }
        oss << strings[i];
    }
    
    return oss.str();
}

/**
 * @brief 连接字符串列表(带格式化)
 */
std::string StringUtils::joinFormatted(const std::vector<std::string>& strings, const std::string& delimiter, const std::string& prefix, const std::string& suffix) {
    if (strings.empty()) {
        return prefix + suffix;
    }
    
    return prefix + join(strings, delimiter) + suffix;
}

// ============================================================================
// 字符串替换实现
// ============================================================================

/**
 * @brief 替换字符串
 */
std::string StringUtils::replace(const std::string& str, const std::string& search, const std::string& replacement, const StringReplaceOptions& options) {
    if (search.empty()) {
        return str;
    }
    
    if (options.use_regex) {
        return regexReplace(str, search, replacement, options.replace_all, !options.case_sensitive);
    }
    
    std::string result = str;
    size_t pos = 0;
    size_t replacements = 0;
    
    while ((pos = find(result, search, pos, !options.case_sensitive)) != std::string::npos && 
           replacements < options.max_replacements) {
        result.replace(pos, search.length(), replacement);
        pos += replacement.length();
        replacements++;
        
        if (!options.replace_all) {
            break;
        }
    }
    
    return result;
}

/**
 * @brief 批量替换字符串
 */
std::string StringUtils::replaceMultiple(const std::string& str, const std::unordered_map<std::string, std::string>& replacements, bool case_sensitive) {
    std::string result = str;
    
    for (const auto& pair : replacements) {
        StringReplaceOptions options;
        options.case_sensitive = case_sensitive;
        options.replace_all = true;
        result = replace(result, pair.first, pair.second, options);
    }
    
    return result;
}

/**
 * @brief 替换字符
 */
std::string StringUtils::replaceChar(const std::string& str, char old_char, char new_char) {
    std::string result = str;
    std::replace(result.begin(), result.end(), old_char, new_char);
    return result;
}

/**
 * @brief 移除字符串中的指定字符
 */
std::string StringUtils::removeChars(const std::string& str, const std::string& chars) {
    std::string result;
    result.reserve(str.length());
    
    for (char c : str) {
        if (chars.find(c) == std::string::npos) {
            result += c;
        }
    }
    
    return result;
}

/**
 * @brief 保留字符串中的指定字符
 */
std::string StringUtils::keepChars(const std::string& str, const std::string& chars) {
    std::string result;
    result.reserve(str.length());
    
    for (char c : str) {
        if (chars.find(c) != std::string::npos) {
            result += c;
        }
    }
    
    return result;
}

// ============================================================================
// 字符串格式化实现
// ============================================================================

/**
 * @brief 格式化浮点数
 */
std::string StringUtils::formatFloat(double value, int precision, bool fixed) {
    std::ostringstream oss;
    oss << std::setprecision(precision);
    
    if (fixed) {
        oss << std::fixed;
    }
    
    oss << value;
    return oss.str();
}

/**
 * @brief 格式化百分比
 */
std::string StringUtils::formatPercent(double value, int precision) {
    return formatFloat(value * 100.0, precision, true) + "%";
}

/**
 * @brief 格式化文件大小
 */
std::string StringUtils::formatFileSize(size_t bytes, bool binary) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB"};
    const char* binary_units[] = {"B", "KiB", "MiB", "GiB", "TiB", "PiB"};
    
    double size = static_cast<double>(bytes);
    int unit_index = 0;
    double divisor = binary ? 1024.0 : 1000.0;
    
    while (size >= divisor && unit_index < 5) {
        size /= divisor;
        unit_index++;
    }
    
    std::ostringstream oss;
    oss << std::setprecision(1) << std::fixed << size << " ";
    oss << (binary ? binary_units[unit_index] : units[unit_index]);
    
    return oss.str();
}

/**
 * @brief 格式化时间间隔
 */
std::string StringUtils::formatDuration(double seconds, bool show_milliseconds) {
    if (seconds < 0) {
        return "0s";
    }
    
    std::ostringstream oss;
    
    int hours = static_cast<int>(seconds / 3600);
    int minutes = static_cast<int>((seconds - hours * 3600) / 60);
    double remaining_seconds = seconds - hours * 3600 - minutes * 60;
    
    if (hours > 0) {
        oss << hours << "h";
        if (minutes > 0 || remaining_seconds > 0) {
            oss << " ";
        }
    }
    
    if (minutes > 0) {
        oss << minutes << "m";
        if (remaining_seconds > 0) {
            oss << " ";
        }
    }
    
    if (remaining_seconds > 0 || (hours == 0 && minutes == 0)) {
        if (show_milliseconds && remaining_seconds < 10) {
            oss << std::setprecision(3) << std::fixed << remaining_seconds << "s";
        } else {
            oss << std::setprecision(1) << std::fixed << remaining_seconds << "s";
        }
    }
    
    return oss.str();
}

// ============================================================================
// 字符串验证实现
// ============================================================================

/**
 * @brief 检查是否为空或只包含空白字符
 */
bool StringUtils::isBlank(const std::string& str) {
    return trim(str).empty();
}

/**
 * @brief 检查是否为数字
 */
bool StringUtils::isNumeric(const std::string& str, bool allow_decimal, bool allow_negative) {
    if (str.empty()) {
        return false;
    }
    
    size_t start = 0;
    
    // 检查负号
    if (str[0] == '-') {
        if (!allow_negative || str.length() == 1) {
            return false;
        }
        start = 1;
    } else if (str[0] == '+') {
        if (str.length() == 1) {
            return false;
        }
        start = 1;
    }
    
    bool has_decimal = false;
    
    for (size_t i = start; i < str.length(); ++i) {
        char c = str[i];
        
        if (c == '.') {
            if (!allow_decimal || has_decimal) {
                return false;
            }
            has_decimal = true;
        } else if (!std::isdigit(c)) {
            return false;
        }
    }
    
    return true;
}

/**
 * @brief 检查是否为整数
 */
bool StringUtils::isInteger(const std::string& str, bool allow_negative) {
    return isNumeric(str, false, allow_negative);
}

/**
 * @brief 检查是否为浮点数
 */
bool StringUtils::isFloat(const std::string& str, bool allow_negative) {
    return isNumeric(str, true, allow_negative) && str.find('.') != std::string::npos;
}

/**
 * @brief 检查是否为字母
 */
bool StringUtils::isAlpha(const std::string& str) {
    if (str.empty()) {
        return false;
    }
    
    return std::all_of(str.begin(), str.end(), [](unsigned char c) {
        return std::isalpha(c);
    });
}

/**
 * @brief 检查是否为字母数字
 */
bool StringUtils::isAlphaNumeric(const std::string& str) {
    if (str.empty()) {
        return false;
    }
    
    return std::all_of(str.begin(), str.end(), [](unsigned char c) {
        return std::isalnum(c);
    });
}

/**
 * @brief 检查是否为有效的邮箱地址
 */
bool StringUtils::isValidEmail(const std::string& str) {
    const std::regex email_regex(R"(^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$)");
    return std::regex_match(str, email_regex);
}

/**
 * @brief 检查是否为有效的URL
 */
bool StringUtils::isValidUrl(const std::string& str) {
    const std::regex url_regex(R"(^https?://[a-zA-Z0-9.-]+(?:\.[a-zA-Z]{2,})+(?:/[^\s]*)?$)");
    return std::regex_match(str, url_regex);
}

/**
 * @brief 检查是否为有效的IP地址
 */
bool StringUtils::isValidIpAddress(const std::string& str, bool ipv6) {
    if (ipv6) {
        // IPv6 正则表达式(简化版)
        const std::regex ipv6_regex(R"(^([0-9a-fA-F]{1,4}:){7}[0-9a-fA-F]{1,4}$)");
        return std::regex_match(str, ipv6_regex);
    } else {
        // IPv4 正则表达式
        const std::regex ipv4_regex(R"(^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$)");
        return std::regex_match(str, ipv4_regex);
    }
}

// ============================================================================
// 字符串转换实现
// ============================================================================

/**
 * @brief 字符串转整数
 */
int StringUtils::toInt(const std::string& str, int default_value, int base) {
    try {
        return std::stoi(str, nullptr, base);
    } catch (...) {
        return default_value;
    }
}

/**
 * @brief 字符串转长整数
 */
long long StringUtils::toLong(const std::string& str, long long default_value, int base) {
    try {
        return std::stoll(str, nullptr, base);
    } catch (...) {
        return default_value;
    }
}

/**
 * @brief 字符串转浮点数
 */
float StringUtils::toFloat(const std::string& str, float default_value) {
    try {
        return std::stof(str);
    } catch (...) {
        return default_value;
    }
}

/**
 * @brief 字符串转双精度浮点数
 */
double StringUtils::toDouble(const std::string& str, double default_value) {
    try {
        return std::stod(str);
    } catch (...) {
        return default_value;
    }
}

/**
 * @brief 字符串转布尔值
 */
bool StringUtils::toBool(const std::string& str, bool default_value) {
    std::string lower_str = toLower(trim(str));
    
    if (lower_str == "true" || lower_str == "1" || lower_str == "yes" || lower_str == "on") {
        return true;
    } else if (lower_str == "false" || lower_str == "0" || lower_str == "no" || lower_str == "off") {
        return false;
    }
    
    return default_value;
}

/**
 * @brief 整数转字符串
 */
std::string StringUtils::fromInt(int value, int base, bool uppercase) {
    std::ostringstream oss;
    
    if (base == 8) {
        oss << std::oct;
    } else if (base == 16) {
        oss << std::hex;
        if (uppercase) {
            oss << std::uppercase;
        }
    }
    
    oss << value;
    return oss.str();
}

/**
 * @brief 长整数转字符串
 */
std::string StringUtils::fromLong(long long value, int base, bool uppercase) {
    std::ostringstream oss;
    
    if (base == 8) {
        oss << std::oct;
    } else if (base == 16) {
        oss << std::hex;
        if (uppercase) {
            oss << std::uppercase;
        }
    }
    
    oss << value;
    return oss.str();
}

/**
 * @brief 浮点数转字符串
 */
std::string StringUtils::fromFloat(float value, int precision, bool fixed) {
    return formatFloat(value, precision, fixed);
}

/**
 * @brief 双精度浮点数转字符串
 */
std::string StringUtils::fromDouble(double value, int precision, bool fixed) {
    return formatFloat(value, precision, fixed);
}

/**
 * @brief 布尔值转字符串
 */
std::string StringUtils::fromBool(bool value, const std::string& true_str, const std::string& false_str) {
    return value ? true_str : false_str;
}

// ============================================================================
// 编码转换实现
// ============================================================================

/**
 * @brief UTF-8转UTF-16
 */
std::u16string StringUtils::utf8ToUtf16(const std::string& utf8_str) {
    try {
        std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
        return converter.from_bytes(utf8_str);
    } catch (...) {
        return u"";
    }
}

/**
 * @brief UTF-16转UTF-8
 */
std::string StringUtils::utf16ToUtf8(const std::u16string& utf16_str) {
    try {
        std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
        return converter.to_bytes(utf16_str);
    } catch (...) {
        return "";
    }
}

/**
 * @brief UTF-8转UTF-32
 */
std::u32string StringUtils::utf8ToUtf32(const std::string& utf8_str) {
    try {
        std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
        return converter.from_bytes(utf8_str);
    } catch (...) {
        return U"";
    }
}

/**
 * @brief UTF-32转UTF-8
 */
std::string StringUtils::utf32ToUtf8(const std::u32string& utf32_str) {
    try {
        std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
        return converter.to_bytes(utf32_str);
    } catch (...) {
        return "";
    }
}

/**
 * @brief 转换字符串编码
 */
std::string StringUtils::convertEncoding(const std::string& str, StringEncoding from_encoding, StringEncoding to_encoding) {
    // 简化实现，实际项目中可能需要使用iconv或其他库
    if (from_encoding == to_encoding) {
        return str;
    }
    
    // 这里只实现UTF-8相关的转换
    if (from_encoding == StringEncoding::UTF8 && to_encoding == StringEncoding::UTF16) {
        auto utf16 = utf8ToUtf16(str);
        return std::string(reinterpret_cast<const char*>(utf16.data()), utf16.size() * sizeof(char16_t));
    }
    
    return str; // 默认返回原字符串
}

/**
 * @brief 检测字符串编码
 */
StringEncoding StringUtils::detectEncoding(const std::string& str) {
    // 简化的编码检测，实际实现会更复杂
    
    // 检查UTF-8 BOM
    if (str.length() >= 3 && 
        static_cast<unsigned char>(str[0]) == 0xEF &&
        static_cast<unsigned char>(str[1]) == 0xBB &&
        static_cast<unsigned char>(str[2]) == 0xBF) {
        return StringEncoding::UTF8;
    }
    
    // 检查是否为有效的UTF-8
    bool is_utf8 = true;
    for (size_t i = 0; i < str.length(); ++i) {
        unsigned char c = static_cast<unsigned char>(str[i]);
        if (c > 127) {
            // 简化的UTF-8检查
            if ((c & 0xE0) == 0xC0) {
                if (i + 1 >= str.length() || (static_cast<unsigned char>(str[i + 1]) & 0xC0) != 0x80) {
                    is_utf8 = false;
                    break;
                }
                i++;
            } else if ((c & 0xF0) == 0xE0) {
                if (i + 2 >= str.length() || 
                    (static_cast<unsigned char>(str[i + 1]) & 0xC0) != 0x80 ||
                    (static_cast<unsigned char>(str[i + 2]) & 0xC0) != 0x80) {
                    is_utf8 = false;
                    break;
                }
                i += 2;
            } else {
                is_utf8 = false;
                break;
            }
        }
    }
    
    return is_utf8 ? StringEncoding::UTF8 : StringEncoding::ASCII;
}

// ============================================================================
// 正则表达式实现
// ============================================================================

/**
 * @brief 获取正则表达式标志
 */
std::regex::flag_type StringUtils::getRegexFlags(bool ignore_case) {
    std::regex::flag_type flags = std::regex::ECMAScript;
    if (ignore_case) {
        flags |= std::regex::icase;
    }
    return flags;
}

/**
 * @brief 正则表达式匹配
 */
bool StringUtils::regexMatch(const std::string& str, const std::string& pattern, bool ignore_case) {
    try {
        std::regex regex(pattern, getRegexFlags(ignore_case));
        return std::regex_match(str, regex);
    } catch (...) {
        return false;
    }
}

/**
 * @brief 正则表达式搜索
 */
std::vector<std::string> StringUtils::regexSearch(const std::string& str, const std::string& pattern, bool ignore_case) {
    std::vector<std::string> result;
    
    try {
        std::regex regex(pattern, getRegexFlags(ignore_case));
        std::sregex_iterator iter(str.begin(), str.end(), regex);
        std::sregex_iterator end;
        
        for (; iter != end; ++iter) {
            result.push_back(iter->str());
        }
    } catch (...) {
        // 忽略正则表达式错误
    }
    
    return result;
}

/**
 * @brief 正则表达式替换
 */
std::string StringUtils::regexReplace(const std::string& str, const std::string& pattern, const std::string& replacement, bool replace_all, bool ignore_case) {
    try {
        std::regex regex(pattern, getRegexFlags(ignore_case));
        
        if (replace_all) {
            return std::regex_replace(str, regex, replacement);
        } else {
            std::smatch match;
            if (std::regex_search(str, match, regex)) {
                return str.substr(0, match.position()) + replacement + str.substr(match.position() + match.length());
            }
        }
    } catch (...) {
        // 忽略正则表达式错误
    }
    
    return str;
}

/**
 * @brief 正则表达式分割
 */
std::vector<std::string> StringUtils::regexSplit(const std::string& str, const std::string& pattern, bool ignore_case) {
    std::vector<std::string> result;
    
    try {
        std::regex regex(pattern, getRegexFlags(ignore_case));
        std::sregex_token_iterator iter(str.begin(), str.end(), regex, -1);
        std::sregex_token_iterator end;
        
        for (; iter != end; ++iter) {
            std::string token = iter->str();
            if (!token.empty()) {
                result.push_back(token);
            }
        }
    } catch (...) {
        result.push_back(str); // 如果正则表达式失败，返回原字符串
    }
    
    return result;
}

// ============================================================================
// 字符串哈希和校验实现
// ============================================================================

/**
 * @brief FNV-1a哈希算法
 */
uint64_t StringUtils::fnv1aHash(const std::string& str) {
    const uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
    const uint64_t FNV_PRIME = 1099511628211ULL;
    
    uint64_t hash = FNV_OFFSET_BASIS;
    for (char c : str) {
        hash ^= static_cast<uint64_t>(c);
        hash *= FNV_PRIME;
    }
    
    return hash;
}

/**
 * @brief MurmurHash3算法(简化版)
 */
uint64_t StringUtils::murmur3Hash(const std::string& str) {
    const uint64_t seed = 0;
    const uint64_t c1 = 0x87c37b91114253d5ULL;
    const uint64_t c2 = 0x4cf5ad432745937fULL;
    
    uint64_t hash = seed;
    const char* data = str.data();
    size_t len = str.length();
    
    // 处理8字节块
    const size_t nblocks = len / 8;
    for (size_t i = 0; i < nblocks; ++i) {
        uint64_t k;
        std::memcpy(&k, data + i * 8, sizeof(k));
        
        k *= c1;
        k = (k << 31) | (k >> 33);
        k *= c2;
        
        hash ^= k;
        hash = (hash << 27) | (hash >> 37);
        hash = hash * 5 + 0x52dce729;
    }
    
    // 处理剩余字节
    const char* tail = data + nblocks * 8;
    uint64_t k1 = 0;
    
    switch (len & 7) {
        case 7: k1 ^= static_cast<uint64_t>(tail[6]) << 48;
        case 6: k1 ^= static_cast<uint64_t>(tail[5]) << 40;
        case 5: k1 ^= static_cast<uint64_t>(tail[4]) << 32;
        case 4: k1 ^= static_cast<uint64_t>(tail[3]) << 24;
        case 3: k1 ^= static_cast<uint64_t>(tail[2]) << 16;
        case 2: k1 ^= static_cast<uint64_t>(tail[1]) << 8;
        case 1: k1 ^= static_cast<uint64_t>(tail[0]);
                k1 *= c1;
                k1 = (k1 << 31) | (k1 >> 33);
                k1 *= c2;
                hash ^= k1;
    }
    
    // 最终化
    hash ^= len;
    hash ^= hash >> 33;
    hash *= 0xff51afd7ed558ccdULL;
    hash ^= hash >> 33;
    hash *= 0xc4ceb9fe1a85ec53ULL;
    hash ^= hash >> 33;
    
    return hash;
}

/**
 * @brief CRC32哈希算法
 */
uint32_t StringUtils::crc32Hash(const std::string& str) {
    static const uint32_t crc32_table[256] = {
        0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
        0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
        0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
        0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
        0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
        0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
        0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
        0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
        0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
        0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
        0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
        0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
        0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
        0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
        0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
        0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
        0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
        0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
        0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
        0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
        0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
        0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
        0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
        0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
        0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
        0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
        0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
        0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
        0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
        0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
        0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
        0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
        0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
        0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
        0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
        0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
        0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
        0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
        0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
        0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
        0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
        0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
        0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
    };
    
    uint32_t crc = 0xFFFFFFFF;
    for (char c : str) {
        crc = crc32_table[(crc ^ static_cast<unsigned char>(c)) & 0xFF] ^ (crc >> 8);
    }
    
    return crc ^ 0xFFFFFFFF;
}

/**
 * @brief 计算字符串哈希值
 */
uint64_t StringUtils::hash(const std::string& str, StringHashAlgorithm algorithm) {
    switch (algorithm) {
        case StringHashAlgorithm::FNV1A:
            return fnv1aHash(str);
        case StringHashAlgorithm::MURMUR3:
            return murmur3Hash(str);
        case StringHashAlgorithm::CRC32:
            return static_cast<uint64_t>(crc32Hash(str));
        default:
            return fnv1aHash(str);
    }
}

/**
 * @brief 计算字符串哈希值(十六进制字符串)
 */
std::string StringUtils::hashHex(const std::string& str, StringHashAlgorithm algorithm) {
    uint64_t hash_value = hash(str, algorithm);
    std::ostringstream oss;
    oss << std::hex << hash_value;
    return oss.str();
}

/**
 * @brief 计算字符串校验和
 */
uint32_t StringUtils::checksum(const std::string& str) {
    uint32_t sum = 0;
    for (char c : str) {
        sum += static_cast<unsigned char>(c);
    }
    return sum;
}

// ============================================================================
// 字符串生成实现
// ============================================================================

/**
 * @brief 生成随机字符串
 */
std::string StringUtils::generateRandom(size_t length, const std::string& charset) {
    if (length == 0 || charset.empty()) {
        return "";
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, charset.size() - 1);
    
    std::string result;
    result.reserve(length);
    
    for (size_t i = 0; i < length; ++i) {
        result += charset[dis(gen)];
    }
    
    return result;
}

/**
 * @brief 生成UUID字符串
 */
std::string StringUtils::generateUuid(bool use_hyphens, bool uppercase) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    const char* hex_chars = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
    
    std::string uuid;
    uuid.reserve(use_hyphens ? 36 : 32);
    
    for (int i = 0; i < 32; ++i) {
        if (use_hyphens && (i == 8 || i == 12 || i == 16 || i == 20)) {
            uuid += '-';
        }
        
        if (i == 12) {
            uuid += '4'; // Version 4
        } else if (i == 16) {
            uuid += hex_chars[(dis(gen) & 0x3) | 0x8]; // Variant bits
        } else {
            uuid += hex_chars[dis(gen)];
        }
    }
    
    return uuid;
}

/**
 * @brief 生成密码
 */
std::string StringUtils::generatePassword(size_t length, bool include_symbols, bool exclude_ambiguous) {
    std::string charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    
    if (include_symbols) {
        charset += "!@#$%^&*()_+-=[]{}|;:,.<>?";
    }
    
    if (exclude_ambiguous) {
        // 移除易混淆的字符
        charset = removeChars(charset, "0O1lI");
    }
    
    return generateRandom(length, charset);
}

// ============================================================================
// 字符串距离和相似度实现
// ============================================================================

/**
 * @brief 计算编辑距离(Levenshtein距离)
 */
size_t StringUtils::editDistance(const std::string& str1, const std::string& str2) {
    size_t len1 = str1.length();
    size_t len2 = str2.length();
    
    if (len1 == 0) return len2;
    if (len2 == 0) return len1;
    
    std::vector<std::vector<size_t>> dp(len1 + 1, std::vector<size_t>(len2 + 1));
    
    // 初始化
    for (size_t i = 0; i <= len1; ++i) {
        dp[i][0] = i;
    }
    for (size_t j = 0; j <= len2; ++j) {
        dp[0][j] = j;
    }
    
    // 动态规划
    for (size_t i = 1; i <= len1; ++i) {
        for (size_t j = 1; j <= len2; ++j) {
            if (str1[i - 1] == str2[j - 1]) {
                dp[i][j] = dp[i - 1][j - 1];
            } else {
                dp[i][j] = 1 + std::min({dp[i - 1][j], dp[i][j - 1], dp[i - 1][j - 1]});
            }
        }
    }
    
    return dp[len1][len2];
}

/**
 * @brief 计算字符串相似度
 */
double StringUtils::similarity(const std::string& str1, const std::string& str2) {
    size_t max_len = std::max(str1.length(), str2.length());
    if (max_len == 0) {
        return 1.0;
    }
    
    size_t distance = editDistance(str1, str2);
    return 1.0 - static_cast<double>(distance) / max_len;
}

/**
 * @brief Jaro相似度算法
 */
double StringUtils::jaroSimilarity(const std::string& str1, const std::string& str2) {
    size_t len1 = str1.length();
    size_t len2 = str2.length();
    
    if (len1 == 0 && len2 == 0) return 1.0;
    if (len1 == 0 || len2 == 0) return 0.0;
    
    size_t match_window = std::max(len1, len2) / 2 - 1;
    if (match_window < 1) match_window = 0;
    
    std::vector<bool> str1_matches(len1, false);
    std::vector<bool> str2_matches(len2, false);
    
    size_t matches = 0;
    size_t transpositions = 0;
    
    // 查找匹配字符
    for (size_t i = 0; i < len1; ++i) {
        size_t start = (i >= match_window) ? i - match_window : 0;
        size_t end = std::min(i + match_window + 1, len2);
        
        for (size_t j = start; j < end; ++j) {
            if (str2_matches[j] || str1[i] != str2[j]) continue;
            
            str1_matches[i] = true;
            str2_matches[j] = true;
            matches++;
            break;
        }
    }
    
    if (matches == 0) return 0.0;
    
    // 计算转置
    size_t k = 0;
    for (size_t i = 0; i < len1; ++i) {
        if (!str1_matches[i]) continue;
        
        while (!str2_matches[k]) k++;
        
        if (str1[i] != str2[k]) transpositions++;
        k++;
    }
    
    double jaro = (static_cast<double>(matches) / len1 + 
                   static_cast<double>(matches) / len2 + 
                   static_cast<double>(matches - transpositions / 2) / matches) / 3.0;
    
    return jaro;
}

/**
 * @brief Jaro-Winkler相似度算法
 */
double StringUtils::jaroWinklerSimilarity(const std::string& str1, const std::string& str2) {
    double jaro = jaroSimilarity(str1, str2);
    
    if (jaro < 0.7) {
        return jaro;
    }
    
    // 计算公共前缀长度(最多4个字符)
    size_t prefix_len = 0;
    size_t max_prefix = std::min({str1.length(), str2.length(), static_cast<size_t>(4)});
    
    for (size_t i = 0; i < max_prefix; ++i) {
        if (str1[i] == str2[i]) {
            prefix_len++;
        } else {
            break;
        }
    }
    
    return jaro + (0.1 * prefix_len * (1.0 - jaro));
}

// ============================================================================
// 辅助函数实现
// ============================================================================

/**
 * @brief 自然排序预处理
 */
std::string StringUtils::naturalSort(const std::string& str) {
    std::string result;
    result.reserve(str.length() * 2);
    
    for (size_t i = 0; i < str.length(); ++i) {
        if (std::isdigit(str[i])) {
            // 找到数字序列的开始和结束
            size_t start = i;
            while (i < str.length() && std::isdigit(str[i])) {
                i++;
            }
            
            // 提取数字并格式化为固定长度
            std::string number = str.substr(start, i - start);
            int value = std::stoi(number);
            
            // 格式化为8位数字(可根据需要调整)
            std::ostringstream oss;
            oss << std::setfill('0') << std::setw(8) << value;
            result += oss.str();
            
            i--; // 因为外层循环会++i
        } else {
            result += str[i];
        }
    }
    
    return result;
}

// hasFlag function moved to header as inline function

} // namespace Utils
} // namespace Core
} // namespace DearTs