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

#include <simconnect/events/events.hpp>
#include <simconnect/requests/requests.hpp>
#include <simconnect/data/data_definitions.hpp>
#include <simconnect/data/init_position.hpp>
#include <simconnect/data_frequency.hpp>

#include <string>

#include <type_traits>
#include <mutex>

#if !defined(NDEBUG)
#include <format>
#include <iostream>
#endif


namespace SimConnect {


class NoMutex
{
public:
    void lock() noexcept {}
    void unlock() noexcept {}
    bool try_lock() noexcept { return true; }
};

class NoGuard
{
public:
	NoGuard() noexcept {}
    explicit NoGuard(NoMutex&) noexcept {}
    explicit NoGuard(const NoMutex&) noexcept {}
};


/**
 * A SimConnect connection.
 */
template <bool ThreadSafe = false>
class Connection
{
public:
    using mutex_type = std::conditional_t<ThreadSafe, std::mutex, NoMutex>;
    using guard_type = std::conditional_t<ThreadSafe, std::lock_guard<mutex_type>, NoGuard>;


private:
    std::string clientName_;			///< The name of the client.
	HANDLE hSimConnect_{ nullptr };		///< The SimConnect handle, if connected.
	HRESULT hr_{ S_OK };				///< The last error code.

    mutex_type mutex_;


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
        guard_type guard(mutex_);

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
            guard_type guard(mutex_);

            DWORD sendId{ 0 };
			SimConnect_GetLastSentPacketID(hSimConnect_, &sendId);
			return sendId;
		}
		return hr();
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


	// Calls to SimConnect

#pragma region General

    // NOTE There is no call to SimConnect_Open here, as the required parameters depend on the type of connection.

	/**
	 * Closes the connection.
	 * @throws SimConnectException if the call fails. This should only happen if the handle is invalid.
	 */
	void close() {
        guard_type guard(mutex_);

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
        guard_type guard(mutex_);

        if (!isOpen()) {
            hr(E_FAIL);
            return false;
        }
		hr(SimConnect_GetNextDispatch(hSimConnect_, &msgPtr, &size));

		return succeeded();
	}


    /**
     * The callback function used by SimConnect_CallDispatch.
     * 
     * @param pData The message data.
     * @param cbData The size of the message data.
     * @param pContext The context pointer, which is a pointer to the function to call.
     */
    static void CALLBACK dispatchCallback(SIMCONNECT_RECV* pData, DWORD cbData, void* pContext) {
        if (pContext != nullptr) {
            auto &func = *reinterpret_cast<std::function<void(const SIMCONNECT_RECV*, DWORD)>*>(pContext);
            if (func) {
                func(pData, cbData);
            }
        }
    }


    /**
     * Calls the given function for the next message.
     * 
     * @param dispatchFunc The function to call for the next message.
     * @returns True if the call was successful.
     */
    [[nodiscard]]
    bool callDispatch(std::function<void(const SIMCONNECT_RECV*, DWORD)> dispatchFunc) {
        guard_type guard(mutex_);

        if (!isOpen()) {
            hr(E_FAIL);
            return false;
        }
        hr(SimConnect_CallDispatch(hSimConnect_, &dispatchCallback, &dispatchFunc));

        return succeeded();
    }

#pragma endregion
#pragma region System State

	/**
	 * Requests a system state.
	 * @param stateName The name of the state to request.
	 * @returns The request ID used to identify the request.
	 */
    [[nodiscard]]
	int requestSystemState(std::string stateName) {
        auto requestId = requests().nextRequestID();

        guard_type guard(mutex_);

        hr(SimConnect_RequestSystemState(hSimConnect_, requestId, stateName.c_str()));

		return requestId;
	}


	/**
	 * Requests a system state.
	 * @param stateName The name of the state to request.
	 * @returns The request ID used to identify the request.
	 */
	void requestSystemState(std::string stateName, unsigned long requestId) {
        guard_type guard(mutex_);

        hr(SimConnect_RequestSystemState(hSimConnect_, requestId, stateName.c_str()));
	}

#pragma endregion
#pragma region System Events

