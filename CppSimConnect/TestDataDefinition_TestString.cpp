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


 // Test marshalling and unmarshalling of string8
TEST(TestDataDefinition, TestString8_1) {
    struct TestString8 {
        std::string field;
    };
    SimConnect::DataDefinition<TestString8> def;
    def.addString8(&TestString8::field, "var");

    ASSERT_FALSE(def.useMapping());
    ASSERT_EQ(def.size(), 8);

    // Marshalling
    TestString8 src{ "abc" };
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);
    auto data = builder.dataBlock();
    ASSERT_EQ(data.size(), 8);

    // Verify marshalled value
    std::string value(reinterpret_cast<const char*>(data.data()), 3);
    ASSERT_EQ(value, "abc");

    // Unmarshalling
    TestString8 dst{ "" };
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_EQ(dst.field, "abc");
}

// Test marshalling and unmarshalling of string32
TEST(TestDataDefinition, TestString32_1) {
    struct TestString32 {
        std::string field;
    };
    SimConnect::DataDefinition<TestString32> def;
    def.addString32(&TestString32::field, "var");

    ASSERT_FALSE(def.useMapping());
    ASSERT_EQ(def.size(), 32);

    // Marshalling
    TestString32 src{ "hello world" };
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);
    auto data = builder.dataBlock();
    ASSERT_EQ(data.size(), 32);

    // Verify marshalled value
    std::string value(reinterpret_cast<const char*>(data.data()), 11);
    ASSERT_EQ(value, "hello world");

    // Unmarshalling
    TestString32 dst{ "" };
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_EQ(dst.field, "hello world");
}

// Test marshalling and unmarshalling of string64
TEST(TestDataDefinition, TestString64_1) {
    struct TestString64 {
        std::string field;
    };
    SimConnect::DataDefinition<TestString64> def;
    def.addString64(&TestString64::field, "var");

    ASSERT_FALSE(def.useMapping());
    ASSERT_EQ(def.size(), 64);

    // Marshalling
    TestString64 src{ "test string 64" };
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);
    auto data = builder.dataBlock();
    ASSERT_EQ(data.size(), 64);

    // Verify marshalled value
    std::string value(reinterpret_cast<const char*>(data.data()), 14);
    ASSERT_EQ(value, "test string 64");

    // Unmarshalling
    TestString64 dst{ "" };
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_EQ(dst.field, "test string 64");
}

// Test marshalling and unmarshalling of string128
TEST(TestDataDefinition, TestString128_1) {
    struct TestString128 {
        std::string field;
    };
    SimConnect::DataDefinition<TestString128> def;
    def.addString128(&TestString128::field, "var");

    ASSERT_FALSE(def.useMapping());
    ASSERT_EQ(def.size(), 128);

    // Marshalling
    TestString128 src{ "test string 128" };
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);
    auto data = builder.dataBlock();
    ASSERT_EQ(data.size(), 128);

    // Verify marshalled value
    std::string value(reinterpret_cast<const char*>(data.data()), 15);
    ASSERT_EQ(value, "test string 128");

    // Unmarshalling
    TestString128 dst{ "" };
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_EQ(dst.field, "test string 128");
}

// Test marshalling and unmarshalling of string256
TEST(TestDataDefinition, TestString256_1) {
    struct TestString256 {
        std::string field;
    };
    SimConnect::DataDefinition<TestString256> def;
    def.addString256(&TestString256::field, "var");

    ASSERT_FALSE(def.useMapping());
    ASSERT_EQ(def.size(), 256);

    // Marshalling
    TestString256 src{ "test string 256" };
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);
    auto data = builder.dataBlock();
    ASSERT_EQ(data.size(), 256);

    // Verify marshalled value
    std::string value(reinterpret_cast<const char*>(data.data()), 15);
    ASSERT_EQ(value, "test string 256");

    // Unmarshalling
    TestString256 dst{ "" };
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_EQ(dst.field, "test string 256");
}

// Test marshalling and unmarshalling of string260
TEST(TestDataDefinition, TestString260_1) {
    struct TestString260 {
        std::string field;
    };
    SimConnect::DataDefinition<TestString260> def;
    def.addString260(&TestString260::field, "var");

    ASSERT_FALSE(def.useMapping());
    ASSERT_EQ(def.size(), 260);

    // Marshalling
    TestString260 src{ "test string 260" };
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);
    auto data = builder.dataBlock();
    ASSERT_EQ(data.size(), 260);

    // Verify marshalled value
    std::string value(reinterpret_cast<const char*>(data.data()), 15);
    ASSERT_EQ(value, "test string 260");

    // Unmarshalling
    TestString260 dst{ "" };
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_EQ(dst.field, "test string 260");
}

// Test marshalling and unmarshalling of stringV
TEST(TestDataDefinition, TestStringV_1) {
    struct TestStringV {
        std::string field;
    };
    SimConnect::DataDefinition<TestStringV> def;
    def.addStringV(&TestStringV::field, "var");

    ASSERT_FALSE(def.useMapping());
    ASSERT_GE(def.size(), 4); // StringV is variable length, min 4 bytes

    // Marshalling
    TestStringV src{ "test string V" };
    SimConnect::Data::DataBlockBuilder builder;
    def.marshall(builder, src);
    auto data = builder.dataBlock();
    ASSERT_GE(data.size(), 13u); // at least the string length + null

    // Verify marshalled value
    std::string value(reinterpret_cast<const char*>(data.data()), 13);
    ASSERT_EQ(value, "test string V");

    // Unmarshalling
    TestStringV dst{ "" };
    SimConnect::Data::DataBlockReader reader(data);
    def.unmarshall(reader, dst);
    ASSERT_EQ(dst.field, "test string V");
}
