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


namespace SimConnect::Facilities {

#pragma pack(push, 1)

/**
 * Represents the data structure for a TaxiName facility child element as returned by SimConnect.
 * A taxi name carries only a single NAME field, so it is surfaced as a plain string rather
 * than a dedicated facility struct.
 */
class TaxiNameData {
    std::array<char, NameLength> name_; // NAME

public:
    inline static bool isTaxiNameData(const Messages::FacilityDataMsg& msg) noexcept {
        return msg.Type == FacilityDataTypes::taxiName;
    }
    inline static const TaxiNameData& from(const Messages::FacilityDataMsg& msg) {
        return *reinterpret_cast<const TaxiNameData*>(&msg.Data);
    }

    /** Returns the taxi name. */
    constexpr std::string_view name() const noexcept { return SimConnect::toString(name_); }
};

#pragma pack(pop)

} // namespace SimConnect::Facilities
