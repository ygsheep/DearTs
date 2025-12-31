/**
 * @file event_bus_test.cpp
 * @brief Unit tests for EventBus
 */

#include <gtest/gtest.h>
#include "core/event/event_bus.h"
#include <thread>
#include <chrono>
#include <atomic>

using namespace DearTs::Core::Event;

// ============================================================================
// Test Events
// ============================================================================

struct TestEvent {
    int value;
    std::string message;
};

struct CounterEvent {
    int count;
};

struct ComplexEvent {
    std::vector<int> numbers;
    std::string text;
    double decimal;
};

// ============================================================================
// EventBus Test Fixture
// ============================================================================

class EventBusTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 清除所有订阅者，避免测试间干扰
        EventBus::instance().clear();
    }

    void TearDown() override {
        // 清除所有订阅者
        EventBus::instance().clear();
    }
};

// ============================================================================
// Basic Subscribe/Publish Tests
// ============================================================================

TEST_F(EventBusTest, SubscribeAndPublish_EventReceived) {
    bool callback_called = false;
    int received_value = 0;

    auto token = EventBus::instance().subscribe<TestEvent>(
        [&](const TestEvent& e) {
            callback_called = true;
            received_value = e.value;
        }
    );

    TestEvent event{ .value = 42, .message = "test" };
    EventBus::instance().publish(event);

    EXPECT_TRUE(callback_called);
    EXPECT_EQ(received_value, 42);
}

TEST_F(EventBusTest, SubscribeMultipleSubscribers_AllReceiveEvent) {
    int callback1_count = 0;
    int callback2_count = 0;
    int callback3_count = 0;

    EventBus::instance().subscribe<TestEvent>(
        [&](const TestEvent& e) { callback1_count++; }
    );
    EventBus::instance().subscribe<TestEvent>(
        [&](const TestEvent& e) { callback2_count++; }
    );
    EventBus::instance().subscribe<TestEvent>(
        [&](const TestEvent& e) { callback3_count++; }
    );

    TestEvent event{ .value = 1, .message = "test" };
    EventBus::instance().publish(event);

    EXPECT_EQ(callback1_count, 1);
    EXPECT_EQ(callback2_count, 1);
    EXPECT_EQ(callback3_count, 1);
}

TEST_F(EventBusTest, PublishMultipleTimes_AllCallbacksReceived) {
    int count = 0;

    EventBus::instance().subscribe<TestEvent>(
        [&](const TestEvent& e) { count += e.value; }
    );

    EventBus::instance().publish(TestEvent{ .value = 10, .message = "" });
    EventBus::instance().publish(TestEvent{ .value = 20, .message = "" });
    EventBus::instance().publish(TestEvent{ .value = 30, .message = "" });

    EXPECT_EQ(count, 60);
}

TEST_F(EventBusTest, DifferentEventTypes_IsolatedDelivery) {
    bool test_event_called = false;
    bool counter_event_called = false;

    EventBus::instance().subscribe<TestEvent>(
        [&](const TestEvent& e) { test_event_called = true; }
    );

    EventBus::instance().subscribe<CounterEvent>(
        [&](const CounterEvent& e) { counter_event_called = true; }
    );

    EventBus::instance().publish(TestEvent{ .value = 1, .message = "" });

    EXPECT_TRUE(test_event_called);
    EXPECT_FALSE(counter_event_called);

    EventBus::instance().publish(CounterEvent{ .count = 1 });

    EXPECT_TRUE(counter_event_called);
}

// ============================================================================
// Unsubscribe Tests
// ============================================================================

TEST_F(EventBusTest, Unsubscribe_CallbackNotCalled) {
    int call_count = 0;

    auto token = EventBus::instance().subscribe<TestEvent>(
        [&](const TestEvent& e) { call_count++; }
    );

    // First publish - should be called
    EventBus::instance().publish(TestEvent{ .value = 1, .message = "" });
    EXPECT_EQ(call_count, 1);

    // Unsubscribe
    EventBus::instance().unsubscribe<TestEvent>(token);

    // Second publish - should not be called
    EventBus::instance().publish(TestEvent{ .value = 2, .message = "" });
    EXPECT_EQ(call_count, 1);  // Still 1
}

TEST_F(EventBusTest, UnsubscribeOneOfMultiple_OthersStillCalled) {
    int callback1_count = 0;
    int callback2_count = 0;

    auto token1 = EventBus::instance().subscribe<TestEvent>(
        [&](const TestEvent& e) { callback1_count++; }
    );

    auto token2 = EventBus::instance().subscribe<TestEvent>(
        [&](const TestEvent& e) { callback2_count++; }
    );

    // First publish
    EventBus::instance().publish(TestEvent{ .value = 1, .message = "" });
    EXPECT_EQ(callback1_count, 1);
    EXPECT_EQ(callback2_count, 1);

    // Unsubscribe first callback
    EventBus::instance().unsubscribe<TestEvent>(token1);

    // Second publish
    EventBus::instance().publish(TestEvent{ .value = 2, .message = "" });
    EXPECT_EQ(callback1_count, 1);  // Not called again
    EXPECT_EQ(callback2_count, 2);  // Called twice
}

