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
#include <cstddef>
#include <cstring>
 
#include <span>
#include <atomic>
#include <string>
#include <vector>
#include <variant>
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
    /**
     * Function type for setting a field of the receiving struct/class. The value is the "next" value in the DataBlockReader.
     * 
     * @param data The struct to receive the data.
     * @param reader The data block reader to read the data from.
     */
    using SetterFunc = std::function<void(StructType& data, Data::DataBlockReader& reader)>;

    /**
     * Function type for setting a field in the data block without a struct. The value is the "next" value in the DataBlockReader.
     * 
     * @param reader The data block reader to read the data from.
     */
    using StatelessSetterFunc = std::function<void(Data::DataBlockReader& reader)>;

    /**
     * Function type for getting a field from a data struct and storing it in the DataBlockBuilder.
     * 
     * @param builder The data block builder to write the data to.
     * @param data The struct containing the data.
     */
    using GetterFunc = std::function<void(Data::DataBlockBuilder& builder, const StructType& data)>;

    /**
     * Function type for storing the next field in the DataBlockBuilder.
     * 
     * @param builder The data block builder to write the data to.
     */
    using StatelessGetterFunc = std::function<void(Data::DataBlockBuilder& builder)>;


    /**
     * Information about a field in the data definition.
     */
    struct FieldInfo {
        std::string simVar{ "" };
        std::string units{ "" };
        SIMCONNECT_DATATYPE dataType{ SIMCONNECT_DATATYPE_INVALID };
        float epsilon{ 0.0f };
        unsigned long datumId{ SIMCONNECT_UNUSED };
        SetterFunc setter;
        StatelessSetterFunc statelessSetter;
        GetterFunc getter;
        StatelessGetterFunc statelessGetter;

        FieldInfo(std::string simVar, std::string units, SIMCONNECT_DATATYPE dataType, float epsilon, unsigned long datumId,
            SetterFunc setter, GetterFunc getter)
        : simVar(simVar), units(units), dataType(dataType), epsilon(epsilon), datumId(datumId), setter(setter), getter(getter) {}

        FieldInfo(std::string simVar, std::string units, SIMCONNECT_DATATYPE dataType, float epsilon, unsigned long datumId,
            StatelessSetterFunc setter, StatelessGetterFunc getter)
        : simVar(simVar), units(units), dataType(dataType), epsilon(epsilon), datumId(datumId), statelessSetter(setter), statelessGetter(getter) {}
    };

    int id_{ -1 };                  ///< The ID of the data definition.
    bool useMapping_{ true };       ///< Whether to map the struct on top of the incoming data for this data definition.
    std::vector<FieldInfo> fields_; ///< The fields in the data definition, containing the information about the field and the getter/setter functions.
    size_t size_{ 0 };              ///< The size of the data definition, used for mapping.

