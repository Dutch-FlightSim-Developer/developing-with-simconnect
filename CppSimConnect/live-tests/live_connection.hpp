#pragma once
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


#include <atomic>
#include <chrono>
#include <format>
#include <iostream>
#include <string>
#include <string_view>

#include <simconnect.hpp>
#include <simconnect/simconnect.hpp>

#include <simconnect/util/console_logger.hpp>
#include <simconnect/util/logger.hpp>

#include <simconnect/windows_event_connection.hpp>
#include <simconnect/windows_event_handler.hpp>


namespace LiveTests {

using namespace SimConnect;
using namespace std::chrono_literals;


using TestConnection = WindowsEventConnection<true, ConsoleLogger>;
using TestMessageHandler = WindowsEventHandler<true, ConsoleLogger>;

constexpr unsigned long TEST_EVENT_DATA = 42;
constexpr unsigned long DEFAULT_EVENT_DATA = 1;
constexpr auto DEFAULT_TIMEOUT = 2s;


/**
 * Helper class to set up an event sender client.
 */
class LiveConnection
{

    /**
    * Handle SimConnect Exception messages.
    *
    * @param msg The exception message to handle.
    */
    static void handleException(const Messages::ExceptionMsg &msg);


public:
    TestConnection connection;
    TestMessageHandler handler;

    std::atomic<bool> gotOpen{ false };

    explicit LiveConnection(std::string_view name) : connection(name), handler(connection)
    {
        connection.logger().level(LogLevel::Debug);
        handler.logger().level(LogLevel::Debug);

        handler.registerHandler<Messages::OpenMsg>(Messages::open, [this](const Messages::OpenMsg &) { gotOpen = true; });
        handler.registerHandler<Messages::ExceptionMsg>(Messages::exception, handleException);
    }

    bool open() { return connection.open(); }

    bool waitForOpen(std::chrono::milliseconds timeout = DEFAULT_TIMEOUT)
    {
        handler.handleUntilOrTimeout([this]() { return gotOpen.load(); }, timeout);
        return gotOpen.load();
    }

    bool openAndWait(std::chrono::milliseconds timeout = DEFAULT_TIMEOUT) { return open() && waitForOpen(timeout); }

    void waitFor(std::chrono::milliseconds duration) {
        handler.handleFor(duration);
    }

    template<typename Predicate>
    bool waitUntil(Predicate pred, std::chrono::milliseconds timeout = DEFAULT_TIMEOUT) {
        handler.handleUntilOrTimeout(pred, timeout);
        return pred();
    }

    void close() { connection.close(); }

    bool succeeded() const { return connection.succeeded(); }
};

}// namespace LiveTests