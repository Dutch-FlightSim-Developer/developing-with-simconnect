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

/**
 * The name used to identify the shared client data area.
 * Both the sender and receiver must use exactly this string.
 */
constexpr static const char* CLIENT_DATA_NAME = "DutchFlightSim.HelloWorld";

/**
 * The size of the message buffer in bytes.
 * Both programs must agree on this value.
 */
constexpr static unsigned int MESSAGE_SIZE = 256;

/**
 * The data layout of the shared client data area.
 * SimConnect transfers raw bytes, so both programs must use an identical struct.
 */
struct HelloWorldData {
    char message[MESSAGE_SIZE];
};
