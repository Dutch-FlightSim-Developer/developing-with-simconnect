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
#include <simconnect/simconnect_datatypes.hpp>
#include <simconnect/message_handler.hpp>

#include <simconnect/requests/facilities/facility_definition.hpp>


namespace SimConnect {


using AirportDetails = LatLonAlt;
using AirportHandler = std::function<void(std::string_view ident, std::string_view region, const AirportDetails& position)>;

using WaypointDetails = LatLonAltMagVar;
using WaypointHandler = std::function<void(std::string_view ident, std::string_view region, const WaypointDetails& details)>;

#pragma pack(push, 1)
struct NdbDetails : public SimConnect::LatLonAltMagVar {
    float frequency;

    inline float frequencyKHz() const noexcept {
        constexpr float kHzFactor = 1'000.0F;
        return frequency / kHzFactor;
    }
};
#pragma pack(pop)

using NdbHandler = std::function<void(std::string_view ident, std::string_view region, const NdbDetails& details)>;

#pragma pack(push, 1)
struct VorDetails : public SimConnect::LatLonAltMagVar {
    float frequency;
    unsigned long flags;
    float localizerCourse;
    LatLonAlt glideslopePosition;
    float glideSlopeAngle;

    inline float frequencyMHz() const noexcept {
        constexpr float MHzFactor = 1'000'000.0F;
        return frequency / MHzFactor;
    }

    inline bool hasNavSignal() const noexcept {
        return (flags & SIMCONNECT_RECV_ID_VOR_LIST_HAS_NAV_SIGNAL) != 0;
    }
    inline bool hasLocalizer() const noexcept {
        return (flags & SIMCONNECT_RECV_ID_VOR_LIST_HAS_LOCALIZER) != 0;
    }
    inline bool hasGlideSlope() const noexcept {
        return ((flags & SIMCONNECT_RECV_ID_VOR_LIST_HAS_GLIDE_SLOPE) != 0) && (glideSlopeAngle > 0.0F);
    }
    inline bool hasDME() const noexcept {
        return (flags & SIMCONNECT_RECV_ID_VOR_LIST_HAS_DME) != 0;
    }

};
#pragma pack(pop)

using VorHandler = std::function<void(std::string_view ident, std::string_view region, const VorDetails& details)>;

template <class M>
class FacilityListHandler : public MessageHandler<RequestId, FacilityListHandler<M>, M, Messages::airportList, Messages::waypointList, Messages::ndbList, Messages::vorList>
{
public:
    using simconnect_message_handler_type = M;
    using connection_type = typename M::connection_type;
    using logger_type = typename M::logger_type;
    using handler_type = typename M::handler_type;

private:
    simconnect_message_handler_type& simConnectMessageHandler_;


    // No copies or moves
    FacilityListHandler(const FacilityListHandler&) = delete;
    FacilityListHandler(FacilityListHandler&&) = delete;
    FacilityListHandler& operator=(const FacilityListHandler&) = delete;
    FacilityListHandler& operator=(FacilityListHandler&&) = delete;

public:
    FacilityListHandler(simconnect_message_handler_type& handler, std::string loggerName = "SimConnect::FacilityListHandler", LogLevel logLevel = LogLevel::Info)
        : MessageHandler<RequestId, FacilityListHandler<M>, M, Messages::airportList, Messages::waypointList, Messages::ndbList, Messages::vorList>(std::move(loggerName), logLevel)
        , simConnectMessageHandler_(handler)
    {
        this->enable(simConnectMessageHandler_);
    }
    FacilityListHandler(simconnect_message_handler_type& handler, typename M::logger_type& parentLogger, std::string loggerName = "SimConnect::FacilityListHandler", LogLevel logLevel = LogLevel::Info)
        : MessageHandler<RequestId, FacilityListHandler<M>, M, Messages::airportList, Messages::waypointList, Messages::ndbList, Messages::vorList>(parentLogger, std::move(loggerName), logLevel)
        , simConnectMessageHandler_(handler)
    {
        this->enable(simConnectMessageHandler_);
    }

    ~FacilityListHandler() = default;

