/**
 * @file task_manager.cpp
 * @brief 任务管理器实现
 */

#include "task_manager.h"
#include "logger.h"
#include <algorithm>

namespace DearTs::Core::Tasks {

// ================ Task Implementation ================

Task::Task(std::string name, TaskFunc func, float max_progress)
    : m_name(std::move(name))
    , m_func(std::move(func))
    , m_max_progress(max_progress)
    , m_should_cancel(false)
{
    LOG_DEBUG("Task created: {}", m_name);
}

Task::~Task() {
    LOG_DEBUG("Task destroyed: {}", m_name);
}

void Task::execute() {
    if (m_status != TaskStatus::Pending) {
        LOG_WARN("Task {} is not in pending state", m_name);
        return;
    }

    LOG_INFO("Task started: {}", m_name);
    m_status = TaskStatus::Running;

    try {
        m_func(m_should_cancel);

        if (!m_should_cancel) {
            markCompleted();
            LOG_INFO("Task completed: {}", m_name);
        } else {
            m_status = TaskStatus::Cancelled;
            LOG_INFO("Task cancelled: {}", m_name);
        }
    } catch (const std::exception& e) {
        markFailed();
        LOG_ERROR("Task failed: {} - {}", m_name, e.what());
    } catch (...) {
        markFailed();
        LOG_ERROR("Task failed with unknown exception: {}", m_name);
    }
}

// ================ TaskManager Implementation ================

TaskManager::~TaskManager() {
    LOG_INFO("TaskManager shutting down...");
    cancelAll();
    waitForAll();
    LOG_INFO("TaskManager shutdown complete");
}

std::shared_ptr<Task> TaskManager::create(
    const std::string& name,
    Task::TaskFunc func,
    TaskType type
) {
    auto task = std::make_shared<Task>(name, std::move(func), 100.0f);
    task->setType(type);

    std::lock_guard<std::mutex> lock(m_tasks_mutex);
    m_tasks.push_back(task);

    LOG_DEBUG("Task created and registered: {}", name);
    return task;
}

std::shared_ptr<Task> TaskManager::launch(
    const std::string& name,
    Task::TaskFunc func,
    TaskType type
) {
    auto task = create(name, std::move(func), type);
    start(task);
    return task;
}

void TaskManager::start(std::shared_ptr<Task> task) {
    if (!task) {
        LOG_ERROR("Cannot start null task");
        return;
    }

    if (task->getType() == TaskType::Blocking) {
        // 阻塞任务：在当前线程执行
        task->execute();
    } else {
        // 异步任务：在后台线程执行
        std::thread([task]() {
            task->execute();
        }).detach();
    }

    LOG_INFO("Task launched: {}", task->getName());
}

void TaskManager::cancel(std::shared_ptr<Task> task) {
    if (!task) {
        return;
    }

    task->cancel();
    LOG_INFO("Task cancellation requested: {}", task->getName());
}

std::vector<std::shared_ptr<Task>> TaskManager::getRunningTasks() const {
    std::lock_guard<std::mutex> lock(m_tasks_mutex);

    std::vector<std::shared_ptr<Task>> running_tasks;
    for (const auto& task : m_tasks) {
        if (task->isRunning()) {
            running_tasks.push_back(task);
        }
    }

    return running_tasks;
}

void TaskManager::update() {
    std::lock_guard<std::mutex> lock(m_tasks_mutex);

    // 移除已完成的非关键任务
    auto it = std::remove_if(m_tasks.begin(), m_tasks.end(),
        [](const std::shared_ptr<Task>& task) {
            // 保留关键任务和正在运行的任务
            if (task->getType() == TaskType::Critical) {
                return false;
            }
            // 移除已完成超过1秒的任务
            return task->isFinished();
        });

    if (it != m_tasks.end()) {
        m_tasks.erase(it, m_tasks.end());
    }
}

void TaskManager::waitForAll() {
    LOG_DEBUG("Waiting for all tasks to complete...");

    while (true) {
        {
            std::lock_guard<std::mutex> lock(m_tasks_mutex);
            bool all_finished = std::all_of(m_tasks.begin(), m_tasks.end(),
                [](const std::shared_ptr<Task>& task) {
                    return task->isFinished();
                });

            if (all_finished || m_tasks.empty()) {
                break;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    LOG_DEBUG("All tasks completed");
}

void TaskManager::cancelAll() {
    std::lock_guard<std::mutex> lock(m_tasks_mutex);

    LOG_INFO("Cancelling all tasks ({} tasks)", m_tasks.size());

    for (auto& task : m_tasks) {
        if (task->isRunning()) {
            task->cancel();
        }
    }
}

size_t TaskManager::getRunningTaskCount() const {
    std::lock_guard<std::mutex> lock(m_tasks_mutex);

    return std::count_if(m_tasks.begin(), m_tasks.end(),
        [](const std::shared_ptr<Task>& task) {
            return task->isRunning();
        });
}

} // namespace DearTs::Core::Tasks
