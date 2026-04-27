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

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <vector>
#include <functional>
#include <optional>

#include <simconnect/simconnect.hpp>
#include <simconnect/simconnect_exception.hpp>
#include <simconnect/connection.hpp>
#include <simconnect/data/data_block_builder.hpp>
#include <simconnect/data/data_block_reader.hpp>


namespace SimConnect {


/**
 * Returns the wire size in bytes for the given ClientDataType.
 */
constexpr size_t sizeOf(ClientDataType type) noexcept {
    switch (type) {
    case ClientDataType::int8:    return sizeof(int8_t);
    case ClientDataType::int16:   return sizeof(int16_t);
    case ClientDataType::int32:   return sizeof(int32_t);
    case ClientDataType::int64:   return sizeof(int64_t);
    case ClientDataType::float32: return sizeof(float);
    case ClientDataType::float64: return sizeof(double);
    default: return 0;
    }
}


/**
 * A client data definition block.
 *
 * Describes the layout of a SimConnect client data area for a specific C++ struct type.
 *
 * **Usage — creating client (owns the data area):**
 * @code
 *   connection.mapClientDataName(myClientDataId, "My.Data.Area");
 *   connection.createClientData(myClientDataId, sizeof(MyStruct));
 *   myDef.define(connection);
 * @endcode
 *
 * **Usage — consumer client (reads or writes the data area):**
 * @code
 *   connection.mapClientDataName(myClientDataId, "My.Data.Area");
 *   myDef.define(connection);    // same define() call, no createClientData
 * @endcode
 *
 * **Two modes:**
 *
 *  - **Raw** (most common): a single blob of `sizeof(StructType)` bytes registered via `addRaw()`.
 *    `useMapping()` returns `true`, allowing the incoming data buffer to be reinterpret_cast'd
 *    directly to `StructType*`. Use `connection.sendClientData(id, def.id(), myStruct)` to send.
 *
 *  - **Typed fields**: individual primitives added via `addInt8()` / `addInt16()` / `addInt32()` /
 *    `addInt64()` / `addFloat32()` / `addFloat64()`. Enables tagged data transfer (datum IDs are
 *    assigned automatically starting at 1). `useMapping()` is always `false` in this mode.
 *    Use `fill()` to populate a struct from received data, and `build()` to serialize a struct for
 *    sending via `connection.sendClientData()`.
 *
 * @tparam StructType The C++ type that maps onto the client data area.
 */
template <typename StructType>
class ClientDataDefinition
{
    /**
     * Function type for writing a field of the receiving struct from the DataBlockReader.
     */
    using SetterFunc = std::function<void(StructType&, Data::DataBlockReader&)>;

    /**
     * Function type for writing a field independently of any struct from the DataBlockReader.
     */
    using StatelessSetterFunc = std::function<void(Data::DataBlockReader&)>;

    /**
     * Function type for reading a field from the struct and writing it to the DataBlockBuilder.
     */
    using GetterFunc = std::function<void(Data::DataBlockBuilder&, const StructType&)>;

    /**
     * Function type for writing a field independently of any struct into the DataBlockBuilder.
     */
    using StatelessGetterFunc = std::function<void(Data::DataBlockBuilder&)>;


    /**
     * Information about a single field in the client data definition.
     */
    struct FieldInfo {
        bool isRaw{ true };
        std::size_t rawSize{ 0 };           ///< For raw fields: number of bytes.
        ClientDataType type{};              ///< For typed fields: the SimConnect primitive type.
        float epsilon{ 0.0f };             ///< For typed fields: change-detection threshold.
        unsigned long datumId{ unused };   ///< Assigned by define(); used for tagged data.

        SetterFunc setter;
        StatelessSetterFunc statelessSetter;
        GetterFunc getter;
        StatelessGetterFunc statelessGetter;

        /// Raw field constructor.
        FieldInfo(std::size_t rawSize, unsigned long datumId, SetterFunc setter, GetterFunc getter)
            : isRaw(true), rawSize(rawSize), datumId(datumId)
            , setter(std::move(setter)), getter(std::move(getter)) {}

