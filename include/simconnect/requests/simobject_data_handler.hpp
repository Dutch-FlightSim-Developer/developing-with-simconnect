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

#include <unordered_map>

#include <simconnect/simobject_type.hpp>
#include <simconnect/message_handler.hpp>


namespace SimConnect {


/**
 * The SimObjectIdHolder class is a simple base class that holds the ID of a SimObject.
 * It is used by the requestDataByType methods to return the ID of the SimObject that the data was requested for.
 */
struct SimObjectIdHolder {
    unsigned long objectId{ 0 };

    constexpr SimObjectIdHolder() = default;
    constexpr SimObjectIdHolder(unsigned long id) : objectId(id) {}
	SimObjectIdHolder(const SIMCONNECT_RECV_SIMOBJECT_DATA& msg) : objectId(msg.dwObjectID) {}
	SimObjectIdHolder(const SimObjectIdHolder&) = default;
    SimObjectIdHolder(SimObjectIdHolder&&) = default;
    SimObjectIdHolder& operator=(const SimObjectIdHolder&) = default;
    SimObjectIdHolder& operator=(SimObjectIdHolder&&) = default;
};


/**
 * The SimObjectDataHandler class provides for responsive handling of SIMCONNECT_RECV_SIMOBJECT_DATA and
 * SIMCONNECT_RECV_SIMOBJECT_DATA_BYTYPE messages.
 * 
 * @note This handler is used to request data from the simulator for a specific object or type.
 * 
 * @tparam M The type of the SimConnect message handler, which must be derived from SimConnectMessageHandler.
 */
template <class M>
class SimObjectDataHandler : public MessageHandler<SIMCONNECT_DATA_REQUEST_ID, SimObjectDataHandler<M>, M, SIMCONNECT_RECV_ID_SIMOBJECT_DATA, SIMCONNECT_RECV_ID_SIMOBJECT_DATA_BYTYPE>
{
public:
    using simconnect_message_handler_type = M;
    using connection_type = typename M::connection_type;

private:
    // No copies or moves
    SimObjectDataHandler(const SimObjectDataHandler&) = delete;
    SimObjectDataHandler(SimObjectDataHandler&&) = delete;
    SimObjectDataHandler& operator=(const SimObjectDataHandler&) = delete;
    SimObjectDataHandler& operator=(SimObjectDataHandler&&) = delete;


public:
    SimObjectDataHandler() = default;
    ~SimObjectDataHandler() = default;


    /**
     * Returns the request ID from the message. This is specific to the SIMCONNECT_RECV_SIMOBJECT_DATA and
	 * SIMCONNECT_RECV_SIMOBJ_DATA_BTYPE messages. The latter type does not actually add fields, so we can
     * use the same method for both.
     * 
     * @param msg The message to get the correlation ID from.
     * @returns The correlation ID from the message.
	 */
    unsigned long correlationId(const SIMCONNECT_RECV& msg) const {
        return static_cast<const SIMCONNECT_RECV_SIMOBJECT_DATA&>(msg).dwRequestID;
    }


    /**
     * Stops a data request and removes the handler if still active.
     * 
     * @param connection The connection to stop the request on.
     * @param dataDef The data definition ID to stop the request for.
     * @param requestId The request ID to stop.
     * @param objectId The object ID to stop the request for. Defaults to the current user's Avatar or Aircraft.
     * @note If the request is not active, this will do nothing.
     */
    void stopDataRequest(connection_type& connection,
        SIMCONNECT_DATA_DEFINITION_ID dataDef,
        unsigned long requestId,
        unsigned long objectId = SIMCONNECT_OBJECT_ID_USER_CURRENT)
    {
        this->removeHandler(requestId);
        connection.stopDataRequest(requestId, dataDef, objectId);
    }


    // Requesting data for specific SimObjects.

    // Three groups of requestData methods are provided, depending on how the caller wants to receive the data:
    // 1. As a raw message data structure (SIMCONNECT_RECV_SIMOBJECT_DATA).
    // 2. As a DataBlockReader that can be used to read the data.
    // 3. As a struct (or class), where the setters and getters are used.
    //
    // For each group, there are methods to request the data once or repeatedly, and tagged. (again once or repeatedly)

    // First, request the data and pass a handler that will receive the raw message data.