public:
    /**
     * Check if the data definition has been sent to SimConnect.
     */
    bool isDefined() const noexcept { return id_ != -1; }


    /**
     * Return the Data Definition Id.
     */
    [[nodiscard]]
    int id() const noexcept { return id_; }


    /**
     * Return the Data Definition Id.
     */
    operator SIMCONNECT_DATA_DEFINITION_ID() const { return static_cast<SIMCONNECT_DATA_DEFINITION_ID>(id_); }


    /**
     * Ask if we have a straight mapping between the struct and the data block.
     */
    [[nodiscard]]
    bool useMapping() const noexcept { return useMapping_; }


    /**
     * Registers a DataDefinition
     */
    void define(Connection& connection) {
        if (isDefined()) {
            return; // Already defined
        }
        id_ = connection.dataDefinitions().nextDataDefID();

        unsigned long datumId{ 1 };
        for (auto& field : fields_) {
            if (field.dataType == SIMCONNECT_DATATYPE_INVALID) {
                throw std::runtime_error("Invalid data type for field: " + field.simVar);
            }

            field.datumId = datumId++;

            connection.addDataDefinition(id_, field.simVar, field.units, field.dataType, field.epsilon, field.datumId);
        }
    }


    /**
     * Add a field to the data definition.
     */
    template <typename FieldType>
    DataDefinition& add(FieldType StructType::* field, SIMCONNECT_DATATYPE dataType, std::string simVar, std::string units = "") {
        GetterFunc getter;
        SetterFunc setter;

        switch (dataType) {

        case SIMCONNECT_DATATYPE_INT32:
        {
            if constexpr (!std::is_same_v<FieldType, int32_t>) {
                useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.
            }
            if constexpr (std::is_same_v<FieldType, int> || std::is_same_v<FieldType, long> ||
                          std::is_same_v<FieldType, int32_t> ||
                          std::is_same_v<FieldType, long long> ||
                          std::is_same_v<FieldType, int64_t> ||
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
            size_ += sizeof(int32_t); // Update the size of the data definition
        }
            break;

        case SIMCONNECT_DATATYPE_INT64:
        {
            if constexpr (!std::is_same_v<FieldType, int64_t>) {
                useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.
            }
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
            size_ += sizeof(int64_t); // Update the size of the data definition
        }
            break;

        case SIMCONNECT_DATATYPE_FLOAT32:
        {
            if constexpr (!std::is_same_v<FieldType, float>) {
                useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.
            }
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
            size_ += sizeof(float); // Update the size of the data definition
        }
            break;

        case SIMCONNECT_DATATYPE_FLOAT64:
        {
            if constexpr (!std::is_same_v<FieldType, double>) {
                useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.
            }
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
            size_ += sizeof(double); // Update the size of the data definition
        }
            break;

        case SIMCONNECT_DATATYPE_STRING8:
        {
            if constexpr (std::is_same_v<FieldType, std::string>) {
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = reader.readString8();
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addString8(data.*field);
                };
            } else if constexpr (std::is_array_v<FieldType> && std::is_same_v<std::remove_extent_t<FieldType>, char> &&
                                 std::extent_v<FieldType> >= 8) {
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    std::memset(data.*field, 0, std::extent_v<FieldType>);
                    std::memcpy(data.*field, reader.readPointer<char>(std::extent_v<FieldType>), 8);
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addString8(std::string_view(data.*field));
                };
            } else {
                throw SimConnectException("Invalid field type for SIMCONNECT_DATATYPE_STRING8.");
            }
        }
            break;

        case SIMCONNECT_DATATYPE_STRING32:
        {
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
        }
            break;

        case SIMCONNECT_DATATYPE_STRING64:
        {
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
        }
            break;

        case SIMCONNECT_DATATYPE_STRING128:
        {
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
        }
            break;

        case SIMCONNECT_DATATYPE_STRING256:
        {
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
        }
            break;

        case SIMCONNECT_DATATYPE_STRING260:
        {
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
        }
            break;

        case SIMCONNECT_DATATYPE_STRINGV:
        {
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
        }
            break;

        case SIMCONNECT_DATATYPE_INITPOSITION:
        {
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
        }
            break;

        case SIMCONNECT_DATATYPE_MARKERSTATE:
        {
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
        }
            break;

        case SIMCONNECT_DATATYPE_WAYPOINT:
        {
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
        }
            break;

        case SIMCONNECT_DATATYPE_LATLONALT:
        {
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
        }
            break;

        case SIMCONNECT_DATATYPE_XYZ:
        {
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
        }
            break;
        }
        fields_.emplace_back(simVar, units, dataType, 0.0f, SIMCONNECT_UNUSED, setter, getter);
        return *this;
    }


    // For SIMCONNECT_DATATYPE_INT32:

    DataDefinition& addInt32(std::string simVar, std::string units, std::function<void(int32_t)> setter, std::function<int32_t()> getter) {
        fields_.emplace_back(simVar, units, SIMCONNECT_DATATYPE_INT32, 0.0f, SIMCONNECT_UNUSED,
            [setter](Data::DataBlockReader& reader) {
                setter(reader.readInt32());
            },
            [getter](Data::DataBlockBuilder& builder) {
                builder.addInt32(getter());
            });
        return *this;
    }
    DataDefinition& addInt32(std::string simVar, std::string units, std::function<void(StructType& data, int32_t value)> setter, std::function<int32_t(const StructType& data)> getter) {
        fields_.emplace_back(simVar, units, SIMCONNECT_DATATYPE_INT32, 0.0f, SIMCONNECT_UNUSED,
            [setter](StructType& data, Data::DataBlockReader& reader) { setter(data, reader.readInt32()); },
            [getter](Data::DataBlockBuilder& builder, const StructType& data) { builder.addInt32(getter(data)); });
        return *this;
    }
    DataDefinition& addInt32(int32_t StructType::* field, std::string simVar, std::string units) {
        fields_.emplace_back(simVar, units, SIMCONNECT_DATATYPE_INT32, 0.0f, SIMCONNECT_UNUSED,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = reader.readInt32();
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addInt32(data.*field);
            });
        return *this;
    }
    DataDefinition& addInt32(int64_t StructType::* field, std::string simVar, std::string units) {
        fields_.emplace_back(simVar, units, SIMCONNECT_DATATYPE_INT32, 0.0f, SIMCONNECT_UNUSED,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = reader.readInt32();
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addInt32(data.*field);
            });
        return *this;
    }
    DataDefinition& addInt32(float StructType::* field, std::string simVar, std::string units) {
        fields_.emplace_back(simVar, units, SIMCONNECT_DATATYPE_INT32, 0.0f, SIMCONNECT_UNUSED,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = static_cast<float>(reader.readInt32());
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addInt32(static_cast<int32_t>(data.*field));
            });
        return *this;
    }
    DataDefinition& addInt32(double StructType::* field, std::string simVar, std::string units) {
        fields_.emplace_back(simVar, units, SIMCONNECT_DATATYPE_INT32, 0.0f, SIMCONNECT_UNUSED,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = static_cast<double>(reader.readInt32());
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addInt32(static_cast<int32_t>(data.*field));
            });
        return *this;
    }
    DataDefinition& addInt32(bool StructType::* field, std::string simVar, std::string units) {
        fields_.emplace_back(simVar, units, SIMCONNECT_DATATYPE_INT32, 0.0f, SIMCONNECT_UNUSED,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = reader.readInt32() != 0;
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addInt32((data.*field) ? 1 : 0);
            });
        return *this;
    }
    DataDefinition& addInt32(std::string StructType::* field, std::string simVar, std::string units) {
        fields_.emplace_back(simVar, units, SIMCONNECT_DATATYPE_INT32, 0.0f, SIMCONNECT_UNUSED,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = std::to_string(reader.readInt32());
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addInt32(stoi(data.*field));
            });
        return *this;
    }


    // For SIMCONNECT_DATATYPE_INT64:

    DataDefinition& addInt64(std::string simVar, std::string units, std::function<void(int64_t)> setter, std::function<int64_t()> getter) {
        fields_.emplace_back(simVar, units, SIMCONNECT_DATATYPE_INT64, 0.0f, SIMCONNECT_UNUSED,
            [setter](Data::DataBlockReader& reader) {
                setter(reader.readInt64());
            },
            [getter](Data::DataBlockBuilder& builder) {
                builder.addInt64(getter());
            });
        return *this;
    }
    DataDefinition& addInt64(std::string simVar, std::string units, std::function<void(StructType& data, int64_t value)> setter, std::function<int64_t(const StructType& data)> getter) {
        fields_.emplace_back(simVar, units, SIMCONNECT_DATATYPE_INT64, 0.0f, SIMCONNECT_UNUSED,
            [setter](StructType& data, Data::DataBlockReader& reader) { setter(data, reader.readInt64()); },
            [getter](Data::DataBlockBuilder& builder, const StructType& data) { builder.addInt64(getter(data)); });
        return *this;
    }
    DataDefinition& addInt64(int32_t StructType::* field, std::string simVar, std::string units) {
        fields_.emplace_back(simVar, units, SIMCONNECT_DATATYPE_INT64, 0.0f, SIMCONNECT_UNUSED,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = static_cast<int32_t>(reader.readInt64());
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addInt64(data.*field);
            });
        return *this;
    }
    DataDefinition& addInt64(int64_t StructType::* field, std::string simVar, std::string units) {
        fields_.emplace_back(simVar, units, SIMCONNECT_DATATYPE_INT64, 0.0f, SIMCONNECT_UNUSED,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = reader.readInt64();
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addInt64(data.*field);
            });
        return *this;
    }
    DataDefinition& addInt64(float StructType::* field, std::string simVar, std::string units) {
        fields_.emplace_back(simVar, units, SIMCONNECT_DATATYPE_INT64, 0.0f, SIMCONNECT_UNUSED,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = static_cast<float>(reader.readInt64());
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addInt64(static_cast<int64_t>(data.*field));
            });
        return *this;
    }
    DataDefinition& addInt64(double StructType::* field, std::string simVar, std::string units) {
        fields_.emplace_back(simVar, units, SIMCONNECT_DATATYPE_INT64, 0.0f, SIMCONNECT_UNUSED,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = static_cast<double>(reader.readInt64());
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addInt64(static_cast<int64_t>(data.*field));
            });
        return *this;
    }
    DataDefinition& addInt64(bool StructType::* field, std::string simVar, std::string units) {
        fields_.emplace_back(simVar, units, SIMCONNECT_DATATYPE_INT64, 0.0f, SIMCONNECT_UNUSED,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = reader.readInt64() != 0;
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addInt64((data.*field) ? 1 : 0);
            });
        return *this;
    }
    DataDefinition& addInt64(std::string StructType::* field, std::string simVar, std::string units) {
        fields_.emplace_back(simVar, units, SIMCONNECT_DATATYPE_INT64, 0.0f, SIMCONNECT_UNUSED,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = std::to_string(reader.readInt64());
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addInt64(stoll(data.*field));
            });
        return *this;
    }

 
    // For SIMCONNECT_DATATYPE_FLOAT32:

    DataDefinition& addFloat32(std::string simVar, std::string units, std::function<void(float)> setter, std::function<float()> getter) {
        fields_.emplace_back(simVar, units, SIMCONNECT_DATATYPE_FLOAT32, 0.0f, SIMCONNECT_UNUSED,
            [setter](Data::DataBlockReader& reader) {
                setter(reader.readFloat32());
            },
            [getter](Data::DataBlockBuilder& builder) {
                builder.addFloat32(getter());
            });
        return *this;
    }
    DataDefinition& addFloat32(std::string simVar, std::string units, std::function<void(StructType& data, float value)> setter, std::function<float(const StructType& data)> getter) {
        fields_.emplace_back(simVar, units, SIMCONNECT_DATATYPE_FLOAT32, 0.0f, SIMCONNECT_UNUSED,
            [setter](StructType& data, Data::DataBlockReader& reader) { setter(data, reader.readFloat32()); },
            [getter](Data::DataBlockBuilder& builder, const StructType& data) { builder.addFloat32(getter(data)); });
        return *this;
    }
    DataDefinition& addFloat32(int32_t StructType::* field, std::string simVar, std::string units) {
        fields_.emplace_back(simVar, units, SIMCONNECT_DATATYPE_FLOAT32, 0.0f, SIMCONNECT_UNUSED,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = static_cast<int32_t>(reader.readFloat32());
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addFloat32(static_cast<float>(data.*field));
            });
        return *this;
    }
    DataDefinition& addFloat32(int64_t StructType::* field, std::string simVar, std::string units) {
        fields_.emplace_back(simVar, units, SIMCONNECT_DATATYPE_FLOAT32, 0.0f, SIMCONNECT_UNUSED,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = static_cast<int64_t>(reader.readFloat32());
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addFloat32(static_cast<float>(data.*field));
            });
        return *this;
    }
    DataDefinition& addFloat32(float StructType::* field, std::string simVar, std::string units) {
        fields_.emplace_back(simVar, units, SIMCONNECT_DATATYPE_FLOAT32, 0.0f, SIMCONNECT_UNUSED,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = reader.readFloat32();
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addFloat32(data.*field);
            });
        return *this;
    }
    DataDefinition& addFloat32(double StructType::* field, std::string simVar, std::string units) {
        fields_.emplace_back(simVar, units, SIMCONNECT_DATATYPE_FLOAT32, 0.0f, SIMCONNECT_UNUSED,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = reader.readFloat32();
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addFloat32(static_cast<float>(data.*field));
            });
        return *this;
    }
    DataDefinition& addFloat32(bool StructType::* field, std::string simVar, std::string units) {
        fields_.emplace_back(simVar, units, SIMCONNECT_DATATYPE_FLOAT32, 0.0f, SIMCONNECT_UNUSED,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = reader.readFloat32() != 0.0f;
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addFloat32((data.*field) ? 1.0f : 0.0f);
            });
        return *this;
    }
    DataDefinition& addFloat32(std::string StructType::* field, std::string simVar, std::string units) {
        fields_.emplace_back(simVar, units, SIMCONNECT_DATATYPE_FLOAT32, 0.0f, SIMCONNECT_UNUSED,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = std::to_string(reader.readFloat32());
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addFloat32(stof(data.*field));
            });
        return *this;
    }


    // For SIMCONNECT_DATATYPE_FLOAT64:

    DataDefinition& addFloat64(std::string simVar, std::string units, std::function<void(double)> setter, std::function<double()> getter) {
        fields_.emplace_back(simVar, units, SIMCONNECT_DATATYPE_FLOAT64, 0.0f, SIMCONNECT_UNUSED,
            [setter](Data::DataBlockReader& reader) {
                setter(reader.readFloat64());
            },
            [getter](Data::DataBlockBuilder& builder) {
                builder.addFloat64(getter());
            });
        return *this;
    }
    DataDefinition& addFloat64(std::string simVar, std::string units, std::function<void(StructType& data, double value)> setter, std::function<double(const StructType& data)> getter) {
        fields_.emplace_back(simVar, units, SIMCONNECT_DATATYPE_FLOAT64, 0.0f, SIMCONNECT_UNUSED,
            [setter](StructType& data, Data::DataBlockReader& reader) { setter(data, reader.readFloat64()); },
            [getter](Data::DataBlockBuilder& builder, const StructType& data) { builder.addFloat64(getter(data)); });
        return *this;
    }
    DataDefinition& addFloat64(int32_t StructType::* field, std::string simVar, std::string units) {
        fields_.emplace_back(simVar, units, SIMCONNECT_DATATYPE_FLOAT64, 0.0f, SIMCONNECT_UNUSED,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = static_cast<int32_t>(reader.readFloat64());
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addFloat64(static_cast<double>(data.*field));
            });
        return *this;
    }
    DataDefinition& addFloat64(int64_t StructType::* field, std::string simVar, std::string units) {
        fields_.emplace_back(simVar, units, SIMCONNECT_DATATYPE_FLOAT64, 0.0f, SIMCONNECT_UNUSED,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = static_cast<int64_t>(reader.readFloat64());
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addFloat64(static_cast<double>(data.*field));
            });
        return *this;
    }
    DataDefinition& addFloat64(float StructType::* field, std::string simVar, std::string units) {
        fields_.emplace_back(simVar, units, SIMCONNECT_DATATYPE_FLOAT64, 0.0f, SIMCONNECT_UNUSED,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = static_cast<float>(reader.readFloat64());
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addFloat64(data.*field);
            });
        return *this;
    }
    DataDefinition& addFloat64(double StructType::* field, std::string simVar, std::string units) {
        fields_.emplace_back(simVar, units, SIMCONNECT_DATATYPE_FLOAT64, 0.0f, SIMCONNECT_UNUSED,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = reader.readFloat64();
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addFloat64(data.*field);
            });
        return *this;
    }
    DataDefinition& addFloat64(bool StructType::* field, std::string simVar, std::string units) {
        fields_.emplace_back(simVar, units, SIMCONNECT_DATATYPE_FLOAT64, 0.0f, SIMCONNECT_UNUSED,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = reader.readFloat64() != 0.0;
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addFloat64((data.*field) ? 1.0 : 0.0);
            });
        return *this;
    }
    DataDefinition& addFloat64(std::string StructType::* field, std::string simVar, std::string units) {
        fields_.emplace_back(simVar, units, SIMCONNECT_DATATYPE_FLOAT64, 0.0f, SIMCONNECT_UNUSED,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = std::to_string(reader.readFloat64());
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addFloat64(stod(data.*field));
            });
        return *this;
    }


    // For SIMCONNECT_DATATYPE_STRING*:

    DataDefinition& addString8(std::string simVar, std::function<void(std::string)> setter, std::function<std::string()> getter) {
        fields_.emplace_back(simVar, "", SIMCONNECT_DATATYPE_STRING8, 0.0f, SIMCONNECT_UNUSED,
            [setter](Data::DataBlockReader& reader) {
                setter(reader.readString8());
            },
            [getter](Data::DataBlockBuilder& builder) {
                builder.addString8(getter());
            });
        return *this;
    }
    DataDefinition& addString8(std::string simVar, std::function<void(StructType& data, std::string value)> setter, std::function<std::string(const StructType& data)> getter) {
        fields_.emplace_back(simVar, "", SIMCONNECT_DATATYPE_STRING8, 0.0f, SIMCONNECT_UNUSED,
            [setter](StructType& data, Data::DataBlockReader& reader) { setter(data, reader.readString8()); },
            [getter](Data::DataBlockBuilder& builder, const StructType& data) { builder.addString8(getter(data)); });
        return *this;
    }
    DataDefinition& addString8(std::string StructType::* field, std::string simVar) {
        fields_.emplace_back(simVar, "", SIMCONNECT_DATATYPE_STRING8, 0.0f, SIMCONNECT_UNUSED,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = reader.readString8();
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addString8(data.*field);
            });
        return *this;
    }
    DataDefinition& addString32(std::string simVar, std::function<void(std::string)> setter, std::function<std::string()> getter) {
        fields_.emplace_back(simVar, "", SIMCONNECT_DATATYPE_STRING32, 0.0f, SIMCONNECT_UNUSED,
            [setter](Data::DataBlockReader& reader) {
                setter(reader.readString32());
            },
            [getter](Data::DataBlockBuilder& builder) {
                builder.addString32(getter());
            });
        return *this;
    }
    DataDefinition& addString32(std::string simVar, std::function<void(StructType& data, std::string value)> setter, std::function<std::string(const StructType& data)> getter) {
        fields_.emplace_back(simVar, "", SIMCONNECT_DATATYPE_STRING32, 0.0f, SIMCONNECT_UNUSED,
            [setter](StructType& data, Data::DataBlockReader& reader) { setter(data, reader.readString32()); },
            [getter](Data::DataBlockBuilder& builder, const StructType& data) { builder.addString32(getter(data)); });
        return *this;
    }
    DataDefinition& addString32(std::string StructType::* field, std::string simVar) {
        fields_.emplace_back(simVar, "", SIMCONNECT_DATATYPE_STRING32, 0.0f, SIMCONNECT_UNUSED,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = reader.readString32();
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addString32(data.*field);
            });
        return *this;
    }
    DataDefinition& addString64(std::string simVar, std::function<void(std::string)> setter, std::function<std::string()> getter) {
        fields_.emplace_back(simVar, "", SIMCONNECT_DATATYPE_STRING64, 0.0f, SIMCONNECT_UNUSED,
            [setter](Data::DataBlockReader& reader) {
                setter(reader.readString64());
            },
            [getter](Data::DataBlockBuilder& builder) {
                builder.addString64(getter());
            });
        return *this;
    }
    DataDefinition& addString64(std::string simVar, std::function<void(StructType& data, std::string value)> setter, std::function<std::string(const StructType& data)> getter) {
        fields_.emplace_back(simVar, "", SIMCONNECT_DATATYPE_STRING64, 0.0f, SIMCONNECT_UNUSED,
            [setter](StructType& data, Data::DataBlockReader& reader) { setter(data, reader.readString64()); },
            [getter](Data::DataBlockBuilder& builder, const StructType& data) { builder.addString64(getter(data)); });
        return *this;
    }
    DataDefinition& addString64(std::string StructType::* field, std::string simVar) {
        fields_.emplace_back(simVar, "", SIMCONNECT_DATATYPE_STRING64, 0.0f, SIMCONNECT_UNUSED,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = reader.readString64();
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addString64(data.*field);
            });
        return *this;
    }
    DataDefinition& addString128(std::string simVar, std::function<void(std::string)> setter, std::function<std::string()> getter) {
        fields_.emplace_back(simVar, "", SIMCONNECT_DATATYPE_STRING128, 0.0f, SIMCONNECT_UNUSED,
            [setter](Data::DataBlockReader& reader) {
                setter(reader.readString128());
            },
            [getter](Data::DataBlockBuilder& builder) {
                builder.addString128(getter());
            });
        return *this;
    }
    DataDefinition& addString128(std::string simVar, std::function<void(StructType& data, std::string value)> setter, std::function<std::string(const StructType& data)> getter) {
        fields_.emplace_back(simVar, "", SIMCONNECT_DATATYPE_STRING128, 0.0f, SIMCONNECT_UNUSED,
            [setter](StructType& data, Data::DataBlockReader& reader) { setter(data, reader.readString128()); },
            [getter](Data::DataBlockBuilder& builder, const StructType& data) { builder.addString128(getter(data)); });
        return *this;
    }
    DataDefinition& addString128(std::string StructType::* field, std::string simVar) {
        fields_.emplace_back(simVar, "", SIMCONNECT_DATATYPE_STRING128, 0.0f, SIMCONNECT_UNUSED,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = reader.readString128();
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addString128(data.*field);
            });
        return *this;
    }
    DataDefinition& addString256(std::string simVar, std::function<void(std::string)> setter, std::function<std::string()> getter) {
        fields_.emplace_back(simVar, "", SIMCONNECT_DATATYPE_STRING256, 0.0f, SIMCONNECT_UNUSED,
            [setter](Data::DataBlockReader& reader) {
                setter(reader.readString256());
            },
            [getter](Data::DataBlockBuilder& builder) {
                builder.addString256(getter());
            });
        return *this;
    }
    DataDefinition& addString256(std::string simVar, std::function<void(StructType& data, std::string value)> setter, std::function<std::string(const StructType& data)> getter) {
        fields_.emplace_back(simVar, "", SIMCONNECT_DATATYPE_STRING256, 0.0f, SIMCONNECT_UNUSED,
            [setter](StructType& data, Data::DataBlockReader& reader) { setter(data, reader.readString256()); },
            [getter](Data::DataBlockBuilder& builder, const StructType& data) { builder.addString256(getter(data)); });
        return *this;
    }
    DataDefinition& addString256(std::string StructType::* field, std::string simVar) {
        fields_.emplace_back(simVar, "", SIMCONNECT_DATATYPE_STRING256, 0.0f, SIMCONNECT_UNUSED,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = reader.readString256();
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addString256(data.*field);
            });
        return *this;
    }
    DataDefinition& addString260(std::string simVar, std::function<void(std::string)> setter, std::function<std::string()> getter) {
        fields_.emplace_back(simVar, "", SIMCONNECT_DATATYPE_STRING260, 0.0f, SIMCONNECT_UNUSED,
            [setter](Data::DataBlockReader& reader) {
                setter(reader.readString260());
            },
            [getter](Data::DataBlockBuilder& builder) {
                builder.addString260(getter());
            });
        return *this;
    }
    DataDefinition& addString260(std::string simVar, std::function<void(StructType& data, std::string value)> setter, std::function<std::string(const StructType& data)> getter) {
        fields_.emplace_back(simVar, "", SIMCONNECT_DATATYPE_STRING260, 0.0f, SIMCONNECT_UNUSED,
            [setter](StructType& data, Data::DataBlockReader& reader) { setter(data, reader.readString260()); },
            [getter](Data::DataBlockBuilder& builder, const StructType& data) { builder.addString260(getter(data)); });
        return *this;
    }
    DataDefinition& addString260(std::string StructType::* field, std::string simVar) {
        fields_.emplace_back(simVar, "", SIMCONNECT_DATATYPE_STRING260, 0.0f, SIMCONNECT_UNUSED,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = reader.readString260();
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addString260(data.*field);
            });
        return *this;
    }
    DataDefinition& addStringV(std::string simVar, std::function<void(std::string)> setter, std::function<std::string()> getter) {
        fields_.emplace_back(simVar, "", SIMCONNECT_DATATYPE_STRINGV, 0.0f, SIMCONNECT_UNUSED,
            [setter](Data::DataBlockReader& reader) {
                setter(reader.readStringV());
            },
            [getter](Data::DataBlockBuilder& builder) {
                builder.addStringV(getter());
            });
        return *this;
    }
    DataDefinition& addStringV(std::string simVar, std::function<void(StructType& data, std::string value)> setter, std::function<std::string(const StructType& data)> getter) {
        fields_.emplace_back(simVar, "", SIMCONNECT_DATATYPE_STRINGV, 0.0f, SIMCONNECT_UNUSED,
            [setter](StructType& data, Data::DataBlockReader& reader) { setter(data, reader.readStringV()); },
            [getter](Data::DataBlockBuilder& builder, const StructType& data) { builder.addStringV(getter(data)); });
        return *this;
    }
    DataDefinition& addStringV(std::string StructType::* field, std::string simVar) {
        fields_.emplace_back(simVar, "", SIMCONNECT_DATATYPE_STRINGV, 0.0f, SIMCONNECT_UNUSED,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = reader.readStringV();
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addStringV(data.*field);
            });
        return *this;
    }


    // For SIMCONNECT_DATATYPE_INITPOSITION:

    DataDefinition& addInitPosition(std::string simVar, std::string units, std::function<void(const SIMCONNECT_DATA_INITPOSITION&)> setter, std::function<const SIMCONNECT_DATA_INITPOSITION&()> getter) {
        fields_.emplace_back(simVar, units, SIMCONNECT_DATATYPE_INITPOSITION, 0.0f, SIMCONNECT_UNUSED,
            [setter](Data::DataBlockReader& reader) {
                setter(reader.readInitPosition());
            },
            [getter](Data::DataBlockBuilder& builder) {
                builder.addInitPosition(getter());
            });
        return *this;
    }
    DataDefinition& addInitPosition(std::string simVar, std::string units, std::function<void(StructType& data, const SIMCONNECT_DATA_INITPOSITION& value)> setter, std::function<SIMCONNECT_DATA_INITPOSITION(const StructType& data)> getter) {
        fields_.emplace_back(simVar, units, SIMCONNECT_DATATYPE_INITPOSITION, 0.0f, SIMCONNECT_UNUSED,
            [setter](StructType& data, Data::DataBlockReader& reader) { setter(data, reader.readInitPosition()); },
            [getter](Data::DataBlockBuilder& builder, const StructType& data) { builder.addInitPosition(getter(data)); });
        return *this;
    }
    DataDefinition& addInitPosition(SIMCONNECT_DATA_INITPOSITION StructType::* field, std::string simVar, std::string units) {
        fields_.emplace_back(simVar, units, SIMCONNECT_DATATYPE_INITPOSITION, 0.0f, SIMCONNECT_UNUSED,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = reader.readInitPosition();
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addInitPosition(data.*field);
            });
        return *this;
    }


    // For SIMCONNECT_DATATYPE_MARKERSTATE:

    DataDefinition& addMarkerState(std::string simVar, std::string units, std::function<void(const SIMCONNECT_DATA_MARKERSTATE&)> setter, std::function<const SIMCONNECT_DATA_MARKERSTATE&()> getter) {
        fields_.emplace_back(simVar, units, SIMCONNECT_DATATYPE_MARKERSTATE, 0.0f, SIMCONNECT_UNUSED,
            [setter](Data::DataBlockReader& reader) {
                setter(reader.readMarkerState());
            },
            [getter](Data::DataBlockBuilder& builder) {
                builder.addMarkerState(getter());
            });
        return *this;
    }
    DataDefinition& addMarkerState(std::string simVar, std::string units, std::function<void(StructType& data, const SIMCONNECT_DATA_MARKERSTATE& value)> setter, std::function<SIMCONNECT_DATA_MARKERSTATE(const StructType& data)> getter) {
        fields_.emplace_back(simVar, units, SIMCONNECT_DATATYPE_MARKERSTATE, 0.0f, SIMCONNECT_UNUSED,
            [setter](StructType& data, Data::DataBlockReader& reader) { setter(data, reader.readMarkerState()); },
            [getter](Data::DataBlockBuilder& builder, const StructType& data) { builder.addMarkerState(getter(data)); });
        return *this;
    }
    DataDefinition& addMarkerState(SIMCONNECT_DATA_MARKERSTATE StructType::* field, std::string simVar, std::string units) {
        fields_.emplace_back(simVar, units, SIMCONNECT_DATATYPE_MARKERSTATE, 0.0f, SIMCONNECT_UNUSED,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = reader.readMarkerState();
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addMarkerState(data.*field);
            });
        return *this;
    }


    // For SIMCONNECT_DATATYPE_WAYPOINT:

    DataDefinition& addWaypoint(std::string simVar, std::string units, std::function<void(const SIMCONNECT_DATA_WAYPOINT&)> setter, std::function<const SIMCONNECT_DATA_WAYPOINT&()> getter) {
        fields_.emplace_back(simVar, units, SIMCONNECT_DATATYPE_WAYPOINT, 0.0f, SIMCONNECT_UNUSED,
            [setter](Data::DataBlockReader& reader) {
                setter(reader.readWaypoint());
            },
            [getter](Data::DataBlockBuilder& builder) {
                builder.addWaypoint(getter());
            });
        return *this;
    }
    DataDefinition& addWaypoint(std::string simVar, std::string units, std::function<void(StructType& data, const SIMCONNECT_DATA_WAYPOINT& value)> setter, std::function<SIMCONNECT_DATA_WAYPOINT(const StructType& data)> getter) {
        fields_.emplace_back(simVar, units, SIMCONNECT_DATATYPE_WAYPOINT, 0.0f, SIMCONNECT_UNUSED,
            [setter](StructType& data, Data::DataBlockReader& reader) { setter(data, reader.readWaypoint()); },
            [getter](Data::DataBlockBuilder& builder, const StructType& data) { builder.addWaypoint(getter(data)); });
        return *this;
    }
    DataDefinition& addWaypoint(SIMCONNECT_DATA_WAYPOINT StructType::* field, std::string simVar, std::string units) {
        fields_.emplace_back(simVar, units, SIMCONNECT_DATATYPE_WAYPOINT, 0.0f, SIMCONNECT_UNUSED,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = reader.readWaypoint();
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addWaypoint(data.*field);
            });
        return *this;
    }


    // For SIMCONNECT_DATATYPE_LATLONALT:

    DataDefinition& addLatLonAlt(std::string simVar, std::string units, std::function<void(const SIMCONNECT_DATA_LATLONALT&)> setter, std::function<const SIMCONNECT_DATA_LATLONALT&()> getter) {
        fields_.emplace_back(simVar, units, SIMCONNECT_DATATYPE_LATLONALT, 0.0f, SIMCONNECT_UNUSED,
            [setter](Data::DataBlockReader& reader) {
                setter(reader.readLatLonAlt());
            },
            [getter](Data::DataBlockBuilder& builder) {
                builder.addLatLonAlt(getter());
            });
        return *this;
    }
    DataDefinition& addLatLonAlt(std::string simVar, std::string units, std::function<void(StructType& data, const SIMCONNECT_DATA_LATLONALT& value)> setter, std::function<SIMCONNECT_DATA_LATLONALT(const StructType& data)> getter) {
        fields_.emplace_back(simVar, units, SIMCONNECT_DATATYPE_LATLONALT, 0.0f, SIMCONNECT_UNUSED,
            [setter](StructType& data, Data::DataBlockReader& reader) { setter(data, reader.readLatLonAlt()); },
            [getter](Data::DataBlockBuilder& builder, const StructType& data) { builder.addLatLonAlt(getter(data)); });
        return *this;
    }
    DataDefinition& addLatLonAlt(SIMCONNECT_DATA_LATLONALT StructType::* field, std::string simVar, std::string units) {
        fields_.emplace_back(simVar, units, SIMCONNECT_DATATYPE_LATLONALT, 0.0f, SIMCONNECT_UNUSED,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = reader.readLatLonAlt();
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addLatLonAlt(data.*field);
            });
        return *this;
    }


    // For SIMCONNECT_DATATYPE_XYZ:

    DataDefinition& addXYZ(std::string simVar, std::string units, std::function<void(const SIMCONNECT_DATA_XYZ&)> setter, std::function<const SIMCONNECT_DATA_XYZ&()> getter) {
        fields_.emplace_back(simVar, units, SIMCONNECT_DATATYPE_XYZ, 0.0f, SIMCONNECT_UNUSED,
            [setter](Data::DataBlockReader& reader) {
                setter(reader.readXYZ());
            },
            [getter](Data::DataBlockBuilder& builder) {
                builder.addXYZ(getter());
            });
        return *this;
    }
    DataDefinition& addXYZ(std::string simVar, std::string units, std::function<void(StructType& data, const SIMCONNECT_DATA_XYZ& value)> setter, std::function<const SIMCONNECT_DATA_XYZ&(const StructType& data)> getter) {
        fields_.emplace_back(simVar, units, SIMCONNECT_DATATYPE_XYZ, 0.0f, SIMCONNECT_UNUSED,
            [setter](StructType& data, Data::DataBlockReader& reader) { setter(data, reader.readXYZ()); },
            [getter](Data::DataBlockBuilder& builder, const StructType& data) { builder.addXYZ(getter(data)); });
        return *this;
    }
    DataDefinition& addXYZ(SIMCONNECT_DATA_XYZ StructType::* field, std::string simVar, std::string units) {
        fields_.emplace_back(simVar, units, SIMCONNECT_DATATYPE_XYZ, 0.0f, SIMCONNECT_UNUSED,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = reader.readXYZ();
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addXYZ(data.*field);
            });
        return *this;
    }


    // Marshalling and Unmarshalling:

    void marshall(Data::DataBlockBuilder& builder, const StructType& data, bool isTagged = false) const {
        for (const auto& field : fields_) {
            if (field.getter) {
                field.getter(builder, data);
            } else if (field.statelessGetter) {
                field.statelessGetter(builder);
            } else {
                throw SimConnectException("Missing getter in DataDefinition::marshall()",
                    "No getter function defined for field: " + field.simVar);
            }
        }
    }


    static constexpr int unTagged = -1;

    void unmarshall(Data::DataBlockReader& reader, StructType& data, int numElems = unTagged) const {
        if (numElems == unTagged) {
            for (auto& field : fields_) {
                if (field.setter) {
                    field.setter(data, reader);
                }
                else if (field.statelessSetter) {
                    field.statelessSetter(reader);
                }
                else {
                    throw SimConnectException("Missing setter in DataDefinition::unmarshall()",
                        "No setter function defined for field: " + field.simVar);
                }
            }
        }
        else { // Tagged data
            while (numElems-- > 0) {
                size_t id = reader.readInt32();
                if (id == 0) {
                    continue; // Skip empty entries
                }
                if (id > fields_.size()) {
                    throw SimConnectException("Invalid field ID in DataDefinition::unmarshall()",
                        "Field ID out of range: " + std::to_string(id));
                }
                auto& field = fields_[id - 1]; // ID is 1-based, fields_ is 0-based
                if (field.setter) {
                    field.setter(data, reader);
                }
                else if (field.statelessSetter) {
                    field.statelessSetter(reader);
                }
                else {
                    throw SimConnectException("Missing setter in DataDefinition::unmarshall()",
                        "No setter function defined for field: " + field.simVar);
                }
            }
        }
    }


    void unmarshall(std::span<const uint8_t> msg, StructType& data, int numElems = unTagged) const {
        Data::DataBlockReader reader(msg);

        unmarshall(reader, data, numElems);
    }

    
    void unmarshall(const SIMCONNECT_RECV_SIMOBJECT_DATA& msg, StructType& data) const {
        Data::DataBlockReader reader(msg);

        unmarshall(reader, data, ((msg.dwFlags & SIMCONNECT_DATA_REQUEST_FLAG_TAGGED) != 0) ? msg.dwDefineCount : unTagged);
    }
};


using StatelessDataDefinition = DataDefinition<std::monostate>;

} // namespace SimConnect