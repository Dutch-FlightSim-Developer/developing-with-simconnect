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

#include <simconnect/connection.hpp>


namespace SimConnect {
 

/**
 * A SimConnect connection with no support for Windows Messaging or Events.
 */
template <bool ThreadSafe = false>
class SimpleConnection : public Connection<ThreadSafe>
{
public:
	/**
	 * Constructor, using the default client name.
	 */
	SimpleConnection() : Connection<ThreadSafe>() {}


	/**
	 * Constructor.
	 * @param name The name of the connection.
	 */
    SimpleConnection(std::string name) : Connection<ThreadSafe>(name) {}


    ~SimpleConnection() {}

    // We don't want copied or moved Connections.

    SimpleConnection(const SimpleConnection&) = delete;
	SimpleConnection(SimpleConnection&&) = delete;
	SimpleConnection& operator=(const SimpleConnection&) = delete;
	SimpleConnection& operator=(SimpleConnection&&) = delete;


	/**
	 * Opens the connection, optionally for a specific configuration.
	 * @param configIndex The index of the configuration section to use, defaults to 0 meaning use the default configuration.
	 * @returns True if the connection is open.
	 * @throws BadConfig if the configuration does not contain the specified index.
	 */
	[[nodiscard]]
	bool open(int configIndex = 0) {
		return this->callOpen(nullptr, 0, nullptr, configIndex);
	}
};

} // namespace SimConnect