        /// Typed field constructor (struct-based setter/getter).
        FieldInfo(ClientDataType type, float epsilon, unsigned long datumId, SetterFunc setter, GetterFunc getter)
            : isRaw(false), type(type), epsilon(epsilon), datumId(datumId)
            , setter(std::move(setter)), getter(std::move(getter)) {}

        /// Typed field constructor (stateless setter/getter).
        FieldInfo(ClientDataType type, float epsilon, unsigned long datumId, StatelessSetterFunc setter, StatelessGetterFunc getter)
            : isRaw(false), type(type), epsilon(epsilon), datumId(datumId)
            , statelessSetter(std::move(setter)), statelessGetter(std::move(getter)) {}
    };


    std::optional<ClientDataDefinitionId> id_{ std::nullopt };
    bool useMapping_{ true };
    std::vector<FieldInfo> fields_;
    size_t size_{ 0 };

public:

    /**
     * Check if the definition has been registered with SimConnect via define().
     */
    [[nodiscard]]
    bool isDefined() const noexcept { return id_.has_value(); }


    /**
     * Return the assigned Client Data Definition Id, or noId if not yet defined.
     */
    [[nodiscard]]
    ClientDataDefinitionId id() const noexcept { return id_.value_or(static_cast<ClientDataDefinitionId>(noId)); }


    /**
     * Implicit conversion to ClientDataDefinitionId.
     */
    operator ClientDataDefinitionId() const noexcept { return id(); }


    /**
     * Whether the incoming data buffer can be reinterpret_cast'd directly to StructType*.
     * True only when addRaw() is the sole registered field.
     */
    [[nodiscard]]
    bool useMapping() const noexcept { return useMapping_; }


    /**
     * Total wire size of the definition in bytes.
     */
    [[nodiscard]]
    size_t size() const noexcept { return size_; }


    /**
     * Register this definition with SimConnect, allocating a new ClientDataDefinitionId.
     *
     * Both the creating client and consumer clients call this identical method. The only
     * difference between creator and consumer is that the creator additionally calls
     * connection.createClientData() before define().
     *
     * Has no effect if the definition is already registered.
     */
    template <class connection_type>
    void define(connection_type& connection) {
        if (isDefined()) {
            return;
        }
        id_ = connection.clientDataDefinitions().nextDataDefID();

        unsigned long datumId{ 1 };
        for (auto& field : fields_) {
            field.datumId = datumId++;
            if (field.isRaw) {
                connection.addClientDataDefinition(id(), field.rawSize, clientDataAutoOffset, field.datumId);
            } else {
                connection.addClientDataDefinition(id(), field.type, clientDataAutoOffset, field.epsilon, field.datumId);
            }
        }
    }


    /**
     * Populate @p data from the next bytes in @p reader.
     *
     * For the raw case with useMapping() == true, the caller may prefer a direct
     * reinterpret_cast of the incoming data pointer over calling fill().
     */
    void fill(StructType& data, Data::DataBlockReader& reader) const {
        for (const auto& field : fields_) {
            if (field.setter) {
                field.setter(data, reader);
            } else if (field.statelessSetter) {
                field.statelessSetter(reader);
            }
        }
    }


    /**
     * Serialize @p data into @p builder for transmission via connection.sendClientData().
     */
    void build(Data::DataBlockBuilder& builder, const StructType& data) const {
        for (const auto& field : fields_) {
            if (field.getter) {
                field.getter(builder, data);
            } else if (field.statelessGetter) {
                field.statelessGetter(builder);
            }
        }
    }


#pragma region addRaw

    /**
     * Add the entire StructType as a single raw blob (sizeof(StructType) bytes).
     *
     * This is the most common registration mode. When this is the only field added,
     * useMapping() returns true and the incoming SimConnect data buffer may be cast
     * directly to const StructType*.
     *
     * If any other field is already registered, or if addRaw() is called again,
     * useMapping() becomes false.
     */
    ClientDataDefinition& addRaw() {
        if (!fields_.empty()) {
            useMapping_ = false;
        }
        fields_.emplace_back(
            sizeof(StructType),
            unused,
            SetterFunc([](StructType& data, Data::DataBlockReader& reader) {
                auto span = reader.readBytes(sizeof(StructType));
                std::memcpy(&data, span.data(), sizeof(StructType));
            }),
            GetterFunc([](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addBytes(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&data), sizeof(StructType)));
            })
        );
        size_ += sizeof(StructType);
        return *this;
    }

