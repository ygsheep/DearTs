/**
 * @file task_manager_test.cpp
 * @brief Unit tests for TaskManager and Task
 */

#include <gtest/gtest.h>
#include "core/tasks/task_manager.h"
#include "core/event/event_bus.h"
#include <thread>
#include <chrono>
#include <atomic>

using namespace DearTs::Core::Tasks;
using DearTs::Core::Event::EventBus;

// ============================================================================
// Task Test Fixture
// ============================================================================

class TaskTest : public ::testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override {}
};

// ============================================================================
// Task Basic Tests
// ============================================================================

TEST_F(TaskTest, TaskConstruction_CreatesValidTask) {
    bool executed = false;

    Task task("TestTask", [&](const std::atomic<bool>& should_cancel) {
        executed = true;
    }, EventBus::instance());

    EXPECT_EQ(task.getName(), "TestTask");
    EXPECT_FALSE(executed);  // Not executed yet
}

TEST_F(TaskTest, TaskGetName_ReturnsCorrectName) {
    Task task("MyTask", [&](const std::atomic<bool>& should_cancel) {}, EventBus::instance());

    EXPECT_EQ(task.getName(), "MyTask");
}

TEST_F(TaskTest, TaskGetType_ReturnsDefaultNormal) {
    Task task("TestTask", [&](const std::atomic<bool>& should_cancel) {}, EventBus::instance());

    EXPECT_EQ(task.getType(), TaskType::Normal);
}

TEST_F(TaskTest, TaskSetType_UpdatesType) {
    Task task("TestTask", [&](const std::atomic<bool>& should_cancel) {}, EventBus::instance());

    task.setType(TaskType::Background);
    EXPECT_EQ(task.getType(), TaskType::Background);

    task.setType(TaskType::Critical);
    EXPECT_EQ(task.getType(), TaskType::Critical);
}

// ============================================================================
// Task Status Tests
// ============================================================================

TEST_F(TaskTest, TaskInitialStatus_IsPending) {
    Task task("TestTask", [&](const std::atomic<bool>& should_cancel) {}, EventBus::instance());

    EXPECT_EQ(task.getStatus(), TaskStatus::Pending);
}

TEST_F(TaskTest, TaskExecute_TransitionsToRunningThenCompleted) {
    bool executed = false;

    // execute() 内部通过 shared_from_this() 发布事件，Task 必须由 shared_ptr 持有
    auto task = std::make_shared<Task>("TestTask", [&](const std::atomic<bool>& should_cancel) {
        executed = true;
    }, EventBus::instance());

    task->execute();

    EXPECT_TRUE(executed);
    EXPECT_EQ(task->getStatus(), TaskStatus::Completed);
    EXPECT_TRUE(task->isFinished());
}

TEST_F(TaskTest, TaskExecute_Twice_IgnoresSecondCall) {
    int counter = 0;

    auto task = std::make_shared<Task>("TestTask", [&](const std::atomic<bool>& should_cancel) {
        counter++;
    }, EventBus::instance());

    task->execute();
    task->execute();

    // 第二次 execute 因 CAS 被忽略，函数只执行一次
    EXPECT_EQ(counter, 1);
    EXPECT_EQ(task->getStatus(), TaskStatus::Completed);
}

TEST_F(TaskTest, TaskExecute_WhenCancelled_MarksCancelled) {
    auto task = std::make_shared<Task>("TestTask", [&](const std::atomic<bool>& should_cancel) {
        // 任务函数正常返回（协作式取消由 execute 检测标志）
    }, EventBus::instance());

    task->cancel();
    // cancel 仅设置标志，状态仍为 Pending
    EXPECT_EQ(task->getStatus(), TaskStatus::Pending);
    EXPECT_TRUE(task->shouldCancel());

    task->execute();

    EXPECT_EQ(task->getStatus(), TaskStatus::Cancelled);
    EXPECT_TRUE(task->isFinished());
}

TEST_F(TaskTest, TaskExecute_WhenException_MarksFailed) {
    auto task = std::make_shared<Task>("TestTask", [&](const std::atomic<bool>& should_cancel) {
        throw std::runtime_error("boom");
    }, EventBus::instance());

    task->execute();

    EXPECT_EQ(task->getStatus(), TaskStatus::Failed);
    EXPECT_TRUE(task->isFinished());
}

