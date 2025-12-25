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

#include <thread>


namespace SimConnect {

/**
 * A SimConnect message handler that employs polling.
 * 
 * **NOTE** This is not a good way to handle SimConnect messages.
 * 
 * @tparam C The SimConnect connection type.
 * @tparam M The handler policy type.
 */
template <class C, class M = MultiHandlerPolicy<Messages::MsgBase>>
class PollingHandler : public SimConnectMessageHandler<C, PollingHandler<C, M>, M>
{
public:
    using connection_type = C;
    using handler_type = M;
	using logger_type = typename connection_type::logger_type;


private:
    std::chrono::milliseconds sleep_duration_ = std::chrono::milliseconds(100);


    PollingHandler(const PollingHandler&) = delete;
    PollingHandler(PollingHandler&&) = delete;
    PollingHandler& operator=(const PollingHandler&) = delete;
    PollingHandler& operator=(PollingHandler&&) = delete;

public:
    PollingHandler(connection_type& connection, LogLevel logLevel = LogLevel::Info)
        : SimConnectMessageHandler<connection_type, PollingHandler<connection_type, handler_type>>(connection, "PollingHandler", logLevel)
    {
    }
    PollingHandler(connection_type& connection, std::chrono::milliseconds sleep_duration, LogLevel logLevel = LogLevel::Info)
        : SimConnectMessageHandler<connection_type, PollingHandler<connection_type, handler_type>>(connection, "PollingHandler", logLevel), sleep_duration_(sleep_duration)
    {}
    ~PollingHandler() = default;


    std::chrono::milliseconds getSleepDuration() const noexcept { return sleep_duration_; }
    void setSleepDuration(std::chrono::milliseconds sleep_duration) noexcept { sleep_duration_ = sleep_duration; }

    /**
     * Handles incoming SimConnect messages.
     * @param connection The connection to handle messages from.
     * @param duration The maximum amount of time to wait for a message, defaults to 0ms meaning don't wait.
     */
    void dispatch(std::chrono::milliseconds duration = std::chrono::milliseconds(0)) {
        const auto deadline = std::chrono::steady_clock::now() + duration;
        do {
            this->dispatchWaitingMessages();
            if (deadline > std::chrono::steady_clock::now()) {
                std::this_thread::sleep_for(sleep_duration_);
            }
        } while (deadline > std::chrono::steady_clock::now());
    }


    /**
     * Handles incoming SimConnect messages until the specified predicate returns true. Note handling will also stop if the connection is closed.
     * 
     * @param predicate The predicate to evaluate.
     * @param checkInterval The interval to wait between checks of the predicate.
     */
    void dispatchUntil(std::function<bool()> predicate, std::chrono::milliseconds checkInterval = std::chrono::milliseconds(100)) {
        while (this->connection().isOpen() && !predicate()) {
            this->dispatchWaitingMessages();
            std::this_thread::sleep_for(checkInterval);
        }
    }


    /**
     * Handles incoming SimConnect messages until the specified deadline is reached or the predicate returns true. Note handling will also stop if the connection is closed.
     * 
     * @param predicate The predicate to evaluate.
     * @param duration The maximum duration to handle messages.
     * @param checkInterval The interval to wait between checks of the predicate.
     */
    void dispatchUntil(std::function<bool()> predicate, std::chrono::milliseconds duration, std::chrono::milliseconds checkInterval = std::chrono::milliseconds(100)) {
        const auto deadline = std::chrono::steady_clock::now() + duration;
        while (this->connection().isOpen() && (std::chrono::steady_clock::now() < deadline) && !predicate()) {
            this->dispatchWaitingMessages();
            std::this_thread::sleep_for(checkInterval);
        }
    }
};
}