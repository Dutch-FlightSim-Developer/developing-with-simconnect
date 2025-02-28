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

#include <simconnect/events/events.hpp>

#include <atomic>


namespace SimConnect {
    
/**
 * A SimConnect connection.
 */
class Connection
{
private:
	std::string clientName_;			///< The name of the client.
	HANDLE hSimConnect_{ nullptr };		///< The SimConnect handle, if connected.
	HRESULT hr_{ S_OK };				///< The last error code.

	std::atomic_ulong requestID_{ 0 };	///< The request ID for the next request.

public:
	/**
	 * Returns the name of the client.
	 * @returns The name of the client.
	 */
	[[nodiscard]]
	const std::string& name() const noexcept { return clientName_; }


	/**
	 * Returns true if the connection is open.
	 * @returns True if the connection is open.
	 */
	[[nodiscard]]
	bool isOpen() const noexcept { return hSimConnect_ != nullptr; }


	/**
	 * Returns the last error code, or 0 if there was no error.
	 * 
	 * @returns The last error code, or 0 if there was no error.
	 */
	HRESULT hr() const noexcept { return hr_; }


	/**
	 * Records an HRESULT.
	 * 
	 * @param hr The HRESULT value.
	 * @returns The HRESULT value.
	 */
	HRESULT hr(HRESULT hr) noexcept {
		hr_ = hr;
		return hr_;
	}


	/**
	 * Returns true if the last call to SimConnect was successful.
	 * 
	 * @returns True if the last call to SimConnect was successful.
	 */
	bool succeeded() const noexcept { return SUCCEEDED(hr()); }


    /**
	 * Returns true if the last call to SimConnect failed.
	 * @returns True if the last call to SimConnect failed.
	 */
	bool failed() const noexcept { return FAILED(hr()); }


protected:
	/**
	 * Opens the connection.
	 * 
	 * @param hWnd The window handle to receive the user messages.
	 * @param userMessageId The user message identifier.
	 * @param windowsEventHandle The Windows event handle.
	 * @param configIndex The index of the configuration section to use, defaults to 0 meaning use the default configuration.
	 */
	[[nodiscard]]
	bool callOpen(HWND hWnd = nullptr, DWORD userMessageId = 0, HANDLE windowsEventHandle = nullptr, DWORD configIndex = 0) {
		if (isOpen()) {
			return true;
		}
		hr(SimConnect_Open(&hSimConnect_, clientName_.c_str(), hWnd, userMessageId, windowsEventHandle, configIndex));
		if (succeeded()) {
			return true;
		}
		if (hr_ == E_INVALIDARG) { // Special case for bad config index
			throw BadConfig(std::format("Unknown configuration section {}.", configIndex));
		}
		return false;
	}


	/**
	 * Assure we close the connection to SimConnect cleanly.
	 */
	~Connection() { close(); }


public:
	Connection() : clientName_("SimConnect client") {}
	Connection(std::string name) : clientName_(name) {}

	// We don't want copied or moved Connections.
	Connection(const Connection&) = delete;
	Connection(Connection&&) = delete;
	Connection& operator=(const Connection&) = delete;
	Connection& operator=(Connection&&) = delete;


	/**
	 * Provides an implicit conversion to the SimConnect handle.
	 * @returns The SimConnect handle.
	 */
	operator HANDLE() const noexcept { return hSimConnect_; }


	/**
	 * Records an HRESULT in an assignment operator.
	 * 
	 * @param hr The HRESULT value.
	 * @returns a reference to the connection. (by convention)
	 */
	Connection& operator=(HRESULT hr) noexcept {
		hr_ = hr;
		return *this;
	}

	/**
	 * Provides an implicit conversion to a `bool` to check the result of the last call to SimConnect.
	 * @returns True if the last call to SimConnect was successful.
	 */
	operator bool() const noexcept { return succeeded(); }


	/**
	 * If the last call to SimConnect was successful, returns the SendID, otherwise returns the last error code.
	 * @returns The SendID if the last call to SimConnect was successful, otherwise returns the last error code.
	 */
	long fetchSendId() const {
		DWORD sendId{ 0 };

		if (SUCCEEDED(hr())) {
			DWORD sendId{ 0 };
			SimConnect_GetLastSentPacketID(hSimConnect_, &sendId);
			return sendId;
		}
		return hr();
	}


	// Calls to SimConnect

	// Category "General"
	//
	// NOTE There is no call to SimConnect_Open here, as the required parameters depend on the type of connection.

	/**
	 * Closes the connection.
	 * @throws SimConnectException if the call fails. This should only happen if the handle is invalid.
	 */
	void close() {
		if (isOpen()) {
			hr(SimConnect_Close(hSimConnect_));
			hSimConnect_ = nullptr;
			if (FAILED(hr_)) {
				throw SimConnectException("SimConnect_Close failed");
			}
		}
	}


	/**
	 * Gets the next incoming message that is waiting.
	 * 
	 * @param msgPtr The message.
	 * @param size The size of the message.
	 * @returns `S_OK` if successful, `E_FAIL` if none were available.
	 */
	int getNextDispatch(SIMCONNECT_RECV*& msgPtr, DWORD& size) {
		return hr(SimConnect_GetNextDispatch(hSimConnect_, &msgPtr, &size));
	}


	/**
	 * Requests a system state.
	 * @param stateName The name of the state to request.
	 * @returns The request ID used to identify the request.
	 */
	int requestSystemState(std::string stateName) {
		auto reqId{ ++requestID_ };

		hr(SimConnect_RequestSystemState(hSimConnect_, reqId, stateName.c_str()));

		return reqId;
	}

	// Category "Events and Data"

	HRESULT subscribeToSystemEvent(event event) {
		return hr(SimConnect_SubscribeToSystemEvent(hSimConnect_, event.id(), event.name().c_str()));
	}

};

}