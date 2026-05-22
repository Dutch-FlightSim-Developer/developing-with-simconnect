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

#pragma once

#include <cstddef>

/**
 * The name used to identify the shared client data area.
 * Both programs must use exactly this string when calling MapClientDataNameToID.
 */
constexpr static const char* CLIENT_DATA_NAME = "DutchFlightSim.PingPong";

/**
 * The data layout of the shared client data area.
 * Both programs write to the same area starting at byte 0.
 */
struct PingPongData {
    inline static constexpr std::size_t dataSize{ 8 };

    char message[dataSize];
};
