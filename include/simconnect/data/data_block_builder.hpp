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

#include <simconnect.hpp>

#include <cstdint>
#include <cstring>

#include <span>
#include <vector>
#include <string>
#include <algorithm>


namespace SimConnect::Data {

class DataBlockBuilder
{
    struct DataField
    {
        std::string name;
        SIMCONNECT_DATATYPE type;
        size_t offset;
        size_t size;
    };
    std::vector<DataField> dataFields_;
    std::vector<uint8_t> dataBlock_;

public:
    DataBlockBuilder() = default;
    DataBlockBuilder(unsigned int size) : dataBlock_(size) {}


    std::span<const uint8_t> dataBlock() const {
        return std::span<const uint8_t>(dataBlock_.data(), dataBlock_.size());
    }

    /**
     * Add a value of type SIMCONNECT_DATATYPE_INT32 to the data block.
     * 
     * @param value The value to add.
     * @return A reference to this DataDefinitionBuilder instance for chaining.
     */
    DataBlockBuilder& addInt32(int32_t value, std::string name = "int32") {
        auto pos = dataBlock_.size();
        dataBlock_.resize(pos + sizeof(int32_t));
        std::memcpy(&dataBlock_[pos], &value, sizeof(int32_t));

        dataFields_.push_back(DataField{ std::move(name), SIMCONNECT_DATATYPE_INT32, pos, sizeof(int32_t) });

        return *this;
    }


    /**
     * Add a value of type SIMCONNECT_DATATYPE_INT64 to the data block.
     * 
     * @param value The value to add.
     * @return A reference to this DataDefinitionBuilder instance for chaining.
     */
    DataBlockBuilder& addInt64(int64_t value, std::string name = "int64") {
        auto pos = dataBlock_.size();
        dataBlock_.resize(pos + sizeof(int64_t));
        std::memcpy(&dataBlock_[pos], &value, sizeof(int64_t));

        dataFields_.push_back(DataField{ std::move(name), SIMCONNECT_DATATYPE_INT64, pos, sizeof(int64_t) });

        return *this;
    }


    /**
     * Add a value of type SIMCONNECT_DATATYPE_FLOAT32 to the data block.
     * 
     * @param value The value to add.
     * @return A reference to this DataDefinitionBuilder instance for chaining.
     */
    DataBlockBuilder& addFloat32(float value, std::string name = "float32") {
        auto pos = dataBlock_.size();
        dataBlock_.resize(pos + sizeof(float));
        std::memcpy(&dataBlock_[pos], &value, sizeof(float));

        dataFields_.push_back(DataField{ std::move(name), SIMCONNECT_DATATYPE_FLOAT32, pos, sizeof(float) });

        return *this;
    }


    /**
     * Add a value of type SIMCONNECT_DATATYPE_FLOAT64 to the data block.
     * 
     * @param value The value to add.
     * @return A reference to this DataDefinitionBuilder instance for chaining.
     */
    DataBlockBuilder& addFloat64(double value, std::string name = "float64") {
        auto pos = dataBlock_.size();
        dataBlock_.resize(pos + sizeof(double));
        std::memcpy(&dataBlock_[pos], &value, sizeof(double));

        dataFields_.push_back(DataField{ std::move(name), SIMCONNECT_DATATYPE_FLOAT64, pos, sizeof(double) });

        return *this;
    }


    /**
     * Convert a length to the corresponding SIMCONNECT_DATATYPE_STRINGx type.
     * 
     * @param length The length of the string.
     * @return The corresponding SIMCONNECT_DATATYPE_STRINGx type.
     * @throws std::invalid_argument if the length is invalid.
     */
    constexpr SIMCONNECT_DATATYPE stringLengthToType(size_t length) {
        switch (length) {
            case 8:   return SIMCONNECT_DATATYPE_STRING8;
            case 32:  return SIMCONNECT_DATATYPE_STRING32;
            case 64:  return SIMCONNECT_DATATYPE_STRING64;
            case 128: return SIMCONNECT_DATATYPE_STRING128;
            case 256: return SIMCONNECT_DATATYPE_STRING256;
            case 260: return SIMCONNECT_DATATYPE_STRING260;
            default:  
                throw std::invalid_argument(
                    "Invalid string length. Valid lengths are 8, 32, 64, 128, 256, and 260.");
        }
    }
 
    
    /**
     * Add a value of type SIMCONNECT_DATATYPE_STRING with a specified length to the data block.
     * 
     * @param value The string value to add.
     * @param length The length of the string (valid values: 8, 32, 64, 128, 256, 260).
     * @param name The name of the field (default is "string").
     * @return A reference to this DataDefinitionBuilder instance for chaining.
     */
    DataBlockBuilder& addString(const std::string& value, size_t length, std::string name = "string") {
        SIMCONNECT_DATATYPE type = stringLengthToType(length);

        auto pos = dataBlock_.size();
        dataBlock_.resize(pos + length);
    
        std::memset(&dataBlock_[pos], 0, length);
        auto copyLen = min(value.size(), length - 1);
        std::memcpy(&dataBlock_[pos], value.data(), copyLen);
    
        // Ensure explicit null-termination (already guaranteed by memset, but explicit for clarity)
        dataBlock_[pos + length - 1] = '\0';
    
        dataFields_.push_back(DataField{ std::move(name), type, pos, length });
    
        return *this;
    }


    /**
     * Add a value of type SIMCONNECT_DATATYPE_STRINGV to the data block.
     * 
     * @param value The string value to add.
     * @param name The name of the field (default is "string").
     * @return A reference to this DataDefinitionBuilder instance for chaining.
     */
    DataBlockBuilder& addStringV(const std::string& value, std::string name = "string") {
        size_t length = value.size() + 1;
        auto pos = dataBlock_.size();
        dataBlock_.resize(pos + length);
    
        std::memset(&dataBlock_[pos], 0, length);
        std::memcpy(&dataBlock_[pos], value.data(), value.size());

        dataBlock_[pos + length - 1] = '\0';
    
        dataFields_.push_back(DataField{ std::move(name), SIMCONNECT_DATATYPE_STRINGV, pos, length });
    
        return *this;
    }
};

} // namespace SimConnect::Data