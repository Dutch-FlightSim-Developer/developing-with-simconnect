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

#include <cstdint>
 
#include <span>
#include <atomic>
#include <string>
#include <vector>
#include <functional>

#include <simconnect/connection.hpp>
#include <simconnect/data/data_block_builder.hpp>
#include <simconnect/data/data_block_reader.hpp>


namespace SimConnect {

/**
 * A data definition block.
 */
template <typename StructType>
class DataDefinition
{
    using SetterFunc = std::function<void(StructType&, Data::DataBlockReader&)>;
    using GetterFunc = std::function<void(Data::DataBlockBuilder&, const StructType&)>;


    struct FieldInfo {
        std::string simVar{ "" };
        std::string units{ "" };
        SIMCONNECT_DATATYPE dataType{ SIMCONNECT_DATATYPE_INVALID };
        float epsilon{ 0.0f };
        unsigned long datumId{ SIMCONNECT_UNUSED };
        SetterFunc setter;
        GetterFunc getter;

        FieldInfo(std::string simVar, std::string units, SIMCONNECT_DATATYPE dataType, float epsilon, unsigned long datumId,
                  SetterFunc setter, GetterFunc getter)
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
    DataDefinition& add(FieldType StructType::* field, SIMCONNECT_DATATYPE dataType, std::string simVar, std::string units = "") {
        GetterFunc getter;
        SetterFunc setter;

        switch (dataType) {

        case SIMCONNECT_DATATYPE_INT32:
            if constexpr (std::is_same_v<FieldType, int> || std::is_same_v<FieldType, long> ||
                          std::is_same_v<FieldType, int32_t> ||
                          std::is_same_v<FieldType, long long> ||
                          std::is_same_v<FieldType, uint64_t> ||
                          std::is_same_v<FieldType, double>) {
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = reader.readInt32();
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addInt32(static_cast<int32_t>(data.*field));
                };
            } else if constexpr (std::is_same_v<FieldType, float>) {
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = static_cast<FieldType>(reader.readInt32());
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addInt32(static_cast<int32_t>(data.*field));
                };
            } else if constexpr (std::is_same_v<FieldType, bool>) {
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = reader.readInt32() != 0;
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addInt32(data.*field ? 1 : 0);
                };
            } else if constexpr (std::is_same_v<FieldType, std::string>) {
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = std::to_string(reader.readInt32());
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addInt32(std::stoi((data.*field).c_str()));
                };
            } else {
                throw SimConnectException("Invalid field type for SIMCONNECT_DATATYPE_INT32.");
            }
            break;

