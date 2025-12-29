/**
 * @file task_manager.h
 * @brief 任务管理器
 * @details 管理异步任务执行、进度跟踪和取消
 * @author DearTs Team
 * @date 2024
 * @version 1.0.0
 */

#pragma once

#include <functional>
#include <string>
#include <memory>
#include <vector>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>

namespace DearTs::Core::Tasks {

/**
 * @brief 任务类型
 */
enum class TaskType {
    Normal,     ///< 普通任务
    Background, ///< 后台任务（不影响UI）
    Blocking,   ///< 阻塞任务（等待完成）
    Critical   ///< 关键任务（高优先级）
};

/**
 * @brief 任务状态
 */
enum class TaskStatus {
    Pending,    ///< 等待执行
    Running,    ///< 正在执行
    Completed,  ///< 已完成
    Cancelled,  ///< 已取消
    Failed      ///< 失败
};

/**
 * @brief 任务类
 */
class Task {
public:
    using TaskFunc = std::function<void(const std::atomic<bool>& should_cancel)>;

    /**
     * @brief 构造函数
     * @param name 任务名称
     * @param func 任务函数
     * @param max_progress 最大进度值（默认 100）
     */
    Task(std::string name, TaskFunc func, float max_progress = 100.0f);

    ~Task();

    /**
     * @brief 获取任务名称
     */
    [[nodiscard]] const std::string& getName() const { return m_name; }

    /**
     * @brief 获取任务类型
     */
    [[nodiscard]] TaskType getType() const { return m_type; }

    /**
     * @brief 设置任务类型
     */
    void setType(TaskType type) { m_type = type; }

    /**
     * @brief 获取任务状态
     */
    [[nodiscard]] TaskStatus getStatus() const { return m_status; }

    /**
     * @brief 获取进度值（0.0 - max_progress）
     */
    [[nodiscard]] float getProgress() const { return m_progress; }

    /**
     * @brief 获取最大进度值
     */
    [[nodiscard]] float getMaxProgress() const { return m_max_progress; }

    /**
     * @brief 获取进度百分比（0.0 - 1.0）
     */
    [[nodiscard]] float getProgressPercent() const {
        return m_max_progress > 0 ? m_progress / m_max_progress : 0;
    }

    /**
     * @brief 更新进度
     */
    void setProgress(float progress) {
        m_progress = progress;
        if (m_progress >= m_max_progress) {
            m_status = TaskStatus::Completed;
        }
    }

    /**
     * @brief 增加进度
     */
    void addProgress(float delta) {
        m_progress += delta;
        if (m_progress >= m_max_progress) {
            m_progress = m_max_progress;
            m_status = TaskStatus::Completed;
        }
    }

    /**
     * @brief 取消任务
     */
    void cancel() {
        m_should_cancel = true;
        m_status = TaskStatus::Cancelled;
    }

    /**
     * @brief 检查是否应该取消
     */
    [[nodiscard]] bool shouldCancel() const { return m_should_cancel; }

    /**
     * @brief 检查是否完成
     */
    [[nodiscard]] bool isFinished() const {
        return m_status == TaskStatus::Completed ||
               m_status == TaskStatus::Cancelled ||
               m_status == TaskStatus::Failed;
    }

    /**
     * @brief 检查是否正在运行
     */
    [[nodiscard]] bool isRunning() const {
        return m_status == TaskStatus::Running;
    }

    /**
     * @brief 设置完成回调
     */
    void onCompleted(std::function<void()> callback) {
        m_completed_callback = std::move(callback);
    }

private:
    /**
     * @brief 执行任务
     */
    void execute();

    /**
     * @brief 标记任务完成
     */
    void markCompleted() {
        m_status = TaskStatus::Completed;
        if (m_completed_callback) {
            m_completed_callback();
        }
    }

    /**
     * @brief 标记任务失败
     */
    void markFailed() {
        m_status = TaskStatus::Failed;
    }

private:
    std::string m_name;
    TaskFunc m_func;
    TaskType m_type = TaskType::Normal;
    TaskStatus m_status = TaskStatus::Pending;

    float m_progress = 0.0f;
    float m_max_progress;

    std::atomic<bool> m_should_cancel;
    std::function<void()> m_completed_callback;

    friend class TaskManager;
};

/**
 * @brief 任务管理器
 *
 * 管理任务的生命周期、执行和状态跟踪
 */
class TaskManager {
public:
    /**
     * @brief 获取单例实例
     */
    static TaskManager& instance() {
        static TaskManager inst;
        return inst;
    }

    /**
     * @brief 创建任务
     * @param name 任务名称
     * @param func 任务函数
     * @param type 任务类型
     * @return 任务指针
     */
    std::shared_ptr<Task> create(
        const std::string& name,
        Task::TaskFunc func,
        TaskType type = TaskType::Normal
    );

    /**
     * @brief 创建并立即启动任务
     * @param name 任务名称
     * @param func 任务函数
     * @param type 任务类型
     * @return 任务指针
     */
    std::shared_ptr<Task> launch(
        const std::string& name,
        Task::TaskFunc func,
        TaskType type = TaskType::Normal
    );

    /**
     * @brief 启动任务
     */
    void start(std::shared_ptr<Task> task);

    /**
     * @brief 取消任务
     */
    void cancel(std::shared_ptr<Task> task);

    /**
     * @brief 获取所有任务
     */
    [[nodiscard]] const std::vector<std::shared_ptr<Task>>& getTasks() const {
        return m_tasks;
    }

    /**
     * @brief 获取正在运行的任务
     */
    [[nodiscard]] std::vector<std::shared_ptr<Task>> getRunningTasks() const;

    /**
     * @brief 更新任务状态（在主循环中调用）
     * @details 清理已完成的任务
     */
    void update();

    /**
     * @brief 等待所有任务完成
     */
    void waitForAll();

    /**
     * @brief 取消所有任务
     */
    void cancelAll();

    /**
     * @brief 获取运行中的任务数量
     */
    [[nodiscard]] size_t getRunningTaskCount() const;

private:
    TaskManager() = default;
    ~TaskManager();

    // 禁止拷贝
    TaskManager(const TaskManager&) = delete;
    TaskManager& operator=(const TaskManager&) = delete;

private:
    std::vector<std::shared_ptr<Task>> m_tasks;
    mutable std::mutex m_tasks_mutex;
};

} // namespace DearTs::Core::Tasks
