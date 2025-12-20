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

#include <simconnect.hpp>


#include <atomic>


namespace SimConnect {


using RequestId = unsigned long;


class Requests {
	std::atomic<RequestId> requestID_{ 0 };	///< The request ID for the next request.

    public:
    Requests() = default;
    ~Requests() = default;

    // No copies or moves
    Requests(const Requests&) = delete;
    Requests(Requests&&) = delete;
    Requests& operator=(const Requests&) = delete;
    Requests& operator=(Requests&&) = delete;

    /**
     * Returns the request ID for the next request.
     * @returns The request ID for the next request.
     */
    [[nodiscard]]
    RequestId nextRequestID() noexcept { return ++requestID_; }
};

} // namespace SimConnect