    /**
     * Subscribe to an event.
     * @param event The event to subscribe to.
     */
	void subscribeToSystemEvent(event event) {
        guard_type guard(mutex_);

        hr(SimConnect_SubscribeToSystemEvent(hSimConnect_, event.id(), event.name().c_str()));
	}

    
    /**
    * Unsubscribe from an event.
    * @param event The event to unsubscribe from.
    */
    void unsubscribeFromSystemEvent(event event) {
        guard_type guard(mutex_);

        hr(SimConnect_UnsubscribeFromSystemEvent(hSimConnect_, event.id()));
    }

#pragma endregion
#pragma region Data Definitions

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
        guard_type guard(mutex_);

        hr(SimConnect_AddToDataDefinition(hSimConnect_, dataDef,
            itemName.c_str(), itemUnits.empty() ? nullptr : itemUnits.c_str(),
            itemDataType,
            itemEpsilon,
            itemDatumId));
#if !defined(NDEBUG)
		std::cerr << std::format("Added to data definition {}, simVar '{}', sendId = {}\n",
			dataDef, itemName, fetchSendId());
#endif
	}

#pragma endregion
#pragma region Data Requests

    /**
     * Request data on the given object.
     * @param dataDef The data definition.
     * @param requestId The request ID.
     * @param frequency The frequency at which to request the data.
     * @param limits The limits for the request in numbers of "periods".
     * @param objectId The object ID to request data for. Defaults to the current user's Avatar or Aircraft.
     * @param onlyWhenChanged If true, the data will only be sent when it changes.
     */
    void requestData(SIMCONNECT_DATA_DEFINITION_ID dataDef, unsigned long requestId,
        DataFrequency frequency = DataFrequency::once(),
        PeriodLimits limits = PeriodLimits::none(),
        unsigned long objectId = SIMCONNECT_OBJECT_ID_USER_CURRENT,
        bool onlyWhenChanged = false)
    {
        guard_type guard(mutex_);

        hr(SimConnect_RequestDataOnSimObject(hSimConnect_, requestId, dataDef,
            objectId,
            frequency,
            onlyWhenChanged ? SIMCONNECT_DATA_REQUEST_FLAG_CHANGED : 0,
            limits.origin,
            frequency.interval,
            limits.limit));
#if !defined(NDEBUG)
		std::cerr << std::format("Requested untagged data on SimObject {} with request ID {} and data definition {}, sendId = {}\n",
			objectId, requestId, dataDef, fetchSendId());
#endif
	}


    /**
     * Request data on the given object. The data must be returned in a tagged format.
     * @param dataDef The data definition.
     * @param requestId The request ID.
     * @param frequency The frequency at which to request the data.
     * @param limits The limits for the request in numbers of "periods".
     * @param objectId The object ID to request data for. Defaults to the current user's Avatar or Aircraft.
     * @param onlyWhenChanged If true, the data will only be sent when it changes.
     */
    void requestDataTagged(SIMCONNECT_DATA_DEFINITION_ID dataDef, unsigned long requestId,
        DataFrequency frequency = DataFrequency::once(),
        PeriodLimits limits = PeriodLimits::none(),
        unsigned long objectId = SIMCONNECT_OBJECT_ID_USER_CURRENT,
        bool onlyWhenChanged = false)
    {
        guard_type guard(mutex_);

        hr(SimConnect_RequestDataOnSimObject(hSimConnect_, requestId, dataDef,
            objectId,
            frequency,
            (onlyWhenChanged ? SIMCONNECT_DATA_REQUEST_FLAG_CHANGED : 0) | SIMCONNECT_DATA_REQUEST_FLAG_TAGGED,
            limits.origin,
            frequency.interval,
            limits.limit));
#if !defined(NDEBUG)
		std::cerr << std::format("Requested tagged data on SimObject {} with request ID {} and data definition {}, sendId = {}\n",
			objectId, requestId, dataDef, fetchSendId());
#endif
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
        guard_type guard(mutex_);

        hr(SimConnect_RequestDataOnSimObject(hSimConnect_, requestId, dataDef, objectId, SIMCONNECT_PERIOD_NEVER));
    }


	/**
	 * Requests data for all SimObjects of a specific type.
	 * 
	 * @note An "OutOfBounds" exception message will be sent if the radius exceeds the maximum allowed, which is 200,000 meters or 200 km.
	 * 
	 * @param dataDef The data definition ID to use for the request.
	 * @param requestId The request ID.
	 * @param radiusInMeters The radius in meters to request data for. If 0, only the user's aircraft is in scope.
	 * @param objectType The type of SimObject to request data for.
	 */
	void requestDataByType(SIMCONNECT_DATA_DEFINITION_ID dataDef, unsigned long requestId,
		unsigned long radiusInMeters, SIMCONNECT_SIMOBJECT_TYPE objectType)
	{
        guard_type guard(mutex_);

		hr(SimConnect_RequestDataOnSimObjectType(hSimConnect_, requestId, dataDef, radiusInMeters, objectType));
	}