TEST_F(TaskTest, TaskExecute_WhenUnknownException_MarksFailed) {
    auto task = std::make_shared<Task>("TestTask", [&](const std::atomic<bool>& should_cancel) {
        throw 42;
    }, EventBus::instance());

    task->execute();

    EXPECT_EQ(task->getStatus(), TaskStatus::Failed);
    EXPECT_TRUE(task->isFinished());
}

TEST_F(TaskTest, TaskCompleted_StatusChangesToCompleted) {
    Task task("TestTask", [&](const std::atomic<bool>& should_cancel) {
        // Task completes immediately
    }, EventBus::instance(), {}, 100.0f);

    task.setProgress(100.0f);
    EXPECT_EQ(task.getStatus(), TaskStatus::Completed);
    EXPECT_TRUE(task.isFinished());
}

// ============================================================================
// Task Progress Tests
// ============================================================================

TEST_F(TaskTest, TaskSetProgress_UpdatesProgress) {
    Task task("TestTask", [&](const std::atomic<bool>& should_cancel) {}, EventBus::instance(), {}, 100.0f);

    task.setProgress(50.0f);
    EXPECT_EQ(task.getProgress(), 50.0f);
}

TEST_F(TaskTest, TaskSetProgress_ExceedsMax_SetsToMax) {
    Task task("TestTask", [&](const std::atomic<bool>& should_cancel) {}, EventBus::instance(), {}, 100.0f);

    task.setProgress(150.0f);
    EXPECT_EQ(task.getProgress(), 100.0f);
}

TEST_F(TaskTest, TaskAddProgress_IncrementsProgress) {
    Task task("TestTask", [&](const std::atomic<bool>& should_cancel) {}, EventBus::instance(), {}, 100.0f);

    task.setProgress(30.0f);
    task.addProgress(20.0f);

    EXPECT_EQ(task.getProgress(), 50.0f);
}

TEST_F(TaskTest, TaskAddProgress_ClampedToMax) {
    Task task("TestTask", [&](const std::atomic<bool>& should_cancel) {}, EventBus::instance(), {}, 100.0f);

    task.addProgress(150.0f);
    EXPECT_EQ(task.getProgress(), 100.0f);
}

TEST_F(TaskTest, TaskGetProgressPercent_ReturnsCorrectPercentage) {
    Task task("TestTask", [&](const std::atomic<bool>& should_cancel) {}, EventBus::instance(), {}, 100.0f);

    task.setProgress(50.0f);
    EXPECT_FLOAT_EQ(task.getProgressPercent(), 0.5f);

    task.setProgress(100.0f);
    EXPECT_FLOAT_EQ(task.getProgressPercent(), 1.0f);
}

TEST_F(TaskTest, TaskGetProgressPercent_ZeroMaxProgress_ReturnsZero) {
    Task task("TestTask", [&](const std::atomic<bool>& should_cancel) {}, EventBus::instance(), {}, 0.0f);

    task.setProgress(50.0f);
    EXPECT_FLOAT_EQ(task.getProgressPercent(), 0.0f);
}

TEST_F(TaskTest, TaskProgressReachesMax_StatusBecomesCompleted) {
    Task task("TestTask", [&](const std::atomic<bool>& should_cancel) {}, EventBus::instance(), {}, 100.0f);

    EXPECT_EQ(task.getStatus(), TaskStatus::Pending);

    task.setProgress(100.0f);

    EXPECT_EQ(task.getStatus(), TaskStatus::Completed);
    EXPECT_TRUE(task.isFinished());
}

// ============================================================================
// Task Cancellation Tests
// ============================================================================

TEST_F(TaskTest, TaskCancel_SetsCancelFlag) {
    Task task("TestTask", [&](const std::atomic<bool>& should_cancel) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }, EventBus::instance());

    task.cancel();

    // 协作式取消：cancel 只设置标志，状态由 execute() 决定
    EXPECT_TRUE(task.shouldCancel());
    EXPECT_EQ(task.getStatus(), TaskStatus::Pending);
}

TEST_F(TaskTest, TaskShouldCancel_ReturnsCancelFlag) {
    Task task("TestTask", [&](const std::atomic<bool>& should_cancel) {}, EventBus::instance());

    EXPECT_FALSE(task.shouldCancel());

    task.cancel();

    EXPECT_TRUE(task.shouldCancel());
}

