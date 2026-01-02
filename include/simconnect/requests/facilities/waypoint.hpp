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

#include <simconnect/requests/facilities/route.hpp>


#include <cstdint>

#include <map>
#include <array>
#include <vector>
#include <string_view>


namespace SimConnect::Facilities {

#pragma pack(push, 1)

/**
 * Represents the data structure for a VOR beacon, as returned by SimConnect.
 */
struct WaypointData : public LatLonAltMagVar {
    std::array<char, ICAOLength> icao_;     // ICAO
    std::array<char, RegionLength> region_; // REGION
    WaypointType type_;                     // TYPE
    std::int32_t isTerminalWaypoint_;       // IS_TERMINAL_WAYPOINT
    std::int32_t nRoutes_;                  // N_ROUTES


public:
    inline static bool isWaypointData(const Messages::FacilityDataMsg& msg) noexcept {
        return msg.Type == FacilityDataTypes::waypoint;
    }
    inline static const WaypointData& from(const Messages::FacilityDataMsg& msg) {
        return *reinterpret_cast<const WaypointData*>(&msg.Data);
    }

    constexpr std::string_view icao() const noexcept { return { icao_.data(), ICAOLength }; }
    constexpr std::string_view region() const noexcept { return { region_.data(), RegionLength }; }
    constexpr WaypointType type() const noexcept { return type_; }

    constexpr bool isTerminalWaypoint() const noexcept { return isTerminalWaypoint_ != 0; }
    constexpr std::int32_t nRoutes() const noexcept { return nRoutes_; }
};

#pragma pack(pop)


struct WaypointFacility {
    WaypointData data;

    constexpr bool haveRoutes() const noexcept { return !routes.empty(); }
    std::vector<RouteData> routes;
};

/**
 * A Builder class for constructing VOR facility definitions.
 * 
 * @tparam MaxLength The maximum length of the facility definition.
 */
template <std::size_t MaxLength>
struct WaypointBuilder
{
    FacilityDefinition<MaxLength> definition;

    constexpr explicit WaypointBuilder(FacilityDefinition<MaxLength> def) : definition{ def } {}

    constexpr Builder<MaxLength> end() const {
        return Builder<MaxLength>{ definition.push(FacilityField::waypointClose) };
    }

    // Child builders
    constexpr RouteBuilder<MaxLength> route() const {
        return RouteBuilder<MaxLength>{ definition.push(FacilityField::routeOpen) };
    }

    // Field setters
    constexpr WaypointBuilder<MaxLength> latitude() const {
        return WaypointBuilder<MaxLength>{ definition.push(FacilityField::waypointLatitude) };
    }
    constexpr WaypointBuilder<MaxLength> longitude() const {
        return WaypointBuilder<MaxLength>{ definition.push(FacilityField::waypointLongitude) };
    }
    constexpr WaypointBuilder<MaxLength> altitude() const {
        return WaypointBuilder<MaxLength>{ definition.push(FacilityField::waypointAltitude) };
    }
    constexpr WaypointBuilder<MaxLength> magVar() const {
        return WaypointBuilder<MaxLength>{ definition.push(FacilityField::waypointMagvar) };
    }
    constexpr WaypointBuilder<MaxLength> icao() const {
        return WaypointBuilder<MaxLength>{ definition.push(FacilityField::waypointICAO) };
    }
    constexpr WaypointBuilder<MaxLength> region() const {
        return WaypointBuilder<MaxLength>{ definition.push(FacilityField::waypointRegion) };
    }
    constexpr WaypointBuilder<MaxLength> type() const {
        return WaypointBuilder<MaxLength>{ definition.push(FacilityField::waypointType) };
    }
    constexpr WaypointBuilder<MaxLength> isTerminalWaypoint() const {
        return WaypointBuilder<MaxLength>{ definition.push(FacilityField::waypointIsTerminalWpt) };
    }
    constexpr WaypointBuilder<MaxLength> nRoutes() const {
        return WaypointBuilder<MaxLength>{ definition.push(FacilityField::waypointNRoutes) };
    }

    constexpr WaypointBuilder<MaxLength> allFields() const {
        return WaypointBuilder<MaxLength>{
            definition
                .push(FacilityField::waypointLatitude)
                .push(FacilityField::waypointLongitude)
                .push(FacilityField::waypointAltitude)
                .push(FacilityField::waypointMagvar)
                .push(FacilityField::waypointICAO)
                .push(FacilityField::waypointRegion)
                .push(FacilityField::waypointType)
                .push(FacilityField::waypointIsTerminalWpt)
                .push(FacilityField::waypointNRoutes)
        };
    }
    
};
} // namespace SimConnect::Facilities