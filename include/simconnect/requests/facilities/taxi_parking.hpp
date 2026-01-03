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

#include <simconnect.hpp>
#include <simconnect/simconnect.hpp>
#include <simconnect/requests/facilities/facility_definition.hpp>
#include <simconnect/requests/facilities/facility_definition_builder.hpp>


#include <cmath>
#include <cstdint>

#include <numbers>
#include <set>
#include <array>
#include <string>
#include <string_view>


namespace SimConnect::Facilities {

#pragma pack(push, 1)

class TaxiParkingData {
    TaxiParkingType type_;          // TYPE
    TaxiPointType taxiPointType_;   // TAXI_POINT_TYPE
    ParkingName name_;              // NAME
    ParkingName suffix_;            // SUFFIX
    int32_t number_;                // NUMBER
    ParkingOrientation orientation_;             // ORIENTATION
    float heading_;                 // HEADING
    float radius_;                  // RADIUS
    float biasX_;                  // BIAS_X
    float biasZ_;                  // BIAS_Z
#if MSFS_2024_SDK
    int32_t nAirlines_;            // N_AIRLINES
#endif

    static constexpr std::array<std::string_view, 38> parkingNameStrings_ = {
        "", "Parking", "N Parking", "NE Parking", "E Parking", "SE Parking",
        "S Parking", "SW Parking", "W Parking", "NW Parking", "Gate", "Dock",
        "Gate A", "Gate B", "Gate C", "Gate D", "Gate E", "Gate F", "Gate G",
        "Gate H", "Gate I", "Gate J", "Gate K", "Gate L", "Gate M", "Gate N",
        "Gate O", "Gate P", "Gate Q", "Gate R", "Gate S", "Gate T", "Gate U",
        "Gate V", "Gate W", "Gate X", "Gate Y", "Gate Z"
    };

public:
    inline static bool isTaxiParkingData(const Messages::FacilityDataMsg& msg) noexcept {
        return msg.Type == FacilityDataTypes::taxiParking;
    }
    inline static const TaxiParkingData& from(const Messages::FacilityDataMsg& msg) {
        return *reinterpret_cast<const TaxiParkingData*>(&msg.Data);
    }
    static constexpr std::string_view parkingNameToString(ParkingName name) noexcept {
        const auto index = static_cast<std::size_t>(name);
        return (index < parkingNameStrings_.size()) ? parkingNameStrings_[index] : "";
    }

    inline std::string formatParkingName() const {
        std::string result{};
        if (name_ != ParkingName::None) {
            result = parkingNameToString(name_);
        }
        if (number_ > 0) {
            if (!result.empty()) { result += " "; }
            result += std::to_string(number_);
        }
        if (suffix_ != ParkingName::None) {
            if (!result.empty()) { result += " "; }
            result += static_cast<char>('A' + (static_cast<unsigned>(suffix_) - 1));
        }
        return result;
    }

    constexpr TaxiParkingType type() const noexcept { return type_; }
    constexpr TaxiPointType taxiPointType() const noexcept { return taxiPointType_; }
    constexpr ParkingName name() const noexcept { return name_; }
    constexpr ParkingName suffix() const noexcept { return suffix_; }
    constexpr int32_t number() const noexcept { return number_; }

    constexpr ParkingOrientation orientation() const noexcept { return orientation_; }
    constexpr bool isOrientationForward() const noexcept { return orientation_ == ParkingOrientation::Forward; }
    constexpr bool isOrientationReverse() const noexcept { return orientation_ == ParkingOrientation::Reverse; }

    constexpr float heading() const noexcept { return heading_; }
    constexpr float radius() const noexcept { return radius_; }

    constexpr float biasX() const noexcept { return biasX_; }
    constexpr double latitude(double airportLatitude, [[maybe_unused]] double airportLongitude) const noexcept {
        // Calculate latitude based on biasX and airport coordinates
        constexpr double earthRadius = 6378137.0; // in meters

        double deltaLat = (biasX_ / earthRadius) * (180.0 / std::numbers::pi);
        return airportLatitude + deltaLat;
    }
    constexpr float biasZ() const noexcept { return biasZ_; }
    inline double longitude(double airportLatitude, double airportLongitude) const noexcept {
        // Calculate longitude based on biasZ and airport coordinates
        constexpr double earthRadius = 6378137.0; // in meters

        double radiusAtLatitude = earthRadius * std::cos(airportLatitude * (std::numbers::pi / 180.0));
        double deltaLon = (biasZ_ / radiusAtLatitude) * (180.0 / std::numbers::pi);
        return airportLongitude + deltaLon;
    }

#if MSFS_2024_SDK
    constexpr int32_t nAirlines() const noexcept { return nAirlines_; }
#endif
};
#pragma pack(pop)


/**
 * A version of the TaxiParkingData structure that includes (optionally) child data.
 */
struct TaxiParkingFacility {
    TaxiParkingData data;

