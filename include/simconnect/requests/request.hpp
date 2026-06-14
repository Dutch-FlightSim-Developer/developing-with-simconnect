#pragma once
/*
 * Copyright (c) 2024, 2025. Bert Laverman
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

#include <simconnect/requests/requests.hpp>
#include <simconnect/messaging/registration.hpp>


namespace SimConnect {


/**
 * Represents a request in the SimConnect system.
 * A request is identified by a RequestId and can have a cleanup function that is called when the request is finished.
 *
 * Requests cannot be copied, but can be moved, to ensure unique ownership.
 */
using Request = Registration<RequestId>;

} // namespace SimConnect