#pragma endregion // addRaw


#pragma region addInt8

    ClientDataDefinition& addInt8(std::function<void(int8_t)> setter, std::function<int8_t()> getter, float epsilon = 0.0f) {
        useMapping_ = false;
        fields_.emplace_back(ClientDataType::int8, epsilon, unused,
            StatelessSetterFunc([setter](Data::DataBlockReader& reader) { setter(reader.readInt8()); }),
            StatelessGetterFunc([getter](Data::DataBlockBuilder& builder) { builder.addInt8(getter()); }));
        size_ += sizeof(int8_t);
        return *this;
    }

    ClientDataDefinition& addInt8(std::function<void(StructType&, int8_t)> setter, std::function<int8_t(const StructType&)> getter, float epsilon = 0.0f) {
        useMapping_ = false;
        fields_.emplace_back(ClientDataType::int8, epsilon, unused,
            SetterFunc([setter](StructType& data, Data::DataBlockReader& reader) { setter(data, reader.readInt8()); }),
            GetterFunc([getter](Data::DataBlockBuilder& builder, const StructType& data) { builder.addInt8(getter(data)); }));
        size_ += sizeof(int8_t);
        return *this;
    }

    ClientDataDefinition& addInt8(int8_t StructType::* field, float epsilon = 0.0f) {
        useMapping_ = false;
        fields_.emplace_back(ClientDataType::int8, epsilon, unused,
            SetterFunc([field](StructType& data, Data::DataBlockReader& reader) { data.*field = reader.readInt8(); }),
            GetterFunc([field](Data::DataBlockBuilder& builder, const StructType& data) { builder.addInt8(data.*field); }));
        size_ += sizeof(int8_t);
        return *this;
    }

    ClientDataDefinition& addInt8(int16_t StructType::* field, float epsilon = 0.0f) {
        useMapping_ = false;
        fields_.emplace_back(ClientDataType::int8, epsilon, unused,
            SetterFunc([field](StructType& data, Data::DataBlockReader& reader) { data.*field = reader.readInt8(); }),
            GetterFunc([field](Data::DataBlockBuilder& builder, const StructType& data) { builder.addInt8(static_cast<int8_t>(data.*field)); }));
        size_ += sizeof(int8_t);
        return *this;
    }

    ClientDataDefinition& addInt8(int32_t StructType::* field, float epsilon = 0.0f) {
        useMapping_ = false;
        fields_.emplace_back(ClientDataType::int8, epsilon, unused,
            SetterFunc([field](StructType& data, Data::DataBlockReader& reader) { data.*field = reader.readInt8(); }),
            GetterFunc([field](Data::DataBlockBuilder& builder, const StructType& data) { builder.addInt8(static_cast<int8_t>(data.*field)); }));
        size_ += sizeof(int8_t);
        return *this;
    }

    ClientDataDefinition& addInt8(int64_t StructType::* field, float epsilon = 0.0f) {
        useMapping_ = false;
        fields_.emplace_back(ClientDataType::int8, epsilon, unused,
            SetterFunc([field](StructType& data, Data::DataBlockReader& reader) { data.*field = reader.readInt8(); }),
            GetterFunc([field](Data::DataBlockBuilder& builder, const StructType& data) { builder.addInt8(static_cast<int8_t>(data.*field)); }));
        size_ += sizeof(int8_t);
        return *this;
    }

    ClientDataDefinition& addInt8(float StructType::* field, float epsilon = 0.0f) {
        useMapping_ = false;
        fields_.emplace_back(ClientDataType::int8, epsilon, unused,
            SetterFunc([field](StructType& data, Data::DataBlockReader& reader) { data.*field = static_cast<float>(reader.readInt8()); }),
            GetterFunc([field](Data::DataBlockBuilder& builder, const StructType& data) { builder.addInt8(static_cast<int8_t>(data.*field)); }));
        size_ += sizeof(int8_t);
        return *this;
    }

    ClientDataDefinition& addInt8(double StructType::* field, float epsilon = 0.0f) {
        useMapping_ = false;
        fields_.emplace_back(ClientDataType::int8, epsilon, unused,
            SetterFunc([field](StructType& data, Data::DataBlockReader& reader) { data.*field = static_cast<double>(reader.readInt8()); }),
            GetterFunc([field](Data::DataBlockBuilder& builder, const StructType& data) { builder.addInt8(static_cast<int8_t>(data.*field)); }));
        size_ += sizeof(int8_t);
        return *this;
    }

    ClientDataDefinition& addInt8(bool StructType::* field, float epsilon = 0.0f) {
        useMapping_ = false;
        fields_.emplace_back(ClientDataType::int8, epsilon, unused,
            SetterFunc([field](StructType& data, Data::DataBlockReader& reader) { data.*field = reader.readInt8() != 0; }),
            GetterFunc([field](Data::DataBlockBuilder& builder, const StructType& data) { builder.addInt8(data.*field ? 1 : 0); }));
        size_ += sizeof(int8_t);
        return *this;
    }

