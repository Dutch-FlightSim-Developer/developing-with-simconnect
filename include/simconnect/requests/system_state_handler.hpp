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
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <simconnect/requests/request_handler.hpp>


namespace SimConnect {

class SystemStateHandler : public RequestHandler<SystemStateHandler, SIMCONNECT_RECV_ID_SYSTEM_STATE> {

    // No copies or moves
    SystemStateHandler(const SystemStateHandler&) = delete;
    SystemStateHandler(SystemStateHandler&&) = delete;
    SystemStateHandler& operator=(const SystemStateHandler&) = delete;
    SystemStateHandler& operator=(SystemStateHandler&&) = delete;

public:
    SystemStateHandler() = default;
    ~SystemStateHandler() = default;


    /**
     * Returns the request ID from the message. This is specific to the SIMCONNECT_RECV_SYSTEM_STATE message.
     *
     * @param msg The message to get the request ID from.
     * @returns The request ID from the message.
     */
    unsigned long requestId(const SIMCONNECT_RECV* msg) const {
        return static_cast<const SIMCONNECT_RECV_SYSTEM_STATE*>(msg)->dwRequestID;
    }


    /**
     * Requests a bool-valued system state.
     * 
     * @param connection The connection to request the state from.
     * @param name The name of the state to request.
     * @param requestHandler The handler to execute when the state is received.
     */
    void requestSystemState(Connection& connection, std::string name, std::function<void(bool)> requestHandler) {
        auto requestId = connection.requests().nextRequestID();

        registerHandler(requestId, [requestHandler](const SIMCONNECT_RECV* msg, [[maybe_unused]] DWORD size) {
            auto& state = *reinterpret_cast<const SIMCONNECT_RECV_SYSTEM_STATE*>(msg);
            requestHandler(state.dwInteger != 0);
        }, true);
        connection.requestSystemState(name, requestId);
    }


    /**
     * Requests a string-valued system state.
     * 
     * @param connection The connection to request the state from.
     * @param name The name of the state to request.
     * @param requestHandler The handler to execute when the state is received.
     */
    void requestSystemState(Connection& connection, std::string name, std::function<void(std::string)> requestHandler) {
        auto requestId = connection.requests().nextRequestID();

        registerHandler(requestId, [requestHandler](const SIMCONNECT_RECV* msg, [[maybe_unused]] DWORD size) {
            auto& state = *reinterpret_cast<const SIMCONNECT_RECV_SYSTEM_STATE*>(msg);
            requestHandler(std::string(state.szString));
        }, true);
        connection.requestSystemState(name, requestId);
    }

};

} // namespace SimConnect