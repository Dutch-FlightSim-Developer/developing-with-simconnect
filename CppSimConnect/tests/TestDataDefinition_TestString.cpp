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

#include <cstddef>
#include <cstring>
#include <string>

#include "gtest/gtest.h"


#include <simconnect/data/data_block_builder.hpp>
#include <simconnect/data/data_block_reader.hpp>
#include <simconnect/data_definition.hpp>


static constexpr const char* testString8{ "123.0" };
static constexpr size_t testString8Len{ 5 };
static constexpr size_t testString8Size{ 8 };
static constexpr const char* testString32{ "123.0 test" };
static constexpr size_t testString32Len{ 10 };
static constexpr size_t testString32Size{ 32 };
static constexpr const char* testString64{ "123.0 test string for 64" };
static constexpr size_t testString64Len{ 24 };
static constexpr size_t testString64Size{ 64 };
static constexpr const char* testString128{ "123.0 test string for 128 characters length" };
static constexpr size_t testString128Len{ 43 };
static constexpr size_t testString128Size{ 128 };
static constexpr const char* testString256{ "123.0 test string for 256 characters length, which is a bit longer than the previous strings to test the functionality properly." };
static constexpr size_t testString256Len{ 128 };
static constexpr size_t testString256Size{ 256 };
static constexpr const char* testString260{ "123.0 test string for 260 characters length, which is a bit longer than the previous strings to test the functionality properly. This string is specifically designed to exceed the 256 character limit by a small margin." };
static constexpr size_t testString260Len{ 218 };
static constexpr size_t testString260Size{ 260 };
static constexpr const char* testStringV{ "This is a variable length string for testing." };
static constexpr size_t testStringVLen{ 45 };
static constexpr size_t testStringVSize{ 46 }; // Including the null terminator

static constexpr const char* testVar = "var";


 // Test marshalling and unmarshalling of string8
TEST(TestDataDefinition, TestString8_1) {
    struct TestString8 { std::string field; };
    SimConnect::DataDefinition<TestString8> def;
    def.addString8(&TestString8::field, testVar);

    ASSERT_FALSE(def.useMapping());
    ASSERT_EQ(def.size(), 8);

    // Marshalling
    const TestString8 src{ .field = testString8 };
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);
    auto data = builder.dataBlock();
    ASSERT_EQ(data.size(), testString8Size);

    // Verify marshalled value
    const std::string value(reinterpret_cast<const char*>(data.data()), testString8Len); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    ASSERT_EQ(value, testString8);

    // Unmarshalling
    TestString8 dst{ .field = "" };
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_EQ(dst.field, testString8);
}

// Test marshalling and unmarshalling of string32
TEST(TestDataDefinition, TestString32_1) {
    struct TestString32 { std::string field; };
    SimConnect::DataDefinition<TestString32> def;
    def.addString32(&TestString32::field, testVar);

    ASSERT_FALSE(def.useMapping());
    ASSERT_EQ(def.size(), testString32Size);

    // Marshalling
    const TestString32 src{ .field = testString32 };
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);

    // Verify marshalled value
    auto data = builder.dataBlock();
    ASSERT_EQ(data.size(), testString32Size);
    const std::string value(reinterpret_cast<const char*>(data.data()), testString32Len); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    ASSERT_EQ(value, testString32);

    // Unmarshalling
    TestString32 dst{ .field = "" };
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_EQ(dst.field, testString32);
}

// Test marshalling and unmarshalling of string64
TEST(TestDataDefinition, TestString64_1) {
    struct TestString64 { std::string field; };
    SimConnect::DataDefinition<TestString64> def;
    def.addString64(&TestString64::field, testVar);

    ASSERT_FALSE(def.useMapping());
    ASSERT_EQ(def.size(), 64);

    // Marshalling
    const TestString64 src{ .field = testString64 };
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);

    // Verify marshalled value
    auto data = builder.dataBlock();
    ASSERT_EQ(data.size(), testString64Size);
    const std::string value(reinterpret_cast<const char*>(data.data()), testString64Len); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    ASSERT_EQ(value, testString64);

    // Unmarshalling
    TestString64 dst{ "" };
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_EQ(dst.field, testString64);
}

// Test marshalling and unmarshalling of string128
TEST(TestDataDefinition, TestString128_1) {
    struct TestString128 { std::string field; };
    SimConnect::DataDefinition<TestString128> def;
    def.addString128(&TestString128::field, testVar);

    ASSERT_FALSE(def.useMapping());
    ASSERT_EQ(def.size(), testString128Size);

    // Marshalling
    const TestString128 src{ .field = testString128 };
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);

    // Verify marshalled value
    auto data = builder.dataBlock();
    ASSERT_EQ(data.size(), testString128Size);
    const std::string value(reinterpret_cast<const char*>(data.data()), testString128Len); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    ASSERT_EQ(value, testString128);

    // Unmarshalling
    TestString128 dst{ "" };
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_EQ(dst.field, testString128);
}

// Test marshalling and unmarshalling of string256
TEST(TestDataDefinition, TestString256_1) {
    struct TestString256 { std::string field; };
    SimConnect::DataDefinition<TestString256> def;
    def.addString256(&TestString256::field, testVar);

    ASSERT_FALSE(def.useMapping());
    ASSERT_EQ(def.size(), testString256Size);

    // Marshalling
    const TestString256 src{ .field = testString256 };
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);

    // Verify marshalled value
    auto data = builder.dataBlock();
    ASSERT_EQ(data.size(), testString256Size);
    const std::string value(reinterpret_cast<const char*>(data.data()), testString256Len); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    ASSERT_EQ(value, testString256);

    // Unmarshalling
    TestString256 dst{ .field = "" };
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_EQ(dst.field, testString256);
}

// Test marshalling and unmarshalling of string260
TEST(TestDataDefinition, TestString260_1) {
    struct TestString260 { std::string field; };
    SimConnect::DataDefinition<TestString260> def;
    def.addString260(&TestString260::field, testVar);

    ASSERT_FALSE(def.useMapping());
    ASSERT_EQ(def.size(), testString260Size);

    // Marshalling
    const TestString260 src{ .field = testString260 };
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);

    // Verify marshalled value
    auto data = builder.dataBlock();
    ASSERT_EQ(data.size(), testString260Size);
    const std::string value(reinterpret_cast<const char*>(data.data()), testString260Len); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    ASSERT_EQ(value, testString260);

    // Unmarshalling
    TestString260 dst{ .field = "" };
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_EQ(dst.field, testString260);
}

// Test marshalling and unmarshalling of stringV
TEST(TestDataDefinition, TestStringV_1) {
    struct TestStringV { std::string field; };
    SimConnect::DataDefinition<TestStringV> def;
    def.addStringV(&TestStringV::field, testVar);

    ASSERT_FALSE(def.useMapping());
    ASSERT_GE(def.size(), 4); // StringV is variable length, min 4 bytes

    // Marshalling
    const TestStringV src{ .field = testStringV };
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);

    // Verify marshalled value
    auto data = builder.dataBlock();
    ASSERT_EQ(data.size(), testStringVSize);
    const std::string value(reinterpret_cast<const char*>(data.data()), testStringVLen); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    ASSERT_EQ(value, testStringV);

    // Unmarshalling
    TestStringV dst{ "" };
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_EQ(dst.field, testStringV);
}
