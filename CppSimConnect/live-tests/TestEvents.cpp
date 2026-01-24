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

#include <gtest/gtest.h>

#include "live_connection.hpp"

#include <simconnect/simconnect.hpp>

#include <simconnect/events/events.hpp>
#include <simconnect/events/event_handler.hpp>

#include <atomic>
#include <chrono>


using namespace SimConnect;
using namespace std::chrono_literals;


//NOLINTBEGIN(readability-function-cognitive-complexity)

TEST(TestEvents, MapEventTwice) {
    LiveTests::LiveConnection connection("MapEventTwiceTest");
    EventHandler<LiveTests::TestMessageHandler> eventHandler(connection.handler);
    
    std::atomic<bool> gotException{false};

    connection.handler.registerHandler<Messages::ExceptionMsg>(Messages::exception, [&]([[maybe_unused]] const Messages::ExceptionMsg& ex) {
            gotException = true;
        }
    );

    ASSERT_TRUE(connection.openAndWait());

    // Get an event
    auto brakeEvt = event::get("Brakes");
    
    // Map it the first time
    eventHandler.mapEvent(brakeEvt);
    EXPECT_TRUE(connection.succeeded());
    EXPECT_FALSE(gotException);

    // Process any potential messages
    connection.waitFor(LiveTests::DEFAULT_TIMEOUT);
    EXPECT_FALSE(gotException);

    // Map the same event again - should be silently skipped
    eventHandler.mapEvent(brakeEvt);
    EXPECT_TRUE(connection.succeeded());
    EXPECT_FALSE(gotException);
    

    // Wait a bit longer to ensure no delayed exceptions
    connection.waitUntil([&gotException]() { return gotException.load(); });

    // Verify no exception was received
    EXPECT_FALSE(gotException) << "Mapping the same event twice should not cause an exception";

    connection.close();
}

TEST(TestEvents, MapMultipleEventsTwice) {
    LiveTests::LiveConnection connection("MapEventTwiceTest");
    EventHandler<LiveTests::TestMessageHandler> eventHandler(connection.handler);

    std::atomic<int> exceptionCount{ 0 };

    connection.handler.registerHandler<Messages::ExceptionMsg>(Messages::exception, [&]([[maybe_unused]] const Messages::ExceptionMsg& ex) {
        exceptionCount++;
        }
    );

    ASSERT_TRUE(connection.openAndWait());

    ASSERT_TRUE(connection.open());

    // Create several events
    auto brakeEvt = event::get("Brakes");
    auto parkingBrakeEvt = event::get("Parking_Brakes");
    auto flapsUpEvt = event::get("Flaps_Up");
    auto flapsDownEvt = event::get("Flaps_Down");

    // Map them all once
    eventHandler.mapEvent(brakeEvt);
    eventHandler.mapEvent(parkingBrakeEvt);
    eventHandler.mapEvent(flapsUpEvt);
    eventHandler.mapEvent(flapsDownEvt);

    EXPECT_TRUE(connection.succeeded());

    // Process messages
    connection.waitUntil([&exceptionCount]() { return exceptionCount.load() > 0; });

    EXPECT_FALSE(exceptionCount.load() > 0) << "We shouldn't have received an exception after one mapping";

    // Map them all again (as would happen if added to multiple groups)
    // These should all be silently skipped
    eventHandler.mapEvent(brakeEvt);
    eventHandler.mapEvent(parkingBrakeEvt);
    eventHandler.mapEvent(flapsUpEvt);
    eventHandler.mapEvent(flapsDownEvt);

    EXPECT_TRUE(connection.succeeded());

    // Wait for potential exceptions
    connection.waitUntil([&exceptionCount]() { return exceptionCount.load() > 0; });

    // Verify no exceptions
    EXPECT_FALSE(exceptionCount.load() > 0) << "Mapping multiple events twice should not cause exceptions. Got " 
                               << exceptionCount.load() << " exception(s)";

    connection.close();
}

//NOLINTEND(readability-function-cognitive-complexity)