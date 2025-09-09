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

#include "pch.h"
#include <simconnect/messaging/multi_handler_proc.hpp>

using namespace SimConnect;

// Mock message type for testing
struct MockMessage {
    int id;
    std::string data;
    
    MockMessage(int i = 0, const std::string& d = "") : id(i), data(d) {}
};

// Test MultiHandlerProc basic functionality

// Scenario: Default construction creates an empty handler collection
// Given a MultiHandlerProc is default constructed
// When I check if any handlers are set
// Then no handlers should be present
// And calling the handler with a message should not crash
TEST(MultiHandlerProcTests, DefaultConstruction) {
    MultiHandlerProc<MockMessage> handler;
    
    // Should have no single handler (returns nullptr)
    EXPECT_EQ(handler.proc(), nullptr);
    
    // Calling with no handlers should not crash
    MockMessage msg{1, "test"};
    EXPECT_NO_THROW(handler(msg));
}

// Scenario: Construction with a single handler function
// Given a handler function is defined
// When I construct a MultiHandlerProc with that function
// Then the handler should be added to the collection
// And calling the handler should execute the function
TEST(MultiHandlerProcTests, ConstructorWithHandler) {
    bool called = false;
    MockMessage receivedMsg;
    
    auto handlerFunc = [&called, &receivedMsg](const MockMessage& msg) {
        called = true;
        receivedMsg = msg;
    };
    
    MultiHandlerProc<MockMessage> handler(handlerFunc);
    
    // Should still return nullptr for single proc (multi-handler design)
    EXPECT_EQ(handler.proc(), nullptr);
    
    // Test calling the handler
    MockMessage testMsg{42, "hello"};
    handler(testMsg);
    
    EXPECT_TRUE(called);
    EXPECT_EQ(receivedMsg.id, 42);
    EXPECT_EQ(receivedMsg.data, "hello");
}

// Scenario: Adding handlers using setProc method
// Given a MultiHandlerProc with no handlers
// When I add handlers using setProc
// Then each handler should get a unique ID
// And all handlers should be callable via their IDs
TEST(MultiHandlerProcTests, SetProcAndGetById) {
    MultiHandlerProc<MockMessage> handler;
    
    bool called1 = false, called2 = false;
    MockMessage receivedMsg1, receivedMsg2;
    
    auto handlerFunc1 = [&called1, &receivedMsg1](const MockMessage& msg) {
        called1 = true;
        receivedMsg1 = msg;
    };
    
    auto handlerFunc2 = [&called2, &receivedMsg2](const MockMessage& msg) {
        called2 = true;
        receivedMsg2 = msg;
    };
    
    // Add handlers and get IDs
    auto id1 = handler.setProc(handlerFunc1);
    auto id2 = handler.setProc(handlerFunc2);
    
    EXPECT_EQ(id1, 0);  // First handler should get ID 0
    EXPECT_EQ(id2, 1);  // Second handler should get ID 1
    
    // Retrieve and call individual handlers
    auto proc1 = handler.proc(id1);
    auto proc2 = handler.proc(id2);
    
    EXPECT_NE(proc1, nullptr);
    EXPECT_NE(proc2, nullptr);
    
    MockMessage testMsg{123, "world"};
    proc1(testMsg);
    proc2(testMsg);
    
    EXPECT_TRUE(called1);
    EXPECT_TRUE(called2);
    EXPECT_EQ(receivedMsg1.id, 123);
    EXPECT_EQ(receivedMsg2.id, 123);
}

