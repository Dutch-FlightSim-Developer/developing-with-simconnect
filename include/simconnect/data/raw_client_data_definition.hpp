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
#include <functional>

#include <simconnect/simconnect.hpp>
#include <simconnect/data/client_data_definition_base.hpp>


namespace SimConnect {


/**
 * Client data definition for a struct that maps directly onto the wire layout.
 *
 * The entire StructType is registered as a single raw datum (sizeof(StructType)
 * bytes). Incoming data is delivered via a single reinterpret_cast — no
 * field-by-field marshalling, no temporary copy.
 *
 * This is the correct choice whenever the C++ type already matches the SimConnect
 * wire layout: PMDG SDK structs, your own tightly packed structs, etc.
 *
 * Usage (receive):
 * @code
 *   RawClientDataDefinition<PMDG_NG3_Data> def;
 *   auto req = dataHandler.requestClientData(dataId, def,
 *       [](const PMDG_NG3_Data& data) { ... });
 * @endcode
 *
 * Usage (send / owner):
 * @code
 *   RawClientDataDefinition<MyStruct> def;
 *   dataHandler.createClientData(dataId, def.size());
 *   def.define(connection);
 *   dataHandler.sendClientData(dataId, def, myStruct);
 * @endcode
 *
 * @tparam StructType  The C++ struct that maps exactly onto the client data area.
 */
template <typename StructType>
class RawClientDataDefinition
    : public ClientDataDefinitionBase<RawClientDataDefinition<StructType>>
{
public:
    using struct_type = StructType;
    using callback_type = std::function<void(const StructType&)>;

    RawClientDataDefinition() noexcept {
        this->size_ = sizeof(StructType);
    }

    template <class connection_type>
    void registerFields(connection_type& connection) {
        connection.addClientDataDefinition(this->id(), sizeof(StructType), clientDataAutoOffset);
    }

    void dispatch(const Messages::ClientDataMsg& msg, const callback_type& cb) const {
        cb(*reinterpret_cast<const StructType*>(&msg.dwData));
    }
};


} // namespace SimConnect
