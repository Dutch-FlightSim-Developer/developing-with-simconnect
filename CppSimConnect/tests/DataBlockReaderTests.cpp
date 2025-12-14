/*
 * Copyright (c) 2025. Bert Laverman
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and limitations under the License.
 */

#include <cstdint>

#include <stdexcept>
#include <string>


#include "gtest/gtest.h"


#include <simconnect/data/data_block_builder.hpp>
#include <simconnect/data/data_block_reader.hpp>

static constexpr int32_t testInt32 = 42;
static constexpr int64_t testInt64 = 0x123456789abcdef0;
static constexpr float testFloat32 = 3.14F;
static constexpr double testFloat64 = 1.718281828459;

TEST(DataBlockReader, ReadPrimitives) {
    SimConnect::Data::DataBlockBuilder builder;
    builder.addInt32(testInt32).addInt64(testInt64).addFloat32(testFloat32).addFloat64(testFloat64);
    auto data = builder.dataBlock();
    SimConnect::Data::DataBlockReader reader(data);

    const int32_t i32 = reader.readInt32();
    const int64_t i64 = reader.readInt64();
    const float f32 = reader.readFloat32();
    const double f64 = reader.readFloat64();

    ASSERT_EQ(i32, testInt32);
    ASSERT_EQ(i64, testInt64);
    ASSERT_FLOAT_EQ(f32, testFloat32);
    ASSERT_DOUBLE_EQ(f64, testFloat64);
}

TEST(DataBlockReader, ReadString) {
    const std::string testStr = "Hello, world!";
    SimConnect::Data::DataBlockBuilder builder;
    builder.addString(testStr, testStr.size());
    auto data = builder.dataBlock();
    SimConnect::Data::DataBlockReader reader(data);
    const std::string out = reader.readString(testStr.size());
    ASSERT_EQ(out, testStr);
}

TEST(DataBlockReader, OutOfRangeThrows) {
    SimConnect::Data::DataBlockBuilder builder;
    builder.addInt32(1);
    auto data = builder.dataBlock();
    SimConnect::Data::DataBlockReader reader(data);
    reader.readInt32();
    ASSERT_THROW(reader.readInt32(), std::out_of_range);
}
