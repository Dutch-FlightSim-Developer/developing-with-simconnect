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

#include <cstddef>
#include <type_traits>
#include <functional>
#include <vector>

#include <simconnect/simconnect.hpp>
#include <simconnect/data/client_data_definition_base.hpp>
#include <simconnect/data/data_block_reader.hpp>
#include <simconnect/data/data_block_builder.hpp>


namespace SimConnect {


/**
 * Client data definition for a struct whose fields are dispatched via user-supplied
 * getter and setter callables.
 *
 * Use this when `MappedClientDataDefinition` member-pointer registration is too rigid:
 * scaling, clamping, unit conversion, conditional logic, or any stateful closure that
 * must run on every read or write.
 *
 * The setter receives a `DataBlockReader` positioned at the field's datum. The getter
 * receives a `DataBlockBuilder` to write into. Both receive a reference to the full struct.
 *
 * **Three transfer modes — same as MappedClientDataDefinition:**
 *
 * - **Untagged (mapped):** when `useMapping() == true` (total wire size equals
 *   `sizeof(StructType)`), incoming data is reinterpret_cast'd directly. Setters
 *   are not called in this path — only valid if the layout is genuinely packed.
 *
 * - **Tagged:** datum/value pairs; only registered setters for matching datum IDs run.
 *
 * - **On-change (`TrackChanges = true`):** tagged partial updates accumulate into
 *   `lastKnown_`; the handler always sees the full current struct.
 *
 * Usage:
 * @code
 *   const float scale = 0.01f;
 *   CustomClientDataDefinition<MyData> def;
 *   def.addField(ClientDataType::int32,
 *       [scale](MyData& d, Data::DataBlockReader& r) { d.altitude = r.readInt32() * scale; },
 *       [scale](Data::DataBlockBuilder& b, const MyData& d) { b.addInt32(static_cast<int32_t>(d.altitude / scale)); });
 *   auto req = dataHandler.requestClientData(dataId, def, [](const MyData& d) { ... });
 * @endcode
 *
 * @tparam StructType    The C++ struct receiving the client data.
 * @tparam TrackChanges  If true, accumulates tagged partial updates into an internal
 *                       `lastKnown_` buffer and delivers the full struct on each callback.
 */
template <typename StructType, bool TrackChanges = false>
class CustomClientDataDefinition
    : public ClientDataDefinitionBase<CustomClientDataDefinition<StructType, TrackChanges>>
{
public:
    using Setter = std::function<void(StructType&, Data::DataBlockReader&)>;
    using Getter = std::function<void(Data::DataBlockBuilder&, const StructType&)>;

private:
    struct FieldInfo {
        ClientDataType type{};
        float epsilon{ 0.0f };
        unsigned long datumId{ unused };
        Setter setter;
        Getter getter;
        size_t rawByteSize{ 0 };  // >0 → raw bytes field; 0 → typed field
    };

    std::vector<FieldInfo> fields_;

    struct NoState {};
    [[no_unique_address]] std::conditional_t<TrackChanges, StructType, NoState> lastKnown_{};


    void unmarshall(Data::DataBlockReader& reader, StructType& data) const {
        for (const auto& field : fields_) {
            field.setter(data, reader);
        }
    }

    void unmarshallTagged(Data::DataBlockReader& reader, StructType& data, unsigned long numElems) const {
        while (numElems-- > 0) {
            const auto id = static_cast<size_t>(reader.readInt32());
            if (id == 0 || id > fields_.size()) {
                continue;
            }
            fields_[id - 1].setter(data, reader);
        }
    }


public:
    using struct_type = StructType;
    using callback_type = std::function<void(const StructType&)>;


    /**
     * True when the total registered wire size equals sizeof(StructType).
     * When true, untagged receives use a direct reinterpret_cast — setters are not called.
     * The caller must ensure no struct padding breaks the layout.
     */
    [[nodiscard]]
    bool useMapping() const noexcept { return this->size_ == sizeof(StructType); }


    /**
     * The current accumulated state. Only available when TrackChanges = true.
     * Updated by every dispatch() call. Not thread-safe without external locking.
     */
    [[nodiscard]]
    const StructType& lastKnown() const noexcept requires TrackChanges { return lastKnown_; }


    /**
     * Register all fields with SimConnect, assigning datum IDs starting at 1.
     * Called once by ClientDataDefinitionBase::define().
     */
    template <class connection_type>
    void registerFields(connection_type& connection) {
        unsigned long datumId{ 1 };
        for (auto& field : fields_) {
            field.datumId = datumId++;
            if (field.rawByteSize > 0) {
                connection.addClientDataDefinition(this->id(), field.rawByteSize, clientDataAutoOffset, field.datumId);
            } else {
                connection.addClientDataDefinition(this->id(), field.type, clientDataAutoOffset, field.epsilon, field.datumId);
            }
        }
    }


    /**
     * Deliver a received client data message to the handler.
     *
     * Selects the dispatch path at runtime from dwFlags:
     *   - Untagged + useMapping(): reinterpret_cast directly to StructType*.
     *   - Untagged + !useMapping(): field-by-field via setters into a temporary (or lastKnown_).
     *   - Tagged: parse datum/value pairs, running only the matching setters.
     *
     * When TrackChanges = true, all paths update lastKnown_ before calling the handler.
     */
    template <typename HandlerFn>
    void dispatch(const Messages::ClientDataMsg& msg, HandlerFn&& handler) {
        const bool isTagged = (msg.dwFlags & DataRequestFlags::tagged) != 0;

        if constexpr (TrackChanges) {
            if (isTagged) {
                Data::DataBlockReader reader(static_cast<const Messages::SimObjectDataMsg&>(msg));
                unmarshallTagged(reader, lastKnown_, msg.dwDefineCount);
            } else if (useMapping()) {
                lastKnown_ = *reinterpret_cast<const StructType*>(&msg.dwData);  //NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
            } else {
                Data::DataBlockReader reader(static_cast<const Messages::SimObjectDataMsg&>(msg));
                unmarshall(reader, lastKnown_);
            }
            handler(lastKnown_);
        } else {
            if (!isTagged && useMapping()) {
                handler(*reinterpret_cast<const StructType*>(&msg.dwData));  //NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
            } else {
                StructType temp{};
                Data::DataBlockReader reader(static_cast<const Messages::SimObjectDataMsg&>(msg));
                if (isTagged) {
                    unmarshallTagged(reader, temp, msg.dwDefineCount);
                } else {
                    unmarshall(reader, temp);
                }
                handler(temp);
            }
        }
    }


    /**
     * Serialize data into a DataBlockBuilder for sending.
     *
     * @param builder  Target buffer.
     * @param data     The struct to serialize.
     * @param isTagged If true, each field is preceded by its datum ID (tagged send format).
     */
    void marshal(Data::DataBlockBuilder& builder, const StructType& data, bool isTagged = false) const {
        for (const auto& field : fields_) {
            if (isTagged) {
                builder.addInt32(static_cast<int32_t>(field.datumId));
            }
            field.getter(builder, data);
        }
    }


#pragma region Field registration

    /**
     * Register a typed SimConnect field with user-supplied setter and getter.
     *
     * The wire size is derived from `type`; it contributes to `size_` for `useMapping()` tracking.
     * Use this for any of the six SimConnect primitive types (int8/16/32/64, float32/64) when
     * the field needs custom logic beyond a simple member-pointer assignment.
     *
     * @param type     SimConnect datum type.
     * @param setter   Called on receive: reads the datum from `reader` and writes into `data`.
     * @param getter   Called on send: reads from `data` and writes the datum into `builder`.
     * @param epsilon  Change-detection threshold (only meaningful with onlyWhenChanged requests).
     */
    CustomClientDataDefinition& addField(ClientDataType type, Setter setter, Getter getter, float epsilon = 0.0f) {
        const auto wireSize = sizeOf(type);
        fields_.emplace_back(type, epsilon, unused, std::move(setter), std::move(getter));
        this->size_ += wireSize;
        return *this;
    }


    /**
     * Register a raw-bytes field with user-supplied setter and getter.
     *
     * No epsilon — change detection is not meaningful for opaque byte blobs.
     *
     * @param byteSize Wire size in bytes. Contributes to `size_` for `useMapping()` tracking.
     * @param setter   Called on receive: reads `byteSize` bytes from `reader` and writes into `data`.
     * @param getter   Called on send: reads from `data` and writes `byteSize` bytes into `builder`.
     */
    CustomClientDataDefinition& addRawField(size_t byteSize, Setter setter, Getter getter) {
        fields_.emplace_back(ClientDataType{}, 0.0f, unused, std::move(setter), std::move(getter));
        fields_.back().rawByteSize = byteSize;
        this->size_ += byteSize;
        return *this;
    }

#pragma endregion // Field registration

};


} // namespace SimConnect
