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

#include <map>
#include <set>
#include <string>
#include <string_view>
#include <functional>

#include <simconnect/simconnect.hpp>
#include <simconnect/message_handler.hpp>


namespace SimConnect {


template <class M>
class SimObjectAndLiveryHandler : public MessageHandler<RequestId, SimObjectAndLiveryHandler<M>, M, Messages::enumerateSimObjectAndLiveryList>
{
public:
    using simconnect_message_handler_type = M;
    using connection_type = typename M::connection_type;
    using logger_type = typename M::logger_type;
    using handler_type = typename M::handler_type;

private:
    simconnect_message_handler_type& simConnectMessageHandler_;


    // No copies or moves
    SimObjectAndLiveryHandler(const SimObjectAndLiveryHandler&) = delete;
    SimObjectAndLiveryHandler(SimObjectAndLiveryHandler&&) = delete;
    SimObjectAndLiveryHandler& operator=(const SimObjectAndLiveryHandler&) = delete;
    SimObjectAndLiveryHandler& operator=(SimObjectAndLiveryHandler&&) = delete;


public:
    SimObjectAndLiveryHandler(simconnect_message_handler_type& handler) : simConnectMessageHandler_(handler)
    {
        this->enable(simConnectMessageHandler_);
    }
    ~SimObjectAndLiveryHandler() = default;


    /**
     * Returns the request ID from the message. This is specific to the Messages::SimObjectDataMsg and
	 * Messages::SimObjectDataByTypeMsg messages. The latter type does not actually add fields, so we can
     * use the same method for both.
     * 
     * @param msg The message to get the correlation ID from.
     * @returns The correlation ID from the message.
	 */
    RequestId correlationId(const Messages::MsgBase& msg) const {
        return static_cast<const Messages::EnumerateSimObjectAndLiveryListMsg&>(msg).dwRequestID;
    }


    /**
     * Requests the enumeration of SimObjects and liveries, invoking the provided handler for each received message.
     * 
     * @param simObjectType The type of SimObject to enumerate.
     * @param handler The handler to invoke for each received message.
     * @param onDone An optional callback to invoke when the enumeration is complete.
     * @return A Request object that can be used to stop the request.
     */
    [[nodiscard]]
    Request requestEnumeration(SimObjectType simObjectType,
        std::function<void(std::string_view title, std::string_view livery)> handler,
        std::function<void()> onDone = nullptr)
    {
        auto requestId = simConnectMessageHandler_.connection().requests().nextRequestID();

        this->registerHandler(requestId, [requestId, handler, onDone](const Messages::MsgBase& msg) {
                const Messages::EnumerateSimObjectAndLiveryListMsg& enumMsg = reinterpret_cast<const Messages::EnumerateSimObjectAndLiveryListMsg&>(msg);

                for (unsigned long i = 0; i < enumMsg.dwArraySize; ++i) {
                    const auto& item = enumMsg.rgData[i];
                    handler(std::string_view(&item.AircraftTitle[0]), std::string_view(&item.LiveryName[0]));
                }

                if (enumMsg.dwEntryNumber == (enumMsg.dwOutOf-1)) { // 0 to dwOutOf-1
                    if (onDone) {
                        onDone();
                    }
                }
            }, false);
        simConnectMessageHandler_.connection().enumerateSimObjectsAndLiveries(requestId, simObjectType);

        return Request{ requestId, [this, requestId]() {
            // No specific stop function for enumeration, so just unregister the handler
            this->removeHandler(requestId);
        } };
    }


    /**
     * Requests the enumeration of SimObjects and liveries, invoking the provided handler with a all
     * data in a map (keyed on title) with sets of liveries.
     * 
     * @param simObjectType The type of SimObject to enumerate.
     * @param handler The handler to invoke with the collected data when enumeration is complete.
     * @return A Request object that can be used to stop the request.
     */
    [[nodiscard]]
    Request requestEnumeration(SimObjectType simObjectType,
        std::function<void(const std::map<std::string, std::set<std::string>>&)> handler)
    {
        auto requestId = simConnectMessageHandler_.connection().requests().nextRequestID();
        auto result = std::make_shared<std::map<std::string, std::set<std::string>>>();

        this->registerHandler(requestId, [handler, result](const Messages::MsgBase& msg) {
                const Messages::EnumerateSimObjectAndLiveryListMsg& enumMsg = reinterpret_cast<const Messages::EnumerateSimObjectAndLiveryListMsg&>(msg);

                for (unsigned long i = 0; i < enumMsg.dwArraySize; ++i) {
                    const auto& item = enumMsg.rgData[i];
                    (*result)[std::string(&item.AircraftTitle[0])].insert(std::string(&item.LiveryName[0]));
                }

                if (enumMsg.dwEntryNumber == (enumMsg.dwOutOf-1)) { // 0 to dwOutOf-1
                    handler(*result);
                }
            }, false);
        simConnectMessageHandler_.connection().enumerateSimObjectsAndLiveries(requestId, simObjectType);

        return Request{ requestId, [this, requestId]() {
            // No specific stop function for enumeration, so just unregister the handler
            this->removeHandler(requestId);
        } };
    }
};

} // namespace SimConnect