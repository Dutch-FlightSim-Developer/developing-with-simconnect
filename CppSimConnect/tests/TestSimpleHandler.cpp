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

#include <tuple>
#include <simconnect/simple_handler.hpp>
#include <simconnect/messaging/simple_handler_proc.hpp>

using namespace SimConnect;

// Mock connection type for testing
class MockConnection {
public:
    using mutex_type = NoMutex;
	using guard_type = NoGuard;
private:
    std::vector<std::tuple<SIMCONNECT_RECV, DWORD>> messages_;
    size_t messageIndex_{0};
    bool isOpen_{true};

public:
    MockConnection() = default;
    
    void addMessage(const SIMCONNECT_RECV& msg, DWORD size) {
        messages_.emplace_back(msg, size);
    }
    
    void addMessage(SIMCONNECT_RECV_ID id, DWORD size = sizeof(SIMCONNECT_RECV)) {
        SIMCONNECT_RECV msg{};
        msg.dwID = static_cast<DWORD>(id);
        msg.dwSize = sizeof(SIMCONNECT_RECV);
        msg.dwVersion = 1;
        addMessage(msg, size);
    }
    
    bool getNextDispatch(SIMCONNECT_RECV*& msg, DWORD& size) {
        if (messageIndex_ >= messages_.size()) {
            return false;
        }

        msg = &std::get<0>(messages_[messageIndex_]);
        size = std::get<1>(messages_[messageIndex_]);
		messageIndex_++;

        return true;
    }
    
    bool callDispatch(std::function<void(const SIMCONNECT_RECV*, DWORD)> dispatchFunc) {
        SIMCONNECT_RECV* msg = nullptr;
        DWORD size = 0;
        
        // Process all available messages (ignoring duration for simplicity in mock)
        if (isOpen() && getNextDispatch(msg, size)) {
            if (msg != nullptr) {
                // Call the handler's dispatch method with the message
                dispatchFunc(msg, size);

                return true;
            }
        }
        return false;
    }
    
    bool isOpen() const { return isOpen_; }
    void close() { isOpen_ = false; }
    void reset() { 
        messageIndex_ = 0; 
        messages_.clear();
        isOpen_ = true;
    }
    
    size_t messageCount() const { return messages_.size(); }
    size_t processedCount() const { return messageIndex_; }
};

// Mock logger for testing
static std::vector<std::string> logs;

class MockLogger {
public:
    MockLogger() = default;
    MockLogger(const std::string&) {}  // Constructor compatible with logger expectations
    
    void warn(const std::string& msg) {
        logs.push_back("WARN: " + msg);
    }
    
    template<typename... Args>
    void warn(const std::string& format, Args&&... args) {
        logs.push_back("WARN: " + std::vformat(format, std::make_format_args(args...)));
    }
};

// Test SimpleHandler basic functionality

// Scenario: Construction with a connection
// Given a mock connection
// When I construct a SimpleHandler with that connection
// Then the handler should be properly initialized
// And should be non-copyable and non-movable
TEST(SimpleHandlerTests, Construction) {
    MockConnection connection;
    
    // Should construct successfully
    EXPECT_NO_THROW(SimpleHandler<MockConnection> handler(connection));
    
    SimpleHandler<MockConnection> handler(connection);
    
    // Should be non-copyable and non-movable
    static_assert(!std::is_copy_constructible_v<SimpleHandler<MockConnection>>);
    static_assert(!std::is_move_constructible_v<SimpleHandler<MockConnection>>);
    static_assert(!std::is_copy_assignable_v<SimpleHandler<MockConnection>>);
    static_assert(!std::is_move_assignable_v<SimpleHandler<MockConnection>>);
}

// Scenario: Dispatching with no messages
// Given a SimpleHandler with an empty connection
// When I call dispatch
// Then it should complete without errors
// And no handlers should be called
TEST(SimpleHandlerTests, DispatchEmptyConnection) {
    MockConnection connection;
    SimpleHandler<MockConnection> handler(connection);
    
    // Should not throw when no messages are available
    EXPECT_NO_THROW(handler.dispatch());
    
    // Should complete immediately
    EXPECT_NO_THROW(handler.dispatch(std::chrono::milliseconds(100)));
}

