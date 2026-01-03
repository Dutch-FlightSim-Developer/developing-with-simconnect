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
#include <simconnect/simconnect_datatypes.hpp>

#include <simconnect/requests/facilities/facility_definition.hpp>
#include <simconnect/requests/facilities/facility_definition_builder.hpp>

#include <simconnect/requests/facilities/taxi_parking.hpp>
#include <simconnect/requests/facilities/frequency.hpp>

#include <cmath>
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
struct VORData {
    LatLonAltMagVar vorPosition;
    LatLonAlt dmePosition;
    LatLonAlt gsPosition;
    LatLonAlt tacanPosition;

    std::int32_t isNav_;                    // IS_NAV
    std::int32_t isDme_;                    // IS_DME
    std::int32_t isTacan_;                  // IS_TACAN
    std::int32_t hasGlideSlope_;            // HAS_GLIDE_SLOPE
    std::int32_t dmeAtNav_;                 // DME_AT_NAV
    std::int32_t dmeAtGlideSlope_;          // DME_AT_GLIDE_SLOPE
    std::int32_t hasBackCourse_;            // HAS_BACK_COURSE

    std::uint32_t frequency_;               // FREQUENCY
    VORType type_;                          // TYPE
    float navRange_;                        // NAV_RANGE
    float localizer_;                       // LOCALIZER
    float localizerWidth_;                  // LOCALIZER_WIDTH
    float glideSlope_;                      // GLIDE_SLOPE
    std::array<char, Name64Length> name_;   // NAME
#if MSFS_2024_SDK
    float dmeBias_;                         // DME_BIAS (2024 only)
    LocalizerCategory lsCategory_;          // LS_CATEGORY (2024 only)
    std::int32_t isTrueReferenced_;         // IS_TRUE_REFERENCED (2024 only)
#endif

public:
    inline static bool isVORData(const Messages::FacilityDataMsg& msg) noexcept {
        return msg.Type == FacilityDataTypes::vor;
    }
    inline static const VORData& from(const Messages::FacilityDataMsg& msg) {
        return *reinterpret_cast<const VORData*>(&msg.Data);
    }

    constexpr bool isNav() const noexcept { return isNav_ != 0; }

    constexpr bool isDme() const noexcept { return isDme_ != 0; }
    constexpr bool dmeAtNav() const noexcept { return dmeAtNav_ != 0; }
    constexpr bool dmeAtGlideSlope() const noexcept { return dmeAtGlideSlope_ != 0; }
#if MSFS_2024_SDK
    constexpr float dmeBias() const noexcept { return dmeBias_; }
#endif

    constexpr bool hasGlideSlope() const noexcept { return hasGlideSlope_ != 0; }
    constexpr float glideSlope() const noexcept { return glideSlope_; }

    constexpr bool isTacan() const noexcept { return isTacan_ != 0; }

    constexpr std::uint32_t frequency() const noexcept { return frequency_; }
    constexpr double frequencyMHz() const noexcept { return frequency_ * FrequencyToMHzFactor; }

    constexpr VORType type() const noexcept { return type_; }
    inline std::string_view name() const noexcept { return SimConnect::toString(name_); }

    constexpr float navRange() const noexcept { return navRange_; }
    constexpr double navRangeNM() const noexcept { return static_cast<double>(navRange_ / 1852.0); }
    constexpr float localizerHeading() const noexcept { return localizer_; }
    constexpr float localizerWidth() const noexcept { return localizerWidth_; }
    constexpr bool hasBackCourse() const noexcept { return hasBackCourse_ != 0; }
#if MSFS_2024_SDK
    constexpr LocalizerCategory lsCategory() const noexcept { return lsCategory_; }
    constexpr bool isTrueReferenced() const noexcept { return isTrueReferenced_ != 0; }
#endif
};

#pragma pack(pop)


/**
 * A Builder class for constructing VOR facility definitions.
 * 
 * @tparam MaxLength The maximum length of the facility definition.
 */
template <std::size_t MaxLength>
struct VORBuilder
{
    FacilityDefinition<MaxLength> definition;

    constexpr explicit VORBuilder(FacilityDefinition<MaxLength> def) : definition{ def } {}

    constexpr Builder<MaxLength> end() const {
        return Builder<MaxLength>{ definition.push(FacilityField::vorClose) };
    }

