/**
 * @file toast_examples.cpp
 * @brief Toast Notification 使用示例
 * @details 展示各种 Toast Notification 的使用场景
 */

#include "plugins/toast/toast_manager.hpp"
#include <thread>
#include <chrono>

using namespace DearTs::Plugins::Toast;

namespace Examples {

// ================ 示例 1: 基本使用 ================

void example_1_basic() {
    // 信息提示
    show_info("欢迎", "欢迎使用 DearTs Framework");

    // 成功提示
    show_success("完成", "操作已成功完成");

    // 警告提示
    show_warning("注意", "磁盘空间不足");

    // 错误提示
    show_error("错误", "无法连接到服务器");
}

// ================ 示例 2: 自定义时长 ================

void example_2_custom_duration() {
    // 短提示（2秒）
    ToastManager::instance().show(
        "已复制",
        "文本已复制到剪贴板",
        ToastType::Info,
        std::chrono::milliseconds(2000)
    );

    // 长提示（8秒）
    ToastManager::instance().show(
        "重要提示",
        "此操作不可撤销，请谨慎操作",
        ToastType::Warning,
        std::chrono::milliseconds(8000)
    );
}

// ================ 示例 3: 获取 ID 并控制 ================

void example_3_with_id() {
    // 显示 Toast 并获取 ID
    int toast_id = ToastManager::instance().info(
        "处理中",
        "正在处理您的请求..."
    );

    // 模拟处理
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // 关闭之前的 Toast
    ToastManager::instance().close(toast_id);

    // 显示完成提示
    ToastManager::instance().success("完成", "处理已完成");
}

// ================ 示例 4: 批量配置 ================

void example_4_configure() {
    // 临时修改配置
    ToastManager::instance().configure([](ToastConfig& config) {
        config.max_toasts = 3;           // 最多显示3个
        config.animation_speed = 5.0f;   // 更快的动画
        config.show_progress_bar = false; // 不显示进度条
    });

    // 使用新配置显示 Toast
    show_info("测试", "这是使用新配置的 Toast");

    // 恢复默认配置
    ToastManager::instance().configure([](ToastConfig& config) {
        config.max_toasts = 5;
        config.animation_speed = 3.0f;
        config.show_progress_bar = true;
    });
}

// ================ 示例 5: 文件操作 ================

void example_5_file_operations() {
    try {
        // 模拟保存文件
        std::string filename = "document.txt";

        // 显示开始提示
        ToastManager::instance().info(
            "保存中",
            "正在保存文件: " + filename
        );

        // 模拟保存操作
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // 显示成功提示
        ToastManager::instance().success(
            "保存成功",
            "文件已成功保存: " + filename
        );

    } catch (const std::exception& e) {
        // 显示错误提示
        ToastManager::instance().error(
            "保存失败",
            std::string("无法保存文件: ") + e.what()
        );
    }
}

// ================ 示例 6: 表单验证 ================

void example_6_form_validation() {
    std::string username = "";
    std::string email = "invalid-email";

    // 验证用户名 (intentionally empty for demo)
    if (username.empty()) {
        ToastManager::instance().warning(
            "验证失败",
            "请输入用户名"
        );
        return;
    }

    // 验证邮箱
    if (email.find('@') == std::string::npos) {
        ToastManager::instance().warning(
            "验证失败",
            "请输入有效的邮箱地址"
        );
        return;
    }

    // 验证通过
    ToastManager::instance().success(
        "注册成功",
        "您的账户已创建"
    );
}

// ================ 示例 7: 进度反馈 ================

void example_7_progress() {
    const int total_steps = 5;

    for (int i = 1; i <= total_steps; i++) {
        // 显示进度
        ToastManager::instance().info(
            "处理中",
            std::string("正在处理第 ") + std::to_string(i) + "/" +
            std::to_string(total_steps) + " 项"
        );

        // 模拟处理
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // 关闭之前的提示
        ToastManager::instance().close_all();
    }

    // 显示完成提示
    ToastManager::instance().success(
        "全部完成",
        "所有项目已处理完毕"
    );
}

// ================ 示例 8: 不同场景的最佳实践 ================

void example_8_best_practices() {
    // ✅ 短暂操作 - 短提示
    ToastManager::instance().success(
        "已保存",
        "更改已保存",
        std::chrono::milliseconds(2000)
    );

    std::this_thread::sleep_for(std::chrono::milliseconds(2500));

    // ✅ 重要信息 - 长提示
    ToastManager::instance().warning(
        "自动保存",
        "文档将在 5 分钟后自动保存",
        std::chrono::milliseconds(6000)
    );

    std::this_thread::sleep_for(std::chrono::milliseconds(6500));

    // ✅ 错误信息 - 允许用户查看
    ToastManager::instance().error(
        "加载失败",
        "无法加载配置文件，将使用默认配置",
        std::chrono::milliseconds(5000)
    );
}

// ================ 示例 9: 与任务系统结合 ================

void example_9_with_tasks() {
    // 注意：这需要 TaskManager 的支持
    // 以下是伪代码示例

    /*
    auto task = TaskManager::instance().launch("加载数据", [](auto& cancel) {
        // 开始提示
        run_on_main_thread([]() {
            ToastManager::instance().info("加载中", "正在从服务器获取数据...");
        });

        // 执行任务...
        auto result = fetch_data(cancel);

        if (cancel.is_cancelled()) {
            // 取消提示
            run_on_main_thread([]() {
                ToastManager::instance().warning("已取消", "加载操作已取消");
            });
            return;
        }

        if (result.is_error()) {
            // 错误提示
            run_on_main_thread([&]() {
                ToastManager::instance().error("加载失败", result.error());
            });
            return;
        }

        // 成功提示
        run_on_main_thread([&]() {
            ToastManager::instance().success(
                "加载完成",
                "成功加载 " + std::to_string(result.data.size()) + " 条数据"
            );
        });
    });
    */
}

// ================ 示例 10: 交互式操作 ================

void example_10_interactive() {
    // 启用点击关闭功能
    ToastManager::instance().configure([](ToastConfig& config) {
        config.click_to_close = true;
    });

    // 显示可点击关闭的 Toast
    ToastManager::instance().info(
        "点击关闭",
        "点击此消息可将其关闭"
    );

    // 用户可以点击 Toast 来关闭它

    // 恢复设置
    std::this_thread::sleep_for(std::chrono::seconds(3));
    ToastManager::instance().configure([](ToastConfig& config) {
        config.click_to_close = false;
    });
}

} // namespace Examples

// ================ 主函数：运行所有示例 ================

int main() {
    // 示例 1: 基本使用
    Examples::example_1_basic();
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // 示例 2: 自定义时长
    Examples::example_2_custom_duration();
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // 示例 3: 获取 ID 并控制
    Examples::example_3_with_id();
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // 示例 4: 批量配置
    Examples::example_4_configure();
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // 示例 5: 文件操作
    Examples::example_5_file_operations();
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // 示例 6: 表单验证
    Examples::example_6_form_validation();
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // 示例 7: 进度反馈
    Examples::example_7_progress();
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // 示例 8: 最佳实践
    Examples::example_8_best_practices();

    return 0;
}
