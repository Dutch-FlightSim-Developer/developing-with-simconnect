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
#include <atomic>
#include <string>


namespace SimConnect {


class DataDefinitions {
	std::atomic_ulong dataDefID_{ 0 };	///< The Data Definition ID for the next Data Definition block.

public:
    DataDefinitions() = default;
    ~DataDefinitions() = default;

    // No copies or moves
    DataDefinitions(const DataDefinitions&) = delete;
    DataDefinitions(DataDefinitions&&) = delete;
    DataDefinitions& operator=(const DataDefinitions&) = delete;
    DataDefinitions& operator=(DataDefinitions&&) = delete;

    /**
     * Returns the Data Definition ID for the next Data Definition block.
     * @returns The Data Definition ID for the next Data Definition block.
     */
    [[nodiscard]]
    int nextDataDefID() noexcept { return ++dataDefID_; }
};

} // namespace SimConnect