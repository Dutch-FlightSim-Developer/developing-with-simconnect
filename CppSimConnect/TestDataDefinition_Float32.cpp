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

// Test marshalling and unmarshalling of float (native)
TEST(TestDataDefinition, TestFloat32_Float) {
    struct TestFloat32 { float field; };
    SimConnect::DataDefinition<TestFloat32> def;
    def.addFloat32(&TestFloat32::field, "var", "unit");

    TestFloat32 src{3.14f};
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);
    auto data = builder.dataBlock();
    float value = 0.0f;
    std::memcpy(&value, data.data(), sizeof(float));
    ASSERT_FLOAT_EQ(value, 3.14f);

    TestFloat32 dst{0.0f};
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_FLOAT_EQ(dst.field, 3.14f);
}

// Test marshalling and unmarshalling of float with int field
TEST(TestDataDefinition, TestFloat32_Int) {
    struct TestFloat32Int { int field; };
    SimConnect::DataDefinition<TestFloat32Int> def;
    def.addFloat32(&TestFloat32Int::field, "var", "unit");

    TestFloat32Int src{42};
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);
    auto data = builder.dataBlock();
    float value = 0.0f;
    std::memcpy(&value, data.data(), sizeof(float));
    ASSERT_FLOAT_EQ(value, 42.0f);

    TestFloat32Int dst{0};
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_EQ(dst.field, 42);
}

// Test marshalling and unmarshalling of float with int64_t field
TEST(TestDataDefinition, TestFloat32_Int64) {
    struct TestFloat32Int64 { int64_t field; };
    SimConnect::DataDefinition<TestFloat32Int64> def;
    def.addFloat32(&TestFloat32Int64::field, "var", "unit");

    TestFloat32Int64 src{static_cast<int64_t>(123)};
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);
    auto data = builder.dataBlock();
    float value = 0.0f;
    std::memcpy(&value, data.data(), sizeof(float));
    ASSERT_FLOAT_EQ(value, 123.0f);

    TestFloat32Int64 dst{0};
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_EQ(dst.field, 123);
}

// Test marshalling and unmarshalling of float with double field
TEST(TestDataDefinition, TestFloat32_Double) {
    struct TestFloat32Double { double field; };
    SimConnect::DataDefinition<TestFloat32Double> def;
    def.addFloat32(&TestFloat32Double::field, "var", "unit");

    TestFloat32Double src{2.5};
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);
    auto data = builder.dataBlock();
    float value = 0.0f;
    std::memcpy(&value, data.data(), sizeof(float));
    ASSERT_FLOAT_EQ(value, 2.5f);

    TestFloat32Double dst{0.0};
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_DOUBLE_EQ(dst.field, 2.5);
}

// Test marshalling and unmarshalling of float with bool field
TEST(TestDataDefinition, TestFloat32_Bool) {
    struct TestFloat32Bool { bool field; };
    SimConnect::DataDefinition<TestFloat32Bool> def;
    def.addFloat32(&TestFloat32Bool::field, "var", "unit");

    TestFloat32Bool src{true};
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);
    auto data = builder.dataBlock();
    float value = 0.0f;
    std::memcpy(&value, data.data(), sizeof(float));
    ASSERT_FLOAT_EQ(value, 1.0f);

    TestFloat32Bool dst{false};
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_TRUE(dst.field);
}

// Test marshalling and unmarshalling of float with string field
TEST(TestDataDefinition, TestFloat32_String) {
    struct TestFloat32String { std::string field; };
    SimConnect::DataDefinition<TestFloat32String> def;
    def.addFloat32(&TestFloat32String::field, "var", "unit");

    TestFloat32String src{"77.5"};
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);
    auto data = builder.dataBlock();
    float value = 0.0f;
    std::memcpy(&value, data.data(), sizeof(float));
    ASSERT_FLOAT_EQ(value, 77.5f);

    TestFloat32String dst{""};
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_EQ(dst.field, "77.5");
}
