/**
 * @file pub_sub_chain_test.cpp
 * @brief 事件流集成测试
 * @details 测试复杂的事件发布/订阅场景
 * @author DearTs Team
 * @date 2025
 */

#include <gtest/gtest.h>
#include "core/event/event_bus.h"
#include "core/plugin/plugin.h"
#include "tests/mocks/mock_plugin.hpp"

using namespace DearTs::Core::Event;
using namespace DearTs::Core::Plugin;
using DearTs::Tests::MockPlugin;

/**
 * @brief 事件流集成测试 Fixture
 */
class EventFlowTest : public ::testing::Test {
protected:
    void SetUp() override {
        PluginManager::instance().clear();
        EventBus::instance().clear();
    }

    void TearDown() override {
        PluginManager::instance().clear();
        EventBus::instance().clear();
    }
};

// ============================================================================
// 多发布者多订阅者测试
// ============================================================================

TEST_F(EventFlowTest, PublisherToMultipleSubscribers) {
    // 定义测试事件
    struct TestEvent {
        int value;
    };

    // 创建订阅者
    int subscriber_a_count = 0;
    int subscriber_b_count = 0;
    int subscriber_c_count = 0;

    auto token_a = EventBus::instance().subscribe<TestEvent>(
        [&subscriber_a_count](const TestEvent& e) {
            subscriber_a_count += e.value;
        }
    );

    auto token_b = EventBus::instance().subscribe<TestEvent>(
        [&subscriber_b_count](const TestEvent& e) {
            subscriber_b_count += e.value * 2;
        }
    );

    auto token_c = EventBus::instance().subscribe<TestEvent>(
        [&subscriber_c_count](const TestEvent& e) {
            subscriber_c_count += e.value * 3;
        }
    );

    // 发布事件
    EventBus::instance().publish(TestEvent{ .value = 10 });

    // 验证所有订阅者都收到事件
    EXPECT_EQ(subscriber_a_count, 10);
    EXPECT_EQ(subscriber_b_count, 20);
    EXPECT_EQ(subscriber_c_count, 30);
}

TEST_F(EventFlowTest, MultiplePublishers_SingleSubscriber) {
    // 定义测试事件
    struct CounterEvent {
        int increment;
    };

    int total_count = 0;

    // 单个订阅者
    auto token = EventBus::instance().subscribe<CounterEvent>(
        [&total_count](const CounterEvent& e) {
            total_count += e.increment;
        }
    );

    // 多个发布者
    EventBus::instance().publish(CounterEvent{ .increment = 5 });
    EventBus::instance().publish(CounterEvent{ .increment = 10 });
    EventBus::instance().publish(CounterEvent{ .increment = 15 });

    EXPECT_EQ(total_count, 30);
}

// ============================================================================
// 事件链测试
// ============================================================================

TEST_F(EventFlowTest, EventChain_DominoEffect) {
    // 定义事件链
    struct EventA {
        std::string data;
    };

    struct EventB {
        std::string data;
    };

    struct EventC {
        std::string data;
    };

    std::string chain_result;

    // 订阅 EventA，触发 EventB
    auto token_a = EventBus::instance().subscribe<EventA>(
        [&chain_result](const EventA& e) {
            chain_result += "A->";
            EventBus::instance().publish(EventB{ .data = e.data + "->B" });
        }
    );

    // 订阅 EventB，触发 EventC
    auto token_b = EventBus::instance().subscribe<EventB>(
        [&chain_result](const EventB& e) {
            chain_result += "B->";
            EventBus::instance().publish(EventC{ .data = e.data + "->C" });
        }
    );

    // 订阅 EventC
    auto token_c = EventBus::instance().subscribe<EventC>(
        [&chain_result](const EventC& e) {
            chain_result += "C";
        }
    );

    // 触发事件链
    EventBus::instance().publish(EventA{ .data = "Start" });

    EXPECT_EQ(chain_result, "A->B->C");
}

TEST_F(EventFlowTest, CircularEventDependency_DoesNotDeadlock) {
    // 定义事件
    struct EventX {
        int count;
    };

    struct EventY {
        int count;
    };

    int x_count = 0;
    int y_count = 0;

    // EventX 触发 EventY
    auto token_x = EventBus::instance().subscribe<EventX>(
        [&x_count, &y_count](const EventX& e) {
            x_count++;
            if (e.count < 3) {
                EventBus::instance().publish(EventY{ .count = e.count + 1 });
            }
        }
    );

    // EventY 触发 EventX
    auto token_y = EventBus::instance().subscribe<EventY>(
        [&y_count, &x_count](const EventY& e) {
            y_count++;
            if (e.count < 3) {
                EventBus::instance().publish(EventX{ .count = e.count + 1 });
            }
        }
    );

    // 启动循环
    EventBus::instance().publish(EventX{ .count = 0 });

    // 验证循环完成（无死锁）
    // EventBus 是同步的，事件链：
    // EventX(0) [x=1] → EventY(1) [y=1] → EventX(2) [x=2] → EventY(3) [y=2, 停止]
    EXPECT_EQ(x_count, 2);
    EXPECT_EQ(y_count, 2);
}

// ============================================================================
// 插件间事件通信测试
// ============================================================================

