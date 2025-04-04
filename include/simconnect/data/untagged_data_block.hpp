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
 * The UntaggedDataBlockBuilder class is a specialized builder for creating untagged data blocks.
 */
class UntaggedDataBlockBuilder : public DataBlock
{
    UntaggedDataBlockBuilder& addString(std::string value, size_t size) {
        if (value.size() < size) {
            add<UntaggedDataBlockBuilder>(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(value.data()), value.size()));
            addPadding<UntaggedDataBlockBuilder>(size - value.size());
        }
        else {
            add<UntaggedDataBlockBuilder>(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(value.data()), size));
        }
        return *this;
    }


public:
    UntaggedDataBlockBuilder() = default;
    UntaggedDataBlockBuilder(unsigned int size) : DataBlock(size) {}
    ~UntaggedDataBlockBuilder() = default;

    UntaggedDataBlockBuilder(const UntaggedDataBlockBuilder&) = default;
    UntaggedDataBlockBuilder(UntaggedDataBlockBuilder&&) = default;
    UntaggedDataBlockBuilder& operator=(const UntaggedDataBlockBuilder&) = default;
    UntaggedDataBlockBuilder& operator=(UntaggedDataBlockBuilder&&) = default;


    /**
     * Add an integer value of type `SIMCONNECT_DATATYPE_INT32` to the block.
     * 
     * @param value The integer value to add.
     * @return A reference to the current object.
     */
    UntaggedDataBlockBuilder& addInt32(int32_t value) {
        return add<UntaggedDataBlockBuilder>(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&value), sizeof(value)));
    }


    /**
     * Add an integer value of type `SIMCONNECT_DATATYPE_INT64` to the block.
     * 
     * @param value The integer value to add.
     * @return A reference to the current object.
     */
    UntaggedDataBlockBuilder& addInt64(int64_t value) {
        return add<UntaggedDataBlockBuilder>(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&value), sizeof(value)));
    }


    /**
     * Add a floating-point value of type `SIMCONNECT_DATATYPE_FLOAT32` to the block.
     * 
     * @param value The floating-point value to add.
     * @return A reference to the current object.
     */
    UntaggedDataBlockBuilder& addFloat32(float value) {
        return add<UntaggedDataBlockBuilder>(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&value), sizeof(value)));
    }


    /**
     * Add a floating-point value of type `SIMCONNECT_DATATYPE_FLOAT64` to the block.
     * 
     * @param value The floating-point value to add.
     * @return A reference to the current object.
     */
    UntaggedDataBlockBuilder& addFloat64(double value) {
        return add<UntaggedDataBlockBuilder>(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&value), sizeof(value)));
    }


    /**
     * Add a string value of type `SIMCONNECT_DATATYPE_STRING8` to the block.
     * **NOTE** The string is truncated to 8 characters if too long. Null termination is in that case not added.
     * 
     * @param value The string value to add.
     * @return A reference to the current object.
     */
    UntaggedDataBlockBuilder& addString8(std::string value) {
        return addString(value, 8);
    }


    /**
     * Add a string value of type `SIMCONNECT_DATATYPE_STRING32` to the block.
     * **NOTE** The string is truncated to 32 characters if too long. Null termination is in that case not added.
     * 
     * @param value The string value to add.
     * @return A reference to the current object.
     */
    UntaggedDataBlockBuilder& addString32(std::string value) {
        return addString(value, 32);
    }


    /**
     * Add a string value of type `SIMCONNECT_DATATYPE_STRING64` to the block.
     * **NOTE** The string is truncated to 64 characters if too long. Null termination is in that case not added.
     * 
     * @param value The string value to add.
     * @return A reference to the current object.
     */
    UntaggedDataBlockBuilder& addString64(std::string value) {
        return addString(value, 64);
    }


    /**
     * Add a string value of type `SIMCONNECT_DATATYPE_STRING128` to the block.
     * **NOTE** The string is truncated to 128 characters if too long. Null termination is in that case not added.
     * 
     * @param value The string value to add.
     * @return A reference to the current object.
     */
    UntaggedDataBlockBuilder& addString128(std::string value) {
        return addString(value, 128);
    }


    /**
     * Add a string value of type `SIMCONNECT_DATATYPE_STRING256` to the block.
     * **NOTE** The string is truncated to 256 characters if too long. Null termination is in that case not added.
     * 
     * @param value The string value to add.
     * @return A reference to the current object.
     */
    UntaggedDataBlockBuilder& addString256(std::string value) {
        return addString(value, 256);
    }


    /**
     * Add a string value of type `SIMCONNECT_DATATYPE_STRING260` to the block.
     * **NOTE** The string is truncated to 260 characters if too long. Null termination is in that case not added.
     * 
     * @param value The string value to add.
     * @return A reference to the current object.
     */
    UntaggedDataBlockBuilder& addString260(std::string value) {
        return addString(value, 260);
    }


    /**
     * Add a string value of type `SIMCONNECT_DATATYPE_STRINGV` to the block.
     * **NOTE** The string is always zero terminated. The length of the string is not limited.
     * 
     * @param value The string value to add.
     * @return A reference to the current object.
     */
    UntaggedDataBlockBuilder& addStringV(std::string value) {
        return add<UntaggedDataBlockBuilder>(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(value.c_str()), value.size() + 1));
    }


    /**
     * Add a value of type `SIMCONNECT_DATATYPE_INITPOSITION` to the block.
     * 
     * @param value The `SIMCONNECT_DATA_INITPOSITION` value to add.
     * @return A reference to the current object.
     */
    UntaggedDataBlockBuilder& addInitPosition(const SIMCONNECT_DATA_INITPOSITION& value) {
        return add<UntaggedDataBlockBuilder>(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&value), sizeof(value)));
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
    UntaggedDataBlockBuilder& addInitPosition(const SIMCONNECT_DATA_LATLONALT& pos,
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
    UntaggedDataBlockBuilder& addMarkerState(const SIMCONNECT_DATA_MARKERSTATE& value) {
        return add<UntaggedDataBlockBuilder>(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&value), sizeof(value)));
    }


    /**
     * Add a value of type `SIMCONNECT_DATA_WAYPOINT` to the block.
     * 
     * @param value The `SIMCONNECT_DATA_WAYPOINT` value to add.
     * @return A reference to the current object.
     */
    UntaggedDataBlockBuilder& addWaypoint(const SIMCONNECT_DATA_WAYPOINT& value) {
        return add<UntaggedDataBlockBuilder>(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&value), sizeof(value)));
    }


    /**
     * Add a value of type `SIMCONNECT_DATA_LATLONALT` to the block.
     * 
     * @param value The `SIMCONNECT_DATA_LATLONALT` value to add.
     * @return A reference to the current object.
     */
    UntaggedDataBlockBuilder& addLatLonAlt(const SIMCONNECT_DATA_LATLONALT& value) {
        return add<UntaggedDataBlockBuilder>(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&value), sizeof(value)));
    }


    /**
     * Add a value of type `SIMCONNECT_DATA_LATLONALT` to the block, provided in its components.
     * 
     * @param lat The latitude value.
     * @param lon The longitude value.
     * @param alt The altitude value.
     * @return A reference to the current object.
     */
    UntaggedDataBlockBuilder& addLatLonAlt(double lat, double lon, double alt) {
        return addFloat64(lat).addFloat64(lon).addFloat64(alt);
    }


    /**
     * Add a value of type `SIMCONNECT_DATA_XYZ` to the block.
     * 
     * @param value The `SIMCONNECT_DATA_XYZ` value to add.
     * @return A reference to the current object.
     */
    UntaggedDataBlockBuilder& addXYZ(const SIMCONNECT_DATA_XYZ& value) {
        return add<UntaggedDataBlockBuilder>(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&value), sizeof(value)));
    }


    /**
     * Add a value of type `SIMCONNECT_DATA_XYZ` to the block, provided in its components.
     * 
     * @param x The x-coordinate value.
     * @param y The y-coordinate value.
     * @param z The z-coordinate value.
     * @return A reference to the current object.
     */
    UntaggedDataBlockBuilder& addXYZ(double x, double y, double z) {
        return addFloat64(x).addFloat64(y).addFloat64(z);
    }

};


