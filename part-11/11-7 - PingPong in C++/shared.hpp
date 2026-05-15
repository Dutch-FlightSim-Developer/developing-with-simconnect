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

#include <cstdint>

/**
 * The name used to identify the shared client data area.
 * Both programs must use exactly this string.
 */
constexpr static const char* CLIENT_DATA_NAME = "DutchFlightSim.PingPongCpp";

/**
 * Wire protocol values for the single int32_t datum in the shared area.
 *
 * ping writes PING; pong writes PONG.  Each side detects the other's write
 * and schedules a reply.
 */
constexpr static int32_t PING = 1; ///< Sent by ping; pong reacts to this value.
constexpr static int32_t PONG = 2; ///< Sent by pong; ping reacts to this value.