TEST_F(EventFlowTest, InterPluginCommunication_PublishAndSubscribe) {
    // 定义插件间通信事件
    struct PluginMessageEvent {
        std::string sender;
        std::string message;
    };

    std::vector<std::string> messages_a;
    std::vector<std::string> messages_b;

    // 订阅者 A
    auto token_a = EventBus::instance().subscribe<PluginMessageEvent>(
        [&messages_a](const PluginMessageEvent& e) {
            messages_a.push_back(e.sender + ": " + e.message);
        }
    );

    // 订阅者 B
    auto token_b = EventBus::instance().subscribe<PluginMessageEvent>(
        [&messages_b](const PluginMessageEvent& e) {
            messages_b.push_back(e.sender + ": " + e.message);
        }
    );

    // 发布消息
    EventBus::instance().publish(PluginMessageEvent{
        .sender = "PluginA",
        .message = "Hello Plugin B!"
    });

    EventBus::instance().publish(PluginMessageEvent{
        .sender = "PluginA",
        .message = "Hello Plugin C!"
    });

    // 验证两个订阅者都收到消息
    EXPECT_EQ(messages_a.size(), 2);
    EXPECT_EQ(messages_b.size(), 2);
    EXPECT_EQ(messages_a[0], "PluginA: Hello Plugin B!");
    EXPECT_EQ(messages_b[1], "PluginA: Hello Plugin C!");
}

TEST_F(EventFlowTest, EventBusUnsubscribeOnPluginUnload) {
    // 定义事件
    struct TestEvent {
        int data;
    };

    int event_count = 0;
    EventToken event_token;

    // 模拟插件生命周期：启用时订阅，禁用时取消订阅
    // 启用阶段
    event_token = EventBus::instance().subscribe<TestEvent>(
        [&event_count](const TestEvent& e) {
            event_count++;
        }
    );

    // 发布事件
    EventBus::instance().publish(TestEvent{ .data = 1 });
    EXPECT_EQ(event_count, 1);

    // 禁用阶段（取消订阅）
    EventBus::instance().unsubscribe<TestEvent>(event_token);
    event_count = 0;  // 重置计数

    // 发布事件，应该不再接收
    EventBus::instance().publish(TestEvent{ .data = 2 });
    EXPECT_EQ(event_count, 0);  // 应该仍然是 0，因为没有订阅

    // 重新订阅（模拟重新启用）
    event_token = EventBus::instance().subscribe<TestEvent>(
        [&event_count](const TestEvent& e) {
            event_count++;
        }
    );

    EventBus::instance().publish(TestEvent{ .data = 3 });
    EXPECT_EQ(event_count, 1);
}

// ============================================================================
// 事件优先级和顺序测试
// ============================================================================

TEST_F(EventFlowTest, SubscriptionOrder_PreservesOrder) {
    // 定义事件
    struct OrderEvent {
        int value;
    };

    std::string execution_order;

    // 按顺序订阅
    auto token_1 = EventBus::instance().subscribe<OrderEvent>(
        [&execution_order](const OrderEvent&) {
            execution_order += "1";
        }
    );

    auto token_2 = EventBus::instance().subscribe<OrderEvent>(
        [&execution_order](const OrderEvent&) {
            execution_order += "2";
        }
    );

    auto token_3 = EventBus::instance().subscribe<OrderEvent>(
        [&execution_order](const OrderEvent&) {
            execution_order += "3";
        }
    );

    // 发布事件
    EventBus::instance().publish(OrderEvent{ .value = 0 });

    // 验证执行顺序
    EXPECT_EQ(execution_order, "123");
}

// ============================================================================
// 事件过滤和条件测试
// ============================================================================

TEST_F(EventFlowTest, ConditionalSubscription_OnlyRelevantEvents) {
    // 定义事件
    struct DataEvent {
        int id;
        int value;
    };

    int target_id_sum = 0;

    // 只订阅特定 ID 的事件
    int target_id = 42;
    auto token = EventBus::instance().subscribe<DataEvent>(
        [target_id, &target_id_sum](const DataEvent& e) {
            if (e.id == target_id) {
                target_id_sum += e.value;
            }
        }
    );

    // 发布多个事件
    EventBus::instance().publish(DataEvent{ .id = 42, .value = 10 });
    EventBus::instance().publish(DataEvent{ .id = 99, .value = 20 });  // 应该被忽略
    EventBus::instance().publish(DataEvent{ .id = 42, .value = 30 });

    // 验证只有目标 ID 的事件被处理
    EXPECT_EQ(target_id_sum, 40);
}

// ============================================================================
// 性能和压力测试
// ============================================================================

TEST_F(EventFlowTest, HighFrequencyEvents_1000Events) {
    // 定义事件
    struct PerformanceEvent {
        int counter;
    };

    int receive_count = 0;

    auto token = EventBus::instance().subscribe<PerformanceEvent>(
        [&receive_count](const PerformanceEvent& e) {
            receive_count++;
        }
    );

    // 高频发布 1000 个事件
    for (int i = 0; i < 1000; ++i) {
        EventBus::instance().publish(PerformanceEvent{ .counter = i });
    }

    EXPECT_EQ(receive_count, 1000);
}

TEST_F(EventFlowTest, MultipleSubscribers_PerformanceTest) {
    // 定义事件
    struct BroadCastEvent {
        int data;
    };

    // 创建 100 个订阅者
    const int num_subscribers = 100;
    std::vector<int> receive_counts(num_subscribers, 0);
    std::vector<EventToken> tokens;

    for (int i = 0; i < num_subscribers; ++i) {
        tokens.push_back(
            EventBus::instance().subscribe<BroadCastEvent>(
                [&receive_counts, i](const BroadCastEvent& e) {
                    receive_counts[i] += e.data;
                }
            )
        );
    }

    // 发布事件
    EventBus::instance().publish(BroadCastEvent{ .data = 10 });

    // 验证所有订阅者都收到事件
    for (int i = 0; i < num_subscribers; ++i) {
        EXPECT_EQ(receive_counts[i], 10);
    }
}