/**
 * The UntaggedDataBlockReader class is a specialized class for extracting data from untagged data blocks.
 */
class UntaggedDataBlockReader : public DataBlock
{
    size_t next_{0};


    /**
     * Read a value from the block.
     * 
     * @param size The size of the value to read.
     * @return The read value.
     * @throws std::out_of_range If the size exceeds the available data.
     */
    template<typename T>
    T read() {
        T value;
        std::memcpy(&value, getSpan(next_, sizeof(T)).data(), sizeof(T));
        next_ += sizeof(T);
        return value;
    }


    /**
     * Read a fixed size string from the block.
     * 
     * @param size The size of the string to read.
     * @return The read string value.
     */
    std::string readString(size_t size) {
        auto value = getSpan(next_, size);
        next_ += size;
        auto end = std::ranges::find(value, 0);
        return std::string(reinterpret_cast<const char*>(value.data()), end - value.begin());
    }


public:
    UntaggedDataBlockReader() = default;
    UntaggedDataBlockReader(std::span<const uint8_t> data)
        : DataBlock(data)
    {}
    UntaggedDataBlockReader(const SIMCONNECT_RECV_SIMOBJECT_DATA& msg)
        : DataBlock(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&msg), msg.dwDefineCount * sizeof(DWORD)))
    {}
    ~UntaggedDataBlockReader() = default;

    UntaggedDataBlockReader(const UntaggedDataBlockReader&) = default;
    UntaggedDataBlockReader(UntaggedDataBlockReader&&) = default;
    UntaggedDataBlockReader& operator=(const UntaggedDataBlockReader&) = default;
    UntaggedDataBlockReader& operator=(UntaggedDataBlockReader&&) = default;


    /**
     * Read a value of type `SIMCONNECT_DATATYPE_INT32` from the block.
     * 
     * @return The read integer value.
     */
    int32_t readInt32() {
        return read<int32_t>();
    }


    /**
     * Read a value of type `SIMCONNECT_DATATYPE_INT64` from the block.
     * 
     * @return The read integer value.
     */
    int64_t readInt64() {
        return read<int64_t>();
    }


    /**
     * Read a value of type `SIMCONNECT_DATATYPE_FLOAT32` from the block.
     * 
     * @return The read floating-point value.
     */
    float readFloat32() {
        return read<float>();
    }


    /**
     * Read a value of type `SIMCONNECT_DATATYPE_FLOAT64` from the block.
     * 
     * @return The read floating-point value.
     */
    double readFloat64() {
        return read<double>();
    }


    /**
     * Read a string value of type `SIMCONNECT_DATATYPE_STRING8` from the block.
     * 
     * @return The read string value.
     */
    std::string readString8() {
        return readString(8);
    }


    /**
     * Read a string value of type `SIMCONNECT_DATATYPE_STRING32` from the block.
     * 
     * @return The read string value.
     */
    std::string readString32() {
        return readString(32);
    }


    /**
     * Read a string value of type `SIMCONNECT_DATATYPE_STRING64` from the block.
     * 
     * @return The read string value.
     */
    std::string readString64() {
        return readString(64);
    }


    /**
     * Read a string value of type `SIMCONNECT_DATATYPE_STRING128` from the block.
     * 
     * @return The read string value.
     */
    std::string readString128() {
        return readString(128);
    }
    

    /**
     * Read a string value of type `SIMCONNECT_DATATYPE_STRING256` from the block.
     * 
     * @return The read string value.
     */
    std::string readString256() {
        return readString(256);
    }


    /**
     * Read a string value of type `SIMCONNECT_DATATYPE_STRING260` from the block.
     * 
     * @return The read string value.
     */
    std::string readString260() {
        return readString(260);
    }


    /**
     * Read a string value of type `SIMCONNECT_DATATYPE_STRINGV` from the block.
     */
    std::string readStringV() {
        auto value = getSpan(next_, size() - next_);
        auto end = std::ranges::find(value, 0);
        next_ += end - value.begin() + 1; // +1 for the null terminator

        return std::string(reinterpret_cast<const char*>(value.data()), end - value.begin());
    }


    /**
     * Read a value of type `SIMCONNECT_DATATYPE_INITPOSITION` from the block.
     * 
     * @return The read `SIMCONNECT_DATA_INITPOSITION` value.
     */
    SIMCONNECT_DATA_INITPOSITION readInitPosition() {
        return read<SIMCONNECT_DATA_INITPOSITION>();
    }


    /**
     * Read a value of type `SIMCONNECT_DATATYPE_MARKERSTATE` from the block.
     * 
     * @return The read `SIMCONNECT_DATA_MARKERSTATE` value.
     */
    SIMCONNECT_DATA_MARKERSTATE readMarkerState() {
        return read<SIMCONNECT_DATA_MARKERSTATE>();
    }


    /**
     * Read a value of type `SIMCONNECT_DATATYPE_WAYPOINT` from the block.
     * 
     * @return The read `SIMCONNECT_DATA_WAYPOINT` value.
     */
    SIMCONNECT_DATA_WAYPOINT readWaypoint() {
        return read<SIMCONNECT_DATA_WAYPOINT>();
    }


    /**
     * Read a value of type `SIMCONNECT_DATATYPE_LATLONALT` from the block.
     * 
     * @return The read `SIMCONNECT_DATA_LATLONALT` value.
     */
    SIMCONNECT_DATA_LATLONALT readLatLonAlt() {
        return read<SIMCONNECT_DATA_LATLONALT>();
    }


    /**
     * Read a value of type `SIMCONNECT_DATATYPE_XYZ` from the block.
     * 
     * @return The read `SIMCONNECT_DATA_XYZ` value.
     */
    SIMCONNECT_DATA_XYZ readXYZ() {
        return read<SIMCONNECT_DATA_XYZ>();
    }
};

} // namespace SimConnect::Data