#pragma endregion // addInt8


#pragma region addInt16

    ClientDataDefinition& addInt16(std::function<void(int16_t)> setter, std::function<int16_t()> getter, float epsilon = 0.0f) {
        useMapping_ = false;
        fields_.emplace_back(ClientDataType::int16, epsilon, unused,
            StatelessSetterFunc([setter](Data::DataBlockReader& reader) { setter(reader.readInt16()); }),
            StatelessGetterFunc([getter](Data::DataBlockBuilder& builder) { builder.addInt16(getter()); }));
        size_ += sizeof(int16_t);
        return *this;
    }

    ClientDataDefinition& addInt16(std::function<void(StructType&, int16_t)> setter, std::function<int16_t(const StructType&)> getter, float epsilon = 0.0f) {
        useMapping_ = false;
        fields_.emplace_back(ClientDataType::int16, epsilon, unused,
            SetterFunc([setter](StructType& data, Data::DataBlockReader& reader) { setter(data, reader.readInt16()); }),
            GetterFunc([getter](Data::DataBlockBuilder& builder, const StructType& data) { builder.addInt16(getter(data)); }));
        size_ += sizeof(int16_t);
        return *this;
    }

    ClientDataDefinition& addInt16(int8_t StructType::* field, float epsilon = 0.0f) {
        useMapping_ = false;
        fields_.emplace_back(ClientDataType::int16, epsilon, unused,
            SetterFunc([field](StructType& data, Data::DataBlockReader& reader) { data.*field = static_cast<int8_t>(reader.readInt16()); }),
            GetterFunc([field](Data::DataBlockBuilder& builder, const StructType& data) { builder.addInt16(data.*field); }));
        size_ += sizeof(int16_t);
        return *this;
    }

    ClientDataDefinition& addInt16(int16_t StructType::* field, float epsilon = 0.0f) {
        useMapping_ = false;
        fields_.emplace_back(ClientDataType::int16, epsilon, unused,
            SetterFunc([field](StructType& data, Data::DataBlockReader& reader) { data.*field = reader.readInt16(); }),
            GetterFunc([field](Data::DataBlockBuilder& builder, const StructType& data) { builder.addInt16(data.*field); }));
        size_ += sizeof(int16_t);
        return *this;
    }

    ClientDataDefinition& addInt16(int32_t StructType::* field, float epsilon = 0.0f) {
        useMapping_ = false;
        fields_.emplace_back(ClientDataType::int16, epsilon, unused,
            SetterFunc([field](StructType& data, Data::DataBlockReader& reader) { data.*field = reader.readInt16(); }),
            GetterFunc([field](Data::DataBlockBuilder& builder, const StructType& data) { builder.addInt16(static_cast<int16_t>(data.*field)); }));
        size_ += sizeof(int16_t);
        return *this;
    }

    ClientDataDefinition& addInt16(int64_t StructType::* field, float epsilon = 0.0f) {
        useMapping_ = false;
        fields_.emplace_back(ClientDataType::int16, epsilon, unused,
            SetterFunc([field](StructType& data, Data::DataBlockReader& reader) { data.*field = reader.readInt16(); }),
            GetterFunc([field](Data::DataBlockBuilder& builder, const StructType& data) { builder.addInt16(static_cast<int16_t>(data.*field)); }));
        size_ += sizeof(int16_t);
        return *this;
    }

    ClientDataDefinition& addInt16(float StructType::* field, float epsilon = 0.0f) {
        useMapping_ = false;
        fields_.emplace_back(ClientDataType::int16, epsilon, unused,
            SetterFunc([field](StructType& data, Data::DataBlockReader& reader) { data.*field = static_cast<float>(reader.readInt16()); }),
            GetterFunc([field](Data::DataBlockBuilder& builder, const StructType& data) { builder.addInt16(static_cast<int16_t>(data.*field)); }));
        size_ += sizeof(int16_t);
        return *this;
    }

    ClientDataDefinition& addInt16(double StructType::* field, float epsilon = 0.0f) {
        useMapping_ = false;
        fields_.emplace_back(ClientDataType::int16, epsilon, unused,
            SetterFunc([field](StructType& data, Data::DataBlockReader& reader) { data.*field = static_cast<double>(reader.readInt16()); }),
            GetterFunc([field](Data::DataBlockBuilder& builder, const StructType& data) { builder.addInt16(static_cast<int16_t>(data.*field)); }));
        size_ += sizeof(int16_t);
        return *this;
    }

    ClientDataDefinition& addInt16(bool StructType::* field, float epsilon = 0.0f) {
        useMapping_ = false;
        fields_.emplace_back(ClientDataType::int16, epsilon, unused,
            SetterFunc([field](StructType& data, Data::DataBlockReader& reader) { data.*field = reader.readInt16() != 0; }),
            GetterFunc([field](Data::DataBlockBuilder& builder, const StructType& data) { builder.addInt16(data.*field ? 1 : 0); }));
        size_ += sizeof(int16_t);
        return *this;
    }

