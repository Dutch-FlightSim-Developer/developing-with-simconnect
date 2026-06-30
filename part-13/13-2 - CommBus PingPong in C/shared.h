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

#include <chrono>

/**
 * CommBus event name for the Ping direction.
 */
constexpr static const char* COMMBUS_PING_EVENT = "DutchFlightSim.Tutorial.Ping";

/**
 * CommBus event name for the Pong direction.
 */
constexpr static const char* COMMBUS_PONG_EVENT = "DutchFlightSim.Tutorial.Pong";

/**
 * How long Ping waits for a Pong reply before giving up.
 */
constexpr static std::chrono::seconds PONG_TIMEOUT{ 3 };

/**
 * Delay between receiving a Pong and sending the next Ping.
 */
constexpr static std::chrono::seconds PING_INTERVAL{ 1 };

/**
 * Total number of Ping/Pong exchanges to complete before exiting.
 */
constexpr static int MAX_EXCHANGES{ 5 };
