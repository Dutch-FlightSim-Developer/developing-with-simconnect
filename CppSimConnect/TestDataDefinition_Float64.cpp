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

// Test marshalling and unmarshalling of double (native)
TEST(TestDataDefinition, TestFloat64_Double) {
    struct TestFloat64 { double field; };
    SimConnect::DataDefinition<TestFloat64> def;
    def.addFloat64(&TestFloat64::field, "var", "unit");

    constexpr TestFloat64 src{2.718281828459};
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);
    auto data = builder.dataBlock();
    double value = 0.0;
    std::memcpy(&value, data.data(), sizeof(double));
    ASSERT_DOUBLE_EQ(value, src.field);

    TestFloat64 dst{0.0};
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_DOUBLE_EQ(dst.field, src.field);
}

// Test marshalling and unmarshalling of double with float field
TEST(TestDataDefinition, TestFloat64_Float) {
    struct TestFloat64Float { float field; };
    SimConnect::DataDefinition<TestFloat64Float> def;
    def.addFloat64(&TestFloat64Float::field, "var", "unit");

    constexpr TestFloat64Float src{3.14f};
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);
    auto data = builder.dataBlock();
    double value = 0.0;
    std::memcpy(&value, data.data(), sizeof(double));
    ASSERT_DOUBLE_EQ(value, src.field);

    TestFloat64Float dst{0.0f};
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_FLOAT_EQ(dst.field, src.field);
}

// Test marshalling and unmarshalling of double with int field
TEST(TestDataDefinition, TestFloat64_Int) {
    struct TestFloat64Int { int field; };
    SimConnect::DataDefinition<TestFloat64Int> def;
    def.addFloat64(&TestFloat64Int::field, "var", "unit");

    constexpr TestFloat64Int src{42};
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);
    auto data = builder.dataBlock();
    double value = 0.0;
    std::memcpy(&value, data.data(), sizeof(double));
    ASSERT_DOUBLE_EQ(value, src.field);

    TestFloat64Int dst{0};
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_EQ(dst.field, src.field);
}

// Test marshalling and unmarshalling of double with int64_t field
TEST(TestDataDefinition, TestFloat64_Int64) {
    struct TestFloat64Int64 { int64_t field; };
    SimConnect::DataDefinition<TestFloat64Int64> def;
    def.addFloat64(&TestFloat64Int64::field, "var", "unit");

    constexpr TestFloat64Int64 src{ 123 };
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);
    auto data = builder.dataBlock();
    double value = 0.0;
    std::memcpy(&value, data.data(), sizeof(double));
    ASSERT_DOUBLE_EQ(value, static_cast<double>(src.field));

    TestFloat64Int64 dst{0};
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_EQ(dst.field, src.field);
}

// Test marshalling and unmarshalling of double with bool field
TEST(TestDataDefinition, TestFloat64_Bool) {
    struct TestFloat64Bool { bool field; };
    SimConnect::DataDefinition<TestFloat64Bool> def;
    def.addFloat64(&TestFloat64Bool::field, "var", "unit");

    constexpr TestFloat64Bool src{true};
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);
    auto data = builder.dataBlock();
    double value = 0.0;
    std::memcpy(&value, data.data(), sizeof(double));
    ASSERT_DOUBLE_EQ(value, src.field ? 1.0 : 0.0);

    TestFloat64Bool dst{false};
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_TRUE(dst.field);
}

// Test marshalling and unmarshalling of double with string field
TEST(TestDataDefinition, TestFloat64_String) {
    struct TestFloat64String { std::string field; };
    SimConnect::DataDefinition<TestFloat64String> def;
    def.addFloat64(&TestFloat64String::field, "var", "unit");

    TestFloat64String src{"77.5"};
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);
    auto data = builder.dataBlock();
    double value = 0.0;
    std::memcpy(&value, data.data(), sizeof(double));
    ASSERT_DOUBLE_EQ(value, std::stod(src.field));

    TestFloat64String dst{""};
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_EQ(dst.field, src.field);
}
