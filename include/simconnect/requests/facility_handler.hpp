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
#include <simconnect/requests/requests.hpp>
#include <simconnect/requests/facilities/facility_definition.hpp>
#include <simconnect/requests/facilities/facility_definition_builder.hpp>

#include <simconnect/util/logger.hpp>


namespace SimConnect {

template <class M>
class FacilityHandler : public MessageHandler<RequestId, FacilityHandler<M>, M, Messages::facilityData, Messages::facilityDataEnd, Messages::facilityMinimalList>
{
public:
    using simconnect_message_handler_type = M;
    using connection_type = typename M::connection_type;
    using logger_type = typename M::logger_type;
    using handler_type = typename M::handler_type;

private:
    simconnect_message_handler_type& simConnectMessageHandler_;


    // No copies or moves
    FacilityHandler(const FacilityHandler&) = delete;
    FacilityHandler(FacilityHandler&&) = delete;
    FacilityHandler& operator=(const FacilityHandler&) = delete;
    FacilityHandler& operator=(FacilityHandler&&) = delete;

public:
    FacilityHandler(simconnect_message_handler_type& handler) : simConnectMessageHandler_(handler)
    {
        this->enable(simConnectMessageHandler_);
    }
    ~FacilityHandler() = default;
    /**
     * Returns the request ID from the message. This is specific to the Messages::FacilityDataMsg and
	 * Messages::FacilityDataEndMsg messages.
     * 
     * @param msg The message to get the correlation ID from.
     * @returns The correlation ID from the message.
	 */
    RequestId correlationId(const Messages::MsgBase& msg) const {
        switch (msg.dwID) {
        case Messages::facilityData:
            return reinterpret_cast<const Messages::FacilityDataMsg&>(msg).UserRequestId;

        case Messages::facilityDataEnd:
            return reinterpret_cast<const Messages::FacilityDataEndMsg&>(msg).RequestId;

        case Messages::facilityMinimalList:
            return reinterpret_cast<const Messages::FacilityMinimalListMsg&>(msg).dwRequestID;

        default:
            break;
        }
        return noRequest;
    }


    /**
     * Builds a definition based on a definition Builder.
     * 
     * @param builder The facility definition builder.
     * @returns The facility definition ID.
     */
    template <std::size_t MaxLength>
    FacilityDefinitionId buildDefinition(Facilities::Builder<MaxLength>& builder) {
        FacilityDefinitionId defId = Facilities::nextFacilityDefinitionId();

        for (std::size_t index = 0; index < builder.definition.fieldCount; ++index) {
            const unsigned fieldId = static_cast<std::size_t>(builder.definition.fields[index]);

            if (!this->simConnectMessageHandler_.connection().addToFacilityDefinition(defId, Facilities::facilityFieldInfos[fieldId].name)) {
                simConnectMessageHandler_.logger().error("Failed to add field {} ('{}') to facility definition {}.",
                                                        fieldId, Facilities::facilityFieldInfos[fieldId].name, defId);
            }
            else {
                simConnectMessageHandler_.logger().debug("Added field {} ('{}') to facility definition {}.",
                                                        fieldId, Facilities::facilityFieldInfos[fieldId].name, defId);
            }
        }
        return defId;
    }


    /**
     * Requests facility data for the specified facility definition ID and ICAO code.
     * 
     * @param facilityDefId The facility definition ID.
     * @param icaoCode The ICAO code of the facility.
     * @param region The region code of the facility.
     * @param onData The callback to be called when facility data is received.
     * @param onEnd The callback to be called when the facility data request is complete.
     * @param onConflict The callback to be called when the ICAO+Region combination was not unique.
     * @returns The Request object representing the facility data request.
     */
    [[nodiscard]]
    Request requestFacilityData(FacilityDefinitionId facilityDefId, std::string_view icaoCode, std::string_view region = "",
                                std::function<void(const Messages::FacilityDataMsg&)> onData = nullptr,
                                std::function<void()> onEnd = nullptr,
                                std::function<void(const Messages::FacilityMinimalListMsg&)> onConflict = nullptr)
    {
        auto requestId = simConnectMessageHandler_.connection().requests().nextRequestID();
        auto& logger = simConnectMessageHandler_.logger();

        this->registerHandler(requestId, [onData, onEnd, onConflict, &logger]
                              (const Messages::MsgBase& msg) {
            switch (msg.dwID) {
            case Messages::facilityData:
            {
                auto &dataMsg = reinterpret_cast<const Messages::FacilityDataMsg&>(msg);
                if (onData) {
                    logger.debug("Received facility data message for request ID {}: type={}, data-id={}, parent-id={}.",
                        dataMsg.UserRequestId, static_cast<int>(dataMsg.Type), dataMsg.UniqueRequestId, dataMsg.ParentUniqueRequestId);
                    onData(dataMsg);
                }
                else {
                    logger.warn("Received facility data message for request ID {}, but no data handler is set.", dataMsg.UserRequestId);
                }
            }
                break;

            case Messages::facilityDataEnd:
            {
                auto &endMsg = reinterpret_cast<const Messages::FacilityDataEndMsg&>(msg);
                if (onEnd) {
                    logger.debug("Received facility data end message for request ID {}.", endMsg.RequestId);
                    onEnd();
                }
                else {
                    logger.warn("Received facility data end message for request ID {}, but no end handler is set.", endMsg.RequestId);
                }
            }
                break;

            case Messages::facilityMinimalList:
            {
                auto &listMsg = reinterpret_cast<const Messages::FacilityMinimalListMsg&>(msg);
                if (onConflict) {
                    onConflict(listMsg);
                }
                else {
                    logger.warn("Received facility minimal list message for request ID {}, but no conflict handler is set.", listMsg.dwRequestID);
                }
            }
                break;

            default:
                logger.warn("Received unexpected message ID {} for facility data request.", msg.dwID);
                break;
            }
        }, false);
        simConnectMessageHandler_.connection().requestFacilityData(requestId, facilityDefId, icaoCode, region);

        return Request{ requestId, [this, requestId]() {
            // No specific stop function for facility data request, so just unregister the handler
            this->removeHandler(requestId);
        } };
    }
};
} // namespace SimConnect