    /**
     * Requests data. The caller passes a handler that will be executed once the
     * data is received. The handler will receive a const reference to the (raw) message data.
     * 
     * @note Discarding or deleting the Request object will stop the request.
     * 
     * @param connection The connection to request the data from.
     * @param dataDef The data definition Id to use for the request.
     * @param handler The handler to execute when the data is received.
     * @param frequency The frequency at which to request the data.
     * @param limits The limits for the request in numbers of "periods".
     * @param objectId The object ID to request data for. Defaults to the current user's Avatar or Aircraft.
     * @param onlyWhenChanged If true, the data will only be requested when it has changed.
     * @return A Request object that can be used to stop the request.
     */
    [[nodiscard]]
    Request requestData(connection_type& connection, SIMCONNECT_DATA_DEFINITION_ID dataDef,
        std::function<void(const SIMCONNECT_RECV_SIMOBJECT_DATA&)> handler,
        DataFrequency frequency = DataFrequency::once(),
        PeriodLimits limits = PeriodLimits::none(),
        unsigned long objectId = SIMCONNECT_OBJECT_ID_USER_CURRENT,
        bool onlyWhenChanged = false)
    {
        auto requestId = connection.requests().nextRequestID();

        this->registerHandler(requestId, [requestId, handler](const SIMCONNECT_RECV& msg) {
                handler(reinterpret_cast<const SIMCONNECT_RECV_SIMOBJECT_DATA&>(msg));
            }, frequency.isOnce());
        connection.requestData(dataDef, requestId, frequency, limits, objectId, onlyWhenChanged);

        return frequency.isOnce() ? Request{requestId} : Request{ requestId, [this, requestId, &connection, &dataDef, objectId]() {
            stopDataRequest(connection, dataDef, requestId, objectId);
        }};
    }


    /**
     * Requests data once. The caller passes a handler that will be executed once the
     * data is received. The handler will receive a const reference to the (raw) message data.
     * 
     * @note Discarding or deleting the Request object will stop the request.
     * 
     * @param connection The connection to request the data from.
     * @param dataDef The data definition Id to use for the request.
     * @param handler The handler to execute when the data is received.
     * @param objectId The object ID to request data for. Defaults to the current user's Avatar or Aircraft.
     * @return A Request object that can be used to stop the request.
     */
    [[nodiscard]]
    Request requestDataOnce(connection_type& connection, SIMCONNECT_DATA_DEFINITION_ID dataDef,
        std::function<void(const SIMCONNECT_RECV_SIMOBJECT_DATA&)> handler,
        unsigned long objectId = SIMCONNECT_OBJECT_ID_USER_CURRENT)
    {
        return requestData(connection, dataDef, handler, DataFrequency::once(), PeriodLimits::none(), objectId, false);
    }


    /**
     * Requests data. The caller passes a handler that will be executed once the
     * data is received. The handler will receive a const reference to the (raw) message data.
     * 
     * @note Discarding or deleting the Request object will stop the request.
     * 
     * @param connection The connection to request the data from.
     * @param dataDef The data definition Id to use for the request.
     * @param handler The handler to execute when the data is received.
     * @param frequency The frequency at which to request the data.
     * @param limits The limits for the request in numbers of "periods".
     * @param objectId The object ID to request data for. Defaults to the current user's Avatar or Aircraft.
     * @param onlyWhenChanged If true, the data will only be requested when it has changed.
     * @return A Request object that can be used to stop the request.
     */
    [[nodiscard]]
    Request requestDataTagged(connection_type& connection, SIMCONNECT_DATA_DEFINITION_ID dataDef,
        std::function<void(const SIMCONNECT_RECV_SIMOBJECT_DATA&)> handler,
        DataFrequency frequency = DataFrequency::once(),
        PeriodLimits limits = PeriodLimits::none(),
        unsigned long objectId = SIMCONNECT_OBJECT_ID_USER_CURRENT,
        bool onlyWhenChanged = false)
    {
        auto requestId = connection.requests().nextRequestID();

        this->registerHandler(requestId, [requestId, handler](const SIMCONNECT_RECV& msg) {
                handler(reinterpret_cast<const SIMCONNECT_RECV_SIMOBJECT_DATA&>(msg));
            }, frequency.isOnce());
        connection.requestDataTagged(dataDef, requestId, frequency, limits, objectId, onlyWhenChanged);

        return frequency.isOnce() ? Request{requestId} : Request{ requestId, [this, requestId, &connection, &dataDef, objectId]() {
            stopDataRequest(connection, dataDef, requestId, objectId);
        }};
    }