#pragma endregion
#pragma region AI

    /**
     * Creates a non-ATC aircraft. (pre-2024, no livery)
     * 
     * @param title The title of the aircraft container.
     * @param tailNumber The tail number of the aircraft.
     * @param initPos The initial position of the aircraft.
     * @param requestId The request ID.
     */
    void createNonATCAircraft(std::string title, std::string tailNumber,
        Data::InitPosition initPos,
        unsigned long requestId)
    {
        guard_type guard(mutex_);

        hr(SimConnect_AICreateNonATCAircraft(hSimConnect_, title.c_str(), tailNumber.c_str(), initPos, requestId));
    }


    /**
     * Creates a non-ATC aircraft. (2024 and later, with livery)
     * 
     * @param title The title of the aircraft container.
     * @param livery The livery of the aircraft.
     * @param tailNumber The tail number of the aircraft.
     * @param initPos The initial position of the aircraft.
     * @param requestId The request ID.
     */
    void createNonATCAircraft(std::string title, std::string livery, std::string tailNumber,
        Data::InitPosition initPos,
        unsigned long requestId)
    {
        guard_type guard(mutex_);

        hr(SimConnect_AICreateNonATCAircraft_EX1(hSimConnect_, title.c_str(), livery.c_str(), tailNumber.c_str(), initPos, requestId));
    }


    /**
     * Creates a parked ATC aircraft. (pre-2024, no livery)
     * 
     * @param title The title of the aircraft container.
     * @param tailNumber The tail number of the aircraft.
     * @param airportIcao The ICAO of the airport to park at.
     * @param requestId The request ID.
     */
    void createParkedAircraft(std::string title, std::string tailNumber, std::string airportIcao, unsigned long requestId)
    {
        guard_type guard(mutex_);

        hr(SimConnect_AICreateParkedATCAircraft(hSimConnect_, title.c_str(), tailNumber.c_str(), airportIcao.c_str(), requestId));
    }


    /**
     * Creates a parked ATC aircraft. (2024 and later, with livery)
     * 
     * @param title The title of the aircraft container.
     * @param livery The livery of the aircraft.
     * @param tailNumber The tail number of the aircraft.
     * @param airportIcao The ICAO of the airport to park at.
     * @param requestId The request ID.
     */
    void createParkedAircraft(std::string title, std::string livery, std::string tailNumber, std::string airportIcao, unsigned long requestId)
    {
        guard_type guard(mutex_);

        hr(SimConnect_AICreateParkedATCAircraft_EX1(hSimConnect_, title.c_str(), livery.c_str(), tailNumber.c_str(), airportIcao.c_str(), requestId));
    }


    /**
     * Creates a SimObject. (pre-2024, no livery)
     * 
     * @param title The title of the aircraft container.
     * @param initPos The initial position of the SimObject.
     * @param requestId The request ID.
     */
    void createSimObject(std::string title, Data::InitPosition initPos, unsigned long requestId)
    {
        guard_type guard(mutex_);

        hr(SimConnect_AICreateSimulatedObject(hSimConnect_, title.c_str(), initPos, requestId));
    }


    /**
     * Creates a SimObject. (2024 and later, with livery)
     * 
     * @param title The title of the aircraft container.
     * @param livery The livery of the aircraft.
     * @param initPos The initial position of the SimObject.
     * @param requestId The request ID.
     */
    void createSimObject(std::string title, std::string livery, Data::InitPosition initPos, unsigned long requestId)
    {
        guard_type guard(mutex_);

        hr(SimConnect_AICreateSimulatedObject_EX1(hSimConnect_, title.c_str(), livery.c_str(), initPos, requestId));
    }


    /**
     * Removes a SimObject.
     * 
     * @param objectId The object ID of the SimObject to remove.
     */
    void removeSimObject(unsigned long objectId, unsigned long requestId)
    {
        guard_type guard(mutex_);

        hr(SimConnect_AIRemoveObject(hSimConnect_, objectId, requestId));
    }

#pragma endregion
};

} // namespace SimConnect