#pragma endregion // addInt16


#pragma region addInt32

    ClientDataDefinition& addInt32(std::function<void(int32_t)> setter, std::function<int32_t()> getter, float epsilon = 0.0f) {
        useMapping_ = false;
        fields_.emplace_back(ClientDataType::int32, epsilon, unused,
            StatelessSetterFunc([setter](Data::DataBlockReader& reader) { setter(reader.readInt32()); }),
            StatelessGetterFunc([getter](Data::DataBlockBuilder& builder) { builder.addInt32(getter()); }));
        size_ += sizeof(int32_t);
        return *this;
    }

    ClientDataDefinition& addInt32(std::function<void(StructType&, int32_t)> setter, std::function<int32_t(const StructType&)> getter, float epsilon = 0.0f) {
        useMapping_ = false;
        fields_.emplace_back(ClientDataType::int32, epsilon, unused,
            SetterFunc([setter](StructType& data, Data::DataBlockReader& reader) { setter(data, reader.readInt32()); }),
            GetterFunc([getter](Data::DataBlockBuilder& builder, const StructType& data) { builder.addInt32(getter(data)); }));
        size_ += sizeof(int32_t);
        return *this;
    }

    ClientDataDefinition& addInt32(int32_t StructType::* field, float epsilon = 0.0f) {
        useMapping_ = false;
        fields_.emplace_back(ClientDataType::int32, epsilon, unused,
            SetterFunc([field](StructType& data, Data::DataBlockReader& reader) { data.*field = reader.readInt32(); }),
            GetterFunc([field](Data::DataBlockBuilder& builder, const StructType& data) { builder.addInt32(data.*field); }));
        size_ += sizeof(int32_t);
        return *this;
    }

    ClientDataDefinition& addInt32(int64_t StructType::* field, float epsilon = 0.0f) {
        useMapping_ = false;
        fields_.emplace_back(ClientDataType::int32, epsilon, unused,
            SetterFunc([field](StructType& data, Data::DataBlockReader& reader) { data.*field = reader.readInt32(); }),
            GetterFunc([field](Data::DataBlockBuilder& builder, const StructType& data) { builder.addInt32(static_cast<int32_t>(data.*field)); }));
        size_ += sizeof(int32_t);
        return *this;
    }

    ClientDataDefinition& addInt32(float StructType::* field, float epsilon = 0.0f) {
        useMapping_ = false;
        fields_.emplace_back(ClientDataType::int32, epsilon, unused,
            SetterFunc([field](StructType& data, Data::DataBlockReader& reader) { data.*field = static_cast<float>(reader.readInt32()); }),
            GetterFunc([field](Data::DataBlockBuilder& builder, const StructType& data) { builder.addInt32(static_cast<int32_t>(data.*field)); }));
        size_ += sizeof(int32_t);
        return *this;
    }

    ClientDataDefinition& addInt32(double StructType::* field, float epsilon = 0.0f) {
        useMapping_ = false;
        fields_.emplace_back(ClientDataType::int32, epsilon, unused,
            SetterFunc([field](StructType& data, Data::DataBlockReader& reader) { data.*field = static_cast<double>(reader.readInt32()); }),
            GetterFunc([field](Data::DataBlockBuilder& builder, const StructType& data) { builder.addInt32(static_cast<int32_t>(data.*field)); }));
        size_ += sizeof(int32_t);
        return *this;
    }

    ClientDataDefinition& addInt32(bool StructType::* field, float epsilon = 0.0f) {
        useMapping_ = false;
        fields_.emplace_back(ClientDataType::int32, epsilon, unused,
            SetterFunc([field](StructType& data, Data::DataBlockReader& reader) { data.*field = reader.readInt32() != 0; }),
            GetterFunc([field](Data::DataBlockBuilder& builder, const StructType& data) { builder.addInt32(data.*field ? 1 : 0); }));
        size_ += sizeof(int32_t);
        return *this;
    }