    /**
     * Requests data once. The caller passes a handler that will be executed once the
     * data is received. The handler will receive a const reference to the (raw) message data.
     * 
     * @note Discarding or deleting the Request object will stop the request.
     * 
     * @param connection The connection to request the data from.
     * @param dataDef The data definition Id to use for the request.
     * @param handler The handler to execute when the data is received.
     * @param objectId The object ID to request data for. Defaults to the current user's Avatar or Aircraft.
     * @return A Request object that can be used to stop the request.
     */
    [[nodiscard]]
    Request requestDataOnceTagged(connection_type& connection, SIMCONNECT_DATA_DEFINITION_ID dataDef,
        std::function<void(const SIMCONNECT_RECV_SIMOBJECT_DATA&)> handler,
        unsigned long objectId = SIMCONNECT_OBJECT_ID_USER_CURRENT)
    {
        return requestDataTagged(connection, dataDef, handler, DataFrequency::once(), PeriodLimits::none(), objectId, false);
    }


    // Next, request the data and pass a handler that will receive a DataBlockReader to read the data.

    /**
     * Requests data. The caller passes a handler that will be executed once the
     * data is received. The handler will receive a reference to a DataBlockReader that can be used to read the data.
     * 
     * @note Discarding or deleting the Request object will stop the request.
     * 
     * @param connection The connection to request the data from.
     * @param dataDef The data definition Id to use for the request.
     * @param handler The handler to execute when the data is received.
     * @param frequency The frequency at which to request the data.
     * @param limits The limits for the request in numbers of "periods".
     * @param objectId The object ID to request data for. Defaults to the current user's Avatar or Aircraft.
     * @param onlyWhenChanged If true, the data will only be requested when it has changed.
     * @return A Request object that can be used to stop the request.
     */
    [[nodiscard]]
    Request requestData(connection_type& connection, SIMCONNECT_DATA_DEFINITION_ID dataDef,
        std::function<void(Data::DataBlockReader&)> handler,
        DataFrequency frequency = DataFrequency::once(),
        PeriodLimits limits = PeriodLimits::none(),
        unsigned long objectId = SIMCONNECT_OBJECT_ID_USER_CURRENT,
        bool onlyWhenChanged = false)
    {
        auto requestId = connection.requests().nextRequestID();

        this->registerHandler(requestId, [requestId, handler](const SIMCONNECT_RECV& msg) {
                Data::DataBlockReader reader(reinterpret_cast<const SIMCONNECT_RECV_SIMOBJECT_DATA&>(msg));

                handler(reader);
            }, frequency.isOnce());
        connection.requestData(dataDef, requestId, frequency, limits, objectId, onlyWhenChanged);

        return frequency.isOnce() ? Request{requestId} : Request{ requestId, [this, requestId, &connection, &dataDef, objectId]() {
            stopDataRequest(connection, dataDef, requestId, objectId);
        }};
    }


    /**
     * Requests data once. The caller passes a handler that will be executed once the
     * data is received. The handler will receive a reference to a DataBlockReader that can be used to read the data.
     * 
     * @note Discarding or deleting the Request object will stop the request.
     * 
     * @param connection The connection to request the data from.
     * @param dataDef The data definition Id to use for the request.
     * @param handler The handler to execute when the data is received.
     * @param objectId The object ID to request data for. Defaults to the current user's Avatar or Aircraft.
     * @param onlyWhenChanged If true, the data will only be requested when it has changed.
     * @return A Request object that can be used to stop the request.
     */
    [[nodiscard]]
    Request requestDataOnce(connection_type& connection, SIMCONNECT_DATA_DEFINITION_ID dataDef,
        std::function<void(Data::DataBlockReader&)> handler,
        unsigned long objectId = SIMCONNECT_OBJECT_ID_USER_CURRENT,
        bool onlyWhenChanged = false)
    {
        return requestData(connection, dataDef, handler, DataFrequency::once(), PeriodLimits::none(), objectId, onlyWhenChanged);
    }