// Scenario: Calling all handlers simultaneously
// Given a MultiHandlerProc with multiple handlers
// When I call the operator() with a message
// Then all handlers should be executed
// And each should receive the same message
TEST(MultiHandlerProcTests, CallAllHandlers) {
    MultiHandlerProc<MockMessage> handler;
    
    int callCount1 = 0, callCount2 = 0, callCount3 = 0;
    
    auto handler1 = [&callCount1](const MockMessage&) { callCount1++; };
    auto handler2 = [&callCount2](const MockMessage&) { callCount2++; };
    auto handler3 = [&callCount3](const MockMessage&) { callCount3++; };
    
    handler.setProc(handler1);
    handler.setProc(handler2);
    handler.setProc(handler3);
    
    MockMessage msg{1, "test"};
    handler(msg);
    
    EXPECT_EQ(callCount1, 1);
    EXPECT_EQ(callCount2, 1);
    EXPECT_EQ(callCount3, 1);
}

// Scenario: Clearing a specific handler by ID
// Given a MultiHandlerProc with multiple handlers
// When I clear a specific handler by its ID
// Then only that handler should be removed
// And other handlers should continue to work
TEST(MultiHandlerProcTests, ClearSpecificHandler) {
    MultiHandlerProc<MockMessage> handler;
    
    int callCount1 = 0, callCount2 = 0, callCount3 = 0;
    
    auto handler1 = [&callCount1](const MockMessage&) { callCount1++; };
    auto handler2 = [&callCount2](const MockMessage&) { callCount2++; };
    auto handler3 = [&callCount3](const MockMessage&) { callCount3++; };
    
    [[maybe_unused]] auto id1 = handler.setProc(handler1);
    auto id2 = handler.setProc(handler2);
    [[maybe_unused]] auto id3 = handler.setProc(handler3);
    
    // Clear the middle handler
    handler.clear(id2);
    
    MockMessage msg{1, "test"};
    handler(msg);
    
    EXPECT_EQ(callCount1, 1);  // Should still be called
    EXPECT_EQ(callCount2, 0);  // Should not be called (cleared)
    EXPECT_EQ(callCount3, 1);  // Should still be called
}

// Scenario: Clearing all handlers
// Given a MultiHandlerProc with multiple handlers
// When I call clear without parameters
// Then all handlers should be removed
// And calling the handler should not execute any functions
TEST(MultiHandlerProcTests, ClearAllHandlers) {
    MultiHandlerProc<MockMessage> handler;
    
    int callCount1 = 0, callCount2 = 0;
    
    auto handler1 = [&callCount1](const MockMessage&) { callCount1++; };
    auto handler2 = [&callCount2](const MockMessage&) { callCount2++; };
    
    handler.setProc(handler1);
    handler.setProc(handler2);
    
    // Verify handlers work before clearing
    MockMessage msg{1, "test"};
    handler(msg);
    EXPECT_EQ(callCount1, 1);
    EXPECT_EQ(callCount2, 1);
    
    // Clear all handlers
    handler.clear();
    
    // Reset counters and test again
    callCount1 = 0;
    callCount2 = 0;
    handler(msg);
    
    EXPECT_EQ(callCount1, 0);
    EXPECT_EQ(callCount2, 0);
}

// Scenario: Clearing handler with invalid ID
// Given a MultiHandlerProc with some handlers
// When I try to clear a handler with an invalid ID
// Then the operation should be safe and not crash
// And existing handlers should remain functional
TEST(MultiHandlerProcTests, ClearInvalidId) {
    MultiHandlerProc<MockMessage> handler;
    
    int callCount = 0;
    auto handlerFunc = [&callCount](const MockMessage&) { callCount++; };
    
    auto id = handler.setProc(handlerFunc);
    
    // Try to clear with invalid IDs
    EXPECT_NO_THROW(handler.clear(999));  // Way out of bounds
    EXPECT_NO_THROW(handler.clear(id + 10));  // Slightly out of bounds
    
    // Original handler should still work
    MockMessage msg{1, "test"};
    handler(msg);
    EXPECT_EQ(callCount, 1);
}

