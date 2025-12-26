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
#include <simconnect/requests/system_state_handler.hpp>

#include <atomic>
#include <chrono>
#include <string>
#include <functional>

using namespace SimConnect;

using namespace std::chrono_literals;


TEST(TestConnection, ReceivesOpenMessage) {
    WindowsEventConnection<> connection;
    WindowsEventHandler<> handler(connection);
    std::atomic<bool> gotOpen{false};

    [[maybe_unused]] auto openHandlerId = handler.registerHandler<SIMCONNECT_RECV_OPEN>(SIMCONNECT_RECV_ID_OPEN, [&](const SIMCONNECT_RECV_OPEN&) { // NOLINT(misc-include-cleaner)
        gotOpen = true;
    });
    [[maybe_unused]] auto defaultHandlerId = handler.registerDefaultHandler([](const SIMCONNECT_RECV&){}); // NOLINT(misc-include-cleaner)

    ASSERT_TRUE(connection.open());

    // Wait up to 2 seconds for the open message
    static constexpr auto twoSeconds = 2000ms;
    handler.handleUntilOrTimeout([&gotOpen]() { return gotOpen.load(); }, twoSeconds);

    EXPECT_TRUE(gotOpen) << "Did not receive SIMCONNECT_RECV_ID_OPEN from CppSimConnect abstraction";
    connection.close();
}

TEST(TestConnection, GracefulClose) {
    WindowsEventConnection<> connection;
    ASSERT_TRUE(connection.open());
    connection.close();
    EXPECT_FALSE(connection.isOpen()) << "Connection should be closed after calling close()";
}

TEST(TestConnection, ExceptionOnUnknownSystemState) {
    WindowsEventConnection<> connection;
    WindowsEventHandler<> handler(connection);
    std::atomic<bool> gotException{false};

    [[maybe_unused]] auto exceptionHandlerId = handler.registerHandler<SIMCONNECT_RECV_EXCEPTION>(SIMCONNECT_RECV_ID_EXCEPTION, [&](const SIMCONNECT_RECV_EXCEPTION&) { // NOLINT(misc-include-cleaner)
        gotException = true;
    });
    [[maybe_unused]] auto defaultHandlerId = handler.registerDefaultHandler([](const SIMCONNECT_RECV&) {});

    ASSERT_TRUE(connection.open());

    SystemStateHandler<WindowsEventHandler<>> requestHandler(handler);
    requestHandler.enable(handler);

    // Request an unknown system state, should trigger exception
    // Use the string overload explicitly
    requestHandler.requestSystemState("UnknownState", std::function<void(std::string)>([](std::string){})); // NOLINT(performance-unnecessary-value-param)

    // Wait up to 2 seconds for the exception message
    static constexpr auto twoSeconds = 2000ms;
    handler.handleUntilOrTimeout([&gotException]() { return gotException.load(); }, twoSeconds);
 
    EXPECT_TRUE(gotException) << "Did not receive exception for unknown system state";
    connection.close();
}