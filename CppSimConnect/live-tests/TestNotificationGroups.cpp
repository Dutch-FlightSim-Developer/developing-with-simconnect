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

#include "gtest/gtest.h"

#include <simconnect/windows_event_connection.hpp>
#include <simconnect/windows_event_handler.hpp>

#include <simconnect/events/events.hpp>
#include <simconnect/events/event_handler.hpp>
#include <simconnect/events/notification_group.hpp>

#include <atomic>
#include <chrono>

using namespace SimConnect;
using namespace std::chrono_literals;


//NOLINTBEGIN(readability-function-cognitive-complexity)

TEST(TestNotificationGroups, BasicGroupCreation) {
    WindowsEventConnection<> connection("NotificationGroupTest");
    WindowsEventHandler<> handler(connection);
    EventHandler<WindowsEventHandler<>> eventHandler(handler);
    
    std::atomic<bool> gotOpen{false};

    handler.registerHandler<SIMCONNECT_RECV_OPEN>(SIMCONNECT_RECV_ID_OPEN, [&](const SIMCONNECT_RECV_OPEN&) { // NOLINT(misc-include-cleaner)
            gotOpen = true;
        }
    );

    ASSERT_TRUE(connection.open());

    // Wait for open message
    static constexpr auto twoSeconds = 2s;
    handler.handleUntilOrTimeout([&gotOpen]() { return gotOpen.load(); }, twoSeconds);
    ASSERT_TRUE(gotOpen);

    // Create an event - no need to map it manually anymore!
    auto brakeEvt = event::get("Brakes");
    EXPECT_FALSE(brakeEvt.isMapped()) << "Event should not be mapped before adding to group";

    // Create a notification group and add event - mapping happens automatically
    [[maybe_unused]]
    auto group = eventHandler
        .createNotificationGroup()
        .withHighestPriority()
        .addEvent(brakeEvt);
    
    EXPECT_TRUE(connection.succeeded());
    EXPECT_TRUE(brakeEvt.isMapped()) << "Event should be automatically mapped when added to group";

    connection.close();
}

TEST(TestNotificationGroups, FluentAPIUsage) {
    WindowsEventConnection<> connection("FluentAPITest");
    WindowsEventHandler<> handler(connection);
    EventHandler<WindowsEventHandler<>> eventHandler(handler);
    
    std::atomic<bool> gotOpen{false};

    handler.registerHandler<SIMCONNECT_RECV_OPEN>(
        SIMCONNECT_RECV_ID_OPEN,
        [&](const SIMCONNECT_RECV_OPEN&) {
            gotOpen = true;
        }
    );

    ASSERT_TRUE(connection.open());

    // Wait for open message
    static constexpr auto twoSeconds = 2s;
    handler.handleUntilOrTimeout([&gotOpen]() { return gotOpen.load(); }, twoSeconds);
    ASSERT_TRUE(gotOpen);

    // Use factory method and fluent API to configure notification group
    [[maybe_unused]]
    auto group = eventHandler
        .createNotificationGroup()
        .withStandardPriority()
        .addEvent(event::get("Brakes"))
        .addEvent(event::get("ParkingBrakes"))
        .addMaskableEvent(event::get("FlapsUp"));

    EXPECT_TRUE(connection.succeeded());

    connection.close();
}

TEST(TestNotificationGroups, PriorityHandling) {
    WindowsEventConnection<> connection("PriorityTest");
    WindowsEventHandler<> handler(connection);
    EventHandler<WindowsEventHandler<>> eventHandler(handler);

    std::atomic<bool> gotOpen{false};

    handler.registerHandler<SIMCONNECT_RECV_OPEN>(
        SIMCONNECT_RECV_ID_OPEN,
        [&](const SIMCONNECT_RECV_OPEN&) {
            gotOpen = true;
        }
    );

    ASSERT_TRUE(connection.open());

    // Wait for open message
    static constexpr auto twoSeconds = 2s;
    handler.handleUntilOrTimeout([&gotOpen]() { return gotOpen.load(); }, twoSeconds);
    ASSERT_TRUE(gotOpen);

    // Create two groups with different priorities
    [[maybe_unused]]
    auto highPriorityGroup = eventHandler
        .createNotificationGroup()
        .withHighestPriority()
        .addEvent(event::get("Brakes"));

    [[maybe_unused]]
    auto lowPriorityGroup = eventHandler
        .createNotificationGroup()
        .withLowestPriority()
        .addEvent(event::get("ParkingBrakes"));

    EXPECT_TRUE(connection.succeeded());

    connection.close();
}

