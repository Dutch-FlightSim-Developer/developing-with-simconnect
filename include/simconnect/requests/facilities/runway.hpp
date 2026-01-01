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


class PavementData {
    float length_;      // LENGTH
    float width_;       // WIDTH
    int32_t enable_;     // ENABLE

public:
    constexpr float length() const noexcept { return length_; }
    constexpr float width() const noexcept { return width_; }
    constexpr bool isEnabled() const noexcept { return enable_ != 0; }
};


class ApproachLightsData {
    ApproachLightsSystem system_;          // SYSTEM
    int32_t strobeCount_;    // STROBE_COUNT
    int32_t hasEndLights_;   // HAS_END_LIGHTS
    int32_t hasREILLights_;  // HAS_REIL_LIGHTS
    int32_t hasTouchdownLights_; // HAS_TOUCHDOWN_LIGHTS
    int32_t onGround_;       // ON_GROUND
    int32_t enable_;         // ENABLE
    float offset_;           // OFFSET
    float spacing_;          // SPACING
    float slope_;            // SLOPE

public:
    constexpr ApproachLightsSystem system() const noexcept { return system_; }
    constexpr int32_t strobeCount() const noexcept { return strobeCount_; }
    constexpr bool hasEndLights() const noexcept { return hasEndLights_ != 0; }
    constexpr bool hasREILLights() const noexcept { return hasREILLights_ != 0; }
    constexpr bool hasTouchdownLights() const noexcept { return hasTouchdownLights_ != 0; }
    constexpr bool onGround() const noexcept { return onGround_ != 0; }
    constexpr bool isEnabled() const noexcept { return enable_ != 0; }
    constexpr float offset() const noexcept { return offset_; }
    constexpr float spacing() const noexcept { return spacing_; }
    constexpr float slope() const noexcept { return slope_; }
};


class VASIData {
    VASIType type_;       // TYPE
    float biasX_;      // BIAS_X
    float biasZ_;      // BIAS_Z
    float spacing_;    // SPACING
    float angle_;      // ANGLE

public:
    constexpr VASIType type() const noexcept { return type_; }
    constexpr float biasX() const noexcept { return biasX_; }
    constexpr float biasZ() const noexcept { return biasZ_; }
    constexpr float spacing() const noexcept { return spacing_; }
    constexpr float angle() const noexcept { return angle_; }
};


class RunwayData {
    double latitude_;       // LATITUDE
    double longitude_;      // LONGITUDE
    double altitude_;       // ALTITUDE
    float heading_;         // HEADING
    float length_;          // LENGTH
    float width_;           // WIDTH
    float patternAltitude_;   // PATTERN_ALTITUDE
    float slope_;           // SLOPE
    float trueSlope_;       // TRUE_SLOPE
    RunwaySurface surface_;   // SURFACE

    int8_t edgeLights_;        // EDGE_LIGHTS
    int8_t centerLights_;      // CENTER_LIGHTS

    // Primary runway data
    std::array<char, ICAOLength> primaryIlsIcao_; // PRIMARY_ILS_ICAO
    std::array<char, RegionLength> primaryIlsRegion_; // PRIMARY_ILS_REGION

    int8_t primaryClosed_;     // PRIMARY_CLOSED
    int8_t primaryTakeoff_;    // PRIMARY_TAKEOFF
    int8_t primaryLanding_;    // PRIMARY_LANDING

    IlsType primaryIlsType_; // PRIMARY_ILS_TYPE
    RunwayNumber primaryNumber_; // PRIMARY_NUMBER
    RunwayDesignator primaryDesignator_; // PRIMARY_DESIGNATOR
    PavementData primaryThreshold_; // PRIMARY_THRESHOLD
    PavementData primaryBlastpad_; // PRIMARY_BLASTPAD
    PavementData primaryOverrun_; // PRIMARY_OVERRUN
    ApproachLightsData primaryApproachLights_; // PRIMARY_APPROACH_LIGHTS
    VASIData primaryLeftVasi_; // PRIMARY_LEFT_VASI
    VASIData primaryRightVasi_; // PRIMARY_RIGHT_VASI

    std::array<char, ICAOLength> secondaryIlsIcao_; // SECONDARY_ILS_ICAO
    std::array<char, RegionLength> secondaryIlsRegion_; // SECONDARY_ILS_REGION

