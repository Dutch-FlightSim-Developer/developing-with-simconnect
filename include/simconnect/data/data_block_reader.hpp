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

#include <cstddef>
#include <cstdint>

#include <simconnect/simconnect.hpp>
#include <simconnect/data/data_block.hpp>


namespace SimConnect::Data {

/**
 * The DataBlockReader class is a specialized class for extracting data from untagged data blocks.
 */
class DataBlockReader : public DataBlock
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


    inline static constexpr size_t headerSize{ 10 * sizeof(unsigned long) }; // Size of the Messages::SimObjectData header.


public:
    DataBlockReader() = default;
    DataBlockReader(std::span<const uint8_t> data)
        : DataBlock(data)
    {}
    DataBlockReader(const Messages::SimObjectData& msg)
        : DataBlock(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&(msg.dwData)), msg.dwSize - headerSize))
    {}
    ~DataBlockReader() = default;

    DataBlockReader(const DataBlockReader&) = default;
    DataBlockReader(DataBlockReader&&) = default;
    DataBlockReader& operator=(const DataBlockReader&) = default;
    DataBlockReader& operator=(DataBlockReader&&) = default;


    /**
     * Read a value of type `DataTypes::Int8` from the block.
     * 
     * @return The read integer value.
     */
    int8_t readInt8() {
        return read<int8_t>();
    }


    /**
     * Read a value of type `DataTypes::Int32` from the block.
     * 
     * @return The read integer value.
     */
    int32_t readInt32() {
        return read<int32_t>();
    }


    /**
     * Read a value of type `DataTypes::Int64` from the block.
     * 
     * @return The read integer value.
     */
    int64_t readInt64() {
        return read<int64_t>();
    }


    /**
     * Read a value of type `DataTypes::Float32` from the block.
     * 
     * @return The read floating-point value.
     */
    float readFloat32() {
        return read<float>();
    }


    /**
     * Read a value of type `DataTypes::Float64` from the block.
     * 
     * @return The read floating-point value.
     */
    double readFloat64() {
        return read<double>();
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


    /**
     * Read a string value of type `DataTypes::String8` from the block.
     * 
     * @return The read string value.
     */
    std::string readString8() {
        return readString(8);
    }


    /**
     * Read a string value of type `DataTypes::String32` from the block.
     * 
     * @return The read string value.
     */
    std::string readString32() {
        return readString(32);
    }


    /**
     * Read a string value of type `DataTypes::String64` from the block.
     * 
     * @return The read string value.
     */
    std::string readString64() {
        return readString(64);
    }


    /**
     * Read a string value of type `DataTypes::String128` from the block.
     * 
     * @return The read string value.
     */
    std::string readString128() {
        return readString(128);
    }
    

    /**
     * Read a string value of type `DataTypes::String256` from the block.
     * 
     * @return The read string value.
     */
    std::string readString256() {
        return readString(256);
    }


    /**
     * Read a string value of type `DataTypes::String260` from the block.
     * 
     * @return The read string value.
     */
    std::string readString260() {
        return readString(260);
    }


    /**
     * Read a string value of type `DataTypes::StringV` from the block.
     */
    std::string readStringV() {
        auto value = getSpan(next_, size() - next_);
        auto end = std::ranges::find(value, 0);
        next_ += end - value.begin() + 1; // +1 for the null terminator

        return std::string(reinterpret_cast<const char*>(value.data()), end - value.begin());
    }


    /**
     * Read a block of data as a pointer to it, checking if the read is within bounds.
     * 
     * @param size The size of the block.
     * @return A pointer to the data.
     */
    template <typename T>
    const T* readPointer(size_t size) {
        if (next_ + size > this->size()) {
            throw std::out_of_range("Attempt to read beyond the end of the data block.");
        }
        const T* ptr = reinterpret_cast<const T*>(getSpan(next_, size).data());
        next_ += size;
        return ptr;
    }


    /**
     * Read a value of type `DataTypes::InitPosition` from the block.
     * 
     * @return The read `DataTypes::InitPosition` value.
     */
    DataTypes::InitPosition readInitPosition() {
        return read<DataTypes::InitPosition>();
    }


    /**
     * Read a value of type `DataTypes::MarkerState` from the block.
     * 
     * @return The read `DataTypes::MarkerState` value.
     */
    DataTypes::MarkerState readMarkerState() {
        return read<DataTypes::MarkerState>();
    }


    /**
     * Read a value of type `DataTypes::Waypoint` from the block.
     * 
     * @return The read `DataTypes::Waypoint` value.
     */
    DataTypes::Waypoint readWaypoint() {
        return read<DataTypes::Waypoint>();
    }


    /**
     * Read a value of type `DataTypes::LatLonAlt` from the block.
     * 
     * @return The read `DataTypes::LatLonAlt` value.
     */
    DataTypes::LatLonAlt readLatLonAlt() {
        return read<DataTypes::LatLonAlt>();
    }


    /**
     * Read a value of type `DataTypes::XYZ` from the block.
     * 
     * @return The read `DataTypes::XYZ` value.
     */
    DataTypes::XYZ readXYZ() {
        return read<DataTypes::XYZ>();
    }
};

} // namespace SimConnect::Data