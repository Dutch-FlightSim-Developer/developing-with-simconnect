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


namespace SimConnect::Facilities {

#pragma pack(push, 1)

class FrequencyData {
    FrequencyType type_;    // TYPE
    int32_t frequency_;     // FREQUENCY
    std::array<char, Name64Length> name_; // NAME

public:
    inline static bool isFrequencyData(const Messages::FacilityDataMsg& msg) {
        return msg.Type == FacilityDataTypes::frequency;
    }
    inline static const FrequencyData& from(const Messages::FacilityDataMsg& msg) {
        return *reinterpret_cast<const FrequencyData*>(&msg.Data);
    }

    constexpr FrequencyType type() const noexcept { return type_; }
    constexpr int32_t frequency() const noexcept { return frequency_; }
    constexpr float frequencyMHz() const noexcept { return static_cast<float>(frequency_) / 1'000'000.0f; }
    constexpr std::string_view name() const noexcept { return { name_.data(), Name64Length }; }
};

#pragma pack(pop)


/**
 * Builder for Frequency facility definitions.
 * 
 * @tparam MaxLength The maximum length of the facility definition.
 */
template<std::size_t MaxLength>
struct FrequencyBuilder
{
    FacilityDefinition<MaxLength> definition;

    constexpr explicit FrequencyBuilder(FacilityDefinition<MaxLength> def) : definition{ def } {}

    constexpr AirportBuilder<MaxLength> end() const {
        return AirportBuilder<MaxLength>{ definition.push(FacilityField::frequencyClose) };
    }

    // Field setters
    constexpr FrequencyBuilder<MaxLength> type() const {
        return FrequencyBuilder<MaxLength>{ definition.push(FacilityField::frequencyType) };
    }
    constexpr FrequencyBuilder<MaxLength> frequency() const {
        return FrequencyBuilder<MaxLength>{ definition.push(FacilityField::frequencyFrequency) };
    }
    constexpr FrequencyBuilder<MaxLength> name() const {
        return FrequencyBuilder<MaxLength>{ definition.push(FacilityField::frequencyName) };
    }


    constexpr FrequencyBuilder<MaxLength> allFields() const {
        return FrequencyBuilder<MaxLength> {
            definition
                .push(FacilityField::frequencyType)
                .push(FacilityField::frequencyFrequency)
                .push(FacilityField::frequencyName)
        };
    }
};

} // namespace SimConnect::Facilities