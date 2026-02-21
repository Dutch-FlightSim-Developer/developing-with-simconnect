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

class VASIData {
    VASIType type_;       // TYPE
    float biasX_;         // BIAS_X
    float biasZ_;         // BIAS_Z
    float spacing_;       // SPACING
    float angle_;         // ANGLE

    static constexpr std::array<std::string_view, 14> VASITypeNames = {
        "None", "VASI 2/1", "VASI 2/2", "VASI 2/3", "VASI 3/1", "VASI 3/2", "VASI 3/3",
        "PAPI 2", "PAPI 4", "Tricolor", "PVASI", "TVASI", "Ball", "APAP"
    };

public:
    inline static bool isVASIData(const Messages::FacilityDataMsg& msg) {
        return msg.Type == FacilityDataTypes::vasi;
    }
    inline static const VASIData& from(const Messages::FacilityDataMsg& msg) {
        return *reinterpret_cast<const VASIData*>(&msg.Data);
    }

    constexpr VASIType typeCode() const noexcept { return type_; }
    constexpr std::string_view type() const noexcept {
        const auto index = static_cast<std::size_t>(type_);
        return (index < VASITypeNames.size()) ? VASITypeNames[index] : "Invalid";
    }
    constexpr float biasX() const noexcept { return biasX_; }
    constexpr float biasZ() const noexcept { return biasZ_; }
    constexpr float spacing() const noexcept { return spacing_; }
    constexpr float angle() const noexcept { return angle_; }
};

#pragma pack(pop)

template <std::size_t MaxLength = 256>
struct VasiBuilder
{
    FacilityDefinition<MaxLength> definition;
    FacilityField closeField;

    constexpr explicit VasiBuilder(FacilityDefinition<MaxLength> def, FacilityField close) 
        : definition{ def }, closeField{ close } {}

    constexpr RunwayBuilder<MaxLength> end() const {
        return RunwayBuilder<MaxLength>{ definition.push(closeField) };
    }

    constexpr VasiBuilder<MaxLength> type() const {
        return VasiBuilder<MaxLength>{ definition.push(FacilityField::vasiType), closeField };
    }
    constexpr VasiBuilder<MaxLength> biasX() const {
        return VasiBuilder<MaxLength>{ definition.push(FacilityField::vasiBiasX), closeField };
    }
    constexpr VasiBuilder<MaxLength> biasZ() const {
        return VasiBuilder<MaxLength>{ definition.push(FacilityField::vasiBiasZ), closeField };
    }
    constexpr VasiBuilder<MaxLength> spacing() const {
        return VasiBuilder<MaxLength>{ definition.push(FacilityField::vasiSpacing), closeField };
    }
    constexpr VasiBuilder<MaxLength> angle() const {
        return VasiBuilder<MaxLength>{ definition.push(FacilityField::vasiAngle), closeField };
    }

    constexpr VasiBuilder<MaxLength> allFields() const {
        return VasiBuilder<MaxLength>{
            definition
                .push(FacilityField::vasiType)
                .push(FacilityField::vasiBiasX)
                .push(FacilityField::vasiBiasZ)
                .push(FacilityField::vasiSpacing)
                .push(FacilityField::vasiAngle),
            closeField
        };
    }
};

} // namespace SimConnect::Facilities
