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

namespace SimConnect::Facilities {

#pragma pack(push, 1)

class ApproachLightsData {
    ApproachLightsSystem system_;          // SYSTEM
    int32_t strobeCount_;                  // STROBE_COUNT
    int32_t hasEndLights_;                 // HAS_END_LIGHTS
    int32_t hasREILLights_;                // HAS_REIL_LIGHTS
    int32_t hasTouchdownLights_;           // HAS_TOUCHDOWN_LIGHTS
    int32_t onGround_;                     // ON_GROUND
    int32_t enable_;                       // ENABLE
    float offset_;                         // OFFSET
    float spacing_;                        // SPACING
    float slope_;                          // SLOPE

public:
    inline static bool isApproachLightsData(const Messages::FacilityDataMsg& msg) {
        return msg.Type == FacilityDataTypes::approachLights;
    }
    inline static const ApproachLightsData& from(const Messages::FacilityDataMsg& msg) {
        return *reinterpret_cast<const ApproachLightsData*>(&msg.Data);
    }

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

#pragma pack(pop)

template <std::size_t MaxLength = 256>
struct ApproachLightsBuilder
{
    FacilityDefinition<MaxLength> definition;
    FacilityField closeField;

    constexpr explicit ApproachLightsBuilder(FacilityDefinition<MaxLength> def, FacilityField close) 
        : definition{ def }, closeField{ close } {}

    constexpr RunwayBuilder<MaxLength> end() const {
        return RunwayBuilder<MaxLength>{ definition.push(closeField) };
    }

    constexpr ApproachLightsBuilder<MaxLength> system() const {
        return ApproachLightsBuilder<MaxLength>{ definition.push(FacilityField::approachLightsSystem), closeField };
    }
    constexpr ApproachLightsBuilder<MaxLength> strobeCount() const {
        return ApproachLightsBuilder<MaxLength>{ definition.push(FacilityField::approachLightsStrobeCount), closeField };
    }
    constexpr ApproachLightsBuilder<MaxLength> hasEndLights() const {
        return ApproachLightsBuilder<MaxLength>{ definition.push(FacilityField::approachLightsHasEndLights), closeField };
    }
    constexpr ApproachLightsBuilder<MaxLength> hasReilLights() const {
        return ApproachLightsBuilder<MaxLength>{ definition.push(FacilityField::approachLightsHasReilLights), closeField };
    }
    constexpr ApproachLightsBuilder<MaxLength> hasTouchdownLights() const {
        return ApproachLightsBuilder<MaxLength>{ definition.push(FacilityField::approachLightsHasTouchdownLights), closeField };
    }
    constexpr ApproachLightsBuilder<MaxLength> onGround() const {
        return ApproachLightsBuilder<MaxLength>{ definition.push(FacilityField::approachLightsOnGround), closeField };
    }
    constexpr ApproachLightsBuilder<MaxLength> enable() const {
        return ApproachLightsBuilder<MaxLength>{ definition.push(FacilityField::approachLightsEnable), closeField };
    }
    constexpr ApproachLightsBuilder<MaxLength> offset() const {
        return ApproachLightsBuilder<MaxLength>{ definition.push(FacilityField::approachLightsOffset), closeField };
    }
    constexpr ApproachLightsBuilder<MaxLength> spacing() const {
        return ApproachLightsBuilder<MaxLength>{ definition.push(FacilityField::approachLightsSpacing), closeField };
    }
    constexpr ApproachLightsBuilder<MaxLength> slope() const {
        return ApproachLightsBuilder<MaxLength>{ definition.push(FacilityField::approachLightsSlope), closeField };
    }

    constexpr ApproachLightsBuilder<MaxLength> allFields() const {
        return ApproachLightsBuilder<MaxLength>{
            definition
                .push(FacilityField::approachLightsSystem)
                .push(FacilityField::approachLightsStrobeCount)
                .push(FacilityField::approachLightsHasEndLights)
                .push(FacilityField::approachLightsHasReilLights)
                .push(FacilityField::approachLightsHasTouchdownLights)
                .push(FacilityField::approachLightsOnGround)
                .push(FacilityField::approachLightsEnable)
                .push(FacilityField::approachLightsOffset)
                .push(FacilityField::approachLightsSpacing)
                .push(FacilityField::approachLightsSlope),
            closeField
        };
    }
};

} // namespace SimConnect::Facilities
