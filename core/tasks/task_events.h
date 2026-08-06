/**
 * @file task_events.h
 * @brief 任务相关事件定义
 * @details 定义任务生命周期事件，用于任务系统与其他组件的通信
 * @author DearTs Team
 * @date 2025
 * @version 1.0.0
 */

#pragma once

#include <string>
#include <memory>

namespace DearTs::Core::Tasks {

class Task;

/**
 * @brief 任务开始事件
 */
struct TaskStartedEvent {
    std::shared_ptr<Task> task;  ///< 任务指针
    std::string task_name;       ///< 任务名称
};

/**
 * @brief 任务进度更新事件
 */
struct TaskProgressEvent {
    std::shared_ptr<Task> task;  ///< 任务指针
    std::string task_name;       ///< 任务名称
    float progress;              ///< 当前进度值
    float progress_percent;      ///< 进度百分比 (0.0 - 100.0)
};

/**
 * @brief 任务完成事件
 */
struct TaskCompletedEvent {
    std::shared_ptr<Task> task;  ///< 任务指针
    std::string task_name;       ///< 任务名称
    float duration_ms;           ///< 执行耗时（毫秒）
};

/**
 * @brief 任务失败事件
 */
struct TaskFailedEvent {
    std::shared_ptr<Task> task;  ///< 任务指针
    std::string task_name;       ///< 任务名称
    std::string error_message;   ///< 错误信息
    float duration_ms;           ///< 执行耗时（毫秒）
};

/**
 * @brief 任务取消事件
 */
struct TaskCancelledEvent {
    std::shared_ptr<Task> task;  ///< 任务指针
    std::string task_name;       ///< 任务名称
    float duration_ms;           ///< 执行耗时（毫秒）
};

} // namespace DearTs::Core::Tasks
