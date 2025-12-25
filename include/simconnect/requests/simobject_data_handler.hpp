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


#include <simconnect/simconnect.hpp>
#include <simconnect/simobject_type.hpp>
#include <simconnect/message_handler.hpp>


namespace SimConnect {


/**
 * The SimObjectIdHolder class is a simple base class that holds the ID of a SimObject.
 * It is used by the requestDataByType methods to return the ID of the SimObject that the data was requested for.
 */
struct SimObjectIdHolder {
    SimObjectId objectId{ 0 };

    constexpr SimObjectIdHolder() = default;
    constexpr SimObjectIdHolder(SimObjectId id) : objectId(id) {}
	SimObjectIdHolder(const Messages::SimObjectDataMsg& msg) : objectId(msg.dwObjectID) {}
	SimObjectIdHolder(const SimObjectIdHolder&) = default;
    SimObjectIdHolder(SimObjectIdHolder&&) = default;
    SimObjectIdHolder& operator=(const SimObjectIdHolder&) = default;
    SimObjectIdHolder& operator=(SimObjectIdHolder&&) = default;
};


/**
 * The SimObjectDataHandler class provides for responsive handling of Messages::SimObjectDataMsg and
 * Messages::SimObjectDataByTypeMsg messages.
 * 
 * @note This handler is used to request data from the simulator for a specific object or type.
 * 
 * @tparam M The type of the SimConnect message handler, which must be derived from SimConnectMessageHandler.
 */
template <class M>
class SimObjectDataHandler
    : public MessageHandler<RequestId, SimObjectDataHandler<M>, M, Messages::simObjectData, Messages::simObjectDataByType>
{
public:
    using simconnect_message_handler_type = M;
    using connection_type = typename M::connection_type;

private:
    simconnect_message_handler_type& simConnectMessageHandler_;


    // No copies or moves
    SimObjectDataHandler(const SimObjectDataHandler&) = delete;
    SimObjectDataHandler(SimObjectDataHandler&&) = delete;
    SimObjectDataHandler& operator=(const SimObjectDataHandler&) = delete;
    SimObjectDataHandler& operator=(SimObjectDataHandler&&) = delete;


public:
    SimObjectDataHandler(simconnect_message_handler_type& handler) : simConnectMessageHandler_(handler)
    {
        this->enable(simConnectMessageHandler_);
    }
    ~SimObjectDataHandler() = default;


    /**
     * Returns the SimConnect message handler.
     */
    [[nodiscard]]
    simconnect_message_handler_type& simConnectMessageHandler() const noexcept {
        return simConnectMessageHandler_;
    }


    /**
     * Returns the request ID from the message. This is specific to the Messages::SimObjectDataMsg and
	 * Messages::SimObjectDataByTypeMsg messages. The latter type does not actually add fields, so we can
     * use the same method for both.
     * 
     * @param msg The message to get the correlation ID from.
     * @returns The correlation ID from the message.
	 */
    unsigned long correlationId(const Messages::MsgBase& msg) const {
        return static_cast<const Messages::SimObjectDataMsg&>(msg).dwRequestID;
    }


    /**
     * Stops a data request and removes the handler if still active.
     * 
     * @param dataDef The data definition ID to stop the request for.
     * @param requestId The request ID to stop.
     * @param objectId The object ID to stop the request for. Defaults to the current user's Avatar or Aircraft.
     * @note If the request is not active, this will do nothing.
     */
    void stopDataRequest(DataDefinitionId dataDef, RequestId requestId, SimObjectId objectId = SimObject::userCurrent)
    {
        this->removeHandler(requestId);
        simConnectMessageHandler_.connection().stopDataRequest(requestId, dataDef, objectId);
    }


    // Requesting data for specific SimObjects.

    // Three groups of requestData methods are provided, depending on how the caller wants to receive the data:
    // 1. As a raw message data structure (Messages::SimObjectDataMsg).
    // 2. As a DataBlockReader that can be used to read the data.
    // 3. As a struct (or class), where the setters and getters are used.
    //
    // For each group, there are methods to request the data once or repeatedly, and tagged. (again once or repeatedly)

    // First, request the data and pass a handler that will receive the raw message data.

#pragma region Raw message data requests

    /**
     * Requests data. The caller passes a handler that will be executed once the
     * data is received. The handler will receive a const reference to the (raw) message data.
     * 
     * @note Discarding or deleting the Request object will stop the request.
     * 
     * @param dataDef The data definition Id to use for the request.
     * @param handler The handler to execute when the data is received.
     * @param frequency The frequency at which to request the data.
     * @param limits The limits for the request in numbers of "periods".
     * @param objectId The object ID to request data for. Defaults to the current user's Avatar or Aircraft.
     * @param onlyWhenChanged If true, the data will only be requested when it has changed.
     * @return A Request object that can be used to stop the request.
     */
    [[nodiscard]]
    Request requestData(DataDefinitionId dataDef,
        std::function<void(const Messages::SimObjectDataMsg&)> handler,
        DataFrequency frequency = DataFrequency::once(),
        PeriodLimits limits = PeriodLimits::none(),
        SimObjectId objectId = SimObject::userCurrent,
        bool onlyWhenChanged = false)
    {
        auto requestId = simConnectMessageHandler_.connection().requests().nextRequestID();

        this->registerHandler(requestId, [requestId, handler](const Messages::MsgBase& msg) {
                handler(reinterpret_cast<const Messages::SimObjectDataMsg&>(msg));
            }, frequency.isOnce());
        simConnectMessageHandler_.connection().requestData(dataDef, requestId, frequency, limits, objectId, onlyWhenChanged);

        return frequency.isOnce() ? Request{requestId} : Request{ requestId, [this, requestId, &dataDef, objectId]() {
            stopDataRequest(dataDef, requestId, objectId);
        }};
    }


    /**
     * Requests data once. The caller passes a handler that will be executed once the
     * data is received. The handler will receive a const reference to the (raw) message data.
     * 
     * @note Discarding or deleting the Request object will stop the request.
     * 
     * @param dataDef The data definition Id to use for the request.
     * @param handler The handler to execute when the data is received.
     * @param objectId The object ID to request data for. Defaults to the current user's Avatar or Aircraft.
     * @return A Request object that can be used to stop the request.
     */
    [[nodiscard]]
    Request requestDataOnce(DataDefinitionId dataDef,
        std::function<void(const Messages::SimObjectDataMsg&)> handler,
        SimObjectId objectId = SimObject::userCurrent)
    {
        return requestData(dataDef, handler, DataFrequency::once(), PeriodLimits::none(), objectId, false);
    }


    /**
     * Requests data. The caller passes a handler that will be executed once the
     * data is received. The handler will receive a const reference to the (raw) message data.
     * 
     * @note Discarding or deleting the Request object will stop the request.
     * 
     * @param dataDef The data definition Id to use for the request.
     * @param handler The handler to execute when the data is received.
     * @param frequency The frequency at which to request the data.
     * @param limits The limits for the request in numbers of "periods".
     * @param objectId The object ID to request data for. Defaults to the current user's Avatar or Aircraft.
     * @param onlyWhenChanged If true, the data will only be requested when it has changed.
     * @return A Request object that can be used to stop the request.
     */
    [[nodiscard]]
    Request requestDataTagged(DataDefinitionId dataDef,
        std::function<void(const Messages::SimObjectDataMsg&)> handler,
        DataFrequency frequency = DataFrequency::once(),
        PeriodLimits limits = PeriodLimits::none(),
        SimObjectId objectId = SimObject::userCurrent,
        bool onlyWhenChanged = false)
    {
        auto requestId = simConnectMessageHandler_.connection().requests().nextRequestID();

        this->registerHandler(requestId, [requestId, handler](const Messages::MsgBase& msg) {
                handler(reinterpret_cast<const Messages::SimObjectDataMsg&>(msg));
            }, frequency.isOnce());
        simConnectMessageHandler_.connection().requestDataTagged(dataDef, requestId, frequency, limits, objectId, onlyWhenChanged);

        return frequency.isOnce() ? Request{requestId} : Request{ requestId, [this, requestId, &dataDef, objectId]() {
            stopDataRequest(dataDef, requestId, objectId);
        }};
    }


    /**
     * Requests data once. The caller passes a handler that will be executed once the
     * data is received. The handler will receive a const reference to the (raw) message data.
     * 
     * @note Discarding or deleting the Request object will stop the request.
     * 
     * @param dataDef The data definition Id to use for the request.
     * @param handler The handler to execute when the data is received.
     * @param objectId The object ID to request data for. Defaults to the current user's Avatar or Aircraft.
     * @return A Request object that can be used to stop the request.
     */
    [[nodiscard]]
    Request requestDataOnceTagged(DataDefinitionId dataDef,
        std::function<void(const Messages::SimObjectDataMsg&)> handler,
        SimObjectId objectId = SimObject::userCurrent)
    {
        return requestDataTagged(dataDef, handler, DataFrequency::once(), PeriodLimits::none(), objectId, false);
    }

#pragma endregion

#pragma region DataBlockReader requests

    // Next, request the data and pass a handler that will receive a DataBlockReader to read the data.

    /**
     * Requests data. The caller passes a handler that will be executed once the
     * data is received. The handler will receive a reference to a DataBlockReader that can be used to read the data.
     * 
     * @note Discarding or deleting the Request object will stop the request.
     * 
     * @param dataDef The data definition Id to use for the request.
     * @param handler The handler to execute when the data is received.
     * @param frequency The frequency at which to request the data.
     * @param limits The limits for the request in numbers of "periods".
     * @param objectId The object ID to request data for. Defaults to the current user's Avatar or Aircraft.
     * @param onlyWhenChanged If true, the data will only be requested when it has changed.
     * @return A Request object that can be used to stop the request.
     */
    [[nodiscard]]
    Request requestData(DataDefinitionId dataDef,
        std::function<void(Data::DataBlockReader&)> handler,
        DataFrequency frequency = DataFrequency::once(),
        PeriodLimits limits = PeriodLimits::none(),
        SimObjectId objectId = SimObject::userCurrent,
        bool onlyWhenChanged = false)
    {
        auto requestId = simConnectMessageHandler_.connection().requests().nextRequestID();

        this->registerHandler(requestId, [requestId, handler](const Messages::MsgBase& msg) {
                Data::DataBlockReader reader(reinterpret_cast<const Messages::SimObjectDataMsg&>(msg));

                handler(reader);
            }, frequency.isOnce());
        simConnectMessageHandler_.connection().requestData(dataDef, requestId, frequency, limits, objectId, onlyWhenChanged);

        return frequency.isOnce() ? Request{requestId} : Request{ requestId, [this, requestId, &dataDef, objectId]() {
            stopDataRequest(dataDef, requestId, objectId);
        }};
    }


    /**
     * Requests data once. The caller passes a handler that will be executed once the
     * data is received. The handler will receive a reference to a DataBlockReader that can be used to read the data.
     * 
     * @note Discarding or deleting the Request object will stop the request.
     * 
     * @param dataDef The data definition Id to use for the request.
     * @param handler The handler to execute when the data is received.
     * @param objectId The object ID to request data for. Defaults to the current user's Avatar or Aircraft.
     * @param onlyWhenChanged If true, the data will only be requested when it has changed.
     * @return A Request object that can be used to stop the request.
     */
    [[nodiscard]]
    Request requestDataOnce(DataDefinitionId dataDef,
        std::function<void(Data::DataBlockReader&)> handler,
        SimObjectId objectId = SimObject::userCurrent,
        bool onlyWhenChanged = false)
    {
        return requestData(dataDef, handler, DataFrequency::once(), PeriodLimits::none(), objectId, onlyWhenChanged);
    }


    /**
     * Requests data in the tagged format. The caller passes a handler that will be executed once the
     * data is received. The handler will receive a reference to a DataBlockReader that can be used to read the data.
     * 
     * @note Discarding or deleting the Request object will stop the request.
     * 
     * @param dataDef The data definition Id to use for the request.
     * @param handler The handler to execute when the data is received.
     * @param frequency The frequency at which to request the data.
     * @param limits The limits for the request in numbers of "periods".
     * @param objectId The object ID to request data for. Defaults to the current user's Avatar or Aircraft.
     * @param onlyWhenChanged If true, the data will only be requested when it has changed.
     * @return A Request object that can be used to stop the request.
     */
    [[nodiscard]]
    Request requestDataTagged(DataDefinitionId dataDef,
        std::function<void(Data::DataBlockReader&)> handler,
        DataFrequency frequency = DataFrequency::once(),
        PeriodLimits limits = PeriodLimits::none(),
        SimObjectId objectId = SimObject::userCurrent,
        bool onlyWhenChanged = false)
    {
        auto requestId = simConnectMessageHandler_.connection().requests().nextRequestID();

        this->registerHandler(requestId, [requestId, handler](const Messages::MsgBase& msg) {
                Data::DataBlockReader reader(reinterpret_cast<const Messages::SimObjectDataMsg&>(msg));

                handler(reader);
            }, frequency.isOnce());
        simConnectMessageHandler_.connection().requestDataTagged(dataDef, requestId, frequency, limits, objectId, onlyWhenChanged);

        return frequency.isOnce() ? Request{requestId} : Request{ requestId, [this, &dataDef, requestId, objectId]() {
            stopDataRequest(dataDef, requestId, objectId);
        }};
    }


    /**
     * Requests data once in the tagged format. The caller passes a handler that will be executed once the
     * data is received. The handler will receive a reference to a DataBlockReader that can be used to read the data.
     * 
     * @note Discarding or deleting the Request object will stop the request.
     * 
     * @param dataDef The data definition Id to use for the request.
     * @param handler The handler to execute when the data is received.
     * @param objectId The object ID to request data for. Defaults to the current user's Avatar or Aircraft.
     * @param onlyWhenChanged If true, the data will only be requested when it has changed.
     * @return A Request object that can be used to stop the request.
     */
    [[nodiscard]]
    Request requestDataOnceTagged(DataDefinitionId dataDef,
        std::function<void(Data::DataBlockReader&)> handler,
        SimObjectId objectId = SimObject::userCurrent,
        bool onlyWhenChanged = false)
    {
        return requestDataTagged(dataDef, handler, DataFrequency::once(), PeriodLimits::none(), objectId, onlyWhenChanged);
    }

#pragma endregion

#pragma region StructType requests

    // Next, request the data and pass a handler that will receive an ephemeral structure with the data unmarshalled into it.

    /**
     * Requests data. The caller passes a handler that will be executed once the data is received.
     * The handler will receive a reference to an ephemeral structure that has the data unmarshalled into it.
     * 
     * @note Discarding or deleting the Request object will stop the request.
     * 
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
    Request requestData(DataDefinition<StructType>& dataDef,
        std::function<void(const StructType&)> handler,
        DataFrequency frequency = DataFrequency::once(),
        PeriodLimits limits = PeriodLimits::none(),
        SimObjectId objectId = SimObject::userCurrent,
        bool onlyWhenChanged = false)
    {
        dataDef.define(simConnectMessageHandler_.connection());

        const auto requestId = simConnectMessageHandler_.connection().requests().nextRequestID();

        if (dataDef.useMapping()) {
            this->registerHandler(requestId, [handler](const Messages::MsgBase& msg) {
                const StructType* data = reinterpret_cast<const StructType*>(&(reinterpret_cast<const Messages::SimObjectDataMsg&>(msg).dwData));
                handler(*data);
                }, frequency.isOnce());
        }
        else {
            this->registerHandler(requestId, [&dataDef, handler](const Messages::MsgBase& msg) {
                StructType data;

                dataDef.unmarshall(reinterpret_cast<const Messages::SimObjectDataMsg&>(msg), data);
                handler(data);
                }, frequency.isOnce());
        }
        simConnectMessageHandler_.connection().requestData(dataDef, requestId, frequency, limits, objectId, onlyWhenChanged);

        return frequency.isOnce() ? Request{requestId} : Request{ requestId, [this, &dataDef, requestId, objectId]() {
            stopDataRequest(dataDef, requestId, objectId);
        }};
    }


    /**
     * Requests data once. The caller passes a handler that will be executed when the data is received.
     * The handler will receive a reference to an ephemeral structure that has the data unmarshalled into it.
     * 
     * @note Discarding or deleting the Request object will stop the request.
     * 
     * @param dataDef The data definition to use for the request.
     * @param handler The handler to execute when the data is received.
     * @param objectId The object ID to request data for. Defaults to the current user's Avatar or Aircraft.
     * @param onlyWhenChanged If true, the data will only be requested when it has changed.
     * @return A Request object that can be used to stop the request.
     * @tparam StructType The type of the structure to receive the data in.
     */
    template <typename StructType>
    [[nodiscard]]
    Request requestDataOnce(DataDefinition<StructType>& dataDef,
        std::function<void(const StructType&)> handler,
        SimObjectId objectId = SimObject::userCurrent,
        bool onlyWhenChanged = false)
    {
        return requestData(dataDef, handler, DataFrequency::once(), PeriodLimits::none(), objectId, onlyWhenChanged);
    }


    /**
     * Requests data in the tagged format. The caller passes a handler that will be executed once the
     * data is received. The handler will receive a reference to an ephemeral structure that has the data unmarshalled into it.
     * 
     * @note Discarding or deleting the Request object will stop the request.
     * 
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
    Request requestDataTagged(DataDefinition<StructType>& dataDef,
        std::function<void(const StructType&)> handler,
        DataFrequency frequency = DataFrequency::once(),
        PeriodLimits limits = PeriodLimits::none(),
        SimObjectId objectId = SimObject::userCurrent,
        bool onlyWhenChanged = false)
    {
        dataDef.define(simConnectMessageHandler_.connection());

        const auto requestId = simConnectMessageHandler_.connection().requests().nextRequestID();

        this->registerHandler(requestId, [requestId, &dataDef, handler](const Messages::MsgBase& msg) {
            StructType data;

            dataDef.unmarshall(reinterpret_cast<const Messages::SimObjectDataMsg&>(msg), data);
            handler(data);
        }, frequency.isOnce());
        simConnectMessageHandler_.connection().requestDataTagged(dataDef, requestId, frequency, limits, objectId, onlyWhenChanged);

        return frequency.isOnce() ? Request{requestId} : Request{ requestId, [this, &dataDef, requestId, objectId]() {
            stopDataRequest(dataDef, requestId, objectId);
        }};
    }


    /**
     * Requests data once, in the tagged format. The caller passes a handler that will be executed once the data is received.
     * The handler will receive a reference to an ephemeral structure that has the data unmarshalled into it.
     * 
     * @note Discarding or deleting the Request object will stop the request.
     * 
     * @param dataDef The data definition to use for the request.
     * @param handler The handler to execute when the data is received.
     * @param objectId The object ID to request data for. Defaults to the current user's Avatar or Aircraft.
     * @param onlyWhenChanged If true, the data will only be requested when it has changed.
     * @return A Request object that can be used to stop the request.
     * @tparam StructType The type of the structure to receive the data in.
     */
    template <typename StructType>
    [[nodiscard]]
    Request requestDataOnceTagged(DataDefinition<StructType>& dataDef,
        std::function<void(const StructType&)> handler,
        SimObjectId objectId = SimObject::userCurrent,
        bool onlyWhenChanged = false)
    {
        return requestData(dataDef, handler, DataFrequency::once(), PeriodLimits::none(), objectId, onlyWhenChanged);
    }

#pragma endregion

#pragma region ByType requests

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
    static void storeObjectId(SimObjectId objectId, StructType& data) {
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
    Request requestDataByType(DataDefinition<StructType>& dataDef,
        std::function<void(const StructType&)> handler, std::function<void()> onDone,
        unsigned long radiusInMeters,
        SimObjectType objectType)
    {
        if (!handler) {
            throw std::invalid_argument("Handler must not be null.");
		}
		auto& logger = simConnectMessageHandler_.connection().logger();

        dataDef.define(simConnectMessageHandler_.connection());
        logger.debug("Data definition ID {} for requestDataByType.", dataDef.id());

        const auto requestId = simConnectMessageHandler_.connection().requests().nextRequestID();

        if (dataDef.useMapping()) {
			logger.debug("Using mapping for requestDataByType with request ID {}.", requestId);
            this->registerHandler(requestId, [&logger = simConnectMessageHandler_.connection().logger(), requestId, handler, onDone](const Messages::MsgBase& msg) {
				const Messages::SimObjectDataByTypeMsg& dataMsg = reinterpret_cast<const Messages::SimObjectDataByTypeMsg&>(msg);
                const StructType* data = reinterpret_cast<const StructType*>(&(dataMsg.dwData));

                logger.trace("RequestDataByType handler invoked for request ID {} with message {} out of {}.",
                    requestId, dataMsg.dwentrynumber, dataMsg.dwoutof);
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
			logger.debug("Not using mapping for requestDataByType with request ID {}.", requestId);
            this->registerHandler(requestId, [&logger = simConnectMessageHandler_.connection().logger(), requestId, &dataDef, handler, onDone](const Messages::MsgBase& msg) {
                const Messages::SimObjectDataByTypeMsg& dataMsg = reinterpret_cast<const Messages::SimObjectDataByTypeMsg&>(msg);

                StructType data;

                logger.trace("RequestDataByType handler invoked for request ID {} with message {} out of {}.",
                    requestId, dataMsg.dwentrynumber, dataMsg.dwoutof);
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
        simConnectMessageHandler_.connection().requestDataByType(dataDef, requestId, radiusInMeters, objectType);

        return Request{ requestId, [this, &dataDef, requestId]() {
            stopDataRequest(dataDef, requestId);
        } };
    }


    /**
     * Requests data by SimObject type, collected in an unordered map. The caller passes a handler that will
     * be executed once the data is received. The handler will receive a reference to an ephemeral structure
     * that has the data unmarshalled into it.
     *
     * @note Discarding or deleting the Request object will stop the request.
     *
     * @param dataDef The data definition to use for the request.
     * @param handler The handler to execute when the data is received.
     * @param radiusInMeters The radius of the area for which to request data. If 0, only the user's aircraft is in scope.
     * @param objectType The type of SimObject to request data for.
     * @return A Request object that can be used to stop the request.
     * @tparam StructType The type of the structure to receive the data in.
     */
    template <typename StructType>
    [[nodiscard]]
    Request requestDataByType(DataDefinition<StructType>& dataDef,
        std::function<void(std::unordered_map<unsigned long, StructType>&)> handler,
        unsigned long radiusInMeters,
        SimObjectType objectType)
    {
        if (!handler) {
            throw std::invalid_argument("Handler must not be null.");
        }
        auto& logger = simConnectMessageHandler_.connection().logger();

        dataDef.define(simConnectMessageHandler_.connection());
        logger.debug("Data definition ID {} for requestDataByType.", dataDef.id());

        const auto requestId = simConnectMessageHandler_.connection().requests().nextRequestID();
        auto result = std::make_shared<std::unordered_map<unsigned long, StructType>>();

        if (dataDef.useMapping()) {
            logger.debug("Using mapping for requestDataByType with request ID {}.", requestId);
            this->registerHandler(requestId,
                            [&logger, requestId, handler, result](const Messages::MsgBase& msg) mutable
                {
                    const Messages::SimObjectDataByTypeMsg& dataMsg = reinterpret_cast<const Messages::SimObjectDataByTypeMsg&>(msg);
                    const StructType& data = reinterpret_cast<const StructType&>(dataMsg.dwData);

					(*result)[dataMsg.dwObjectID] = data;

                    logger.trace("RequestDataByType (map) handler invoked for request ID {} with message {} out of {} for ObjectID {}, {} record(s) collected.",
                        requestId, dataMsg.dwentrynumber, dataMsg.dwoutof, dataMsg.dwObjectID, result->size());
                    if (dataMsg.dwentrynumber == dataMsg.dwoutof) {
						handler(*result);
                    }
                }, false);
        }
        else {
            logger.debug("Not using mapping for requestDataByType with request ID {}.", requestId);
            this->registerHandler(requestId, [&logger, requestId, &dataDef, handler, result](const Messages::MsgBase& msg) mutable
                {
                    const Messages::SimObjectDataByTypeMsg& dataMsg = reinterpret_cast<const Messages::SimObjectDataByTypeMsg&>(msg);

                    StructType data;

                    dataDef.unmarshall(dataMsg, data);
					storeObjectId(dataMsg.dwObjectID, data);
                    (*result)[dataMsg.dwObjectID] = data;

                    logger.trace("RequestDataByType (map) handler invoked for request ID {} with message {} out of {} for ObjectID {}, {} record(s) collected.",
                        requestId, dataMsg.dwentrynumber, dataMsg.dwoutof, dataMsg.dwObjectID, result->size());
                    if (dataMsg.dwentrynumber == dataMsg.dwoutof) {
                        handler(*result);
                    }
                }, false);
        }
        simConnectMessageHandler_.connection().requestDataByType(dataDef, requestId, radiusInMeters, objectType);

        return Request{ requestId, [this, &dataDef, requestId]() {
            stopDataRequest(dataDef, requestId);
        } };
    }


    /**
     * Requests data for all SimObjects of a specific type. The caller passes a handler that will be executed once the
     * data is received. The handler will receive a const reference to the (raw) message data.
     * 
     * @note An "OutOfBounds" exception message will be sent if the radius exceeds the maximum allowed, which is 200,000 meters or 200 km.
     * 
     * @param dataDef The data definition Id to use for the request.
     * @param handler The handler to execute when the data is received.
     * @param onDone An optional handler to execute when all data has been received.
     * @param radiusInMeters The radius of the area for which to request data. If 0, only the user's aircraft is in scope.
     * @param objectType The type of SimObject to request data for.
	 */
    [[nodiscard]]
    Request requestDataByType(DataDefinitionId dataDef,
        std::function<void(const Messages::SimObjectDataByTypeMsg&)> handler, std::function<void()> onDone,
        unsigned long radiusInMeters,
        SimObjectType objectType)
    {
        auto requestId = simConnectMessageHandler_.connection().requests().nextRequestID();

        this->registerHandler(requestId, [requestId, handler, onDone](const Messages::MsgBase& msg) {
			const Messages::SimObjectDataByTypeMsg& dataMsg = reinterpret_cast<const Messages::SimObjectDataByTypeMsg&>(msg);
            handler(dataMsg);
            if (dataMsg.dwentrynumber == dataMsg.dwoutof) {
                if (onDone) {
                    onDone();
                }
            }
            }, false);
        simConnectMessageHandler_.connection().requestDataByType(dataDef, requestId, radiusInMeters, objectType);

        return Request{ requestId, [this, &dataDef, requestId]() {
            stopDataRequest(dataDef, requestId);
		} };
	}

#pragma endregion

#pragma region Send data methods

    /**
     * Sends raw data to a SimObject.
     * 
     * @param dataDef The data definition Id to use for the send.
     * @param objectId The object ID to send the data to. Defaults to the current user's Avatar or Aircraft.
     * @param data The raw data to send.
     */
    void sendData(DataDefinitionId dataDef, SimObjectId objectId, const void* data)
    {
        simConnectMessageHandler_.connection().sendData(dataDef, objectId, data);
    }


    /**
     * Sends data to a SimObject.
     * 
     * @param dataDef The data definition to use for the send.
     * @param objectId The object ID to send the data to. Defaults to the current user's Avatar or Aircraft.
     * @param data The data to send.
     * @tparam StructType The type of the structure containing the data to send.
     */
    template <typename StructType>
    void sendData(DataDefinition<StructType>& dataDef, SimObjectId objectId, const StructType& data)
    {
        dataDef.define(simConnectMessageHandler_.connection());

        simConnectMessageHandler_.connection().sendData(dataDef, objectId, data);
    }

#pragma endregion

};

} // namespace SimConnect