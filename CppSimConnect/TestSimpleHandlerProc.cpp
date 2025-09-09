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
#include <simconnect/messaging/simple_handler_proc.hpp>

using namespace SimConnect;

// Mock message type for testing
struct MockMessage {
    int id;
    std::string data;
    
    MockMessage(int i = 0, const std::string& d = "") : id(i), data(d) {}
};

// Test SimpleHandlerProc basic functionality

// Scenario: Default construction creates an empty handler
// Given a SimpleHandlerProc is default constructed
// When I check if a handler is set
// Then no handler should be present
// And calling the handler with a message should not crash
TEST(SimpleHandlerProcTests, DefaultConstruction) {
    SimpleHandlerProc<MockMessage> handler;
    
    // Should have no handler set initially
    EXPECT_EQ(handler.proc(), nullptr);
    
    // Calling with no handler should not crash
    MockMessage msg{1, "test"};
    EXPECT_NO_THROW(handler(msg));
}

// Scenario: Construction with a handler function
// Given a handler function is defined
// When I construct a SimpleHandlerProc with that function
// Then the handler should be set
// And calling the handler should execute the function
TEST(SimpleHandlerProcTests, ConstructorWithHandler) {
    bool called = false;
    MockMessage receivedMsg;
    
    auto handlerFunc = [&called, &receivedMsg](const MockMessage& msg) {
        called = true;
        receivedMsg = msg;
    };
    
    SimpleHandlerProc<MockMessage> handler(handlerFunc);
    
    // Should have handler set
    EXPECT_NE(handler.proc(), nullptr);
    
    // Test calling the handler
    MockMessage testMsg{42, "hello"};
    handler(testMsg);
    
    EXPECT_TRUE(called);
    EXPECT_EQ(receivedMsg.id, 42);
    EXPECT_EQ(receivedMsg.data, "hello");
}

// Scenario: Setting a handler using setProc method
// Given a SimpleHandlerProc with no handler
// When I set a handler using setProc
// Then the method should return monostate
// And the handler should be available
// And calling the handler should execute the function
TEST(SimpleHandlerProcTests, SetProcAndCall) {
    SimpleHandlerProc<MockMessage> handler;
    
    bool called = false;
    MockMessage receivedMsg;
    
    auto handlerFunc = [&called, &receivedMsg](const MockMessage& msg) {
        called = true;
        receivedMsg = msg;
    };
    
    // Set the handler
    auto result = handler.setProc(handlerFunc);
    EXPECT_EQ(result, std::monostate{});  // Should return monostate
    
    // Verify handler is set
    EXPECT_NE(handler.proc(), nullptr);
    
    // Test calling
    MockMessage testMsg{123, "world"};
    handler(testMsg);
    
    EXPECT_TRUE(called);
    EXPECT_EQ(receivedMsg.id, 123);
    EXPECT_EQ(receivedMsg.data, "world");
}

// Scenario: Retrieving handler using proc with monostate parameter
// Given a SimpleHandlerProc with a handler set
// When I call proc with a monostate parameter
// Then I should get back the handler function
// And the returned function should be callable
TEST(SimpleHandlerProcTests, ProcWithMonostate) {
    SimpleHandlerProc<MockMessage> handler;
    
    bool called = false;
    auto handlerFunc = [&called](const MockMessage&) { called = true; };
    
    handler.setProc(handlerFunc);
    
    // Test proc() with monostate parameter
    auto proc = handler.proc(std::monostate{});
    EXPECT_NE(proc, nullptr);
    
    // Call the retrieved proc
    MockMessage msg{1, "test"};
    proc(msg);
    EXPECT_TRUE(called);
}