// Scenario: Dispatching with default handler
// Given a SimpleHandler with a default message handler
// When I add messages and dispatch
// Then the default handler should receive all messages
TEST(SimpleHandlerTests, DispatchWithDefaultHandler) {
    MockConnection connection;
    SimpleHandler<MockConnection> handler(connection);
    
    std::vector<SIMCONNECT_RECV_ID> receivedIds;
    
    // Set default handler
    handler.setDefaultHandler([&receivedIds](const SIMCONNECT_RECV& msg) {
        receivedIds.push_back(static_cast<SIMCONNECT_RECV_ID>(msg.dwID));
    });
    
    // Add test messages
    connection.addMessage(SIMCONNECT_RECV_ID_OPEN);
    connection.addMessage(SIMCONNECT_RECV_ID_QUIT);
    connection.addMessage(SIMCONNECT_RECV_ID_EXCEPTION);
    
    // Dispatch messages
    handler.dispatch();
    
    EXPECT_EQ(receivedIds.size(), 3);
    EXPECT_EQ(receivedIds[0], SIMCONNECT_RECV_ID_OPEN);
    EXPECT_EQ(receivedIds[1], SIMCONNECT_RECV_ID_QUIT);
    EXPECT_EQ(receivedIds[2], SIMCONNECT_RECV_ID_EXCEPTION);
}

// Scenario: Dispatching with specific message handlers
// Given a SimpleHandler with handlers for specific message types
// When I add messages and dispatch
// Then only the appropriate handlers should be called
TEST(SimpleHandlerTests, DispatchWithSpecificHandlers) {
    MockConnection connection;
    SimpleHandler<MockConnection> handler(connection);
    
    int openCount = 0;
    int quitCount = 0;
    int defaultCount = 0;
    
    // Register specific handlers
    handler.registerHandlerProc(SIMCONNECT_RECV_ID_OPEN, [&openCount](const SIMCONNECT_RECV&) {
        openCount++;
    });
    
    handler.registerHandlerProc(SIMCONNECT_RECV_ID_QUIT, [&quitCount](const SIMCONNECT_RECV&) {
        quitCount++;
    });
    
    // Set default handler for unhandled messages
    handler.setDefaultHandler([&defaultCount](const SIMCONNECT_RECV&) {
        defaultCount++;
    });
    
    // Add test messages
    connection.addMessage(SIMCONNECT_RECV_ID_OPEN);
    connection.addMessage(SIMCONNECT_RECV_ID_QUIT);
    connection.addMessage(SIMCONNECT_RECV_ID_EXCEPTION);  // Should go to default
    connection.addMessage(SIMCONNECT_RECV_ID_OPEN);      // Second open
    
    // Dispatch messages
    handler.dispatch();
    
    EXPECT_EQ(openCount, 2);      // Two OPEN messages
    EXPECT_EQ(quitCount, 1);      // One QUIT message  
    EXPECT_EQ(defaultCount, 1);   // One EXCEPTION message (unhandled)
}

// Scenario: Dispatching with typed message handlers
// Given a SimpleHandler with typed message handlers
// When I add messages and dispatch
// Then the handlers should receive correctly typed messages
TEST(SimpleHandlerTests, DispatchWithTypedHandlers) {
    MockConnection connection;
    SimpleHandler<MockConnection> handler(connection);
    
    DWORD receivedOpenId = 0;
    DWORD receivedQuitId = 0;
    
    // Register typed handlers - these will receive SIMCONNECT_RECV references
    // but can be cast to specific types in real usage
    handler.registerHandler<SIMCONNECT_RECV_OPEN>(SIMCONNECT_RECV_ID_OPEN, 
        [&receivedOpenId](const SIMCONNECT_RECV_OPEN& msg) {
            receivedOpenId = msg.dwID;
        });
    
    handler.registerHandler<SIMCONNECT_RECV_QUIT>(SIMCONNECT_RECV_ID_QUIT,
        [&receivedQuitId](const SIMCONNECT_RECV_QUIT& msg) {
            receivedQuitId = msg.dwID;
        });
    
    // Add test messages
    connection.addMessage(SIMCONNECT_RECV_ID_OPEN);
    connection.addMessage(SIMCONNECT_RECV_ID_QUIT);
    
    // Dispatch messages
    handler.dispatch();
    
    EXPECT_EQ(receivedOpenId, static_cast<DWORD>(SIMCONNECT_RECV_ID_OPEN));
    EXPECT_EQ(receivedQuitId, static_cast<DWORD>(SIMCONNECT_RECV_ID_QUIT));
}