    int8_t secondaryClosed_;   // SECONDARY_CLOSED
    int8_t secondaryTakeoff_;  // SECONDARY_TAKEOFF
    int8_t secondaryLanding_;  // SECONDARY_LANDING

    IlsType secondaryIlsType_; // SECONDARY_ILS_TYPE
    RunwayNumber secondaryNumber_; // SECONDARY_NUMBER
    RunwayDesignator secondaryDesignator_; // SECONDARY_DESIGNATOR
    PavementData secondaryThreshold_; // SECONDARY_THRESHOLD
    PavementData secondaryBlastpad_; // SECONDARY_BLASTPAD
    PavementData secondaryOverrun_; // SECONDARY_OVERRUN
    ApproachLightsData secondaryApproachLights_; // SECONDARY_APPROACH_LIGHTS
    VASIData secondaryLeftVasi_; // SECONDARY_LEFT_VASI
    VASIData secondaryRightVasi_; // SECONDARY_RIGHT_VASI

public:
    constexpr double latitude() const noexcept { return latitude_; }
    constexpr double longitude() const noexcept { return longitude_; }
    constexpr double altitude() const noexcept { return altitude_; }
    constexpr float heading() const noexcept { return heading_; }
    constexpr float length() const noexcept { return length_; }
    constexpr float width() const noexcept { return width_; }
    constexpr float patternAltitude() const noexcept { return patternAltitude_; }
    constexpr float slope() const noexcept { return slope_; }
    constexpr float trueSlope() const noexcept { return trueSlope_; }
    constexpr RunwaySurface surface() const noexcept { return surface_; }

    constexpr bool hasEdgeLights() const noexcept { return edgeLights_ != 0; }
    constexpr bool hasCenterLights() const noexcept { return centerLights_ != 0; }

    // Primary runway data getters
    constexpr std::string_view primaryIlsIcao() const noexcept { return { primaryIlsIcao_.data(), ICAOLength }; }
    constexpr std::string_view primaryIlsRegion() const noexcept { return { primaryIlsRegion_.data(), RegionLength }; }

    constexpr bool isPrimaryClosed() const noexcept { return primaryClosed_ != 0; }
    constexpr bool isPrimaryTakeoffAllowed() const noexcept { return primaryTakeoff_ != 0; }
    constexpr bool isPrimaryLandingAllowed() const noexcept { return primaryLanding_ != 0; }

    constexpr IlsType primaryIlsType() const noexcept { return primaryIlsType_; }
    constexpr RunwayNumber primaryNumber() const noexcept { return primaryNumber_; }
    constexpr RunwayDesignator primaryDesignator() const noexcept { return primaryDesignator_; }
    constexpr const PavementData& primaryThreshold() const noexcept { return primaryThreshold_; }
    constexpr const PavementData& primaryBlastpad() const noexcept { return primaryBlastpad_; }
    constexpr const PavementData& primaryOverrun() const noexcept { return primaryOverrun_; }
    constexpr const ApproachLightsData& primaryApproachLights() const noexcept { return primaryApproachLights_; }
    constexpr const VASIData& primaryLeftVasi() const noexcept { return primaryLeftVasi_; }
    constexpr const VASIData& primaryRightVasi() const noexcept { return primaryRightVasi_; }

    // Secondary runway data getters
    constexpr std::string_view secondaryIlsIcao() const noexcept { return { secondaryIlsIcao_.data(), ICAOLength }; }
    constexpr std::string_view secondaryIlsRegion() const noexcept { return { secondaryIlsRegion_.data(), RegionLength }; }

    constexpr bool isSecondaryClosed() const noexcept { return secondaryClosed_ != 0; }
    constexpr bool isSecondaryTakeoffAllowed() const noexcept { return secondaryTakeoff_ != 0; }
    constexpr bool isSecondaryLandingAllowed() const noexcept { return secondaryLanding_ != 0; }

