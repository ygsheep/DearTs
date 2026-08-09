/**
 * @file task_manager.h
 * @brief 异步任务管理器
 * @details 管理任务执行、进度跟踪和协作式取消
 *          - 使用 ThreadPool 替代裸 std::thread（限制并发）
 *          - 抽取 ITaskManager 接口支持 DI
 *          - 状态字段原子化，消除数据竞争
 * @author DearTs Team
 * @date 2025
 * @version 2.0.0
 */

#pragma once

#include <algorithm>
#include <functional>
#include <string>
#include <memory>
#include <vector>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <thread>
#include <type_traits>

#include "task_events.h"
#include "core/event/event_bus.h"
#include "core/event/i_event_bus.h"
#include "core/tasks/thread_pool.h"

namespace DearTs::Core::Tasks {

/**
 * @brief 任务类型
 */
enum class TaskType {
    Normal,     ///< 普通任务
    Background, ///< 后台任务（不影响UI）
    Blocking,   ///< 阻塞任务（等待完成）
    Critical    ///< 关键任务（高优先级）
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

class TaskManager;

/**
 * @brief 任务类
 */
class Task : public std::enable_shared_from_this<Task> {
public:
    using TaskFunc = std::function<void(const std::atomic<bool>& should_cancel)>;
    using FinishedCallback = std::function<void()>;

    /**
     * @brief 构造函数
     * @param name 任务名称
     * @param func 任务函数
     * @param event_bus 事件总线引用（DI，通过它发布生命周期事件）
     * @param on_finished 任务结束时的通知回调（用于唤醒 TaskManager 等待）
     * @param max_progress 最大进度值（默认 100）
     */
    Task(std::string name, TaskFunc func,
         Event::IEventBus& event_bus,
         FinishedCallback on_finished = {},
         float max_progress = 100.0f);

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
    void setType(const TaskType& type) { m_type = type; }

    /**
     * @brief 获取任务状态（线程安全，原子读取）
     */
    [[nodiscard]] TaskStatus getStatus() const {
        return m_status.load(std::memory_order_acquire);
    }

    /**
     * @brief 获取进度值（线程安全，原子读取）
     */
    [[nodiscard]] float getProgress() const {
        return m_progress.load(std::memory_order_relaxed);
    }

    /**
     * @brief 获取最大进度值（线程安全，原子读取）
     */
    [[nodiscard]] float getMaxProgress() const {
        return m_max_progress.load(std::memory_order_relaxed);
    }

    /**
     * @brief 获取进度百分比（0.0 - 1.0，线程安全）
     */
    [[nodiscard]] float getProgressPercent() const {
        const float max = m_max_progress.load(std::memory_order_relaxed);
        if (max <= 0) return 0;
        return m_progress.load(std::memory_order_relaxed) / max;
    }

    /**
     * @brief 设置进度（线程安全）。达到上限时自动标记 Completed
     */
    void setProgress(float progress) {
        const float max = m_max_progress.load(std::memory_order_relaxed);
        const float clamped = std::min(progress, max);
        m_progress.store(clamped, std::memory_order_relaxed);
        if (clamped >= max) {
            m_status.store(TaskStatus::Completed, std::memory_order_release);
        }
    }

    /**
     * @brief 增量更新进度（线程安全，CAS 循环保证并发不丢更新）
     */
    void addProgress(float delta) {
        const float max = m_max_progress.load(std::memory_order_relaxed);
        float current = m_progress.load(std::memory_order_relaxed);
        float next;
        do {
            next = std::min(current + delta, max);
        } while (!m_progress.compare_exchange_weak(current, next,
            std::memory_order_relaxed, std::memory_order_relaxed));
        if (next >= max) {
            m_status.store(TaskStatus::Completed, std::memory_order_release);
        }
    }

    /**
     * @brief 取消任务（协作式）
     * @details 仅设置取消请求标志，不立即修改 status；由 execute() 检测后置 Cancelled
     */
    void cancel() {
        m_should_cancel.store(true, std::memory_order_release);
    }

    /**
     * @brief 检查是否应该取消（线程安全）
     */
    [[nodiscard]] bool shouldCancel() const {
        return m_should_cancel.load(std::memory_order_acquire);
    }

    /**
     * @brief 检查是否完成（线程安全）
     */
    [[nodiscard]] bool isFinished() const {
        const auto s = m_status.load(std::memory_order_acquire);
        return s == TaskStatus::Completed ||
               s == TaskStatus::Cancelled ||
               s == TaskStatus::Failed;
    }

    /**
     * @brief 检查是否正在运行（线程安全）
     */
    [[nodiscard]] bool isRunning() const {
        return m_status.load(std::memory_order_acquire) == TaskStatus::Running;
    }

    /**
     * @brief 设置完成回调
     */
    void onCompleted(std::function<void()> callback) {
        m_completed_callback = std::move(callback);
    }

    /**
     * @brief 执行任务（状态机入口）
     * @details 状态转换：Pending → Running → (Completed|Cancelled|Failed)。
     *          使用 CAS 保证仅处于 Pending 时才进入 Running，避免重复 execute。
     */
    void execute();

    /**
     * @brief 读取开始时间点（线程安全）
     */
    [[nodiscard]] std::chrono::steady_clock::time_point start_time() const noexcept {
        return std::chrono::steady_clock::time_point{
            std::chrono::nanoseconds{m_start_time_ns.load(std::memory_order_relaxed)}
        };
    }

private:
    /**
     * @brief 标记任务完成
     */
    void markCompleted();

