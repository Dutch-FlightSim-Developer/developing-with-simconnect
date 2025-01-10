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


#include <simconnect/handler.hpp>

#include <thread>


namespace SimConnect {

/**
 * A SimConnect message handler that employs polling.
 * 
 * **NOTE** This is not a good way to handle SimConnect messages.
 */
template <class C>
class PollingHandler : public Handler<C, PollingHandler<C>>
{
private:
    std::chrono::milliseconds sleep_duration_ = std::chrono::milliseconds(100);

public:
    PollingHandler(C& connection) : Handler<C, PollingHandler<C>>(connection) {}
    PollingHandler(C& connection, std::chrono::milliseconds sleep_duration) : Handler<C, PollingHandler<C>>(connection), sleep_duration_(sleep_duration) {}
    virtual ~PollingHandler() {}

    PollingHandler(const PollingHandler&) = delete;
    PollingHandler(PollingHandler&&) = delete;
    PollingHandler& operator=(const PollingHandler&) = delete;
    PollingHandler& operator=(PollingHandler&&) = delete;

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
};
}