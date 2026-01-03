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

#include <simconnect/simconnect.hpp>
#include <simconnect/message_handler.hpp>


namespace SimConnect {

namespace SystemState {

    constexpr static const char* aircraftLoaded = "AircraftLoaded";
    constexpr static const char* flightLoaded = "FlightLoaded";
    constexpr static const char* flightPlan = "FlightPlan";
    constexpr static const char* dialogMode = "DialogMode";
    constexpr static const char* sim = "Sim";

}


template <class M>
class SystemStateHandler : public MessageHandler<RequestId, SystemStateHandler<M>, M, Messages::systemState> {
public:
    using simconnect_message_handler_type = M;
	using connection_type = typename simconnect_message_handler_type::connection_type;


private:
    simconnect_message_handler_type& simConnectMessageHandler_;


    // No copies or moves
    SystemStateHandler(const SystemStateHandler&) = delete;
    SystemStateHandler(SystemStateHandler&&) = delete;
    SystemStateHandler& operator=(const SystemStateHandler&) = delete;
    SystemStateHandler& operator=(SystemStateHandler&&) = delete;

public:
    SystemStateHandler(simconnect_message_handler_type& handler) : simConnectMessageHandler_(handler)
    {
        this->enable(simConnectMessageHandler_);
    }
    ~SystemStateHandler() = default;


    /**
     * Returns the correlation ID from the message. This is specific to the Messages::systemState message.
     *
     * @param msg The message to get the correlation ID from.
     * @returns The correlation ID from the message.
     */
    RequestId correlationId(const Messages::MsgBase& msg) {
        return static_cast<const Messages::SystemStateMsg*>(&msg)->dwRequestID;
    }


    /**
     * Requests a bool-valued system state.
     * 
     * @param name The name of the state to request.
     * @param requestHandler The handler to execute when the state is received.
     */
    void requestSystemState(std::string name, std::function<void(bool)> requestHandler) {
        auto requestId = simConnectMessageHandler_.connection().requests().nextRequestID();

        this->registerHandler(requestId, [requestHandler](const Messages::MsgBase& msg) {
            auto& state = reinterpret_cast<const Messages::SystemStateMsg&>(msg);
            requestHandler(state.dwInteger != 0);
        }, true);
        simConnectMessageHandler_.connection().requestSystemState(name, requestId);
    }


    /**
     * Requests a string-valued system state.
     * 
     * @param name The name of the state to request.
     * @param requestHandler The handler to execute when the state is received.
     */
    void requestSystemState(std::string name, std::function<void(std::string)> requestHandler) {
        auto requestId = simConnectMessageHandler_.connection().requests().nextRequestID();

        this->registerHandler(requestId, [requestHandler](const Messages::MsgBase& msg) {
            auto& state = reinterpret_cast<const Messages::SystemStateMsg&>(msg);
            requestHandler(std::string(state.szString));
        }, true);
        simConnectMessageHandler_.connection().requestSystemState(name, requestId);
    }

};

} // namespace SimConnect