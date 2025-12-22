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

#include <ranges>


#include <simconnect/simconnect.hpp>
#include <simconnect/data/data_block.hpp>


namespace SimConnect::Data {
    
/**
 * The DataBlockBuilder class is a specialized builder for creating untagged data blocks.
 */
class DataBlockBuilder : public DataBlock
{
public:
    DataBlockBuilder() = default;
    DataBlockBuilder(unsigned int size) : DataBlock(size) {}
    ~DataBlockBuilder() = default;

    DataBlockBuilder(const DataBlockBuilder&) = default;
    DataBlockBuilder(DataBlockBuilder&&) = default;
    DataBlockBuilder& operator=(const DataBlockBuilder&) = default;
    DataBlockBuilder& operator=(DataBlockBuilder&&) = default;


    /**
     * Add an integer value of type `DataTypes::Int8` to the block.
     * 
     * @param value The integer value to add.
     * @return A reference to the current object.
     */
    DataBlockBuilder& addInt8(int8_t value) {
        return add<DataBlockBuilder>(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&value), sizeof(value)));
    }


    /**
     * Add an integer value of type `DataTypes::Int32` to the block.
     * 
     * @param value The integer value to add.
     * @return A reference to the current object.
     */
    DataBlockBuilder& addInt32(int32_t value) {
        return add<DataBlockBuilder>(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&value), sizeof(value)));
    }


    /**
     * Add an integer value of type `DataTypes::Int64` to the block.
     * 
     * @param value The integer value to add.
     * @return A reference to the current object.
     */
    DataBlockBuilder& addInt64(int64_t value) {
        return add<DataBlockBuilder>(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&value), sizeof(value)));
    }


    /**
     * Add a floating-point value of type `DataTypes::Float32` to the block.
     * 
     * @param value The floating-point value to add.
     * @return A reference to the current object.
     */
    DataBlockBuilder& addFloat32(float value) {
        return add<DataBlockBuilder>(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&value), sizeof(value)));
    }


    /**
     * Add a floating-point value of type `DataTypes::Float64` to the block.
     * 
     * @param value The floating-point value to add.
     * @return A reference to the current object.
     */
    DataBlockBuilder& addFloat64(double value) {
        return add<DataBlockBuilder>(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&value), sizeof(value)));
    }


    /**
     * Add a string value to the block. The value is truncated or padded (with zeros) to the specified size.
     * 
     * @param value The string value to add.
     * @param size The size of the string to add.
     * @return A reference to the current object.
     */
    DataBlockBuilder& addString(const std::string_view value, size_t size) {
        if (value.size() < size) {
            add<DataBlockBuilder>(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(value.data()), value.size()));
            addPadding<DataBlockBuilder>(size - value.size());
        }
        else {
            add<DataBlockBuilder>(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(value.data()), size));
        }
        return *this;
    }


    /**
     * Add a string value of type `DataTypes::String8` to the block.
     * **NOTE** The string is truncated to 8 characters if too long. Null termination is in that case not added.
     * 
     * @param value The string value to add.
     * @return A reference to the current object.
     */
    DataBlockBuilder& addString8(const std::string_view value) {
        return addString(value, 8);
    }


    /**
     * Add a string value of type `DataTypes::String32` to the block.
     * **NOTE** The string is truncated to 32 characters if too long. Null termination is in that case not added.
     * 
     * @param value The string value to add.
     * @return A reference to the current object.
     */
    DataBlockBuilder& addString32(const std::string_view value) {
        return addString(value, 32);
    }


    /**
     * Add a string value of type `DataTypes::String64` to the block.
     * **NOTE** The string is truncated to 64 characters if too long. Null termination is in that case not added.
     * 
     * @param value The string value to add.
     * @return A reference to the current object.
     */
    DataBlockBuilder& addString64(const std::string_view value) {
        return addString(value, 64);
    }


    /**
     * Add a string value of type `DataTypes::String128` to the block.
     * **NOTE** The string is truncated to 128 characters if too long. Null termination is in that case not added.
     * 
     * @param value The string value to add.
     * @return A reference to the current object.
     */
    DataBlockBuilder& addString128(const std::string_view value) {
        return addString(value, 128);
    }


    /**
     * Add a string value of type `DataTypes::String256` to the block.
     * **NOTE** The string is truncated to 256 characters if too long. Null termination is in that case not added.
     * 
     * @param value The string value to add.
     * @return A reference to the current object.
     */
    DataBlockBuilder& addString256(const std::string_view value) {
        return addString(value, 256);
    }


    /**
     * Add a string value of type `DataTypes::String260` to the block.
     * **NOTE** The string is truncated to 260 characters if too long. Null termination is in that case not added.
     * 
     * @param value The string value to add.
     * @return A reference to the current object.
     */
    DataBlockBuilder& addString260(const std::string_view value) {
        return addString(value, 260);
    }


    /**
     * Add a string value of type `DataTypes::StringV` to the block.
     * **NOTE** The string is always zero terminated. The length of the string is not limited.
     * 
     * @param value The string value to add.
     * @return A reference to the current object.
     */
    DataBlockBuilder& addStringV(const std::string_view value) {
        return add<DataBlockBuilder>(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(value.data()), value.size() + 1));
    }


    /**
     * Add a value of type `DataTypes::InitPosition` to the block.
     * 
     * @param value The `DataTypes::InitPosition` value to add.
     * @return A reference to the current object.
     */
    DataBlockBuilder& addInitPosition(const DataTypes::InitPosition& value) {
        return add<DataBlockBuilder>(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&value), sizeof(value)));
    }


    /**
     * Add a value of type `DataTypes::InitPosition` to the block, provided in its components.
     * 
     * @param pos A `DataTypes::LatLonAlt` value to add.
     * @param pitchBankHeading A `DataTypes::PitchBankHeading` value for pitch, bank, and heading.
     * @param onGround Indicates if the position is on the ground (default is true).
     * @param airspeed The airspeed value (default is 0).
     * @return A reference to the current object.
     */
    DataBlockBuilder& addInitPosition(const DataTypes::LatLonAlt& pos,
                                      const DataTypes::PitchBankHeading& pitchBankHeading,
                                      bool onGround = true, int32_t airspeed = 0)
    {
        return addLatLonAlt(pos)
            .addFloat64(pitchBankHeading.Pitch).addFloat64(pitchBankHeading.Bank).addFloat64(pitchBankHeading.Heading)
            .addInt32(onGround ? 1 : 0)
            .addInt32(airspeed);
    }


    /**
     * Add a value of type `DataTypes::MarkerState` to the block.
     * 
     * @param value The `DataTypes::MarkerState` value to add.
     * @return A reference to the current object.
     */
    DataBlockBuilder& addMarkerState(const DataTypes::MarkerState& value) {
        return add<DataBlockBuilder>(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&value), sizeof(value)));
    }


    /**
     * Add a value of type `DataTypes::Waypoint` to the block.
     * 
     * @param value The `DataTypes::Waypoint` value to add.
     * @return A reference to the current object.
     */
    DataBlockBuilder& addWaypoint(const DataTypes::Waypoint& value) {
        return add<DataBlockBuilder>(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&value), sizeof(value)));
    }


    /**
     * Add a value of type `DataTypes::LatLonAlt` to the block.
     * 
     * @param value The `DataTypes::LatLonAlt` value to add.
     * @return A reference to the current object.
     */
    DataBlockBuilder& addLatLonAlt(const DataTypes::LatLonAlt& value) {
        return add<DataBlockBuilder>(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&value), sizeof(value)));
    }


    /**
     * Add a value of type `DataTypes::LatLonAlt` to the block, provided in its components.
     * 
     * @param lat The latitude value.
     * @param lon The longitude value.
     * @param alt The altitude value.
     * @return A reference to the current object.
     */
    DataBlockBuilder& addLatLonAlt(double lat, double lon, double alt) {
        return addFloat64(lat).addFloat64(lon).addFloat64(alt);
    }


    /**
     * Add a value of type `DataTypes::XYZ` to the block.
     * 
     * @param value The `DataTypes::XYZ` value to add.
     * @return A reference to the current object.
     */
    DataBlockBuilder& addXYZ(const DataTypes::XYZ& value) {
        return add<DataBlockBuilder>(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&value), sizeof(value)));
    }


    /**
     * Add a value of type `DataTypes::XYZ` to the block, provided in its components.
     * 
     * @param x The x-coordinate value.
     * @param y The y-coordinate value.
     * @param z The z-coordinate value.
     * @return A reference to the current object.
     */
    DataBlockBuilder& addXYZ(double x, double y, double z) {
        return addFloat64(x).addFloat64(y).addFloat64(z);
    }

};

} // namespace SimConnect::Data