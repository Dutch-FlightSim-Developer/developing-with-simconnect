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
#include <memory>
#include <string>

using namespace SimConnect;
using namespace std::chrono_literals;


//NOLINTBEGIN(readability-function-cognitive-complexity)

TEST(TestMultiClient, MultipleIndependentConnections) {
    // Create two separate connections with different names
    WindowsEventConnection<> connection1("TestClient1");
    WindowsEventConnection<> connection2("TestClient2");

    // Create separate handlers for each connection
    WindowsEventHandler<> handler1(connection1);
    WindowsEventHandler<> handler2(connection2);

    // Track open messages independently
    std::atomic<bool> client1GotOpen{false};
    std::atomic<bool> client2GotOpen{false};

    // Track application names to verify independence using smart pointers
    std::unique_ptr<std::string> client1AppName;
    std::unique_ptr<std::string> client2AppName;

    // Register handlers for each client
    [[maybe_unused]] auto open1HandlerId = handler1.registerHandler<SIMCONNECT_RECV_OPEN>(SIMCONNECT_RECV_ID_OPEN, [&](const SIMCONNECT_RECV_OPEN& msg) { // NOLINT(misc-include-cleaner)
            client1GotOpen = true;
            client1AppName = std::make_unique<std::string>(msg.szApplicationName);
        }
    );

    [[maybe_unused]] auto open2HandlerId = handler2.registerHandler<SIMCONNECT_RECV_OPEN>(
        SIMCONNECT_RECV_ID_OPEN, 
        [&](const SIMCONNECT_RECV_OPEN& msg) {
            client2GotOpen = true;
            client2AppName = std::make_unique<std::string>(msg.szApplicationName);
        }
    );

    // Register default handlers
    [[maybe_unused]] auto default1HandlerId = handler1.registerDefaultHandler([](const SIMCONNECT_RECV&){}); // NOLINT(misc-include-cleaner)
    [[maybe_unused]] auto default2HandlerId = handler2.registerDefaultHandler([](const SIMCONNECT_RECV&){}); // NOLINT(misc-include-cleaner)

    // Open both connections
    ASSERT_TRUE(connection1.open()) << "Client 1 should connect successfully";
    ASSERT_TRUE(connection2.open()) << "Client 2 should connect successfully";

    // Verify both connections are open
    EXPECT_TRUE(connection1.isOpen()) << "Client 1 connection should be open";
    EXPECT_TRUE(connection2.isOpen()) << "Client 2 connection should be open";

    // Process messages for both handlers independently
    static constexpr auto twoSeconds = 2s;
    handler1.handleUntilOrTimeout([&client1GotOpen]() { return client1GotOpen.load(); }, twoSeconds);
    handler2.handleUntilOrTimeout([&client2GotOpen]() { return client2GotOpen.load(); }, twoSeconds);

    // Verify both clients received open messages
    EXPECT_TRUE(client1GotOpen) << "Client 1 should receive OPEN message";
    EXPECT_TRUE(client2GotOpen) << "Client 2 should receive OPEN message";

    // Verify both got application names
    ASSERT_NE(client1AppName, nullptr) << "Client 1 should receive application name";
    ASSERT_NE(client2AppName, nullptr) << "Client 2 should receive application name";

    // Both should connect to the same simulator
    EXPECT_EQ(*client1AppName, *client2AppName) 
        << "Both clients should connect to the same simulator";

    // Close first connection
    connection1.close();
    EXPECT_FALSE(connection1.isOpen()) << "Client 1 should be closed";
    EXPECT_TRUE(connection2.isOpen()) << "Client 2 should still be open after closing Client 1";

    // Verify Client 2 can still dispatch messages
    constexpr int maxDispatchAttempts = 5;
    constexpr auto dispatchTimeout = 50ms;
    bool client2StillResponsive = false;
    for (int i = 0; i < maxDispatchAttempts; ++i) {
        handler2.handleFor(dispatchTimeout);
        if (connection2.isOpen()) {
            client2StillResponsive = true;
            break;
        }
    }
    EXPECT_TRUE(client2StillResponsive) << "Client 2 should remain responsive after Client 1 closes";

    // Close second connection
    connection2.close();
    EXPECT_FALSE(connection2.isOpen()) << "Client 2 should be closed";

    // No manual cleanup needed - smart pointers handle it automatically
}

