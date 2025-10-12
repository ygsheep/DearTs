#pragma once

#include <windows.h>
#include <functional>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>

namespace DearTs::Core::Window::Widgets::Clipboard {

/**
 * @brief 剪切板监听器回调函数类型
 */
using ClipboardChangeCallback = std::function<void(const std::string& content)>;

/**
 * @brief 剪切板监听器类
 *
 * 负责监听系统剪切板的变化，当剪切板内容发生改变时触发回调函数。
 * 使用Windows API实现高效的剪切板监听机制。
 */
class ClipboardMonitor {
public:
    /**
     * @brief 构造函数
     */
    ClipboardMonitor();

    /**
     * @brief 析构函数
     */
    ~ClipboardMonitor();

    /**
     * @brief 开始监听剪切板变化
     * @param hwnd 接收剪切板消息的窗口句柄
     * @return 是否成功开始监听
     */
    bool startMonitoring(HWND hwnd);

    /**
     * @brief 停止监听剪切板
     */
    void stopMonitoring();

    /**
     * @brief 设置剪切板变化回调函数
     * @param callback 回调函数，接收新的剪切板内容
     */
    void setChangeCallback(ClipboardChangeCallback callback);

    /**
     * @brief 获取当前剪切板内容
     * @return 当前剪切板中的文本内容，如果无内容则返回空字符串
     */
    std::string getCurrentClipboardContent();

    /**
     * @brief 检查是否正在监听
     * @return 监听状态
     */
    bool isMonitoring() const { return is_monitoring_; }

    /**
     * @brief 获取监听器实例（单例模式）
     * @return 监听器实例引用
     */
    static ClipboardMonitor& getInstance();

private:
    /**
     * @brief 窗口消息处理函数（静态）
     */
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    /**
     * @brief 处理剪切板变化消息
     */
    void handleClipboardChange();

    /**
     * @brief 获取剪切板文本内容
     * @return 剪切板中的文本
     */
    std::string getClipboardText();

    /**
     * @brief 获取Unicode剪切板文本内容
     * @return 剪切板中的Unicode文本
     */
    std::string getClipboardUnicodeText();

    /**
     * @brief 检查文本内容是否有效
     * @param text 要检查的文本
     * @return 是否为有效内容
     */
    bool isValidContent(const std::string& text);

    /**
     * @brief 清理剪切板内容（移除多余空白字符）
     * @param text 原始文本
     * @return 清理后的文本
     */
    std::string cleanContent(const std::string& text);

    // 成员变量
    HWND hwnd_;                                      // 监听窗口句柄
    std::atomic<bool> is_monitoring_;                // 监听状态
    ClipboardChangeCallback callback_;                // 回调函数
    std::string last_clipboard_content_;             // 上次的剪切板内容
    std::mutex callback_mutex_;                      // 回调函数保护锁
    static ClipboardMonitor* instance_;               // 单例实例
    static WNDPROC original_window_proc_;            // 原始窗口过程
};

} // namespace DearTs::Core::Window::Widgets::Clipboard