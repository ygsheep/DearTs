#include "clipboard_monitor.h"
#include "../../utils/logger.h"
#include <algorithm>
#include <sstream>
#include <regex>

namespace DearTs::Core::Window::Widgets::Clipboard {

// 静态成员初始化
ClipboardMonitor* ClipboardMonitor::instance_ = nullptr;
WNDPROC ClipboardMonitor::original_window_proc_ = nullptr;

ClipboardMonitor::ClipboardMonitor()
    : hwnd_(nullptr)
    , is_monitoring_(false) {
    DEARTS_LOG_INFO("ClipboardMonitor构造函数");
    instance_ = this;
}

ClipboardMonitor::~ClipboardMonitor() {
    DEARTS_LOG_INFO("ClipboardMonitor析构函数");
    stopMonitoring();
    if (instance_ == this) {
        instance_ = nullptr;
    }
}

ClipboardMonitor& ClipboardMonitor::getInstance() {
    static ClipboardMonitor instance;
    return instance;
}

bool ClipboardMonitor::startMonitoring(HWND hwnd) {
    if (is_monitoring_) {
        DEARTS_LOG_WARN("剪切板监听已在运行中");
        return true;
    }

    if (!hwnd) {
        DEARTS_LOG_ERROR("无效的窗口句柄");
        return false;
    }

    hwnd_ = hwnd;

    // 设置剪切板监听器
    if (!AddClipboardFormatListener(hwnd)) {
        DWORD error = GetLastError();
        DEARTS_LOG_ERROR("添加剪切板格式监听器失败，错误代码: " + std::to_string(error));

        // 尝试使用旧方法
        DEARTS_LOG_INFO("尝试使用SetClipboardViewer方法");
        HWND next_viewer = SetClipboardViewer(hwnd);
        if (next_viewer == nullptr && GetLastError() != 0) {
            DEARTS_LOG_ERROR("设置剪切板查看器失败，错误代码: " + std::to_string(GetLastError()));
            return false;
        }
    }

    // 子类化窗口以处理消息
    original_window_proc_ = reinterpret_cast<WNDPROC>(
        SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WindowProc))
    );

    if (!original_window_proc_) {
        DEARTS_LOG_ERROR("窗口过程子类化失败，错误代码: " + std::to_string(GetLastError()));
        return false;
    }

    is_monitoring_ = true;
    DEARTS_LOG_INFO("剪切板监听启动成功");

    // 获取初始剪切板内容
    last_clipboard_content_ = getCurrentClipboardContent();
    if (!last_clipboard_content_.empty()) {
        DEARTS_LOG_INFO("初始剪切板内容长度: " + std::to_string(last_clipboard_content_.length()));
    }

    return true;
}

void ClipboardMonitor::stopMonitoring() {
    if (!is_monitoring_) {
        return;
    }

    DEARTS_LOG_INFO("停止剪切板监听");

    // 恢复原始窗口过程
    if (hwnd_ && original_window_proc_) {
        SetWindowLongPtr(hwnd_, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(original_window_proc_));
        original_window_proc_ = nullptr;
    }

    // 移除剪切板格式监听器
    if (hwnd_) {
        RemoveClipboardFormatListener(hwnd_);
        ChangeClipboardChain(hwnd_, nullptr);
    }

    is_monitoring_ = false;
    hwnd_ = nullptr;
    DEARTS_LOG_INFO("剪切板监听已停止");
}

void ClipboardMonitor::setChangeCallback(ClipboardChangeCallback callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    callback_ = callback;
    DEARTS_LOG_INFO("剪切板变化回调函数已设置");
}

std::string ClipboardMonitor::getCurrentClipboardContent() {
    if (!OpenClipboard(nullptr)) {
        DEARTS_LOG_WARN("无法打开剪切板");
        return "";
    }

    std::string content = "";

    // 尝试获取Unicode文本
    content = getClipboardUnicodeText();

    // 如果Unicode文本为空，尝试获取ANSI文本
    if (content.empty()) {
        content = getClipboardText();
    }

    CloseClipboard();

    // 清理内容
    content = cleanContent(content);

    return content;
}