// Scenario: Clearing handler using monostate parameter
// Given a SimpleHandlerProc with a handler set
// When I call clear with a monostate parameter
// Then the handler should be removed
TEST(SimpleHandlerProcTests, ClearWithMonostate) {
    SimpleHandlerProc<MockMessage> handler;
    
    auto handlerFunc = [](const MockMessage&) { /* do nothing */ };
    handler.setProc(handlerFunc);
    
    // Verify handler is set
    EXPECT_NE(handler.proc(), nullptr);
    
    // Clear with monostate
    handler.clear(std::monostate{});
    
    // Verify handler is cleared
    EXPECT_EQ(handler.proc(), nullptr);
}

// Scenario: Clearing handler without parameters
// Given a SimpleHandlerProc with a handler set
// When I call clear without parameters
// Then the handler should be removed
TEST(SimpleHandlerProcTests, ClearNoParam) {
    SimpleHandlerProc<MockMessage> handler;
    
    auto handlerFunc = [](const MockMessage&) { /* do nothing */ };
    handler.setProc(handlerFunc);
    
    // Verify handler is set
    EXPECT_NE(handler.proc(), nullptr);
    
    // Clear without parameters
    handler.clear();
    
    // Verify handler is cleared
    EXPECT_EQ(handler.proc(), nullptr);
}

// Scenario: Calling handler when no handler is set
// Given a SimpleHandlerProc with no handler set
// When I call the handler with a message
// Then the call should not crash or throw an exception
TEST(SimpleHandlerProcTests, CallWithNullHandler) {
    SimpleHandlerProc<MockMessage> handler;
    
    // No handler set, calling should not crash
    MockMessage msg{1, "test"};
    EXPECT_NO_THROW(handler(msg));
}

// Scenario: Replacing an existing handler with a new one
// Given a SimpleHandlerProc with an initial handler
// When I set a new handler
// Then only the new handler should be called
// And the old handler should no longer execute
TEST(SimpleHandlerProcTests, ReplaceHandler) {
    SimpleHandlerProc<MockMessage> handler;
    
    int callCount1 = 0;
    int callCount2 = 0;
    
    auto handler1 = [&callCount1](const MockMessage&) { callCount1++; };
    auto handler2 = [&callCount2](const MockMessage&) { callCount2++; };
    
    // Set first handler
    handler.setProc(handler1);
    
    MockMessage msg{1, "test"};
    handler(msg);
    EXPECT_EQ(callCount1, 1);
    EXPECT_EQ(callCount2, 0);
    
    // Replace with second handler
    handler.setProc(handler2);
    
    handler(msg);
    EXPECT_EQ(callCount1, 1);  // Should not increase
    EXPECT_EQ(callCount2, 1);  // Should increase
}

// Scenario: Copy construction preserves handler
// Given a SimpleHandlerProc with a handler set
// When I copy construct a new SimpleHandlerProc from it
// Then both objects should have the handler
// And the copied handler should be functional
TEST(SimpleHandlerProcTests, CopyConstructor) {
    SimpleHandlerProc<MockMessage> original;
    
    bool called = false;
    auto handlerFunc = [&called](const MockMessage&) { called = true; };
    original.setProc(handlerFunc);
    
    // Copy construct
    SimpleHandlerProc<MockMessage> copy(original);
    
    // Both should have the handler
    EXPECT_NE(original.proc(), nullptr);
    EXPECT_NE(copy.proc(), nullptr);
    
    // Test calling on copy
    MockMessage msg{1, "test"};
    copy(msg);
    EXPECT_TRUE(called);
}

// Scenario: Assignment operator preserves handler
// Given two SimpleHandlerProc objects where one has a handler
// When I assign the first to the second
// Then both objects should have the handler
// And the assigned handler should be functional
TEST(SimpleHandlerProcTests, AssignmentOperator) {
    SimpleHandlerProc<MockMessage> handler1;
    SimpleHandlerProc<MockMessage> handler2;
    
    bool called = false;
    auto handlerFunc = [&called](const MockMessage&) { called = true; };
    handler1.setProc(handlerFunc);
    
    // Assignment
    handler2 = handler1;
    
    // Both should have the handler
    EXPECT_NE(handler1.proc(), nullptr);
    EXPECT_NE(handler2.proc(), nullptr);
    
    // Test calling on assigned handler
    MockMessage msg{1, "test"};
    handler2(msg);
    EXPECT_TRUE(called);
}

