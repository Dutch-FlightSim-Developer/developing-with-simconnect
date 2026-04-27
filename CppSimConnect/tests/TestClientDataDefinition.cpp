/*
 * Copyright (c) 2026. Bert Laverman
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
#include <numbers>

#include "gtest/gtest.h"

#include <simconnect/simconnect.hpp>
#include <simconnect/data/client_data_definition.hpp>
#include <simconnect/data/data_block_builder.hpp>
#include <simconnect/data/data_block_reader.hpp>

// We want all our structs to be 4-byte aligned, which is the alignment SimConnect expects.
#pragma pack(push, 4)

// NOLINTBEGIN(readability-function-cognitive-complexity)

// ---------------------------------------------------------------------------
// sizeOf helper
// ---------------------------------------------------------------------------

TEST(TestClientDataDefinition, SizeOfInt8)    { EXPECT_EQ(SimConnect::sizeOf(SimConnect::ClientDataType::int8),    sizeof(int8_t));  }
TEST(TestClientDataDefinition, SizeOfInt16)   { EXPECT_EQ(SimConnect::sizeOf(SimConnect::ClientDataType::int16),   sizeof(int16_t)); }
TEST(TestClientDataDefinition, SizeOfInt32)   { EXPECT_EQ(SimConnect::sizeOf(SimConnect::ClientDataType::int32),   sizeof(int32_t)); }
TEST(TestClientDataDefinition, SizeOfInt64)   { EXPECT_EQ(SimConnect::sizeOf(SimConnect::ClientDataType::int64),   sizeof(int64_t)); }
TEST(TestClientDataDefinition, SizeOfFloat32) { EXPECT_EQ(SimConnect::sizeOf(SimConnect::ClientDataType::float32), sizeof(float));   }
TEST(TestClientDataDefinition, SizeOfFloat64) { EXPECT_EQ(SimConnect::sizeOf(SimConnect::ClientDataType::float64), sizeof(double));  }

// ---------------------------------------------------------------------------
// Initial state
// ---------------------------------------------------------------------------

TEST(TestClientDataDefinition, InitialState) {
    struct Data { int32_t value; };
    const SimConnect::ClientDataDefinition<Data> def;

    EXPECT_FALSE(def.isDefined());
    EXPECT_TRUE(def.useMapping());
    EXPECT_EQ(def.size(), 0U);
}

// ---------------------------------------------------------------------------
// addRaw
// ---------------------------------------------------------------------------

TEST(TestClientDataDefinition, AddRaw_UseMappingTrueAndCorrectSize) {
    struct Data { int32_t a; float b; };
    SimConnect::ClientDataDefinition<Data> def;
    def.addRaw();

    EXPECT_TRUE(def.useMapping());
    EXPECT_EQ(def.size(), sizeof(Data));
}

TEST(TestClientDataDefinition, AddRaw_Twice_UseMappingFalse) {
    struct Data { int32_t value; };
    SimConnect::ClientDataDefinition<Data> def;
    def.addRaw().addRaw();

    EXPECT_FALSE(def.useMapping());
    EXPECT_EQ(def.size(), 2 * sizeof(Data));
}

TEST(TestClientDataDefinition, AddRaw_AfterTypedField_UseMappingStaysFalse) {
    struct Data { int32_t a; float b; };
    SimConnect::ClientDataDefinition<Data> def;
    def.addInt32(&Data::a).addRaw();

    EXPECT_FALSE(def.useMapping());
}

TEST(TestClientDataDefinition, AddRaw_RoundTrip) {
    struct Data { int32_t a; float b; double c; };
    SimConnect::ClientDataDefinition<Data> def;
    def.addRaw();

    const Data src{ .a = 42, .b = 3.14F, .c = std::numbers::e };
    SimConnect::Data::DataBlockBuilder builder;
    def.build(builder, src);

    auto reader = SimConnect::Data::DataBlockReader(builder.dataBlock());
    Data dst{};
    def.fill(dst, reader);

    EXPECT_EQ(dst.a, src.a);
    EXPECT_FLOAT_EQ(dst.b, src.b);
    EXPECT_DOUBLE_EQ(dst.c, src.c);
}

// ---------------------------------------------------------------------------
// addInt8 — member pointer
// ---------------------------------------------------------------------------

TEST(TestClientDataDefinition, AddInt8_MemberPtr_UseMappingFalseAndCorrectSize) {
    struct Data { int8_t value; };
    SimConnect::ClientDataDefinition<Data> def;
    def.addInt8(&Data::value);

    EXPECT_FALSE(def.useMapping());
    EXPECT_EQ(def.size(), sizeof(int8_t));
}

TEST(TestClientDataDefinition, AddInt8_MemberPtr_RoundTrip) {
    struct Data { int8_t value; };
    SimConnect::ClientDataDefinition<Data> def;
    def.addInt8(&Data::value);

    const Data src{ .value = -42 };
    SimConnect::Data::DataBlockBuilder builder;
    def.build(builder, src);

    auto reader = SimConnect::Data::DataBlockReader(builder.dataBlock());
    Data dst{};
    def.fill(dst, reader);

    EXPECT_EQ(dst.value, src.value);
}

// ---------------------------------------------------------------------------
// addInt16 — member pointer
// ---------------------------------------------------------------------------

TEST(TestClientDataDefinition, AddInt16_MemberPtr_UseMappingFalseAndCorrectSize) {
    struct Data { int16_t value; };
    SimConnect::ClientDataDefinition<Data> def;
    def.addInt16(&Data::value);

    EXPECT_FALSE(def.useMapping());
    EXPECT_EQ(def.size(), sizeof(int16_t));
}

TEST(TestClientDataDefinition, AddInt16_MemberPtr_RoundTrip) {
    struct Data { int16_t value; };
    SimConnect::ClientDataDefinition<Data> def;
    def.addInt16(&Data::value);

    const Data src{ .value = -1000 };
    SimConnect::Data::DataBlockBuilder builder;
    def.build(builder, src);

    auto reader = SimConnect::Data::DataBlockReader(builder.dataBlock());
    Data dst{};
    def.fill(dst, reader);

    EXPECT_EQ(dst.value, src.value);
}

// ---------------------------------------------------------------------------
// addInt32 — member pointer
// ---------------------------------------------------------------------------

TEST(TestClientDataDefinition, AddInt32_MemberPtr_UseMappingFalseAndCorrectSize) {
    struct Data { int32_t value; };
    SimConnect::ClientDataDefinition<Data> def;
    def.addInt32(&Data::value);

    EXPECT_FALSE(def.useMapping());
    EXPECT_EQ(def.size(), sizeof(int32_t));
}

TEST(TestClientDataDefinition, AddInt32_MemberPtr_RoundTrip) {
    struct Data { int32_t value; };
    SimConnect::ClientDataDefinition<Data> def;
    def.addInt32(&Data::value);

    const Data src{ .value = 12345678 };
    SimConnect::Data::DataBlockBuilder builder;
    def.build(builder, src);

    auto reader = SimConnect::Data::DataBlockReader(builder.dataBlock());
    Data dst{};
    def.fill(dst, reader);

    EXPECT_EQ(dst.value, src.value);
}

// ---------------------------------------------------------------------------
// addInt64 — member pointer
// ---------------------------------------------------------------------------

TEST(TestClientDataDefinition, AddInt64_MemberPtr_UseMappingFalseAndCorrectSize) {
    struct Data { int64_t value; };
    SimConnect::ClientDataDefinition<Data> def;
    def.addInt64(&Data::value);

    EXPECT_FALSE(def.useMapping());
    EXPECT_EQ(def.size(), sizeof(int64_t));
}

TEST(TestClientDataDefinition, AddInt64_MemberPtr_RoundTrip) {
    struct Data { int64_t value; };
    SimConnect::ClientDataDefinition<Data> def;
    def.addInt64(&Data::value);

    const Data src{ .value = 0x123456789abcdef0LL };
    SimConnect::Data::DataBlockBuilder builder;
    def.build(builder, src);

    auto reader = SimConnect::Data::DataBlockReader(builder.dataBlock());
    Data dst{};
    def.fill(dst, reader);

    EXPECT_EQ(dst.value, src.value);
}

// ---------------------------------------------------------------------------
// addFloat32 — member pointer
// ---------------------------------------------------------------------------

TEST(TestClientDataDefinition, AddFloat32_MemberPtr_UseMappingFalseAndCorrectSize) {
    struct Data { float value; };
    SimConnect::ClientDataDefinition<Data> def;
    def.addFloat32(&Data::value);

    EXPECT_FALSE(def.useMapping());
    EXPECT_EQ(def.size(), sizeof(float));
}

TEST(TestClientDataDefinition, AddFloat32_MemberPtr_RoundTrip) {
    struct Data { float value; };
    SimConnect::ClientDataDefinition<Data> def;
    def.addFloat32(&Data::value);

    const Data src{ .value = std::numbers::pi_v<float> };
    SimConnect::Data::DataBlockBuilder builder;
    def.build(builder, src);

    auto reader = SimConnect::Data::DataBlockReader(builder.dataBlock());
    Data dst{};
    def.fill(dst, reader);

    EXPECT_FLOAT_EQ(dst.value, src.value);
}

// ---------------------------------------------------------------------------
// addFloat64 — member pointer
// ---------------------------------------------------------------------------

TEST(TestClientDataDefinition, AddFloat64_MemberPtr_UseMappingFalseAndCorrectSize) {
    struct Data { double value; };
    SimConnect::ClientDataDefinition<Data> def;
    def.addFloat64(&Data::value);

    EXPECT_FALSE(def.useMapping());
    EXPECT_EQ(def.size(), sizeof(double));
}

TEST(TestClientDataDefinition, AddFloat64_MemberPtr_RoundTrip) {
    struct Data { double value; };
    SimConnect::ClientDataDefinition<Data> def;
    def.addFloat64(&Data::value);

    const Data src{ .value = std::numbers::e };
    SimConnect::Data::DataBlockBuilder builder;
    def.build(builder, src);

    auto reader = SimConnect::Data::DataBlockReader(builder.dataBlock());
    Data dst{};
    def.fill(dst, reader);

    EXPECT_DOUBLE_EQ(dst.value, src.value);
}

// ---------------------------------------------------------------------------
// Multiple typed fields — size accumulation and round-trip
// ---------------------------------------------------------------------------

TEST(TestClientDataDefinition, MultipleTypedFields_SizeAccumulation) {
    struct Data { int32_t a; float b; double c; };
    SimConnect::ClientDataDefinition<Data> def;
    def.addInt32(&Data::a).addFloat32(&Data::b).addFloat64(&Data::c);

    EXPECT_EQ(def.size(), sizeof(int32_t) + sizeof(float) + sizeof(double));
}

TEST(TestClientDataDefinition, MultipleTypedFields_RoundTrip) {
    struct Data { int32_t a; float b; double c; int8_t d; int16_t e; int64_t f; };
    SimConnect::ClientDataDefinition<Data> def;
    def.addInt32(&Data::a)
       .addFloat32(&Data::b)
       .addFloat64(&Data::c)
       .addInt8(&Data::d)
       .addInt16(&Data::e)
       .addInt64(&Data::f);

    const Data src{ .a = -7, .b = 1.5F, .c = 9.81, .d = 99, .e = -500, .f = 0x0EADBEEFCAFELL };
    SimConnect::Data::DataBlockBuilder builder;
    def.build(builder, src);

    auto reader = SimConnect::Data::DataBlockReader(builder.dataBlock());
    Data dst{};
    def.fill(dst, reader);

    EXPECT_EQ(dst.a, src.a);
    EXPECT_FLOAT_EQ(dst.b, src.b);
    EXPECT_DOUBLE_EQ(dst.c, src.c);
    EXPECT_EQ(dst.d, src.d);
    EXPECT_EQ(dst.e, src.e);
    EXPECT_EQ(dst.f, src.f);
}

// ---------------------------------------------------------------------------
// Stateless lambda setters/getters
// ---------------------------------------------------------------------------

TEST(TestClientDataDefinition, StatelessLambdas_RoundTrip) {
    struct Data { int32_t count; double speed; };
    constexpr int32_t countValue = 55;
    constexpr double speedValue = 123.456;
    Data external{ .count = countValue, .speed = speedValue };

    SimConnect::ClientDataDefinition<Data> def;
    def.addInt32(
        [&external](int32_t v) { external.count = v; },
        [&external]() -> int32_t { return external.count; }
    ).addFloat64(
        [&external](double v) { external.speed = v; },
        [&external]() -> double { return external.speed; }
    );

    // build() reads from the lambdas (ignores the dummy struct)
    Data dummy{};
    SimConnect::Data::DataBlockBuilder builder;
    def.build(builder, dummy);

    // reset state, then fill() should restore it via lambdas
    external.count = 0;
    external.speed = 0.0;

    auto reader = SimConnect::Data::DataBlockReader(builder.dataBlock());
    def.fill(dummy, reader);

    EXPECT_EQ(external.count, countValue);
    EXPECT_DOUBLE_EQ(external.speed, speedValue);
}

// ---------------------------------------------------------------------------
// Struct-based lambda setters/getters
// ---------------------------------------------------------------------------

TEST(TestClientDataDefinition, StructLambdas_RoundTrip) {
    struct Data { int32_t altitude; double latitude; };
    SimConnect::ClientDataDefinition<Data> def;

    // setter multiplies by 10, getter divides by 10 — value is preserved end-to-end
    constexpr int factor = 10;
    def.addInt32(
        [](Data& data, int32_t value) { data.altitude = value * factor; },
        [](const Data& data) -> int32_t { return data.altitude / factor; }
    ).addFloat64(
        [](Data& data, double value) { data.latitude = value; },
        [](const Data& data) -> double { return data.latitude; }
    );

    const int32_t altitudeValue = 10000;
    const double latitudeValue = 52.383917;
    const Data src{ .altitude = altitudeValue, .latitude = latitudeValue };
    SimConnect::Data::DataBlockBuilder builder;
    def.build(builder, src);   // writes altitude/10 = 1000

    auto reader = SimConnect::Data::DataBlockReader(builder.dataBlock());
    Data dst{};
    def.fill(dst, reader);     // reads 1000, stores 1000*10 = 10000

    EXPECT_EQ(dst.altitude, src.altitude);
    EXPECT_DOUBLE_EQ(dst.latitude, src.latitude);
}

// NOLINTEND(readability-function-cognitive-complexity)

#pragma pack(pop)
