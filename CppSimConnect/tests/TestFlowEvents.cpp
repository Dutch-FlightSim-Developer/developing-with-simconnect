/*
 * Copyright (c) 2026. Bert Laverman
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "gtest/gtest.h"

#include <algorithm>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include <simconnect/simconnect.hpp>
#include <simconnect/simple_handler.hpp>
#include <simconnect/connection.hpp>
#include <simconnect/util/null_logger.hpp>
#include <simconnect/events/flow_events.hpp>

using namespace SimConnect;

#if MSFS_2024_SDK

//NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables,misc-include-cleaner,cppcoreguidelines-pro-bounds-array-to-pointer-decay)

// Mock connection: feeds queued FlowEvent messages and counts (un)subscribe calls.
class FlowEventsMockConnection {
public:
    using mutex_type = NoMutex;
    using guard_type = NoGuard;
    using logger_type = NullLogger;

private:
    std::vector<Messages::FlowEventMsg> messages_;
    size_t messageIndex_{0};
    bool isOpen_{true};
    NullLogger logger_;

public:
    int subscribeCount{0};
    int unsubscribeCount{0};

    void addFlowEvent(FlowEventId flowEvent, std::string_view path = "") {
        Messages::FlowEventMsg msg{};
        msg.dwID = static_cast<unsigned long>(Messages::flowEvent);
        msg.dwSize = sizeof(msg);
        msg.dwVersion = 1;
        msg.FlowEvent = static_cast<SIMCONNECT_FLOW_EVENT>(flowEvent);
        const auto len = (std::min)(path.size(), sizeof(msg.FltPath) - 1);
        std::copy_n(path.data(), len, msg.FltPath);
        msg.FltPath[len] = '\0';
        messages_.push_back(msg);
    }

    bool callDispatch(const std::function<void(const SIMCONNECT_RECV*, unsigned long)>& dispatchFunc) {
        if (isOpen_ && messageIndex_ < messages_.size()) {
            const auto& msg = messages_[messageIndex_++];
            dispatchFunc(reinterpret_cast<const SIMCONNECT_RECV*>(&msg), msg.dwSize);
            return true;
        }
        return false;
    }

    [[nodiscard]] bool isOpen() const { return isOpen_; }
    void close() { isOpen_ = false; }

    FlowEventsMockConnection& subscribeToFlowEvents() { ++subscribeCount; return *this; }
    FlowEventsMockConnection& unsubscribeFromFlowEvents() { ++unsubscribeCount; return *this; }

    NullLogger& logger() noexcept { return logger_; }
};

using TestHandler = SimpleHandler<FlowEventsMockConnection>;

// Scenario: Constructing FlowEvents
// Given a handler with no subscriptions
// When I construct a FlowEvents facade
// Then it should not subscribe to SimConnect's flow events
TEST(FlowEventsTests, ConstructionDoesNotSubscribe) {
    FlowEventsMockConnection connection;
    TestHandler handler(connection);

    const FlowEvents<TestHandler> flowEvents(handler);

    EXPECT_EQ(connection.subscribeCount, 0);
}

// Scenario: Subscribing with a single callback
// Given a FlowEvents facade
// When I subscribe with a single callback and a fltLoad event arrives
// Then the callback should receive the event id and flight path
// And SimConnect's flow events should be subscribed to exactly once
TEST(FlowEventsTests, SubscribeSingleCallback_ReceivesEvent) {
    FlowEventsMockConnection connection;
    TestHandler handler(connection);
    FlowEvents<TestHandler> flowEvents(handler);

    FlowEventId receivedId{};
    std::string receivedPath;
    auto reg = flowEvents.subscribe([&](FlowEventId id, std::string_view path) {
        receivedId = id;
        receivedPath = path;
    });

    connection.addFlowEvent(FlowEventIds::fltLoad, "test.flt");
    handler.handle();

    EXPECT_EQ(receivedId, FlowEventIds::fltLoad);
    EXPECT_EQ(receivedPath, "test.flt");
    EXPECT_EQ(connection.subscribeCount, 1);
}

// Scenario: Subscribing with per-event handlers
// Given a FlowEvents facade
// When I subscribe with a FlowEventHandlers struct and matching events arrive
// Then only the handlers for those event types should be called
TEST(FlowEventsTests, SubscribeStructHandlers_DispatchesMatchingHandlers) {
    FlowEventsMockConnection connection;
    TestHandler handler(connection);
    FlowEvents<TestHandler> flowEvents(handler);

    std::string fltLoadedPath;
    bool flightStartCalled{ false };
    bool planeCrashCalled{ false };

    FlowEventHandlers handlers;
    handlers.fltLoaded = [&](std::string_view path) { fltLoadedPath = path; };
    handlers.flightStart = [&]() { flightStartCalled = true; };
    auto reg = flowEvents.subscribe(std::move(handlers));

    connection.addFlowEvent(FlowEventIds::fltLoaded, "a.flt");
    connection.addFlowEvent(FlowEventIds::flightStart);
    handler.handle();

    EXPECT_EQ(fltLoadedPath, "a.flt");
    EXPECT_TRUE(flightStartCalled);
    EXPECT_FALSE(planeCrashCalled);
}

// Scenario: Subscribing more than once
// Given a FlowEvents facade
// When two independent handlers subscribe
// Then SimConnect's flow events should still be subscribed to only once
TEST(FlowEventsTests, MultipleSubscriptions_SubscribeOnlyOnce) {
    FlowEventsMockConnection connection;
    TestHandler handler(connection);
    FlowEvents<TestHandler> flowEvents(handler);

    auto reg1 = flowEvents.subscribe([](FlowEventId, std::string_view) {});
    auto reg2 = flowEvents.subscribe([](FlowEventId, std::string_view) {});

    EXPECT_EQ(connection.subscribeCount, 1);
}

// Scenario: Stopping one of several subscriptions
// Given two active subscriptions
// When one of them is stopped
// Then SimConnect's flow events should remain subscribed
// And the remaining handler should still receive events
TEST(FlowEventsTests, StopOneOfTwo_KeepsSubscriptionAndOtherHandler) {
    FlowEventsMockConnection connection;
    TestHandler handler(connection);
    FlowEvents<TestHandler> flowEvents(handler);

    int firstCount{0};
    int secondCount{0};
    auto reg1 = flowEvents.subscribe([&](FlowEventId, std::string_view) { ++firstCount; });
    auto reg2 = flowEvents.subscribe([&](FlowEventId, std::string_view) { ++secondCount; });

    reg1.stop();
    EXPECT_EQ(connection.unsubscribeCount, 0);

    connection.addFlowEvent(FlowEventIds::flightEnd);
    handler.handle();

    EXPECT_EQ(firstCount, 0);
    EXPECT_EQ(secondCount, 1);
}

// Scenario: Stopping the last subscription
// Given a single active subscription
// When it is stopped
// Then SimConnect's flow events should be unsubscribed from
TEST(FlowEventsTests, StopLastSubscription_Unsubscribes) {
    FlowEventsMockConnection connection;
    TestHandler handler(connection);
    FlowEvents<TestHandler> flowEvents(handler);

    auto reg = flowEvents.subscribe([](FlowEventId, std::string_view) {});
    reg.stop();

    EXPECT_EQ(connection.subscribeCount, 1);
    EXPECT_EQ(connection.unsubscribeCount, 1);
}

// Scenario: Destroying FlowEvents with an active subscription
// Given a FlowEvents facade with an active subscription
// When the facade is destroyed without stopping the subscription
// Then SimConnect's flow events should be unsubscribed from
TEST(FlowEventsTests, DestructorUnsubscribesIfStillActive) {
    FlowEventsMockConnection connection;
    TestHandler handler(connection);

    {
        FlowEvents<TestHandler> flowEvents(handler);
        auto reg = flowEvents.subscribe([](FlowEventId, std::string_view) {});
        EXPECT_EQ(connection.subscribeCount, 1);
    }

    EXPECT_EQ(connection.unsubscribeCount, 1);
}

//NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables,misc-include-cleaner,cppcoreguidelines-pro-bounds-array-to-pointer-decay)

#endif // MSFS_2024_SDK