// Scenario: Move construction transfers handler
// Given a SimpleHandlerProc with a handler set
// When I move construct a new SimpleHandlerProc from it
// Then the new object should have the handler
// And the handler should be functional
TEST(SimpleHandlerProcTests, MoveConstructor) {
    SimpleHandlerProc<MockMessage> original;
    
    bool called = false;
    auto handlerFunc = [&called](const MockMessage&) { called = true; };
    original.setProc(handlerFunc);
    
    // Move construct
    SimpleHandlerProc<MockMessage> moved(std::move(original));
    
    // Moved-to object should have the handler
    EXPECT_NE(moved.proc(), nullptr);
    
    // Test calling on moved object
    MockMessage msg{1, "test"};
    moved(msg);
    EXPECT_TRUE(called);
}

// Scenario: Move assignment transfers handler
// Given two SimpleHandlerProc objects where one has a handler
// When I move assign the first to the second
// Then the target object should have the handler
// And the handler should be functional
TEST(SimpleHandlerProcTests, MoveAssignment) {
    SimpleHandlerProc<MockMessage> handler1;
    SimpleHandlerProc<MockMessage> handler2;
    
    bool called = false;
    auto handlerFunc = [&called](const MockMessage&) { called = true; };
    handler1.setProc(handlerFunc);
    
    // Move assignment
    handler2 = std::move(handler1);
    
    // Moved-to object should have the handler
    EXPECT_NE(handler2.proc(), nullptr);
    
    // Test calling on moved-to object
    MockMessage msg{1, "test"};
    handler2(msg);
    EXPECT_TRUE(called);
}

// Scenario: Working with real SimConnect message types
// Given a SimpleHandlerProc configured for SIMCONNECT_RECV messages
// When I set a handler and call it with a SimConnect message
// Then the handler should receive the correct message data
TEST(SimpleHandlerProcTests, WithSimConnectRecv) {
    SimpleHandlerProc<SIMCONNECT_RECV> handler;
    
    bool called = false;
    DWORD receivedId = 0;
    
    auto handlerFunc = [&called, &receivedId](const SIMCONNECT_RECV& msg) {
        called = true;
        receivedId = msg.dwID;
    };
    
    handler.setProc(handlerFunc);
    
    // Create a mock SIMCONNECT_RECV message
    SIMCONNECT_RECV msg{};
    msg.dwSize = sizeof(SIMCONNECT_RECV);
    msg.dwVersion = 1;  // Use a simple version number
    msg.dwID = static_cast<DWORD>(SIMCONNECT_RECV_ID_OPEN);
    
    handler(msg);
    
    EXPECT_TRUE(called);
    EXPECT_EQ(receivedId, static_cast<DWORD>(SIMCONNECT_RECV_ID_OPEN));
}

// Scenario: Handler with lambda that captures by reference
// Given a SimpleHandlerProc with a lambda that captures variables by reference
// When I call the handler multiple times with different messages
// Then the captured variables should accumulate the message data
TEST(SimpleHandlerProcTests, LambdaCaptureByReference) {
    SimpleHandlerProc<MockMessage> handler;
    
    std::vector<int> receivedIds;
    
    auto handlerFunc = [&receivedIds](const MockMessage& msg) {
        receivedIds.push_back(msg.id);
    };
    
    handler.setProc(handlerFunc);
    
    // Call multiple times
    handler(MockMessage{1, "first"});
    handler(MockMessage{2, "second"});
    handler(MockMessage{3, "third"});
    
    EXPECT_EQ(receivedIds.size(), 3);
    EXPECT_EQ(receivedIds[0], 1);
    EXPECT_EQ(receivedIds[1], 2);
    EXPECT_EQ(receivedIds[2], 3);
}