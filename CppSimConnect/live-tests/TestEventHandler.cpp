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

#include <gtest/gtest.h>

#include "live_connection.hpp"

#include <simconnect/simconnect.hpp>
#include <simconnect/util/logger.hpp>

#include <simconnect/events/events.hpp>
#include <simconnect/events/event_handler.hpp>
#include <simconnect/events/notification_group.hpp>

#include <atomic>
#include <chrono>
#include <string>
#include <string_view>
#include <functional>


using namespace SimConnect;
using namespace std::chrono_literals;

namespace {


/**
 * Helper function to create a unique custom event for testing.
 * Uses custom event IDs to avoid conflicts between tests.
 * 
 * @param baseName Optional base name for the event (for debugging)
 * @returns A new unique event
 */
event createTestEvent() {
    static std::atomic<int> eventCounter{ 0 };
    const std::string eventName = "#" + std::to_string(Events::customEventMin + eventCounter++);
    return event::get(eventName);
}
}


/**
 * Helper class to set up an event sender client.
 */
class EventSender : public LiveTests::LiveConnection {
public:
    EventHandler<LiveTests::TestMessageHandler> eventHandler;

    explicit EventSender(std::string_view name)
        : LiveConnection(name), eventHandler(handler)
    {
        eventHandler.logger().level(LogLevel::Debug);
    }


    NotificationGroup<LiveTests::TestMessageHandler> createGroupWithEvent(event evt) {
        return eventHandler
            .createNotificationGroup()
            .withHighestPriority()
            .addEvent(evt);
    }

    void sendEvent(event evt, NotificationGroupId groupId, unsigned long data = 0) {
        eventHandler.sendEvent(evt, groupId, data);
    }

    void sendEventWithPriority(event evt, unsigned long priority, unsigned long data = 0) {
        eventHandler.sendEventWithPriority(evt, priority, data);
    }
};


/**
 * Helper class to set up an event receiver client.
 */
class EventReceiver : public LiveTests::LiveConnection {
public:
    EventHandler<LiveTests::TestMessageHandler> eventHandler;

    explicit EventReceiver(std::string_view name)
        : LiveConnection(name), eventHandler(handler)
    {
        eventHandler.logger().level(LogLevel::Debug);
    }

    NotificationGroup<LiveTests::TestMessageHandler> createGroupWithEvent(event evt) {
        return eventHandler
            .createNotificationGroup()
            .withStandardPriority()
            .addEvent(evt);
    }

    template<typename EventType = Messages::EventMsg, typename F>
    void registerEventHandler(event evt, F&& callback, bool autoRemove = false) {
        eventHandler.registerEventHandler<EventType>(evt, std::function<void(const EventType&)>(std::forward<F>(callback)), autoRemove);
    }

    void removeEventHandler(event evt) {
        eventHandler.removeEventHandler(evt);
    }

};


//NOLINTBEGIN(readability-function-cognitive-complexity)

// Test that we can send and receive an event between two clients
TEST(TestEventHandler, SendAndReceiveEvent) {
    EventSender sender("EventSender");
    EventReceiver receiver("EventReceiver");

    std::atomic<bool> receivedEvent{ false };
    std::atomic<EventId> receivedEventId{ 0 };
    std::atomic<unsigned long> receivedData{ 0 };

    ASSERT_TRUE(sender.openAndWait());
    ASSERT_TRUE(receiver.openAndWait());

    // Create a unique test event
    auto testEvt = createTestEvent();

    [[maybe_unused]]
    auto receiverGroup = receiver.createGroupWithEvent(testEvt);
    EXPECT_TRUE(receiver.succeeded()) << "Failed to create notification group on receiver";

    // Register event handler on receiver
    receiver.registerEventHandler<Messages::EventMsg>(
        testEvt,
        [&](const Messages::EventMsg& msg) {
            receivedEvent = true;
            receivedEventId = msg.uEventID;
            receivedData = msg.dwData;
        }
    );

    // Map the event on the sender side
    auto senderGroup = sender.createGroupWithEvent(testEvt);
    EXPECT_TRUE(sender.succeeded()) << "Failed to create notification group on sender";

    // Send event from sender
    sender.sendEvent(testEvt, senderGroup.id(), LiveTests::TEST_EVENT_DATA);
    EXPECT_TRUE(sender.succeeded()) << "Failed to send event from sender";

    // Wait for receiver to get the event
    EXPECT_TRUE(receiver.waitUntil([&receivedEvent]() { return receivedEvent.load(); }));

    EXPECT_TRUE(receivedEvent) << "Receiver did not receive event";
    EXPECT_EQ(receivedEventId, testEvt.id()) << "Received wrong event ID";
    EXPECT_EQ(receivedData, LiveTests::TEST_EVENT_DATA) << "Received wrong data";

    sender.close();
    receiver.close();
}