    /**
     * Requests data in the tagged format. The caller passes a handler that will be executed once the
     * data is received. The handler will receive a reference to a DataBlockReader that can be used to read the data.
     * 
     * @note Discarding or deleting the Request object will stop the request.
     * 
     * @param connection The connection to request the data from.
     * @param dataDef The data definition Id to use for the request.
     * @param handler The handler to execute when the data is received.
     * @param frequency The frequency at which to request the data.
     * @param limits The limits for the request in numbers of "periods".
     * @param objectId The object ID to request data for. Defaults to the current user's Avatar or Aircraft.
     * @param onlyWhenChanged If true, the data will only be requested when it has changed.
     * @return A Request object that can be used to stop the request.
     */
    [[nodiscard]]
    Request requestDataTagged(connection_type& connection, SIMCONNECT_DATA_DEFINITION_ID dataDef,
        std::function<void(Data::DataBlockReader&)> handler,
        DataFrequency frequency = DataFrequency::once(),
        PeriodLimits limits = PeriodLimits::none(),
        unsigned long objectId = SIMCONNECT_OBJECT_ID_USER_CURRENT,
        bool onlyWhenChanged = false)
    {
        auto requestId = connection.requests().nextRequestID();

        this->registerHandler(requestId, [requestId, handler](const SIMCONNECT_RECV& msg) {
                Data::DataBlockReader reader(reinterpret_cast<const SIMCONNECT_RECV_SIMOBJECT_DATA&>(msg));

                handler(reader);
            }, frequency.isOnce());
        connection.requestDataTagged(dataDef, requestId, frequency, limits, objectId, onlyWhenChanged);

        return frequency.isOnce() ? Request{requestId} : Request{ requestId, [this, &connection, &dataDef, requestId, objectId]() {
            stopDataRequest(connection, dataDef, requestId, objectId);
        }};
    }


    /**
     * Requests data once in the tagged format. The caller passes a handler that will be executed once the
     * data is received. The handler will receive a reference to a DataBlockReader that can be used to read the data.
     * 
     * @note Discarding or deleting the Request object will stop the request.
     * 
     * @param connection The connection to request the data from.
     * @param dataDef The data definition Id to use for the request.
     * @param handler The handler to execute when the data is received.
     * @param objectId The object ID to request data for. Defaults to the current user's Avatar or Aircraft.
     * @param onlyWhenChanged If true, the data will only be requested when it has changed.
     * @return A Request object that can be used to stop the request.
     */
    [[nodiscard]]
    Request requestDataOnceTagged(connection_type& connection, SIMCONNECT_DATA_DEFINITION_ID dataDef,
        std::function<void(Data::DataBlockReader&)> handler,
        unsigned long objectId = SIMCONNECT_OBJECT_ID_USER_CURRENT,
        bool onlyWhenChanged = false)
    {
        return requestDataTagged(connection, dataDef, handler, DataFrequency::once(), PeriodLimits::none(), objectId, onlyWhenChanged);
    }


    // Next, request the data and pass a handler that will receive an ephemeral structure with the data unmarshalled into it.

    /**
     * Requests data. The caller passes a handler that will be executed once the data is received.
     * The handler will receive a reference to an ephemeral structure that has the data unmarshalled into it.
     * 
     * @note Discarding or deleting the Request object will stop the request.
     * 
     * @param connection The connection to request the data from.
     * @param dataDef The data definition to use for the request.
     * @param handler The handler to execute when the data is received.
     * @param frequency The frequency at which to request the data.
     * @param limits The limits for the request in numbers of "periods".
     * @param objectId The object ID to request data for. Defaults to the current user's Avatar or Aircraft.
     * @param onlyWhenChanged If true, the data will only be requested when it has changed.
     * @return A Request object that can be used to stop the request.
     * @tparam StructType The type of the structure to receive the data in.
     */
    template <typename StructType>
    [[nodiscard]]
    Request requestData(connection_type& connection, DataDefinition<StructType>& dataDef,
        std::function<void(const StructType&)> handler,
        DataFrequency frequency = DataFrequency::once(),
        PeriodLimits limits = PeriodLimits::none(),
        unsigned long objectId = SIMCONNECT_OBJECT_ID_USER_CURRENT,
        bool onlyWhenChanged = false)
    {
        dataDef.define(connection);

        const auto requestId = connection.requests().nextRequestID();

        if (dataDef.useMapping()) {
            this->registerHandler(requestId, [requestId, &dataDef, handler](const SIMCONNECT_RECV& msg) {
                const StructType* data = reinterpret_cast<const StructType*>(&(reinterpret_cast<const SIMCONNECT_RECV_SIMOBJECT_DATA&>(msg).dwData));
                handler(*data);
                }, frequency.isOnce());
        }
        else {
            this->registerHandler(requestId, [requestId, &dataDef, handler](const SIMCONNECT_RECV& msg) {
                StructType data;

                dataDef.unmarshall(reinterpret_cast<const SIMCONNECT_RECV_SIMOBJECT_DATA&>(msg), data);
                handler(data);
                }, frequency.isOnce());
        }
        connection.requestData(dataDef, requestId, frequency, limits, objectId, onlyWhenChanged);

        return frequency.isOnce() ? Request{requestId} : Request{ requestId, [this, &connection, &dataDef, requestId, objectId]() {
            stopDataRequest(connection, dataDef, requestId, objectId);
        }};
    }


