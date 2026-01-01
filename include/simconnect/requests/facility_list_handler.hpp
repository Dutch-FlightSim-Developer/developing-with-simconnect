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

#include <cmath>

#include <simconnect/simconnect.hpp>
#include <simconnect/message_handler.hpp>

#include <simconnect/requests/facilities/facility_definition.hpp>


namespace SimConnect {


#pragma pack(push, 1)
struct AirportDetails {
    DataTypes::LatLonAlt position;

    constexpr double latitude() const noexcept {
        return position.Latitude;
    }
    constexpr double latitudeNormalized() const noexcept {
        return std::fabs(position.Latitude);
    }
    constexpr char latitudeDirection() const noexcept {
        if (position.Latitude == 0.0) {
            return ' ';
        }
        return (position.Latitude > 0.0) ? 'N' : 'S';
    }

    constexpr double longitude() const noexcept {
        return position.Longitude;
    }
    constexpr double longitudeNormalized() const noexcept {
        return std::fabs(position.Longitude);
    }
    constexpr char longitudeDirection() const noexcept {
        if (position.Longitude == 0.0) {
            return ' ';
        }
        return (position.Longitude > 0.0) ? 'E' : 'W';
    }

    constexpr double altitude() const noexcept {
        return position.Altitude;
    }
    constexpr double altitudeMeters() const noexcept {
        return position.Altitude;
    }
    constexpr long altitudeFeet() const noexcept {
        constexpr double feetPerMeter = 3.28084;
        return static_cast<long>(position.Altitude * feetPerMeter);
    }
};
#pragma pack(pop)

using AirportHandler = std::function<void(std::string_view ident, std::string_view region, const AirportDetails& position)>;

#pragma pack(push, 1)
struct WaypointDetails {
    DataTypes::LatLonAlt position;
    float magVar;

    constexpr double latitude() const noexcept {
        return position.Latitude;
    }
    double latitudeNormalized() const noexcept {
        return std::fabs(position.Latitude);
    }
    constexpr char latitudeDirection() const noexcept {
        if (position.Latitude == 0.0) {
            return ' ';
        }
        return (position.Latitude > 0.0) ? 'N' : 'S';
    }

    constexpr double longitude() const noexcept {
        return position.Longitude;
    }
    double longitudeNormalized() const noexcept {
        return std::fabs(position.Longitude);
    }
    constexpr char longitudeDirection() const noexcept {
        if (position.Longitude == 0.0) {
            return ' ';
        }
        return (position.Longitude > 0.0) ? 'E' : 'W';
    }

    constexpr double altitude() const noexcept {
        return position.Altitude;
    }
    constexpr double altitudeMeters() const noexcept {
        return position.Altitude;
    }
    constexpr long altitudeFeet() const noexcept {
        constexpr double feetPerMeter = 3.28084;
        return static_cast<long>(position.Altitude * feetPerMeter);
    }

    constexpr float MagVarNormalized() const noexcept {
        return (magVar > 180.0F) ? (360.0 - magVar) : magVar;
    }
    constexpr char magVarDirection() const noexcept {
        if ((magVar == 0.0F) || (magVar == 180.0F)) {
            return ' ';
        }
        return (magVar < 180.0F) ? 'E' : 'W';
    }
};
#pragma pack(pop)

using WaypointHandler = std::function<void(std::string_view ident, std::string_view region, const WaypointDetails& details)>;

#pragma pack(push, 1)
struct NdbDetails {
    DataTypes::LatLonAlt position;
    float magVar;
    float frequency;

    constexpr double latitude() const noexcept {
        return position.Latitude;
    }
    double latitudeNormalized() const noexcept {
        return std::fabs(position.Latitude);
    }
    constexpr char latitudeDirection() const noexcept {
        if (position.Latitude == 0.0) {
            return ' ';
        }
        return (position.Latitude > 0.0) ? 'N' : 'S';
    }

    constexpr double longitude() const noexcept {
        return position.Longitude;
    }
    double longitudeNormalized() const noexcept {
        return std::fabs(position.Longitude);
    }
    constexpr char longitudeDirection() const noexcept {
        if (position.Longitude == 0.0) {
            return ' ';
        }
        return (position.Longitude > 0.0) ? 'E' : 'W';
    }

    constexpr double altitude() const noexcept {
        return position.Altitude;
    }
    constexpr double altitudeMeters() const noexcept {
        return position.Altitude;
    }
    constexpr long altitudeFeet() const noexcept {
        constexpr double feetPerMeter = 3.28084;
        return static_cast<long>(position.Altitude * feetPerMeter);
    }

