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
#include <format>
#include <span>
#include <atomic>
#include <string>
#include <vector>
#include <variant>
#include <functional>
#include <optional>

#include <simconnect/simconnect.hpp>
#include <simconnect/simconnect_exception.hpp>
#include <simconnect/connection.hpp>
#include <simconnect/data/data_block_builder.hpp>
#include <simconnect/data/data_block_reader.hpp>


namespace SimConnect {


constexpr size_t stringSize(DataType dataType) {
    switch (dataType) {
    case DataTypes::string8:   return 8;
    case DataTypes::string32:  return 32;
    case DataTypes::string64:  return 64;
    case DataTypes::string128: return 128;
    case DataTypes::string256: return 256;
    case DataTypes::string260: return 260;
    default: return 0; // Invalid or unsupported string type
    }
}


// Primary template - false case
template<typename T>
struct is_char_array : std::false_type {};

// Specialization for std::array<char, N>
template<std::size_t N>
struct is_char_array<std::array<char, N>> : std::true_type {};

// Helper variable template (C++17+)
template<typename T>
inline constexpr bool is_char_array_v = is_char_array<T>::value;


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
        DataType dataType{ DataTypes::invalid };
        float epsilon{ 0.0f };
        unsigned long datumId{ unused };
        SetterFunc setter;
        StatelessSetterFunc statelessSetter;
        GetterFunc getter;
        StatelessGetterFunc statelessGetter;

        FieldInfo(std::string simVar, std::string units, DataType dataType, float epsilon, unsigned long datumId,
            SetterFunc setter, GetterFunc getter)
        : simVar(simVar), units(units), dataType(dataType), epsilon(epsilon), datumId(datumId), setter(setter), getter(getter) {}

        FieldInfo(std::string simVar, std::string units, DataType dataType, float epsilon, unsigned long datumId,
            StatelessSetterFunc setter, StatelessGetterFunc getter)
        : simVar(simVar), units(units), dataType(dataType), epsilon(epsilon), datumId(datumId), statelessSetter(setter), statelessGetter(getter) {}
    };

    std::optional<DataDefinitionId> id_{ std::nullopt };    ///< The ID of the data definition.
    bool useMapping_{ true };                               ///< Whether to map the struct on top of the incoming data for this data definition.
    std::vector<FieldInfo> fields_;                         ///< The fields in the data definition, containing the information about the field and the getter/setter functions.
    size_t size_{ 0 };                                      ///< The size of the data definition, used for mapping.

