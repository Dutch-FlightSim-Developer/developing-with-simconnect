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
#include <simconnect/data/data_block_builder.hpp>
#include <simconnect/data/data_block_reader.hpp>

TEST(DataBlockBuilder, AddAndGetRawData) {
    SimConnect::Data::DataBlockBuilder builder;
    builder.addInt32(42).addInt64(0x123456789abcdef0).addFloat32(3.14f).addFloat64(2.718281828459);
    auto data = builder.dataBlock();
    ASSERT_EQ(data.size(), sizeof(int32_t) + sizeof(int64_t) + sizeof(float) + sizeof(double));
    int32_t i32 = 0;
    int64_t i64 = 0;
    float f32 = 0.0f;
    double f64 = 0.0;
    size_t offset = 0;
    std::memcpy(&i32, data.data() + offset, sizeof(i32)); offset += sizeof(i32);
    std::memcpy(&i64, data.data() + offset, sizeof(i64)); offset += sizeof(i64);
    std::memcpy(&f32, data.data() + offset, sizeof(f32)); offset += sizeof(f32);
    std::memcpy(&f64, data.data() + offset, sizeof(f64));
    ASSERT_EQ(i32, 42);
    ASSERT_EQ(i64, 0x123456789abcdef0);
    ASSERT_FLOAT_EQ(f32, 3.14f);
    ASSERT_DOUBLE_EQ(f64, 2.718281828459);
}

TEST(DataBlockBuilder, AddStringAndSpan) {
    SimConnect::Data::DataBlockBuilder builder;
    std::string s = "Hello";
    builder.addString(s, s.size());
    auto data = builder.dataBlock();
    ASSERT_EQ(data.size(), s.size());
    ASSERT_EQ(std::string(reinterpret_cast<const char*>(data.data()), data.size()), s);
}

TEST(DataBlockBuilder, Chaining) {
    SimConnect::Data::DataBlockBuilder builder;
    builder.addInt32(1).addInt32(2).addInt32(3);
    auto data = builder.dataBlock();
    ASSERT_EQ(data.size(), 3 * sizeof(int32_t));
    int32_t arr[3];
    std::memcpy(arr, data.data(), sizeof(arr));
    ASSERT_EQ(arr[0], 1);
    ASSERT_EQ(arr[1], 2);
    ASSERT_EQ(arr[2], 3);
}
