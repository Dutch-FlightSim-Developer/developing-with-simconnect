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

#include <any>
#include <functional>
 #include <atomic>


#include <simconnect/connection.hpp>


namespace SimConnect {

/**
 * A data definition block.
 */
template <typename StructType>
class DataDefinition
{
    struct FieldInfo {
        std::string simVar{ "" };
        std::string units{ "" };
        SIMCONNECT_DATATYPE dataType{ SIMCONNECT_DATATYPE_INVALID };
        float epsilon{ 0.0f };
        unsigned long datumId{ SIMCONNECT_UNUSED };
        std::function<void(StructType&, const std::any&)> setter;
        std::function<std::any(const StructType&)> getter;

        FieldInfo(std::string simVar, std::string units, SIMCONNECT_DATATYPE dataType, float epsilon, unsigned long datumId,
                  std::function<void(StructType&, const std::any&)> setter, std::function<std::any(const StructType&)> getter)
            : simVar(simVar), units(units), dataType(dataType), epsilon(epsilon), datumId(datumId), setter(setter), getter(getter) {}
    };

    Connection& connection_;    ///< The connection to SimConnect.
    int id_{ -1 };              ///< The ID of the data definition.
    std::vector<FieldInfo> fields_;

    static std::atomic_int nextDefId_;

public:

    DataDefinition(Connection& connection) : connection_(connection) {}

    bool isDefined() const noexcept { return id_ != -1; }


    [[nodiscard]]
    int id() const noexcept { return id_; }

    operator SIMCONNECT_DATA_DEFINITION_ID() const { return static_cast<SIMCONNECT_DATA_DEFINITION_ID>(id_); }

    template <typename FieldType>
    DataDefinition& add(FieldType StructType::* field, std::string simVar, std::string units = "") {
        fields_.push_back(FieldInfo( simVar, units, SIMCONNECT_DATATYPE_INT32, 0.0f, SIMCONNECT_UNUSED,
                            [field](StructType& data, const std::any& value) { data.*field = std::any_cast<FieldType>(value); },
                            [field](const StructType& data) { return std::any(data.*field); } ));
        return *this;
    }

    template <typename FieldType>
    DataDefinition& add(FieldType StructType::* field, SIMCONNECT_DATATYPE dataType, std::string simVar, std::string units = "") {
        fields_.push_back(FieldInfo( simVar, units, dataType, 0.0f, SIMCONNECT_UNUSED,
                            [field](StructType& data, const std::any& value) { data.*field = std::any_cast<FieldType>(value); },
                            [field](const StructType& data) { return std::any(data.*field); } ));
        return *this;
    }

    void extract(const SIMCONNECT_RECV_SIMOBJECT_DATA& msg, StructType& data) const {
        auto pData = reinterpret_cast<const StructType*>(&msg.dwData);
        for (auto& field : fields_) {
            switch (field.dataType) {
            case SIMCONNECT_DATATYPE_INT32:
                field.setter(data, *reinterpret_cast<const int32_t*>(pData));
                pData += sizeof(int32_t);
                break;
            case SIMCONNECT_DATATYPE_INT64:
                field.setter(data, *reinterpret_cast<const int64_t*>(pData));
                pData += sizeof(int64_t);
                break;
            case SIMCONNECT_DATATYPE_FLOAT32:
                field.setter(data, *reinterpret_cast<const float*>(pData));
                pData += sizeof(float);
                break;
            case SIMCONNECT_DATATYPE_FLOAT64:
                field.setter(data, *reinterpret_cast<const double*>(pData));
                pData += sizeof(double);
                break;
            case SIMCONNECT_DATATYPE_STRINGV:
                field.setter(data, std::string(reinterpret_cast<const char*>(pData)));
                pData += std::strlen(reinterpret_cast<const char*>(pData)) + 1;
                break;
            default:
                throw SimConnectException("Unknown data type.");
            }
        }
    }
};


} // namespace SimConnect