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


namespace SimConnect::Data {


struct InitPosition {
    double latitude{ 0.0 };         ///< Latitude in degrees.
    double longitude{ 0.0 };        ///< Longitude in degrees.
    double altitude{ 0.0 };         ///< Altitude in feet.
    double pitch{ 0.0 };            ///< Pitch in degrees.
    double bank{ 0.0 };             ///< Bank in degrees.
    double heading{ 0.0 };          ///< Heading in degrees.
    bool onGround{ true };          ///< True if on ground, false if in air.
    unsigned long airspeed{ 0 };    ///< Airspeed in knots.

    // Rule of five (all defaulted since no dynamic resources)
    constexpr InitPosition() noexcept = default;
    constexpr InitPosition(const InitPosition&) noexcept = default;
    constexpr InitPosition(InitPosition&&) noexcept = default;
    constexpr InitPosition& operator=(const InitPosition&) noexcept = default;
    constexpr InitPosition& operator=(InitPosition&&) noexcept = default;
    constexpr ~InitPosition() noexcept = default;

    /// Full constructor
    constexpr InitPosition(double lat, double lon, double alt,
                           double pit = 0.0, double bk = 0.0, double hdg = 0.0,
                           bool onG = true, unsigned long as = 0) noexcept
        : latitude(lat), longitude(lon), altitude(alt), pitch(pit), bank(bk), heading(hdg), onGround(onG), airspeed(as) {}

    // Fluent, constexpr setters returning a new InitPosition so chains can be evaluated at compile time.
    [[nodiscard]] constexpr InitPosition withLatitude(double lat) const noexcept { InitPosition p = *this; p.latitude = lat; return p; }
    [[nodiscard]] constexpr InitPosition withLongitude(double lon) const noexcept { InitPosition p = *this; p.longitude = lon; return p; }
    [[nodiscard]] constexpr InitPosition withAltitude(double alt) const noexcept { InitPosition p = *this; p.altitude = alt; return p; }
    [[nodiscard]] constexpr InitPosition withPitch(double pit) const noexcept { InitPosition p = *this; p.pitch = pit; return p; }
    [[nodiscard]] constexpr InitPosition withBank(double bk) const noexcept { InitPosition p = *this; p.bank = bk; return p; }
    [[nodiscard]] constexpr InitPosition withHeading(double hdg) const noexcept { InitPosition p = *this; p.heading = hdg; return p; }
    [[nodiscard]] constexpr InitPosition withOnGround(bool onG) const noexcept { InitPosition p = *this; p.onGround = onG; return p; }
    [[nodiscard]] constexpr InitPosition setOnGround() const noexcept { InitPosition p = *this; p.onGround = true; return p; }
    [[nodiscard]] constexpr InitPosition setInAir() const noexcept { InitPosition p = *this; p.onGround = false; return p; }
    [[nodiscard]] constexpr InitPosition withAirspeed(unsigned long as) const noexcept { InitPosition p = *this; p.airspeed = as; return p; }
    [[nodiscard]] constexpr InitPosition atCruiseSpeed() const noexcept { InitPosition p = *this; p.airspeed = INITPOSITION_AIRSPEED_CRUISE; return p; }
    [[nodiscard]] constexpr InitPosition keepAirspeed() const noexcept { InitPosition p = *this; p.airspeed = INITPOSITION_AIRSPEED_KEEP; return p; }

    // Static factory helpers for common construction patterns
    [[nodiscard]] static constexpr InitPosition fromLatLonAlt(double lat, double lon, double alt) noexcept {
        return InitPosition(lat, lon, alt);
    }

    [[nodiscard]] static constexpr InitPosition onGroundAt(double lat, double lon, double alt) noexcept {
        return InitPosition(lat, lon, alt, 0.0, 0.0, 0.0, true, 0);
    }

    [[nodiscard]] static constexpr InitPosition inAirAt(double lat, double lon, double alt, unsigned long as = 0) noexcept {
        return InitPosition(lat, lon, alt, 0.0, 0.0, 0.0, false, as);
    }

    // Equality for testing and constexpr comparisons
    [[nodiscard]] constexpr bool operator==(const InitPosition& o) const noexcept = default;

    // Conversion to SimConnect native type
    [[nodiscard]] constexpr operator DataTypes::InitPosition() const noexcept {
        return DataTypes::InitPosition{
            .Latitude = latitude,
            .Longitude = longitude,
            .Altitude = altitude,
            .Pitch = pitch,
            .Bank = bank,
            .Heading = heading,
            .OnGround = onGround ? 1u : 0u,
            .Airspeed = airspeed
        };
    }

    // Static factory from SimConnect native type
    [[nodiscard]] static constexpr InitPosition from(const DataTypes::InitPosition& simPos) noexcept {
        return InitPosition(
            simPos.Latitude,
            simPos.Longitude, 
            simPos.Altitude,
            simPos.Pitch,
            simPos.Bank,
            simPos.Heading,
            simPos.OnGround != 0,
            simPos.Airspeed
        );
    }

};

} // namespace SimConnect::Data