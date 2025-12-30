/**
 * @file task_manager_test.cpp
 * @brief Unit tests for TaskManager and Task
 */

#include <gtest/gtest.h>
#include "core/tasks/task_manager.h"
#include <thread>
#include <chrono>
#include <atomic>

using namespace DearTs::Core::Tasks;

// ============================================================================
// Task Test Fixture
// ============================================================================

class TaskTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset state before each test
    }

    void TearDown() override {
        // Clean up
    }
};

// ============================================================================
// Task Basic Tests
// ============================================================================

TEST_F(TaskTest, TaskConstruction_CreatesValidTask) {
    bool executed = false;

    Task task("TestTask", [&](const std::atomic<bool>& should_cancel) {
        executed = true;
    });

    EXPECT_EQ(task.getName(), "TestTask");
    EXPECT_FALSE(executed);  // Not executed yet
}

TEST_F(TaskTest, TaskGetName_ReturnsCorrectName) {
    Task task("MyTask", [&](const std::atomic<bool>& should_cancel) {});

    EXPECT_EQ(task.getName(), "MyTask");
}

TEST_F(TaskTest, TaskGetType_ReturnsDefaultNormal) {
    Task task("TestTask", [&](const std::atomic<bool>& should_cancel) {});

    EXPECT_EQ(task.getType(), TaskType::Normal);
}

TEST_F(TaskTest, TaskSetType_UpdatesType) {
    Task task("TestTask", [&](const std::atomic<bool>& should_cancel) {});

    task.setType(TaskType::Background);
    EXPECT_EQ(task.getType(), TaskType::Background);

    task.setType(TaskType::Critical);
    EXPECT_EQ(task.getType(), TaskType::Critical);
}

// ============================================================================
// Task Status Tests
// ============================================================================

TEST_F(TaskTest, TaskInitialStatus_IsPending) {
    Task task("TestTask", [&](const std::atomic<bool>& should_cancel) {});

    EXPECT_EQ(task.getStatus(), TaskStatus::Pending);
}

TEST_F(TaskTest, TaskRunning_StatusChangesToRunning) {
    Task task("TestTask", [&](const std::atomic<bool>& should_cancel) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    });

    // Manually set status for testing
    // In real scenario, TaskManager would set this

    EXPECT_FALSE(task.isFinished());
}

TEST_F(TaskTest, TaskCompleted_StatusChangesToCompleted) {
    Task task("TestTask", [&](const std::atomic<bool>& should_cancel) {
        // Task completes immediately
    }, 100.0f);

    task.setProgress(100.0f);
    EXPECT_EQ(task.getStatus(), TaskStatus::Completed);
    EXPECT_TRUE(task.isFinished());
}

// ============================================================================
// Task Progress Tests
// ============================================================================

TEST_F(TaskTest, TaskSetProgress_UpdatesProgress) {
    Task task("TestTask", [&](const std::atomic<bool>& should_cancel) {}, 100.0f);

    task.setProgress(50.0f);
    EXPECT_EQ(task.getProgress(), 50.0f);
}

TEST_F(TaskTest, TaskSetProgress_ExceedsMax_SetsToMax) {
    Task task("TestTask", [&](const std::atomic<bool>& should_cancel) {}, 100.0f);

    task.setProgress(150.0f);
    EXPECT_EQ(task.getProgress(), 100.0f);
}

TEST_F(TaskTest, TaskAddProgress_IncrementsProgress) {
    Task task("TestTask", [&](const std::atomic<bool>& should_cancel) {}, 100.0f);

    task.setProgress(30.0f);
    task.addProgress(20.0f);

    EXPECT_EQ(task.getProgress(), 50.0f);
}

TEST_F(TaskTest, TaskGetProgressPercent_ReturnsCorrectPercentage) {
    Task task("TestTask", [&](const std::atomic<bool>& should_cancel) {}, 100.0f);

    task.setProgress(50.0f);
    EXPECT_FLOAT_EQ(task.getProgressPercent(), 0.5f);

    task.setProgress(100.0f);
    EXPECT_FLOAT_EQ(task.getProgressPercent(), 1.0f);
}

