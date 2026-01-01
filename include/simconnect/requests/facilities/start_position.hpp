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



class StartData {
    double latitude_;       // LATITUDE
    double longitude_;      // LONGITUDE
    double altitude_;       // ALTITUDE
    float heading_;         // HEADING
    RunwayNumber number_;        // NUMBER
    RunwayDesignator designator_;    // DESIGNATOR
    StartType type_;          // TYPE

public:
    constexpr double latitude() const noexcept { return latitude_; }
    constexpr double longitude() const noexcept { return longitude_; }
    constexpr double altitude() const noexcept { return altitude_; }
    constexpr float heading() const noexcept { return heading_; }
    constexpr RunwayNumber number() const noexcept { return number_; }
    constexpr RunwayDesignator designator() const noexcept { return designator_; }
    constexpr StartType type() const noexcept { return type_; }
};


#pragma pack(pop)

} // namespace SimConnect::Facilities