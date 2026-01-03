/**
 * @file test_plugin_export.cpp
 * @brief 测试插件导出函数
 * @details 这个文件实现了 DLL 导出函数，用于插件自动发现
 */

#include "test_plugin.hpp"
#include "core/plugin/plugin.h"

using namespace DearTs::Core::Plugin;

// 导出函数：创建插件实例
extern "C" {
    /**
     * @brief 创建插件实例
     * @note 插件系统通过符号 "dearts_create_plugin" 查找此函数
     */
    __declspec(dllexport) IPlugin* dearts_create_plugin() {
        LOG_INFO("TestPlugin: dearts_create_plugin() called");
        return new TestPlugin::TestPlugin();
    }

    /**
     * @brief 销毁插件实例
     * @param plugin 要销毁的插件指针
     * @note 插件系统通过符号 "dearts_destroy_plugin" 查找此函数
     */
    __declspec(dllexport) void dearts_destroy_plugin(IPlugin* plugin) {
        LOG_INFO("TestPlugin: dearts_destroy_plugin() called");
        delete plugin;
    }
}