    /**
     * Returns the request ID from the message. This is specific to the Messages::SimObjectDataMsg and
	 * Messages::SimObjectDataByTypeMsg messages. The latter type does not actually add fields, so we can
     * use the same method for both.
     * 
     * @param msg The message to get the correlation ID from.
     * @returns The correlation ID from the message.
	 */
    RequestId correlationId(const Messages::MsgBase& msg) {
        switch (msg.dwID) {
        case Messages::airportList:
        {
            const RequestId reqId = reinterpret_cast<const Messages::AirportListMsg&>(msg).dwRequestID;
            this->logger().debug("Processing AirportListMsg for request {}.", reqId);
            return reqId;
        }

        case Messages::waypointList:
        {
            RequestId reqId = reinterpret_cast<const Messages::WaypointListMsg&>(msg).dwRequestID;
            this->logger().debug("Processing WaypointListMsg for request {}.", reqId);
            return reqId;
        }

        case Messages::ndbList:
        {
            RequestId reqId = reinterpret_cast<const Messages::NdbListMsg&>(msg).dwRequestID;
            this->logger().debug("Processing NdbListMsg for request {}.", reqId);
            return reqId;
        }

        case Messages::vorList:
        {
            RequestId reqId = reinterpret_cast<const Messages::VorListMsg&>(msg).dwRequestID;
            this->logger().debug("Processing VorListMsg for request {}.", reqId);
            return reqId;
        }

        default:
            this->logger().warn("Received unknown message ID {} when trying to get correlation ID.", static_cast<int>(msg.dwID));
            break;
        }
        return noRequest;
    }


#pragma region airports

    /**
     * Requests the enumeration of airports, invoking the provided handler for each received message.
     * 
     * @param scope The scope of the facilities list to enumerate.
     * @param handler The handler to invoke for each received message.
     * @param onDone An optional callback to invoke when the enumeration is complete.
     * @return A Request object that can be used to stop the request.
     */
    [[nodiscard]]
    Request listAirports(FacilitiesListScope scope,
        std::function<void(std::string_view ident, std::string_view region, const AirportDetails& details)> handler,
        std::function<void()> onDone = nullptr)
    {
        auto requestId = simConnectMessageHandler_.connection().requests().nextRequestID();

        auto& logger = this->logger();
        this->registerHandler(requestId, [handler, onDone, &logger](const Messages::MsgBase& msg) {
                const Messages::AirportListMsg& enumMsg = reinterpret_cast<const Messages::AirportListMsg&>(msg);

                logger.debug("Received airport list message: request ID {}, array size {}, entry number {}/{}",
                    enumMsg.dwRequestID, enumMsg.dwArraySize, enumMsg.dwEntryNumber, enumMsg.dwOutOf);

                for (unsigned long i = 0; i < enumMsg.dwArraySize; ++i) {
                    const auto& item = enumMsg.rgData[i];
                    handler(std::string_view(&item.Ident[0]),
                            std::string_view(&item.Region[0]),
                            *reinterpret_cast<const AirportDetails*>(&item.Latitude));
                }

                if (enumMsg.dwEntryNumber == (enumMsg.dwOutOf-1)) { // 0 to dwOutOf-1
                    if (onDone) {
                        onDone();
                    }
                }
            }, false);
        simConnectMessageHandler_.connection().listFacilities(requestId, scope, FacilityListTypes::airport);

        return Request{ requestId, [this, requestId]() {
            // No specific stop function for enumeration, so just unregister the handler
            this->removeHandler(requestId);
        } };
    }
#pragma endregion

#pragma region waypoints