    /**
     * @brief 标记任务失败
     */
    void markFailed(const std::string& error_message = "Unknown error");

private:
    std::string m_name;
    TaskFunc m_func;
    TaskType m_type = TaskType::Normal;

    // 状态与进度字段全部原子化，消除 setProgress/execute 间的数据竞争
    std::atomic<TaskStatus> m_status{TaskStatus::Pending};
    std::atomic<float> m_progress{0.0F};
    std::atomic<float> m_max_progress;

    std::atomic<bool> m_should_cancel{false};
    std::function<void()> m_completed_callback;

    // start_time 原子化（存 nanoseconds since epoch），消除潜在数据竞争
    std::atomic<int64_t> m_start_time_ns{0};

    // DI：注入的事件总线与结束回调
    Event::IEventBus& m_event_bus;
    FinishedCallback m_on_finished;
};

// ============================================================
// ITaskManager 接口（DI 化）
// ============================================================

/**
 * @brief 任务管理器抽象接口
 * @details 允许测试注入 MockTaskManager，解除对单例的硬依赖。
 *          生产代码用 TaskManager（继承 ITaskManager）。
 */
class ITaskManager {
public:
    virtual ~ITaskManager() = default;

    virtual std::shared_ptr<Task> create(
        const std::string& name,
        Task::TaskFunc func,
        TaskType type = TaskType::Normal) = 0;

    virtual std::shared_ptr<Task> launch(
        const std::string& name,
        Task::TaskFunc func,
        TaskType type = TaskType::Normal) = 0;

    virtual void start(std::shared_ptr<Task> task) = 0;
    virtual void cancel(std::shared_ptr<Task> task) = 0;

    [[nodiscard]] virtual std::vector<std::shared_ptr<Task>> getTasks() const = 0;
    [[nodiscard]] virtual std::vector<std::shared_ptr<Task>> getRunningTasks() const = 0;
    [[nodiscard]] virtual size_t getRunningTaskCount() const = 0;

    virtual void update() = 0;
    virtual void waitForAll() = 0;
    virtual void cancelAll() = 0;

    /// @brief 等待指定任务结束
    /// @param task 待等待的任务
    virtual void wait(std::shared_ptr<Task> task) = 0;
};

// ============================================================
// TaskManager 默认实现（基于 ThreadPool）
// ============================================================

/**
 * @brief 任务管理器
 *
 * 管理任务的生命周期、执行和状态跟踪
 */
class TaskManager final : public ITaskManager {  // 单例类，禁止继承
public:
    /**
     * @brief 获取单例实例（线程安全，Magic Statics）
     */
    static TaskManager& instance() noexcept {
        static TaskManager inst(Event::EventBus::instance());
        return inst;
    }

    // 删除所有拷贝和移动操作
    TaskManager(const TaskManager&) = delete;
    TaskManager& operator=(const TaskManager&) = delete;
    TaskManager(TaskManager&&) = delete;
    TaskManager& operator=(TaskManager&&) = delete;

    /**
     * @brief 构造（DI：显式注入 IEventBus）
     */
    explicit TaskManager(Event::IEventBus& event_bus)
        : m_pool(0), m_event_bus(event_bus) {}

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
    ) override;

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
    ) override;

    /**
     * @brief 启动任务
     */
    void start(std::shared_ptr<Task> task) override;

    /**
     * @brief 取消任务
     */
    void cancel(std::shared_ptr<Task> task) override;

    /**
     * @brief 获取所有任务（返回拷贝，线程安全）
     */
    [[nodiscard]] std::vector<std::shared_ptr<Task>> getTasks() const override;

    /**
     * @brief 获取正在运行的任务
     */
    [[nodiscard]] std::vector<std::shared_ptr<Task>> getRunningTasks() const override;

    /**
     * @brief 获取运行中的任务数量
     */
    [[nodiscard]] size_t getRunningTaskCount() const override;

    /**
     * @brief 更新任务状态（在主循环中调用）
     * @details 清理已完成的任务
     */
    void update() override;

    /**
     * @brief 等待所有任务完成
     */
    void waitForAll() override;

    /**
     * @brief 取消所有任务
     */
    void cancelAll() override;

    /**
     * @brief 等待指定任务结束（30 秒兜底超时）
     */
    void wait(std::shared_ptr<Task> task) override;

    /**
     * @brief 工作线程数（诊断 / 测试用）
     */
    [[nodiscard]] size_t worker_count() const noexcept { return m_pool.worker_count(); }

    /**
     * @brief 队列积压数（诊断用）
     */
    [[nodiscard]] size_t pending_count() const { return m_pool.pending_count(); }

private:
    ~TaskManager() override;

private:
    std::vector<std::shared_ptr<Task>> m_entries;
    mutable std::mutex m_tasks_mutex;       // 保护 m_entries / 任务状态查询
    std::condition_variable m_tasks_cv;     // waitForAll 等待用
    std::mutex m_wait_mutex;                // wait(task) 独立锁，避免与 m_tasks_mutex 死锁
    std::condition_variable m_wait_cv;      // wait(task) 独立 cv，由 m_on_finished notify
    ThreadPool m_pool;
    Event::IEventBus& m_event_bus;
};

} // namespace DearTs::Core::Tasks