    // Field setters
    constexpr VORBuilder<MaxLength> vorLatitude() const {
        return VORBuilder<MaxLength>{ definition.push(FacilityField::vorVorLatitude) };
    }
    constexpr VORBuilder<MaxLength> vorLongitude() const {
        return VORBuilder<MaxLength>{ definition.push(FacilityField::vorVorLongitude) };
    }
    constexpr VORBuilder<MaxLength> vorAltitude() const {
        return VORBuilder<MaxLength>{ definition.push(FacilityField::vorVorAltitude) };
    }
    constexpr VORBuilder<MaxLength> dmeLatitude() const {
        return VORBuilder<MaxLength>{ definition.push(FacilityField::vorDmeLatitude) };
    }
    constexpr VORBuilder<MaxLength> dmeLongitude() const {
        return VORBuilder<MaxLength>{ definition.push(FacilityField::vorDmeLongitude) };
    }
    constexpr VORBuilder<MaxLength> dmeAltitude() const {
        return VORBuilder<MaxLength>{ definition.push(FacilityField::vorDmeAltitude) };
    }
    constexpr VORBuilder<MaxLength> gsLatitude() const {
        return VORBuilder<MaxLength>{ definition.push(FacilityField::vorGsLatitude) };
    }
    constexpr VORBuilder<MaxLength> gsLongitude() const {
        return VORBuilder<MaxLength>{ definition.push(FacilityField::vorGsLongitude) };
    }
    constexpr VORBuilder<MaxLength> gsAltitude() const {
        return VORBuilder<MaxLength>{ definition.push(FacilityField::vorGsAltitude) };
    }
    constexpr VORBuilder<MaxLength> tacanLatitude() const {
        return VORBuilder<MaxLength>{ definition.push(FacilityField::vorTacanLatitude) };
    }
    constexpr VORBuilder<MaxLength> tacanLongitude() const {
        return VORBuilder<MaxLength>{ definition.push(FacilityField::vorTacanLongitude) };
    }
    constexpr VORBuilder<MaxLength> tacanAltitude() const {
        return VORBuilder<MaxLength>{ definition.push(FacilityField::vorTacanAltitude) };
    }
    constexpr VORBuilder<MaxLength> isNav() const {
        return VORBuilder<MaxLength>{ definition.push(FacilityField::vorIsNav) };
    }
    constexpr VORBuilder<MaxLength> isDme() const {
        return VORBuilder<MaxLength>{ definition.push(FacilityField::vorIsDme) };
    }
    constexpr VORBuilder<MaxLength> isTacan() const {
        return VORBuilder<MaxLength>{ definition.push(FacilityField::vorIsTacan) };
    }
    constexpr VORBuilder<MaxLength> hasGlideSlope() const {
        return VORBuilder<MaxLength>{ definition.push(FacilityField::vorHasGlideSlope) };
    }
    constexpr VORBuilder<MaxLength> dmeAtNav() const {
        return VORBuilder<MaxLength>{ definition.push(FacilityField::vorDmeAtNav) };
    }
    constexpr VORBuilder<MaxLength> dmeAtGlideSlope() const {
        return VORBuilder<MaxLength>{ definition.push(FacilityField::vorDmeAtGlideSlope) };
    }
    constexpr VORBuilder<MaxLength> hasBackCourse() const {
        return VORBuilder<MaxLength>{ definition.push(FacilityField::vorHasBackCourse) };
    }
    constexpr VORBuilder<MaxLength> frequency() const {
        return VORBuilder<MaxLength>{ definition.push(FacilityField::vorFrequency) };
    }
    constexpr VORBuilder<MaxLength> type() const {
        return VORBuilder<MaxLength>{ definition.push(FacilityField::vorType) };
    }
    constexpr VORBuilder<MaxLength> navRange() const {
        return VORBuilder<MaxLength>{ definition.push(FacilityField::vorNavRange) };
    }
    constexpr VORBuilder<MaxLength> magVar() const {
        return VORBuilder<MaxLength>{ definition.push(FacilityField::vorMagvar) };
    }
    constexpr VORBuilder<MaxLength> localizer() const {
        return VORBuilder<MaxLength>{ definition.push(FacilityField::vorLocalizer) };
    }
    constexpr VORBuilder<MaxLength> localizerWidth() const {
        return VORBuilder<MaxLength>{ definition.push(FacilityField::vorLocalizerWidth) };
    }
    constexpr VORBuilder<MaxLength> glideSlope() const {
        return VORBuilder<MaxLength>{ definition.push(FacilityField::vorGlideSlope) };
    }
    constexpr VORBuilder<MaxLength> name() const {
        return VORBuilder<MaxLength>{ definition.push(FacilityField::vorName) };
    }
#if MSFS_2024_SDK
    constexpr VORBuilder<MaxLength> dmeBias() const {
        return VORBuilder<MaxLength>{ definition.push(FacilityField::vorDmeBias) };
    }
    constexpr VORBuilder<MaxLength> lsCategory() const {
        return VORBuilder<MaxLength>{ definition.push(FacilityField::vorLsCategory) };
    }
    constexpr VORBuilder<MaxLength> isTrueReferenced() const {
        return VORBuilder<MaxLength>{ definition.push(FacilityField::vorIsTrueReferenced) };
    }
#endif

    constexpr VORBuilder<MaxLength> allFields() const {
        return VORBuilder<MaxLength>{
            definition
                .push(FacilityField::vorVorLatitude)
                .push(FacilityField::vorVorLongitude)
                .push(FacilityField::vorVorAltitude)
                .push(FacilityField::vorMagvar)
                .push(FacilityField::vorDmeLatitude)
                .push(FacilityField::vorDmeLongitude)
                .push(FacilityField::vorDmeAltitude)
                .push(FacilityField::vorGsLatitude)
                .push(FacilityField::vorGsLongitude)
                .push(FacilityField::vorGsAltitude)
                .push(FacilityField::vorTacanLatitude)
                .push(FacilityField::vorTacanLongitude)
                .push(FacilityField::vorTacanAltitude)
                .push(FacilityField::vorIsNav)
                .push(FacilityField::vorIsDme)
                .push(FacilityField::vorIsTacan)
                .push(FacilityField::vorHasGlideSlope)
                .push(FacilityField::vorDmeAtNav)
                .push(FacilityField::vorDmeAtGlideSlope)
                .push(FacilityField::vorHasBackCourse)
                .push(FacilityField::vorFrequency)
                .push(FacilityField::vorType)
                .push(FacilityField::vorNavRange)
                .push(FacilityField::vorLocalizer)
                .push(FacilityField::vorLocalizerWidth)
                .push(FacilityField::vorGlideSlope)
                .push(FacilityField::vorName)
#if MSFS_2024_SDK
                .push(FacilityField::vorDmeBias)
                .push(FacilityField::vorLsCategory)
                .push(FacilityField::vorIsTrueReferenced)
#endif
        };
    }
    
};
} // namespace SimConnect::Facilities