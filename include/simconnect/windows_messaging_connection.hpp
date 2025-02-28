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


/**
 * A SimConnect connection with support for Windows Messaging.
 */
class WindowsMessagingConnection : public Connection {
	/**
	 * The Windows handle to the Window whose message queue will receive notifications for incoming messages. 
	 */
    HWND hWnd_{ nullptr };

	/**
	 * The message id to use for the notification messages. If 0, SimConnect will not send notifications.
	 */
    DWORD userMessageId_{ 0 };

public:
	/**
	 * Constructor, using the default client name.
	 */
	WindowsMessagingConnection() : Connection() {}


	/**
	 * Constructor.
	 * @param name The name of the connection.
	 */
	WindowsMessagingConnection(std::string name) : Connection(name) {}

	/**
	 * Constructor, using the default client name.
	 * @param hWnd The window handle to use for the SIMCONNECT messages.
	 * @param userMessageId The message id to use for the SIMCONNECT messages.
	 */
    WindowsMessagingConnection(HWND hWnd, DWORD userMessageId) : Connection(), hWnd_(hWnd), userMessageId_(userMessageId) {}

	/**
	 * Constructor.
	 * @param name The name of the connection.
	 * @param hWnd The window handle to use for the SIMCONNECT messages.
	 * @param userMessageId The message id to use for the SIMCONNECT messages.
	 */
	WindowsMessagingConnection(std::string name, HWND hWnd, DWORD userMessageId) : Connection(name), hWnd_(hWnd), userMessageId_(userMessageId) {}


	~WindowsMessagingConnection() { }


    // We don't want copied or moved Connections.
    WindowsMessagingConnection(const WindowsMessagingConnection&) = delete;
    WindowsMessagingConnection(WindowsMessagingConnection&&) = delete;
    WindowsMessagingConnection& operator=(const WindowsMessagingConnection&) = delete;
    WindowsMessagingConnection& operator=(WindowsMessagingConnection&&) = delete;


	/**
	 * @returns The window handle to use for the SIMCONNECT message notifications.
	 */
	HWND hWND() const noexcept { return hWnd_; }


	/**
	 * Sets the window handle to use for the SIMCONNECT message notifications.
     * 
	 * @param hWnd The window handle to use for the SIMCONNECT messages.
	 */
	void hWND(HWND hWnd) noexcept { hWnd_ = hWnd; }


	/**
	 * @returns The message id to use for the SIMCONNECT messages.
	 */
	DWORD userMessageId() const noexcept { return userMessageId_; }


	/**
	 * Sets the message id to use for the SIMCONNECT messages.
     * 
	 * @param userMessageId The message id to use for the SIMCONNECT messages.
	 */
	void userMessageId(DWORD userMessageId) noexcept { userMessageId_ = userMessageId; }


	/**
	 * Opens the connection, overriding any settings passed to the constructor.
     * 
	 * @param hWnd The window handle to use for the SIMCONNECT messages.
	 * @param userMessageId The message id to use for the SIMCONNECT messages.
	 * @param configIndex The optional index of the configuration section to use, defaults to 0 meaning use the default configuration.
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
	 * Opens the connection, optionally for a specific configuration.
     * 
	 * @param configIndex The index of the configuration section to use, defaults to 0 meaning use the default configuration.
	 * @returns True if the connection is open.
	 * @throws SimConnectException if no Window handle or valid message id is set.
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