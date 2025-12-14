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
#include <cstring>
#include <string>
#include <array>


#include "gtest/gtest.h"


#include <simconnect/data/data_block_builder.hpp>

static constexpr int32_t testInt32 = 42;
static constexpr int64_t testInt64 = 0x123456789abcdef0;
static constexpr float testFloat32 = 3.14F;
static constexpr double testFloat64 = 1.718281828459;

TEST(DataBlockBuilder, AddAndGetRawData) {
    SimConnect::Data::DataBlockBuilder builder;
    builder.addInt32(testInt32).addInt64(testInt64).addFloat32(testFloat32).addFloat64(testFloat64);
    auto data = builder.dataBlock();
    ASSERT_EQ(data.size(), sizeof(int32_t) + sizeof(int64_t) + sizeof(float) + sizeof(double));
    int32_t i32 = 0;
    int64_t i64 = 0;
    float f32 = 0.0F;
    double f64 = 0.0;
    size_t offset = 0;
    std::memcpy(&i32, data.data() + offset, sizeof(i32)); offset += sizeof(i32);
    std::memcpy(&i64, data.data() + offset, sizeof(i64)); offset += sizeof(i64);
    std::memcpy(&f32, data.data() + offset, sizeof(f32)); offset += sizeof(f32);
    std::memcpy(&f64, data.data() + offset, sizeof(f64));
    ASSERT_EQ(i32, testInt32);
    ASSERT_EQ(i64, testInt64);
    ASSERT_FLOAT_EQ(f32, testFloat32);
    ASSERT_DOUBLE_EQ(f64, testFloat64);
}

TEST(DataBlockBuilder, AddStringAndSpan) {
    const std::string testStr = "Hello";

    SimConnect::Data::DataBlockBuilder builder;
    builder.addString(testStr, testStr.size());
    auto data = builder.dataBlock();
    ASSERT_EQ(data.size(), testStr.size());
    ASSERT_EQ(std::string(reinterpret_cast<const char*>(data.data()), data.size()), testStr);   // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
}

TEST(DataBlockBuilder, Chaining) {
    SimConnect::Data::DataBlockBuilder builder;
    builder.addInt32(1).addInt32(2).addInt32(3);
    auto data = builder.dataBlock();
    ASSERT_EQ(data.size(), 3 * sizeof(int32_t));
    std::array<int32_t,3> arr{ { 0 } };
    std::memcpy(arr.data(), data.data(), sizeof(arr));
    ASSERT_EQ(arr[0], 1);
    ASSERT_EQ(arr[1], 2);
    ASSERT_EQ(arr[2], 3);
}
