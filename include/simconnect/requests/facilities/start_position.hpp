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
 #include <simconnect/requests/facilities/facility_definition.hpp>
 #include <simconnect/requests/facilities/facility_definition_builder.hpp>

 #include <cstdint>

 #include <array>
 #include <string_view>


namespace SimConnect::Facilities {

#pragma pack(push, 1)


class StartData {
    StartType type_;                // TYPE
    LatLonAlt position_;            // LATITUDE, LONGITUDE, ALTITUDE
    float heading_;                 // HEADING
    RunwayNumber number_;           // NUMBER (only valid for StartType::Runway)
    RunwayDesignator designator_;   // DESIGNATOR (only valid for StartType::Runway)

    static constexpr std::array<std::string_view, 5> StartTypeNames = {
        "None", "Runway", "Water", "Helipad", "Track"
    };
    static constexpr std::array<std::string_view, 46> RunwayNumberNames = {
        "",
        "01", "02", "03", "04", "05", "06", "07", "08", "09", "10",
        "11", "12", "13", "14", "15", "16", "17", "18", "19", "20",
        "21", "22", "23", "24", "25", "26", "27", "28", "29", "30",
        "31", "32", "33", "34", "35", "36",
        "North", "NorthEast", "East", "SouthEast", "South", "SouthWest", "West", "NorthWest",
        "Last"
    };
    static constexpr std::array<std::string_view, 8> RunwayDesignatorNames = {
        "",
        "L", "R", "C", "Water", "A", "B",
        "Last"
    };

public:
    inline static bool isStartData(const Messages::FacilityDataMsg& msg) noexcept {
        return msg.Type == FacilityDataTypes::start;
    }
    inline static const StartData& from(const Messages::FacilityDataMsg& msg) {
        return *reinterpret_cast<const StartData*>(&msg.Data);
    }

    constexpr StartType type() const noexcept { return type_; }
    constexpr std::string_view startType() const noexcept {
        const auto index = static_cast<std::size_t>(type_);
        return (index < StartTypeNames.size()) ? StartTypeNames[index] : "Invalid";
    }

    constexpr const LatLonAlt& position() const noexcept { return position_; }

    constexpr float heading() const noexcept { return heading_; }

    constexpr RunwayNumber numberValue() const noexcept { return number_; }
    constexpr std::string_view number() const noexcept {
        const auto index = static_cast<std::size_t>(number_);
        return (index < RunwayNumberNames.size()) ? RunwayNumberNames[index] : "Invalid";
    }

    constexpr RunwayDesignator designatorValue() const noexcept { return designator_; }
    constexpr std::string_view designator() const noexcept {
        const auto index = static_cast<std::size_t>(designator_);
        return (index < RunwayDesignatorNames.size()) ? RunwayDesignatorNames[index] : "Invalid";
    }
};


#pragma pack(pop)


/**
 * Builder for Start facility definitions.
 *
 * @tparam MaxLength The maximum length of the facility definition.
 */
template <std::size_t MaxLength>
struct StartBuilder
{
    FacilityDefinition<MaxLength> definition;

    constexpr explicit StartBuilder(FacilityDefinition<MaxLength> def) : definition{ def } {}

    constexpr AirportBuilder<MaxLength> end() const {
        return AirportBuilder<MaxLength>{ definition.push(FacilityField::startClose) };
    }

    // Field setters
    constexpr StartBuilder<MaxLength> type() const {
        return StartBuilder<MaxLength>{ definition.push(FacilityField::startType) };
    }
    constexpr StartBuilder<MaxLength> latitude() const {
        return StartBuilder<MaxLength>{ definition.push(FacilityField::startLatitude) };
    }
    constexpr StartBuilder<MaxLength> longitude() const {
        return StartBuilder<MaxLength>{ definition.push(FacilityField::startLongitude) };
    }
    constexpr StartBuilder<MaxLength> altitude() const {
        return StartBuilder<MaxLength>{ definition.push(FacilityField::startAltitude) };
    }
    constexpr StartBuilder<MaxLength> heading() const {
        return StartBuilder<MaxLength>{ definition.push(FacilityField::startHeading) };
    }
    constexpr StartBuilder<MaxLength> number() const {
        return StartBuilder<MaxLength>{ definition.push(FacilityField::startNumber) };
    }
    constexpr StartBuilder<MaxLength> designator() const {
        return StartBuilder<MaxLength>{ definition.push(FacilityField::startDesignator) };
    }

    constexpr StartBuilder<MaxLength> allFields() const {
        return StartBuilder<MaxLength>{
            definition
                .push(FacilityField::startType)
                .push(FacilityField::startLatitude)
                .push(FacilityField::startLongitude)
                .push(FacilityField::startAltitude)
                .push(FacilityField::startHeading)
                .push(FacilityField::startNumber)
                .push(FacilityField::startDesignator)
        };
    }
};

} // namespace SimConnect::Facilities