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


// Test marshalling and unmarshalling of int64_t
TEST(TestDataDefinition, TestInt64_Int64) {
    struct TestInt64 { int64_t field; };
    SimConnect::DataDefinition<TestInt64> def;
    def.addInt64(&TestInt64::field, testVar, testUnit);

    ASSERT_TRUE(def.useMapping());
    ASSERT_EQ(def.size(), sizeof(int64_t));

    // Marshalling
    const TestInt64 src{ .field = testInt64};
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);

    // Verify marshalled value
    auto data = builder.dataBlock();
    ASSERT_EQ(data.size(), sizeof(int64_t));
    int64_t value = zeroInt64;
    std::memcpy(&value, data.data(), sizeof(int64_t));
    ASSERT_EQ(value, testInt64);

    // Unmarshalling
    TestInt64 dst{.field = zeroInt64};
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_EQ(dst.field, testInt64);
}

// Test marshalling and unmarshalling of int64_t with int field
TEST(TestDataDefinition, TestInt64_Int) {
    struct TestInt64Int { int field; };
    SimConnect::DataDefinition<TestInt64Int> def;
    def.addInt64(&TestInt64Int::field, testVar, testUnit);

    // Marshall
    const TestInt64Int src{.field = testInt32};
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);

    // Verify marshalled data
    auto data = builder.dataBlock();
    int64_t value = zeroInt64;
    std::memcpy(&value, data.data(), sizeof(int64_t));
    ASSERT_EQ(value, testInt32);

    // Verify unmarshalling
    TestInt64Int dst{.field = zeroInt32};
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_EQ(dst.field, testInt32);
}

// Test marshalling and unmarshalling of int64_t with float field
TEST(TestDataDefinition, TestInt64_Float) {
    struct TestInt64Float { float field; };
    SimConnect::DataDefinition<TestInt64Float> def;
    def.addInt64(&TestInt64Float::field, testVar, testUnit);

    // Marshall
    const TestInt64Float src{ .field = testFloat32};
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);

    // Verify marshalled data
    auto data = builder.dataBlock();
    int64_t value = zeroInt64;
    std::memcpy(&value, data.data(), sizeof(int64_t));
    ASSERT_EQ(value, testInt64);

    // Verify unmarshalling
    TestInt64Float dst{ .field = zeroFloat32 };
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_FLOAT_EQ(dst.field, testFloat32);
}

// Test marshalling and unmarshalling of int64_t with double field
TEST(TestDataDefinition, TestInt64_Double) {
    struct TestInt64Double { double field; };
    SimConnect::DataDefinition<TestInt64Double> def;
    def.addInt64(&TestInt64Double::field, testVar, testUnit);

    // Marshall
    const TestInt64Double src{ .field = testFloat64 };
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);

    // Verify marshalled data
    auto data = builder.dataBlock();
    int64_t value = zeroInt64;
    std::memcpy(&value, data.data(), sizeof(int64_t));
    ASSERT_EQ(value, testInt64);

    // Verify unmarshalling
    TestInt64Double dst{ .field = zeroFloat64 };
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_DOUBLE_EQ(dst.field, testFloat64);
}

// Test marshalling and unmarshalling of int64_t with bool field
TEST(TestDataDefinition, TestInt64_Bool) {
    struct TestInt64Bool { bool field; };
    SimConnect::DataDefinition<TestInt64Bool> def;
    def.addInt64(&TestInt64Bool::field, testVar, testUnit);

    // Marshall
    const TestInt64Bool src{ .field = true };
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);

    // Verify marshalled data
    auto data = builder.dataBlock();
    int64_t value = zeroInt64;
    std::memcpy(&value, data.data(), sizeof(int64_t));
    ASSERT_EQ(value, 1);

    // Verify unmarshalling
    TestInt64Bool dst{ .field = false };
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_TRUE(dst.field);
}

// Test marshalling and unmarshalling of int64_t with string field
TEST(TestDataDefinition, TestInt64_String) {
    struct TestInt64String { std::string field; };
    SimConnect::DataDefinition<TestInt64String> def;
    def.addInt64(&TestInt64String::field, testVar, testUnit);

    // Marshall
    const TestInt64String src{ .field = testString };
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);

    // Verify marshalled data
    auto data = builder.dataBlock();
    int64_t value = zeroInt64;
    std::memcpy(&value, data.data(), sizeof(int64_t));
    ASSERT_EQ(value, testInt64);

    // Verify unmarshalling
    TestInt64String dst{ .field = "" };
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_EQ(dst.field, testStringTrunc);
}
