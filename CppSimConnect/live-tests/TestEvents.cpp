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
 * See the License for the specific language governing permissions and limitations under the License.
 */

#include "gtest/gtest.h"

#include <simconnect/windows_event_connection.hpp>
#include <simconnect/windows_event_handler.hpp>
#include <simconnect/events/event_handler.hpp>
#include <simconnect/events/events.hpp>

#include <atomic>
#include <chrono>
#include <iostream>


using namespace SimConnect;
using namespace std::chrono_literals;


//NOLINTBEGIN(readability-function-cognitive-complexity)
TEST(TestEvents, MapEventTwice) {
    WindowsEventConnection<> connection("MapEventTwiceTest");
    WindowsEventHandler<> handler(connection);
    EventHandler<WindowsEventHandler<>> eventHandler(handler);
    
    std::atomic<bool> gotOpen{false};
    std::atomic<bool> gotException{false};

    handler.registerHandler<SIMCONNECT_RECV_OPEN>(SIMCONNECT_RECV_ID_OPEN, [&](const SIMCONNECT_RECV_OPEN&) { gotOpen = true; } ); // NOLINT(misc-include-cleaner)

    handler.registerHandler<SIMCONNECT_RECV_EXCEPTION>(SIMCONNECT_RECV_ID_EXCEPTION, [&](const SIMCONNECT_RECV_EXCEPTION& ex) { // NOLINT(misc-include-cleaner)
            gotException = true;
            // Log the exception for debugging
            std::cerr << "SimConnect Exception: " << ex.dwException << '\n';
        }
    );

    ASSERT_TRUE(connection.open());

    // Wait for open message
    static constexpr auto twoSeconds = 100ms;
    handler.handleUntilOrTimeout([&gotOpen]() { return gotOpen.load(); }, twoSeconds);
    ASSERT_TRUE(gotOpen);

    // Get an event
    auto brakeEvt = event::get("Brakes");
    
    // Initially not mapped
    EXPECT_FALSE(brakeEvt.isMapped());

    // Map it the first time
    eventHandler.mapEvent(brakeEvt);
    EXPECT_TRUE(connection.succeeded());
    EXPECT_FALSE(gotException);
    
    // Now it should be marked as mapped
    EXPECT_TRUE(brakeEvt.isMapped());

    // Process any potential messages
    handler.handleFor(twoSeconds);
    EXPECT_FALSE(gotException);

    // Map the same event again - should be silently skipped
    eventHandler.mapEvent(brakeEvt);
    EXPECT_TRUE(connection.succeeded());
    EXPECT_FALSE(gotException);
    
    // Still mapped
    EXPECT_TRUE(brakeEvt.isMapped());

    // Wait a bit longer to ensure no delayed exceptions
    handler.handleUntilOrTimeout([&gotException]() { return gotException.load(); }, twoSeconds);

    // Verify no exception was received
    EXPECT_FALSE(gotException) << "Mapping the same event twice should not cause an exception";

    connection.close();
}

TEST(TestEvents, MapMultipleEventsTwice) {
    WindowsEventConnection<> connection("MapMultipleEventsTwiceTest");
    WindowsEventHandler<> handler(connection);
    EventHandler<WindowsEventHandler<>> eventHandler(handler);
    
    std::atomic<bool> gotOpen{false};
    std::atomic<bool> gotException{false};
    std::atomic<int> exceptionCount{0};

    handler.registerHandler<SIMCONNECT_RECV_OPEN>(
        SIMCONNECT_RECV_ID_OPEN,
        [&](const SIMCONNECT_RECV_OPEN&) {
            gotOpen = true;
        }
    );

    handler.registerHandler<SIMCONNECT_RECV_EXCEPTION>(
        SIMCONNECT_RECV_ID_EXCEPTION,
        [&](const SIMCONNECT_RECV_EXCEPTION& ex) {
            gotException = true;
            exceptionCount++;
            std::cerr << "SimConnect Exception #" << exceptionCount.load() 
                      << ": " << ex.dwException << '\n';
        }
    );

    ASSERT_TRUE(connection.open());

    // Wait for open message
    static constexpr auto twoSeconds = 100ms;
    handler.handleUntilOrTimeout([&gotOpen]() { return gotOpen.load(); }, twoSeconds);
    ASSERT_TRUE(gotOpen);

    // Create several events
    auto brakeEvt = event::get("Brakes");
    auto parkingBrakeEvt = event::get("Parking_Brakes");
    auto flapsUpEvt = event::get("Flaps_Up");
    auto flapsDownEvt = event::get("Flaps_Down");

    // Verify none are mapped initially
    EXPECT_FALSE(brakeEvt.isMapped());
    EXPECT_FALSE(parkingBrakeEvt.isMapped());
    EXPECT_FALSE(flapsUpEvt.isMapped());
    EXPECT_FALSE(flapsDownEvt.isMapped());

    // Map them all once
    eventHandler.mapEvent(brakeEvt);
    eventHandler.mapEvent(parkingBrakeEvt);
    eventHandler.mapEvent(flapsUpEvt);
    eventHandler.mapEvent(flapsDownEvt);

    EXPECT_TRUE(connection.succeeded());
    
    // Verify all are now mapped
    EXPECT_TRUE(brakeEvt.isMapped());
    EXPECT_TRUE(parkingBrakeEvt.isMapped());
    EXPECT_TRUE(flapsUpEvt.isMapped());
    EXPECT_TRUE(flapsDownEvt.isMapped());

    // Process messages
    handler.handleUntilOrTimeout([&gotException]() { return gotException.load(); }, twoSeconds);

    EXPECT_FALSE(gotException) << "We shouldn't have received an exception after one mapping";

    // Map them all again (as would happen if added to multiple groups)
    // These should all be silently skipped
    eventHandler.mapEvent(brakeEvt);
    eventHandler.mapEvent(parkingBrakeEvt);
    eventHandler.mapEvent(flapsUpEvt);
    eventHandler.mapEvent(flapsDownEvt);

    EXPECT_TRUE(connection.succeeded());

    // Wait for potential exceptions
    handler.handleUntilOrTimeout([&gotException]() { return gotException.load(); }, twoSeconds);

    // Verify no exceptions
    EXPECT_FALSE(gotException) << "Mapping multiple events twice should not cause exceptions. Got " 
                               << exceptionCount.load() << " exception(s)";

    connection.close();
}

TEST(TestEvents, MappedStatusAfterClose) {
    WindowsEventConnection<> connection("MappedStatusTest");
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
    static constexpr auto twoSeconds = 100ms;
    handler.handleUntilOrTimeout([&gotOpen]() { return gotOpen.load(); }, twoSeconds);
    ASSERT_TRUE(gotOpen);

    // Create and map an event
    auto testEvt = event::get("TestEvent");
    EXPECT_FALSE(testEvt.isMapped());
    
    eventHandler.mapEvent(testEvt);
    EXPECT_TRUE(testEvt.isMapped());

    // Close the connection - this should clear all mapped flags
    connection.close();
    
    // Verify the event is no longer marked as mapped
    EXPECT_FALSE(testEvt.isMapped()) << "Mapped flags should be cleared when connection is closed";
}
//NOLINTEND(readability-function-cognitive-complexity)