    constexpr float MagVarNormalized() const noexcept {
        return (magVar > 180.0F) ? (360.0 - magVar) : magVar;
    }
    constexpr char magVarDirection() const noexcept {
        if ((magVar == 0.0F) || (magVar == 180.0F)) {
            return ' ';
        }
        return (magVar < 180.0F) ? 'E' : 'W';
    }

    inline float frequencyKHz() const noexcept {
        constexpr float kHzFactor = 1'000.0F;
        return frequency / kHzFactor;
    }
};
#pragma pack(pop)

using NdbHandler = std::function<void(std::string_view ident, std::string_view region, const NdbDetails& details)>;

#pragma pack(push, 1)
struct VorDetails {
    DataTypes::LatLonAlt position;
    float magVar;
    float frequency;
    unsigned long flags;
    float localizerCourse;
    DataTypes::LatLonAlt localizerPosition;
    float glideSlopeAngle;

    constexpr double latitude() const noexcept {
        return position.Latitude;
    }
    double latitudeNormalized() const noexcept {
        return std::fabs(position.Latitude);
    }
    constexpr char latitudeDirection() const noexcept {
        if (position.Latitude == 0.0) {
            return ' ';
        }
        return (position.Latitude > 0.0) ? 'N' : 'S';
    }

    constexpr double longitude() const noexcept {
        return position.Longitude;
    }
    double longitudeNormalized() const noexcept {
        return std::fabs(position.Longitude);
    }
    constexpr char longitudeDirection() const noexcept {
        if (position.Longitude == 0.0) {
            return ' ';
        }
        return (position.Longitude > 0.0) ? 'E' : 'W';
    }

    constexpr double altitude() const noexcept {
        return position.Altitude;
    }
    constexpr double altitudeMeters() const noexcept {
        return position.Altitude;
    }
    constexpr long altitudeFeet() const noexcept {
        return static_cast<long>(position.Altitude * Facilities::MetersToFeetFactor);
    }

    constexpr float MagVarNormalized() const noexcept {
        return (magVar > 180.0F) ? (360.0F - magVar) : magVar;
    }
    constexpr char magVarDirection() const noexcept {
        if ((magVar == 0.0F) || (magVar == 180.0F)) {
            return ' ';
        }
        return (magVar < 180.0F) ? 'E' : 'W';
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

    inline float frequencyMHz() const noexcept {
        constexpr float MHzFactor = 1'000'000.0F;
        return frequency / MHzFactor;
    }

    constexpr double localizerLatitude() const noexcept {
        return localizerPosition.Latitude;
    }
    double localizerLatitudeNormalized() const noexcept {
        return std::fabs(localizerPosition.Latitude);
    }
    constexpr char localizerLatitudeDirection() const noexcept {
        if (localizerPosition.Latitude == 0.0) {
            return ' ';
        }
        return (localizerPosition.Latitude > 0.0) ? 'N' : 'S';
    }

    constexpr double localizerLongitude() const noexcept {
        return localizerPosition.Longitude;
    }
    double localizerLongitudeNormalized() const noexcept {
        return std::fabs(localizerPosition.Longitude);
    }
    constexpr char localizerLongitudeDirection() const noexcept {
        if (localizerPosition.Longitude == 0.0) {
            return ' ';
        }
        return (localizerPosition.Longitude > 0.0) ? 'E' : 'W';
    }

    constexpr double localizerAltitude() const noexcept {
        return localizerPosition.Altitude;
    }
    constexpr double localizerAltitudeMeters() const noexcept {
        return localizerPosition.Altitude;
    }
    constexpr long localizerAltitudeFeet() const noexcept {
        constexpr double feetPerMeter = 3.28084;
        return static_cast<long>(localizerPosition.Altitude * feetPerMeter);
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
    FacilityListHandler(simconnect_message_handler_type& handler) : simConnectMessageHandler_(handler)
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
    RequestId correlationId(const Messages::MsgBase& msg) const {
        switch (msg.dwID) {
        case Messages::airportList:
            return reinterpret_cast<const Messages::AirportListMsg&>(msg).dwRequestID;

        case Messages::waypointList:
            return reinterpret_cast<const Messages::WaypointListMsg&>(msg).dwRequestID;

        case Messages::ndbList:
            return reinterpret_cast<const Messages::NdbListMsg&>(msg).dwRequestID;

        case Messages::vorList:
            return reinterpret_cast<const Messages::VorListMsg&>(msg).dwRequestID;

        default:
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

        this->registerHandler(requestId, [handler, onDone](const Messages::MsgBase& msg) {
                const Messages::AirportListMsg& enumMsg = reinterpret_cast<const Messages::AirportListMsg&>(msg);

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