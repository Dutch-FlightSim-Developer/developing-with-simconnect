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
     * Add an integer value of type `SIMCONNECT_DATATYPE_INT8` to the block.
     * 
     * @param value The integer value to add.
     * @return A reference to the current object.
     */
    DataBlockBuilder& addInt8(int8_t value) {
        return add<DataBlockBuilder>(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&value), sizeof(value)));
    }


    /**
     * Add an integer value of type `SIMCONNECT_DATATYPE_INT32` to the block.
     * 
     * @param value The integer value to add.
     * @return A reference to the current object.
     */
    DataBlockBuilder& addInt32(int32_t value) {
        return add<DataBlockBuilder>(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&value), sizeof(value)));
    }


    /**
     * Add an integer value of type `SIMCONNECT_DATATYPE_INT64` to the block.
     * 
     * @param value The integer value to add.
     * @return A reference to the current object.
     */
    DataBlockBuilder& addInt64(int64_t value) {
        return add<DataBlockBuilder>(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&value), sizeof(value)));
    }


    /**
     * Add a floating-point value of type `SIMCONNECT_DATATYPE_FLOAT32` to the block.
     * 
     * @param value The floating-point value to add.
     * @return A reference to the current object.
     */
    DataBlockBuilder& addFloat32(float value) {
        return add<DataBlockBuilder>(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&value), sizeof(value)));
    }


    /**
     * Add a floating-point value of type `SIMCONNECT_DATATYPE_FLOAT64` to the block.
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
     * Add a string value of type `SIMCONNECT_DATATYPE_STRING8` to the block.
     * **NOTE** The string is truncated to 8 characters if too long. Null termination is in that case not added.
     * 
     * @param value The string value to add.
     * @return A reference to the current object.
     */
    DataBlockBuilder& addString8(const std::string_view value) {
        return addString(value, 8);
    }


    /**
     * Add a string value of type `SIMCONNECT_DATATYPE_STRING32` to the block.
     * **NOTE** The string is truncated to 32 characters if too long. Null termination is in that case not added.
     * 
     * @param value The string value to add.
     * @return A reference to the current object.
     */
    DataBlockBuilder& addString32(const std::string_view value) {
        return addString(value, 32);
    }


    /**
     * Add a string value of type `SIMCONNECT_DATATYPE_STRING64` to the block.
     * **NOTE** The string is truncated to 64 characters if too long. Null termination is in that case not added.
     * 
     * @param value The string value to add.
     * @return A reference to the current object.
     */
    DataBlockBuilder& addString64(const std::string_view value) {
        return addString(value, 64);
    }


    /**
     * Add a string value of type `SIMCONNECT_DATATYPE_STRING128` to the block.
     * **NOTE** The string is truncated to 128 characters if too long. Null termination is in that case not added.
     * 
     * @param value The string value to add.
     * @return A reference to the current object.
     */
    DataBlockBuilder& addString128(const std::string_view value) {
        return addString(value, 128);
    }


    /**
     * Add a string value of type `SIMCONNECT_DATATYPE_STRING256` to the block.
     * **NOTE** The string is truncated to 256 characters if too long. Null termination is in that case not added.
     * 
     * @param value The string value to add.
     * @return A reference to the current object.
     */
    DataBlockBuilder& addString256(const std::string_view value) {
        return addString(value, 256);
    }


    /**
     * Add a string value of type `SIMCONNECT_DATATYPE_STRING260` to the block.
     * **NOTE** The string is truncated to 260 characters if too long. Null termination is in that case not added.
     * 
     * @param value The string value to add.
     * @return A reference to the current object.
     */
    DataBlockBuilder& addString260(const std::string_view value) {
        return addString(value, 260);
    }


    /**
     * Add a string value of type `SIMCONNECT_DATATYPE_STRINGV` to the block.
     * **NOTE** The string is always zero terminated. The length of the string is not limited.
     * 
     * @param value The string value to add.
     * @return A reference to the current object.
     */
    DataBlockBuilder& addStringV(const std::string_view value) {
        return add<DataBlockBuilder>(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(value.data()), value.size() + 1));
    }


    /**
     * Add a value of type `SIMCONNECT_DATATYPE_INITPOSITION` to the block.
     * 
     * @param value The `SIMCONNECT_DATA_INITPOSITION` value to add.
     * @return A reference to the current object.
     */
    DataBlockBuilder& addInitPosition(const SIMCONNECT_DATA_INITPOSITION& value) {
        return add<DataBlockBuilder>(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&value), sizeof(value)));
    }


    /**
     * Add a value of type `SIMCONNECT_DATA_INITPOSITION` to the block, provided in its components.
     * 
     * @param pos A `SIMCONNECT_DATA_LATLONALT` value to add.
     * @param pitchBankHeading A `SIMCONNECT_DATA_PBH` value for pitch, bank, and heading.
     * @param onGround Indicates if the position is on the ground (default is true).
     * @param airspeed The airspeed value (default is 0).
     * @return A reference to the current object.
     */
    DataBlockBuilder& addInitPosition(const SIMCONNECT_DATA_LATLONALT& pos,
                                      const SIMCONNECT_DATA_PBH& pitchBankHeading,
                                      bool onGround = true, int32_t airspeed = 0)
    {
        return addLatLonAlt(pos)
            .addFloat64(pitchBankHeading.Pitch).addFloat64(pitchBankHeading.Bank).addFloat64(pitchBankHeading.Heading)
            .addInt32(onGround ? 1 : 0)
            .addInt32(airspeed);
    }


    /**
     * Add a value of type `SIMCONNECT_DATA_MARKERSTATE` to the block.
     * 
     * @param value The `SIMCONNECT_DATA_MARKERSTATE` value to add.
     * @return A reference to the current object.
     */
    DataBlockBuilder& addMarkerState(const SIMCONNECT_DATA_MARKERSTATE& value) {
        return add<DataBlockBuilder>(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&value), sizeof(value)));
    }


    /**
     * Add a value of type `SIMCONNECT_DATA_WAYPOINT` to the block.
     * 
     * @param value The `SIMCONNECT_DATA_WAYPOINT` value to add.
     * @return A reference to the current object.
     */
    DataBlockBuilder& addWaypoint(const SIMCONNECT_DATA_WAYPOINT& value) {
        return add<DataBlockBuilder>(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&value), sizeof(value)));
    }


    /**
     * Add a value of type `SIMCONNECT_DATA_LATLONALT` to the block.
     * 
     * @param value The `SIMCONNECT_DATA_LATLONALT` value to add.
     * @return A reference to the current object.
     */
    DataBlockBuilder& addLatLonAlt(const SIMCONNECT_DATA_LATLONALT& value) {
        return add<DataBlockBuilder>(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&value), sizeof(value)));
    }


    /**
     * Add a value of type `SIMCONNECT_DATA_LATLONALT` to the block, provided in its components.
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
     * Add a value of type `SIMCONNECT_DATA_XYZ` to the block.
     * 
     * @param value The `SIMCONNECT_DATA_XYZ` value to add.
     * @return A reference to the current object.
     */
    DataBlockBuilder& addXYZ(const SIMCONNECT_DATA_XYZ& value) {
        return add<DataBlockBuilder>(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&value), sizeof(value)));
    }


    /**
     * Add a value of type `SIMCONNECT_DATA_XYZ` to the block, provided in its components.
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