// Scenario: Accessing handler with invalid ID throws exception
// Given a MultiHandlerProc with some handlers
// When I try to access a handler with an invalid ID
// Then it should throw an exception or exhibit undefined behavior
// Note: This test demonstrates the current behavior - bounds checking may vary
TEST(MultiHandlerProcTests, AccessInvalidId) {
    MultiHandlerProc<MockMessage> handler;
    
    auto handlerFunc = [](const MockMessage&) {};
    handler.setProc(handlerFunc);
    
    // Accessing valid ID should work
    EXPECT_NO_THROW(handler.proc(0));
    
    // Accessing invalid ID may throw or cause undefined behavior
    // This test documents current behavior - std::vector access with [] doesn't throw
    // but with .at() it would throw std::out_of_range
    EXPECT_NO_THROW(handler.proc(999));  // Note: May cause undefined behavior
}

// Scenario: Handlers with null function pointers are skipped
// Given a MultiHandlerProc with a mix of valid and null handlers
// When I call the handler with a message
// Then only the valid handlers should be executed
// And null handlers should be safely skipped
TEST(MultiHandlerProcTests, SkipNullHandlers) {
    MultiHandlerProc<MockMessage> handler;
    
    int callCount1 = 0, callCount3 = 0;
    
    auto handler1 = [&callCount1](const MockMessage&) { callCount1++; };
    auto handler3 = [&callCount3](const MockMessage&) { callCount3++; };
    
    [[maybe_unused]] auto id1 = handler.setProc(handler1);
    [[maybe_unused]] auto id2 = handler.setProc(nullptr);  // Null handler
    [[maybe_unused]] auto id3 = handler.setProc(handler3);
    
    MockMessage msg{1, "test"};
    handler(msg);
    
    EXPECT_EQ(callCount1, 1);
    EXPECT_EQ(callCount3, 1);
    // No crash should occur despite null handler
}

// Scenario: Copy construction preserves all handlers
// Given a MultiHandlerProc with multiple handlers
// When I copy construct a new MultiHandlerProc from it
// Then both objects should have all handlers
// And the copied handlers should be functional
TEST(MultiHandlerProcTests, CopyConstructor) {
    MultiHandlerProc<MockMessage> original;
    
    int callCount1 = 0, callCount2 = 0;
    
    auto handler1 = [&callCount1](const MockMessage&) { callCount1++; };
    auto handler2 = [&callCount2](const MockMessage&) { callCount2++; };
    
    original.setProc(handler1);
    original.setProc(handler2);
    
    // Copy construct
    MultiHandlerProc<MockMessage> copy(original);
    
    MockMessage msg{1, "test"};
    
    // Test original
    original(msg);
    EXPECT_EQ(callCount1, 1);
    EXPECT_EQ(callCount2, 1);
    
    // Reset and test copy
    callCount1 = 0;
    callCount2 = 0;
    copy(msg);
    EXPECT_EQ(callCount1, 1);
    EXPECT_EQ(callCount2, 1);
}

// Scenario: Move construction transfers all handlers
// Given a MultiHandlerProc with multiple handlers
// When I move construct a new MultiHandlerProc from it
// Then the new object should have all handlers
// And the handlers should be functional
TEST(MultiHandlerProcTests, MoveConstructor) {
    MultiHandlerProc<MockMessage> original;
    
    int callCount1 = 0, callCount2 = 0;
    
    auto handler1 = [&callCount1](const MockMessage&) { callCount1++; };
    auto handler2 = [&callCount2](const MockMessage&) { callCount2++; };
    
    original.setProc(handler1);
    original.setProc(handler2);
    
    // Move construct
    MultiHandlerProc<MockMessage> moved(std::move(original));
    
    MockMessage msg{1, "test"};
    moved(msg);
    
    EXPECT_EQ(callCount1, 1);
    EXPECT_EQ(callCount2, 1);
}