    /**
     * Requests data once. The caller passes a handler that will be executed when the data is received.
     * The handler will receive a reference to an ephemeral structure that has the data unmarshalled into it.
     * 
     * @note Discarding or deleting the Request object will stop the request.
     * 
     * @param connection The connection to request the data from.
     * @param dataDef The data definition to use for the request.
     * @param handler The handler to execute when the data is received.
     * @param objectId The object ID to request data for. Defaults to the current user's Avatar or Aircraft.
     * @param onlyWhenChanged If true, the data will only be requested when it has changed.
     * @return A Request object that can be used to stop the request.
     * @tparam StructType The type of the structure to receive the data in.
     */
    template <typename StructType>
    [[nodiscard]]
    Request requestDataOnce(connection_type& connection, DataDefinition<StructType>& dataDef,
        std::function<void(const StructType&)> handler,
        unsigned long objectId = SIMCONNECT_OBJECT_ID_USER_CURRENT,
        bool onlyWhenChanged = false)
    {
        return requestData(connection, dataDef, handler, DataFrequency::once(), PeriodLimits::none(), objectId, onlyWhenChanged);
    }


    /**
     * Requests data in the tagged format. The caller passes a handler that will be executed once the
     * data is received. The handler will receive a reference to an ephemeral structure that has the data unmarshalled into it.
     * 
     * @note Discarding or deleting the Request object will stop the request.
     * 
     * @param connection The connection to request the data from.
     * @param dataDef The data definition to use for the request.
     * @param handler The handler to execute when the data is received.
     * @param frequency The frequency at which to request the data.
     * @param limits The limits for the request in numbers of "periods".
     * @param objectId The object ID to request data for. Defaults to the current user's Avatar or Aircraft.
     * @param onlyWhenChanged If true, the data will only be requested when it has changed.
     * @return A Request object that can be used to stop the request.
     * @tparam StructType The type of the structure to receive the data in.
     */
    template <typename StructType>
    [[nodiscard]]
    Request requestDataTagged(connection_type& connection, DataDefinition<StructType>& dataDef,
        std::function<void(const StructType&)> handler,
        DataFrequency frequency = DataFrequency::once(),
        PeriodLimits limits = PeriodLimits::none(),
        unsigned long objectId = SIMCONNECT_OBJECT_ID_USER_CURRENT,
        bool onlyWhenChanged = false)
    {
        dataDef.define(connection);

        const auto requestId = connection.requests().nextRequestID();

        this->registerHandler(requestId, [requestId, &dataDef, handler](const SIMCONNECT_RECV& msg) {
            StructType data;

            dataDef.unmarshall(reinterpret_cast<const SIMCONNECT_RECV_SIMOBJECT_DATA&>(msg), data);
            handler(data);
        }, frequency.isOnce());
        connection.requestDataTagged(dataDef, requestId, frequency, limits, objectId, onlyWhenChanged);

        return frequency.isOnce() ? Request{requestId} : Request{ requestId, [this, &connection, &dataDef, requestId, objectId]() {
            stopDataRequest(connection, dataDef, requestId, objectId);
        }};
    }


    /**
     * Requests data once, in the tagged format. The caller passes a handler that will be executed once the data is received.
     * The handler will receive a reference to an ephemeral structure that has the data unmarshalled into it.
     * 
     * @note Discarding or deleting the Request object will stop the request.
     * 
     * @param connection The connection to request the data from.
     * @param dataDef The data definition to use for the request.
     * @param handler The handler to execute when the data is received.
     * @param objectId The object ID to request data for. Defaults to the current user's Avatar or Aircraft.
     * @param onlyWhenChanged If true, the data will only be requested when it has changed.
     * @return A Request object that can be used to stop the request.
     * @tparam StructType The type of the structure to receive the data in.
     */
    template <typename StructType>
    [[nodiscard]]
    Request requestDataOnceTagged(connection_type& connection, DataDefinition<StructType>& dataDef,
        std::function<void(const StructType&)> handler,
        unsigned long objectId = SIMCONNECT_OBJECT_ID_USER_CURRENT,
        bool onlyWhenChanged = false)
    {
        return requestData(connection, dataDef, handler, DataFrequency::once(), PeriodLimits::none(), objectId, onlyWhenChanged);
    }


