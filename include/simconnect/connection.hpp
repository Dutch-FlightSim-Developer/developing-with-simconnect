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

#include <simconnect.hpp>


namespace SimConnect {
    
/**
 * A SimConnect connection.
 */
class Connection
{
private:
	std::string clientName;
	HANDLE hSimConnect{ nullptr };
	HRESULT hr{ S_OK };

protected:
	[[nodiscard]]
	bool callOpen(HWND hWnd = nullptr, DWORD userMessageId = 0, HANDLE windowsEventHandle = nullptr, DWORD configIndex = 0) {
		if (isOpen()) {
			return true;
		}
		hr = SimConnect_Open(&hSimConnect, clientName.c_str(), hWnd, userMessageId, windowsEventHandle, configIndex);
		if (SUCCEEDED(hr)) {
			return true;
		}
		if (hr == E_INVALIDARG) {
			throw BadConfig(std::format("Unknown configuration section {}.", configIndex));
		}
		return false;
	}

public:
	Connection() : clientName("SimConnect client") {}
	Connection(std::string name) : clientName(name) {}
	virtual ~Connection() { close(); }

	// We don't want copied or moved Connections.
	Connection(const Connection&) = delete;
	Connection(Connection&&) = delete;
	Connection& operator=(const Connection&) = delete;
	Connection& operator=(Connection&&) = delete;

	/**
		* Returns the SimConnect handle.
		* @returns The SimConnect handle.
		*/
	operator HANDLE() const noexcept { return hSimConnect; }

	/**
		* Returns true if the connection is open.
		* @returns True if the connection is open.
		*/
	bool isOpen() const noexcept { return hSimConnect != nullptr; }

	/**
	* Closes the connection.
	* @throws SimConnectException if the call fails. This should only happen if the handle is invalid.
		*/
	void close() {
		if (hSimConnect) {
			hr = SimConnect_Close(hSimConnect);
			hSimConnect = nullptr;
			if (FAILED(hr)) {
				throw SimConnectException("SimConnect_Close failed");
			}
		}
	}

	/**
		* Returns the last error code, or 0 if there was no error.
		* @returns The last error code, or 0 if there was no error.
		*/
	HRESULT lastError() const noexcept { return hr; }

	/**
		* Returns true if the last call to SimConnect was successful.
		* @returns True if the last call to SimConnect was successful.
		*/
	operator bool() const noexcept { return SUCCEEDED(hr); }
};

}