TEST(TestMultiClient, IndependentSystemStateRequests) {
    // Create two separate connections
    WindowsEventConnection<> connection1("SystemStateClient1");
    WindowsEventConnection<> connection2("SystemStateClient2");

    WindowsEventHandler<> handler1(connection1);
    WindowsEventHandler<> handler2(connection2);

    // Track messages
    std::atomic<int> client1MessageCount{0};
    std::atomic<int> client2MessageCount{0};
    std::atomic<bool> client1GotAircraftLoaded{false};
    std::atomic<bool> client2GotAircraftLoaded{false};

    // Register default handlers that count messages
    [[maybe_unused]] auto default1HandlerId = handler1.registerDefaultHandler(
        [&](const SIMCONNECT_RECV&) {
            client1MessageCount++;
        }
    );

    [[maybe_unused]] auto default2HandlerId = handler2.registerDefaultHandler(
        [&](const SIMCONNECT_RECV&) {
            client2MessageCount++;
        }
    );

    // Open connections
    ASSERT_TRUE(connection1.open());
    ASSERT_TRUE(connection2.open());

    // Create system state handlers
    SystemStateHandler<WindowsEventHandler<>> stateHandler1(handler1);
    SystemStateHandler<WindowsEventHandler<>> stateHandler2(handler2);

    stateHandler1.enable(handler1);
    stateHandler2.enable(handler2);

    // Request system states from each client
    stateHandler1.requestSystemState("AircraftLoaded", [&](std::string) { // NOLINT(performance-unnecessary-value-param)
        client1GotAircraftLoaded = true;
    });

    stateHandler2.requestSystemState("AircraftLoaded", [&](std::string) { // NOLINT(performance-unnecessary-value-param)
        client2GotAircraftLoaded = true;
    });

    // Process messages
    static constexpr auto threeSeconds = 3s;
    handler1.handleUntilOrTimeout([&client1GotAircraftLoaded]() { return client1GotAircraftLoaded.load(); }, threeSeconds);
    handler2.handleUntilOrTimeout([&client2GotAircraftLoaded]() { return client2GotAircraftLoaded.load(); }, threeSeconds);

    // Both clients should receive their responses
    EXPECT_TRUE(client1GotAircraftLoaded) << "Client 1 should receive AircraftLoaded response";
    EXPECT_TRUE(client2GotAircraftLoaded) << "Client 2 should receive AircraftLoaded response";

    // Both clients should have received messages
    EXPECT_GT(client1MessageCount.load(), 0) << "Client 1 should have received messages";
    EXPECT_GT(client2MessageCount.load(), 0) << "Client 2 should have received messages";

    // Clean up
    connection1.close();
    connection2.close();
}

TEST(TestMultiClient, SimultaneousReconnection) {
    // Test that one client can reconnect while another remains connected
    WindowsEventConnection<> connection1("ReconnectClient1");
    WindowsEventConnection<> connection2("StableClient2");

    WindowsEventHandler<> handler1(connection1);
    WindowsEventHandler<> handler2(connection2);

    std::atomic<int> client1OpenCount{0};
    std::atomic<int> client2OpenCount{0};

    [[maybe_unused]] auto open1HandlerId = handler1.registerHandler<SIMCONNECT_RECV_OPEN>(SIMCONNECT_RECV_ID_OPEN, [&](const SIMCONNECT_RECV_OPEN&) { // NOLINT(misc-include-cleaner)
            client1OpenCount++;
        }
    );

    [[maybe_unused]] auto open2HandlerId = handler2.registerHandler<SIMCONNECT_RECV_OPEN>(SIMCONNECT_RECV_ID_OPEN, [&](const SIMCONNECT_RECV_OPEN&) { // NOLINT(misc-include-cleaner)
            client2OpenCount++;
        }
    );

    [[maybe_unused]] auto default1HandlerId = handler1.registerDefaultHandler([](const SIMCONNECT_RECV&){}); // NOLINT(misc-include-cleaner)
    [[maybe_unused]] auto default2HandlerId = handler2.registerDefaultHandler([](const SIMCONNECT_RECV&){}); // NOLINT(misc-include-cleaner)

    // Initial connection for both
    ASSERT_TRUE(connection1.open());
    ASSERT_TRUE(connection2.open());

    // Wait for initial open messages
    static constexpr auto twoSeconds = 2s;
    handler1.handleUntilOrTimeout([&client1OpenCount]() { return client1OpenCount.load() > 0; }, twoSeconds);
    handler2.handleUntilOrTimeout([&client2OpenCount]() { return client2OpenCount.load() > 0; }, twoSeconds);

    EXPECT_EQ(client1OpenCount.load(), 1) << "Client 1 should receive first OPEN";
    EXPECT_EQ(client2OpenCount.load(), 1) << "Client 2 should receive first OPEN";

    // Close and reconnect client 1
    connection1.close();
    EXPECT_FALSE(connection1.isOpen());
    EXPECT_TRUE(connection2.isOpen()) << "Client 2 should remain open";

    // Reconnect client 1
    ASSERT_TRUE(connection1.open());

    // Wait for second open message on client 1
    handler1.handleUntilOrTimeout([&client1OpenCount]() { return client1OpenCount.load() >= 2; }, twoSeconds);

    // Client 1 should have received 2 open messages (initial + reconnect)
    EXPECT_EQ(client1OpenCount.load(), 2) << "Client 1 should receive OPEN after reconnection";
    
    // Client 2 should still only have 1 open message
    EXPECT_EQ(client2OpenCount.load(), 1) << "Client 2 OPEN count should not change";
    EXPECT_TRUE(connection2.isOpen()) << "Client 2 should still be connected";

    // Clean up
    connection1.close();
    connection2.close();
}

//NOLINTEND(readability-function-cognitive-complexity)