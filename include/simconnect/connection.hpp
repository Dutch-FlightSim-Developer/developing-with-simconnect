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
#include <simconnect/simconnect.hpp>
#include <simconnect/simconnect_exception.hpp>
#include <simconnect/simconnect_error.hpp>

#include <simconnect/events/events.hpp>
#include <simconnect/events/notification_group.hpp>
#include <simconnect/requests/requests.hpp>

#include <simconnect/data/data_definitions.hpp>
#include <simconnect/data/init_position.hpp>
#include <simconnect/data_frequency.hpp>

#include <simconnect/util/null_logger.hpp>
#include <simconnect/util/statefull_object.hpp>

#include <span>
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
 * 
 * @tparam Derived The derived connection type.
 * @tparam ThreadSafe Whether to make the connection thread-safe.
 * @tparam L The logger type.
 */
template <class Derived, bool ThreadSafe = false, class L = NullLogger>
class Connection : public StateFullObject
{
public:
	using logger_type = L;
    using mutex_type = std::conditional_t<ThreadSafe, std::mutex, NoMutex>;
    using guard_type = std::conditional_t<ThreadSafe, std::lock_guard<mutex_type>, NoGuard>;


    /**
     * Opens the connection, optionally for a specific configuration.
     * 
     * @param configIndex The index of the configuration section to use, defaults to 0 meaning use the default configuration.
     * @returns A Result containing a reference to the Derived connection if successful, or an Error if failed.
     */
    Result<Derived&> open(int configIndex = 0) {
        return static_cast<Derived*>(this)->open(configIndex);
    }


private:
    std::string clientName_;			///< The name of the client.
	HANDLE hSimConnect_{ nullptr };		///< The SimConnect handle, if connected.

    mutex_type mutex_;


