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
 * A SimConnect message handler.
 */
template <bool ThreadSafe = false, class handler_type = SimpleHandlerProc<SIMCONNECT_RECV>, class logger_type = NullLogger>
class WindowsEventHandler : public SimConnectMessageHandler<WindowsEventConnection<ThreadSafe>, WindowsEventHandler<ThreadSafe, handler_type, logger_type>, handler_type, logger_type>
{
public:
	using connection_type = WindowsEventConnection<ThreadSafe>;


    WindowsEventHandler(WindowsEventConnection<ThreadSafe>& connection) : SimConnectMessageHandler<WindowsEventConnection<ThreadSafe>, WindowsEventHandler<ThreadSafe, handler_type, logger_type>, handler_type, logger_type>(connection) {}
    ~WindowsEventHandler() {}

    WindowsEventHandler(const WindowsEventHandler&) = delete;
    WindowsEventHandler(WindowsEventHandler&&) = delete;
    WindowsEventHandler& operator=(const WindowsEventHandler&) = delete;
    WindowsEventHandler& operator=(WindowsEventHandler&&) = delete;

    /**
     * Handles incoming SimConnect messages.
     * @param connection The connection to handle messages from.
     * @param duration The maximum amount of time to wait for a message, defaults to 0ms meaning don't wait.
     */
    void dispatch(std::chrono::milliseconds duration = std::chrono::milliseconds(0)) {
        const auto deadline = std::chrono::steady_clock::now() + duration;
        do {
            if (this->isAutoClosing() && !this->connection_.isOpen()) {
                break;
            }
            if (this->connection_.checkForMessage(std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now()))) {
                this->dispatchWaitingMessages();
            }
        } while (deadline > std::chrono::steady_clock::now());
    }
};
}