    constexpr IlsType secondaryIlsType() const noexcept { return secondaryIlsType_; }
    constexpr RunwayNumber secondaryNumber() const noexcept { return secondaryNumber_; }
    constexpr RunwayDesignator secondaryDesignator() const noexcept { return secondaryDesignator_; }
    constexpr const PavementData& secondaryThreshold() const noexcept { return secondaryThreshold_; }
    constexpr const PavementData& secondaryBlastpad() const noexcept { return secondaryBlastpad_; }
    constexpr const PavementData& secondaryOverrun() const noexcept { return secondaryOverrun_; }
    constexpr const ApproachLightsData& secondaryApproachLights() const noexcept { return secondaryApproachLights_; }
    constexpr const VASIData& secondaryLeftVasi() const noexcept { return secondaryLeftVasi_; }
    constexpr const VASIData& secondaryRightVasi() const noexcept { return secondaryRightVasi_; }
};

#pragma pack(pop)

template <std::size_t MaxLength = 256>
struct RunwayBuilder
{
    FacilityDefinition<MaxLength> definition;

    constexpr explicit RunwayBuilder(FacilityDefinition<MaxLength> def) : definition{ def } {}

    constexpr AirportBuilder<MaxLength> end() const {
        return AirportBuilder<MaxLength>{ definition.push(FacilityField::runwayClose) };
    }

    // Children builders
    constexpr PavementBuilder<MaxLength> primaryThreshold() const {
        return PavementBuilder<MaxLength>{ definition.push(FacilityField::pavementOpen) };
    }
    constexpr PavementBuilder<MaxLength> primaryBlastpad() const {
        return PavementBuilder<MaxLength>{ definition.push(FacilityField::pavementOpen) };
    }
    constexpr PavementBuilder<MaxLength> primaryOverrun() const {
        return PavementBuilder<MaxLength>{ definition.push(FacilityField::pavementOpen) };
    }
    constexpr ApproachLightsBuilder<MaxLength> primaryApproachLights() const {
        return ApproachLightsBuilder<MaxLength>{ definition.push(FacilityField::approachLightsOpen) };
    }
    constexpr VasiBuilder<MaxLength> primaryLeftVasi() const {
        return VasiBuilder<MaxLength>{ definition.push(FacilityField::vasiOpen) };
    }
    constexpr VasiBuilder<MaxLength> primaryRightVasi() const {
        return VasiBuilder<MaxLength>{ definition.push(FacilityField::vasiOpen) };
    }
    constexpr PavementBuilder<MaxLength> secondaryThreshold() const {
        return PavementBuilder<MaxLength>{ definition.push(FacilityField::pavementOpen) };
    }
    constexpr PavementBuilder<MaxLength> secondaryBlastpad() const {
        return PavementBuilder<MaxLength>{ definition.push(FacilityField::pavementOpen) };
    }
    constexpr PavementBuilder<MaxLength> secondaryOverrun() const {
        return PavementBuilder<MaxLength>{ definition.push(FacilityField::pavementOpen) };
    }
    constexpr ApproachLightsBuilder<MaxLength> secondaryApproachLights() const {
        return ApproachLightsBuilder<MaxLength>{ definition.push(FacilityField::approachLightsOpen) };
    }
    constexpr VasiBuilder<MaxLength> secondaryLeftVasi() const {
        return VasiBuilder<MaxLength>{ definition.push(FacilityField::vasiOpen) };
    }
    constexpr VasiBuilder<MaxLength> secondaryRightVasi() const {
        return VasiBuilder<MaxLength>{ definition.push(FacilityField::vasiOpen) };
    }

