#pragma once
/*
 * Copyright (c) 2024. Bert Laverman
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


#pragma warning(push, 3)

#include <Windows.h>
#include <SimConnect.h>

#pragma warning(pop)

#include <string>


namespace SimConnect
{

enum class SimulatorVersion {
    Unknown,
    FSX,            // !defined(SIMCONNECT_REFSTRUCT) && SIMCONNECT_DATATYPE_MAX == 17
    P3D,            // defined(QWORD) (quickest test for the 64-bit versions), also !defined(SIMCONNECT_REFSTRUCT), and (SIMCONNECT_DATATYPE_MAX == 28)
    MSFS2020,       // SIMCONNECT_DATATYPE_MAX == 17
    MSFS2024        // SIMCONNECT_DATATYPE_MAX == 18
};

/**
 * A shorthand test if we need to avoid using MSFS 2024 specific features.
 */
#if defined(SIMCONNECT_TYPEDEF)
#define MSFS_2024_SDK 1
#else
#define MSFS_2024_SDK 0
#endif

/**
 * Detects which simulator SDK is being compiled against at compile-time.
 * 
 * Detection logic:
 * - MSFS 2024: SIMCONNECT_TYPEDEF is defined (2024-specific macro)
 * - MSFS 2020: SIMCONNECT_DATATYPE_MAX == 17 and modern SDK structure
 * - P3D: QWORD is defined (Prepar3D extension)
 * - FSX: Legacy SDK (SIMCONNECT_DATATYPE_MAX == 17, no modern features)
 * 
 * @returns The SimulatorVersion enum value.
 */
consteval SimulatorVersion getSimulatorVersion() {
#if defined(QWORD)
    // Prepar3D defines QWORD type
    return SimulatorVersion::P3D;
#elif !defined(SIMCONNECT_REFSTRUCT)
    // FSX didn't have SIMCONNECT_REFSTRUCT defined
    return SimulatorVersion::FSX;
#else
    if constexpr (SIMCONNECT_DATATYPE::SIMCONNECT_DATATYPE_MAX == 18) {
        // MSFS 2024 added SIMCONNECT_DATATYPE_INT8
        return SimulatorVersion::MSFS2024;
    }
    else if constexpr (SIMCONNECT_DATATYPE::SIMCONNECT_DATATYPE_MAX == 17) {
        // Up to MSFS 2024 there were only 17 data types, excluding P3D which added several large sized numerical types
        return SimulatorVersion::MSFS2020;
    }
    return SimulatorVersion::Unknown;
#endif
}


/**
 * Get the simulator version as a string at compile-time.
 * 
 * @returns A string_view representing the simulator version.
 */
consteval std::string_view getSimulatorVersionString() {
    switch (getSimulatorVersion()) {
        case SimulatorVersion::FSX:
            return "FSX/ESP";
        case SimulatorVersion::P3D:
            return "Prepar3D";
        case SimulatorVersion::MSFS2020:
            return "MSFS 2020";
        case SimulatorVersion::MSFS2024:
            return "MSFS 2024";
        default:
            return "Unknown";
    }
}


/**
 * Compile-time constant for the current simulator version.
 */
inline constexpr SimulatorVersion simulatorVersion = getSimulatorVersion();

/**
 * Compile-time constant for the current simulator version string.
 */
inline constexpr std::string_view simulatorVersionString = getSimulatorVersionString();

} // namespace SimConnect