/**
 * @file clipboard_parser_plugin.hpp
 * @brief 剪切板智能解析插件
 * @details 监听剪切板变化，提供智能分词和内容提取功能
 * @author DearTs Team
 * @date 2025
 * @version 1.0.0
 */

#pragma once

#include "core/plugin/plugin.h"
#include <memory>
#include <mutex>
#include <atomic>

// 前向声明 cppjieba
namespace cppjieba {
    class Jieba;
}

namespace DearTs::Plugins::ClipboardParser {

/**
 * @brief 剪切板解析插件类
 *
 * 功能：
 * - 监听剪切板内容变化
 * - 智能分词（中文+英文）
 * - 提取 URL、文件路径、邮箱、电话号码
 * - 卡片式 UI 展示
 * - 一键复制功能
 * - 可配置快捷键
 * - 启动时后台加载 jieba 分词器
 */
class ClipboardParserPlugin : public Core::Plugin::IPlugin {
public:
    ClipboardParserPlugin();
    ~ClipboardParserPlugin() override;

    /**
     * @brief 获取插件信息
     */
    Core::Plugin::PluginInfo get_info() const override;

    /**
     * @brief 插件加载时调用
     */
    Core::Result<void, std::string> on_load() override;

    /**
     * @brief 插件卸载时调用
     */
    void on_unload() override;

    /**
     * @brief 插件启用时调用
     */
    void on_enable() override;

    /**
     * @brief 插件禁用时调用
     */
    void on_disable() override;

    /**
     * @brief 获取 jieba 分词器实例（线程安全）
     */
    static cppjieba::Jieba* get_jieba();

    /**
     * @brief 检查 jieba 是否已加载
     */
    static bool is_jieba_ready();

    /**
     * @brief 获取 jieba 字典路径
     */
    static std::string get_jieba_dict_path();

private:
    /**
     * @brief 启动 jieba 后台加载
     */
    void start_jieba_background_load();

    // 静态 jieba 实例和状态
    static std::unique_ptr<cppjieba::Jieba> s_jieba;
    static std::mutex s_jieba_mutex;
    static std::atomic<bool> s_jieba_loading;
    static std::atomic<bool> s_jieba_ready;
};

} // namespace DearTs::Plugins::ClipboardParser
