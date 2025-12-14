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

#include <chrono>


namespace SimConnect {


/**
 * A SimConnect connection with support for notifications through a Windows Event.
 */
template <bool ThreadSafe = false, class L = NullLogger>
class WindowsEventConnection : public Connection<ThreadSafe, L> {
public:
	using logger_type = L;

private:
	/**
	 * The event handle to use for signalling that SIMCONNECT messages are available.
	 */
    HANDLE eventHandle_{ nullptr };

public:

	/**
	 * Constructor, using the default client name.
	 */
    WindowsEventConnection() : eventHandle_() {}


	/**
	 * Constructor.
	 * @param name The name of the connection.
	 */
    WindowsEventConnection(std::string name) : Connection<ThreadSafe, L>(name) {}


	/**
	 * Constructor, using the default client name.
	 * @param eventHandle The event handle to use for signalling that SIMCONNECT messages are available.
	 */
    WindowsEventConnection(HANDLE eventHandle) : Connection<ThreadSafe, L>(), eventHandle_(eventHandle) {}


	/**
	 * Constructor.
	 * @param name The name of the connection.
	 * @param eventHandle The event handle to use for signalling that SIMCONNECT messages are available.
	 */
    WindowsEventConnection(std::string name, HANDLE eventHandle) : Connection<ThreadSafe, L>(name), eventHandle_(eventHandle) {}

    ~WindowsEventConnection() {
        if (eventHandle_ != nullptr) {
            CloseHandle(eventHandle_);
        }
    }

    WindowsEventConnection(const WindowsEventConnection&) = delete;
    WindowsEventConnection(WindowsEventConnection&&) = delete;
    WindowsEventConnection& operator=(const WindowsEventConnection&) = delete;
    WindowsEventConnection& operator=(WindowsEventConnection&&) = delete;


	/**
	 * Opens the connection. The `windowsEventHandle_` member will be ignored.
	 * @param windowsEventHandle The event handle to use for signalling that SIMCONNECT messages are available.
	 * @param configIndex The index of the configuration section to use, defaults to 0 meaning use the default configuration.
	 * @returns True if the connection is open.
	 * @throws BadConfig if the configuration does not contain the specified index.
	 */
	[[nodiscard]]
	bool open(HANDLE windowsEventHandle, int configIndex = 0) {
		return this->callOpen(nullptr, 0, windowsEventHandle, configIndex);
	}

	/**
	 * Opens the connection, optionally for a specific configuration. Will create an event handle if one is not provided.
	 * @param configIndex The index of the configuration section to use, defaults to 0 meaning use the default configuration.
	 * @returns True if the connection is open.
	 * @throws BadConfig if the configuration does not contain the specified index.
	 */
    [[nodiscard]]
    bool open(int configIndex = 0) {
        if (eventHandle_ == nullptr) {
            eventHandle_ = ::CreateEvent(nullptr, false, false, nullptr);
        }
        return open(eventHandle_, configIndex);
    }
    
	/**
	 * Checks if a message is available.
	 * @param duration The maximum amount of time to wait for a message, defaults to 0ms meaning don't wait.
	 * @returns True if a message is available.
	 */
	bool checkForMessage(std::chrono::milliseconds duration = std::chrono::milliseconds(0)) {
		return ::WaitForSingleObject(eventHandle_, (duration < std::chrono::milliseconds(0)) ? 0 : static_cast<DWORD>(duration.count())) == WAIT_OBJECT_0;
	}

	/**
	 * Waits for a message to become available.
	 * @param duration The maximum amount of time to wait for a message, defaults to 0ms meaning wait indefinitely.
	 * @returns True if a message is available.
	 */
	bool waitForMessage(std::chrono::milliseconds duration = std::chrono::milliseconds(0)) {
		return ::WaitForSingleObject(eventHandle_, (duration <= std::chrono::milliseconds(0)) ? INFINITE : static_cast<DWORD>(duration.count())) == WAIT_OBJECT_0;
	}
};

} // namespace SimConnect