TEST_F(EventBusTest, UnsubscribeInvalidToken_DoesNothing) {
    int call_count = 0;

    EventBus::instance().subscribe<TestEvent>(
        [&](const TestEvent& e) { call_count++; }
    );

    // Try to unsubscribe with invalid token
    EventToken invalid_token;
    EXPECT_NO_THROW({
        EventBus::instance().unsubscribe<TestEvent>(invalid_token);
    });

    // Should still receive events
    EventBus::instance().publish(TestEvent{ .value = 1, .message = "" });
    EXPECT_EQ(call_count, 1);
}

// ============================================================================
// Token RAII Tests
// ============================================================================

TEST_F(EventBusTest, TokenDestroyed_AutoUnsubscribes) {
    int call_count = 0;

    {
        // EventGuard goes out of scope and auto-unsubscribes
        EventGuard<TestEvent> guard = make_event_guard<TestEvent>(
            [&](const TestEvent& e) { call_count++; }
        );

        EventBus::instance().publish(TestEvent{ .value = 1, .message = "" });
        EXPECT_EQ(call_count, 1);
    }  // Guard destroyed here, auto-unsubscribes

    // Should not be called anymore
    EventBus::instance().publish(TestEvent{ .value = 2, .message = "" });
    EXPECT_EQ(call_count, 1);  // Still 1
}

TEST_F(EventBusTest, TokenMove_CorrectBehavior) {
    int call_count = 0;

    auto token1 = EventBus::instance().subscribe<TestEvent>(
        [&](const TestEvent& e) { call_count++; }
    );

    // Move token
    EventToken token2 = std::move(token1);

    // token1 should be invalid
    EXPECT_FALSE(token1.is_valid());
    EXPECT_TRUE(token2.is_valid());

    // Should still receive events through token2
    EventBus::instance().publish(TestEvent{ .value = 1, .message = "" });
    EXPECT_EQ(call_count, 1);

    // Unsubscribe using token2
    EventBus::instance().unsubscribe<TestEvent>(token2);

    // Should not receive events anymore
    EventBus::instance().publish(TestEvent{ .value = 2, .message = "" });
    EXPECT_EQ(call_count, 1);
}

// ============================================================================
// Complex Event Data Tests
// ============================================================================

