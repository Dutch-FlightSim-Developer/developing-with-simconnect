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
class VORData {
    double vorLatitude_;                    // VOR_LATITUDE
    double vorLongitude_;                   // VOR_LONGITUDE
    double vorAltitude_;                    // VOR_ALTITUDE

    double dmeLatitude_;                    // DME_LATITUDE
    double dmeLongitude_;                   // DME_LONGITUDE
    double dmeAltitude_;                    // DME_ALTITUDE

    double gsLatitude_;                     // GS_LATITUDE
    double gsLongitude_;                    // GS_LONGITUDE
    double gsAltitude_;                     // GS_ALTITUDE

    double tacanLatitude_;                  // TACAN_LATITUDE
    double tacanLongitude_;                 // TACAN_LONGITUDE
    double tacanAltitude_;                  // TACAN_ALTITUDE

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
    float magVar_;                          // MAGVAR
    float localizer_;                       // LOCALIZER
    float localizerWidth_;                  // LOCALIZER_WIDTH
    float glideSlope_;                      // GLIDE_SLOPE
    std::array<char, Name64Length> name_;   // NAME
    float dmeBias_;                         // DME_BIAS
    LocalizerCategory lsCategory_;          // LS_CATEGORY
    std::int32_t isTrueReferenced_;         // IS_TRUE_REFERENCED

public:
    inline static bool isVORData(const Messages::FacilityDataMsg& msg) noexcept {
        return msg.Type == FacilityDataTypes::vor;
    }
    inline static const VORData& from(const Messages::FacilityDataMsg& msg) {
        return *reinterpret_cast<const VORData*>(&msg.Data);
    }

    constexpr bool isNav() const noexcept { return isNav_ != 0; }
    constexpr double vorLatitude() const noexcept { return vorLatitude_; }
    constexpr double vorLatitudeNormalized() const noexcept { return std::fabs(vorLatitude_); }
    constexpr char vorLatitudeDirection() const noexcept { if (vorLatitude_ == 0) { return ' '; } return (vorLatitude_ > 0) ? 'N' : 'S'; }
    constexpr double vorLongitude() const noexcept { return vorLongitude_; }
    constexpr double vorLongitudeNormalized() const noexcept { return std::fabs(vorLongitude_); }
    constexpr char vorLongitudeDirection() const noexcept { if (vorLongitude_ == 0) { return ' '; } return (vorLongitude_ > 0) ? 'E' : 'W'; }
    constexpr double vorAltitude() const noexcept { return vorAltitude_; }
    constexpr double vorAltitudeMeters() const noexcept { return vorAltitude_; }
    constexpr double vorAltitudeFeet() const noexcept { return vorAltitude_ * MetersToFeetFactor; }
    constexpr float magVar() const noexcept { return magVar_; }

    constexpr bool isDme() const noexcept { return isDme_ != 0; }
    constexpr bool dmeAtNav() const noexcept { return dmeAtNav_ != 0; }
    constexpr bool dmeAtGlideSlope() const noexcept { return dmeAtGlideSlope_ != 0; }
    constexpr double dmeLatitude() const noexcept { return dmeLatitude_; }
    constexpr double dmeLongitude() const noexcept { return dmeLongitude_; }
    constexpr double dmeAltitude() const noexcept { return dmeAltitude_; }
    constexpr double dmeAltitudeMeters() const noexcept { return dmeAltitude_; }
    constexpr double dmeAltitudeFeet() const noexcept { return dmeAltitude_ * MetersToFeetFactor; }
    constexpr float dmeBias() const noexcept { return dmeBias_; }

    constexpr bool hasGlideSlope() const noexcept { return hasGlideSlope_ != 0; }
    constexpr double gsLatitude() const noexcept { return gsLatitude_; }
    constexpr double gsLongitude() const noexcept { return gsLongitude_; }
    constexpr double gsAltitude() const noexcept { return gsAltitude_; }
    constexpr double gsAltitudeMeters() const noexcept { return gsAltitude_; }
    constexpr double gsAltitudeFeet() const noexcept { return gsAltitude_ * MetersToFeetFactor; }
    constexpr float glideSlopeDegrees() const noexcept { return glideSlope_; }

    constexpr bool isTacan() const noexcept { return isTacan_ != 0; }
    constexpr double tacanLatitude() const noexcept { return tacanLatitude_; }
    constexpr double tacanLongitude() const noexcept { return tacanLongitude_; }
    constexpr double tacanAltitude() const noexcept { return tacanAltitude_; }
    constexpr double tacanAltitudeMeters() const noexcept { return tacanAltitude_; }
    constexpr double tacanAltitudeFeet() const noexcept { return tacanAltitude_ * MetersToFeetFactor; }


    constexpr std::uint32_t frequency() const noexcept { return frequency_; }
    constexpr double frequencyMHz() const noexcept { return frequency_ * FrequencyToMHzFactor; }

    constexpr VORType type() const noexcept { return type_; }
    inline std::string_view name() const noexcept { return SimConnect::toString(name_); }

    constexpr float navRange() const noexcept { return navRange_; }
    constexpr float localizerHeading() const noexcept { return localizer_; }
    constexpr float localizerWidth() const noexcept { return localizerWidth_; }
    constexpr bool hasBackCourse() const noexcept { return hasBackCourse_ != 0; }
    constexpr LocalizerCategory lsCategory() const noexcept { return lsCategory_; }
    constexpr bool isTrueReferenced() const noexcept { return isTrueReferenced_ != 0; }
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
    constexpr VORBuilder<MaxLength> dmeBias() const {
        return VORBuilder<MaxLength>{ definition.push(FacilityField::vorDmeBias) };
    }
    constexpr VORBuilder<MaxLength> lsCategory() const {
        return VORBuilder<MaxLength>{ definition.push(FacilityField::vorLsCategory) };
    }
    constexpr VORBuilder<MaxLength> isTrueReferenced() const {
        return VORBuilder<MaxLength>{ definition.push(FacilityField::vorIsTrueReferenced) };
    }

    constexpr VORBuilder<MaxLength> allFields() const {
        return VORBuilder<MaxLength>{
            definition
                .push(FacilityField::vorVorLatitude)
                .push(FacilityField::vorVorLongitude)
                .push(FacilityField::vorVorAltitude)
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
                .push(FacilityField::vorMagvar)
                .push(FacilityField::vorLocalizer)
                .push(FacilityField::vorLocalizerWidth)
                .push(FacilityField::vorGlideSlope)
                .push(FacilityField::vorName)
                .push(FacilityField::vorDmeBias)
                .push(FacilityField::vorLsCategory)
                .push(FacilityField::vorIsTrueReferenced)
        };
    }
    
};
} // namespace SimConnect::Facilities