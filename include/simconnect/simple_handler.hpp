#pragma once
/*
 * Copyright (c) 2024. Bert Laverman
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

#include <simconnect/simconnect.hpp>
#include <simconnect/simconnect_message_handler.hpp>

namespace SimConnect {

/**
 * A Simple SimConnect message handler. This handler has no blocking handler, so it will not wait for messages.
 * 
 * @tparam C The SimConnect connection type.
 * @tparam H The handler processor type.
 */
template <class C, class H = MultiHandlerPolicy<Messages::MsgBase>>
class SimpleHandler : public SimConnectMessageHandler<C, SimpleHandler<C, H>, H>
{
public:
    using connection_type = C;
    using handler_type = H;


    SimpleHandler(connection_type& connection, LogLevel logLevel = LogLevel::Info)
        : SimConnectMessageHandler<connection_type, SimpleHandler<connection_type, handler_type>, handler_type>(connection, "SimpleHandler", logLevel)
    {
    }


    ~SimpleHandler()  = default;

    SimpleHandler(const SimpleHandler&) = delete;
    SimpleHandler(SimpleHandler&&) = delete;
    SimpleHandler& operator=(const SimpleHandler&) = delete;
    SimpleHandler& operator=(SimpleHandler&&) = delete;

    /**
     * Handles incoming SimConnect messages. Note that for the SimpleHandler, the duration parameter is ignored.
     * 
     * @param duration The maximum amount of time to wait for a message, defaults to 0ms meaning don't wait.
     */
    void dispatchFor([[maybe_unused]] std::chrono::milliseconds duration = noWait) {
        this->dispatchWaitingMessages();
    }


    /**
     * Handles any waiting messages until the specified predicate returns true. Note that for the SimpleHandler, this method performs the same as dispatch().
     * 
     * @param predicate The predicate to evaluate.
     * @param checkInterval The interval to check the predicate, defaults to 100ms.
     */
    void dispatchUntil([[maybe_unused]]std::function<bool()> predicate, [[maybe_unused]] std::chrono::milliseconds checkInterval = defaultDispatchInterval) {
        dispatch();
    }


    /**
     * Handles incoming SimConnect messages until the connection is closed. Note that for the SimpleHandler, this method performs the same as dispatch().
     * If you actually want it to wait, use the PollingHandler instead.
     */
    void dispatchUntilClosed() {
        dispatch();
    }


    /**
     * Handles any waiting messages until the specified deadline is reached or the predicate returns true. Note that for the SimpleHandler, this method performs the same as dispatch().
     * 
     * @param predicate The predicate to evaluate.
     * @param duration The maximum duration to handle messages.
     * @param checkInterval The interval to check the predicate, defaults to 100ms.
     */
    void dispatchUntilOrTimeout([[maybe_unused]] std::function<bool()> predicate, [[maybe_unused]] std::chrono::milliseconds duration, [[maybe_unused]] std::chrono::milliseconds checkInterval = defaultDispatchInterval) {
        dispatch();
    }
};
}