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


namespace SimConnect {


inline constexpr double MetersToFeetFactor = 3.28084;  ///< Factor to convert meters to feet.
inline constexpr double FrequencyToKHzFactor = 0.001;    ///< Factor to convert Hz to KHz.
inline constexpr double FrequencyToMHzFactor = 0.000001;     ///< Factor to convert KHz to MHz.

constexpr static double dabs(double value) noexcept {
    return (value < 0.0) ? -value : value;
}


#pragma pack(push, 1)
/**
 * Represents a latitude, longitude, and altitude triplet, as exchanged through the SimConnect API.
 * Altitude is in meters.
 */
struct LatLonAlt {
    double latitude;
    double longitude;
    double altitude;

    
    inline double latitudeNormalized() const noexcept {
        return dabs(latitude);
    }
    constexpr char latitudeDirection() const noexcept {
        if (latitude == 0.0) {
            return ' ';
        }
        return (latitude > 0.0) ? 'N' : 'S';
    }

    double longitudeNormalized() const noexcept {
        return dabs(longitude);
    }
    constexpr char longitudeDirection() const noexcept {
        if (longitude == 0.0) {
            return ' ';
        }
        return (longitude > 0.0) ? 'E' : 'W';
    }

    constexpr double altitudeMeters() const noexcept {
        return altitude;
    }
    constexpr int altitudeFeet() const noexcept {
        return static_cast<int>(altitude * MetersToFeetFactor);
    }
};

struct LatLonAltMagVar : public LatLonAlt {
    float magVar;  ///< Magnetic variation in degrees.

    constexpr float magneticVariation() const noexcept {
        return magVar;
    }
    constexpr float magVarNormalized() const noexcept {
        return (magVar > 180.0F) ? (360.0F - magVar) : magVar;
    }
    constexpr char magVarDirection() const noexcept {
        if ((magVar == 0.0F) || (magVar == 180.0F)) {
            return ' ';
        }
        return (magVar < 180.0F) ? 'E' : 'W';
    }
};
#pragma pack(pop)

} // namespace SimConnect::Datatypes