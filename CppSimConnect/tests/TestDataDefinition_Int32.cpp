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
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cstdint>
#include <cstring>

#include "gtest/gtest.h"


#include <simconnect/data/data_block_builder.hpp>
#include <simconnect/data/data_block_reader.hpp>
#include <simconnect/data_definition.hpp>


static constexpr int32_t zeroInt32{ 0 };
static constexpr int32_t testInt32{ 123 };
static constexpr int64_t zeroInt64{ 0 };
static constexpr int64_t testInt64{ 123 };
static constexpr float testFloat32{ 123.0F };
static constexpr float zeroFloat32{ 0.0F };
static constexpr double testFloat64{ 123.0 };
static constexpr double zeroFloat64{ 0.0 };
static constexpr const char* testString{ "123.0" };
static constexpr const char* testStringTrunc{ "123" };

static constexpr const char* testVar = "var";
static constexpr const char* testUnit = "unit";


// Test marshalling and unmarshalling of int32_t with int field
TEST(TestDataDefinition, TestInt32_Int) {
    struct TestInt32Int { int field; };
    SimConnect::DataDefinition<TestInt32Int> def;
    def.addInt32(&TestInt32Int::field, testVar, testUnit);

    // Marshall
    const TestInt32Int src{ .field = testInt32};
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);

    // Verify marshalled data
    auto data = builder.dataBlock();
    int32_t value = zeroInt32;
    std::memcpy(&value, data.data(), sizeof(int32_t));
    ASSERT_EQ(value, testInt32);

    // Verify unmarshalling
    TestInt32Int dst{ .field = zeroInt32 };
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_EQ(dst.field, testInt32);
}

// Test marshalling and unmarshalling of int32_t with int64_t field
TEST(TestDataDefinition, TestInt32_Int64) {
    struct TestInt32Int64 { int64_t field; };
    SimConnect::DataDefinition<TestInt32Int64> def;
    def.addInt32(&TestInt32Int64::field, testVar, testUnit);

    // Marshall
    const TestInt32Int64 src{ .field = testInt64 };
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);

    // Verify marshalled data
    auto data = builder.dataBlock();
    int32_t value = zeroInt32;
    std::memcpy(&value, data.data(), sizeof(int32_t));
    ASSERT_EQ(value, testInt32);

    TestInt32Int64 dst{ .field = zeroInt64 };
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_EQ(dst.field, testInt64);
}

// Test marshalling and unmarshalling of int32_t with float field
TEST(TestDataDefinition, TestInt32_Float) {
    struct TestInt32Float { float field; };
    SimConnect::DataDefinition<TestInt32Float> def;
    def.addInt32(&TestInt32Float::field, testVar, testUnit);

    // Marshall
    const TestInt32Float src{ .field = testFloat32 };
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);

    // Verify marshalled data
    auto data = builder.dataBlock();
    int32_t value = zeroInt32;
    std::memcpy(&value, data.data(), sizeof(int32_t));
    ASSERT_EQ(value, testInt32);

    // Verify unmarshalling
    TestInt32Float dst{.field = zeroFloat32};
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_FLOAT_EQ(dst.field, testFloat32);
}

// Test marshalling and unmarshalling of int32_t with double field
TEST(TestDataDefinition, TestInt32_Double) {
    struct TestInt32Double { double field; };
    SimConnect::DataDefinition<TestInt32Double> def;
    def.addInt32(&TestInt32Double::field, testVar, testUnit);

    // Marshall
    const TestInt32Double src{.field = testFloat64};
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);

    // Verify marshalled data
    auto data = builder.dataBlock();
    int32_t value = zeroInt32;
    std::memcpy(&value, data.data(), sizeof(int32_t));
    ASSERT_EQ(value, testInt32);

    // Verify unmarshalling
    TestInt32Double dst{.field = zeroFloat64};
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_DOUBLE_EQ(dst.field, testFloat64);
}

// Test marshalling and unmarshalling of int32_t with bool field
TEST(TestDataDefinition, TestInt32_Bool) {
    struct TestInt32Bool { bool field; };
    SimConnect::DataDefinition<TestInt32Bool> def;
    def.addInt32(&TestInt32Bool::field, testVar, testUnit);

    // Marshall
    const TestInt32Bool src{ .field = true };
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);

    // Verify marshalled data
    auto data = builder.dataBlock();
    int32_t value = zeroInt32;
    std::memcpy(&value, data.data(), sizeof(int32_t));
    ASSERT_EQ(value, 1);

    // Verify unmarshalling
    TestInt32Bool dst{ .field = false };
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_TRUE(dst.field);
}

// Test marshalling and unmarshalling of int32_t with string field
TEST(TestDataDefinition, TestInt32_String) {
    struct TestInt32String { std::string field; };
    SimConnect::DataDefinition<TestInt32String> def;
    def.addInt32(&TestInt32String::field, testVar, testUnit);

    // Marshall
    const TestInt32String src{ .field = testString };
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);

    // Verify marshalled data
    auto data = builder.dataBlock();
    int32_t value = zeroInt32;
    std::memcpy(&value, data.data(), sizeof(int32_t));
    ASSERT_EQ(value, testInt32);

    // Verify unmarshalling
    TestInt32String dst{ .field = "" };
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_EQ(dst.field, testStringTrunc);
}