public:
    /**
     * Check if the data definition has been sent to SimConnect.
     */
    bool isDefined() const noexcept { return id_.has_value(); }


    /**
     * Return the Data Definition Id.
     */
    [[nodiscard]]
    DataDefinitionId id() const noexcept { return id_.value_or(noId); }


    /**
     * Return the Data Definition Id.
     */
    operator DataDefinitionId() const noexcept { return id(); }


    /**
     * Ask if we have a straight mapping between the struct and the data block.
     */
    [[nodiscard]]
    bool useMapping() const noexcept { return useMapping_; }


    /**
	 * Disable the mapping for this data definition, for example because we want the objectId added in.
     */
	void disableMapping() noexcept { useMapping_ = false; }


    /**
     * Returns the size of the data definition.
     * 
     * @return The size of the data definition in bytes.
     */
    [[nodiscard]]
    size_t size() const noexcept { return size_; }


    /**
     * Registers a DataDefinition
     */
    template <class connection_type>
    void define(connection_type& connection) {
        if (isDefined()) {
            return; // Already defined
        }
        id_ = connection.dataDefinitions().nextDataDefID();

        unsigned long datumId{ 1 };
        for (auto& field : fields_) {
            if (field.dataType == DataTypes::invalid) {
                throw std::runtime_error("Invalid data type for field: " + field.simVar);
            }

            field.datumId = datumId++;

            connection.addDataDefinition(id(), field.simVar, field.units, field.dataType, field.epsilon, field.datumId);
        }
    }


    /**
     * Add a field to the data definition.
     * 
     * @param field The field in the struct to add.
     * @param dataType The SimConnect data type of the field.
     * @param simVar The simulator variable to request for this field.
     * @param units The units to request this value to be returned in. Defaults to an empty string, meaning no units are requested.
     * @return A reference to the current DataDefinition object, allowing for method chaining.
     */
    template <typename FieldType>
    DataDefinition& add(FieldType StructType::* field, DataType dataType, std::string simVar, std::string units = "") {
        GetterFunc getter; // Function to get the value from the struct so it can be added to the DataBlockBuilder.
        SetterFunc setter; // Function to set the value in the struct from the DataBlockReader.

        switch (dataType) {
        // Depending on the datatype, different conversions are needed between the value as transferred to/from
        // SimConnect and the value in the struct. This is done by using the setter and getter functions.
        // As long as all fields exactly match the SimConnect data types, we can use a direct mapping.

        // Numerical values are pretty straightforward.

        case DataTypes::int8:
        {
            if constexpr (!std::is_same_v<FieldType, int8_t>) {
                useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.
            }
            if constexpr (std::is_same_v<FieldType, int> || std::is_same_v<FieldType, long> || std::is_same_v<FieldType, long long> ||
                          std::is_same_v<FieldType, int32_t> || std::is_same_v<FieldType, int64_t> ||
                          std::is_same_v<FieldType, float> ||
                          std::is_same_v<FieldType, float> || std::is_same_v<FieldType, double>)
            { // Both setter and getter need a cast
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = static_cast<FieldType>(reader.readInt8());
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addInt8(static_cast<int8_t>(data.*field));
                };
                size_ += sizeof(int8_t); // Update the size of the data definition
            }
            else if constexpr (std::is_same_v<FieldType, char>)
            { // The setter can use automatic conversion, the getter needs a cast
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = reader.readInt8();
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addInt8(static_cast<int8_t>(data.*field));
                };
                size_ += sizeof(int8_t); // Update the size of the data definition
            }
            else if constexpr (std::is_same_v<FieldType, bool>)
            { // We support bool fields
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = reader.readInt8() != 0;
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addInt8(data.*field ? 1 : 0);
                };
                size_ += sizeof(int8_t); // Update the size of the data definition
            }
            else if constexpr (std::is_same_v<FieldType, std::string>)
            { // We support string fields, but we use just the standard conversion to/from string
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = std::format("{}", reader.readInt8());
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addInt8(static_cast<int8_t>(std::stoi((data.*field).c_str())));
                };
                size_ += sizeof(int8_t); // Update the size of the data definition
            }
            else { // Sorry, the rest is a no-go
                useMapping_ = false; // Make sure we'll not use direct mapping.
                throw SimConnectException("Invalid field type for DataTypes::int8.");
            }
        }
            break;

        case DataTypes::int32:
        {
            if constexpr (!std::is_same_v<FieldType, int32_t>) {
                useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.
            }
            if constexpr (std::is_same_v<FieldType, int> || std::is_same_v<FieldType, long> ||
                          std::is_same_v<FieldType, int32_t> ||
                          std::is_same_v<FieldType, long long> ||
                          std::is_same_v<FieldType, int64_t> ||
                          std::is_same_v<FieldType, double>)
            { // The setter can use automatic conversion, the getter needs a cast
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = reader.readInt32();
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addInt32(static_cast<int32_t>(data.*field));
                };
                size_ += sizeof(int32_t); // Update the size of the data definition
            }
            else if constexpr (std::is_same_v<FieldType, float>)
            { // Both setter and getter need a cast
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = static_cast<FieldType>(reader.readInt32());
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addInt32(static_cast<int32_t>(data.*field));
                };
                size_ += sizeof(int32_t); // Update the size of the data definition
            }
            else if constexpr (std::is_same_v<FieldType, bool>)
            { // We support bool fields
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = reader.readInt32() != 0;
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addInt32(data.*field ? 1 : 0);
                };
                size_ += sizeof(int32_t); // Update the size of the data definition
            }
            else if constexpr (std::is_same_v<FieldType, std::string>)
            { // We support string fields, but we use just the standard conversion to/from string
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = std::format("{}", reader.readInt32());
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addInt32(std::stoi((data.*field).c_str()));
                };
                size_ += sizeof(int32_t); // Update the size of the data definition
            }
            else { // Sorry, the rest is a no-go
                useMapping_ = false; // Make sure we'll not use direct mapping.
                throw SimConnectException("Invalid field type for DataTypes::int32.");
            }
        }
            break;

        case DataTypes::int64:
        {
            if constexpr (!std::is_same_v<FieldType, int64_t>) {
                useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.
            }
            if constexpr (std::is_same_v<FieldType, int> || std::is_same_v<FieldType, long> ||
                          std::is_same_v<FieldType, int32_t> ||
                          std::is_same_v<FieldType, float> ||
                          std::is_same_v<FieldType, double>)
            { // Both setter and getter need a cast
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = static_cast<FieldType>(reader.readInt64());
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addInt64(static_cast<int64_t>(data.*field));
                };
                size_ += sizeof(int64_t); // Update the size of the data definition
            }
            else if constexpr (std::is_same_v<FieldType, long long> ||
                               std::is_same_v<FieldType, uint64_t>)
            { // The setter can use automatic conversion, the getter needs a cast
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = reader.readInt64();
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addInt64(static_cast<int64_t>(data.*field));
                };
                size_ += sizeof(int64_t); // Update the size of the data definition
            }
            else if constexpr (std::is_same_v<FieldType, bool>)
            { // We support bool fields, but a 64-bit integer is wasteful IMHO
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = reader.readInt64() != 0;
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addInt64(data.*field ? 1 : 0);
                };
                size_ += sizeof(int64_t); // Update the size of the data definition
            }
            else if constexpr (std::is_same_v<FieldType, std::string>)
            { // We support string fields, but we use just the standard conversion to/from string
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = std::format("{}", reader.readInt64());
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addInt64(std::stoll((data.*field).c_str()));
                };
                size_ += sizeof(int64_t); // Update the size of the data definition
            }
            else { // Sorry, the rest is a no-go
                useMapping_ = false; // Make sure we'll not use direct mapping.
                throw SimConnectException("Invalid field type for DataTypes::int64.");
            }
        }
            break;

        case DataTypes::float32:
        {
            if constexpr (!std::is_same_v<FieldType, float>) {
                useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.
            }
            if constexpr (std::is_same_v<FieldType, int> || std::is_same_v<FieldType, long> ||
                          std::is_same_v<FieldType, int32_t> ||
                          std::is_same_v<FieldType, long long> ||
                          std::is_same_v<FieldType, uint64_t>)
            { // Both setter and getter need a cast
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = static_cast<FieldType>(reader.readFloat32());
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addFloat32(static_cast<float>(data.*field));
                };
                size_ += sizeof(float); // Update the size of the data definition
            }
            else if constexpr (std::is_floating_point_v<FieldType>)
            { // The setter can use automatic conversion, the getter needs a cast
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = reader.readFloat32();
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addFloat32(static_cast<float>(data.*field));
                };
                size_ += sizeof(float); // Update the size of the data definition
            }
            else if constexpr (std::is_same_v<FieldType, std::string>)
            { // We support string fields, but we use just the standard conversion to/from string
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = std::format("{}", reader.readFloat32());
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addFloat32(std::stof((data.*field).c_str()));
                };
                size_ += sizeof(float); // Update the size of the data definition
            }
            else { // Sorry, the rest is a no-go
                useMapping_ = false; // Make sure we'll not use direct mapping.
                throw SimConnectException("Invalid field type for DataTypes::float32.");
            }
        }
            break;

        case DataTypes::float64:
        {
            if constexpr (!std::is_same_v<FieldType, double>) {
                useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.
            }
            if constexpr (std::is_same_v<FieldType, int> || std::is_same_v<FieldType, long> ||
                          std::is_same_v<FieldType, int32_t> ||
                          std::is_same_v<FieldType, long long> ||
                          std::is_same_v<FieldType, uint64_t> ||
                          std::is_same_v<FieldType, float>)
            { // Both setter and getter need a cast
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = static_cast<FieldType>(reader.readFloat64());
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addFloat64(static_cast<double>(data.*field));
                };
                size_ += sizeof(double); // Update the size of the data definition
            }
            else if constexpr (std::is_same_v<FieldType, double>)
            { // The setter can use automatic conversion, the getter needs a cast
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = reader.readFloat64();
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addFloat64(data.*field);
                };
                size_ += sizeof(double); // Update the size of the data definition
            }
            else if constexpr (std::is_same_v<FieldType, std::string>)
            { // We support string fields, but we use just the standard conversion to/from string
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = std::format("{}", reader.readFloat64());
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addFloat64(std::stod((data.*field).c_str()));
                };
                size_ += sizeof(double); // Update the size of the data definition
            }
            else { // Sorry, the rest is a no-go
                useMapping_ = false; // Make sure we'll not use direct mapping.
                throw SimConnectException("Invalid field type for DataTypes::float64.");
            }
        }
            break;

        // String types are a bit more complex, as they can be fixed size or variable size.
        // Direct mapping requires a fixed size array-of-char. For the rest we can use the
        // DataBlockReader and DataBlockBuilder to read/write the string values.

        case DataTypes::string8:
        case DataTypes::string32:
        case DataTypes::string64:
        case DataTypes::string128:
        case DataTypes::string256:
        case DataTypes::string260:
        {
            if constexpr (std::is_same_v<FieldType, std::string>)
            {
                useMapping_ = false; // We cannot map an std::string.

                setter = [field, dataType](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = reader.readString(stringSize(dataType));
                };
                getter = [field, dataType](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addString(data.*field, stringSize(dataType));
                };
				size_ += stringSize(dataType); // Update the size of the data definition
            }
            else if constexpr (std::is_array_v<FieldType> && std::is_same_v<std::remove_extent_t<FieldType>, char>)
            {
                if (std::extent_v<FieldType> < stringSize(dataType)) {
                    throw SimConnectException("Invalid field type for a string.",
						std::format("Field type (char[{}]) is too small for DataTypes::string{}.", std::extent_v<FieldType>, stringSize(dataType)));
                }
                if (std::extent_v<FieldType> != stringSize(dataType)) {
                    useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.
                }

                // For a char array, we require the field to be at least the length of the SimConnect string type. std::memset()
                // and std::memcpy() should be the most efficient, but for the getter we let the DataBlockBuilder handle it.
                setter = [field, dataType](StructType& data, Data::DataBlockReader& reader) {
                    std::memset(data.*field, 0, std::extent_v<FieldType>);
                    std::memcpy(data.*field, reader.readPointer<char>(std::extent_v<FieldType>), stringSize(dataType));
                };
                getter = [field, dataType](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addString(std::string_view(data.*field), stringSize(dataType));
                };
                size_ += stringSize(dataType); // Update the size of the data definition
            }
            else if constexpr (is_char_array_v<FieldType>)
            {
                if (std::tuple_size_v<FieldType> < stringSize(dataType)) {
                    throw SimConnectException("Invalid field type for a string.",
                        std::format("Field type (char[{}]) is too small for DataTypes::string{}.", std::tuple_size_v<FieldType>, stringSize(dataType)));
                }
                if (std::tuple_size_v<FieldType> != stringSize(dataType)) {
                    useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.
                }
                // Perfect match for direct mapping
                setter = [field, dataType](StructType& data, Data::DataBlockReader& reader) {
                    std::memset((data.*field).data(), 0, (data.*field).size());
                    std::memcpy((data.*field).data(), reader.readPointer<char>((data.*field).size()), stringSize(dataType));
                };
                getter = [field, dataType](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addString(std::string_view((data.*field).data()), stringSize(dataType));
                };
                size_ += stringSize(dataType); // Update the size of the data definition
            }
            else { // Sorry, the rest is a no-go
                useMapping_ = false; // Make sure we'll not use direct mapping.
                throw SimConnectException("Incompatible field type for DataDefinition",
                    std::format("Invalid field type for DataTypes::string{}.", stringSize(dataType)));
            }
        }
            break;

        case DataTypes::stringV:
        { // If you're using DataTypes::StringV, you should use std::string.
            useMapping_ = false; // We cannot map an std::string.

            if constexpr (std::is_same_v<FieldType, std::string>) {
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = reader.readStringV();
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addStringV(data.*field);
                };
				size_ += 4; // This actually will have variable size, but 4 bytes is the minimum.
            }
            else { // Sorry, the rest is a no-go
                useMapping_ = false; // Make sure we'll not use direct mapping.
                throw SimConnectException("Invalid field type for DataTypes::StringV.");
            }
        }
            break;

        case DataTypes::initPosition:
        {
            if constexpr (std::is_same_v<FieldType, DataTypes::InitPosition>) {
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = reader.readInitPosition();
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addInitPosition(data.*field);
                };
				size_ += sizeof(DataTypes::InitPosition); // Update the size of the data definition
            }
            else {
                useMapping_ = false; // Make sure we'll not use direct mapping.
                throw SimConnectException("Invalid field type for DataTypes::InitPosition.");
            }
        }
            break;

        case DataTypes::markerState:
        {
            if constexpr (std::is_same_v<FieldType, DataTypes::MarkerState>) {
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = reader.readMarkerState();
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addMarkerState(data.*field);
                };
				size_ += sizeof(DataTypes::MarkerState); // Update the size of the data definition
            } else {
                useMapping_ = false; // Make sure we'll not use direct mapping.
                throw SimConnectException("Invalid field type for DataTypes::MarkerState.");
            }
        }
            break;

        case DataTypes::waypoint:
        {
            if constexpr (std::is_same_v<FieldType, DataTypes::Waypoint>) {
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = reader.readWaypoint();
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addWaypoint(data.*field);
                };
				size_ += sizeof(DataTypes::Waypoint); // Update the size of the data definition
            } else {
                useMapping_ = false; // Make sure we'll not use direct mapping.
                throw SimConnectException("Invalid field type for DataTypes::Waypoint.");
            }
        }
            break;

        case DataTypes::latLonAlt:
        {
            if constexpr (std::is_same_v<FieldType, DataTypes::LatLonAlt>) {
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = reader.readLatLonAlt();
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addLatLonAlt(data.*field);
                };
				size_ += sizeof(DataTypes::LatLonAlt); // Update the size of the data definition
            } else {
                useMapping_ = false; // Make sure we'll not use direct mapping.
                throw SimConnectException("Invalid field type for DataTypes::LatLonAlt.");
            }
        }
            break;

        case DataTypes::xyz:
        {
            if constexpr (std::is_same_v<FieldType, DataTypes::XYZ>) {
                setter = [field](StructType& data, Data::DataBlockReader& reader) {
                    data.*field = reader.readXYZ();
                };
                getter = [field](Data::DataBlockBuilder& builder, const StructType& data) {
                    builder.addXYZ(data.*field);
                };
				size_ += sizeof(DataTypes::XYZ); // Update the size of the data definition
            } else {
                useMapping_ = false; // Make sure we'll not use direct mapping.
                throw SimConnectException("Invalid field type for DataTypes::XYZ.");
            }
        }
            break;
    
        case DataTypes::invalid:
        case DataTypes::max:
            throw SimConnectException("DataDefinition error", std::format("Invalid data type specified for field '{}'.", simVar));
        }
        fields_.emplace_back(simVar, units, dataType, 0.0f, unused, setter, getter);
        return *this;
    }


    // The following methods are convenience methods for adding fields of specific types to the data definition.
    // These methods do not require you to specify the SimConnect data type, as it's inferred from the method name.
    // For each type there is also the option to provide custom setter and getter functions instead of a field pointer.

    //TODO: Add support for epsilon.

    // For DataTypes::int32:

    DataDefinition& addInt32(std::string simVar, std::string units, std::function<void(int32_t)> setter, std::function<int32_t()> getter) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, units, DataTypes::int32, 0.0f, unused,
            [setter](Data::DataBlockReader& reader) {
                setter(reader.readInt32());
            },
            [getter](Data::DataBlockBuilder& builder) {
                builder.addInt32(getter());
            });
        size_ += sizeof(int32_t);
        return *this;
    }
    DataDefinition& addInt32(std::string simVar, std::string units, std::function<void(StructType& data, int32_t value)> setter, std::function<int32_t(const StructType& data)> getter) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, units, DataTypes::int32, 0.0f, unused,
            [setter](StructType& data, Data::DataBlockReader& reader) { setter(data, reader.readInt32()); },
            [getter](Data::DataBlockBuilder& builder, const StructType& data) { builder.addInt32(getter(data)); });
        size_ += sizeof(int32_t);
        return *this;
    }
    DataDefinition& addInt32(int32_t StructType::* field, std::string simVar, std::string units) {
        fields_.emplace_back(simVar, units, DataTypes::int32, 0.0f, unused,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = reader.readInt32();
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addInt32(data.*field);
            });
        size_ += sizeof(int32_t);
        return *this;
    }
    DataDefinition& addInt32(int64_t StructType::* field, std::string simVar, std::string units) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, units, DataTypes::int32, 0.0f, unused,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = reader.readInt32();
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addInt32(static_cast<int32_t>(data.*field));
            });
        size_ += sizeof(int32_t);
        return *this;
    }
    DataDefinition& addInt32(float StructType::* field, std::string simVar, std::string units) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, units, DataTypes::int32, 0.0f, unused,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = static_cast<float>(reader.readInt32());
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addInt32(static_cast<int32_t>(data.*field));
            });
        size_ += sizeof(int32_t);
        return *this;
    }
    DataDefinition& addInt32(double StructType::* field, std::string simVar, std::string units) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, units, DataTypes::int32, 0.0f, unused,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = static_cast<double>(reader.readInt32());
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addInt32(static_cast<int32_t>(data.*field));
            });
        size_ += sizeof(int32_t);
        return *this;
    }
    DataDefinition& addInt32(bool StructType::* field, std::string simVar, std::string units) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, units, DataTypes::int32, 0.0f, unused,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = reader.readInt32() != 0;
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addInt32((data.*field) ? 1 : 0);
            });
        size_ += sizeof(int32_t);
        return *this;
    }
    DataDefinition& addInt32(std::string StructType::* field, std::string simVar, std::string units) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, units, DataTypes::int32, 0.0f, unused,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = std::format("{}", reader.readInt32());
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addInt32(stoi(data.*field));
            });
        size_ += sizeof(int32_t);
        return *this;
    }


    // For DataTypes::int64:

    DataDefinition& addInt64(std::string simVar, std::string units, std::function<void(int64_t)> setter, std::function<int64_t()> getter) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, units, DataTypes::int64, 0.0f, unused,
            [setter](Data::DataBlockReader& reader) {
                setter(reader.readInt64());
            },
            [getter](Data::DataBlockBuilder& builder) {
                builder.addInt64(getter());
            });
        size_ += sizeof(int64_t);
        return *this;
    }
    DataDefinition& addInt64(std::string simVar, std::string units, std::function<void(StructType& data, int64_t value)> setter, std::function<int64_t(const StructType& data)> getter) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, units, DataTypes::int64, 0.0f, unused,
            [setter](StructType& data, Data::DataBlockReader& reader) { setter(data, reader.readInt64()); },
            [getter](Data::DataBlockBuilder& builder, const StructType& data) { builder.addInt64(getter(data)); });
        size_ += sizeof(int64_t);
        return *this;
    }
    DataDefinition& addInt64(int32_t StructType::* field, std::string simVar, std::string units) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, units, DataTypes::int64, 0.0f, unused,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = static_cast<int32_t>(reader.readInt64());
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addInt64(data.*field);
            });
        size_ += sizeof(int64_t);
        return *this;
    }
    DataDefinition& addInt64(int64_t StructType::* field, std::string simVar, std::string units) {
        fields_.emplace_back(simVar, units, DataTypes::int64, 0.0f, unused,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = reader.readInt64();
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addInt64(data.*field);
            });
        size_ += sizeof(int64_t);
        return *this;
    }
    DataDefinition& addInt64(float StructType::* field, std::string simVar, std::string units) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, units, DataTypes::int64, 0.0f, unused,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = static_cast<float>(reader.readInt64());
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addInt64(static_cast<int64_t>(data.*field));
            });
        size_ += sizeof(int64_t);
        return *this;
    }
    DataDefinition& addInt64(double StructType::* field, std::string simVar, std::string units) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, units, DataTypes::int64, 0.0f, unused,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = static_cast<double>(reader.readInt64());
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addInt64(static_cast<int64_t>(data.*field));
            });
        size_ += sizeof(int64_t);
        return *this;
    }
    DataDefinition& addInt64(bool StructType::* field, std::string simVar, std::string units) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, units, DataTypes::int64, 0.0f, unused,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = reader.readInt64() != 0;
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addInt64((data.*field) ? 1 : 0);
            });
        size_ += sizeof(int64_t);
        return *this;
    }
    DataDefinition& addInt64(std::string StructType::* field, std::string simVar, std::string units) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, units, DataTypes::int64, 0.0f, unused,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = std::format("{}", reader.readInt64());
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addInt64(stoll(data.*field));
            });
        size_ += sizeof(int64_t);
        return *this;
    }

 
    // For DataTypes::float32:

    DataDefinition& addFloat32(std::string simVar, std::string units, std::function<void(float)> setter, std::function<float()> getter) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, units, DataTypes::float32, 0.0f, unused,
            [setter](Data::DataBlockReader& reader) {
                setter(reader.readFloat32());
            },
            [getter](Data::DataBlockBuilder& builder) {
                builder.addFloat32(getter());
            });
        size_ += sizeof(float);
        return *this;
    }
    DataDefinition& addFloat32(std::string simVar, std::string units, std::function<void(StructType& data, float value)> setter, std::function<float(const StructType& data)> getter) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, units, DataTypes::float32, 0.0f, unused,
            [setter](StructType& data, Data::DataBlockReader& reader) { setter(data, reader.readFloat32()); },
            [getter](Data::DataBlockBuilder& builder, const StructType& data) { builder.addFloat32(getter(data)); });
        size_ += sizeof(float);
        return *this;
    }
    DataDefinition& addFloat32(int32_t StructType::* field, std::string simVar, std::string units) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, units, DataTypes::float32, 0.0f, unused,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = static_cast<int32_t>(reader.readFloat32());
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addFloat32(static_cast<float>(data.*field));
            });
        size_ += sizeof(float);
        return *this;
    }
    DataDefinition& addFloat32(int64_t StructType::* field, std::string simVar, std::string units) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, units, DataTypes::float32, 0.0f, unused,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = static_cast<int64_t>(reader.readFloat32());
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addFloat32(static_cast<float>(data.*field));
            });
        size_ += sizeof(float);
        return *this;
    }
    DataDefinition& addFloat32(float StructType::* field, std::string simVar, std::string units) {
        fields_.emplace_back(simVar, units, DataTypes::float32, 0.0f, unused,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = reader.readFloat32();
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addFloat32(data.*field);
            });
        size_ += sizeof(float);
        return *this;
    }
    DataDefinition& addFloat32(double StructType::* field, std::string simVar, std::string units) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, units, DataTypes::float32, 0.0f, unused,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = reader.readFloat32();
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addFloat32(static_cast<float>(data.*field));
            });
        size_ += sizeof(float);
        return *this;
    }
    DataDefinition& addFloat32(bool StructType::* field, std::string simVar, std::string units) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, units, DataTypes::float32, 0.0f, unused,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = reader.readFloat32() != 0.0f;
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addFloat32((data.*field) ? 1.0f : 0.0f);
            });
        size_ += sizeof(float);
        return *this;
    }
    DataDefinition& addFloat32(std::string StructType::* field, std::string simVar, std::string units) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, units, DataTypes::float32, 0.0f, unused,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = std::format("{}", reader.readFloat32());
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addFloat32(stof(data.*field));
            });
        size_ += sizeof(float);
        return *this;
    }


    // For DataTypes::float64:

    DataDefinition& addFloat64(std::string simVar, std::string units, std::function<void(double)> setter, std::function<double()> getter) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, units, DataTypes::float64, 0.0f, unused,
            [setter](Data::DataBlockReader& reader) {
                setter(reader.readFloat64());
            },
            [getter](Data::DataBlockBuilder& builder) {
                builder.addFloat64(getter());
            });
        size_ += sizeof(double);
        return *this;
    }
    DataDefinition& addFloat64(std::string simVar, std::string units, std::function<void(StructType& data, double value)> setter, std::function<double(const StructType& data)> getter) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, units, DataTypes::float64, 0.0f, unused,
            [setter](StructType& data, Data::DataBlockReader& reader) { setter(data, reader.readFloat64()); },
            [getter](Data::DataBlockBuilder& builder, const StructType& data) { builder.addFloat64(getter(data)); });
        size_ += sizeof(double);
        return *this;
    }
    DataDefinition& addFloat64(int32_t StructType::* field, std::string simVar, std::string units) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, units, DataTypes::float64, 0.0f, unused,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = static_cast<int32_t>(reader.readFloat64());
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addFloat64(static_cast<double>(data.*field));
            });
        size_ += sizeof(double);
        return *this;
    }
    DataDefinition& addFloat64(int64_t StructType::* field, std::string simVar, std::string units) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, units, DataTypes::float64, 0.0f, unused,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = static_cast<int64_t>(reader.readFloat64());
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addFloat64(static_cast<double>(data.*field));
            });
        size_ += sizeof(double);
        return *this;
    }
    DataDefinition& addFloat64(float StructType::* field, std::string simVar, std::string units) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, units, DataTypes::float64, 0.0f, unused,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = static_cast<float>(reader.readFloat64());
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addFloat64(data.*field);
            });
        size_ += sizeof(double);
        return *this;
    }
    DataDefinition& addFloat64(double StructType::* field, std::string simVar, std::string units) {
        fields_.emplace_back(simVar, units, DataTypes::float64, 0.0f, unused,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = reader.readFloat64();
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addFloat64(data.*field);
            });
        size_ += sizeof(double);
        return *this;
    }
    DataDefinition& addFloat64(bool StructType::* field, std::string simVar, std::string units) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, units, DataTypes::float64, 0.0f, unused,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = reader.readFloat64() != 0.0;
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addFloat64((data.*field) ? 1.0 : 0.0);
            });
        size_ += sizeof(double);
        return *this;
    }
    DataDefinition& addFloat64(std::string StructType::* field, std::string simVar, std::string units) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, units, DataTypes::float64, 0.0f, unused,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = std::format("{}", reader.readFloat64());
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addFloat64(stod(data.*field));
            });
        size_ += sizeof(double);
        return *this;
    }


    // For DataTypes::string*:
    //TODO: character array fields...

    DataDefinition& addString8(std::string simVar, std::function<void(std::string)> setter, std::function<std::string()> getter) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, "", DataTypes::string8, 0.0f, unused,
            [setter](Data::DataBlockReader& reader) {
                setter(reader.readString8());
            },
            [getter](Data::DataBlockBuilder& builder) {
                builder.addString8(getter());
            });
        size_ += 8;
        return *this;
    }
    DataDefinition& addString8(std::string simVar, std::function<void(StructType& data, std::string value)> setter, std::function<std::string(const StructType& data)> getter) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, "", DataTypes::string8, 0.0f, unused,
            [setter](StructType& data, Data::DataBlockReader& reader) { setter(data, reader.readString8()); },
            [getter](Data::DataBlockBuilder& builder, const StructType& data) { builder.addString8(getter(data)); });
        size_ += 8;
        return *this;
    }
    DataDefinition& addString8(std::string StructType::* field, std::string simVar) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, "", DataTypes::string8, 0.0f, unused,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = reader.readString8();
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addString8(data.*field);
            });
        size_ += 8;
        return *this;
    }
    DataDefinition& addString32(std::string simVar, std::function<void(std::string)> setter, std::function<std::string()> getter) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, "", DataTypes::string32, 0.0f, unused,
            [setter](Data::DataBlockReader& reader) {
                setter(reader.readString32());
            },
            [getter](Data::DataBlockBuilder& builder) {
                builder.addString32(getter());
            });
        size_ += 32;
        return *this;
    }
    DataDefinition& addString32(std::string simVar, std::function<void(StructType& data, std::string value)> setter, std::function<std::string(const StructType& data)> getter) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, "", DataTypes::string32, 0.0f, unused,
            [setter](StructType& data, Data::DataBlockReader& reader) { setter(data, reader.readString32()); },
            [getter](Data::DataBlockBuilder& builder, const StructType& data) { builder.addString32(getter(data)); });
        size_ += 32;
        return *this;
    }
    DataDefinition& addString32(std::string StructType::* field, std::string simVar) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, "", DataTypes::string32, 0.0f, unused,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = reader.readString32();
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addString32(data.*field);
            });
        size_ += 32;
        return *this;
    }
    DataDefinition& addString64(std::string simVar, std::function<void(std::string)> setter, std::function<std::string()> getter) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, "", DataTypes::string64, 0.0f, unused,
            [setter](Data::DataBlockReader& reader) {
                setter(reader.readString64());
            },
            [getter](Data::DataBlockBuilder& builder) {
                builder.addString64(getter());
            });
        size_ += 64;
        return *this;
    }
    DataDefinition& addString64(std::string simVar, std::function<void(StructType& data, std::string value)> setter, std::function<std::string(const StructType& data)> getter) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, "", DataTypes::string64, 0.0f, unused,
            [setter](StructType& data, Data::DataBlockReader& reader) { setter(data, reader.readString64()); },
            [getter](Data::DataBlockBuilder& builder, const StructType& data) { builder.addString64(getter(data)); });
        size_ += 64;
        return *this;
    }
    DataDefinition& addString64(std::string StructType::* field, std::string simVar) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, "", DataTypes::string64, 0.0f, unused,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = reader.readString64();
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addString64(data.*field);
            });
        size_ += 64;
        return *this;
    }
    DataDefinition& addString128(std::string simVar, std::function<void(std::string)> setter, std::function<std::string()> getter) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, "", DataTypes::string128, 0.0f, unused,
            [setter](Data::DataBlockReader& reader) {
                setter(reader.readString128());
            },
            [getter](Data::DataBlockBuilder& builder) {
                builder.addString128(getter());
            });
        size_ += 128;
        return *this;
    }
    DataDefinition& addString128(std::string simVar, std::function<void(StructType& data, std::string value)> setter, std::function<std::string(const StructType& data)> getter) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, "", DataTypes::string128, 0.0f, unused,
            [setter](StructType& data, Data::DataBlockReader& reader) { setter(data, reader.readString128()); },
            [getter](Data::DataBlockBuilder& builder, const StructType& data) { builder.addString128(getter(data)); });
        size_ += 128;
        return *this;
    }
    DataDefinition& addString128(std::string StructType::* field, std::string simVar) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, "", DataTypes::string128, 0.0f, unused,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = reader.readString128();
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addString128(data.*field);
            });
        size_ += 128;
        return *this;
    }
    DataDefinition& addString256(std::string simVar, std::function<void(std::string)> setter, std::function<std::string()> getter) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, "", DataTypes::string256, 0.0f, unused,
            [setter](Data::DataBlockReader& reader) {
                setter(reader.readString256());
            },
            [getter](Data::DataBlockBuilder& builder) {
                builder.addString256(getter());
            });
        size_ += 256;
        return *this;
    }
    DataDefinition& addString256(std::string simVar, std::function<void(StructType& data, std::string value)> setter, std::function<std::string(const StructType& data)> getter) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, "", DataTypes::string256, 0.0f, unused,
            [setter](StructType& data, Data::DataBlockReader& reader) { setter(data, reader.readString256()); },
            [getter](Data::DataBlockBuilder& builder, const StructType& data) { builder.addString256(getter(data)); });
        size_ += 256;
        return *this;
    }
    DataDefinition& addString256(std::string StructType::* field, std::string simVar) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, "", DataTypes::string256, 0.0f, unused,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = reader.readString256();
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addString256(data.*field);
            });
        size_ += 256;
        return *this;
    }
    DataDefinition& addString260(std::string simVar, std::function<void(std::string)> setter, std::function<std::string()> getter) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, "", DataTypes::string260, 0.0f, unused,
            [setter](Data::DataBlockReader& reader) {
                setter(reader.readString260());
            },
            [getter](Data::DataBlockBuilder& builder) {
                builder.addString260(getter());
            });
        size_ += 260;
        return *this;
    }
    DataDefinition& addString260(std::string simVar, std::function<void(StructType& data, std::string value)> setter, std::function<std::string(const StructType& data)> getter) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, "", DataTypes::string260, 0.0f, unused,
            [setter](StructType& data, Data::DataBlockReader& reader) { setter(data, reader.readString260()); },
            [getter](Data::DataBlockBuilder& builder, const StructType& data) { builder.addString260(getter(data)); });
        size_ += 260;
        return *this;
    }
    DataDefinition& addString260(std::string StructType::* field, std::string simVar) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, "", DataTypes::string260, 0.0f, unused,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = reader.readString260();
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addString260(data.*field);
            });
        size_ += 260;
        return *this;
    }
    DataDefinition& addStringV(std::string simVar, std::function<void(std::string)> setter, std::function<std::string()> getter) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, "", DataTypes::stringV, 0.0f, unused,
            [setter](Data::DataBlockReader& reader) {
                setter(reader.readStringV());
            },
            [getter](Data::DataBlockBuilder& builder) {
                builder.addStringV(getter());
            });
        size_ += 4; // StringV is variable length, with a minimum of 4 bytes for the length.
        return *this;
    }
    DataDefinition& addStringV(std::string simVar, std::function<void(StructType& data, std::string value)> setter, std::function<std::string(const StructType& data)> getter) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, "", DataTypes::stringV, 0.0f, unused,
            [setter](StructType& data, Data::DataBlockReader& reader) { setter(data, reader.readStringV()); },
            [getter](Data::DataBlockBuilder& builder, const StructType& data) { builder.addStringV(getter(data)); });
        size_ += 4; // StringV is variable length, with a minimum of 4 bytes for the length.
        return *this;
    }
    DataDefinition& addStringV(std::string StructType::* field, std::string simVar) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, "", DataTypes::stringV, 0.0f, unused,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = reader.readStringV();
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addStringV(data.*field);
            });
        size_ += 4; // StringV is variable length, with a minimum of 4 bytes for the length.
        return *this;
    }


    // For DataTypes::InitPosition:

    DataDefinition& addInitPosition(std::string simVar, std::string units, std::function<void(const DataTypes::InitPosition&)> setter, std::function<const DataTypes::InitPosition&()> getter) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, units, DataTypes::initPosition, 0.0f, unused,
            [setter](Data::DataBlockReader& reader) {
                setter(reader.readInitPosition());
            },
            [getter](Data::DataBlockBuilder& builder) {
                builder.addInitPosition(getter());
            });
        size_ += sizeof(DataTypes::InitPosition);
        return *this;
    }
    DataDefinition& addInitPosition(std::string simVar, std::string units, std::function<void(StructType& data, const DataTypes::InitPosition& value)> setter, std::function<DataTypes::InitPosition(const StructType& data)> getter) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, units, DataTypes::initPosition, 0.0f, unused,
            [setter](StructType& data, Data::DataBlockReader& reader) { setter(data, reader.readInitPosition()); },
            [getter](Data::DataBlockBuilder& builder, const StructType& data) { builder.addInitPosition(getter(data)); });
        size_ += sizeof(DataTypes::InitPosition);
        return *this;
    }
    DataDefinition& addInitPosition(DataTypes::InitPosition StructType::* field, std::string simVar, std::string units) {
        fields_.emplace_back(simVar, units, DataTypes::initPosition, 0.0f, unused,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = reader.readInitPosition();
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addInitPosition(data.*field);
            });
        size_ += sizeof(DataTypes::InitPosition);
        return *this;
    }


    // For DataTypes::markerState:

    DataDefinition& addMarkerState(std::string simVar, std::string units, std::function<void(const DataTypes::MarkerState&)> setter, std::function<const DataTypes::MarkerState&()> getter) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, units, DataTypes::markerState, 0.0f, unused,
            [setter](Data::DataBlockReader& reader) {
                setter(reader.readMarkerState());
            },
            [getter](Data::DataBlockBuilder& builder) {
                builder.addMarkerState(getter());
            });
        size_ += sizeof(DataTypes::MarkerState);
        return *this;
    }
    DataDefinition& addMarkerState(std::string simVar, std::string units, std::function<void(StructType& data, const DataTypes::MarkerState& value)> setter, std::function<DataTypes::MarkerState(const StructType& data)> getter) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, units, DataTypes::markerState, 0.0f, unused,
            [setter](StructType& data, Data::DataBlockReader& reader) { setter(data, reader.readMarkerState()); },
            [getter](Data::DataBlockBuilder& builder, const StructType& data) { builder.addMarkerState(getter(data)); });
        size_ += sizeof(DataTypes::MarkerState);
        return *this;
    }
    DataDefinition& addMarkerState(DataTypes::MarkerState StructType::* field, std::string simVar, std::string units) {
        fields_.emplace_back(simVar, units, DataTypes::markerState, 0.0f, unused,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = reader.readMarkerState();
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addMarkerState(data.*field);
            });
        size_ += sizeof(DataTypes::MarkerState);
        return *this;
    }


    // For DataTypes::waypoint:

    DataDefinition& addWaypoint(std::string simVar, std::string units, std::function<void(const DataTypes::Waypoint&)> setter, std::function<const DataTypes::Waypoint&()> getter) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, units, DataTypes::waypoint, 0.0f, unused,
            [setter](Data::DataBlockReader& reader) {
                setter(reader.readWaypoint());
            },
            [getter](Data::DataBlockBuilder& builder) {
                builder.addWaypoint(getter());
            });
        size_ += sizeof(DataTypes::Waypoint);
        return *this;
    }
    DataDefinition& addWaypoint(std::string simVar, std::string units, std::function<void(StructType& data, const DataTypes::Waypoint& value)> setter, std::function<DataTypes::Waypoint(const StructType& data)> getter) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, units, DataTypes::waypoint, 0.0f, unused,
            [setter](StructType& data, Data::DataBlockReader& reader) { setter(data, reader.readWaypoint()); },
            [getter](Data::DataBlockBuilder& builder, const StructType& data) { builder.addWaypoint(getter(data)); });
        size_ += sizeof(DataTypes::Waypoint);
        return *this;
    }
    DataDefinition& addWaypoint(DataTypes::Waypoint StructType::* field, std::string simVar, std::string units) {
        fields_.emplace_back(simVar, units, DataTypes::waypoint, 0.0f, unused,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = reader.readWaypoint();
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addWaypoint(data.*field);
            });
        size_ += sizeof(DataTypes::Waypoint);
        return *this;
    }


    // For DataTypes::latLonAlt:

    DataDefinition& addLatLonAlt(std::string simVar, std::string units, std::function<void(const DataTypes::LatLonAlt&)> setter, std::function<const DataTypes::LatLonAlt&()> getter) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, units, DataTypes::latLonAlt, 0.0f, unused,
            [setter](Data::DataBlockReader& reader) {
                setter(reader.readLatLonAlt());
            },
            [getter](Data::DataBlockBuilder& builder) {
                builder.addLatLonAlt(getter());
            });
        size_ += sizeof(DataTypes::LatLonAlt);
        return *this;
    }
    DataDefinition& addLatLonAlt(std::string simVar, std::string units, std::function<void(StructType& data, const DataTypes::LatLonAlt& value)> setter, std::function<DataTypes::LatLonAlt(const StructType& data)> getter) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, units, DataTypes::latLonAlt, 0.0f, unused,
            [setter](StructType& data, Data::DataBlockReader& reader) { setter(data, reader.readLatLonAlt()); },
            [getter](Data::DataBlockBuilder& builder, const StructType& data) { builder.addLatLonAlt(getter(data)); });
        size_ += sizeof(DataTypes::LatLonAlt);
        return *this;
    }
    DataDefinition& addLatLonAlt(DataTypes::LatLonAlt StructType::* field, std::string simVar, std::string units) {
        fields_.emplace_back(simVar, units, DataTypes::latLonAlt, 0.0f, unused,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = reader.readLatLonAlt();
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addLatLonAlt(data.*field);
            });
        size_ += sizeof(DataTypes::LatLonAlt);
        return *this;
    }


    // For DataTypes::xyz:

    DataDefinition& addXYZ(std::string simVar, std::string units, std::function<void(const DataTypes::XYZ&)> setter, std::function<const DataTypes::XYZ&()> getter) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, units, DataTypes::xyz, 0.0f, unused,
            [setter](Data::DataBlockReader& reader) {
                setter(reader.readXYZ());
            },
            [getter](Data::DataBlockBuilder& builder) {
                builder.addXYZ(getter());
            });
        size_ += sizeof(DataTypes::XYZ);
        return *this;
    }
    DataDefinition& addXYZ(std::string simVar, std::string units, std::function<void(StructType& data, const DataTypes::XYZ& value)> setter, std::function<const DataTypes::XYZ&(const StructType& data)> getter) {
        useMapping_ = false; // We cannot map this field directly, so we will not use the mapping.

        fields_.emplace_back(simVar, units, DataTypes::xyz, 0.0f, unused,
            [setter](StructType& data, Data::DataBlockReader& reader) { setter(data, reader.readXYZ()); },
            [getter](Data::DataBlockBuilder& builder, const StructType& data) { builder.addXYZ(getter(data)); });
        size_ += sizeof(DataTypes::XYZ);
        return *this;
    }
    DataDefinition& addXYZ(DataTypes::XYZ StructType::* field, std::string simVar, std::string units) {
        fields_.emplace_back(simVar, units, DataTypes::xyz, 0.0f, unused,
            [field](StructType& data, Data::DataBlockReader& reader) {
                data.*field = reader.readXYZ();
            },
            [field](Data::DataBlockBuilder& builder, const StructType& data) {
                builder.addXYZ(data.*field);
            });
        size_ += sizeof(DataTypes::XYZ);
        return *this;
    }


    // Marshalling and Unmarshalling:

    /**
     * Marshall the data into a DataBlockBuilder.
     * 
     * @param builder The DataBlockBuilder to write to.
     * @param data The data to marshall.
     * @param isTagged If true, the data will be written using the tagged format.
     */
    void marshall(Data::DataBlockBuilder& builder, const StructType& data, [[maybe_unused]] bool isTagged = false) const {
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


    inline static constexpr int unTagged = -1;

    /**
     * Unmarshall the data from a DataBlockReader.
     *
     * @param reader The DataBlockReader to read from.
     * @param data The data to unmarshall.
     * @param numElems The number of elements to read if tagged (default is unTagged).
     */
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


    /**
     * Unmarshall the data from a span of bytes.
     * 
     * @param msg The span of bytes containing the data.
     * @param data The data to unmarshall.
     * @param numElems The number of elements to read if tagged (default is unTagged).
     */
    void unmarshall(std::span<const uint8_t> msg, StructType& data, int numElems = unTagged) const {
        Data::DataBlockReader reader(msg);

        unmarshall(reader, data, numElems);
    }


    /**
     * Unmarshall the data from a Messages::SimObjectData message.
     *
     * @param msg The Messages::SimObjectData message containing the data.
     * @param data The data to unmarshall.
     */
    void unmarshall(const Messages::SimObjectData& msg, StructType& data) const {
        Data::DataBlockReader reader(msg);

        unmarshall(reader, data, ((msg.dwFlags & DataRequestFlags::tagged) != 0) ? msg.dwDefineCount : unTagged);
    }
};


using StatelessDataDefinition = DataDefinition<std::monostate>;

} // namespace SimConnect