#pragma once
/*
 * Copyright (c) 2026. Bert Laverman
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
#include <atomic>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

#include <simconnect.hpp>
#include <simconnect/simconnect.hpp>
#include <simconnect/message_handler.hpp>
#include <simconnect/data/raw_client_data_definition.hpp>
#include <simconnect/data/mapped_client_data_definition.hpp>
#include <simconnect/data/custom_client_data_definition.hpp>
#include <simconnect/data/stateless_client_data_definition.hpp>
#include <simconnect/data/data_block_reader.hpp>


namespace SimConnect {


/**
 * The ClientDataHandler class provides for responsive handling of Messages::ClientDataMsg messages.
 * 
 * @note This handler is used to request data from the simulator for a specific object or type.
 * @note This handler can keep track of client data names if storeClientDataNames is true, but this is not enabled by default as it adds some overhead.
 * 
 * @tparam M The type of the SimConnect message handler, which must be derived from SimConnectMessageHandler.
 * @tparam storeClientDataNames Whether to store client data names.
 */
template <class M, bool storeClientDataNames = false>
class ClientDataHandler
    : public MessageHandler<RequestId, ClientDataHandler<M, storeClientDataNames>, M, Messages::clientData>
{
public:
    using simconnect_message_handler_type = M;
    using connection_type = typename M::connection_type;
    using mutex_type = typename connection_type::mutex_type;
    using guard_type = typename connection_type::guard_type;
    using lock_type = typename connection_type::lock_type;

    using ClientDataNames = std::conditional_t<storeClientDataNames, std::vector<std::string>, std::monostate>;
    using ClientDataNameMap = std::conditional_t<storeClientDataNames, std::unordered_map<std::string, ClientDataId>, std::monostate>;

private:
    simconnect_message_handler_type& simConnectMessageHandler_;

    mutex_type clientDataNamesMutex_;
    ClientDataNames clientDataNames_;
    ClientDataNameMap clientDataNameMap_;


    // No copies or moves
    ClientDataHandler(const ClientDataHandler&) = delete;
    ClientDataHandler(ClientDataHandler&&) = delete;
    ClientDataHandler& operator=(const ClientDataHandler&) = delete;
    ClientDataHandler& operator=(ClientDataHandler&&) = delete;


public:
    ClientDataHandler(simconnect_message_handler_type& handler) : simConnectMessageHandler_(handler)
    {
        this->enable(simConnectMessageHandler_);
    }
    ~ClientDataHandler() = default;


    /**
     * Returns the SimConnect message handler.
     */
    [[nodiscard]]
    simconnect_message_handler_type& simConnectMessageHandler() const noexcept {
        return simConnectMessageHandler_;
    }


    /**
     * Returns the request ID from the message. This is specific to the Messages::ClientDataMsg messages.
     * 
     * @param msg The message to get the correlation ID from.
     * @returns The correlation ID from the message.
	 */
    unsigned long correlationId(const Messages::MsgBase& msg) {
        return static_cast<const Messages::ClientDataMsg&>(msg).dwRequestID;
    }


#pragma region Client Data Areas Creation and mapping

    /**
     * Maps a client data name to a client data ID. If the name has already been mapped and stored, the existing ID is returned.
     * 
     * @note If storeClientDataNames is false, the name will not be stored and a new ID will be generated for each call.
     * 
     * @param name The client data name to map.
     * @return The client data ID that the name is mapped to.
     */
    ClientDataId mapClientDataName(std::string_view name) {
        static std::atomic_int32_t nextId{ 1 }; // start at 1 to avoid using noClientDataId (0) as a valid ID

        ClientDataId id{ noClientDataId };
        auto nameString = std::string{ name };

        if constexpr (storeClientDataNames) {
            lock_type lock(clientDataNamesMutex_);
            auto findIt = clientDataNameMap_.find(nameString);
            if (findIt != clientDataNameMap_.end()) {
                return findIt->second;
            }
            id = static_cast<ClientDataId>(nextId++);
            simConnectMessageHandler_.connection().mapClientDataName(id, nameString);
            if (!simConnectMessageHandler_.connection()) {
                auto expected = static_cast<std::int32_t>(id + 1);
                nextId.compare_exchange_strong(expected, static_cast<std::int32_t>(id));
                return noClientDataId;
            }
            if (id >= clientDataNames_.size()) {
                clientDataNames_.resize(id + 1, std::string{});
            }
            clientDataNames_[id] = nameString;
            clientDataNameMap_[std::move(nameString)] = id;
        }
        else {
             id = static_cast<ClientDataId>(nextId++); // no fancy stuff, always generate a new ID and hope for the best.
             simConnectMessageHandler_.connection().mapClientDataName(id, nameString);
             if (!simConnectMessageHandler_.connection()) {
                auto expected = static_cast<std::int32_t>(id + 1);
                nextId.compare_exchange_strong(expected, static_cast<std::int32_t>(id));
                return noClientDataId;
            }
        }

        return id;
    }


    /**
     * Gets the client data ID that a client data name is mapped to.
     * 
     * @param name The client data name to get the ID for.
     * @return The client data ID that the name is mapped to, or noClientDataId if the name is not mapped.
     */
    ClientDataId getClientDataId(std::string_view name) const {
        if constexpr (storeClientDataNames) {
            lock_type lock(clientDataNamesMutex_);
            auto findIt = clientDataNameMap_.find(std::string{ name });
            if (findIt != clientDataNameMap_.end()) {
                return findIt->second;
            }
        }
        return noClientDataId;
    }


    /**
     * Gets the client data name that a client data ID is mapped to.
     * 
     * @param id The client data ID to get the name for.
     * @return The client data name that the ID is mapped to, or an empty string view if the ID is not mapped.
     */
    std::string_view getClientDataName(ClientDataId id) const {
        if constexpr (storeClientDataNames) {
            lock_type lock(clientDataNamesMutex_);
            if (id < clientDataNames_.size()) {
                return clientDataNames_[id];
            }
        }
        return std::string_view{};
    }


    /**
     * Creates a client data definition with the specified client data ID and data size.
     * 
     * @param clientDataId The client data ID to create the definition for.
     * @param dataSize The size of the client data.
     * @return True if the client data definition was created successfully, false otherwise.
     */
    bool createClientData(ClientDataId clientDataId, std::size_t dataSize) {
        return simConnectMessageHandler_.connection().createClientData(clientDataId, dataSize, false);
    }


    /**
     * Creates a shared client data definition with the specified client data ID and data size.
     * 
     * @param clientDataId The client data ID to create the definition for.
     * @param dataSize The size of the client data.
     * @return True if the shared client data definition was created successfully, false otherwise.
     */
    bool createSharedClientData(ClientDataId clientDataId, std::size_t dataSize) {
        return simConnectMessageHandler_.connection().createClientData(clientDataId, dataSize, true);
    }

#pragma endregion Client Data Areas

#pragma region Stopping Requests

    /**
     * Stops a client data request and removes the handler if still active.
     * 
     * @param clientDataId The client data ID to stop the request for.
     * @param dataDef The data definition ID to stop the request for.
     * @param requestId The request ID to stop.
     * @note If the request is not active, this will do nothing.
     */
    void stopClientDataRequest(ClientDataId clientDataId, ClientDataDefinitionId dataDef, RequestId requestId)
    {
        this->removeHandler(requestId);
        simConnectMessageHandler_.connection().stopClientDataRequest(clientDataId, dataDef, requestId);
    }

#pragma endregion // Stopping Requests

#pragma region Client Data Sending

    /**
     * Send a struct to a client data area using a MappedClientDataDefinition (untagged).
     *
     * Valid only when `def.useMapping() == true` — i.e. every field was registered and the
     * total wire size equals `sizeof(StructType)`. Logs an error and returns without sending
     * if the condition is not met; use sendClientDataTagged() in that case.
     */
    template <typename StructType, bool TrackChanges>
    void sendClientData(ClientDataId clientDataId, MappedClientDataDefinition<StructType, TrackChanges>& def, const StructType& data)
    {
        if (!def.useMapping()) {
            simConnectMessageHandler_.connection().logger().error(
                "sendClientData: MappedClientDataDefinition wire size ({}) != sizeof(StructType) ({}); "
                "use sendClientDataTagged() instead.",
                def.size(), sizeof(StructType));
            return;
        }
        simConnectMessageHandler_.connection().sendClientData(clientDataId, def.id(), data);
    }

    /**
     * Send a struct to a client data area using a MappedClientDataDefinition (tagged datum/value format).
     */
    template <typename StructType, bool TrackChanges>
    void sendClientDataTagged(ClientDataId clientDataId, MappedClientDataDefinition<StructType, TrackChanges>& def, const StructType& data)
    {
        simConnectMessageHandler_.connection().sendClientDataTagged(clientDataId, def.id(), data);
    }

    /**
     * Send a struct to a client data area using a CustomClientDataDefinition (tagged datum/value format).
     *
     * @note Untagged send is not available for CustomClientDataDefinition — useMapping() is always
     *       false because user-supplied getters/setters make the wire layout unpredictable.
     *       Always use this tagged overload.
     */
    template <typename StructType, bool TrackChanges>
    void sendClientDataTagged(ClientDataId clientDataId, CustomClientDataDefinition<StructType, TrackChanges>& def, const StructType& data)
    {
        simConnectMessageHandler_.connection().sendClientDataTagged(clientDataId, def.id(), data);
    }

#pragma endregion // Client Data Sending

#pragma region Raw message Client Data Requests

    /**
     * Request a client data block. The caller passes a handler that will be executed once the data is received.
     * The handler will receive a const reference to the raw message data.
     *
     * @note Discarding or deleting the Request object will stop the request.
     *
     * @param clientDataId The client data ID to request.
     * @param definitionId The client data definition ID to use for the request.
     * @param handler The handler to execute when the data is received.
     * @param frequency The frequency at which to request the data.
     * @param limits The limits for the request in numbers of "periods".
     * @param onlyWhenChanged Whether to only receive updates when the data has changed.
     * @return A Request object that can be used to stop the request.
     */
    [[nodiscard]]
    Request requestClientData(
        ClientDataId clientDataId, ClientDataDefinitionId definitionId,
        std::function<void(const Messages::ClientDataMsg&)> handler,
        ClientDataFrequency frequency = ClientDataFrequency::once(),
        PeriodLimits limits = PeriodLimits::none(),
        bool onlyWhenChanged = false)
    {
        const auto requestId = simConnectMessageHandler_.connection().requests().nextRequestID();

        this->registerHandler(requestId, [handler](const Messages::MsgBase& msg) {
            handler(reinterpret_cast<const Messages::ClientDataMsg&>(msg));
            }, frequency.isOnce());
        simConnectMessageHandler_.connection().requestClientData(clientDataId, definitionId, requestId, frequency, limits, onlyWhenChanged);

        return frequency.isOnce() ? Request{ requestId } : Request{ requestId, [this, clientDataId, definitionId, requestId]() { this->stopClientDataRequest(clientDataId, definitionId, requestId); } };
    }


    /**
     * Request a client data block once. The caller passes a handler that will be executed once the data is received.
     * The handler will receive a const reference to the raw message data.
     *
     * @note Discarding or deleting the Request object will stop the request.
     *
     * @param clientDataId The client data ID to request.
     * @param definitionId The client data definition ID to use for the request.
     * @param handler The handler to execute when the data is received.
     * @return A Request object that can be used to stop the request.
     */
    [[nodiscard]]
    Request requestClientDataOnce(
        ClientDataId clientDataId, ClientDataDefinitionId definitionId,
        std::function<void(const Messages::ClientDataMsg&)> handler)
    {
        return requestClientData(clientDataId, definitionId, handler, ClientDataFrequency::once());
    }


    /**
     * Request a client data block in the tagged format. The caller passes a handler that will be executed once the data is received.
     * The handler will receive a const reference to the raw message data.
     *
     * @note Discarding or deleting the Request object will stop the request.
     *
     * @param clientDataId The client data ID to request.
     * @param definitionId The client data definition ID to use for the request.
     * @param handler The handler to execute when the data is received.
     * @param frequency The frequency at which to request the data.
     * @param limits The limits for the request in numbers of "periods".
     * @param onlyWhenChanged Whether to only receive updates when the data has changed.
     * @return A Request object that can be used to stop the request.
     */
    [[nodiscard]]
    Request requestClientDataTagged(
        ClientDataId clientDataId, ClientDataDefinitionId definitionId,
        std::function<void(const Messages::ClientDataMsg&)> handler,
        ClientDataFrequency frequency = ClientDataFrequency::once(),
        PeriodLimits limits = PeriodLimits::none(),
        bool onlyWhenChanged = false)
    {
        const auto requestId = simConnectMessageHandler_.connection().requests().nextRequestID();

        this->registerHandler(requestId, [handler](const Messages::MsgBase& msg) {
            handler(reinterpret_cast<const Messages::ClientDataMsg&>(msg));
            }, frequency.isOnce());
        simConnectMessageHandler_.connection().requestClientDataTagged(clientDataId, definitionId, requestId, frequency, limits, onlyWhenChanged);

        return frequency.isOnce() ? Request{ requestId } : Request{ requestId, [this, clientDataId, definitionId, requestId]() { this->stopClientDataRequest(clientDataId, definitionId, requestId); } };
    }


    /**
     * Request a client data block once in the tagged format. The caller passes a handler that will be executed once the data is received.
     * The handler will receive a const reference to the raw message data.
     *
     * @note Discarding or deleting the Request object will stop the request.
     *
     * @param clientDataId The client data ID to request.
     * @param definitionId The client data definition ID to use for the request.
     * @param handler The handler to execute when the data is received.
     * @return A Request object that can be used to stop the request.
     */
    [[nodiscard]]
    Request requestClientDataOnceTagged(
        ClientDataId clientDataId, ClientDataDefinitionId definitionId,
        std::function<void(const Messages::ClientDataMsg&)> handler)
    {
        return requestClientDataTagged(clientDataId, definitionId, handler, ClientDataFrequency::once());
    }

#pragma endregion // Raw message Client Data Requests

#pragma region DataBlockReader Client Data Requests

    /**
     * Request a client data block. The caller passes a handler that will be executed once the data is received.
     * The handler will receive a reference to a DataBlockReader that can be used to read the data.
     *
     * @note Discarding or deleting the Request object will stop the request.
     *
     * @param clientDataId The client data ID to request.
     * @param definitionId The client data definition ID to use for the request.
     * @param handler The handler to execute when the data is received.
     * @param frequency The frequency at which to request the data.
     * @param limits The limits for the request in numbers of "periods".
     * @param onlyWhenChanged Whether to only receive updates when the data has changed.
     * @return A Request object that can be used to stop the request.
     */
    [[nodiscard]]
    Request requestClientData(
        ClientDataId clientDataId, ClientDataDefinitionId definitionId,
        std::function<void(Data::DataBlockReader&)> handler,
        ClientDataFrequency frequency = ClientDataFrequency::once(),
        PeriodLimits limits = PeriodLimits::none(),
        bool onlyWhenChanged = false)
    {
        const auto requestId = simConnectMessageHandler_.connection().requests().nextRequestID();

        this->registerHandler(requestId, [handler](const Messages::MsgBase& msg) {
            Data::DataBlockReader reader(reinterpret_cast<const Messages::SimObjectDataMsg&>(msg));
            handler(reader);
            }, frequency.isOnce());
        simConnectMessageHandler_.connection().requestClientData(clientDataId, definitionId, requestId, frequency, limits, onlyWhenChanged);

        return frequency.isOnce() ? Request{ requestId } : Request{ requestId, [this, clientDataId, definitionId, requestId]() { this->stopClientDataRequest(clientDataId, definitionId, requestId); } };
    }


    /**
     * Request a client data block once. The caller passes a handler that will be executed once the data is received.
     * The handler will receive a reference to a DataBlockReader that can be used to read the data.
     *
     * @note Discarding or deleting the Request object will stop the request.
     *
     * @param clientDataId The client data ID to request.
     * @param definitionId The client data definition ID to use for the request.
     * @param handler The handler to execute when the data is received.
     * @return A Request object that can be used to stop the request.
     */
    [[nodiscard]]
    Request requestClientDataOnce(
        ClientDataId clientDataId, ClientDataDefinitionId definitionId,
        std::function<void(Data::DataBlockReader&)> handler)
    {
        return requestClientData(clientDataId, definitionId, handler, ClientDataFrequency::once());
    }


    /**
     * Request a client data block in the tagged format. The caller passes a handler that will be executed once the data is received.
     * The handler will receive a reference to a DataBlockReader that can be used to read the data.
     *
     * @note Discarding or deleting the Request object will stop the request.
     *
     * @param clientDataId The client data ID to request.
     * @param definitionId The client data definition ID to use for the request.
     * @param handler The handler to execute when the data is received.
     * @param frequency The frequency at which to request the data.
     * @param limits The limits for the request in numbers of "periods".
     * @param onlyWhenChanged Whether to only receive updates when the data has changed.
     * @return A Request object that can be used to stop the request.
     */
    [[nodiscard]]
    Request requestClientDataTagged(
        ClientDataId clientDataId, ClientDataDefinitionId definitionId,
        std::function<void(Data::DataBlockReader&)> handler,
        ClientDataFrequency frequency = ClientDataFrequency::once(),
        PeriodLimits limits = PeriodLimits::none(),
        bool onlyWhenChanged = false)
    {
        const auto requestId = simConnectMessageHandler_.connection().requests().nextRequestID();

        this->registerHandler(requestId, [handler](const Messages::MsgBase& msg) {
            Data::DataBlockReader reader(reinterpret_cast<const Messages::SimObjectDataMsg&>(msg));
            handler(reader);
            }, frequency.isOnce());
        simConnectMessageHandler_.connection().requestClientDataTagged(clientDataId, definitionId, requestId, frequency, limits, onlyWhenChanged);

        return frequency.isOnce() ? Request{ requestId } : Request{ requestId, [this, clientDataId, definitionId, requestId]() { this->stopClientDataRequest(clientDataId, definitionId, requestId); } };
    }


    /**
     * Request a client data block once in the tagged format. The caller passes a handler that will be executed once the data is received.
     * The handler will receive a reference to a DataBlockReader that can be used to read the data.
     *
     * @note Discarding or deleting the Request object will stop the request.
     *
     * @param clientDataId The client data ID to request.
     * @param definitionId The client data definition ID to use for the request.
     * @param handler The handler to execute when the data is received.
     * @return A Request object that can be used to stop the request.
     */
    [[nodiscard]]
    Request requestClientDataOnceTagged(
        ClientDataId clientDataId, ClientDataDefinitionId definitionId,
        std::function<void(Data::DataBlockReader&)> handler)
    {
        return requestClientDataTagged(clientDataId, definitionId, handler, ClientDataFrequency::once());
    }

#pragma endregion // DataBlockReader Client Data Requests

#pragma region RawClientDataDefinition Client Data Requests

    /**
     * Send a struct to a client data area using a RawClientDataDefinition.
     */
    template <typename StructType>
    void sendClientData(ClientDataId clientDataId, RawClientDataDefinition<StructType>& def, const StructType& data)
    {
        simConnectMessageHandler_.connection().sendClientData(clientDataId, def.id(), data);
    }


    /**
     * Request client data, delivering each update as a const StructType reference.
     * Defines the area if not yet defined.
     */
    template <typename DefType, typename HandlerType>
        requires ClientDataDefinitionConcept<DefType, decltype(simConnectMessageHandler_.connection())> &&
                 ClientDataHandlerConcept<HandlerType, typename DefType::struct_type>
    [[nodiscard]]
    Request requestClientData(
        ClientDataId clientDataId,
        DefType& def,
        HandlerType handler,
        ClientDataFrequency frequency = ClientDataFrequency::once(),
        PeriodLimits limits = PeriodLimits::none(),
        bool onlyWhenChanged = false)
    {
        def.define(simConnectMessageHandler_.connection());
        const auto defId = def.id();
        const auto requestId = simConnectMessageHandler_.connection().requests().nextRequestID();

        this->registerHandler(requestId, [&def, handler](const Messages::MsgBase& msg) {
            def.dispatch(reinterpret_cast<const Messages::ClientDataMsg&>(msg), handler);  //NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
        }, frequency.isOnce());
        simConnectMessageHandler_.connection().requestClientData(clientDataId, defId, requestId, frequency, limits, onlyWhenChanged);

        return frequency.isOnce()
            ? Request{ requestId }
            : Request{ requestId, [this, clientDataId, defId, requestId]() {
                this->stopClientDataRequest(clientDataId, defId, requestId);
            }};
    }


    /**
     * Request client data once, delivering the update as a const StructType reference.
     */
    template <typename StructType>
    [[nodiscard]]
    Request requestClientDataOnce(
        ClientDataId clientDataId, RawClientDataDefinition<StructType>& def,
        std::function<void(const StructType&)> handler)
    {
        return requestClientData(clientDataId, def, std::move(handler), ClientDataFrequency::once());
    }


    /**
     * Request client data in tagged format, delivering each update as a const StructType reference.
     * For a raw definition there is only one datum, so tagged and untagged deliver identically.
     */
    template <typename StructType>
    [[nodiscard]]
    Request requestClientDataTagged(
        ClientDataId clientDataId, RawClientDataDefinition<StructType>& def,
        std::function<void(const StructType&)> handler,
        ClientDataFrequency frequency = ClientDataFrequency::once(),
        PeriodLimits limits = PeriodLimits::none(),
        bool onlyWhenChanged = false)
    {
        def.define(simConnectMessageHandler_.connection());
        const auto defId = def.id();
        const auto requestId = simConnectMessageHandler_.connection().requests().nextRequestID();

        this->registerHandler(requestId, [handler](const Messages::MsgBase& msg) {
            const StructType* data = reinterpret_cast<const StructType*>(  //NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
                &reinterpret_cast<const Messages::ClientDataMsg&>(msg).dwData);  //NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
            handler(*data);
        }, frequency.isOnce());
        simConnectMessageHandler_.connection().requestClientDataTagged(clientDataId, defId, requestId, frequency, limits, onlyWhenChanged);

        return frequency.isOnce()
            ? Request{ requestId }
            : Request{ requestId, [this, clientDataId, defId, requestId]() {
                this->stopClientDataRequest(clientDataId, defId, requestId);
            }};
    }


    /**
     * Request client data once in tagged format, delivering the update as a const StructType reference.
     */
    template <typename StructType>
    [[nodiscard]]
    Request requestClientDataOnceTagged(
        ClientDataId clientDataId, RawClientDataDefinition<StructType>& def,
        std::function<void(const StructType&)> handler)
    {
        return requestClientDataTagged(clientDataId, def, std::move(handler), ClientDataFrequency::once());
    }

#pragma endregion // RawClientDataDefinition Client Data Requests

#pragma region StatelessClientDataDefinition Client Data Requests

    /**
     * Send all bidirectional fields (those with getters) to a client data area.
     * Fields registered without a getter are skipped. Sends in untagged format.
     */
    void sendClientData(ClientDataId clientDataId, StatelessClientDataDefinition& def) {
        Data::DataBlockBuilder builder;
        def.marshal(builder);
        simConnectMessageHandler_.connection().sendClientData(clientDataId, def.id(), builder.dataBlock());
    }


    /**
     * Request client data, firing each datum's own callback on every update.
     * No outer handler — per-datum callbacks registered on `def` handle all delivery.
     */
    [[nodiscard]]
    Request requestClientData(
        ClientDataId clientDataId,
        StatelessClientDataDefinition& def,
        ClientDataFrequency frequency = ClientDataFrequency::once(),
        PeriodLimits limits = PeriodLimits::none(),
        bool onlyWhenChanged = false)
    {
        return requestClientData(clientDataId, def, [](){}, frequency, limits, onlyWhenChanged);
    }


    /**
     * Request client data, firing each datum's own callback on every update.
     * `done` is called after all per-datum callbacks complete each time data arrives.
     */
    template <typename HandlerFn>
        requires std::invocable<HandlerFn>
    [[nodiscard]]
    Request requestClientData(
        ClientDataId clientDataId,
        StatelessClientDataDefinition& def,
        HandlerFn done,
        ClientDataFrequency frequency = ClientDataFrequency::once(),
        PeriodLimits limits = PeriodLimits::none(),
        bool onlyWhenChanged = false)
    {
        def.define(simConnectMessageHandler_.connection());
        const auto defId = def.id();
        const auto requestId = simConnectMessageHandler_.connection().requests().nextRequestID();

        this->registerHandler(requestId, [&def, done](const Messages::MsgBase& msg) {
            def.dispatch(reinterpret_cast<const Messages::ClientDataMsg&>(msg), done);  //NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
        }, frequency.isOnce());
        simConnectMessageHandler_.connection().requestClientData(clientDataId, defId, requestId, frequency, limits, onlyWhenChanged);

        return frequency.isOnce()
            ? Request{ requestId }
            : Request{ requestId, [this, clientDataId, defId, requestId]() {
                this->stopClientDataRequest(clientDataId, defId, requestId);
            }};
    }

#pragma endregion // StatelessClientDataDefinition Client Data Requests

};

} // namespace SimConnect
