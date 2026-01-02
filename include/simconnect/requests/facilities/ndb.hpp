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
struct NDBData : public LatLonAltMagVar {
    std::uint32_t frequency_;               // FREQUENCY
    std::array<char, Name64Length> name_;   // NAME
    NDBType type_;                          // TYPE

    float range_;                           // RANGE

    std::int32_t isTerminalNDB_;            // IS_TERMINAL_NDB
    std::int32_t bfoRequired_;              // BFO_REQUIRED

public:
    inline static bool isNDBData(const Messages::FacilityDataMsg& msg) noexcept {
        return msg.Type == FacilityDataTypes::ndb;
    }
    inline static const NDBData& from(const Messages::FacilityDataMsg& msg) {
        return *reinterpret_cast<const NDBData*>(&msg.Data);
    }

    constexpr std::uint32_t frequency() const noexcept { return frequency_; }
    constexpr double frequencyKHz() const noexcept { return frequency_ * FrequencyToKHzFactor; }
    inline std::string_view name() const noexcept { return SimConnect::toString(name_); }
    constexpr NDBType type() const noexcept { return type_; }

    constexpr float range() const noexcept { return range_; }
    constexpr double rangeNM() const noexcept { return static_cast<double>(range_ / 1852.0); }

    constexpr bool isTerminalNDB() const noexcept { return isTerminalNDB_ != 0; }
    constexpr bool bfoRequired() const noexcept { return bfoRequired_ != 0; }
};

#pragma pack(pop)


/**
 * A Builder class for constructing VOR facility definitions.
 * 
 * @tparam MaxLength The maximum length of the facility definition.
 */
template <std::size_t MaxLength>
struct NDBBuilder
{
    FacilityDefinition<MaxLength> definition;

    constexpr explicit NDBBuilder(FacilityDefinition<MaxLength> def) : definition{ def } {}

    constexpr Builder<MaxLength> end() const {
        return Builder<MaxLength>{ definition.push(FacilityField::ndbClose) };
    }

    // Field setters
    constexpr NDBBuilder<MaxLength> latitude() const {
        return NDBBuilder<MaxLength>{ definition.push(FacilityField::ndbLatitude) };
    }
    constexpr NDBBuilder<MaxLength> longitude() const {
        return NDBBuilder<MaxLength>{ definition.push(FacilityField::ndbLongitude) };
    }
    constexpr NDBBuilder<MaxLength> altitude() const {
        return NDBBuilder<MaxLength>{ definition.push(FacilityField::ndbAltitude) };
    }
    constexpr NDBBuilder<MaxLength> magVar() const {
        return NDBBuilder<MaxLength>{ definition.push(FacilityField::ndbMagvar) };
    }
    constexpr NDBBuilder<MaxLength> frequency() const {
        return NDBBuilder<MaxLength>{ definition.push(FacilityField::ndbFrequency) };
    }
    constexpr NDBBuilder<MaxLength> name() const {
        return NDBBuilder<MaxLength>{ definition.push(FacilityField::ndbName) };
    }
    constexpr NDBBuilder<MaxLength> type() const {
        return NDBBuilder<MaxLength>{ definition.push(FacilityField::ndbType) };
    }
    constexpr NDBBuilder<MaxLength> range() const {
        return NDBBuilder<MaxLength>{ definition.push(FacilityField::ndbRange) };
    }
    constexpr NDBBuilder<MaxLength> isTerminalNDB() const {
        return NDBBuilder<MaxLength>{ definition.push(FacilityField::ndbIsTerminalNDB) };
    }
    constexpr NDBBuilder<MaxLength> isBFORequired() const {
        return NDBBuilder<MaxLength>{ definition.push(FacilityField::ndbBfoRequired) };
    }

    constexpr NDBBuilder<MaxLength> allFields() const {
        return NDBBuilder<MaxLength>{
            definition
                .push(FacilityField::ndbLatitude)
                .push(FacilityField::ndbLongitude)
                .push(FacilityField::ndbAltitude)
                .push(FacilityField::ndbMagvar)
                .push(FacilityField::ndbFrequency)
                .push(FacilityField::ndbName)
                .push(FacilityField::ndbType)
                .push(FacilityField::ndbRange)
                .push(FacilityField::ndbIsTerminalNDB)
                .push(FacilityField::ndbBfoRequired)
        };
    }
    
};
} // namespace SimConnect::Facilities