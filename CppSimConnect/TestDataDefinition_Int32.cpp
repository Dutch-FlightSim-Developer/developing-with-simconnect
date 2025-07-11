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

#include "pch.h"

#include <simconnect/windows_event_connection.hpp>
#include <simconnect/windows_event_handler.hpp>

#include <simconnect/data_definition.hpp>

// Test marshalling and unmarshalling of int32_t with int field
TEST(TestDataDefinition, TestInt32_Int) {
    struct TestInt32Int { int field; };
    SimConnect::DataDefinition<TestInt32Int> def;
    def.addInt32(&TestInt32Int::field, "var", "unit");

    TestInt32Int src{123};
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);
    auto data = builder.dataBlock();
    int32_t value = 0;
    std::memcpy(&value, data.data(), sizeof(int32_t));
    ASSERT_EQ(value, 123);

    TestInt32Int dst{0};
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_EQ(dst.field, 123);
}

// Test marshalling and unmarshalling of int32_t with int64_t field
TEST(TestDataDefinition, TestInt32_Int64) {
    struct TestInt32Int64 { int64_t field; };
    SimConnect::DataDefinition<TestInt32Int64> def;
    def.addInt32(&TestInt32Int64::field, "var", "unit");

    TestInt32Int64 src{static_cast<int32_t>(456)};
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);
    auto data = builder.dataBlock();
    int32_t value = 0;
    std::memcpy(&value, data.data(), sizeof(int32_t));
    ASSERT_EQ(value, 456);

    TestInt32Int64 dst{0};
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_EQ(dst.field, 456);
}

// Test marshalling and unmarshalling of int32_t with float field
TEST(TestDataDefinition, TestInt32_Float) {
    struct TestInt32Float { float field; };
    SimConnect::DataDefinition<TestInt32Float> def;
    def.addInt32(&TestInt32Float::field, "var", "unit");

    TestInt32Float src{7.0f};
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);
    auto data = builder.dataBlock();
    int32_t value = 0;
    std::memcpy(&value, data.data(), sizeof(int32_t));
    ASSERT_EQ(value, 7);

    TestInt32Float dst{0.0f};
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_FLOAT_EQ(dst.field, 7.0f);
}

// Test marshalling and unmarshalling of int32_t with double field
TEST(TestDataDefinition, TestInt32_Double) {
    struct TestInt32Double { double field; };
    SimConnect::DataDefinition<TestInt32Double> def;
    def.addInt32(&TestInt32Double::field, "var", "unit");

    TestInt32Double src{8.0};
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);
    auto data = builder.dataBlock();
    int32_t value = 0;
    std::memcpy(&value, data.data(), sizeof(int32_t));
    ASSERT_EQ(value, 8);

    TestInt32Double dst{0.0};
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_DOUBLE_EQ(dst.field, 8.0);
}

// Test marshalling and unmarshalling of int32_t with bool field
TEST(TestDataDefinition, TestInt32_Bool) {
    struct TestInt32Bool { bool field; };
    SimConnect::DataDefinition<TestInt32Bool> def;
    def.addInt32(&TestInt32Bool::field, "var", "unit");

    TestInt32Bool src{true};
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);
    auto data = builder.dataBlock();
    int32_t value = 0;
    std::memcpy(&value, data.data(), sizeof(int32_t));
    ASSERT_EQ(value, 1);

    TestInt32Bool dst{false};
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_TRUE(dst.field);
}

// Test marshalling and unmarshalling of int32_t with string field
TEST(TestDataDefinition, TestInt32_String) {
    struct TestInt32String { std::string field; };
    SimConnect::DataDefinition<TestInt32String> def;
    def.addInt32(&TestInt32String::field, "var", "unit");

    TestInt32String src{"99"};
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);
    auto data = builder.dataBlock();
    int32_t value = 0;
    std::memcpy(&value, data.data(), sizeof(int32_t));
    ASSERT_EQ(value, 99);

    TestInt32String dst{""};
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_EQ(dst.field, "99");
}


