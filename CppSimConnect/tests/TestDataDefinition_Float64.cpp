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


// Test marshalling and unmarshalling of double with double field
TEST(TestDataDefinition, TestFloat64_Float64) {
    struct TestFloat64Double { double field; };
    SimConnect::DataDefinition<TestFloat64Double> def;
    def.addFloat64(&TestFloat64Double::field, testVar, testUnit);

    // Marshall
    const TestFloat64Double src{ .field = testFloat64};
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);

    // Verify marshalled data
    auto data = builder.dataBlock();
    double value = zeroFloat64;
    std::memcpy(&value, data.data(), sizeof(double));
    ASSERT_DOUBLE_EQ(value, testFloat64);

    // Verify unmarshalling
    TestFloat64Double dst{ .field = zeroFloat64 };
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_DOUBLE_EQ(dst.field, src.field);
}

// Test marshalling and unmarshalling of double with float field
TEST(TestDataDefinition, TestFloat64_Float) {
    struct TestFloat64Float { float field; };
    SimConnect::DataDefinition<TestFloat64Float> def;
    def.addFloat64(&TestFloat64Float::field, testVar, testUnit);

    // Marshall
    const TestFloat64Float src{ .field = testFloat32 };
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);

    // Verify marshalled data
    auto data = builder.dataBlock();
    double value = zeroFloat64;
    std::memcpy(&value, data.data(), sizeof(double));
    ASSERT_DOUBLE_EQ(value, src.field);

    // Verify unmarshalling
    TestFloat64Float dst{ .field = zeroFloat32 };
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_FLOAT_EQ(dst.field, testFloat32);
}

// Test marshalling and unmarshalling of double with int field
TEST(TestDataDefinition, TestFloat64_Int) {
    struct TestFloat64Int { int field; };
    SimConnect::DataDefinition<TestFloat64Int> def;
    def.addFloat64(&TestFloat64Int::field, testVar, testUnit);

    // Marshall
    const TestFloat64Int src{ .field = testInt32 };
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);

    // Verify marshalled data
    auto data = builder.dataBlock();
    double value = zeroFloat64;
    std::memcpy(&value, data.data(), sizeof(double));
    ASSERT_DOUBLE_EQ(value, src.field);

    // Verify unmarshalling
    TestFloat64Int dst{ .field = zeroInt32 };
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_EQ(dst.field, testInt32);
}

// Test marshalling and unmarshalling of double with int64_t field
TEST(TestDataDefinition, TestFloat64_Int64) {
    struct TestFloat64Int64 { int64_t field; };
    SimConnect::DataDefinition<TestFloat64Int64> def;
    def.addFloat64(&TestFloat64Int64::field, testVar, testUnit);

    // Marshall
    const TestFloat64Int64 src{ .field = testInt64 };
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);

    // Verify marshalled data
    auto data = builder.dataBlock();
    double value = zeroFloat64;
    std::memcpy(&value, data.data(), sizeof(double));
    ASSERT_DOUBLE_EQ(value, static_cast<double>(src.field));

    // Verify unmarshalling
    TestFloat64Int64 dst{ .field = zeroInt64 };
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_EQ(dst.field, testInt64);
}

// Test marshalling and unmarshalling of double with bool field
TEST(TestDataDefinition, TestFloat64_Bool) {
    struct TestFloat64Bool { bool field; };
    SimConnect::DataDefinition<TestFloat64Bool> def;
    def.addFloat64(&TestFloat64Bool::field, testVar, testUnit);

    // Marshall
    const TestFloat64Bool src{ .field = true };
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);

    // Verify marshalled data
    auto data = builder.dataBlock();
    double value = zeroFloat64;
    std::memcpy(&value, data.data(), sizeof(double));
    ASSERT_DOUBLE_EQ(value, src.field ? 1.0 : 0.0);

    // Verify unmarshalling
    TestFloat64Bool dst{false};
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_TRUE(dst.field);
}

// Test marshalling and unmarshalling of double with string field
TEST(TestDataDefinition, TestFloat64_String) {
    struct TestFloat64String { std::string field; };
    SimConnect::DataDefinition<TestFloat64String> def;
    def.addFloat64(&TestFloat64String::field, testVar, testUnit);

    // Marshall
    const TestFloat64String src{ .field = testString};
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);

    // Verify marshalled data
    auto data = builder.dataBlock();
    double value = zeroFloat64;
    std::memcpy(&value, data.data(), sizeof(double));
    ASSERT_DOUBLE_EQ(value, testFloat64);

    // Verify unmarshalling
    TestFloat64String dst{ .field = ""};
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_EQ(dst.field, testStringTrunc);
}