    inline bool haveAirlines() const noexcept { return !airlines.empty(); }
    std::set<std::string> airlines{};
};


/**
 * Builder for TaxiParking facility definitions.
 * 
 * @tparam MaxLength The maximum length of the facility definition.
 */
template <std::size_t MaxLength>
struct TaxiParkingBuilder
{
    FacilityDefinition<MaxLength> definition;

    constexpr explicit TaxiParkingBuilder(FacilityDefinition<MaxLength> def) : definition{ def } {}

    constexpr AirportBuilder<MaxLength> end() const {
        return AirportBuilder<MaxLength>{ definition.push(FacilityField::taxiParkingClose) };
    }

    // Children builders
    constexpr AirlineBuilder<MaxLength> airline() const {
        return AirlineBuilder<MaxLength>{ definition.push(FacilityField::airlineOpen) };
    }

    // Field setters
    constexpr TaxiParkingBuilder<MaxLength> type() const {
        return TaxiParkingBuilder<MaxLength>{ definition.push(FacilityField::taxiParkingType) };
    }
    constexpr TaxiParkingBuilder<MaxLength> taxiPointType() const {
        return TaxiParkingBuilder<MaxLength>{ definition.push(FacilityField::taxiParkingTaxiPointType) };
    }
    constexpr TaxiParkingBuilder<MaxLength> name() const {
        return TaxiParkingBuilder<MaxLength>{ definition.push(FacilityField::taxiParkingName) };
    }
    constexpr TaxiParkingBuilder<MaxLength> suffix() const {
        return TaxiParkingBuilder<MaxLength>{ definition.push(FacilityField::taxiParkingSuffix) };
    }
    constexpr TaxiParkingBuilder<MaxLength> number() const {
        return TaxiParkingBuilder<MaxLength>{ definition.push(FacilityField::taxiParkingNumber) };
    }
    constexpr TaxiParkingBuilder<MaxLength> orientation() const {
        return TaxiParkingBuilder<MaxLength>{ definition.push(FacilityField::taxiParkingOrientation) };
    }
    constexpr TaxiParkingBuilder<MaxLength> heading() const {
        return TaxiParkingBuilder<MaxLength>{ definition.push(FacilityField::taxiParkingHeading) };
    }
    constexpr TaxiParkingBuilder<MaxLength> radius() const {
        return TaxiParkingBuilder<MaxLength>{ definition.push(FacilityField::taxiParkingRadius) };
    }
    constexpr TaxiParkingBuilder<MaxLength> biasX() const {
        return TaxiParkingBuilder<MaxLength>{ definition.push(FacilityField::taxiParkingBiasX) };
    }
    constexpr TaxiParkingBuilder<MaxLength> biasZ() const {
        return TaxiParkingBuilder<MaxLength>{ definition.push(FacilityField::taxiParkingBiasZ) };
    }
#if MSFS_2024_SDK
    constexpr TaxiParkingBuilder<MaxLength> nAirlines() const {
        return TaxiParkingBuilder<MaxLength>{ definition.push(FacilityField::taxiParkingNAirlines) };
    }
#endif

    constexpr TaxiParkingBuilder<MaxLength> allFields() const {
        return TaxiParkingBuilder<MaxLength>{
            definition
                .push(FacilityField::taxiParkingType)
                .push(FacilityField::taxiParkingTaxiPointType)
                .push(FacilityField::taxiParkingName)
                .push(FacilityField::taxiParkingSuffix)
                .push(FacilityField::taxiParkingNumber)
                .push(FacilityField::taxiParkingOrientation)
                .push(FacilityField::taxiParkingHeading)
                .push(FacilityField::taxiParkingRadius)
                .push(FacilityField::taxiParkingBiasX)
                .push(FacilityField::taxiParkingBiasZ)
#if MSFS_2024_SDK
                .push(FacilityField::taxiParkingNAirlines)
#endif
        };
    }
};

} // namespace SimConnect::Facilities