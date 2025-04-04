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
#include <span>
#include <vector>


namespace SimConnect::Data {

class DataBlock
{
    std::vector<uint8_t> dataBlock_;

protected:
    /**
     * Add a block of data to the data block.
     * 
     * @tparam B The type of the derived class.
     * @param data The data to add.
     * @return A reference to the current object.
     */
    template<class B>
        requires std::is_base_of_v<DataBlock, B>
    B& add(std::span<const uint8_t> data) {
        dataBlock_.insert(dataBlock_.end(), data.begin(), data.end());
        return *static_cast<B*>(this);
    }

    /**
     * Add a given amount of padding, with a default padding of zeroes.
     * 
     * @tparam B The type of the derived class.
     * @param size The size of the padding to add.
     * @param padding The value to use for padding. Default is 0.
     * @return A reference to the current object.
     */
    template<class B>
        requires std::is_base_of_v<DataBlock, B>
    B& addPadding(size_t size, uint8_t padding = 0) {
        dataBlock_.insert(dataBlock_.end(), size, padding);
        return *static_cast<B*>(this);
    }


    /**
     * Return a part of the data block as a span.
     * 
     * @param offset The offset from the start of the data block.
     * @param size The size of the span to return.
     * @return A span of the specified part of the data block.
     */
    std::span<const uint8_t> getSpan(size_t offset, size_t size) const {
        if (offset + size > dataBlock_.size()) {
            throw std::out_of_range("Offset and size exceed data block size.");
        }
        return std::span<const uint8_t>(dataBlock_.data() + offset, size);
    }


public:
    DataBlock() = default;
    DataBlock(unsigned int size) : dataBlock_(size) {}
    DataBlock(const std::span<const uint8_t> data) : dataBlock_(data.begin(), data.end()) {}
    ~DataBlock() = default;

    DataBlock(const DataBlock&) = default;
    DataBlock(DataBlock&&) = default;
    DataBlock& operator=(const DataBlock&) = default;
    DataBlock& operator=(DataBlock&&) = default;


    /**
     * Reserve space for the specified size in the block.
     * 
     * @param size The size to reserve.
     */
    void reserve(size_t size) {
        dataBlock_.reserve(size);
    }


    /**
     * Return the current size of the block.
     * 
     * @return The size of the block.
     */
    size_t size() const noexcept {
        return dataBlock_.size();
    }


    /**
     * clear the block.
     */
    void clear() noexcept {
        dataBlock_.clear();
    }


    /**
     * Resize the block to the specified size. This will fill any new space with zeros.
     * 
     * @param size The new size of the block.
     */
    void resize(size_t size) {
        dataBlock_.resize(size, 0);
    }


    /**
     * Get a span of the data block.
     * 
     * @return A span of the data block.
     */
    std::span<const uint8_t> dataBlock() const noexcept {
        return std::span<const uint8_t>(dataBlock_.data(), dataBlock_.size());
    }


    /**
     * Set the data block to the specified data.
     * 
     * @param data The data to set.
     */
    void setData(const std::span<const uint8_t> data) {
        dataBlock_.assign(data.begin(), data.end());
    }
};

} // namespace SimConnect::Data