std::string ClipboardMonitor::getClipboardText() {
    HANDLE hData = GetClipboardData(CF_TEXT);
    if (!hData) {
        return "";
    }

    char* pszText = static_cast<char*>(GlobalLock(hData));
    if (!pszText) {
        return "";
    }

    std::string text(pszText);
    GlobalUnlock(hData);

    return text;
}

std::string ClipboardMonitor::getClipboardUnicodeText() {
    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (!hData) {
        return "";
    }

    wchar_t* pszText = static_cast<wchar_t*>(GlobalLock(hData));
    if (!pszText) {
        return "";
    }

    // 转换为UTF-8
    int length = WideCharToMultiByte(CP_UTF8, 0, pszText, -1, nullptr, 0, nullptr, nullptr);
    if (length <= 0) {
        GlobalUnlock(hData);
        return "";
    }

    std::string text(length - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, pszText, -1, &text[0], length, nullptr, nullptr);

    GlobalUnlock(hData);
    return text;
}

bool ClipboardMonitor::isValidContent(const std::string& text) {
    if (text.empty()) {
        return false;
    }

    // 检查文本长度（限制最大长度以避免处理过大的内容）
    if (text.length() > 100000) { // 100KB限制
        DEARTS_LOG_WARN("剪切板内容过长，跳过处理: " + std::to_string(text.length()) + " 字符");
        return false;
    }

    // 检查是否只包含空白字符
    bool has_non_whitespace = false;
    for (char c : text) {
        if (!isspace(static_cast<unsigned char>(c))) {
            has_non_whitespace = true;
            break;
        }
    }

    return has_non_whitespace;
}

std::string ClipboardMonitor::cleanContent(const std::string& text) {
    if (text.empty()) {
        return text;
    }

    std::string cleaned = text;

    // 移除首尾空白字符
    size_t start = cleaned.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) {
        return "";
    }

    size_t end = cleaned.find_last_not_of(" \t\n\r");
    cleaned = cleaned.substr(start, end - start + 1);

    // 替换多个连续的空白字符为单个空格
    std::regex whitespace_pattern("\\s+");
    cleaned = std::regex_replace(cleaned, whitespace_pattern, " ");

    return cleaned;
}

LRESULT CALLBACK ClipboardMonitor::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (instance_ && hwnd == instance_->hwnd_) {
        switch (msg) {
            case WM_CLIPBOARDUPDATE:
                instance_->handleClipboardChange();
                return 0;

            case WM_DRAWCLIPBOARD:
                // 处理旧式剪切板查看器消息
                instance_->handleClipboardChange();
                break;

            case WM_CHANGECBCHAIN:
                // 处理剪切板查看器链变化
                break;

            case WM_DESTROY:
                // 窗口销毁时停止监听
                instance_->stopMonitoring();
                break;
        }
    }

    // 调用原始窗口过程
    if (instance_ && original_window_proc_) {
        return CallWindowProc(original_window_proc_, hwnd, msg, wParam, lParam);
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void ClipboardMonitor::handleClipboardChange() {
    try {
        // 获取当前剪切板内容
        std::string current_content = getCurrentClipboardContent();

        // 检查内容是否发生了变化
        if (current_content != last_clipboard_content_) {
            DEARTS_LOG_INFO("检测到剪切板内容变化");

            // 验证内容有效性
            if (isValidContent(current_content)) {
                DEARTS_LOG_INFO("新剪切板内容: " +
                              std::to_string(current_content.length()) + " 字符");

                // 调用回调函数
                {
                    std::lock_guard<std::mutex> lock(callback_mutex_);
                    if (callback_) {
                        callback_(current_content);
                    }
                }

                // 更新上次内容记录
                last_clipboard_content_ = current_content;
            } else {
                DEARTS_LOG_DEBUG("剪切板内容无效或为空，忽略");
            }
        }
    } catch (const std::exception& e) {
        DEARTS_LOG_ERROR("处理剪切板变化时发生异常: " + std::string(e.what()));
    }
}

} // namespace DearTs::Core::Window::Widgets::Clipboard