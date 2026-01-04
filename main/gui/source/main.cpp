/**
 * @file main.cpp
 * @brief DearTs 工具箱应用入口
 * @details 基于 DearTs 框架的工具箱应用主程序
 */

#include "dearts_application.hpp"
#include "liblogger/logger.h"
#include "core/config/config_manager.h"
#include <SDL3/SDL_main.h>
#include <iostream>
#include <memory>
#include <filesystem>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

using namespace DearTs;

/**
 * @brief 初始化日志系统（从 ConfigManager 读取配置）
 */
void init_logger() {
    auto logger = Logger::get_instance();
    auto& config = Core::Config::ConfigManager::instance();

    // 先尝试加载配置文件（如果存在）
    // 这样日志系统可以使用配置的文件路径
    auto load_result = config.load_from_file("config.json");
    if (load_result.isErr()) {
        // 配置文件不存在或加载失败，使用默认值
        std::cout << "配置文件未找到或加载失败，使用默认配置" << std::endl;
    }

    // 注册日志配置元数据
    config.register_meta("logger.level", {
        .description = "Log level (0=TRACE, 1=DEBUG, 2=INFO, 3=WARN, 4=ERROR, 5=FATAL)",
        .default_value = 1,  // DEBUG
        .is_required = false
    });

    config.register_meta("logger.file_enabled", {
        .description = "Enable log file output",
        .default_value = true,
        .is_required = false
    });

    config.register_meta("logger.file_path", {
        .description = "Log file path (relative to executable or absolute)",
        .default_value = std::string("logs/app.log"),
        .is_required = false
    });

    // 从配置加载日志设置（现在配置已经加载了）
    int log_level = config.get_or<int>("logger.level", 1);  // DEBUG
    bool file_enabled = config.get_or<bool>("logger.file_enabled", true);
    std::string log_file_path = config.get_or<std::string>("logger.file_path", "logs/app.log");

    // 设置日志级别
    logger->set_level(static_cast<LogLevel>(log_level));

    // 构建完整的日志文件路径
    std::filesystem::path log_file;
    #ifdef _WIN32
    if (!std::filesystem::path(log_file_path).is_absolute()) {
        // 相对于可执行文件目录
        char exe_path[1024];
        GetModuleFileNameA(NULL, exe_path, sizeof(exe_path));
        std::filesystem::path exe_dir = std::filesystem::path(exe_path).parent_path();
        log_file = exe_dir / log_file_path;
    } else {
        log_file = log_file_path;
    }
    #else
    log_file = log_file_path;
    #endif

    // 启用文件输出（如果配置启用）
    if (file_enabled) {
        logger->enable_file_output(log_file.string(), true);
    }

    LOG_INFO("========================================");
    LOG_INFO("日志系统初始化完成");
    LOG_INFO("日志级别: {}", static_cast<int>(logger->get_level()));
    if (file_enabled) {
        LOG_INFO("日志文件: {}", log_file.string());
    } else {
        LOG_INFO("文件输出: 已禁用");
    }
    LOG_INFO("========================================");
}

/**
 * @brief 主函数
 */
int main(int argc, char* argv[]) {
    (void)argc;  // 未使用参数
    (void)argv;  // 未使用参数

    // 初始化日志系统
    init_logger();
    std::cout << "========================================" << std::endl;
    std::cout << "   DearTs 工具箱应用 v1.0.0" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "\n功能特性:" << std::endl;
    std::cout << "  • ImGui 界面框架" << std::endl;
    std::cout << "  • 主题切换支持" << std::endl;
    std::cout << "  • 快捷键系统" << std::endl;
    std::cout << "  • 命令面板 (Ctrl+P)" << std::endl;
    std::cout << "  • 自定义标题栏" << std::endl;
    std::cout << "  • 可停靠窗口" << std::endl;
    std::cout << "  • 配置管理" << std::endl;
    std::cout << "\n快捷键:" << std::endl;
    std::cout << "  Ctrl+O   - 打开文件" << std::endl;
    std::cout << "  Ctrl+S   - 保存文件" << std::endl;
    std::cout << "  Ctrl+Q   - 退出应用" << std::endl;
    std::cout << "  F11      - 切换全屏" << std::endl;
    std::cout << "  Ctrl+P   - 命令面板" << std::endl;
    std::cout << "\n========================================\n" << std::endl;

    // 配置应用程序
    Main::GUI::DearTsApplication app;
    Core::App::ApplicationConfig config;
    config.name = "DearTs 工具箱";
    config.version = "1.0.0";
    config.window_width = 1600;
    config.window_height = 900;
    config.enable_vsync = true;
    config.borderless = true;  // 启用无边框窗口（自定义标题栏）

    // 初始化
    std::cout << "[1/3] 初始化应用..." << std::endl;
    if (!app.initialize(config)) {
        std::cerr << "✗ 初始化失败！" << std::endl;
        return -1;
    }
    std::cout << "✓ 初始化成功" << std::endl;

    // 运行
    std::cout << "\n[2/3] 运行应用..." << std::endl;
    std::cout << "(关闭窗口或按 Ctrl+Q 退出)\n" << std::endl;
    int exit_code = app.run();

    // 关闭
    std::cout << "\n[3/3] 关闭应用..." << std::endl;
    app.shutdown();
    std::cout << "✓ 关闭完成" << std::endl;

    std::cout << "\n========================================" << std::endl;
    if (exit_code == 0) {
        std::cout << "✓ 程序正常退出" << std::endl;
    } else {
        std::cout << "✗ 程序异常退出 (代码: " << exit_code << ")" << std::endl;
    }
    std::cout << "========================================\n" << std::endl;

    return exit_code;
}
