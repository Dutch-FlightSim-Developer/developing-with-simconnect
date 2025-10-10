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

#include "pch.h"
#include <simconnect/data/data_block_builder.hpp>
#include <simconnect/data/data_block_reader.hpp>

TEST(DataBlockReader, ReadPrimitives) {
    SimConnect::Data::DataBlockBuilder builder;
    builder.addInt32(42).addInt64(0x123456789abcdef0).addFloat32(3.14f).addFloat64(2.718281828459);
    auto data = builder.dataBlock();
    SimConnect::Data::DataBlockReader reader(data);
    int32_t i32 = reader.readInt32();
    int64_t i64 = reader.readInt64();
    float f32 = reader.readFloat32();
    double f64 = reader.readFloat64();
    ASSERT_EQ(i32, 42);
    ASSERT_EQ(i64, 0x123456789abcdef0);
    ASSERT_FLOAT_EQ(f32, 3.14f);
    ASSERT_DOUBLE_EQ(f64, 2.718281828459);
}

TEST(DataBlockReader, ReadString) {
    std::string s = "Hello, world!";
    SimConnect::Data::DataBlockBuilder builder;
    builder.addString(s, s.size());
    auto data = builder.dataBlock();
    SimConnect::Data::DataBlockReader reader(data);
    std::string out = reader.readString(s.size());
    ASSERT_EQ(out, s);
}

TEST(DataBlockReader, OutOfRangeThrows) {
    SimConnect::Data::DataBlockBuilder builder;
    builder.addInt32(1);
    auto data = builder.dataBlock();
    SimConnect::Data::DataBlockReader reader(data);
    reader.readInt32();
    ASSERT_THROW(reader.readInt32(), std::out_of_range);
}
