#pragma once
/*
 * Copyright (c) 2024. Bert Laverman
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

class WindowsMessagingConnection : public Connection {
    HWND hWnd_{ nullptr };
    DWORD userMessageId_{ 0 };

public:

    WindowsMessagingConnection() {}
    WindowsMessagingConnection(std::string name) : Connection(name) {}
    WindowsMessagingConnection(HWND hWnd, DWORD userMessageId) : hWnd_(hWnd), userMessageId_(userMessageId) {}
    WindowsMessagingConnection(std::string name, HWND hWnd, DWORD userMessageId) : Connection(name), hWnd_(hWnd), userMessageId_(userMessageId) {}

    ~WindowsMessagingConnection() { }

    WindowsMessagingConnection(const WindowsMessagingConnection&) = delete;
    WindowsMessagingConnection(WindowsMessagingConnection&&) = delete;
    WindowsMessagingConnection& operator=(const WindowsMessagingConnection&) = delete;
    WindowsMessagingConnection& operator=(WindowsMessagingConnection&&) = delete;


	/**
		* Opens the connection.
		* @param hWnd The window handle to use for the SIMCONNECT messages.
		* @param userMessageId The message id to use for the SIMCONNECT messages.
		* @param configIndex The index of the configuration section to use, defaults to 0 meaning use the default configuration.
		* @returns True if the connection is open.
		* @throws BadConfig if the configuration does not contain the specified index.
		*/
	[[nodiscard]]
	bool open(HWND hWnd, DWORD userMessageId, int configIndex = 0) {
		if (isOpen()) {
			return true;
		}
		hWnd_ = hWnd;
		userMessageId_ = userMessageId;

        return open(configIndex);
	}

	/**
		* Opens the connection, optionally for a specific configuration. Will create an event handle if one is not provided.
		* @param configIndex The index of the configuration section to use, defaults to 0 meaning use the default configuration.
		* @returns True if the connection is open.
		* @throws BadConfig if the configuration does not contain the specified index.
		*/
    [[nodiscard]]
    bool open(int configIndex = 0) {
        if (hWnd_ == nullptr) {
            throw SimConnectException("hWnd is null.");
        }
        if (userMessageId_ < WM_USER) {
            throw SimConnectException("userMessageId is less than WM_USER.");
        }
		return callOpen(hWnd_, userMessageId_, nullptr, configIndex);
    }
};

}