	// Requesting data for all SimObjects of a specific type.

    // In contrast to the first group, these requests always concern "Once" requests and cannot specify flags.
    // This effectively means data will always be untagged.


    /**
     * Stores the object ID in the data structure if it is a SimObjectIdHolder.
     * This is used to store the object ID in the data structure when the data is received
     * 
     * @param objectId The object ID to store.
     * @param data The data structure to store the object ID in.
     */
    template <typename StructType>
    static void storeObjectId(unsigned long objectId, StructType& data) {
        if constexpr (std::is_base_of_v<SimObjectIdHolder, StructType>) {
            data.objectId = objectId;
        }
    }


    /**
     * Requests data by SimObject type. The caller passes a handler that will be executed once the data is received.
     * The handler will receive a reference to an ephemeral structure that has the data unmarshalled into it.
     *
     * @note Discarding or deleting the Request object will stop the request.
     *
     * @param connection The connection to request the data from.
     * @param dataDef The data definition to use for the request.
     * @param handler The handler to execute when the data is received.
	 * @param onDone An optional handler to execute when all data has been received.
	 * @param radiusInMeters The radius of the area for which to request data. If 0, only the user's aircraft is in scope.
	 * @param objectType The type of SimObject to request data for.
     * @return A Request object that can be used to stop the request.
     * @tparam StructType The type of the structure to receive the data in.
     */
    template <typename StructType>
    [[nodiscard]]
    Request requestDataByType(connection_type& connection, DataDefinition<StructType>& dataDef,
        std::function<void(const StructType&)> handler, std::function<void()> onDone,
        unsigned long radiusInMeters,
        SIMCONNECT_SIMOBJECT_TYPE objectType)
    {
        if (!handler) {
            throw std::invalid_argument("Handler must not be null.");
		}
        dataDef.define(connection);

        const auto requestId = connection.requests().nextRequestID();

        if (dataDef.useMapping()) {
            this->registerHandler(requestId, [requestId, &dataDef, handler, onDone](const SIMCONNECT_RECV& msg) {
				const SIMCONNECT_RECV_SIMOBJECT_DATA_BYTYPE& dataMsg = reinterpret_cast<const SIMCONNECT_RECV_SIMOBJECT_DATA_BYTYPE&>(msg);
                const StructType* data = reinterpret_cast<const StructType*>(&(dataMsg.dwData));

                // A mapped struct cannot contain the objectId
                handler(*data);

                if (dataMsg.dwentrynumber == dataMsg.dwoutof) {
                    if (onDone) {
                        onDone();
					}
                }
            }, false);
        }
        else {
            this->registerHandler(requestId, [requestId, &dataDef, handler, onDone](const SIMCONNECT_RECV& msg) {
                const SIMCONNECT_RECV_SIMOBJECT_DATA_BYTYPE& dataMsg = reinterpret_cast<const SIMCONNECT_RECV_SIMOBJECT_DATA_BYTYPE&>(msg);

                StructType data;

                dataDef.unmarshall(dataMsg, data);
				storeObjectId(dataMsg.dwObjectID, data);

                handler(data);

                if (dataMsg.dwentrynumber == dataMsg.dwoutof) {
                    if (onDone) {
                        onDone();
                    }
                }
            }, false);
        }
        connection.requestDataByType(dataDef, requestId, radiusInMeters, objectType);

        return Request{ requestId, [this, &connection, &dataDef, requestId]() {
            stopDataRequest(connection, dataDef, requestId);
        } };
    }


