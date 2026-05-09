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


namespace SimConnect {


/**
 * Client data definition where each datum carries its own typed callback.
 * No aggregate struct is involved — each field handles itself.
 *
 * Use this when there is no meaningful C++ struct to map onto the client data area:
 * when you only care about a subset of datums, when each datum feeds a different
 * subsystem, or when building a definition from runtime configuration.
 *
 * Each field is registered with a callback that receives the decoded value directly.
 * No temporary struct is created; no reinterpret_cast shortcut is attempted.
 *
 * `dispatch()` accepts an optional "batch done" callable (default no-op) that fires
 * after all per-datum callbacks have run. This satisfies `ClientDataDefinitionConcept`
 * and lets callers signal completion if needed.
 *
 * Usage:
 * @code
 *   StatelessClientDataDefinition def;
 *   def.addInt32([](int32_t altitude) { log("alt={}", altitude); })
 *      .addFloat32([](float speed) { log("spd={}", speed); });
 *
 *   // No outer handler needed — use the convenience overload:
 *   auto req = dataHandler.requestClientData(dataId, def);
 *
 *   // Or supply a "done" callback:
 *   auto req = dataHandler.requestClientData(dataId, def, []() { refresh_ui(); });
 * @endcode
 */
class StatelessClientDataDefinition
    : public ClientDataDefinitionBase<StatelessClientDataDefinition>
{
    struct FieldInfo {
        ClientDataType type{};
        float epsilon{ 0.0f };
        unsigned long datumId{ unused };
        std::function<void(Data::DataBlockReader&)> callback;
        size_t rawByteSize{ 0 };  // >0 → raw bytes field; 0 → typed field
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
     * Deliver a received client data message to per-datum callbacks, then call `done`.
     *
     * - Untagged: callbacks fire in registration order, each advancing the reader.
     * - Tagged: datum ID is parsed per entry; only the matching callback fires.
     *
     * @param msg   The received client data message.
     * @param done  Called after all per-datum callbacks complete. May be a no-op.
     */
    template <typename HandlerFn>
    void dispatch(const Messages::ClientDataMsg& msg, HandlerFn&& done) {
        const bool isTagged = (msg.dwFlags & DataRequestFlags::tagged) != 0;

        Data::DataBlockReader reader(static_cast<const Messages::SimObjectDataMsg&>(msg));

        if (isTagged) {
            auto numElems = msg.dwDefineCount;
            while (numElems-- > 0) {
                const auto id = static_cast<size_t>(reader.readInt32());
                if (id == 0 || id > fields_.size()) {
                    continue;
                }
                fields_[id - 1].callback(reader);
            }
        } else {
            for (const auto& field : fields_) {
                field.callback(reader);
            }
        }

        std::invoke(std::forward<HandlerFn>(done));
    }


#pragma region Field registration

    StatelessClientDataDefinition& addInt8(std::function<void(int8_t)> cb, float epsilon = 0.0f) {
        fields_.emplace_back(ClientDataType::int8, epsilon, unused,
            [cb = std::move(cb)](Data::DataBlockReader& r) { cb(r.readInt8()); });
        this->size_ += sizeof(int8_t);
        return *this;
    }

    StatelessClientDataDefinition& addInt16(std::function<void(int16_t)> cb, float epsilon = 0.0f) {
        fields_.emplace_back(ClientDataType::int16, epsilon, unused,
            [cb = std::move(cb)](Data::DataBlockReader& r) { cb(r.readInt16()); });
        this->size_ += sizeof(int16_t);
        return *this;
    }

    StatelessClientDataDefinition& addInt32(std::function<void(int32_t)> cb, float epsilon = 0.0f) {
        fields_.emplace_back(ClientDataType::int32, epsilon, unused,
            [cb = std::move(cb)](Data::DataBlockReader& r) { cb(r.readInt32()); });
        this->size_ += sizeof(int32_t);
        return *this;
    }

    StatelessClientDataDefinition& addInt64(std::function<void(int64_t)> cb, float epsilon = 0.0f) {
        fields_.emplace_back(ClientDataType::int64, epsilon, unused,
            [cb = std::move(cb)](Data::DataBlockReader& r) { cb(r.readInt64()); });
        this->size_ += sizeof(int64_t);
        return *this;
    }

    StatelessClientDataDefinition& addFloat32(std::function<void(float)> cb, float epsilon = 0.0f) {
        fields_.emplace_back(ClientDataType::float32, epsilon, unused,
            [cb = std::move(cb)](Data::DataBlockReader& r) { cb(r.readFloat32()); });
        this->size_ += sizeof(float);
        return *this;
    }

    StatelessClientDataDefinition& addFloat64(std::function<void(double)> cb, float epsilon = 0.0f) {
        fields_.emplace_back(ClientDataType::float64, epsilon, unused,
            [cb = std::move(cb)](Data::DataBlockReader& r) { cb(r.readFloat64()); });
        this->size_ += sizeof(double);
        return *this;
    }

    /**
     * Register a raw-bytes field of sizeof(T) bytes.
     *
     * The callback receives a const reference to a local copy produced by memcpy.
     * T must be trivially copyable.
     *
     * @tparam T   Type of the value. Must be trivially copyable.
     * @param cb   Called with the decoded value on every receive.
     */
    template <typename T>
    StatelessClientDataDefinition& addRaw(std::function<void(const T&)> cb) {
        static_assert(std::is_trivially_copyable_v<T>, "addRaw requires a trivially copyable type");
        fields_.emplace_back(ClientDataType{}, 0.0f, unused,
            [cb = std::move(cb)](Data::DataBlockReader& r) {
                auto bytes = r.readBytes(sizeof(T));
                T value{};
                std::memcpy(&value, bytes.data(), sizeof(T));
                cb(value);
            }
        );
        fields_.back().rawByteSize = sizeof(T);
        this->size_ += sizeof(T);
        return *this;
    }

#pragma endregion // Field registration

};


} // namespace SimConnect