        case SIMCONNECT_DATATYPE_INT64:
            if constexpr (std::is_same_v<FieldType, int> || std::is_same_v<FieldType, long> ||
                          std::is_same_v<FieldType, int32_t> ||
                          std::is_same_v<FieldType, float> ||
                          std::is_same_v<FieldType, double>) {
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = static_cast<FieldType>(reader.readInt64());
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addInt64(static_cast<int64_t>(data.*field));
                };
            } else if constexpr (std::is_same_v<FieldType, long long> ||
                                 std::is_same_v<FieldType, uint64_t>) {
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = reader.readInt64();
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addInt64(static_cast<int64_t>(data.*field));
                };
            } else if constexpr (std::is_same_v<FieldType, bool>) { // Wasteful, but your choice
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = reader.readInt64() != 0;
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addInt64(data.*field ? 1 : 0);
                };
            } else if constexpr (std::is_same_v<FieldType, std::string>) {
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = std::to_string(reader.readInt64());
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addInt64(std::stoll((data.*field).c_str()));
                };
            } else {
                throw SimConnectException("Invalid field type for SIMCONNECT_DATATYPE_INT64.");
            }
            break;

        case SIMCONNECT_DATATYPE_FLOAT32:
            if constexpr (std::is_same_v<FieldType, int> || std::is_same_v<FieldType, long> ||
                          std::is_same_v<FieldType, int32_t> ||
                          std::is_same_v<FieldType, long long> ||
                          std::is_same_v<FieldType, uint64_t>) {
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = static_cast<FieldType>(reader.readFloat32());
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addFloat32(static_cast<float>(data.*field));
                };
            } else if constexpr (std::is_floating_point_v<FieldType>) {
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = reader.readFloat32();
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addFloat32(static_cast<float>(data.*field));
                };
            } else if constexpr (std::is_same_v<FieldType, bool>) { // You must be a little bit silly to want this, but it is possible
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = (reader.readFloat32() != 0.0f);
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addFloat32(data.*field ? 1.0f : 0.0f);
                };
            } else if constexpr (std::is_same_v<FieldType, std::string>) {
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = std::to_string(reader.readFloat32());
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addFloat32(std::stof((data.*field).c_str()));
                };
            } else {
                throw SimConnectException("Invalid field type for SIMCONNECT_DATATYPE_FLOAT32.");
            }
            break;

        case SIMCONNECT_DATATYPE_FLOAT64:
            if constexpr (std::is_same_v<FieldType, int> || std::is_same_v<FieldType, long> ||
                          std::is_same_v<FieldType, int32_t> ||
                          std::is_same_v<FieldType, long long> ||
                          std::is_same_v<FieldType, uint64_t> ||
                          std::is_same_v<FieldType, float>) {
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = static_cast<FieldType>(reader.readFloat64());
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addFloat64(static_cast<double>(data.*field));
                };
            } else if constexpr (std::is_same_v<FieldType, double>) {
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = reader.readFloat64();
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addFloat64(data.*field);
                };
            } else if constexpr (std::is_same_v<FieldType, bool>) { // You must be a little bit silly to want this, but it is possible
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = reader.readFloat64() != 0.0f;
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addFloat64(data.*field ? 1.0d : 0.0d);
                };
            } else if constexpr (std::is_same_v<FieldType, std::string>) {
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = std::to_string(reader.readFloat64());
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addFloat64(std::stod((data.*field).c_str()));
                };
            } else {
                throw SimConnectException("Invalid field type for SIMCONNECT_DATATYPE_FLOAT64.");
            }
            break;

        case SIMCONNECT_DATATYPE_STRING8:
            if constexpr (std::is_same_v<FieldType, std::string>) {
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = reader.readString8();
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addString8(data.*field);
                };
            } else {
                throw SimConnectException("Invalid field type for SIMCONNECT_DATATYPE_STRING8.");
            }
            break;

        case SIMCONNECT_DATATYPE_STRING32:
            if constexpr (std::is_same_v<FieldType, std::string>) {
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = reader.readString32();
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addString32(data.*field);
                };
            } else {
                throw SimConnectException("Invalid field type for SIMCONNECT_DATATYPE_STRING32.");
            }
            break;

        case SIMCONNECT_DATATYPE_STRING64:
            if constexpr (std::is_same_v<FieldType, std::string>) {
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = reader.readString64();
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addString64(data.*field);
                };
            } else {
                throw SimConnectException("Invalid field type for SIMCONNECT_DATATYPE_STRING64.");
            }
            break;

        case SIMCONNECT_DATATYPE_STRING128:
            if constexpr (std::is_same_v<FieldType, std::string>) {
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = reader.readString128();
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addString128(data.*field);
                };
            } else {
                throw SimConnectException("Invalid field type for SIMCONNECT_DATATYPE_STRING128.");
            }
            break;

        case SIMCONNECT_DATATYPE_STRING256:
            if constexpr (std::is_same_v<FieldType, std::string>) {
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = reader.readString256();
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addString256(data.*field);
                };
            } else {
                throw SimConnectException("Invalid field type for SIMCONNECT_DATATYPE_STRING256.");
            }
            break;

        case SIMCONNECT_DATATYPE_STRING260:
            if constexpr (std::is_same_v<FieldType, std::string>) {
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = reader.readString260();
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addString260(data.*field);
                };
            } else {
                throw SimConnectException("Invalid field type for SIMCONNECT_DATATYPE_STRING260.");
            }
            break;

        case SIMCONNECT_DATATYPE_STRINGV:
            if constexpr (std::is_same_v<FieldType, std::string>) {
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = reader.readStringV();
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addStringV(data.*field);
                };
            } else {
                throw SimConnectException("Invalid field type for SIMCONNECT_DATATYPE_STRINGV.");
            }
            break;

        case SIMCONNECT_DATATYPE_INITPOSITION:
            if constexpr (std::is_same_v<FieldType, SIMCONNECT_DATA_INITPOSITION>) {
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = reader.readInitPosition();
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addInitPosition(data.*field);
                };
            } else {
                throw SimConnectException("Invalid field type for SIMCONNECT_DATATYPE_INITPOSITION.");
            }
            break;

        case SIMCONNECT_DATATYPE_MARKERSTATE:
            if constexpr (std::is_same_v<FieldType, SIMCONNECT_DATA_MARKERSTATE>) {
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = reader.readMarkerState();
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addMarkerState(data.*field);
                };
            } else {
                throw SimConnectException("Invalid field type for SIMCONNECT_DATATYPE_MARKERSTATE.");
            }
            break;

        case SIMCONNECT_DATATYPE_WAYPOINT:
            if constexpr (std::is_same_v<FieldType, SIMCONNECT_DATA_WAYPOINT>) {
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = reader.readWaypoint();
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addWaypoint(data.*field);
                };
            } else {
                throw SimConnectException("Invalid field type for SIMCONNECT_DATATYPE_WAYPOINT.");
            }
            break;

        case SIMCONNECT_DATATYPE_LATLONALT:
            if constexpr (std::is_same_v<FieldType, SIMCONNECT_DATA_LATLONALT>) {
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = reader.readLatLonAlt();
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addLatLonAlt(data.*field);
                };
            } else {
                throw SimConnectException("Invalid field type for SIMCONNECT_DATATYPE_LATLONALT.");
            }
            break;

        case SIMCONNECT_DATATYPE_XYZ:
            if constexpr (std::is_same_v<FieldType, SIMCONNECT_DATA_XYZ>) {
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = reader.readXYZ();
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addXYZ(data.*field);
                };
            } else {
                throw SimConnectException("Invalid field type for SIMCONNECT_DATATYPE_XYZ.");
            }
            break;
        }
        fields_.emplace_back(simVar, units, dataType, 0.0f, SIMCONNECT_UNUSED, setter, getter);
        return *this;
    }


    void extract(std::span<const uint8_t> msg, StructType& data) const {
        Data::DataBlockReader reader(msg);

        for (auto& field : fields_) {
            if (field.setter) {
                field.setter(data, reader);
            } else {
                throw SimConnectException("Missing setter in DataDefinition::extract()",
                    "No setter function defined for field: " + field.simVar);
            }
        }
    }

    void extract(const SIMCONNECT_RECV_SIMOBJECT_DATA& msg, StructType& data) const {
        extract(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&msg.dwData), msg.dwDefineCount));
    }
};


} // namespace SimConnect