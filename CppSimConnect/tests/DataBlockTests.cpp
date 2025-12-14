/*
 * Copyright (c) 2025. Bert Laverman
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
 * See the License for the specific language governing permissions and limitations under the License.
 */

#include <cstdint>
#include <cstddef>
#include <vector>
#include <span>

#include "gtest/gtest.h"


#include <simconnect/data/data_block.hpp>

TEST(DataBlock, DefaultCtorAndSize) {
    const SimConnect::Data::DataBlock block;
    ASSERT_EQ(block.size(), 0U);
}

TEST(DataBlock, ReserveAndResize) {
    SimConnect::Data::DataBlock block;

    constexpr size_t initialSize = 100;
    constexpr size_t resizedSize = 50;
    constexpr size_t smallerSize = 10;

    block.reserve(initialSize);
    block.resize(resizedSize);
    ASSERT_EQ(block.size(), resizedSize);
    block.resize(smallerSize);
    ASSERT_EQ(block.size(), smallerSize);
}

TEST(DataBlock, SetDataAndGetSpan) {
    SimConnect::Data::DataBlock block;
    std::vector<uint8_t> testData = {1,2,3,4};
    block.setData(std::span<const uint8_t>(testData.data(), testData.size()));

    auto span = block.dataBlock();
    ASSERT_EQ(span.size(), testData.size());
    ASSERT_EQ(span[0], 1);
    ASSERT_EQ(span[2], 3);
}

TEST(DataBlock, Clear) {
    constexpr size_t smallerSize = 10;
    SimConnect::Data::DataBlock block(smallerSize);
    ASSERT_EQ(block.size(), smallerSize);
    block.clear();
    ASSERT_EQ(block.size(), 0U);
}
