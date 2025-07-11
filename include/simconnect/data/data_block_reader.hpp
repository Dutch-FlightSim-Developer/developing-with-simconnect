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


public:
    DataBlockReader() = default;
    DataBlockReader(std::span<const uint8_t> data)
        : DataBlock(data)
    {}
    DataBlockReader(const SIMCONNECT_RECV_SIMOBJECT_DATA& msg)
        : DataBlock(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&(msg.dwData)), msg.dwSize - (10*sizeof(DWORD))))
    {}
    ~DataBlockReader() = default;

    DataBlockReader(const DataBlockReader&) = default;
    DataBlockReader(DataBlockReader&&) = default;
    DataBlockReader& operator=(const DataBlockReader&) = default;
    DataBlockReader& operator=(DataBlockReader&&) = default;


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
     * Read a block of data as a pointer to it, checking if the read is within bounds.
     * 
     * @param size The size of the block.
     * @return A pointer to the data.
     */
    template <typename T>
    const T* readPointer(unsigned int size) {
        if (next_ + size > this->size()) {
            throw std::out_of_range("Attempt to read beyond the end of the data block.");
        }
        const T* ptr = reinterpret_cast<const T*>(getSpan(next_, size).data());
        next_ += size;
        return ptr;
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