TEST(TestNotificationGroups, MaskableEvents) {
    WindowsEventConnection<> connection("MaskableEventsTest");
    WindowsEventHandler<> handler(connection);
    EventHandler<WindowsEventHandler<>> eventHandler(handler);

    std::atomic<bool> gotOpen{false};

    handler.registerHandler<SIMCONNECT_RECV_OPEN>(
        SIMCONNECT_RECV_ID_OPEN,
        [&](const SIMCONNECT_RECV_OPEN&) {
            gotOpen = true;
        }
    );

    ASSERT_TRUE(connection.open());

    // Wait for open message
    static constexpr auto twoSeconds = 2s;
    handler.handleUntilOrTimeout([&gotOpen]() { return gotOpen.load(); }, twoSeconds);
    ASSERT_TRUE(gotOpen);

    // Create group with both maskable and non-maskable events
    [[maybe_unused]]
    auto group = eventHandler
        .createNotificationGroup()
        .withMaskablePriority()
        .addMaskableEvent(event::get("Brakes"))
        .addEvent(event::get("ParkingBrakes"));

    EXPECT_TRUE(connection.succeeded());

    connection.close();
}

TEST(TestNotificationGroups, MultipleGroupsPerClient) {
    WindowsEventConnection<> connection("MultiGroupTest");
    WindowsEventHandler<> handler(connection);
    EventHandler<WindowsEventHandler<>> eventHandler(handler);

    std::atomic<bool> gotOpen{false};

    handler.registerHandler<SIMCONNECT_RECV_OPEN>(
        SIMCONNECT_RECV_ID_OPEN,
        [&](const SIMCONNECT_RECV_OPEN&) {
            gotOpen = true;
        }
    );

    ASSERT_TRUE(connection.open());

    // Wait for open message
    static constexpr auto twoSeconds = 2s;
    handler.handleUntilOrTimeout([&gotOpen]() { return gotOpen.load(); }, twoSeconds);
    ASSERT_TRUE(gotOpen);

    // No need to map events since we're not transmitting them
    // The event::get() system maintains its own internal registry

    // Group 1: Brake-related events with highest priority
    [[maybe_unused]]
    auto group1 = eventHandler
        .createNotificationGroup()
        .withHighestPriority()
        .addEvent(event::get("Brakes"));

    // Group 2: Flap-related events with standard priority
    [[maybe_unused]]
    auto group2 = eventHandler
        .createNotificationGroup()
        .withStandardPriority()
        .addEvent(event::get("FlapsUp"))
        .addEvent(event::get("FlapsDown"));

    // Group 3: Landing gear events with lowest priority
    [[maybe_unused]]
    auto group3 = eventHandler
        .createNotificationGroup()
        .withLowestPriority()
        .addEvent(event::get("GearUp"))
        .addEvent(event::get("GearDown"));

    EXPECT_TRUE(connection.succeeded());

    connection.close();
}

TEST(TestNotificationGroups, RemoveAndClearEvents) {
    WindowsEventConnection<> connection("RemoveClearTest");
    WindowsEventHandler<> handler(connection);
    EventHandler<WindowsEventHandler<>> eventHandler(handler);

    std::atomic<bool> gotOpen{false};

    handler.registerHandler<SIMCONNECT_RECV_OPEN>(
        SIMCONNECT_RECV_ID_OPEN,
        [&](const SIMCONNECT_RECV_OPEN&) {
            gotOpen = true;
        }
    );

    ASSERT_TRUE(connection.open());

    // Wait for open message
    static constexpr auto twoSeconds = 2s;
    handler.handleUntilOrTimeout([&gotOpen]() { return gotOpen.load(); }, twoSeconds);
    ASSERT_TRUE(gotOpen);

    // Create group and add multiple events
    auto group = eventHandler
        .createNotificationGroup()
        .withStandardPriority()
        .addEvent(event::get("Brakes"))
        .addEvent(event::get("FlapsUp"))
        .addEvent(event::get("GearUp"));
    
    EXPECT_TRUE(connection.succeeded());

    // Remove one event
    group.removeEvent(event::get("FlapsUp"));
    EXPECT_TRUE(connection.succeeded());

    // Clear all events from group
    group.clear();
    EXPECT_TRUE(connection.succeeded());

    connection.close();
}