// Scenario: Auto-closing behavior on QUIT message
// Given a SimpleHandler with auto-closing enabled
// When I dispatch a QUIT message
// Then the connection should be automatically closed
TEST(SimpleHandlerTests, AutoClosingOnQuit) {
    MockConnection connection;
    SimpleHandler<MockConnection> handler(connection);
    
    // Enable auto-closing
    handler.autoClosing(true);
    EXPECT_TRUE(handler.isAutoClosing());
    
    // Add QUIT message
    connection.addMessage(SIMCONNECT_RECV_ID_QUIT);
    
    // Connection should be open initially
    EXPECT_TRUE(connection.isOpen());
    
    // Dispatch the QUIT message
    handler.dispatch();
    
    // Connection should be closed after processing QUIT
    EXPECT_FALSE(connection.isOpen());
}

// Scenario: No auto-closing when disabled
// Given a SimpleHandler with auto-closing disabled
// When I dispatch a QUIT message
// Then the connection should remain open
TEST(SimpleHandlerTests, NoAutoClosingWhenDisabled) {
    MockConnection connection;
    SimpleHandler<MockConnection> handler(connection);
    
    // Auto-closing should be disabled by default
    EXPECT_FALSE(handler.isAutoClosing());
    
    // Add QUIT message
    connection.addMessage(SIMCONNECT_RECV_ID_QUIT);
    
    // Dispatch the QUIT message
    handler.dispatch();
    
    // Connection should still be open
    EXPECT_TRUE(connection.isOpen());
    
    // Test explicitly disabling auto-closing
    handler.autoClosing(false);
    EXPECT_FALSE(handler.isAutoClosing());
}

// Scenario: Handling messages when connection closes during dispatch
// Given a SimpleHandler dispatching multiple messages
// When the connection closes during processing
// Then dispatch should stop processing remaining messages
TEST(SimpleHandlerTests, DispatchStopsWhenConnectionCloses) {
    MockConnection connection;
    SimpleHandler<MockConnection> handler(connection);
    
    int processedCount = 0;
    
    handler.setDefaultHandler([&processedCount, &connection](const SIMCONNECT_RECV& msg) {
        processedCount++;
        if (msg.dwID == static_cast<DWORD>(SIMCONNECT_RECV_ID_QUIT)) {
            connection.close();  // Simulate connection closing
        }
    });
    
    // Add multiple messages
    connection.addMessage(SIMCONNECT_RECV_ID_OPEN);
    connection.addMessage(SIMCONNECT_RECV_ID_QUIT);      // This will close the connection
    connection.addMessage(SIMCONNECT_RECV_ID_EXCEPTION); // This should not be processed
    
    // Dispatch messages
    handler.dispatch();
    
    // Should have processed only the first two messages
    EXPECT_EQ(processedCount, 2);
    EXPECT_EQ(connection.processedCount(), 2);
}

// Scenario: Connection returns the handler reference
// Given a SimpleHandler
// When I call the connection() method
// Then it should return a reference to the connection
TEST(SimpleHandlerTests, ConnectionAccess) {
    MockConnection connection;
    SimpleHandler<MockConnection> handler(connection);
    
    // Should return reference to the same connection
    EXPECT_EQ(&handler.connection(), &connection);
    
    // Should be able to use the connection through the handler
    handler.connection().addMessage(SIMCONNECT_RECV_ID_OPEN);
    EXPECT_EQ(connection.messageCount(), 1);
}

