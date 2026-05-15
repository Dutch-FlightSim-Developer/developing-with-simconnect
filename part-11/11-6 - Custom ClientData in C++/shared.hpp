/*
 * Copyright (c) 2026. Bert Laverman
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

#pragma once

#include <array>
#include <cstddef>

/**
 * The name used to identify the shared client data area.
 * Both the sender and receiver must use exactly this string.
 */
constexpr static const char* CLIENT_DATA_NAME = "DutchFlightSim.FlightParams";

/**
 * The data layout used internally by both programs.
 *
 * Both fields are stored as human-readable decimal strings internally, but
 * the CustomClientDataDefinition translates them to/from int32_t values on
 * the wire.  The wire format is two int32_t datums (8 bytes total), while the
 * struct is 64 bytes — sizeof(FlightData) != wire size, so useMapping() is
 * always false for CustomClientDataDefinition, and the library always routes
 * through the registered getters/setters rather than a direct cast.
 */
constexpr static std::size_t kFieldSize = 32; ///< Buffer size for each numeric string field.

struct FlightData {
    std::array<char, kFieldSize> altitudeStr; ///< Altitude in feet, stored as decimal string (e.g. "35000").
    std::array<char, kFieldSize> speedStr;    ///< Speed in knots, stored as decimal string (e.g. "450").
};