TEST(TestNotificationGroups, AllPriorityLevels) {
    WindowsEventConnection<> connection("PriorityLevelsTest");
    WindowsEventHandler<> handler(connection);
    EventHandler<WindowsEventHandler<>> eventHandler(handler);

    std::atomic<bool> gotOpen{false};

    handler.registerHandler<SIMCONNECT_RECV_OPEN>(
        SIMCONNECT_RECV_ID_OPEN,
        [&](const SIMCONNECT_RECV_OPEN&) {
            gotOpen = true;
        }
    );

    ASSERT_TRUE(connection.open());

    // Wait for open message
    static constexpr auto twoSeconds = 2s;
    handler.handleUntilOrTimeout([&gotOpen]() { return gotOpen.load(); }, twoSeconds);
    ASSERT_TRUE(gotOpen);

    // Test all five priority levels
    auto highestGroup = eventHandler
        .createNotificationGroup()
        .withHighestPriority()
        .addEvent(event::get("Event1"));
    EXPECT_EQ(highestGroup.priority(), SIMCONNECT_GROUP_PRIORITY_HIGHEST); // NOLINT(misc-include-cleaner)

    auto maskableGroup = eventHandler
        .createNotificationGroup()
        .withMaskablePriority()
        .addEvent(event::get("Event2"));
    EXPECT_EQ(maskableGroup.priority(), SIMCONNECT_GROUP_PRIORITY_HIGHEST_MASKABLE); // NOLINT(misc-include-cleaner)

    auto standardGroup = eventHandler
        .createNotificationGroup()
        .withStandardPriority()
        .addEvent(event::get("Event3"));
    EXPECT_EQ(standardGroup.priority(), SIMCONNECT_GROUP_PRIORITY_STANDARD); // NOLINT(misc-include-cleaner)

    auto defaultGroup = eventHandler
        .createNotificationGroup()
        .withDefaultPriority()
        .addEvent(event::get("Event4"));
    EXPECT_EQ(defaultGroup.priority(), SIMCONNECT_GROUP_PRIORITY_DEFAULT); // NOLINT(misc-include-cleaner)

    auto lowestGroup = eventHandler
        .createNotificationGroup()
        .withLowestPriority()
        .addEvent(event::get("Event5"));
    EXPECT_EQ(lowestGroup.priority(), SIMCONNECT_GROUP_PRIORITY_LOWEST); // NOLINT(misc-include-cleaner)

    EXPECT_TRUE(connection.succeeded());

    connection.close();
}

TEST(TestNotificationGroups, IndependentGroupsAcrossClients) {
    // Create two separate connections
    WindowsEventConnection<> connection1("GroupClient1");
    WindowsEventConnection<> connection2("GroupClient2");

    WindowsEventHandler<> handler1(connection1);
    WindowsEventHandler<> handler2(connection2);

    EventHandler<WindowsEventHandler<>> eventHandler1(handler1);
    EventHandler<WindowsEventHandler<>> eventHandler2(handler2);

    std::atomic<bool> client1GotOpen{false};
    std::atomic<bool> client2GotOpen{false};

    // Setup handlers
    handler1.registerHandler<SIMCONNECT_RECV_OPEN>(
        SIMCONNECT_RECV_ID_OPEN,
        [&](const SIMCONNECT_RECV_OPEN&) {
            client1GotOpen = true;
        }
    );

    handler2.registerHandler<SIMCONNECT_RECV_OPEN>(
        SIMCONNECT_RECV_ID_OPEN,
        [&](const SIMCONNECT_RECV_OPEN&) {
            client2GotOpen = true;
        }
    );

    // Open both connections
    ASSERT_TRUE(connection1.open());
    ASSERT_TRUE(connection2.open());

    // Wait for open message
    static constexpr auto twoSeconds = 2s;
    handler1.handleUntilOrTimeout([&client1GotOpen]() { return client1GotOpen.load(); }, twoSeconds);
    handler2.handleUntilOrTimeout([&client2GotOpen]() { return client2GotOpen.load(); }, twoSeconds);
    ASSERT_TRUE(client1GotOpen);
    ASSERT_TRUE(client2GotOpen);

    // Each client creates its own notification group
    [[maybe_unused]]
    auto group1 = eventHandler1
        .createNotificationGroup()
        .withHighestPriority()
        .addEvent(event::get("Brakes"));
    EXPECT_TRUE(connection1.succeeded());

    [[maybe_unused]]
    auto group2 = eventHandler2
        .createNotificationGroup()
        .withHighestPriority()  // Same priority, but independent
        .addEvent(event::get("FlapsUp"));
    EXPECT_TRUE(connection2.succeeded());

    // Verify groups are independent (both succeed)
    EXPECT_TRUE(connection1.succeeded());
    EXPECT_TRUE(connection2.succeeded());

    // Clean up
    connection1.close();
    connection2.close();
}

//NOLINTEND(readability-function-cognitive-complexity)