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

#include <cstring>
#include <type_traits>
#include <functional>
#include <vector>

#include <simconnect/simconnect.hpp>
#include <simconnect/data/client_data_definition_base.hpp>
#include <simconnect/data/data_block_reader.hpp>
#include <simconnect/data/data_block_builder.hpp>


namespace SimConnect {


/**
 * Client data definition for a struct whose fields map exactly onto SimConnect's primitive types.
 *
 * Each field is registered via a member pointer. Because the member type must exactly match the
 * SimConnect primitive (e.g. `int32_t` for `ClientDataType::int32`, `float` for `float32`), no
 * cross-type conversion is performed.
 *
 * **Three transfer modes — all supported by one definition:**
 *
 * - **Untagged (mapped):** when the wire layout matches the struct exactly
 *   (`useMapping() == true`), incoming data is reinterpret_cast'd directly to `StructType*`.
 *   No getters/setters are called. Zero overhead over `RawClientDataDefinition`.
 *
 * - **Tagged:** datum IDs interleaved in the wire data are matched against registered fields.
 *   Getters/setters are auto-derived from the member pointer at registration time.
 *
 * - **On-change (`TrackChanges = true`):** tagged partial updates are accumulated into
 *   `lastKnown_` so the handler always receives the full current struct, not just the
 *   changed fields. Required when using `SIMCONNECT_CLIENT_DATA_PERIOD_ON_SET` with tagged
 *   format, because SimConnect only sends fields that changed.
 *
 * Field layout must match the wire order. The caller is responsible for ensuring no
 * unexpected struct padding breaks the untagged mapped cast.
 *
 * Usage:
 * @code
 *   MappedClientDataDefinition<MyData> def;
 *   def.addInt32(&MyData::altitude).addFloat32(&MyData::speed);
 *   auto req = dataHandler.requestClientData(dataId, def,
 *       [](const MyData& d) { ... });
 * @endcode
 *
 * @tparam StructType    The C++ struct whose fields map onto the client data area.
 * @tparam TrackChanges  If true, accumulates tagged partial updates into an internal
 *                       `lastKnown_` buffer and delivers the full struct on each callback.
 */
template <typename StructType, bool TrackChanges = false>
class MappedClientDataDefinition
    : public ClientDataDefinitionBase<MappedClientDataDefinition<StructType, TrackChanges>>
{
    struct FieldInfo {
        ClientDataType type{};
        float epsilon{ 0.0f };
        unsigned long datumId{ unused };
        std::function<void(StructType&, Data::DataBlockReader&)> setter;
        std::function<void(Data::DataBlockBuilder&, const StructType&)> getter;
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
     * True when the total registered wire size equals sizeof(StructType), meaning an
     * untagged receive can be served by a direct reinterpret_cast instead of field-by-field
     * unmarshalling. The caller must ensure no struct padding breaks the layout.
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
     *   - Untagged + !useMapping(): field-by-field unmarshall into a temporary (or lastKnown_).
     *   - Tagged: parse datum/value pairs, updating only changed fields.
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

    MappedClientDataDefinition& addInt8(int8_t StructType::* field, float epsilon = 0.0f) {
        fields_.emplace_back(ClientDataType::int8, epsilon, unused,
            [field](StructType& data, Data::DataBlockReader& reader) { data.*field = reader.readInt8(); },
            [field](Data::DataBlockBuilder& builder, const StructType& data) { builder.addInt8(data.*field); });
        this->size_ += sizeof(int8_t);
        return *this;
    }

    MappedClientDataDefinition& addInt16(int16_t StructType::* field, float epsilon = 0.0f) {
        fields_.emplace_back(ClientDataType::int16, epsilon, unused,
            [field](StructType& data, Data::DataBlockReader& reader) { data.*field = reader.readInt16(); },
            [field](Data::DataBlockBuilder& builder, const StructType& data) { builder.addInt16(data.*field); });
        this->size_ += sizeof(int16_t);
        return *this;
    }

    MappedClientDataDefinition& addInt32(int32_t StructType::* field, float epsilon = 0.0f) {
        fields_.emplace_back(ClientDataType::int32, epsilon, unused,
            [field](StructType& data, Data::DataBlockReader& reader) { data.*field = reader.readInt32(); },
            [field](Data::DataBlockBuilder& builder, const StructType& data) { builder.addInt32(data.*field); });
        this->size_ += sizeof(int32_t);
        return *this;
    }

    MappedClientDataDefinition& addInt64(int64_t StructType::* field, float epsilon = 0.0f) {
        fields_.emplace_back(ClientDataType::int64, epsilon, unused,
            [field](StructType& data, Data::DataBlockReader& reader) { data.*field = reader.readInt64(); },
            [field](Data::DataBlockBuilder& builder, const StructType& data) { builder.addInt64(data.*field); });
        this->size_ += sizeof(int64_t);
        return *this;
    }

    MappedClientDataDefinition& addFloat32(float StructType::* field, float epsilon = 0.0f) {
        fields_.emplace_back(ClientDataType::float32, epsilon, unused,
            [field](StructType& data, Data::DataBlockReader& reader) { data.*field = reader.readFloat32(); },
            [field](Data::DataBlockBuilder& builder, const StructType& data) { builder.addFloat32(data.*field); });
        this->size_ += sizeof(float);
        return *this;
    }

    MappedClientDataDefinition& addFloat64(double StructType::* field, float epsilon = 0.0f) {
        fields_.emplace_back(ClientDataType::float64, epsilon, unused,
            [field](StructType& data, Data::DataBlockReader& reader) { data.*field = reader.readFloat64(); },
            [field](Data::DataBlockBuilder& builder, const StructType& data) { builder.addFloat64(data.*field); });
        this->size_ += sizeof(double);
        return *this;
    }

    /**
     * Register a raw-bytes field of sizeof(T) bytes.
     *
     * Tagged mode uses memcpy; untagged mode uses the direct reinterpret_cast path
     * when useMapping() is true (i.e. total wire size equals sizeof(StructType)).
     * No epsilon — change detection is not meaningful for opaque byte blobs.
     *
     * @tparam T    Type of the struct member. Must be trivially copyable.
     * @param field Member pointer into StructType.
     */
    template <typename T>
    MappedClientDataDefinition& addRaw(T StructType::* field) {
        static_assert(std::is_trivially_copyable_v<T>, "addRaw requires a trivially copyable type");
        fields_.emplace_back(ClientDataType{}, 0.0f, unused,
            [field](StructType& data, Data::DataBlockReader& reader) {
                auto bytes = reader.readBytes(sizeof(T));
                std::memcpy(&(data.*field), bytes.data(), sizeof(T));
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addBytes(reinterpret_cast<const uint8_t*>(&(data.*field)), sizeof(T));  //NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
            }
        );
        fields_.back().rawByteSize = sizeof(T);
        this->size_ += sizeof(T);
        return *this;
    }

#pragma endregion // Field registration

};


} // namespace SimConnect