    /**
     * Requests data by SimObject type, collected in an unordered map. The caller passes a handler that will
     * be executed once the data is received. The handler will receive a reference to an ephemeral structure
     * that has the data unmarshalled into it.
     *
     * @note Discarding or deleting the Request object will stop the request.
     *
     * @param connection The connection to request the data from.
     * @param dataDef The data definition to use for the request.
     * @param handler The handler to execute when the data is received.
     * @param radiusInMeters The radius of the area for which to request data. If 0, only the user's aircraft is in scope.
     * @param objectType The type of SimObject to request data for.
     * @return A Request object that can be used to stop the request.
     * @tparam StructType The type of the structure to receive the data in.
     */
    template <typename StructType>
    [[nodiscard]]
    Request requestDataByType(connection_type& connection, DataDefinition<StructType>& dataDef,
        std::function<void(std::unordered_map<unsigned long, StructType>&)> handler,
        unsigned long radiusInMeters,
        SIMCONNECT_SIMOBJECT_TYPE objectType)
    {
        if (!handler) {
            throw std::invalid_argument("Handler must not be null.");
        }
        dataDef.define(connection);

        const auto requestId = connection.requests().nextRequestID();
        std::unordered_map<unsigned long, StructType> result;

        if (dataDef.useMapping()) {
            this->registerHandler(requestId,
                            [requestId, &dataDef, handler, result](const SIMCONNECT_RECV& msg) mutable
                {
                    const SIMCONNECT_RECV_SIMOBJECT_DATA_BYTYPE& dataMsg = reinterpret_cast<const SIMCONNECT_RECV_SIMOBJECT_DATA_BYTYPE&>(msg);
                    const StructType& data = reinterpret_cast<const StructType&>(dataMsg.dwData);

					result[dataMsg.dwObjectID] = data;
                    if (dataMsg.dwentrynumber == dataMsg.dwoutof) {
						handler(result);
                    }
                }, false);
        }
        else {
            this->registerHandler(requestId, [requestId, &dataDef, handler, result](const SIMCONNECT_RECV& msg) mutable
                {
                    const SIMCONNECT_RECV_SIMOBJECT_DATA_BYTYPE& dataMsg = reinterpret_cast<const SIMCONNECT_RECV_SIMOBJECT_DATA_BYTYPE&>(msg);

                    StructType data;

                    dataDef.unmarshall(dataMsg, data);
					storeObjectId(dataMsg.dwObjectID, data);
                    result[dataMsg.dwObjectID] = data;

                    if (dataMsg.dwentrynumber == dataMsg.dwoutof) {
                        handler(result);
                    }
                }, false);
        }
        connection.requestDataByType(dataDef, requestId, radiusInMeters, objectType);

        return Request{ requestId, [this, &connection, &dataDef, requestId]() {
            stopDataRequest(connection, dataDef, requestId);
        } };
    }


    /**
     * Requests data for all SimObjects of a specific type. The caller passes a handler that will be executed once the
     * data is received. The handler will receive a const reference to the (raw) message data.
     * 
     * @note An "OutOfBounds" exception message will be sent if the radius exceeds the maximum allowed, which is 200,000 meters or 200 km.
     * 
     * @param connection The connection to request the data from.
     * @param dataDef The data definition Id to use for the request.
     * @param handler The handler to execute when the data is received.
     * @param onDone An optional handler to execute when all data has been received.
     * @param radiusInMeters The radius of the area for which to request data. If 0, only the user's aircraft is in scope.
     * @param objectType The type of SimObject to request data for.
	 */
    [[nodiscard]]
    Request requestDataByType(connection_type& connection, SIMCONNECT_DATA_DEFINITION_ID dataDef,
        std::function<void(const SIMCONNECT_RECV_SIMOBJECT_DATA_BYTYPE&)> handler, std::function<void()> onDone,
        unsigned long radiusInMeters,
        SIMCONNECT_SIMOBJECT_TYPE objectType)
    {
        auto requestId = connection.requests().nextRequestID();

        this->registerHandler(requestId, [requestId, handler, onDone](const SIMCONNECT_RECV& msg) {
			const SIMCONNECT_RECV_SIMOBJECT_DATA_BYTYPE& dataMsg = reinterpret_cast<const SIMCONNECT_RECV_SIMOBJECT_DATA_BYTYPE&>(msg);
            handler(dataMsg);
            if (dataMsg.dwentrynumber == dataMsg.dwoutof) {
                if (onDone) {
                    onDone();
                }
            }
            }, false);
        connection.requestDataByType(dataDef, requestId, radiusInMeters, objectType);

        return Request{ requestId, [this, &connection, dataDef, requestId]() {
            stopDataRequest(connection, dataDef, requestId);
		} };
	}

};

} // namespace SimConnect