TEST_F(TaskTest, TaskFunctionRespectsCancelFlag) {
    bool cancelled_checked = false;

    auto task = std::make_shared<Task>("TestTask", [&](const std::atomic<bool>& should_cancel) {
        while (!should_cancel) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        cancelled_checked = true;
    }, EventBus::instance());

    task->cancel();
    task->execute();

    EXPECT_TRUE(cancelled_checked);
    EXPECT_TRUE(task->shouldCancel());
    EXPECT_EQ(task->getStatus(), TaskStatus::Cancelled);
}

// ============================================================================
// Task Finished Tests
// ============================================================================

TEST_F(TaskTest, TaskIsFinished_Completed_ReturnsTrue) {
    Task task("TestTask", [&](const std::atomic<bool>& should_cancel) {}, EventBus::instance(), {}, 100.0f);
    task.setProgress(100.0f);

    EXPECT_TRUE(task.isFinished());
}

TEST_F(TaskTest, TaskIsFinished_Cancelled_ReturnsTrue) {
    auto task = std::make_shared<Task>("TestTask", [&](const std::atomic<bool>& should_cancel) {}, EventBus::instance());
    task->cancel();
    task->execute();

    EXPECT_TRUE(task->isFinished());
}

TEST_F(TaskTest, TaskIsFinished_Pending_ReturnsFalse) {
    Task task("TestTask", [&](const std::atomic<bool>& should_cancel) {}, EventBus::instance());

    EXPECT_FALSE(task.isFinished());
}

// ============================================================================
// TaskManager Tests
// ============================================================================

class TaskManagerTest : public ::testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override {}
};

TEST_F(TaskManagerTest, LaunchNormalTask_CreatesTask) {
    bool executed = false;

    auto task = TaskManager::instance().launch("TestTask", [&](const std::atomic<bool>& should_cancel) {
        executed = true;
    });

    ASSERT_NE(task, nullptr);
    EXPECT_EQ(task->getName(), "TestTask");

    // 等待任务完成
    TaskManager::instance().wait(task);
    EXPECT_TRUE(executed);
    EXPECT_TRUE(task->isFinished());
}