    /**
     * Requests the enumeration of waypoints, invoking the provided handler for each received message.
     * 
     * @param scope The scope of the facilities list to enumerate.
     * @param handler The handler to invoke for each received message.
     * @param onDone An optional callback to invoke when the enumeration is complete.
     * @return A Request object that can be used to stop the request.
     */
    [[nodiscard]]
    Request listWaypoints(FacilitiesListScope scope,
        std::function<void(std::string_view ident, std::string_view region, const WaypointDetails& details)> handler,
        std::function<void()> onDone = nullptr)
    {
        auto requestId = simConnectMessageHandler_.connection().requests().nextRequestID();

        this->registerHandler(requestId, [handler, onDone](const Messages::MsgBase& msg) {
                const Messages::WaypointListMsg& enumMsg = reinterpret_cast<const Messages::WaypointListMsg&>(msg);

                for (unsigned long i = 0; i < enumMsg.dwArraySize; ++i) {
                    const auto& item = enumMsg.rgData[i];
                    handler(std::string_view(&item.Ident[0]),
                            std::string_view(&item.Region[0]),
                            *reinterpret_cast<const WaypointDetails*>(&item.Latitude));
                }

                if (enumMsg.dwEntryNumber == (enumMsg.dwOutOf-1)) { // 0 to dwOutOf-1
                    if (onDone) {
                        onDone();
                    }
                }
            }, false);
        simConnectMessageHandler_.connection().listFacilities(requestId, scope, FacilityListTypes::waypoint);

        return Request{ requestId, [this, requestId]() {
            // No specific stop function for enumeration, so just unregister the handler
            this->removeHandler(requestId);
        } };
    }

#pragma endregion

#pragma region NDBs

    /**
     * Requests the enumeration of NDBs, invoking the provided handler for each received message.
     * 
     * @param scope The scope of the facilities list to enumerate.
     * @param handler The handler to invoke for each received message.
     * @param onDone An optional callback to invoke when the enumeration is complete.
     * @return A Request object that can be used to stop the request.
     */
    [[nodiscard]]
    Request listNDBs(FacilitiesListScope scope,
        std::function<void(std::string_view ident, std::string_view region, const NdbDetails& details)> handler,
        std::function<void()> onDone = nullptr)
    {
        auto requestId = simConnectMessageHandler_.connection().requests().nextRequestID();

        this->registerHandler(requestId, [handler, onDone](const Messages::MsgBase& msg) {
                const Messages::NdbListMsg& enumMsg = reinterpret_cast<const Messages::NdbListMsg&>(msg);

                for (unsigned long i = 0; i < enumMsg.dwArraySize; ++i) {
                    const auto& item = enumMsg.rgData[i];
                    handler(std::string_view(&item.Ident[0]),
                            std::string_view(&item.Region[0]),
                            *reinterpret_cast<const NdbDetails*>(&item.Latitude));
                }

                if (enumMsg.dwEntryNumber == (enumMsg.dwOutOf-1)) { // 0 to dwOutOf-1
                    if (onDone) {
                        onDone();
                    }
                }
            }, false);
        simConnectMessageHandler_.connection().listFacilities(requestId, scope, FacilityListTypes::ndb);

        return Request{ requestId, [this, requestId]() {
            // No specific stop function for enumeration, so just unregister the handler
            this->removeHandler(requestId);
        } };
    }

#pragma endregion

#pragma region VORs

    /**
     * Requests the enumeration of VORs, invoking the provided handler for each received message.
     * 
     * @param scope The scope of the facilities list to enumerate.
     * @param handler The handler to invoke for each received message.
     * @param onDone An optional callback to invoke when the enumeration is complete.
     * @return A Request object that can be used to stop the request.
     */
    [[nodiscard]]
    Request listVORs(FacilitiesListScope scope,
        std::function<void(std::string_view ident, std::string_view region, const VorDetails& details)> handler,
        std::function<void()> onDone = nullptr)
    {
        auto requestId = simConnectMessageHandler_.connection().requests().nextRequestID();

        this->registerHandler(requestId, [handler, onDone](const Messages::MsgBase& msg) {
                const Messages::VorListMsg& enumMsg = reinterpret_cast<const Messages::VorListMsg&>(msg);

                for (unsigned long i = 0; i < enumMsg.dwArraySize; ++i) {
                    const auto& item = enumMsg.rgData[i];
                    handler(std::string_view(&item.Ident[0]),
                            std::string_view(&item.Region[0]),
                            *reinterpret_cast<const VorDetails*>(&item.Latitude));
                }

                if (enumMsg.dwEntryNumber == (enumMsg.dwOutOf-1)) { // 0 to dwOutOf-1
                    if (onDone) {
                        onDone();
                    }
                }
            }, false);
        simConnectMessageHandler_.connection().listFacilities(requestId, scope, FacilityListTypes::vor);

        return Request{ requestId, [this, requestId]() {
            // No specific stop function for enumeration, so just unregister the handler
            this->removeHandler(requestId);
        } };
    }

#pragma endregion
};

} // namespace SimConnect