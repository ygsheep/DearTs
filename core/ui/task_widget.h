/**
 * @file task_widget.h
 * @brief 任务管理器 UI 组件
 * @details 提供任务进度显示和管理的 UI 界面
 * @author DearTs Team
 * @date 2025
 * @version 1.0.0
 */

#pragma once

#include <imgui.h>
#include "../tasks/task_manager.h"

namespace DearTs::Core::UI {

/**
 * @brief 任务窗口组件
 *
 * 提供任务管理器的 UI 界面：
 * - 显示正在运行的任务
 * - 显示任务进度条
 * - 支持取消任务
 */
class TaskWidget {
public:
    /**
     * @brief 渲染任务窗口
     */
    static void render();

    /**
     * @brief 渲染单个任务
     * @param task 任务指针
     */
    static void renderTask(const std::shared_ptr<Tasks::Task>& task);

    /**
     * @brief 渲染紧凑的任务指示器（在状态栏中显示）
     */
    static void renderCompact();

private:
    /**
     * @brief 获取任务状态文本
     */
    static const char* getStatusText(Tasks::TaskStatus status);

    /**
     * @brief 获取任务类型文本
     */
    static const char* getTypeText(Tasks::TaskType type);

    /**
     * @brief 获取任务状态颜色
     */
    static ImVec4 getStatusColor(Tasks::TaskStatus status);
};

} // namespace DearTs::Core::UI
