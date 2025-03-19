#pragma once
/*
 * Copyright (c) 2024. Bert Laverman
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


 #include <atomic>


#include <simconnect/connection.hpp>


namespace SimConnect {

/**
 * A data definition block.
 */
template <typename StructType>
class DataDefinition
{
    template <typename FieldInfoClass>
    struct FieldInfo {
        std::string simVar{ "" };
        std::string units{ "" };
        SIMCONNECT_DATATYPE dataType{ SIMCONNECT_DATATYPE_INVALID };
        float epsilon{ 0.0f };
        unsigned long datumId{ SIMCONNECT_UNUSED };

        void addToDataDefinition(Connection& connection, DataDefinition& dataDef) {
            static_cast<FieldInfoClass*>(this)->addToDataDefinition(connection, dataDef);
        }
    };

    Connection& connection_;    ///< The connection to SimConnect.
    int id_{ -1 };              ///< The ID of the data definition.
    std::vector<std::tuple<StructType StructType::*, std::string, std::string>> fields_;

    static std::atomic_int nextDefId_;

public:

    DataDefinition(Connection& connection) : connection_(connection) {}

    bool isDefined() const noexcept { return id_ != -1; }


    [[nodiscard]]
    int id() const noexcept { return id_; }

    operator SIMCONNECT_DATA_DEFINITION_ID() const { return static_cast<SIMCONNECT_DATA_DEFINITION_ID>(id_); }

    template <typename FieldType>
    DataDefinition& add(FieldType StructType::* field, std::string simVar, std::string units = "") {
        fields_.push_back({ field, simVar, units });
        return *this;
    }
};


} // namespace SimConnect