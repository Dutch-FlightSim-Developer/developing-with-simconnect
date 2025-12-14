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

#include <simconnect/events/system_events.hpp>
#include <simconnect/events/event_handler.hpp>

#include <atomic>
#include <chrono>

using namespace SimConnect;


static constexpr auto fiveSeconds = std::chrono::seconds(5);


// Test that we can receive a timed event using the WindowsEventHandler and EventHandler
TEST(TestEventHandler, ReceiveTimedEvent) {
    WindowsEventConnection<> connection;
    WindowsEventHandler<> handler(connection);
    EventHandler<WindowsEventHandler<>> eventHandler(handler);
    
    std::atomic<bool> receivedEvent{false};

    ASSERT_TRUE(connection.open());

    auto oneSecondEvent = Events::oneSec();

    eventHandler.registerEventHandler<SIMCONNECT_RECV_EVENT>(oneSecondEvent, [&]([[maybe_unused]] const SIMCONNECT_RECV_EVENT& msg) { // NOLINT(misc-include-cleaner)
        receivedEvent = true;
    });
	connection.subscribeToSystemEvent(oneSecondEvent);

    // Wait for event
    handler.dispatch(fiveSeconds);

    EXPECT_TRUE(receivedEvent) << "Did not receive event";

	connection.unsubscribeFromSystemEvent(oneSecondEvent);
	receivedEvent = false;

	// Wait for event again
	handler.dispatch(fiveSeconds);

	EXPECT_FALSE(receivedEvent) << "Received event after unsubscribing";

    connection.close();
}


// Test that two EventHandlers both receive the same event
TEST(TestEventHandler, MultipleHandlersReceiveEvent) {
    WindowsEventConnection<> connection;
    WindowsEventHandler<> handler(connection);
    EventHandler<WindowsEventHandler<>> eventHandler1(handler);
    EventHandler<WindowsEventHandler<>> eventHandler2(handler);
    
    std::atomic<bool> receivedEvent1{false};
    std::atomic<bool> receivedEvent2{false};
    ASSERT_TRUE(connection.open());
    auto oneSecondEvent = Events::oneSec();
    eventHandler1.registerEventHandler<SIMCONNECT_RECV_EVENT>(oneSecondEvent, [&]([[maybe_unused]] const SIMCONNECT_RECV_EVENT& msg) { // NOLINT(misc-include-cleaner)
        receivedEvent1 = true;
    });
    eventHandler2.registerEventHandler<SIMCONNECT_RECV_EVENT>(oneSecondEvent, [&]([[maybe_unused]] const SIMCONNECT_RECV_EVENT& msg) { // NOLINT(misc-include-cleaner)
        receivedEvent2 = true;
    });
    connection.subscribeToSystemEvent(oneSecondEvent);
    // Wait for event
    handler.dispatch(fiveSeconds);
    EXPECT_TRUE(receivedEvent1) << "First handler did not receive event";
    EXPECT_TRUE(receivedEvent2) << "Second handler did not receive event";
    connection.unsubscribeFromSystemEvent(oneSecondEvent);
    connection.close();
}