TEST_F(EventBusTest, ComplexEvent_PassedCorrectly) {
    ComplexEvent received{};

    EventBus::instance().subscribe<ComplexEvent>(
        [&](const ComplexEvent& e) {
            received = e;
        }
    );

    ComplexEvent event{
        .numbers = {1, 2, 3, 4, 5},
        .text = "Hello, World!",
        .decimal = 3.14159
    };

    EventBus::instance().publish(event);

    EXPECT_EQ(received.numbers.size(), 5);
    EXPECT_EQ(received.numbers[0], 1);
    EXPECT_EQ(received.numbers[4], 5);
    EXPECT_EQ(received.text, "Hello, World!");
    EXPECT_DOUBLE_EQ(received.decimal, 3.14159);
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

TEST_F(EventBusTest, ConcurrentPublish_NoCrash) {
    const int num_threads = 10;
    const int publishes_per_thread = 100;

    std::atomic<int> total_count{0};

    EventBus::instance().subscribe<TestEvent>(
        [&](const TestEvent& e) {
            total_count.fetch_add(e.value);
        }
    );

    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([i, publishes_per_thread]() {
            for (int j = 0; j < publishes_per_thread; ++j) {
                EventBus::instance().publish(TestEvent{
                    .value = 1,
                    .message = "thread_" + std::to_string(i)
                });
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(total_count.load(), num_threads * publishes_per_thread);
}

TEST_F(EventBusTest, ConcurrentSubscribeUnsubscribe_ThreadSafe) {
    const int num_operations = 100;

    std::vector<EventToken> tokens;

    // Subscribe
    for (int i = 0; i < num_operations; ++i) {
        tokens.push_back(EventBus::instance().subscribe<TestEvent>(
            [i](const TestEvent& e) {
                // Callback does nothing
            }
        ));
    }

    // Unsubscribe all
    for (auto& token : tokens) {
        EXPECT_NO_THROW({
            EventBus::instance().unsubscribe<TestEvent>(token);
        });
    }

    // Should be no subscribers left
    int call_count = 0;
    EventBus::instance().subscribe<TestEvent>(
        [&](const TestEvent& e) { call_count++; }
    );

    EventBus::instance().publish(TestEvent{ .value = 1, .message = "" });
    EXPECT_EQ(call_count, 1);  // Only the last subscriber
}

// ============================================================================
// Exception Handling Tests
// ============================================================================

TEST_F(EventBusTest, ExceptionInCallback_DoesNotCrash) {
    int callback1_count = 0;
    int callback2_count = 0;

    EventBus::instance().subscribe<TestEvent>(
        [&](const TestEvent& e) {
            callback1_count++;
            if (e.value == 42) {
                throw std::runtime_error("Test exception");
            }
        }
    );

    EventBus::instance().subscribe<TestEvent>(
        [&](const TestEvent& e) {
            callback2_count++;
        }
    );

    // Publish event that triggers exception
    EXPECT_NO_THROW({
        EventBus::instance().publish(TestEvent{ .value = 42, .message = "" });
    });

    // Both callbacks should be called (exception doesn't stop propagation)
    EXPECT_EQ(callback1_count, 1);
    EXPECT_EQ(callback2_count, 1);
}

TEST_F(EventBusTest, MultipleCallbacksThrowing_AllStillCalled) {
    int callback1_count = 0;
    int callback2_count = 0;
    int callback3_count = 0;

    EventBus::instance().subscribe<TestEvent>(
        [&](const TestEvent& e) {
            callback1_count++;
            throw std::runtime_error("Exception 1");
        }
    );

    EventBus::instance().subscribe<TestEvent>(
        [&](const TestEvent& e) {
            callback2_count++;
            throw std::runtime_error("Exception 2");
        }
    );

    EventBus::instance().subscribe<TestEvent>(
        [&](const TestEvent& e) {
            callback3_count++;
            throw std::runtime_error("Exception 3");
        }
    );

    EXPECT_NO_THROW({
        EventBus::instance().publish(TestEvent{ .value = 1, .message = "" });
    });

    EXPECT_EQ(callback1_count, 1);
    EXPECT_EQ(callback2_count, 1);
    EXPECT_EQ(callback3_count, 1);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(EventBusTest, SubscribeWithEmptyCallback_Works) {
    EXPECT_NO_THROW({
        auto token = EventBus::instance().subscribe<TestEvent>(
            [&](const TestEvent& e) {
                // Empty callback
            }
        );

        EventBus::instance().publish(TestEvent{ .value = 1, .message = "" });
    });
}

TEST_F(EventBusTest, SubscribeSameCallbackMultipleTimes_AllCalled) {
    int call_count = 0;
    auto callback = [&](const TestEvent& e) { call_count++; };

    EventBus::instance().subscribe<TestEvent>(callback);
    EventBus::instance().subscribe<TestEvent>(callback);
    EventBus::instance().subscribe<TestEvent>(callback);

    EventBus::instance().publish(TestEvent{ .value = 1, .message = "" });

    EXPECT_EQ(call_count, 3);
}

TEST_F(EventBusTest, PublishToNoSubscribers_DoesNothing) {
    // No subscribers for this event type
    EXPECT_NO_THROW({
        EventBus::instance().publish(TestEvent{ .value = 1, .message = "" });
    });
}

TEST_F(EventBusTest, PublishWhileCallbackExecuting_DoesNotDeadlock) {
    bool outer_called = false;
    bool inner_called = false;

    EventBus::instance().subscribe<TestEvent>(
        [&](const TestEvent& e) {
            outer_called = true;
            // Publish another event while handling an event
            EventBus::instance().publish(CounterEvent{ .count = 1 });
        }
    );

    EventBus::instance().subscribe<CounterEvent>(
        [&](const CounterEvent& e) {
            inner_called = true;
        }
    );

    EXPECT_NO_THROW({
        EventBus::instance().publish(TestEvent{ .value = 1, .message = "" });
    });

    EXPECT_TRUE(outer_called);
    EXPECT_TRUE(inner_called);
}

// ============================================================================
// Token Validation Tests
// ============================================================================

TEST_F(EventBusTest, DefaultToken_IsInvalid) {
    EventToken token;
    EXPECT_FALSE(token.is_valid());
    EXPECT_EQ(token.get_id(), 0);
}

TEST_F(EventBusTest, ValidToken_HasNonZeroId) {
    auto token = EventBus::instance().subscribe<TestEvent>(
        [&](const TestEvent& e) {}
    );

    EXPECT_TRUE(token.is_valid());
    EXPECT_NE(token.get_id(), 0);
}

TEST_F(EventBusTest, InvalidateToken_MakesItInvalid) {
    auto token = EventBus::instance().subscribe<TestEvent>(
        [&](const TestEvent& e) {}
    );

    EXPECT_TRUE(token.is_valid());

    token.invalidate();
    EXPECT_FALSE(token.is_valid());
}

// ============================================================================
// Unsubscribe All Tests
// ============================================================================

TEST_F(EventBusTest, SubscribeMultipleThenUnsubscribeAll_NoneCalled) {
    std::vector<EventToken> tokens;
    int call_count = 0;

    for (int i = 0; i < 10; ++i) {
        tokens.push_back(EventBus::instance().subscribe<TestEvent>(
            [&](const TestEvent& e) { call_count++; }
        ));
    }

    // All should be called
    EventBus::instance().publish(TestEvent{ .value = 1, .message = "" });
    EXPECT_EQ(call_count, 10);

    // Unsubscribe all
    call_count = 0;
    for (auto& token : tokens) {
        EventBus::instance().unsubscribe<TestEvent>(token);
    }

    // None should be called
    EventBus::instance().publish(TestEvent{ .value = 2, .message = "" });
    EXPECT_EQ(call_count, 0);
}