#pragma endregion // addInt32


#pragma region addInt64

    ClientDataDefinition& addInt64(std::function<void(int64_t)> setter, std::function<int64_t()> getter, float epsilon = 0.0f) {
        useMapping_ = false;
        fields_.emplace_back(ClientDataType::int64, epsilon, unused,
            StatelessSetterFunc([setter](Data::DataBlockReader& reader) { setter(reader.readInt64()); }),
            StatelessGetterFunc([getter](Data::DataBlockBuilder& builder) { builder.addInt64(getter()); }));
        size_ += sizeof(int64_t);
        return *this;
    }

    ClientDataDefinition& addInt64(std::function<void(StructType&, int64_t)> setter, std::function<int64_t(const StructType&)> getter, float epsilon = 0.0f) {
        useMapping_ = false;
        fields_.emplace_back(ClientDataType::int64, epsilon, unused,
            SetterFunc([setter](StructType& data, Data::DataBlockReader& reader) { setter(data, reader.readInt64()); }),
            GetterFunc([getter](Data::DataBlockBuilder& builder, const StructType& data) { builder.addInt64(getter(data)); }));
        size_ += sizeof(int64_t);
        return *this;
    }

    ClientDataDefinition& addInt64(int32_t StructType::* field, float epsilon = 0.0f) {
        useMapping_ = false;
        fields_.emplace_back(ClientDataType::int64, epsilon, unused,
            SetterFunc([field](StructType& data, Data::DataBlockReader& reader) { data.*field = static_cast<int32_t>(reader.readInt64()); }),
            GetterFunc([field](Data::DataBlockBuilder& builder, const StructType& data) { builder.addInt64(data.*field); }));
        size_ += sizeof(int64_t);
        return *this;
    }

    ClientDataDefinition& addInt64(int64_t StructType::* field, float epsilon = 0.0f) {
        useMapping_ = false;
        fields_.emplace_back(ClientDataType::int64, epsilon, unused,
            SetterFunc([field](StructType& data, Data::DataBlockReader& reader) { data.*field = reader.readInt64(); }),
            GetterFunc([field](Data::DataBlockBuilder& builder, const StructType& data) { builder.addInt64(data.*field); }));
        size_ += sizeof(int64_t);
        return *this;
    }

    ClientDataDefinition& addInt64(float StructType::* field, float epsilon = 0.0f) {
        useMapping_ = false;
        fields_.emplace_back(ClientDataType::int64, epsilon, unused,
            SetterFunc([field](StructType& data, Data::DataBlockReader& reader) { data.*field = static_cast<float>(reader.readInt64()); }),
            GetterFunc([field](Data::DataBlockBuilder& builder, const StructType& data) { builder.addInt64(static_cast<int64_t>(data.*field)); }));
        size_ += sizeof(int64_t);
        return *this;
    }

    ClientDataDefinition& addInt64(double StructType::* field, float epsilon = 0.0f) {
        useMapping_ = false;
        fields_.emplace_back(ClientDataType::int64, epsilon, unused,
            SetterFunc([field](StructType& data, Data::DataBlockReader& reader) { data.*field = static_cast<double>(reader.readInt64()); }),
            GetterFunc([field](Data::DataBlockBuilder& builder, const StructType& data) { builder.addInt64(static_cast<int64_t>(data.*field)); }));
        size_ += sizeof(int64_t);
        return *this;
    }

    ClientDataDefinition& addInt64(bool StructType::* field, float epsilon = 0.0f) {
        useMapping_ = false;
        fields_.emplace_back(ClientDataType::int64, epsilon, unused,
            SetterFunc([field](StructType& data, Data::DataBlockReader& reader) { data.*field = reader.readInt64() != 0; }),
            GetterFunc([field](Data::DataBlockBuilder& builder, const StructType& data) { builder.addInt64(data.*field ? 1 : 0); }));
        size_ += sizeof(int64_t);
        return *this;
    }

