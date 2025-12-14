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
#include <utility>
#include <functional>

using namespace SimConnect;

using namespace std::chrono_literals;


// Test: Request known system state (AircraftLoaded) and expect a string result
TEST(TestSystemState, RequestAircraftLoaded) {
    WindowsEventConnection<> connection;
    WindowsEventHandler<> handler(connection);
    std::atomic<bool> gotResult{false};
    std::string result;

    [[maybe_unused]] auto defaultHandlerId = handler.registerDefaultHandler([](const SIMCONNECT_RECV&) {}); // NOLINT(misc-include-cleaner)
    ASSERT_TRUE(connection.open());

    SystemStateHandler<WindowsEventHandler<>> requestHandler(handler);
    requestHandler.enable(handler);

    requestHandler.requestSystemState("AircraftLoaded", [&](std::string value) {
        result = std::move(value);
        gotResult = true;
    });

    // Wait up to 2 seconds for the result
    constexpr int maxAttempts = 20;
    constexpr auto waitInterval = 100ms;
    for (int i = 0; i < maxAttempts && !gotResult; ++i) {
        handler.dispatch(waitInterval);
    }
    EXPECT_TRUE(gotResult) << "Did not receive AircraftLoaded system state";
    EXPECT_FALSE(result.empty()) << "AircraftLoaded system state should not be empty";
    connection.close();
}

// Test: Request known boolean system state (DialogMode) and expect a bool result
TEST(TestSystemState, RequestDialogMode) {
    WindowsEventConnection<> connection;
    WindowsEventHandler<> handler(connection);
    std::atomic<bool> gotResult{false};
    std::atomic<bool> dialogMode{false};

    [[maybe_unused]] auto defaultHandlerId = handler.registerDefaultHandler([](const SIMCONNECT_RECV&) {});
    ASSERT_TRUE(connection.open());

    SystemStateHandler<WindowsEventHandler<>> requestHandler(handler);
    requestHandler.enable(handler);

    requestHandler.requestSystemState("DialogMode", [&](bool value) {
        dialogMode = value;
        gotResult = true;
    });

    // Wait up to 2 seconds for the result
    constexpr int maxAttempts = 20;
    constexpr auto waitInterval = 100ms;
    for (int i = 0; i < maxAttempts && !gotResult; ++i) {
        handler.dispatch(waitInterval);
    }
    EXPECT_TRUE(gotResult) << "Did not receive DialogMode system state";
    // No assert on dialogMode value, just that we got a result
    connection.close();
}

// Test: Request unknown system state and expect an exception
TEST(TestSystemState, ExceptionOnUnknownSystemState) {
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

    requestHandler.requestSystemState("UnknownState", std::function<void(std::string)>([](std::string){})); // NOLINT(performance-unnecessary-value-param)

    // Wait up to 2 seconds for the exception message
    constexpr int maxAttempts = 20;
    constexpr auto waitInterval = 100ms;
    for (int i = 0; i < maxAttempts && !gotException; ++i) {
        handler.dispatch(waitInterval);
    }
    EXPECT_TRUE(gotException) << "Did not receive exception for unknown system state";
    connection.close();
}