    logger_type logger_{ "SimConnect::Connection" }; ///< The logger for this connection.


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
     * Returns the logger.
     * 
     * @returns The logger.
     */
	logger_type& logger() noexcept { return logger_; }


protected:
	/**
	 * Opens the connection.
	 * 
	 * @param hWnd The window handle to receive the user messages.
	 * @param userMessageId The user message identifier.
	 * @param windowsEventHandle The Windows event handle.
	 * @param configIndex The index of the configuration section to use, defaults to 0 meaning use the default configuration.
	 */
	Derived& callOpen(HWND hWnd, DWORD userMessageId, HANDLE windowsEventHandle, unsigned configIndex = 0) {
        guard_type guard(mutex_);

        if (isOpen()) {
			return *static_cast<Derived*>(this);
		}
		state(SimConnect_Open(&hSimConnect_, clientName_.c_str(), hWnd, userMessageId, windowsEventHandle, configIndex));

		if (this->state() == E_INVALIDARG) { // Special case for bad config index
			logger_.error("Open called with unknown configuration section {}.", configIndex);
		}
        else if (failed()) {
            logger_.error("SimConnect_Open failed with error code 0x{:08X}.", this->state());
        }
		return *static_cast<Derived*>(this);
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
	 * If the last call to SimConnect was successful, returns the SendID, otherwise returns the last error code.
	 * @returns The SendID if the last call to SimConnect was successful, otherwise returns the last error code.
	 */
	[[nodiscard]]
	SendId fetchSendId() const {
		if (succeeded()) {
            guard_type guard(mutex_);

            SendId sendId{ 0 };
			SimConnect_GetLastSentPacketID(hSimConnect_, &sendId);
			return sendId;
		}
		return noId;
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
	Derived& close() {
        guard_type guard(mutex_);

        if (isOpen()) {
			state(SimConnect_Close(hSimConnect_));
			hSimConnect_ = nullptr;
			if (this->failed()) {
				logger_.error("SimConnect_Close failed with error code 0x{:08X}.", state());
			}
            
            // Clear all mapped event flags to allow re-mapping on reconnect
            event::clearAllMappedFlags();
		}
        return *static_cast<Derived*>(this);
	}


	/**
	 * Gets the next incoming message that is waiting.
	 * 
	 * @param msgPtr The message.
	 * @param size The size of the message.
	 * @returns `S_OK` if successful, `E_FAIL` if none were available.
	 */
	[[nodiscard]]
	bool getNextDispatch(Messages::MsgBase*& msgPtr, unsigned long& size) {
        guard_type guard(mutex_);

        if (!isOpen()) {
            state(E_FAIL);
            return false;
        }
		state(SimConnect_GetNextDispatch(hSimConnect_, &msgPtr, &size));

		return succeeded();
	}


    /**
     * The callback function used by SimConnect_CallDispatch.
     * 
     * @param pData The message data.
     * @param cbData The size of the message data.
     * @param pContext The context pointer, which is a pointer to the function to call.
     */
    static void CALLBACK dispatchCallback(Messages::MsgBase* pData, unsigned long cbData, void* pContext) {
        if (pContext != nullptr) {
            auto &func = *reinterpret_cast<std::function<void(const Messages::MsgBase*, unsigned long)>*>(pContext);
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
    bool callDispatch(std::function<void(const Messages::MsgBase*, unsigned long)> dispatchFunc) {
        guard_type guard(mutex_);

        if (!isOpen()) {
            state(E_FAIL);
            return false;
        }
        state(SimConnect_CallDispatch(hSimConnect_, &dispatchCallback, &dispatchFunc));

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
	Result<RequestId> requestSystemState(std::string stateName) {
		logger().trace("Requesting system state '{}'", stateName);

        auto requestId = requests().nextRequestID();

        guard_type guard(mutex_);

        state(SimConnect_RequestSystemState(hSimConnect_, requestId, stateName.c_str()));

		return Result<RequestId>(requestId, state(), "SimConnect_RequestSystemState failed");
	}


	/**
	 * Requests a system state.
	 * @param stateName The name of the state to request.
	 * @returns The request ID used to identify the request.
	 */
	Derived& requestSystemState(std::string stateName, RequestId requestId) {
        logger().trace("Requesting system state '{}'", stateName);

        guard_type guard(mutex_);

        state(SimConnect_RequestSystemState(hSimConnect_, requestId, stateName.c_str()));

        if (failed()) {
            logger_.error("SimConnect_RequestSystemState failed with error code 0x{:08X}.", state());
        }

        return *static_cast<Derived*>(this);
	}

#pragma endregion

#pragma region Notification Groups

    /**
     * Sets the priority of a notification group.
     * 
     * @param groupId The notification group ID.
     * @param priority The priority to set.
     */
    Derived& setNotificationGroupPriority(NotificationGroupId groupId, int priority) {
        logger().trace("Setting notification group ID {} priority to {}", groupId, priority);

        guard_type guard(mutex_);

        state(SimConnect_SetNotificationGroupPriority(hSimConnect_, groupId, priority));
        if (failed()) {
            logger_.error("SimConnect_SetNotificationGroupPriority failed with error code 0x{:08X}.", state());
        }
        return static_cast<Derived&>(*this);
    }


    /**
     * Adds a client event to a notification group.
     * 
     * @param groupId The notification group ID.
     * @param evt The event to add.
     * @param maskable True if the event is maskable.
     */
    Derived& addClientEventToNotificationGroup(NotificationGroupId groupId, event evt, bool maskable = false) {
        logger().trace("Adding event '{}' to notification group ID {} (maskable={})", evt.name(), groupId, maskable);

        guard_type guard(mutex_);

        state(SimConnect_AddClientEventToNotificationGroup(hSimConnect_, groupId, evt.id(), maskable ? 1 : 0));
        if (failed()) {
            logger_.error("SimConnect_AddClientEventToNotificationGroup failed with error code 0x{:08X}.", state());
        }
        return static_cast<Derived&>(*this);
    }


    /**
     * Removes a client event from a notification group.
     * 
     * @param groupId The notification group ID.
     * @param evt The event to remove.
     */
    Derived& removeClientEventFromNotificationGroup(NotificationGroupId groupId, event evt) {
        logger().trace("Removing event '{}' from notification group ID {}", evt.name(), groupId);

        guard_type guard(mutex_);

        state(SimConnect_RemoveClientEvent(hSimConnect_, groupId, evt.id()));
        if (failed()) {
            logger_.error("SimConnect_RemoveClientEvent failed with error code 0x{:08X}.", state());
        }
        return static_cast<Derived&>(*this);
    }


    /**
     * Clears all events from a notification group.
     * 
     * @param groupId The notification group ID.
     */
    Derived& clearNotificationGroup(NotificationGroupId groupId) {
        logger().trace("Clearing notification group ID {}", groupId);

        guard_type guard(mutex_);

        state(SimConnect_ClearNotificationGroup(hSimConnect_, groupId));
        if (failed()) {
            logger_.error("SimConnect_ClearNotificationGroup failed with error code 0x{:08X}.", state());
        }
        return static_cast<Derived&>(*this);
    }


    /**
     * Requests notification group information.
     * 
     * @param groupId The notification group ID.
     */
    Derived& requestNotificationGroup(NotificationGroupId groupId) {
        logger().trace("Requesting notification group ID {}", groupId);

        guard_type guard(mutex_);

        state(SimConnect_RequestNotificationGroup(hSimConnect_, groupId, 0, 0));
        if (failed()) {
            logger_.error("SimConnect_RequestNotificationGroup failed with error code 0x{:08X}.", state());
        }
        return static_cast<Derived&>(*this);
    }

#pragma endregion

#pragma region Events

    /**
     * Maps a client event to a simulator event.
     * The event will be mapped using its own name.
     * 
     * @param evt The event to map.
     */
    Derived& mapClientEvent(event evt) {
        // Check if already mapped to avoid Exceptions::eventIdDuplicate (exception 9)
        if (evt.isMapped()) {
            logger().trace("Event '{}' (ID {}) is already mapped, skipping", evt.name(), evt.id());
            return static_cast<Derived&>(*this);
        }

        logger().trace("Mapping client event ID {} to sim event '{}'", evt.id(), evt.name());

        guard_type guard(mutex_);

        state(SimConnect_MapClientEventToSimEvent(hSimConnect_, evt.id(), evt.name().c_str()));
        
        if (this->succeeded()) {
            evt.setMapped();
        } else {
            logger_.error("SimConnect_MapClientEventToSimEvent failed with error code 0x{:08X}.", state());
        }
        
        return static_cast<Derived&>(*this);
    }


    /**
     * Sends an Event.
     * 
     * @param objectId The object ID to send the event to.
     * @param evt The event to send.
     * @param groupId The notification group ID.
     * @param data The optional data to send with the event.
     */
    Derived& transmitClientEvent(SimObjectId objectId, event evt, NotificationGroupId groupId, unsigned long data = 0) {
        logger().trace("Transmitting client event '{}' to object ID {} in group ID {} with data {}",
            evt.name(), objectId, groupId, data);

        guard_type guard(mutex_);

        state(SimConnect_TransmitClientEvent(hSimConnect_, objectId, evt.id(), data, groupId, 0));
        if (failed()) {
            logger_.error("SimConnect_TransmitClientEvent failed with error code 0x{:08X}.", state());
        }
        return static_cast<Derived&>(*this);
    }


    /**
     * Sends an Event, not in a group, but with a priority.
     * 
     * @param objectId The object ID to send the event to.
     * @param evt The event to send.
     * @param priority The priority of the event.
     * @param data The optional data to send with the event.
     */
    Derived& transmitClientEventWithPriority(SimObjectId objectId, event evt, Events::Priority priority, unsigned long data = 0) {
        logger().trace("Transmitting client event '{}' to object ID {} with priority {} and data {}",
            evt.name(), objectId, priority, data);

        guard_type guard(mutex_);

        state(SimConnect_TransmitClientEvent(hSimConnect_, objectId, evt.id(), data, priority, Events::groupIdIsPriority));
        if (failed()) {
            logger_.error("SimConnect_TransmitClientEvent failed with error code 0x{:08X}.", state());
        }
        return static_cast<Derived&>(*this);
    }


    /**
     * Send an Event with more data.
     * 
     * @param objectId The object ID to send the event to.
     * @param evt The event to send.
     * @param groupId The notification group ID to send the event to.
     * @param data0 The first data value to send with the event.
     * @param data1 The second data value to send with the event.
     * @param data2 The third data value to send with the event.
     * @param data3 The fourth data value to send with the event.
     * @param data4 The fifth data value to send with the event.
     */
    Derived& transmitClientEvent(SimObjectId objectId, event evt, NotificationGroupId groupId, unsigned long data0, unsigned long data1, unsigned long data2 = 0, unsigned long data3 = 0, unsigned long data4 = 0) {
        logger().trace("Transmitting client event '{}' to object ID {} in group ID {} with data {}, {}, {}, {}, {}",
            evt.name(), objectId, groupId, data0, data1, data2, data3, data4);

        guard_type guard(mutex_);

        state(SimConnect_TransmitClientEvent_EX1(hSimConnect_, objectId, evt.id(), groupId, 0, data0, data1, data2, data3, data4));
        if (failed()) {
            logger_.error("SimConnect_TransmitClientEvent_EX1 failed with error code 0x{:08X}.", state());
        }
        return static_cast<Derived&>(*this);
    }


    /**
     * Send an Event with more data, not in a group, but with a priority.
     * 
     * @param objectId The object ID to send the event to.
     * @param evt The event to send.
     * @param priority The priority of the event.
     * @param data0 The first data value to send with the event.
     * @param data1 The second data value to send with the event.
     * @param data2 The third data value to send with the event.
     * @param data3 The fourth data value to send with the event.
     * @param data4 The fifth data value to send with the event.
     */
    Derived& transmitClientEventWithPriority(SimObjectId objectId, event evt, Events::Priority priority, unsigned long data0, unsigned long data1, unsigned long data2 = 0, unsigned long data3 = 0, unsigned long data4 = 0) {
        logger().trace("Transmitting client event '{}' to object ID {} with priority {} and data {}, {}, {}, {}, {}",
            evt.name(), objectId, priority, data0, data1, data2, data3, data4);

        guard_type guard(mutex_);

        state(SimConnect_TransmitClientEvent_EX1(hSimConnect_, objectId, evt.id(), priority, Events::groupIdIsPriority, data0, data1, data2, data3, data4));
        if (failed()) {
            logger_.error("SimConnect_TransmitClientEvent_EX1 failed with error code 0x{:08X}.", state());
        }
        return static_cast<Derived&>(*this);
    }

#pragma endregion

#pragma region System Events

    /**
     * Subscribe to an event.
     * @param event The event to subscribe to.
     */
	Derived& subscribeToSystemEvent(event event) {
        logger().trace("Subscribing to system event '{}'", event.name());

        guard_type guard(mutex_);

        state(SimConnect_SubscribeToSystemEvent(hSimConnect_, event.id(), event.name().c_str()));
        if (failed()) {
            logger_.error("SimConnect_SubscribeToSystemEvent failed with error code 0x{:08X}.", state());
        }
        return static_cast<Derived&>(*this);
	}

    
    /**
    * Unsubscribe from an event.
    * @param event The event to unsubscribe from.
    */
    Derived& unsubscribeFromSystemEvent(event event) {
        logger().trace("Unsubscribing to system event '{}'", event.name());

        guard_type guard(mutex_);

        state(SimConnect_UnsubscribeFromSystemEvent(hSimConnect_, event.id()));
        if (failed()) {
            logger_.error("SimConnect_UnsubscribeFromSystemEvent failed with error code 0x{:08X}.", state());
        }
        return static_cast<Derived&>(*this);
    }

#pragma endregion

#pragma region Input Groups

    /**
     * Sets the priority of an input group.
     * 
     * @param groupId The input group ID.
     * @param priority The priority to set.
     * @return The connection reference for chaining.
     */
    Derived& setInputGroupPriority(InputGroupId groupId, Events::Priority priority) {
        logger().trace("Setting input group ID {} priority to {}", groupId, priority);

        guard_type guard(mutex_);

        state(SimConnect_SetInputGroupPriority(hSimConnect_, groupId, priority));
        if (failed()) {
            logger_.error("SimConnect_SetInputGroupPriority failed with error code 0x{:08X}.", state());
        }
        return static_cast<Derived&>(*this);
    }


    /**
     * Sets the state of an input group.
     * 
     * @param groupId The input group ID.
     * @param groupState The state to set (Events::State::on or Events::State::off).
     * @return The connection reference for chaining.
     */
    Derived& setInputGroupState(InputGroupId groupId, Events::State groupState) {
        logger().trace("Setting input group ID {} state to {}", groupId, groupState);
        guard_type guard(mutex_);

        state(SimConnect_SetInputGroupState(hSimConnect_, groupId, groupState));
        if (failed()) {
            logger_.error("SimConnect_SetInputGroupState failed with error code 0x{:08X}.", state());
        }
        return static_cast<Derived&>(*this);
    }


    /**
     * Maps an input event to a client event.
     * 
     * @param evt The client event ID.
     * @param inputEvent The input event string (e.g., "VK_SPACE").
     * @param groupId The input group ID (optional, defaults to unused).
     * @return The connection reference for chaining.
     */
    Derived& mapInputEventToClientEvent(event evt, std::string inputEvent, InputGroupId groupId = unused) {
        logger().trace("Mapping input event '{}' to client event '{}' in group ID {}", inputEvent, evt.name(), groupId);

        guard_type guard(mutex_);

        state(SimConnect_MapInputEventToClientEvent_EX1(hSimConnect_, groupId, inputEvent.c_str(), evt.id()));
        if (failed()) {
            logger_.error("SimConnect_MapInputEventToClientEvent_EX1 failed with error code 0x{:08X}.", state());
        }
        return static_cast<Derived&>(*this);
    }


    /**
     * Adds a client event to an input group.
     * 
     * @param groupId The input group ID.
     * @param evt The event to add.
     * @param inputEvent The input event definition string.
     * @return The connection reference for chaining.
     */
    Derived& addClientEventToInputGroup(InputGroupId groupId, event evt, std::string inputEvent) {
        logger().trace("Adding event '{}' (input: '{}') to input group ID {}", evt.name(), inputEvent, groupId);

        guard_type guard(mutex_);

        state(SimConnect_MapInputEventToClientEvent_EX1(hSimConnect_, groupId, inputEvent.c_str(), evt.id()));
        if (failed()) {
            logger_.error("SimConnect_AddInputEvent failed with error code 0x{:08X}.", state());
        }
        return static_cast<Derived&>(*this);
    }


    /**
     * Removes an input event from an input group.
     * 
     * @param groupId The input group ID.
     * @param inputEvent The input event definition string.
     * @return The connection reference for chaining.
     */
    Derived& removeInputEvent(InputGroupId groupId, std::string inputEvent) {
        logger().trace("Removing input event '{}' from input group ID {}", inputEvent, groupId);

        guard_type guard(mutex_);

        state(SimConnect_RemoveInputEvent(hSimConnect_, groupId, inputEvent.c_str()));
        if (failed()) {
            logger_.error("SimConnect_RemoveInputEvent failed with error code 0x{:08X}.", state());
        }
        return static_cast<Derived&>(*this);
    }


    /**
     * Clears all input events from an input group.
     * 
     * @param groupId The input group ID.
     * @return The connection reference for chaining.
     */
    Derived& clearInputGroup(InputGroupId groupId) {
        logger().trace("Clearing input group ID {}", groupId);

        guard_type guard(mutex_);

        state(SimConnect_ClearInputGroup(hSimConnect_, groupId));
        if (failed()) {
            logger_.error("SimConnect_ClearInputGroup failed with error code 0x{:08X}.", state());
        }
        return static_cast<Derived&>(*this);
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
     * @param itemName The name of the simulation variable.
     * @param itemUnits The units for the simulation variable.
     * @param itemDataType The data type of the simulation variable.
     * @param itemEpsilon The epsilon value for change detection.
     * @param itemDatumId The datum ID for the item.
     */
    Derived& addDataDefinition(DataDefinitionId dataDef, const std::string& itemName, const std::string& itemUnits,
                           DataType itemDataType, float itemEpsilon = 0.0f, unsigned long itemDatumId = unused) {
		logger_.trace("Adding to data definition {}, simVar '{}', sendId = {}\n", dataDef, itemName, fetchSendId());

        guard_type guard(mutex_);

        state(SimConnect_AddToDataDefinition(hSimConnect_, dataDef,
            itemName.c_str(), itemUnits.empty() ? nullptr : itemUnits.c_str(),
            itemDataType,
            itemEpsilon,
            itemDatumId));
        if (failed()) {
            logger_.error("SimConnect_AddToDataDefinition failed with error code 0x{:08X}.", state());
        }
        return static_cast<Derived&>(*this);
	}

#pragma endregion

#pragma region Data Requests

    /**
     * Request data on the given object.
     * 
     * @param dataDef The data definition.
     * @param requestId The request ID.
     * @param frequency The frequency at which to request the data.
     * @param limits The limits for the request in numbers of "periods".
     * @param objectId The object ID to request data for. Defaults to the current user's Avatar or Aircraft.
     * @param onlyWhenChanged If true, the data will only be sent when it changes.
     */
    Derived& requestData(DataDefinitionId dataDef, RequestId requestId,
        DataFrequency frequency = DataFrequency::once(),
        PeriodLimits limits = PeriodLimits::none(),
        SimObjectId objectId = SimObject::userCurrent,
        bool onlyWhenChanged = false)
    {
        guard_type guard(mutex_);

        state(SimConnect_RequestDataOnSimObject(hSimConnect_, requestId, dataDef,
            objectId,
            frequency,
            onlyWhenChanged ? DataRequestFlags::whenChanged : DataRequestFlags::defaultFlag,
            limits.origin,
            frequency.interval,
            limits.limit));
		logger_.trace("Requested untagged data on SimObject {} with request ID {} and data definition {}, sendId = {}\n",
			objectId, requestId, dataDef, fetchSendId());
		
        if (failed()) {
            logger_.error("SimConnect_RequestDataOnSimObject failed with error code 0x{:08X}.", state());
        }
		return static_cast<Derived&>(*this);
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
    Derived& requestDataTagged(DataDefinitionId dataDef, RequestId requestId,
        DataFrequency frequency = DataFrequency::once(),
        PeriodLimits limits = PeriodLimits::none(),
        SimObjectId objectId = SimObject::userCurrent,
        bool onlyWhenChanged = false)
    {
        guard_type guard(mutex_);

        state(SimConnect_RequestDataOnSimObject(hSimConnect_, requestId, dataDef,
            objectId,
            frequency,
            (onlyWhenChanged ? DataRequestFlags::whenChanged : DataRequestFlags::defaultFlag) | DataRequestFlags::tagged,
            limits.origin,
            frequency.interval,
            limits.limit));

        logger_.trace("Requested tagged data on SimObject {} with request ID {} and data definition {}, sendId = {}\n",
			objectId, requestId, dataDef, fetchSendId());
			
        if (failed()) {
            logger_.error("SimConnect_RequestDataOnSimObject failed with error code 0x{:08X}.", state());
        }
		return static_cast<Derived&>(*this);
    }


    /**
     * Stops a data request.
     * 
     * @param dataDef The data definition ID.
     * @param requestId The request ID.
     * @param objectId The object ID to stop the request for. Defaults to the current user's Avatar or Aircraft.
     */
    Derived& stopDataRequest(DataDefinitionId dataDef, RequestId requestId, SimObjectId objectId = SimObject::userCurrent)
    {
        guard_type guard(mutex_);

        state(SimConnect_RequestDataOnSimObject(hSimConnect_, requestId, dataDef, objectId, DataPeriods::never));
        if (failed()) {
            logger_.error("SimConnect_RequestDataOnSimObject failed with error code 0x{:08X}.", state());
        }
        return static_cast<Derived&>(*this);
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
	Derived& requestDataByType(DataDefinitionId dataDef, RequestId requestId,
		unsigned long radiusInMeters, SimObjectType objectType)
	{
        guard_type guard(mutex_);

		state(SimConnect_RequestDataOnSimObjectType(hSimConnect_, requestId, dataDef, radiusInMeters, objectType));
        if (failed()) {
            logger_.error("SimConnect_RequestDataOnSimObjectType failed with error code 0x{:08X}.", state());
        }
        return static_cast<Derived&>(*this);
	}


    /**
     * Sends data to a SimObject.
     * 
     * @param dataDef The data definition ID.
     * @param objectId The object ID to send the data to.
     * @param data The data to send.
     */
    template <typename T>
    Derived& sendData(DataDefinitionId dataDef, SimObjectId objectId, const T& data)
    {
        logger_.trace("Setting data on SimObject ID {} with data definition ID {}, size {}", objectId, dataDef, sizeof(T));

        guard_type guard(mutex_);

        state(SimConnect_SetDataOnSimObject(hSimConnect_, dataDef, objectId, DataSetFlags::defaultFlag, 1, sizeof(T), const_cast<void*>(&data)));
        if (failed()) {
            logger_.error("SimConnect_SetDataOnSimObject failed with error code 0x{:08X}.", state());
        }
        return static_cast<Derived&>(*this);
    }


    /**
     * Sends raw data to a SimObject.
     * 
     * @param dataDef The data definition ID.
     * @param objectId The object ID to send the data to.
     * @param data The data to send.
     * @param count The number of data blocks to send. Defaults to 1.
     * @param blockSize The size of each data block, defaults to 0. If 0, the size is calculated as data.size() / count.
     */
    Derived& sendData(DataDefinitionId dataDef, SimObjectId objectId, std::span<const uint8_t> data, unsigned long count = 1, unsigned long blockSize = 0)
    {
        if (blockSize == 0) {
            blockSize = data.size() / count;
        }
        if (data.size() != blockSize * count) {
            logger_.error("Data size {} does not match count {} * blockSize {}", data.size(), count, blockSize);
            state(E_INVALIDARG);
            return static_cast<Derived&>(*this);
        }
        logger_.trace("Setting data on SimObject ID {} with data definition ID {}, size {}, count {}, blockSize {}",
            objectId, dataDef, data.size(), count, blockSize);

        guard_type guard(mutex_);

        state(SimConnect_SetDataOnSimObject(hSimConnect_, dataDef, objectId, DataSetFlags::defaultFlag, count, blockSize, const_cast<uint8_t*>(data.data())));
        if (failed()) {
            logger_.error("SimConnect_SetDataOnSimObject failed with error code 0x{:08X}.", state());
        }
        return static_cast<Derived&>(*this);
    }


    /**
     * Sends raw data to a SimObject.
     * 
     * @param dataDef The data definition ID.
     * @param objectId The object ID to send the data to.
     * @param data The data to send.
     * @param count The number of data blocks to send. Defaults to 1.
     * @param blockSize The size of each data block, defaults to 0. If 0, the size is calculated as data.size() / count.
     */
    Derived& sendDataTagged(DataDefinitionId dataDef, SimObjectId objectId, std::span<const uint8_t> data, unsigned long count = 1, unsigned long blockSize = 0)
    {
        if (blockSize == 0) {
            blockSize = data.size() / count;
        }
        if (data.size() != blockSize * count) {
            logger_.error("Data size {} does not match count {} * blockSize {}", data.size(), count, blockSize);
            state(E_INVALIDARG);
            return static_cast<Derived&>(*this);
        }
        logger_.trace("Setting data on SimObject ID {} with data definition ID {}, size {}, count {}, blockSize {}",
            objectId, dataDef, data.size(), count, blockSize);

        guard_type guard(mutex_);

        state(SimConnect_SetDataOnSimObject(hSimConnect_, dataDef, objectId, DataSetFlags::tagged, count, blockSize, const_cast<uint8_t*>(data.data())));
        if (failed()) {
            logger_.error("SimConnect_SetDataOnSimObject failed with error code 0x{:08X}.", state());
        }
        return static_cast<Derived&>(*this);
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
    Derived& createNonATCAircraft(std::string title, std::string tailNumber,
        Data::InitPosition initPos,
        RequestId requestId)
    {
        guard_type guard(mutex_);

        state(SimConnect_AICreateNonATCAircraft(hSimConnect_, title.c_str(), tailNumber.c_str(), initPos, requestId));
        if (failed()) {
            logger_.error("SimConnect_AICreateNonATCAircraft failed with error code 0x{:08X}.", state());
        }
        return static_cast<Derived&>(*this);
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
    Derived& createNonATCAircraft(std::string title, std::string livery, std::string tailNumber,
        Data::InitPosition initPos,
        RequestId requestId)
    {
        guard_type guard(mutex_);

        state(SimConnect_AICreateNonATCAircraft_EX1(hSimConnect_, title.c_str(), livery.c_str(), tailNumber.c_str(), initPos, requestId));
        if (failed()) {
            logger_.error("SimConnect_AICreateNonATCAircraft_EX1 failed with error code 0x{:08X}.", state());
        }
        return static_cast<Derived&>(*this);
    }


    /**
     * Creates a parked ATC aircraft. (pre-2024, no livery)
     * 
     * @param title The title of the aircraft container.
     * @param tailNumber The tail number of the aircraft.
     * @param airportIcao The ICAO of the airport to park at.
     * @param requestId The request ID.
     */
    Derived& createParkedAircraft(std::string title, std::string tailNumber, std::string airportIcao, RequestId requestId)
    {
        guard_type guard(mutex_);

        state(SimConnect_AICreateParkedATCAircraft(hSimConnect_, title.c_str(), tailNumber.c_str(), airportIcao.c_str(), requestId));
        if (failed()) {
            logger_.error("SimConnect_AICreateParkedATCAircraft failed with error code 0x{:08X}.", state());
        }
        return static_cast<Derived&>(*this);
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
    Derived& createParkedAircraft(std::string title, std::string livery, std::string tailNumber, std::string airportIcao, RequestId requestId)
    {
        guard_type guard(mutex_);

        state(SimConnect_AICreateParkedATCAircraft_EX1(hSimConnect_, title.c_str(), livery.c_str(), tailNumber.c_str(), airportIcao.c_str(), requestId));
        if (failed()) {
            logger_.error("SimConnect_AICreateParkedATCAircraft_EX1 failed with error code 0x{:08X}.", state());
        }
        return static_cast<Derived&>(*this);
    }


    /**
     * Creates a SimObject. (pre-2024, no livery)
     * 
     * @param title The title of the aircraft container.
     * @param initPos The initial position of the SimObject.
     * @param requestId The request ID.
     */
    Derived& createSimObject(std::string title, Data::InitPosition initPos, RequestId requestId)
    {
        guard_type guard(mutex_);

        state(SimConnect_AICreateSimulatedObject(hSimConnect_, title.c_str(), initPos, requestId));
        if (failed()) {
            logger_.error("SimConnect_AICreateSimulatedObject failed with error code 0x{:08X}.", state());
        }
        return static_cast<Derived&>(*this);
    }


    /**
     * Creates a SimObject. (2024 and later, with livery)
     * 
     * @param title The title of the aircraft container.
     * @param livery The livery of the aircraft.
     * @param initPos The initial position of the SimObject.
     * @param requestId The request ID.
     */
    Derived& createSimObject(std::string title, std::string livery, Data::InitPosition initPos, RequestId requestId)
    {
        guard_type guard(mutex_);

        state(SimConnect_AICreateSimulatedObject_EX1(hSimConnect_, title.c_str(), livery.c_str(), initPos, requestId));
        if (failed()) {
            logger_.error("SimConnect_AICreateSimulatedObject_EX1 failed with error code 0x{:08X}.", state());
        }
        return static_cast<Derived&>(*this);
    }


    /**
     * Removes a SimObject.
     * 
     * @param objectId The object ID of the SimObject to remove.
     * @param requestId The request ID.
     */
    Derived& removeSimObject(SimObjectId objectId, RequestId requestId)
    {
        guard_type guard(mutex_);

        state(SimConnect_AIRemoveObject(hSimConnect_, objectId, requestId));
        if (failed()) {
            logger_.error("SimConnect_AIRemoveObject failed with error code 0x{:08X}.", state());
        }
        return static_cast<Derived&>(*this);
    }

#pragma endregion
};

} // namespace SimConnect