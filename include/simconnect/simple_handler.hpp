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


#include <simconnect/simconnect_message_handler.hpp>

namespace SimConnect {

/**
 * A Simple SimConnect message handler. This handler has no blocking handler, so it will not wait for messages.
 * @tparam connection_type The SimConnect connection type.
 * @tparam handler_type The handler processor type.
 * @tparam logger_type The logger type.
 */
template <class connection_type, class handler_type = SimpleHandlerProc<SIMCONNECT_RECV>, class logger_type = NullLogger>
class SimpleHandler : public SimConnectMessageHandler<connection_type, SimpleHandler<connection_type, handler_type, logger_type>, handler_type, logger_type>
{
public:
    SimpleHandler(connection_type& connection) : SimConnectMessageHandler<connection_type, SimpleHandler<connection_type, handler_type, logger_type>, handler_type, logger_type>(connection) {}
    virtual ~SimpleHandler() {}

    SimpleHandler(const SimpleHandler&) = delete;
    SimpleHandler(SimpleHandler&&) = delete;
    SimpleHandler& operator=(const SimpleHandler&) = delete;
    SimpleHandler& operator=(SimpleHandler&&) = delete;

    /**
     * Handles incoming SimConnect messages.
     * @param duration The maximum amount of time to wait for a message, defaults to 0ms meaning don't wait.
     */
    void dispatch([[maybe_unused]] std::chrono::milliseconds duration = std::chrono::milliseconds(0)) {
        this->dispatchWaitingMessages();
    }
};
}