    // Field setters
    constexpr RunwayBuilder<MaxLength> latitude() const {
        return RunwayBuilder<MaxLength>{ definition.push(FacilityField::runwayLatitude) };
    }
    constexpr RunwayBuilder<MaxLength> longitude() const {
        return RunwayBuilder<MaxLength>{ definition.push(FacilityField::runwayLongitude) };
    }
    constexpr RunwayBuilder<MaxLength> altitude() const {
        return RunwayBuilder<MaxLength>{ definition.push(FacilityField::runwayAltitude) };
    }
    constexpr RunwayBuilder<MaxLength> heading() const {
        return RunwayBuilder<MaxLength>{ definition.push(FacilityField::runwayHeading) };
    }
    constexpr RunwayBuilder<MaxLength> length() const {
        return RunwayBuilder<MaxLength>{ definition.push(FacilityField::runwayLength) };
    }
    constexpr RunwayBuilder<MaxLength> width() const {
        return RunwayBuilder<MaxLength>{ definition.push(FacilityField::runwayWidth) };
    }
    constexpr RunwayBuilder<MaxLength> patternAltitude() const {
        return RunwayBuilder<MaxLength>{ definition.push(FacilityField::runwayPatternAltitude) };
    }
    constexpr RunwayBuilder<MaxLength> slope() const {
        return RunwayBuilder<MaxLength>{ definition.push(FacilityField::runwaySlope) };
    }
    constexpr RunwayBuilder<MaxLength> trueSlope() const {
        return RunwayBuilder<MaxLength>{ definition.push(FacilityField::runwayTrueSlope) };
    }
    constexpr RunwayBuilder<MaxLength> surface() const {
        return RunwayBuilder<MaxLength>{ definition.push(FacilityField::runwaySurface) };
    }
    constexpr RunwayBuilder<MaxLength> primaryIlsIcao() const {
        return RunwayBuilder<MaxLength>{ definition.push(FacilityField::runwayPrimaryILSICAO) };
    }
    constexpr RunwayBuilder<MaxLength> primaryIlsRegion() const {
        return RunwayBuilder<MaxLength>{ definition.push(FacilityField::runwayPrimaryILSRegion) };
    }
    constexpr RunwayBuilder<MaxLength> primaryIlsType() const {
        return RunwayBuilder<MaxLength>{ definition.push(FacilityField::runwayPrimaryILSType) };
    }
    constexpr RunwayBuilder<MaxLength> primaryNumber() const {
        return RunwayBuilder<MaxLength>{ definition.push(FacilityField::runwayPrimaryNumber) };
    }
    constexpr RunwayBuilder<MaxLength> primaryDesignator() const {
        return RunwayBuilder<MaxLength>{ definition.push(FacilityField::runwayPrimaryDesignator) };
    }
    constexpr RunwayBuilder<MaxLength> secondaryIlsIcao() const {
        return RunwayBuilder<MaxLength>{ definition.push(FacilityField::runwaySecondaryILSICAO) };
    }
    constexpr RunwayBuilder<MaxLength> secondaryIlsRegion() const {
        return RunwayBuilder<MaxLength>{ definition.push(FacilityField::runwaySecondaryILSRegion) };
    }
    constexpr RunwayBuilder<MaxLength> secondaryIlsType() const {
        return RunwayBuilder<MaxLength>{ definition.push(FacilityField::runwaySecondaryILSType) };
    }
    constexpr RunwayBuilder<MaxLength> secondaryNumber() const {
        return RunwayBuilder<MaxLength>{ definition.push(FacilityField::runwaySecondaryNumber) };
    }
    constexpr RunwayBuilder<MaxLength> secondaryDesignator() const {
        return RunwayBuilder<MaxLength>{ definition.push(FacilityField::runwaySecondaryDesignator) };
    }
    constexpr RunwayBuilder<MaxLength> edgeLights() const {
        return RunwayBuilder<MaxLength>{ definition.push(FacilityField::runwayEdgeLights) };
    }
    constexpr RunwayBuilder<MaxLength> centerLights() const {
        return RunwayBuilder<MaxLength>{ definition.push(FacilityField::runwayCenterLights) };
    }
    constexpr RunwayBuilder<MaxLength> primaryClosed() const {
        return RunwayBuilder<MaxLength>{ definition.push(FacilityField::runwayPrimaryClosed) };
    }
    constexpr RunwayBuilder<MaxLength> secondaryClosed() const {
        return RunwayBuilder<MaxLength>{ definition.push(FacilityField::runwaySecondaryClosed) };
    }
    constexpr RunwayBuilder<MaxLength> primaryTakeoff() const {
        return RunwayBuilder<MaxLength>{ definition.push(FacilityField::runwayPrimaryTakeoff) };
    }
    constexpr RunwayBuilder<MaxLength> primaryLanding() const {
        return RunwayBuilder<MaxLength>{ definition.push(FacilityField::runwayPrimaryLanding) };
    }
    constexpr RunwayBuilder<MaxLength> secondaryTakeoff() const {
        return RunwayBuilder<MaxLength>{ definition.push(FacilityField::runwaySecondaryTakeoff) };
    }
    constexpr RunwayBuilder<MaxLength> secondaryLanding() const {
        return RunwayBuilder<MaxLength>{ definition.push(FacilityField::runwaySecondaryLanding) };
    }
};

} // namespace SimConnect::Facilities