#pragma endregion // addInt64


#pragma region addFloat32

    ClientDataDefinition& addFloat32(std::function<void(float)> setter, std::function<float()> getter, float epsilon = 0.0f) {
        useMapping_ = false;
        fields_.emplace_back(ClientDataType::float32, epsilon, unused,
            StatelessSetterFunc([setter](Data::DataBlockReader& reader) { setter(reader.readFloat32()); }),
            StatelessGetterFunc([getter](Data::DataBlockBuilder& builder) { builder.addFloat32(getter()); }));
        size_ += sizeof(float);
        return *this;
    }

    ClientDataDefinition& addFloat32(std::function<void(StructType&, float)> setter, std::function<float(const StructType&)> getter, float epsilon = 0.0f) {
        useMapping_ = false;
        fields_.emplace_back(ClientDataType::float32, epsilon, unused,
            SetterFunc([setter](StructType& data, Data::DataBlockReader& reader) { setter(data, reader.readFloat32()); }),
            GetterFunc([getter](Data::DataBlockBuilder& builder, const StructType& data) { builder.addFloat32(getter(data)); }));
        size_ += sizeof(float);
        return *this;
    }

    ClientDataDefinition& addFloat32(int32_t StructType::* field, float epsilon = 0.0f) {
        useMapping_ = false;
        fields_.emplace_back(ClientDataType::float32, epsilon, unused,
            SetterFunc([field](StructType& data, Data::DataBlockReader& reader) { data.*field = static_cast<int32_t>(reader.readFloat32()); }),
            GetterFunc([field](Data::DataBlockBuilder& builder, const StructType& data) { builder.addFloat32(static_cast<float>(data.*field)); }));
        size_ += sizeof(float);
        return *this;
    }

    ClientDataDefinition& addFloat32(int64_t StructType::* field, float epsilon = 0.0f) {
        useMapping_ = false;
        fields_.emplace_back(ClientDataType::float32, epsilon, unused,
            SetterFunc([field](StructType& data, Data::DataBlockReader& reader) { data.*field = static_cast<int64_t>(reader.readFloat32()); }),
            GetterFunc([field](Data::DataBlockBuilder& builder, const StructType& data) { builder.addFloat32(static_cast<float>(data.*field)); }));
        size_ += sizeof(float);
        return *this;
    }

    ClientDataDefinition& addFloat32(float StructType::* field, float epsilon = 0.0f) {
        useMapping_ = false;
        fields_.emplace_back(ClientDataType::float32, epsilon, unused,
            SetterFunc([field](StructType& data, Data::DataBlockReader& reader) { data.*field = reader.readFloat32(); }),
            GetterFunc([field](Data::DataBlockBuilder& builder, const StructType& data) { builder.addFloat32(data.*field); }));
        size_ += sizeof(float);
        return *this;
    }

    ClientDataDefinition& addFloat32(double StructType::* field, float epsilon = 0.0f) {
        useMapping_ = false;
        fields_.emplace_back(ClientDataType::float32, epsilon, unused,
            SetterFunc([field](StructType& data, Data::DataBlockReader& reader) { data.*field = reader.readFloat32(); }),
            GetterFunc([field](Data::DataBlockBuilder& builder, const StructType& data) { builder.addFloat32(static_cast<float>(data.*field)); }));
        size_ += sizeof(float);
        return *this;
    }

