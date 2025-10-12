#include "pch.h"

#include <simconnect/windows_event_connection.hpp>
#include <simconnect/windows_event_handler.hpp>
#include <simconnect/requests/system_state_handler.hpp>

#include <atomic>
#include <chrono>
#include <string>
#include <functional>

using namespace SimConnect;

// Test: Request known system state (AircraftLoaded) and expect a string result
TEST(TestSystemState, RequestAircraftLoaded) {
    WindowsEventConnection<> connection;
    WindowsEventHandler<> handler(connection);
    std::atomic<bool> gotResult{false};
    std::string result;

    handler.setDefaultHandler([](const SIMCONNECT_RECV&) {});
    ASSERT_TRUE(connection.open());

    SystemStateHandler<WindowsEventHandler<>> requestHandler;
    requestHandler.enable(handler);

    requestHandler.requestSystemState(connection, "AircraftLoaded", [&](std::string value) {
        result = value;
        gotResult = true;
    });

    // Wait up to 2 seconds for the result
    for (int i = 0; i < 20 && !gotResult; ++i) {
        handler.dispatch(std::chrono::milliseconds(100));
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

    handler.setDefaultHandler([](const SIMCONNECT_RECV&) {});
    ASSERT_TRUE(connection.open());

    SystemStateHandler<WindowsEventHandler<>> requestHandler;
    requestHandler.enable(handler);

    requestHandler.requestSystemState(connection, "DialogMode", [&](bool value) {
        dialogMode = value;
        gotResult = true;
    });

    // Wait up to 2 seconds for the result
    for (int i = 0; i < 20 && !gotResult; ++i) {
        handler.dispatch(std::chrono::milliseconds(100));
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

    handler.registerHandler<SIMCONNECT_RECV_EXCEPTION>(SIMCONNECT_RECV_ID_EXCEPTION, [&](const SIMCONNECT_RECV_EXCEPTION&) {
        gotException = true;
    });
    handler.setDefaultHandler([](const SIMCONNECT_RECV&) {});
    ASSERT_TRUE(connection.open());

    SystemStateHandler<WindowsEventHandler<>> requestHandler;
    requestHandler.enable(handler);

    requestHandler.requestSystemState(connection, "UnknownState", std::function<void(std::string)>([](std::string){}));

    // Wait up to 2 seconds for the exception message
    for (int i = 0; i < 20 && !gotException; ++i) {
        handler.dispatch(std::chrono::milliseconds(100));
    }
    EXPECT_TRUE(gotException) << "Did not receive exception for unknown system state";
    connection.close();
}