// Scenario: Getting specific and default handlers
// Given a SimpleHandler with registered handlers
// When I query for handlers
// Then I should get the correct handler references
TEST(SimpleHandlerTests, HandlerRetrieval) {
    MockConnection connection;
    SimpleHandler<MockConnection> handler(connection);
    
    bool openHandlerCalled = false;
    bool defaultHandlerCalled = false;
    
    auto openHandler = [&openHandlerCalled](const SIMCONNECT_RECV&) {
        openHandlerCalled = true;
    };
    
    auto defaultHandler = [&defaultHandlerCalled](const SIMCONNECT_RECV&) {
        defaultHandlerCalled = true;
    };
    
    // Register handlers
    handler.registerHandlerProc(SIMCONNECT_RECV_ID_OPEN, openHandler);
    handler.setDefaultHandler(defaultHandler);
    
    // Get handler references (these variables document the interface)
    [[maybe_unused]] const auto& retrievedOpenHandler = handler.getHandler(SIMCONNECT_RECV_ID_OPEN);
    [[maybe_unused]] const auto& retrievedDefaultHandler = handler.defaultHandler();
    
    // Test that retrieved handlers work
    connection.addMessage(SIMCONNECT_RECV_ID_OPEN);
    connection.addMessage(SIMCONNECT_RECV_ID_EXCEPTION); // Will use default
    
    handler.dispatch();
    
    EXPECT_TRUE(openHandlerCalled);
    EXPECT_TRUE(defaultHandlerCalled);
}

// Scenario: Handling malformed messages
// Given a SimpleHandler with a message that has incorrect size
// When I dispatch the message
// Then the handler should log a warning but continue processing
TEST(SimpleHandlerTests, MalformedMessageHandling) {
    MockConnection connection;
    SimpleHandler<MockConnection, SimpleHandlerProc<SIMCONNECT_RECV>, MockLogger> handler(connection);
    
    // Add a message with incorrect size information
    SIMCONNECT_RECV msg{};
    msg.dwID = static_cast<DWORD>(SIMCONNECT_RECV_ID_OPEN);
    msg.dwSize = sizeof(SIMCONNECT_RECV) + 100;  // Claim larger size than actual
    msg.dwVersion = 1;

    connection.addMessage(msg, sizeof(msg));

    bool handlerCalled = false;
    handler.setDefaultHandler([&handlerCalled](const SIMCONNECT_RECV&) {
        handlerCalled = true;
    });
	logs.clear();  // Clear previous logs
    
    // Should not throw despite size mismatch
    EXPECT_NO_THROW(handler.dispatch());

    EXPECT_EQ(logs.size(), 1); // Should log a warning about size mismatch
    EXPECT_FALSE(handlerCalled);
}

// Scenario: Dispatch with duration parameter
// Given a SimpleHandler
// When I call dispatch with a duration parameter
// Then it should pass the duration to the underlying dispatch mechanism
// Note: Since our mock doesn't implement timing, this mainly tests the interface
TEST(SimpleHandlerTests, DispatchWithDuration) {
    MockConnection connection;
    SimpleHandler<MockConnection> handler(connection);
    
    // Should not throw with various duration values
    EXPECT_NO_THROW(handler.dispatch(std::chrono::milliseconds(0)));
    EXPECT_NO_THROW(handler.dispatch(std::chrono::milliseconds(100)));
    EXPECT_NO_THROW(handler.dispatch(std::chrono::milliseconds(1000)));
    
    // Test with messages
    connection.addMessage(SIMCONNECT_RECV_ID_OPEN);
    
    bool handlerCalled = false;
    handler.setDefaultHandler([&handlerCalled](const SIMCONNECT_RECV&) {
        handlerCalled = true;
    });
    
    handler.dispatch(std::chrono::milliseconds(50));
    EXPECT_TRUE(handlerCalled);
}