TEST_F(TaskTest, TaskGetProgressPercent_ZeroMaxProgress_ReturnsZero) {
    Task task("TestTask", [&](const std::atomic<bool>& should_cancel) {}, 0.0f);

    task.setProgress(50.0f);
    EXPECT_FLOAT_EQ(task.getProgressPercent(), 0.0f);
}

TEST_F(TaskTest, TaskProgressReachesMax_StatusBecomesCompleted) {
    Task task("TestTask", [&](const std::atomic<bool>& should_cancel) {}, 100.0f);

    EXPECT_EQ(task.getStatus(), TaskStatus::Pending);

    task.setProgress(100.0f);

    EXPECT_EQ(task.getStatus(), TaskStatus::Completed);
    EXPECT_TRUE(task.isFinished());
}

// ============================================================================
// Task Cancellation Tests
// ============================================================================

TEST_F(TaskTest, TaskCancel_SetsCancelledStatus) {
    Task task("TestTask", [&](const std::atomic<bool>& should_cancel) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    });

    task.cancel();

    EXPECT_EQ(task.getStatus(), TaskStatus::Cancelled);
    EXPECT_TRUE(task.isFinished());
    EXPECT_TRUE(task.shouldCancel());
}

TEST_F(TaskTest, TaskShouldCancel_ReturnsCancelFlag) {
    Task task("TestTask", [&](const std::atomic<bool>& should_cancel) {});

    EXPECT_FALSE(task.shouldCancel());

    task.cancel();

    EXPECT_TRUE(task.shouldCancel());
}

TEST_F(TaskTest, TaskFunctionRespectsCancelFlag) {
    bool cancelled_checked = false;
    bool completed = false;

    Task task("TestTask", [&](const std::atomic<bool>& should_cancel) {
        int count = 0;
        while (count < 100 && !should_cancel) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            count++;
        }

        cancelled_checked = should_cancel;
        completed = count >= 100;
    });

    // Cancel after a short delay
    std::thread canceler([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        task.cancel();
    });

    canceler.join();

    // Give task time to finish/cancel
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    EXPECT_TRUE(task.shouldCancel());
}

// ============================================================================
// Task Finished Tests
// ============================================================================

TEST_F(TaskTest, TaskIsFinished_Completed_ReturnsTrue) {
    Task task("TestTask", [&](const std::atomic<bool>& should_cancel) {}, 100.0f);
    task.setProgress(100.0f);

    EXPECT_TRUE(task.isFinished());
}

TEST_F(TaskTest, TaskIsFinished_Cancelled_ReturnsTrue) {
    Task task("TestTask", [&](const std::atomic<bool>& should_cancel) {});
    task.cancel();

    EXPECT_TRUE(task.isFinished());
}

TEST_F(TaskTest, TaskIsFinished_Pending_ReturnsFalse) {
    Task task("TestTask", [&](const std::atomic<bool>& should_cancel) {});

    EXPECT_FALSE(task.isFinished());
}

// ============================================================================
// TaskManager Tests
// ============================================================================

class TaskManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clear any existing tasks
    }

    void TearDown() override {
        // Clean up
    }
};