TEST_F(TaskManagerTest, LaunchTask_ExecutesFunction) {
    std::atomic<int> counter{0};

    auto task = TaskManager::instance().launch("CounterTask", [&](const std::atomic<bool>& should_cancel) {
        for (int i = 0; i < 10; ++i) {
            counter++;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    TaskManager::instance().wait(task);
    EXPECT_EQ(counter.load(), 10);
}

TEST_F(TaskManagerTest, LaunchMultipleTasks_AllExecute) {
    std::atomic<int> count{0};
    std::vector<std::shared_ptr<Task>> tasks;

    for (int i = 0; i < 5; ++i) {
        tasks.push_back(TaskManager::instance().launch("Task" + std::to_string(i), [&](const std::atomic<bool>& should_cancel) {
            count++;
        }));
    }

    for (auto& t : tasks) {
        TaskManager::instance().wait(t);
    }

    EXPECT_EQ(count.load(), 5);
}

TEST_F(TaskManagerTest, CancelTask_StopsExecution) {
    std::atomic<int> counter{0};

    auto task = TaskManager::instance().launch("LongTask", [&](const std::atomic<bool>& should_cancel) {
        while (!should_cancel && counter < 1000) {
            counter++;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    // Cancel after a short delay
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    task->cancel();

    TaskManager::instance().wait(task);

    // Should have been cancelled before reaching 1000
    EXPECT_LT(counter.load(), 1000);
    EXPECT_EQ(task->getStatus(), TaskStatus::Cancelled);
}

TEST_F(TaskManagerTest, TaskProgress_UpdatesCorrectly) {
    auto task = TaskManager::instance().launch("ProgressTask", [&](const std::atomic<bool>& cancel) {
        for (int i = 0; i <= 100; i += 10) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }, TaskType::Normal);

    TaskManager::instance().wait(task);

    EXPECT_EQ(task->getProgress(), 100.0f);
    EXPECT_TRUE(task->isFinished());
}

// ============================================================================
// Task Type Tests
// ============================================================================

TEST_F(TaskManagerTest, LaunchBackgroundTask_HasCorrectType) {
    auto task = TaskManager::instance().launch(
        "BackgroundTask",
        [&](const std::atomic<bool>& cancel) {},
        TaskType::Background
    );

    EXPECT_EQ(task->getType(), TaskType::Background);
}

TEST_F(TaskManagerTest, LaunchCriticalTask_HasCorrectType) {
    auto task = TaskManager::instance().launch(
        "CriticalTask",
        [&](const std::atomic<bool>& cancel) {},
        TaskType::Critical
    );

    EXPECT_EQ(task->getType(), TaskType::Critical);
}

TEST_F(TaskManagerTest, LaunchBlockingTask_WaitsForCompletion) {
    bool executed = false;

    auto task = TaskManager::instance().launch(
        "BlockingTask",
        [&](const std::atomic<bool>& cancel) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            executed = true;
        },
        TaskType::Blocking
    );

    // Blocking task 同步执行，返回时应已完成
    EXPECT_TRUE(task->isFinished());
    EXPECT_TRUE(executed);
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

TEST_F(TaskManagerTest, ConcurrentLaunch_ThreadSafe) {
    const int num_threads = 10;
    std::vector<std::thread> threads;
    std::atomic<int> task_count{0};

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([i, &task_count]() {
            TaskManager::instance().launch("ConcurrentTask" + std::to_string(i), [&](const std::atomic<bool>& cancel) {
                task_count++;
            });
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // 等待所有任务完成
    TaskManager::instance().waitForAll();

    EXPECT_EQ(task_count.load(), num_threads);
}

// ============================================================================
// ThreadPool Tests
// ============================================================================

TEST_F(TaskManagerTest, ThreadPool_WorkersStarted) {
    // hardware_concurrency() 至少为 1
    EXPECT_GE(TaskManager::instance().worker_count(), 1u);
}

// ============================================================================
// DI / ITaskManager Tests
// ============================================================================

class MockTaskManager : public ITaskManager {
public:
    std::shared_ptr<Task> last_task;

    std::shared_ptr<Task> create(const std::string& name, Task::TaskFunc func, TaskType type = TaskType::Normal) override {
        last_task = std::make_shared<Task>(name, std::move(func), EventBus::instance());
        last_task->setType(type);
        return last_task;
    }

    std::shared_ptr<Task> launch(const std::string& name, Task::TaskFunc func, TaskType type = TaskType::Normal) override {
        return create(name, std::move(func), type);
    }

    void start(std::shared_ptr<Task> task) override {
        last_task = std::move(task);
    }

    void cancel(std::shared_ptr<Task> task) override {}

    std::vector<std::shared_ptr<Task>> getTasks() const override { return {}; }
    std::vector<std::shared_ptr<Task>> getRunningTasks() const override { return {}; }
    size_t getRunningTaskCount() const override { return 0; }
    void update() override {}
    void waitForAll() override {}
    void cancelAll() override {}
    void wait(std::shared_ptr<Task> task) override {}
};

TEST_F(TaskManagerTest, ITaskManager_CanBeMocked) {
    MockTaskManager mock;

    auto task = mock.create("Mocked", [&](const std::atomic<bool>& should_cancel) {}, TaskType::Background);

    ASSERT_NE(task, nullptr);
    EXPECT_EQ(task->getName(), "Mocked");
    EXPECT_EQ(task->getType(), TaskType::Background);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(TaskManagerTest, LaunchTaskWithEmptyName_Works) {
    auto task = TaskManager::instance().launch("", [&](const std::atomic<bool>& cancel) {});

    ASSERT_NE(task, nullptr);
    TaskManager::instance().wait(task);
}

TEST_F(TaskManagerTest, LaunchTaskWithImmediateCompletion) {
    bool executed = false;

    auto task = TaskManager::instance().launch("ImmediateTask", [&](const std::atomic<bool>& cancel) {
        executed = true;
    });

    TaskManager::instance().wait(task);

    EXPECT_TRUE(executed);
    EXPECT_TRUE(task->isFinished());
}

TEST_F(TaskManagerTest, TaskWithException_DoesNotCrash) {
    auto task = TaskManager::instance().launch("ExceptionTask", [&](const std::atomic<bool>& cancel) {
        throw std::runtime_error("Test exception");
    });

    TaskManager::instance().wait(task);

    EXPECT_EQ(task->getStatus(), TaskStatus::Failed);
    EXPECT_TRUE(task->isFinished());
}

TEST_F(TaskManagerTest, Wait_NullTask_ReturnsImmediately) {
    // 不应崩溃
    TaskManager::instance().wait(nullptr);
}