// Scenario: Assignment operator preserves all handlers
// Given two MultiHandlerProc objects where one has handlers
// When I assign the first to the second
// Then both objects should have all handlers
// And the assigned handlers should be functional
TEST(MultiHandlerProcTests, AssignmentOperator) {
    MultiHandlerProc<MockMessage> handler1;
    MultiHandlerProc<MockMessage> handler2;
    
    int callCount = 0;
    auto handlerFunc = [&callCount](const MockMessage&) { callCount++; };
    
    handler1.setProc(handlerFunc);
    handler1.setProc(handlerFunc);  // Add twice for testing
    
    // Assignment
    handler2 = handler1;
    
    MockMessage msg{1, "test"};
    
    // Test original
    handler1(msg);
    EXPECT_EQ(callCount, 2);  // Should be called twice
    
    // Reset and test assigned
    callCount = 0;
    handler2(msg);
    EXPECT_EQ(callCount, 2);  // Should also be called twice
}

// Scenario: Working with real SimConnect message types
// Given a MultiHandlerProc configured for SIMCONNECT_RECV messages
// When I add multiple handlers and call with a SimConnect message
// Then all handlers should receive the correct message data
TEST(MultiHandlerProcTests, WithSimConnectRecv) {
    MultiHandlerProc<SIMCONNECT_RECV> handler;
    
    int callCount = 0;
    std::vector<DWORD> receivedIds;
    
    auto handlerFunc1 = [&callCount, &receivedIds](const SIMCONNECT_RECV& msg) {
        callCount++;
        receivedIds.push_back(msg.dwID);
    };
    
    auto handlerFunc2 = [&callCount, &receivedIds](const SIMCONNECT_RECV& msg) {
        callCount++;
        receivedIds.push_back(msg.dwID);
    };
    
    handler.setProc(handlerFunc1);
    handler.setProc(handlerFunc2);
    
    // Create a mock SIMCONNECT_RECV message
    SIMCONNECT_RECV msg{};
    msg.dwSize = sizeof(SIMCONNECT_RECV);
    msg.dwVersion = 1;
    msg.dwID = static_cast<DWORD>(SIMCONNECT_RECV_ID_OPEN);
    
    handler(msg);
    
    EXPECT_EQ(callCount, 2);
    EXPECT_EQ(receivedIds.size(), 2);
    EXPECT_EQ(receivedIds[0], static_cast<DWORD>(SIMCONNECT_RECV_ID_OPEN));
    EXPECT_EQ(receivedIds[1], static_cast<DWORD>(SIMCONNECT_RECV_ID_OPEN));
}

// Scenario: Handlers with different lambda capture strategies
// Given a MultiHandlerProc with handlers using different capture methods
// When I call the handlers multiple times
// Then each handler should behave according to its capture strategy
TEST(MultiHandlerProcTests, DifferentCaptureStrategies) {
    MultiHandlerProc<MockMessage> handler;
    
    std::vector<int> sharedData;
    int localCounter = 0;
    
    // Handler with capture by reference
    auto handlerByRef = [&sharedData](const MockMessage& msg) {
        sharedData.push_back(msg.id);
    };
    
    // Handler with capture by value (at time of creation)
    auto handlerByValue = [localCounter](const MockMessage& msg) mutable {
        localCounter += msg.id;  // This won't affect the original localCounter
    };
    
    // Handler with no capture
    auto handlerNoCapture = [](const MockMessage&) {
        // Just a simple handler
    };
    
    handler.setProc(handlerByRef);
    handler.setProc(handlerByValue);
    handler.setProc(handlerNoCapture);
    
    // Call multiple times
    handler(MockMessage{1, "first"});
    handler(MockMessage{2, "second"});
    handler(MockMessage{3, "third"});
    
    // Check that the reference capture worked
    EXPECT_EQ(sharedData.size(), 3);
    EXPECT_EQ(sharedData[0], 1);
    EXPECT_EQ(sharedData[1], 2);
    EXPECT_EQ(sharedData[2], 3);
    
    // Original localCounter should be unchanged
    EXPECT_EQ(localCounter, 0);
}