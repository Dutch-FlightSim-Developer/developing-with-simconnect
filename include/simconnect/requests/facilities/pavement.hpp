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

class PavementData {
    float length_;      // LENGTH
    float width_;       // WIDTH
    int32_t enable_;    // ENABLE

public:
    inline static bool isPavementData(const Messages::FacilityDataMsg& msg) {
        return msg.Type == FacilityDataTypes::pavement;
    }
    inline static const PavementData& from(const Messages::FacilityDataMsg& msg) {
        return *reinterpret_cast<const PavementData*>(&msg.Data);
    }

    constexpr float length() const noexcept { return length_; }
    constexpr float lengthMeters() const noexcept { return length_; }
    constexpr float lengthFeet() const noexcept { return static_cast<float>(length_ * MetersToFeetFactor); }
    constexpr float width() const noexcept { return width_; }
    constexpr float widthMeters() const noexcept { return width_; }
    constexpr float widthFeet() const noexcept { return static_cast<float>(width_ * MetersToFeetFactor); }
    constexpr bool isEnabled() const noexcept { return enable_ != 0; }
};

#pragma pack(pop)

template <std::size_t MaxLength = 256>
struct PavementBuilder
{
    FacilityDefinition<MaxLength> definition;
    FacilityField closeField;

    constexpr explicit PavementBuilder(FacilityDefinition<MaxLength> def, FacilityField close) 
        : definition{ def }, closeField{ close } {}

    constexpr RunwayBuilder<MaxLength> end() const {
        return RunwayBuilder<MaxLength>{ definition.push(closeField) };
    }

    constexpr PavementBuilder<MaxLength> length() const {
        return PavementBuilder<MaxLength>{ definition.push(FacilityField::pavementLength), closeField };
    }
    constexpr PavementBuilder<MaxLength> width() const {
        return PavementBuilder<MaxLength>{ definition.push(FacilityField::pavementWidth), closeField };
    }
    constexpr PavementBuilder<MaxLength> enable() const {
        return PavementBuilder<MaxLength>{ definition.push(FacilityField::pavementEnable), closeField };
    }

    constexpr PavementBuilder<MaxLength> allFields() const {
        return PavementBuilder<MaxLength>{
            definition
                .push(FacilityField::pavementLength)
                .push(FacilityField::pavementWidth)
                .push(FacilityField::pavementEnable),
            closeField
        };
    }
};

} // namespace SimConnect::Facilities