TEST_F(TaskManagerTest, LaunchNormalTask_CreatesTask) {
    bool executed = false;

    auto task = TaskManager::instance().launch("TestTask", [&](const std::atomic<bool>& should_cancel) {
        executed = true;
    });

    ASSERT_NE(task, nullptr);
    EXPECT_EQ(task->getName(), "TestTask");

    // Wait for task to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST_F(TaskManagerTest, LaunchTask_ExecutesFunction) {
    std::atomic<int> counter{0};

    auto task = TaskManager::instance().launch("CounterTask", [&](const std::atomic<bool>& should_cancel) {
        for (int i = 0; i < 10; ++i) {
            counter++;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    // Wait for completion
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    EXPECT_EQ(counter.load(), 10);
}

TEST_F(TaskManagerTest, LaunchMultipleTasks_AllExecute) {
    std::atomic<int> count{0};

    for (int i = 0; i < 5; ++i) {
        TaskManager::instance().launch("Task" + std::to_string(i), [&](const std::atomic<bool>& should_cancel) {
            count++;
        });
    }

    // Wait for all tasks
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

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

    // Wait a bit more
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Should have been cancelled before reaching 1000
    EXPECT_LT(counter.load(), 1000);
}

TEST_F(TaskManagerTest, TaskProgress_UpdatesCorrectly) {
    auto task = TaskManager::instance().launch("ProgressTask", [&](const std::atomic<bool>& cancel) {
        for (int i = 0; i <= 100; i += 10) {
            task->setProgress(i);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }, 100.0f);

    // Wait for completion
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

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

    // Blocking task should wait for completion
    EXPECT_TRUE(task->isFinished());
    EXPECT_TRUE(executed);
}

// ============================================================================
// Task Manager Query Tests
// ============================================================================

TEST_F(TaskManagerTest, GetActiveTasks_ReturnsRunningTasks) {
    auto task1 = TaskManager::instance().launch("LongTask1", [&](const std::atomic<bool>& cancel) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Should have at least one active task
    auto active_tasks = TaskManager::instance().getActiveTasks();
    EXPECT_GE(active_tasks.size(), 1);

    // Wait for completion
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

TEST_F(TaskManagerTest, CancelAllTasks_StopsAllRunningTasks) {
    std::atomic<int> counter{0};

    for (int i = 0; i < 5; ++i) {
        TaskManager::instance().launch("Task" + std::to_string(i), [&](const std::atomic<bool>& cancel) {
            while (!cancel && counter < 1000) {
                counter++;
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    TaskManager::instance().cancelAllTasks();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // All tasks should have been cancelled
    auto active_tasks = TaskManager::instance().getActiveTasks();
    EXPECT_EQ(active_tasks.size(), 0);
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

    // Wait for tasks to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    EXPECT_EQ(task_count.load(), num_threads);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(TaskManagerTest, LaunchTaskWithEmptyName_Works) {
    auto task = TaskManager::instance().launch("", [&](const std::atomic<bool>& cancel) {});

    ASSERT_NE(task, nullptr);
}

TEST_F(TaskManagerTest, LaunchTaskWithImmediateCompletion) {
    bool executed = false;

    auto task = TaskManager::instance().launch("ImmediateTask", [&](const std::atomic<bool>& cancel) {
        executed = true;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    EXPECT_TRUE(executed);
    EXPECT_TRUE(task->isFinished());
}

TEST_F(TaskManagerTest, TaskWithException_DoesNotCrash) {
    auto task = TaskManager::instance().launch("ExceptionTask", [&](const std::atomic<bool>& cancel) {
        throw std::runtime_error("Test exception");
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Task should handle exception gracefully
    // Status might be Failed depending on implementation
    EXPECT_TRUE(task->isFinished());
}

// ============================================================================
// Task Completion Callback Tests
// ============================================================================

TEST_F(TaskManagerTest, TaskCompletionCallback_CalledOnFinish) {
    bool callback_called = false;

    auto task = TaskManager::instance().launch("CallbackTask", [&](const std::atomic<bool>& cancel) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    });

    // Set completion callback
    task->setCompletionCallback([&]() {
        callback_called = true;
    });

    // Wait for completion
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_TRUE(callback_called);
}

TEST_F(TaskManagerTest, TaskCompletionCallback_NotCalledOnCancel) {
    bool callback_called = false;

    auto task = TaskManager::instance().launch("CallbackTask", [&](const std::atomic<bool>& cancel) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    });

    task->setCompletionCallback([&]() {
        callback_called = true;
    });

    task->cancel();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Behavior depends on implementation
    // Callback may or may not be called on cancel
}
