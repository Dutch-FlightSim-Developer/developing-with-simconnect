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
#include <simconnect/windows_event_connection.hpp>

namespace SimConnect {


/**
 * A SimConnect message handler.
 */
template <bool ThreadSafe = false, class L = NullLogger, class M = MultiHandlerPolicy<Messages::MsgBase>>
class WindowsEventHandler : public SimConnectMessageHandler<WindowsEventConnection<ThreadSafe, L>, WindowsEventHandler<ThreadSafe, L, M>, M>
{
public:
    using connection_type = WindowsEventConnection<ThreadSafe, L>;
    using handler_id_type = typename M::handler_id_type;
	using handler_proc_type = typename M::handler_proc_type;
	using logger_type = L;

private:
    WindowsEventHandler(const WindowsEventHandler&) = delete;
    WindowsEventHandler(WindowsEventHandler&&) = delete;
    WindowsEventHandler& operator=(const WindowsEventHandler&) = delete;
    WindowsEventHandler& operator=(WindowsEventHandler&&) = delete;

public:
    WindowsEventHandler(connection_type& connection, LogLevel logLevel = LogLevel::Info)
        : SimConnectMessageHandler<WindowsEventConnection<ThreadSafe, L>, WindowsEventHandler<ThreadSafe, L, M>, M>(connection, "WindowsEventHandler", logLevel)
    {
    }

    ~WindowsEventHandler() = default;

    
    /**
     * Handles incoming SimConnect messages.
     * 
     * @param duration The maximum amount of time to wait for a message, defaults to 0ms meaning don't wait.
     */
    void dispatchFor(std::chrono::milliseconds duration = noWait) {
        const auto deadline = std::chrono::steady_clock::now() + duration;
        do {
            if (this->isAutoClosing() && !this->connection_.isOpen()) {
                break;
            }
            if (this->connection_.checkForMessage((duration == noWait) ? noWait : (std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now())))) {
                this->dispatchWaitingMessages();
            }
        } while (deadline > std::chrono::steady_clock::now());
    }


    /**
     * Dispatches messages until the specified predicate returns true. Note dispatching will also stop if the connection is closed.
     *
     * @param predicate The predicate to evaluate to determine when to stop dispatching messages.
     * @param checkInterval The interval to wait between checks of the predicate.
     */
    void dispatchUntil(std::function<bool()> predicate, std::chrono::milliseconds checkInterval = defaultDispatchInterval) {
        while (!predicate()) {
            if (this->isAutoClosing() && !this->connection_.isOpen()) {
                break;
            }
            if (this->connection_.checkForMessage(checkInterval)) {
                this->dispatchWaitingMessages();
            }
        }
    }


    /**
     * Dispatches messages until the connection is closed.
     */
    void dispatchUntilClosed() {
        while (this->connection_.isOpen()) {
            if (this->connection_.checkForMessage(defaultDispatchInterval)) {
                this->dispatchWaitingMessages();
            }
        }
    }


    /**
     * Dispatches messages until the specified deadline is reached or the predicate returns true. Note dispatching will also stop if the connection is closed.
     *
     * @param predicate The predicate to evaluate to determine when to stop dispatching messages.
     * @param duration The maximum duration to dispatch messages.
     * @param checkInterval The interval to wait between checks of the predicate.
     */
    void dispatchUntilOrTimeout(std::function<bool()> predicate, std::chrono::milliseconds duration, std::chrono::milliseconds checkInterval = defaultDispatchInterval) {
        auto startTime = std::chrono::steady_clock::now();
        auto deadline = startTime + duration;

        while (!predicate() && (std::chrono::steady_clock::now() < deadline)) {
            if (this->isAutoClosing() && !this->connection_.isOpen()) {
                break;
            }
            if (this->connection_.checkForMessage(checkInterval)) {
                this->dispatchWaitingMessages();
            }
        }
    }
};
}