#pragma endregion // addFloat32


#pragma region addFloat64

    ClientDataDefinition& addFloat64(std::function<void(double)> setter, std::function<double()> getter, float epsilon = 0.0f) {
        useMapping_ = false;
        fields_.emplace_back(ClientDataType::float64, epsilon, unused,
            StatelessSetterFunc([setter](Data::DataBlockReader& reader) { setter(reader.readFloat64()); }),
            StatelessGetterFunc([getter](Data::DataBlockBuilder& builder) { builder.addFloat64(getter()); }));
        size_ += sizeof(double);
        return *this;
    }

    ClientDataDefinition& addFloat64(std::function<void(StructType&, double)> setter, std::function<double(const StructType&)> getter, float epsilon = 0.0f) {
        useMapping_ = false;
        fields_.emplace_back(ClientDataType::float64, epsilon, unused,
            SetterFunc([setter](StructType& data, Data::DataBlockReader& reader) { setter(data, reader.readFloat64()); }),
            GetterFunc([getter](Data::DataBlockBuilder& builder, const StructType& data) { builder.addFloat64(getter(data)); }));
        size_ += sizeof(double);
        return *this;
    }

    ClientDataDefinition& addFloat64(int32_t StructType::* field, float epsilon = 0.0f) {
        useMapping_ = false;
        fields_.emplace_back(ClientDataType::float64, epsilon, unused,
            SetterFunc([field](StructType& data, Data::DataBlockReader& reader) { data.*field = static_cast<int32_t>(reader.readFloat64()); }),
            GetterFunc([field](Data::DataBlockBuilder& builder, const StructType& data) { builder.addFloat64(data.*field); }));
        size_ += sizeof(double);
        return *this;
    }

    ClientDataDefinition& addFloat64(int64_t StructType::* field, float epsilon = 0.0f) {
        useMapping_ = false;
        fields_.emplace_back(ClientDataType::float64, epsilon, unused,
            SetterFunc([field](StructType& data, Data::DataBlockReader& reader) { data.*field = static_cast<int64_t>(reader.readFloat64()); }),
            GetterFunc([field](Data::DataBlockBuilder& builder, const StructType& data) { builder.addFloat64(static_cast<double>(data.*field)); }));
        size_ += sizeof(double);
        return *this;
    }

    ClientDataDefinition& addFloat64(float StructType::* field, float epsilon = 0.0f) {
        useMapping_ = false;
        fields_.emplace_back(ClientDataType::float64, epsilon, unused,
            SetterFunc([field](StructType& data, Data::DataBlockReader& reader) { data.*field = reader.readFloat64(); }),
            GetterFunc([field](Data::DataBlockBuilder& builder, const StructType& data) { builder.addFloat64(data.*field); }));
        size_ += sizeof(double);
        return *this;
    }

    ClientDataDefinition& addFloat64(double StructType::* field, float epsilon = 0.0f) {
        useMapping_ = false;
        fields_.emplace_back(ClientDataType::float64, epsilon, unused,
            SetterFunc([field](StructType& data, Data::DataBlockReader& reader) { data.*field = reader.readFloat64(); }),
            GetterFunc([field](Data::DataBlockBuilder& builder, const StructType& data) { builder.addFloat64(data.*field); }));
        size_ += sizeof(double);
        return *this;
    }

#pragma endregion // addFloat64

};

} // namespace SimConnect
