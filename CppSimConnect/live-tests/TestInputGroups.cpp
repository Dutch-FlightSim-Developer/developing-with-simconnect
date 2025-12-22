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

#include "gtest/gtest.h"

#include <simconnect/windows_event_connection.hpp>
#include <simconnect/windows_event_handler.hpp>

#include <simconnect/events/events.hpp>
#include <simconnect/events/event_handler.hpp>
#include <simconnect/events/input_group.hpp>

#include <atomic>
#include <chrono>

using namespace SimConnect;
using namespace std::chrono_literals;


//NOLINTBEGIN(readability-function-cognitive-complexity)

//NOLINTEND(readability-function-cognitive-complexity)