// Test that two EventHandlers on the same client both receive the same event
TEST(TestEventHandler, MultipleHandlersReceiveEvent) {
    EventSender sender("EventSenderMulti");
    EventReceiver receiver("EventReceiverMulti");

    // Create a second event handler on the receiver
    EventHandler<LiveTests::TestMessageHandler> receiverEventHandler2(receiver.handler);

    std::atomic<bool> receivedEvent1{false};
    std::atomic<bool> receivedEvent2{false};

    ASSERT_TRUE(sender.openAndWait());
    ASSERT_TRUE(receiver.openAndWait());

    // Create a unique test event
    auto testEvt = createTestEvent();

    [[maybe_unused]]
    auto receiverGroup = receiver.createGroupWithEvent(testEvt);

    // Register two handlers on the receiver for the same event
    receiver.registerEventHandler<Messages::EventMsg>(
        testEvt,
        [&]([[maybe_unused]] const Messages::EventMsg& msg) {
            receivedEvent1 = true;
        }
    );

    receiverEventHandler2.registerEventHandler<Messages::EventMsg>(
        testEvt,
        [&]([[maybe_unused]] const Messages::EventMsg& msg) {
            receivedEvent2 = true;
        }
    );

    // Map and send event
    auto senderGroup = sender.createGroupWithEvent(testEvt);
    sender.sendEvent(testEvt, senderGroup.id(), LiveTests::DEFAULT_EVENT_DATA);

    // Wait for both handlers to receive the event
    EXPECT_TRUE(receiver.waitUntil([&receivedEvent1, &receivedEvent2]() {
        return receivedEvent1.load() && receivedEvent2.load();
    }));

    EXPECT_TRUE(receivedEvent1) << "First handler did not receive event";
    EXPECT_TRUE(receivedEvent2) << "Second handler did not receive event";

    sender.close();
    receiver.close();
}


// Test sending event with priority instead of group
TEST(TestEventHandler, SendEventWithPriority) {
    EventSender sender("PrioritySender");
    EventReceiver receiver("PriorityReceiver");

    std::atomic<bool> receivedEvent{false};

    ASSERT_TRUE(sender.openAndWait());
    ASSERT_TRUE(receiver.openAndWait());

    // Create a unique test event
    auto testEvt = createTestEvent();
    [[maybe_unused]]
    auto receiverGroup = receiver.createGroupWithEvent(testEvt);

    receiver.registerEventHandler<Messages::EventMsg>(
        testEvt,
        [&]([[maybe_unused]] const Messages::EventMsg& msg) {
            receivedEvent = true;
        }
    );

    // Map and send event with priority
    [[maybe_unused]]
    auto senderGroup = sender.createGroupWithEvent(testEvt);
    sender.sendEventWithPriority(testEvt, Events::standardPriority, LiveTests::DEFAULT_EVENT_DATA);

    EXPECT_TRUE(receiver.waitUntil([&receivedEvent]() { return receivedEvent.load(); }));
    EXPECT_TRUE(receivedEvent) << "Receiver did not receive priority event";

    sender.close();
    receiver.close();
}


// Test auto-remove functionality for event handlers
TEST(TestEventHandler, AutoRemoveEventHandler) {
    EventSender sender("AutoRemoveSender");
    EventReceiver receiver("AutoRemoveReceiver");

    std::atomic<int> eventCount{0};

    ASSERT_TRUE(sender.openAndWait());
    ASSERT_TRUE(receiver.openAndWait());

    auto testEvt = createTestEvent();
    [[maybe_unused]]
    auto receiverGroup = receiver.createGroupWithEvent(testEvt);

    // Register handler with autoRemove=true
    receiver.registerEventHandler<Messages::EventMsg>(
        testEvt,
        [&]([[maybe_unused]] const Messages::EventMsg& msg) {
            eventCount++;
        },
        true  // autoRemove
    );

    [[maybe_unused]]
    auto senderGroup = sender.createGroupWithEvent(testEvt);

    // Send first event - should be received and handler removed
    sender.sendEvent(testEvt, senderGroup.id(), LiveTests::DEFAULT_EVENT_DATA);
    EXPECT_TRUE(receiver.waitUntil([&eventCount]() { return eventCount.load() >= 1; }));
    EXPECT_EQ(eventCount, 1) << "First event not received";

    // Send second event - should NOT be received (handler auto-removed)
    sender.sendEvent(testEvt, senderGroup.id(), LiveTests::DEFAULT_EVENT_DATA);
    receiver.waitFor(LiveTests::DEFAULT_TIMEOUT);

    EXPECT_EQ(eventCount, 1) << "Event received after handler should have been auto-removed";

    sender.close();
    receiver.close();
}


// Test manual removal of event handler
TEST(TestEventHandler, RemoveEventHandler) {
    EventSender sender("RemoveSender");
    EventReceiver receiver("RemoveReceiver");

    std::atomic<int> eventCount{0};

    ASSERT_TRUE(sender.openAndWait());
    ASSERT_TRUE(receiver.openAndWait());

    auto testEvt = createTestEvent();
    [[maybe_unused]]
    auto receiverGroup = receiver.createGroupWithEvent(testEvt);

    receiver.registerEventHandler<Messages::EventMsg>(
        testEvt,
        [&]([[maybe_unused]] const Messages::EventMsg& msg) {
            eventCount++;
        }
    );

    auto senderGroup = sender.createGroupWithEvent(testEvt);

    // Send first event - should be received
    sender.sendEvent(testEvt, senderGroup.id(), LiveTests::DEFAULT_EVENT_DATA);
    EXPECT_TRUE(receiver.waitUntil([&eventCount]() { return eventCount.load() >= 1; }));
    EXPECT_EQ(eventCount, 1) << "First event not received";

    // Remove the handler
    receiver.removeEventHandler(testEvt);

    // Send second event - should NOT be received
    sender.sendEvent(testEvt, senderGroup.id(), LiveTests::DEFAULT_EVENT_DATA);
    receiver.waitFor(LiveTests::DEFAULT_TIMEOUT);

    EXPECT_EQ(eventCount, 1) << "Event received after handler was removed";

    sender.close();
    receiver.close();
}

//NOLINTEND(readability-function-cognitive-complexity)