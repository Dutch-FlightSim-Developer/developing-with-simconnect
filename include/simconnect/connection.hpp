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
#include <simconnect/requests/requests.hpp>
#include <simconnect/data/data_definitions.hpp>
#include <simconnect/data_frequency.hpp>

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
	[[nodiscard]]
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
	[[nodiscard]]
	bool succeeded() const noexcept { return SUCCEEDED(hr()); }


    /**
	 * Returns true if the last call to SimConnect failed.
	 * @returns True if the last call to SimConnect failed.
	 */
	[[nodiscard]]
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
	bool callOpen(HWND hWnd, DWORD userMessageId, HANDLE windowsEventHandle, DWORD configIndex = 0) {
		if (isOpen()) {
			return true;
		}
		hr(SimConnect_Open(&hSimConnect_, clientName_.c_str(), hWnd, userMessageId, windowsEventHandle, configIndex));

		if (hr_ == E_INVALIDARG) { // Special case for bad config index
			throw BadConfig(std::format("Unknown configuration section {}.", configIndex));
		}
		return succeeded();
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
	[[nodiscard]]
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
	[[nodiscard]]
	operator bool() const noexcept { return succeeded(); }


	/**
	 * If the last call to SimConnect was successful, returns the SendID, otherwise returns the last error code.
	 * @returns The SendID if the last call to SimConnect was successful, otherwise returns the last error code.
	 */
	[[nodiscard]]
	long fetchSendId() const {
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
	[[nodiscard]]
	bool getNextDispatch(SIMCONNECT_RECV*& msgPtr, DWORD& size) {
		hr(SimConnect_GetNextDispatch(hSimConnect_, &msgPtr, &size));

		return succeeded();
	}


    /**
     * Request IDs are managed by the Requests class.
     * 
     * @returns The Requests object.
     */
    [[nodiscard]]
    Requests& requests() {
        static Requests requests;

        return requests;
    }


	/**
	 * Requests a system state.
	 * @param stateName The name of the state to request.
	 * @returns The request ID used to identify the request.
	 */
    [[nodiscard]]
	int requestSystemState(std::string stateName) {
        auto requestId = requests().nextRequestID();

		hr(SimConnect_RequestSystemState(hSimConnect_, requestId, stateName.c_str()));

		return requestId;
	}


	/**
	 * Requests a system state.
	 * @param stateName The name of the state to request.
	 * @returns The request ID used to identify the request.
	 */
	void requestSystemState(std::string stateName, unsigned long requestId) {
		hr(SimConnect_RequestSystemState(hSimConnect_, requestId, stateName.c_str()));
	}

	// Category "Events and Data"

    /**
     * Subscribe to an event.
     * @param event The event to subscribe to.
     */
	void subscribeToSystemEvent(event event) {
		hr(SimConnect_SubscribeToSystemEvent(hSimConnect_, event.id(), event.name().c_str()));
	}

    
    /**
    * Unsubscribe from an event.
    * @param event The event to unsubscribe from.
    */
    void unsubscribeFromSystemEvent(event event) {
        hr(SimConnect_UnsubscribeFromSystemEvent(hSimConnect_, event.id()));
    }


    /**
     * Data Definitions are managed by the DataDefinitions class.
     */
    [[nodiscard]]
    DataDefinitions& dataDefinitions() {
        static DataDefinitions dataDefs;

        return dataDefs;
    }


    /**
     * Add a data item to a data definition.
     * @param dataDef The data definition to add the item to.
     * @param item The data item to add.
     */
    void addDataDefinition(SIMCONNECT_DATA_DEFINITION_ID dataDef, const std::string& itemName, const std::string& itemUnits,
                           SIMCONNECT_DATATYPE itemDataType, float itemEpsilon = 0.0f, unsigned long itemDatumId = SIMCONNECT_UNUSED) {
        hr(SimConnect_AddToDataDefinition(hSimConnect_, dataDef,
            itemName.c_str(), itemUnits.empty() ? nullptr : itemUnits.c_str(),
            itemDataType,
            itemEpsilon,
            itemDatumId));
    }


    /**
     * Request data on the given object.
     * @param dataDef The data definition.
     * @param requestId The request ID.
     * @param frequency The frequency at which to request the data.
     * @param objectId The object ID to request data for. Defaults to the current user's Avatar or Aircraft.
     * @param onlyWhenChanged If true, the data will only be sent when it changes.
     */
    void requestData(SIMCONNECT_DATA_DEFINITION_ID dataDef, unsigned long requestId,
        DataFrequency frequency = DataFrequency::once(),
        unsigned long objectId = SIMCONNECT_OBJECT_ID_USER_CURRENT,
        bool onlyWhenChanged = false)
    {
        hr(SimConnect_RequestDataOnSimObject(hSimConnect_, requestId, dataDef,
            objectId,
            frequency.period,
            onlyWhenChanged ? SIMCONNECT_DATA_REQUEST_FLAG_CHANGED : 0,
            frequency.origin,
            frequency.interval,
            frequency.limit));
    }


    /**
     * Request data on the given object. The data must be returned in a tagged format.
     * @param dataDef The data definition.
     * @param requestId The request ID.
     * @param frequency The frequency at which to request the data.
     * @param objectId The object ID to request data for. Defaults to the current user's Avatar or Aircraft.
     * @param onlyWhenChanged If true, the data will only be sent when it changes.
     */
    void requestDataTagged(SIMCONNECT_DATA_DEFINITION_ID dataDef, unsigned long requestId,
        DataFrequency frequency = DataFrequency::once(),
        unsigned long objectId = SIMCONNECT_OBJECT_ID_USER_CURRENT,
        bool onlyWhenChanged = false)
    {
        hr(SimConnect_RequestDataOnSimObject(hSimConnect_, requestId, dataDef,
            objectId,
            frequency.period,
            (onlyWhenChanged ? SIMCONNECT_DATA_REQUEST_FLAG_CHANGED : 0) | SIMCONNECT_DATA_REQUEST_FLAG_TAGGED,
            frequency.origin,
            frequency.interval,
            frequency.limit));
    }


    /**
     * Stops a data request.
     * 
     * @param dataDef The data definition ID.
     * @param requestId The request ID.
     * @param objectId The object ID to stop the request for. Defaults to the current user's Avatar or Aircraft.
     */
    void stopDataRequest(SIMCONNECT_DATA_DEFINITION_ID dataDef, unsigned long requestId,
        unsigned long objectId = SIMCONNECT_OBJECT_ID_USER_CURRENT)
    {
        hr(SimConnect_RequestDataOnSimObject(hSimConnect_, requestId, dataDef, objectId, SIMCONNECT_PERIOD_NEVER));
    }
};

} // namespace SimConnect