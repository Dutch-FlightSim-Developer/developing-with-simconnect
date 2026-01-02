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
#include <simconnect/simconnect_datatypes.hpp>

#include <simconnect/requests/facilities/facility_definition.hpp>
#include <simconnect/requests/facilities/facility_definition_builder.hpp>


#include <cstdint>

#include <map>
#include <array>
#include <string_view>


namespace SimConnect::Facilities {

#pragma pack(push, 1)

/**
 * Represents the data structure for a VOR beacon, as returned by SimConnect.
 */
struct RouteData {
    RouteType type_;                     // TYPE

    std::array<char, ICAOLength> nextIcao_;     // NEXT_ICAO
    std::array<char, RegionLength> nextRegion_; // NEXT_REGION
    RouteWaypointType nextWaypointType_;       // NEXT_TYPE
    LatLonAlt nextPosition_;               // NEXT_POSITION

    std::array<char, ICAOLength> prevIcao_;     // PREV_ICAO
    std::array<char, RegionLength> prevRegion_; // PREV_REGION
    RouteWaypointType prevWaypointType_;       // PREV_TYPE
    LatLonAlt prevPosition_;               // PREV_POSITION

public:
    inline static bool isRouteData(const Messages::FacilityDataMsg& msg) noexcept {
        return msg.Type == FacilityDataTypes::route;
    }
    inline static const RouteData& from(const Messages::FacilityDataMsg& msg) {
        return *reinterpret_cast<const RouteData*>(&msg.Data);
    }

    constexpr RouteType type() const noexcept { return type_; }

    constexpr std::string_view nextIcao() const noexcept { return { nextIcao_.data(), ICAOLength }; }
    constexpr std::string_view nextRegion() const noexcept { return { nextRegion_.data(), RegionLength }; }
    constexpr RouteWaypointType nextWaypointType() const noexcept { return nextWaypointType_; }
    constexpr const LatLonAlt& nextPosition() const noexcept { return nextPosition_; }

    constexpr std::string_view prevIcao() const noexcept { return { prevIcao_.data(), ICAOLength }; }
    constexpr std::string_view prevRegion() const noexcept { return { prevRegion_.data(), RegionLength }; }
    constexpr RouteWaypointType prevWaypointType() const noexcept { return prevWaypointType_; }
    constexpr const LatLonAlt& prevPosition() const noexcept { return prevPosition_; }
};

#pragma pack(pop)


/**
 * A Builder class for constructing VOR facility definitions.
 * 
 * @tparam MaxLength The maximum length of the facility definition.
 */
template <std::size_t MaxLength>
struct RouteBuilder
{
    FacilityDefinition<MaxLength> definition;

    constexpr explicit RouteBuilder(FacilityDefinition<MaxLength> def) : definition{ def } {}

    constexpr WaypointBuilder<MaxLength> end() const {
        return WaypointBuilder<MaxLength>{ definition.push(FacilityField::routeClose) };
    }

    // Field setters
    constexpr RouteBuilder<MaxLength> type() const {
        return RouteBuilder<MaxLength>{ definition.push(FacilityField::routeType) };
    }
    constexpr RouteBuilder<MaxLength> nextICAO() const {
        return RouteBuilder<MaxLength>{ definition.push(FacilityField::routeNextICAO) };
    }
    constexpr RouteBuilder<MaxLength> nextRegion() const {
        return RouteBuilder<MaxLength>{ definition.push(FacilityField::routeNextRegion) };
    }
    constexpr RouteBuilder<MaxLength> nextType() const {
        return RouteBuilder<MaxLength>{ definition.push(FacilityField::routeNextType) };
    }
    constexpr RouteBuilder<MaxLength> nextPosition() const {
        return RouteBuilder<MaxLength>{ definition
            .push(FacilityField::routeNextLatitude)
            .push(FacilityField::routeNextLongitude)
            .push(FacilityField::routeNextAltitude)
        };
    }
    constexpr RouteBuilder<MaxLength> prevICAO() const {
        return RouteBuilder<MaxLength>{ definition.push(FacilityField::routePrevICAO) };
    }
    constexpr RouteBuilder<MaxLength> prevRegion() const {
        return RouteBuilder<MaxLength>{ definition.push(FacilityField::routePrevRegion) };
    }
    constexpr RouteBuilder<MaxLength> prevType() const {
        return RouteBuilder<MaxLength>{ definition.push(FacilityField::routePrevType) };
    }
    constexpr RouteBuilder<MaxLength> prevPosition() const {
        return RouteBuilder<MaxLength>{ definition
            .push(FacilityField::routePrevLatitude)
            .push(FacilityField::routePrevLongitude)
            .push(FacilityField::routePrevAltitude)
        };
    }

    constexpr RouteBuilder<MaxLength> allFields() const {
        return RouteBuilder<MaxLength>{
            definition
                .push(FacilityField::routeType)
                .push(FacilityField::routeNextICAO)
                .push(FacilityField::routeNextRegion)
                .push(FacilityField::routeNextType)
                .push(FacilityField::routeNextLatitude)
                .push(FacilityField::routeNextLongitude)
                .push(FacilityField::routeNextAltitude)
                .push(FacilityField::routePrevICAO)
                .push(FacilityField::routePrevRegion)
                .push(FacilityField::routePrevType)
                .push(FacilityField::routePrevLatitude)
                .push(FacilityField::routePrevLongitude)
                .push(FacilityField::routePrevAltitude)
        };
    }
    
};
} // namespace SimConnect::Facilities