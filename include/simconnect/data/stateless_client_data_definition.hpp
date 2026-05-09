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
#include <cstddef>
#include <functional>
#include <vector>

#include <simconnect/simconnect.hpp>
#include <simconnect/data/client_data_definition_base.hpp>
#include <simconnect/data/data_block_reader.hpp>
#include <simconnect/data/data_block_builder.hpp>


namespace SimConnect {


/**
 * Client data definition where each datum carries its own typed callbacks.
 * No aggregate struct is involved — each field handles itself.
 *
 * Use this when there is no meaningful C++ struct to map onto the client data area:
 * when you only care about a subset of datums, when each datum feeds a different
 * subsystem, or when building a definition from runtime configuration.
 *
 * Fields are registered with a receive setter and an optional send getter.
 * Fields without a getter are receive-only; `marshal()` silently skips them.
 *
 * `dispatch()` accepts an optional "batch done" callable (default no-op) that fires
 * after all per-datum setters have run. This satisfies `ClientDataDefinitionConcept`
 * and lets callers signal completion if needed.
 *
 * Usage (receive-only):
 * @code
 *   StatelessClientDataDefinition def;
 *   def.addInt32([](int32_t altitude) { log("alt={}", altitude); })
 *      .addFloat32([](float speed) { log("spd={}", speed); });
 *   auto req = dataHandler.requestClientData(dataId, def);
 * @endcode
 *
 * Usage (bidirectional):
 * @code
 *   StatelessClientDataDefinition def;
 *   def.addInt32(
 *       [&state](int32_t v) { state.altitude = v; },   // setter
 *       [&state]() -> int32_t { return state.altitude; } // getter
 *   ).addFloat64(
 *       [&state](double v) { state.speed = v; },
 *       [&state]() -> double { return state.speed; }
 *   );
 *   dataHandler.sendClientData(dataId, def);
 *   auto req = dataHandler.requestClientData(dataId, def);
 * @endcode
 */
class StatelessClientDataDefinition
    : public ClientDataDefinitionBase<StatelessClientDataDefinition>
{
    using Setter = std::function<void(Data::DataBlockReader&)>;
    using Getter = std::function<void(Data::DataBlockBuilder&)>;

    struct FieldInfo {
        ClientDataType type{};
        float epsilon{ 0.0f };
        unsigned long datumId{ unused };
        Setter setter;
        Getter getter;            // empty → receive-only field
        size_t rawByteSize{ 0 }; // >0 → raw bytes field; 0 → typed field
    };

    std::vector<FieldInfo> fields_;


public:
    using callback_type = std::function<void()>;


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
     * Deliver a received client data message to per-datum setters, then call `done`.
     *
     * @param msg   The received client data message.
     * @param done  Called after all per-datum setters complete. May be a no-op.
     */
    template <typename HandlerFn>
    void dispatch(const Messages::ClientDataMsg& msg, HandlerFn&& done) {
        const bool isTagged = (msg.dwFlags & DataRequestFlags::tagged) != 0;

        Data::DataBlockReader reader(static_cast<const Messages::SimObjectDataMsg&>(msg));

        unmarshall(reader, isTagged ? msg.dwDefineCount : taggingNotUsed);

        done();
    }


    /**
     * Serialize all bidirectional fields into a DataBlockBuilder for sending.
     * Fields registered without a getter are silently skipped.
     *
     * @param builder  Target buffer.
     * @param isTagged If true, each field is preceded by its datum ID.
     */
    void marshal(Data::DataBlockBuilder& builder, bool isTagged = false) const {
        for (const auto& field : fields_) {
            if (!field.getter) { continue; }
            if (isTagged) {
                builder.addInt32(static_cast<int32_t>(field.datumId));
            }
            field.getter(builder);
        }
    }


    /**
     * Invoke setters for the given data.
     * 
     * @param reader    The DataBlockReader to read from.
     * @param numElems  The number of tagged entries to read, or taggingNotUsed to read all fields in order.
     */
    void unmarshall(Data::DataBlockReader& reader, unsigned long numElems = taggingNotUsed) {
        if (numElems == taggingNotUsed) {
            for (const auto& field : fields_) {
                field.setter(reader);
            }
        } else {
            while (numElems-- > 0) {
                const auto id = static_cast<size_t>(reader.readInt32());
                if (id == 0 || id > fields_.size()) {
                    continue;
                }
                fields_[id - 1].setter(reader);
            }
        }
    }


#pragma region Field registration

    // ----- int8 -----

    StatelessClientDataDefinition& addInt8(std::function<void(int8_t)> set, float epsilon = 0.0f) {
        fields_.emplace_back(ClientDataType::int8, epsilon, unused,
            [set = std::move(set)](Data::DataBlockReader& r) { set(r.readInt8()); },
            Getter{}, 0u);
        this->size_ += sizeof(int8_t);
        return *this;
    }

    StatelessClientDataDefinition& addInt8(std::function<void(int8_t)> set, std::function<int8_t()> get, float epsilon = 0.0f) {
        fields_.emplace_back(ClientDataType::int8, epsilon, unused,
            [set = std::move(set)](Data::DataBlockReader& r) { set(r.readInt8()); },
            [get = std::move(get)](Data::DataBlockBuilder& b) { b.addInt8(get()); },
            0u);
        this->size_ += sizeof(int8_t);
        return *this;
    }

    // ----- int16 -----

    StatelessClientDataDefinition& addInt16(std::function<void(int16_t)> set, float epsilon = 0.0f) {
        fields_.emplace_back(ClientDataType::int16, epsilon, unused,
            [set = std::move(set)](Data::DataBlockReader& r) { set(r.readInt16()); },
            Getter{}, 0u);
        this->size_ += sizeof(int16_t);
        return *this;
    }

    StatelessClientDataDefinition& addInt16(std::function<void(int16_t)> set, std::function<int16_t()> get, float epsilon = 0.0f) {
        fields_.emplace_back(ClientDataType::int16, epsilon, unused,
            [set = std::move(set)](Data::DataBlockReader& r) { set(r.readInt16()); },
            [get = std::move(get)](Data::DataBlockBuilder& b) { b.addInt16(get()); },
            0u);
        this->size_ += sizeof(int16_t);
        return *this;
    }

    // ----- int32 -----

    StatelessClientDataDefinition& addInt32(std::function<void(int32_t)> set, float epsilon = 0.0f) {
        fields_.emplace_back(ClientDataType::int32, epsilon, unused,
            [set = std::move(set)](Data::DataBlockReader& r) { set(r.readInt32()); },
            Getter{}, 0u);
        this->size_ += sizeof(int32_t);
        return *this;
    }

    StatelessClientDataDefinition& addInt32(std::function<void(int32_t)> set, std::function<int32_t()> get, float epsilon = 0.0f) {
        fields_.emplace_back(ClientDataType::int32, epsilon, unused,
            [set = std::move(set)](Data::DataBlockReader& r) { set(r.readInt32()); },
            [get = std::move(get)](Data::DataBlockBuilder& b) { b.addInt32(get()); },
            0u);
        this->size_ += sizeof(int32_t);
        return *this;
    }

    // ----- int64 -----

    StatelessClientDataDefinition& addInt64(std::function<void(int64_t)> set, float epsilon = 0.0f) {
        fields_.emplace_back(ClientDataType::int64, epsilon, unused,
            [set = std::move(set)](Data::DataBlockReader& r) { set(r.readInt64()); },
            Getter{}, 0u);
        this->size_ += sizeof(int64_t);
        return *this;
    }

    StatelessClientDataDefinition& addInt64(std::function<void(int64_t)> set, std::function<int64_t()> get, float epsilon = 0.0f) {
        fields_.emplace_back(ClientDataType::int64, epsilon, unused,
            [set = std::move(set)](Data::DataBlockReader& r) { set(r.readInt64()); },
            [get = std::move(get)](Data::DataBlockBuilder& b) { b.addInt64(get()); },
            0u);
        this->size_ += sizeof(int64_t);
        return *this;
    }

    // ----- float32 -----

    StatelessClientDataDefinition& addFloat32(std::function<void(float)> set, float epsilon = 0.0f) {
        fields_.emplace_back(ClientDataType::float32, epsilon, unused,
            [set = std::move(set)](Data::DataBlockReader& r) { set(r.readFloat32()); },
            Getter{}, 0u);
        this->size_ += sizeof(float);
        return *this;
    }

    StatelessClientDataDefinition& addFloat32(std::function<void(float)> set, std::function<float()> get, float epsilon = 0.0f) {
        fields_.emplace_back(ClientDataType::float32, epsilon, unused,
            [set = std::move(set)](Data::DataBlockReader& r) { set(r.readFloat32()); },
            [get = std::move(get)](Data::DataBlockBuilder& b) { b.addFloat32(get()); },
            0u);
        this->size_ += sizeof(float);
        return *this;
    }

    // ----- float64 -----

    StatelessClientDataDefinition& addFloat64(std::function<void(double)> set, float epsilon = 0.0f) {
        fields_.emplace_back(ClientDataType::float64, epsilon, unused,
            [set = std::move(set)](Data::DataBlockReader& r) { set(r.readFloat64()); },
            Getter{}, 0u);
        this->size_ += sizeof(double);
        return *this;
    }

    StatelessClientDataDefinition& addFloat64(std::function<void(double)> set, std::function<double()> get, float epsilon = 0.0f) {
        fields_.emplace_back(ClientDataType::float64, epsilon, unused,
            [set = std::move(set)](Data::DataBlockReader& r) { set(r.readFloat64()); },
            [get = std::move(get)](Data::DataBlockBuilder& b) { b.addFloat64(get()); },
            0u);
        this->size_ += sizeof(double);
        return *this;
    }

    // ----- raw bytes -----

    /**
     * Register a receive-only raw-bytes field of sizeof(T) bytes.
     * T must be trivially copyable.
     */
    template <typename T>
    StatelessClientDataDefinition& addRaw(std::function<void(const T&)> set) {
        static_assert(std::is_trivially_copyable_v<T>, "addRaw requires a trivially copyable type");
        fields_.emplace_back(ClientDataType{}, 0.0f, unused,
            [set = std::move(set)](Data::DataBlockReader& r) {
                auto bytes = r.readBytes(sizeof(T));
                T value{};
                std::memcpy(&value, bytes.data(), sizeof(T));
                set(value);
            },
            Getter{}, sizeof(T));
        this->size_ += sizeof(T);
        return *this;
    }

    /**
     * Register a bidirectional raw-bytes field of sizeof(T) bytes.
     * The getter writes sizeof(T) bytes into the builder directly.
     * T must be trivially copyable.
     */
    template <typename T>
    StatelessClientDataDefinition& addRaw(std::function<void(const T&)> set, std::function<void(Data::DataBlockBuilder&)> get) {
        static_assert(std::is_trivially_copyable_v<T>, "addRaw requires a trivially copyable type");
        fields_.emplace_back(ClientDataType{}, 0.0f, unused,
            [set = std::move(set)](Data::DataBlockReader& r) {
                auto bytes = r.readBytes(sizeof(T));
                T value{};
                std::memcpy(&value, bytes.data(), sizeof(T));
                set(value);
            },
            std::move(get), sizeof(T));
        this->size_ += sizeof(T);
        return *this;
    }

#pragma endregion // Field registration

};


} // namespace SimConnect
