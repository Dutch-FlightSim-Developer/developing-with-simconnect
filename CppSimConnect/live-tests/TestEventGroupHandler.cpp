/*
 * Copyright (c) 2025. Bert Laverman
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
#include <simconnect/events/input_group.hpp>

#include <atomic>
#include <chrono>
#include <string>
#include <string_view>
#include <functional>
#include <initializer_list>


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
 * Helper class to set up an event sender client with event handler support.
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
};


/**
 * Helper class to set up an event receiver client with group handler support.
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

    NotificationGroup<LiveTests::TestMessageHandler> createGroupWithEvents(std::initializer_list<event> events) {
        auto group = eventHandler
            .createNotificationGroup()
            .withStandardPriority();
        
        for (auto evt : events) {
            group.addEvent(evt);
        }
        
        return group;
    }

    template<typename EventType = Messages::EventMsg, typename F>
    void registerEventHandler(event evt, F&& callback, bool autoRemove = false) {
        eventHandler.registerEventHandler<EventType>(evt, std::function<void(const EventType&)>(std::forward<F>(callback)), autoRemove);
    }

    template<typename EventType = Messages::EventMsg, typename F>
    void registerEventGroupHandler(EventGroupId groupId, F&& callback, bool autoRemove = false) {
        eventHandler.registerEventGroupHandler<EventType>(groupId, std::function<void(const EventType&)>(std::forward<F>(callback)), autoRemove);
    }

    void removeEventHandler(event evt) {
        eventHandler.removeEventHandler(evt);
    }

    void removeEventGroupHandler(EventGroupId groupId) {
        eventHandler.removeEventGroupHandler(groupId);
    }
};


//NOLINTBEGIN(readability-function-cognitive-complexity)

// Test that we can register a handler for an entire notification group
TEST(TestEventGroupHandler, ReceiveNotificationGroupEvent) {
    EventSender sender("EventGroupSender");
    EventReceiver receiver("EventGroupReceiver");
    
    std::atomic<bool> receivedGroupEvent{false};
    std::atomic<EventId> receivedEventId{0};

    ASSERT_TRUE(sender.openAndWait());
    ASSERT_TRUE(receiver.openAndWait());

    // Create unique test events
    auto brakeEvt = createTestEvent();
    auto parkingBrakeEvt = createTestEvent();
    
    auto receiverGroup = receiver.createGroupWithEvents({brakeEvt, parkingBrakeEvt});
    EXPECT_TRUE(receiver.succeeded());

    // Register a group handler that should receive ALL events in the group
    receiver.registerEventGroupHandler<Messages::EventMsg>(
        receiverGroup.id(),
        [&](const Messages::EventMsg& msg) {
            receivedGroupEvent = true;
            receivedEventId = msg.uEventID;
        }
    );

    // Map and send event from sender
    auto senderGroup = sender.createGroupWithEvent(brakeEvt);
    sender.sendEvent(brakeEvt, senderGroup.id(), LiveTests::DEFAULT_EVENT_DATA);

    // Wait for the group handler to receive the event
    EXPECT_TRUE(receiver.waitUntil([&receivedGroupEvent]() { return receivedGroupEvent.load(); }));

    EXPECT_TRUE(receivedGroupEvent) << "Group handler did not receive event";
    EXPECT_EQ(receivedEventId, brakeEvt.id()) << "Received wrong event ID";

    sender.close();
    receiver.close();
}




// Test that group handlers and individual event handlers can coexist
TEST(TestEventGroupHandler, GroupAndIndividualHandlers) {
    EventSender sender("GroupAndIndividualSender");
    EventReceiver receiver("GroupAndIndividualReceiver");
    
    std::atomic<bool> receivedGroupEvent{false};
    std::atomic<bool> receivedIndividualEvent{false};

    ASSERT_TRUE(sender.openAndWait());
    ASSERT_TRUE(receiver.openAndWait());

    auto brakeEvt = createTestEvent();
    auto parkingBrakeEvt = createTestEvent();
    
    auto receiverGroup = receiver.createGroupWithEvents({brakeEvt, parkingBrakeEvt});

    // Register both a group handler and an individual event handler
    receiver.registerEventGroupHandler<Messages::EventMsg>(
        receiverGroup.id(),
        [&]([[maybe_unused]] const Messages::EventMsg& msg) {
            receivedGroupEvent = true;
        }
    );

    receiver.registerEventHandler<Messages::EventMsg>(
        brakeEvt,
        [&]([[maybe_unused]] const Messages::EventMsg& msg) {
            receivedIndividualEvent = true;
        }
    );

    // Send an event in the group
    auto senderGroup = sender.createGroupWithEvent(brakeEvt);
    sender.sendEvent(brakeEvt, senderGroup.id(), LiveTests::DEFAULT_EVENT_DATA);

    // Wait for both handlers to receive the event
    EXPECT_TRUE(receiver.waitUntil([&receivedGroupEvent, &receivedIndividualEvent]() { 
        return receivedGroupEvent.load() && receivedIndividualEvent.load(); 
    }));

    EXPECT_TRUE(receivedGroupEvent) << "Group handler did not receive event";
    EXPECT_TRUE(receivedIndividualEvent) << "Individual handler did not receive event";

    sender.close();
    receiver.close();
}




// Test handling multiple events in the same group
TEST(TestEventGroupHandler, MultipleEventsInGroup) {
    EventSender sender("MultipleGroupEventsSender");
    EventReceiver receiver("MultipleGroupEventsReceiver");
    
    std::atomic<int> eventCount{0};
    std::atomic<EventId> lastEventId{0};

    ASSERT_TRUE(sender.openAndWait());
    ASSERT_TRUE(receiver.openAndWait());

    auto brakeEvt = createTestEvent();
    auto parkingBrakeEvt = createTestEvent();
    auto gearEvt = createTestEvent();
    
    auto receiverGroup = receiver.createGroupWithEvents({brakeEvt, parkingBrakeEvt, gearEvt});

    // Register a group handler that counts events
    receiver.registerEventGroupHandler<Messages::EventMsg>(
        receiverGroup.id(),
        [&](const Messages::EventMsg& msg) {
            eventCount++;
            lastEventId = msg.uEventID;
        }
    );

    // Create sender groups and send multiple different events
    auto senderGroup1 = sender.createGroupWithEvent(brakeEvt);
    auto senderGroup2 = sender.createGroupWithEvent(parkingBrakeEvt);
    auto senderGroup3 = sender.createGroupWithEvent(gearEvt);

    sender.sendEvent(brakeEvt, senderGroup1.id(), LiveTests::DEFAULT_EVENT_DATA);
    sender.sendEvent(parkingBrakeEvt, senderGroup2.id(), LiveTests::DEFAULT_EVENT_DATA);
    sender.sendEvent(gearEvt, senderGroup3.id(), LiveTests::DEFAULT_EVENT_DATA);

    // Wait for all three events
    EXPECT_TRUE(receiver.waitUntil([&eventCount]() { return eventCount.load() >= 3; }));

    EXPECT_EQ(eventCount, 3) << "Did not receive all three events";

    sender.close();
    receiver.close();
}




// Test removing a group handler
TEST(TestEventGroupHandler, RemoveGroupHandler) {
    EventSender sender("RemoveGroupHandlerSender");
    EventReceiver receiver("RemoveGroupHandlerReceiver");
    
    std::atomic<int> eventCount{0};

    ASSERT_TRUE(sender.openAndWait());
    ASSERT_TRUE(receiver.openAndWait());

    auto brakeEvt = createTestEvent();
    
    auto receiverGroup = receiver.createGroupWithEvent(brakeEvt);

    // Register a group handler
    receiver.registerEventGroupHandler<Messages::EventMsg>(
        receiverGroup.id(),
        [&]([[maybe_unused]] const Messages::EventMsg& msg) {
            eventCount++;
        }
    );

    auto senderGroup = sender.createGroupWithEvent(brakeEvt);

    // Send an event - should be received
    sender.sendEvent(brakeEvt, senderGroup.id(), LiveTests::DEFAULT_EVENT_DATA);
    EXPECT_TRUE(receiver.waitUntil([&eventCount]() { return eventCount.load() >= 1; }));
    EXPECT_EQ(eventCount, 1) << "First event not received";

    // Remove the group handler
    receiver.removeEventGroupHandler(receiverGroup.id());

    // Send another event - should NOT be received
    sender.sendEvent(brakeEvt, senderGroup.id(), LiveTests::DEFAULT_EVENT_DATA);
    receiver.waitFor(LiveTests::DEFAULT_TIMEOUT);

    EXPECT_EQ(eventCount, 1) << "Event received after handler was removed";

    sender.close();
    receiver.close();
}




// Test auto-remove functionality for group handlers
TEST(TestEventGroupHandler, AutoRemoveGroupHandler) {
    EventSender sender("AutoRemoveGroupHandlerSender");
    EventReceiver receiver("AutoRemoveGroupHandlerReceiver");
    
    std::atomic<int> eventCount{0};

    ASSERT_TRUE(sender.openAndWait());
    ASSERT_TRUE(receiver.openAndWait());

    auto brakeEvt = createTestEvent();
    
    auto receiverGroup = receiver.createGroupWithEvent(brakeEvt);

    // Register a group handler with autoRemove=true
    receiver.registerEventGroupHandler<Messages::EventMsg>(
        receiverGroup.id(),
        [&]([[maybe_unused]] const Messages::EventMsg& msg) {
            eventCount++;
        },
        true  // autoRemove
    );

    auto senderGroup = sender.createGroupWithEvent(brakeEvt);

    // Send first event - should be received and handler removed
    sender.sendEvent(brakeEvt, senderGroup.id(), LiveTests::DEFAULT_EVENT_DATA);
    EXPECT_TRUE(receiver.waitUntil([&eventCount]() { return eventCount.load() >= 1; }));
    EXPECT_EQ(eventCount, 1) << "First event not received";

    // Send second event - should NOT be received (handler auto-removed)
    sender.sendEvent(brakeEvt, senderGroup.id(), LiveTests::DEFAULT_EVENT_DATA);
    receiver.waitFor(LiveTests::DEFAULT_TIMEOUT);

    EXPECT_EQ(eventCount, 1) << "Event received after handler should have been auto-removed";

    sender.close();
    receiver.close();
}




// Test multiple groups with separate handlers
TEST(TestEventGroupHandler, MultipleGroupHandlers) {
    EventSender sender("MultipleGroupHandlersSender");
    EventReceiver receiver("MultipleGroupHandlersReceiver");
    
    std::atomic<int> group1Count{0};
    std::atomic<int> group2Count{0};

    ASSERT_TRUE(sender.openAndWait());
    ASSERT_TRUE(receiver.openAndWait());

    auto brakeEvt = createTestEvent();
    auto flapEvt = createTestEvent();
    
    auto receiverGroup1 = receiver.createGroupWithEvent(brakeEvt);
    auto receiverGroup2 = receiver.createGroupWithEvent(flapEvt);

    // Register handlers for both groups
    receiver.registerEventGroupHandler<Messages::EventMsg>(
        receiverGroup1.id(),
        [&]([[maybe_unused]] const Messages::EventMsg& msg) {
            group1Count++;
        }
    );

    receiver.registerEventGroupHandler<Messages::EventMsg>(
        receiverGroup2.id(),
        [&]([[maybe_unused]] const Messages::EventMsg& msg) {
            group2Count++;
        }
    );

    // Send events to different groups
    auto senderGroup1 = sender.createGroupWithEvent(brakeEvt);
    auto senderGroup2 = sender.createGroupWithEvent(flapEvt);

    sender.sendEvent(brakeEvt, senderGroup1.id(), LiveTests::DEFAULT_EVENT_DATA);
    sender.sendEvent(flapEvt, senderGroup2.id(), LiveTests::DEFAULT_EVENT_DATA);
    sender.sendEvent(brakeEvt, senderGroup1.id(), LiveTests::DEFAULT_EVENT_DATA);

    EXPECT_TRUE(receiver.waitUntil([&group1Count, &group2Count]() { 
        return group1Count.load() >= 2 && group2Count.load() >= 1; 
    }));

    EXPECT_EQ(group1Count, 2) << "Group 1 did not receive correct number of events";
    EXPECT_EQ(group2Count, 1) << "Group 2 did not receive correct number of events";

    sender.close();
    receiver.close();
}




// Test input group with group handler
TEST(TestEventGroupHandler, InputGroupHandler) {
    EventReceiver receiver("InputGroupHandlerTest");

    ASSERT_TRUE(receiver.openAndWait());

    auto brakeEvt = createTestEvent();
    
    // Create an input group
    auto inputGroup = receiver.eventHandler
        .createInputGroup()
        .withStandardPriority()
        .addEvent(brakeEvt, "Shift+B");

    EXPECT_TRUE(receiver.succeeded());

    // Register a group handler for the input group
    std::atomic<bool> receivedEvent{false};
    receiver.registerEventGroupHandler<Messages::EventMsg>(
        inputGroup.id(),
        [&]([[maybe_unused]] const Messages::EventMsg& msg) {
            receivedEvent = true;
        }
    );

    // Note: Input events are triggered by user input, so we can only test
    // that the handler registration doesn't crash and the group is created properly
    EXPECT_TRUE(receiver.succeeded());
    EXPECT_FALSE(receivedEvent) << "Should not have received event without user input";

    receiver.close();
}

//NOLINTEND(readability-function-cognitive-complexity)