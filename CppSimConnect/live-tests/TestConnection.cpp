#include "pch.h"

#include <simconnect/windows_event_connection.hpp>
#include <simconnect/windows_event_handler.hpp>
#include <simconnect/requests/system_state_handler.hpp>

#include <atomic>
#include <chrono>
#include <string>
#include <functional>

using namespace SimConnect;

TEST(TestConnection, ReceivesOpenMessage) {
    WindowsEventConnection<> connection;
    WindowsEventHandler<> handler(connection);
    std::atomic<bool> gotOpen{false};

    handler.registerHandler<SIMCONNECT_RECV_OPEN>(SIMCONNECT_RECV_ID_OPEN, [&](const SIMCONNECT_RECV_OPEN&) {
        gotOpen = true;
    });
    handler.setDefaultHandler([](const SIMCONNECT_RECV&){});

    ASSERT_TRUE(connection.open());

    // Wait up to 2 seconds for the open message
    for (int i = 0; i < 20 && !gotOpen; ++i) {
        handler.dispatch(std::chrono::milliseconds(100));
    }
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

    handler.registerHandler<SIMCONNECT_RECV_EXCEPTION>(SIMCONNECT_RECV_ID_EXCEPTION, [&](const SIMCONNECT_RECV_EXCEPTION&) {
        gotException = true;
    });
    handler.setDefaultHandler([](const SIMCONNECT_RECV&) {});

    ASSERT_TRUE(connection.open());

    SystemStateHandler<WindowsEventHandler<>> requestHandler;
    requestHandler.enable(handler);

    // Request an unknown system state, should trigger exception
    // Use the string overload explicitly
    requestHandler.requestSystemState(connection, "UnknownState", std::function<void(std::string)>([](std::string){}));

    // Wait up to 2 seconds for the exception message
    for (int i = 0; i < 20 && !gotException; ++i) {
        handler.dispatch(std::chrono::milliseconds(100));
    }
    EXPECT_TRUE(gotException) << "Did not receive exception for unknown system state";
    connection.close();
}