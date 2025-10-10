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

#include "pch.h"
#include <simconnect/data/data_block.hpp>
#include <vector>
#include <cstring>

TEST(DataBlock, DefaultCtorAndSize) {
    SimConnect::Data::DataBlock block;
    ASSERT_EQ(block.size(), 0u);
}

TEST(DataBlock, ReserveAndResize) {
    SimConnect::Data::DataBlock block;
    block.reserve(100);
    block.resize(50);
    ASSERT_EQ(block.size(), 50u);
    block.resize(10);
    ASSERT_EQ(block.size(), 10u);
}

TEST(DataBlock, SetDataAndGetSpan) {
    SimConnect::Data::DataBlock block;
    std::vector<uint8_t> v = {1,2,3,4,5};
    block.setData(std::span<const uint8_t>(v.data(), v.size()));
    ASSERT_EQ(block.size(), 5u);
	auto span = block.dataBlock();
    ASSERT_EQ(span.size(), 5u);
    ASSERT_EQ(span[0], 1);
    ASSERT_EQ(span[2], 3);
}

TEST(DataBlock, Clear) {
    SimConnect::Data::DataBlock block(10);
    ASSERT_EQ(block.size(), 10u);
    block.clear();
    ASSERT_EQ(block.size(), 0u);
}
