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
#include <cstdint>

/**
 * The name used to identify the shared client data area.
 * Both the sender and receiver must use exactly this string.
 */
constexpr static const char* CLIENT_DATA_NAME = "DutchFlightSim.HelloWorldMapped";

/**
 * The size of the message buffer in bytes.
 * Both programs must agree on this value.
 */
constexpr static unsigned int MESSAGE_SIZE = 256;

/**
 * The data layout of the shared client data area.
 *
 * Fields are registered individually via MappedClientDataDefinition, so
 * SimConnect knows each field's type. Because the two fields together cover
 * the full struct (256 + 4 = 260 == sizeof(HelloWorldData)), useMapping()
 * returns true and the library can deliver data via a direct reinterpret_cast —
 * no field-by-field copy on the receive path.
 */
struct HelloWorldData {
    std::array<char, MESSAGE_SIZE> message; ///< Text message from the sender.
    int32_t                        updateCount; ///< How many updates the sender has sent so far.
};

static_assert(sizeof(HelloWorldData) == MESSAGE_SIZE + sizeof(int32_t),
    "HelloWorldData layout changed — update MappedClientDataDefinition registrations");
