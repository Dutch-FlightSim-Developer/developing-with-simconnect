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

// Test marshalling and unmarshalling of int64_t
TEST(TestDataDefinition, TestInt64_Int64) {
    struct TestInt64 { int64_t field; };
    SimConnect::DataDefinition<TestInt64> def;
    def.addInt64(&TestInt64::field, "var", "unit");

    ASSERT_TRUE(def.useMapping());
    ASSERT_EQ(def.size(), sizeof(int64_t));

    // Marshalling
    TestInt64 src{0x123456789abcdef0};
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);
    auto data = builder.dataBlock();
    ASSERT_EQ(data.size(), sizeof(int64_t));

    // Verify marshalled value
    int64_t value = 0;
    std::memcpy(&value, data.data(), sizeof(int64_t));
    ASSERT_EQ(value, 0x123456789abcdef0);

    // Unmarshalling
    TestInt64 dst{0};
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_EQ(dst.field, 0x123456789abcdef0);
}

// Test marshalling and unmarshalling of int64_t with int field
TEST(TestDataDefinition, TestInt64_Int) {
    struct TestInt64Int { int field; };
    SimConnect::DataDefinition<TestInt64Int> def;
    def.addInt64(&TestInt64Int::field, "var", "unit");

    TestInt64Int src{123};
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);
    auto data = builder.dataBlock();
    int64_t value = 0;
    std::memcpy(&value, data.data(), sizeof(int64_t));
    ASSERT_EQ(value, 123);

    TestInt64Int dst{0};
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_EQ(dst.field, 123);
}

// Test marshalling and unmarshalling of int64_t with float field
TEST(TestDataDefinition, TestInt64_Float) {
    struct TestInt64Float { float field; };
    SimConnect::DataDefinition<TestInt64Float> def;
    def.addInt64(&TestInt64Float::field, "var", "unit");

    TestInt64Float src{7.0f};
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);
    auto data = builder.dataBlock();
    int64_t value = 0;
    std::memcpy(&value, data.data(), sizeof(int64_t));
    ASSERT_EQ(value, 7);

    TestInt64Float dst{0.0f};
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_FLOAT_EQ(dst.field, 7.0f);
}

// Test marshalling and unmarshalling of int64_t with double field
TEST(TestDataDefinition, TestInt64_Double) {
    struct TestInt64Double { double field; };
    SimConnect::DataDefinition<TestInt64Double> def;
    def.addInt64(&TestInt64Double::field, "var", "unit");

    TestInt64Double src{8.0};
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);
    auto data = builder.dataBlock();
    int64_t value = 0;
    std::memcpy(&value, data.data(), sizeof(int64_t));
    ASSERT_EQ(value, 8);

    TestInt64Double dst{0.0};
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_DOUBLE_EQ(dst.field, 8.0);
}

// Test marshalling and unmarshalling of int64_t with bool field
TEST(TestDataDefinition, TestInt64_Bool) {
    struct TestInt64Bool { bool field; };
    SimConnect::DataDefinition<TestInt64Bool> def;
    def.addInt64(&TestInt64Bool::field, "var", "unit");

    TestInt64Bool src{true};
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);
    auto data = builder.dataBlock();
    int64_t value = 0;
    std::memcpy(&value, data.data(), sizeof(int64_t));
    ASSERT_EQ(value, 1);

    TestInt64Bool dst{false};
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_TRUE(dst.field);
}

// Test marshalling and unmarshalling of int64_t with string field
TEST(TestDataDefinition, TestInt64_String) {
    struct TestInt64String { std::string field; };
    SimConnect::DataDefinition<TestInt64String> def;
    def.addInt64(&TestInt64String::field, "var", "unit");

    TestInt64String src{"99"};
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);
    auto data = builder.dataBlock();
    int64_t value = 0;
    std::memcpy(&value, data.data(), sizeof(int64_t));
    ASSERT_EQ(value, 99);

    TestInt64String dst{""};
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_EQ(dst.field, "99");
}
